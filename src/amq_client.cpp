#include "amq_client.h"
#include "logger.h"
#include <stdexcept>

namespace sar_atr {

AMQClient::AMQClient() : connected_(false) {
    client_.init_asio();
    client_.set_access_channels(websocketpp::log::alevel::none);
    client_.set_error_channels(websocketpp::log::elevel::none);
    
    client_.set_open_handler([this](websocketpp::connection_hdl hdl) {
        this->onOpen(hdl);
    });
    
    client_.set_message_handler([this](websocketpp::connection_hdl hdl, WebSocketClient::message_ptr msg) {
        this->onMessage(hdl, msg);
    });
    
    client_.set_close_handler([this](websocketpp::connection_hdl hdl) {
        this->onClose(hdl);
    });
    
    client_.set_fail_handler([this](websocketpp::connection_hdl hdl) {
        this->onFail(hdl);
    });
}

AMQClient::~AMQClient() {
    disconnect();
}

void AMQClient::connect(const std::string& broker_address) {
    Logger::info("Connecting to AMQ broker: " + broker_address);
    
    try {
        websocketpp::lib::error_code ec;
        WebSocketClient::connection_ptr con = client_.get_connection(broker_address, ec);
        
        if (ec) {
            throw std::runtime_error("Failed to create connection: " + ec.message());
        }
        
        connection_hdl_ = con->get_handle();
        client_.connect(con);
        
        // Start the client run loop in a separate thread
        run_thread_ = std::thread([this]() {
            try {
                client_.run();
            } catch (const std::exception& e) {
                Logger::error("WebSocket client error: " + std::string(e.what()));
            }
        });
        
        // Wait for connection to establish (10 seconds max)
        int retries = 100;
        while (!connected_ && retries-- > 0) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
        
        if (!connected_) {
            throw std::runtime_error("Connection timeout");
        }
        
    } catch (const std::exception& e) {
        Logger::error("Connection failed: " + std::string(e.what()));
        throw;
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
        client_.send(connection_hdl_, subscribe_frame, websocketpp::frame::opcode::text);
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
        client_.send(connection_hdl_, send_frame, websocketpp::frame::opcode::text);
    } catch (const std::exception& e) {
        Logger::error("Failed to publish message: " + std::string(e.what()));
        throw;
    }
}

void AMQClient::disconnect() {
    if (connected_) {
        Logger::info("Disconnecting from AMQ broker");
        connected_ = false;
        
        try {
            client_.close(connection_hdl_, websocketpp::close::status::normal, "");
        } catch (...) {
            // Ignore errors during disconnect
        }
    }
    
    if (run_thread_.joinable()) {
        client_.stop();
        run_thread_.join();
    }
}

bool AMQClient::isConnected() const {
    return connected_;
}

void AMQClient::run() {
    client_.run();
}

void AMQClient::onOpen(websocketpp::connection_hdl hdl) {
    Logger::info("WebSocket connection established");
    
    // Send STOMP CONNECT frame
    std::string connect_frame = "CONNECT\n";
    connect_frame += "accept-version:1.2\n";
    connect_frame += "host:/\n\n";
    connect_frame += '\0';
    
    try {
        client_.send(hdl, connect_frame, websocketpp::frame::opcode::text);
        connected_ = true;
        Logger::info("STOMP handshake initiated");
    } catch (const std::exception& e) {
        Logger::error("Failed to send CONNECT frame: " + std::string(e.what()));
    }
}

void AMQClient::onMessage(websocketpp::connection_hdl hdl, WebSocketClient::message_ptr msg) {
    std::string payload = msg->get_payload();
    
    // Parse STOMP frame
    if (payload.find("CONNECTED") == 0) {
        Logger::info("STOMP connection confirmed");
        return;
    }
    
    if (payload.find("MESSAGE") == 0) {
        // Extract message body from STOMP frame
        size_t body_start = payload.find("\n\n");
        if (body_start != std::string::npos) {
            std::string body = payload.substr(body_start + 2);
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

void AMQClient::onClose(websocketpp::connection_hdl hdl) {
    Logger::info("WebSocket connection closed");
    connected_ = false;
}

void AMQClient::onFail(websocketpp::connection_hdl hdl) {
    Logger::error("WebSocket connection failed");
    connected_ = false;
}

} // namespace sar_atr
