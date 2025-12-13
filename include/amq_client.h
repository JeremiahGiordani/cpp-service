#ifndef AMQ_CLIENT_H
#define AMQ_CLIENT_H

#include <string>
#include <functional>
#include <websocketpp/config/asio_no_tls_client.hpp>
#include <websocketpp/client.hpp>
#include <thread>
#include <atomic>

namespace sar_atr {

typedef websocketpp::client<websocketpp::config::asio_client> WebSocketClient;
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
    WebSocketClient client_;
    websocketpp::connection_hdl connection_hdl_;
    std::atomic<bool> connected_;
    std::thread run_thread_;
    
    MessageCallback message_callback_;
    
    void onOpen(websocketpp::connection_hdl hdl);
    void onMessage(websocketpp::connection_hdl hdl, WebSocketClient::message_ptr msg);
    void onClose(websocketpp::connection_hdl hdl);
    void onFail(websocketpp::connection_hdl hdl);
};

} // namespace sar_atr

#endif // AMQ_CLIENT_H
