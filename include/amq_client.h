#ifndef AMQ_CLIENT_H
#define AMQ_CLIENT_H

#include <string>
#include <functional>
#include <thread>
#include <atomic>
#include <mutex>
#include <queue>

namespace sar_atr {

typedef std::function<void(const std::string&)> MessageCallback;

/**
 * @class AMQClient
 * @brief WebSocket client for ActiveMQ message broker communication
 * 
 * Handles connection, subscription, and publishing to AMQ topics over WebSocket
 */
class AMQClient {
public:
    AMQClient();
    ~AMQClient();
    
    /**
     * @brief Connect to the AMQ broker
     * @param broker_address WebSocket address (e.g., ws://localhost:9000)
     */
    void connect(const std::string& broker_address);
    
    /**
     * @brief Subscribe to a topic and set message callback
     * @param topic Topic name to subscribe to
     * @param callback Function to call when messages arrive
     */
    void subscribe(const std::string& topic, MessageCallback callback);
    
    /**
     * @brief Publish a message to a topic
     * @param topic Topic name to publish to
     * @param message Message content
     */
    void publish(const std::string& topic, const std::string& message);
    
    /**
     * @brief Disconnect from the broker
     */
    void disconnect();
    
    /**
     * @brief Check if client is connected
     */
    bool isConnected() const;
    
    /**
     * @brief Run the client event loop (blocking)
     */
    void run();
    
private:
    int socket_fd_;
    std::atomic<bool> connected_;
    std::atomic<bool> running_;
    std::thread run_thread_;
    std::thread receive_thread_;
    
    std::string host_;
    int port_;
    std::string path_;
    
    MessageCallback message_callback_;
    
    std::mutex send_mutex_;
    std::queue<std::string> send_queue_;
    
    // WebSocket functions
    bool connectSocket(const std::string& broker_address);
    bool performWebSocketHandshake();
    void receiveLoop();
    void sendFrame(const std::string& data);
    std::string receiveFrame();
    void parseStompMessage(const std::string& message);
    std::string createWebSocketFrame(const std::string& data);
    bool parseWebSocketFrame(const std::string& data, std::string& payload);
};

} // namespace sar_atr

#endif // AMQ_CLIENT_H
