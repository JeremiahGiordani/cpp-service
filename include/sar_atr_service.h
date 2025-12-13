#ifndef SAR_ATR_SERVICE_H
#define SAR_ATR_SERVICE_H

#include "amq_client.h"
#include "config_manager.h"
#include "inference_engine.h"
#include "uci_messages.h"
#include <memory>
#include <atomic>

namespace sar_atr {

/**
 * @class SarAtrService
 * @brief Main service orchestrator for SAR ATR UCI processing
 * 
 * Coordinates between AMQ messaging, inference engine, and UCI message handling
 */
class SarAtrService {
public:
    /**
     * @brief Constructor
     * @param config Service configuration
     * @param inference_engine Pointer to inference engine implementation
     */
    SarAtrService(const ServiceConfig& config, 
                  std::shared_ptr<InferenceEngine> inference_engine);
    
    /**
     * @brief Initialize and start the service
     */
    void start();
    
    /**
     * @brief Stop the service
     */
    void stop();
    
    /**
     * @brief Check if service is running
     */
    bool isRunning() const;
    
private:
    ServiceConfig config_;
    std::shared_ptr<InferenceEngine> inference_engine_;
    std::unique_ptr<AMQClient> amq_client_;
    std::atomic<bool> running_;
    SystemInfo system_info_;
    
    /**
     * @brief Handle incoming FileLocation UCI messages
     */
    void handleFileLocationMessage(const std::string& message);
    
    /**
     * @brief Process detections and publish UCI messages
     */
    void processAndPublishResults(const std::string& nitf_path,
                                   const std::vector<DetectionResult>& detections);
    
    /**
     * @brief Calculate and log bandwidth savings from chip-based transmission
     */
    void calculateBandwidthSavings(const std::string& nitf_path,
                                    const std::vector<DetectionResult>& detections,
                                    int published_count);
};

} // namespace sar_atr

#endif // SAR_ATR_SERVICE_H
