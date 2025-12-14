#include "amq_client.h"
#include "logger.h"
#include <stdexcept>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <cstring>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <cctype>

namespace sar_atr {

AMQClient::AMQClient() : socket_fd_(-1), connected_(false), running_(false), port_(0) {
}

AMQClient::~AMQClient() {
    disconnect();
}

bool AMQClient::connectSocket(const std::string& broker_address) {
    // Parse WebSocket URL (ws://host:port/path)
    size_t pos = broker_address.find("://");
    if (pos == std::string::npos) {
        Logger::error("Invalid WebSocket URL format");
        return false;
    }
    
    std::string url = broker_address.substr(pos + 3);
    pos = url.find(':');
    if (pos == std::string::npos) {
        Logger::error("Port not specified in URL");
        return false;
    }
    
    host_ = url.substr(0, pos);
    url = url.substr(pos + 1);
    
    pos = url.find('/');
    if (pos != std::string::npos) {
        port_ = std::stoi(url.substr(0, pos));
        path_ = url.substr(pos);
    } else {
        port_ = std::stoi(url);
        path_ = "/";
    }
    
    Logger::info("Connecting to " + host_ + ":" + std::to_string(port_) + path_);
    
    // Resolve hostname
    struct hostent* server = gethostbyname(host_.c_str());
    if (server == nullptr) {
        Logger::error("Failed to resolve hostname: " + host_);
        return false;
    }
    
    // Create socket
    socket_fd_ = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_fd_ < 0) {
        Logger::error("Failed to create socket");
        return false;
    }
    
    // Connect to server
    struct sockaddr_in serv_addr;
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    memcpy(&serv_addr.sin_addr.s_addr, server->h_addr, server->h_length);
    serv_addr.sin_port = htons(port_);
    
    if (::connect(socket_fd_, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
        Logger::error("Failed to connect to server");
        close(socket_fd_);
        socket_fd_ = -1;
        return false;
    }
    
    return true;
}

bool AMQClient::performWebSocketHandshake() {
    // Generate WebSocket key
    std::string key = "dGhlIHNhbXBsZSBub25jZQ=="; // Static for simplicity
    
    // Send WebSocket handshake
    std::ostringstream handshake;
    handshake << "GET " << path_ << " HTTP/1.1\r\n";
    handshake << "Host: " << host_ << ":" << port_ << "\r\n";
    handshake << "Upgrade: websocket\r\n";
    handshake << "Connection: Upgrade\r\n";
    handshake << "Sec-WebSocket-Key: " << key << "\r\n";
    handshake << "Sec-WebSocket-Version: 13\r\n";
    handshake << "Sec-WebSocket-Protocol: stomp\r\n";
    handshake << "\r\n";
    
    std::string handshake_str = handshake.str();
    if (send(socket_fd_, handshake_str.c_str(), handshake_str.length(), 0) < 0) {
        Logger::error("Failed to send WebSocket handshake");
        return false;
    }
    
    // Receive handshake response
    char buffer[4096];
    ssize_t received = recv(socket_fd_, buffer, sizeof(buffer) - 1, 0);
    if (received <= 0) {
        Logger::error("Failed to receive WebSocket handshake response");
        return false;
    }
    
    buffer[received] = '\0';
    std::string response(buffer);
    
    // Check for successful upgrade (HTTP 101 Switching Protocols)
    if (response.find("101") == std::string::npos) {
        Logger::error("WebSocket handshake failed: " + response);
        return false;
    }
    
    // Verify it's a WebSocket upgrade (case-insensitive check)
    std::string response_lower = response;
    std::transform(response_lower.begin(), response_lower.end(), response_lower.begin(), ::tolower);
    if (response_lower.find("upgrade:") == std::string::npos || 
        response_lower.find("websocket") == std::string::npos) {
        Logger::error("WebSocket handshake failed: missing upgrade headers");
        return false;
    }
    
    Logger::info("WebSocket handshake successful");
    return true;
}

void AMQClient::connect(const std::string& broker_address) {
    Logger::info("Connecting to AMQ broker: " + broker_address);
    
    try {
        if (!connectSocket(broker_address)) {
            throw std::runtime_error("Failed to connect socket");
        }
        
        if (!performWebSocketHandshake()) {
            close(socket_fd_);
            socket_fd_ = -1;
            throw std::runtime_error("Failed to perform WebSocket handshake");
        }
        
        connected_ = true;
        running_ = true;
        
        // Send STOMP CONNECT frame
        std::string connect_frame = "CONNECT\n";
        connect_frame += "accept-version:1.2\n";
        connect_frame += "host:/\n\n";
        connect_frame += '\0';
        
        sendFrame(connect_frame);
        Logger::info("STOMP handshake initiated");
        
        // Start receive thread
        receive_thread_ = std::thread([this]() {
            receiveLoop();
        });
        
        // Wait a bit for STOMP connection to establish
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        
    } catch (const std::exception& e) {
        Logger::error("Connection failed: " + std::string(e.what()));
        throw;
    }
}

std::string AMQClient::createWebSocketFrame(const std::string& data) {
    std::string frame;
    
    // FIN=1, opcode=1 (text frame)
    frame += static_cast<char>(0x81);
    
    // Mask bit set, payload length
    size_t len = data.length();
    if (len <= 125) {
        frame += static_cast<char>(0x80 | len);
    } else if (len <= 65535) {
        frame += static_cast<char>(0x80 | 126);
        frame += static_cast<char>((len >> 8) & 0xFF);
        frame += static_cast<char>(len & 0xFF);
    } else {
        frame += static_cast<char>(0x80 | 127);
        for (int i = 7; i >= 0; i--) {
            frame += static_cast<char>((len >> (i * 8)) & 0xFF);
        }
    }
    
    // Masking key (simple static key for simplicity)
    unsigned char mask[4] = {0x12, 0x34, 0x56, 0x78};
    frame.append(reinterpret_cast<char*>(mask), 4);
    
    // Masked payload
    for (size_t i = 0; i < len; i++) {
        frame += static_cast<char>(data[i] ^ mask[i % 4]);
    }
    
    return frame;
}

bool AMQClient::parseWebSocketFrame(const std::string& data, std::string& payload) {
    if (data.length() < 2) {
        return false;
    }
    
    size_t pos = 0;
    
    // Parse header
    unsigned char byte1 = data[pos++];
    unsigned char byte2 = data[pos++];
    
    bool fin = (byte1 & 0x80) != 0;
    unsigned char opcode = byte1 & 0x0F;
    bool masked = (byte2 & 0x80) != 0;
    unsigned long long payload_len = byte2 & 0x7F;
    
    // Handle extended payload length
    if (payload_len == 126) {
        if (data.length() < pos + 2) return false;
        payload_len = (static_cast<unsigned char>(data[pos]) << 8) | 
                      static_cast<unsigned char>(data[pos + 1]);
        pos += 2;
    } else if (payload_len == 127) {
        if (data.length() < pos + 8) return false;
        payload_len = 0;
        for (int i = 0; i < 8; i++) {
            payload_len = (payload_len << 8) | static_cast<unsigned char>(data[pos++]);
        }
    }
    
    // Skip mask if present (server shouldn't mask)
    if (masked) {
        pos += 4;
    }
    
    // Extract payload
    if (data.length() < pos + payload_len) {
        return false;
    }
    
    payload = data.substr(pos, payload_len);
    return true;
}

void AMQClient::sendFrame(const std::string& data) {
    if (!connected_ || socket_fd_ < 0) {
        throw std::runtime_error("Cannot send: not connected");
    }
    
    std::lock_guard<std::mutex> lock(send_mutex_);
    
    std::string frame = createWebSocketFrame(data);
    ssize_t sent = send(socket_fd_, frame.c_str(), frame.length(), 0);
    
    if (sent < 0) {
        Logger::error("Failed to send frame");
        throw std::runtime_error("Failed to send frame");
    }
}

void AMQClient::receiveLoop() {
    char buffer[8192];
    std::string accumulated;
    
    while (running_ && connected_) {
        ssize_t received = recv(socket_fd_, buffer, sizeof(buffer), 0);
        
        if (received <= 0) {
            if (received < 0) {
                Logger::error("Receive error");
            }
            connected_ = false;
            break;
        }
        
        accumulated.append(buffer, received);
        
        // Try to parse WebSocket frames (there may be multiple)
        while (true) {
            std::string payload;
            if (parseWebSocketFrame(accumulated, payload)) {
                // Successfully parsed a frame, remove it from accumulated
                // This is a simplified approach - in production you'd want to track bytes consumed
                accumulated.clear();
                parseStompMessage(payload);
            } else {
                // Not enough data yet or incomplete frame
                break;
            }
        }
    }
    
    Logger::info("Receive loop ended");
}

void AMQClient::parseStompMessage(const std::string& message) {
    if (message.find("CONNECTED") == 0) {
        Logger::info("STOMP connection confirmed");
        return;
    }
    
    if (message.find("MESSAGE") == 0) {
        // Extract message body from STOMP frame
        size_t body_start = message.find("\n\n");
        if (body_start != std::string::npos) {
            std::string body = message.substr(body_start + 2);
            // Remove null terminator if present
            if (!body.empty() && body.back() == '\0') {
                body.pop_back();
            }
            
            if (message_callback_) {
                message_callback_(body);
            }
        }
    }
}


void AMQClient::subscribe(const std::string& topic, MessageCallback callback) {
    if (!connected_) {
        throw std::runtime_error("Cannot subscribe: not connected");
    }
    
    Logger::info("Subscribing to topic: " + topic);
    message_callback_ = callback;
    
    // Send STOMP SUBSCRIBE frame
    std::string subscribe_frame = "SUBSCRIBE\n";
    subscribe_frame += "destination:/topic/" + topic + "\n";
    subscribe_frame += "id:sub-0\n";
    subscribe_frame += "ack:auto\n\n";
    subscribe_frame += '\0';
    
    try {
        sendFrame(subscribe_frame);
        Logger::info("Successfully subscribed to: " + topic);
    } catch (const std::exception& e) {
        Logger::error("Failed to subscribe: " + std::string(e.what()));
        throw;
    }
}

void AMQClient::publish(const std::string& topic, const std::string& message) {
    if (!connected_) {
        throw std::runtime_error("Cannot publish: not connected");
    }
    
    // Send STOMP SEND frame
    std::string send_frame = "SEND\n";
    send_frame += "destination:/topic/" + topic + "\n";
    send_frame += "content-type:application/json\n";
    send_frame += "content-length:" + std::to_string(message.length()) + "\n\n";
    send_frame += message;
    send_frame += '\0';
    
    try {
        sendFrame(send_frame);
    } catch (const std::exception& e) {
        Logger::error("Failed to publish message: " + std::string(e.what()));
        throw;
    }
}

void AMQClient::disconnect() {
    if (connected_) {
        Logger::info("Disconnecting from AMQ broker");
        connected_ = false;
        running_ = false;
    }
    
    if (receive_thread_.joinable()) {
        receive_thread_.join();
    }
    
    if (socket_fd_ >= 0) {
        close(socket_fd_);
        socket_fd_ = -1;
    }
}

bool AMQClient::isConnected() const {
    return connected_;
}

void AMQClient::run() {
    // Keep the main thread alive while connected
    while (running_) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

} // namespace sar_atr
