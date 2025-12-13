#include "sar_atr_service.h"
#include "logger.h"
#include <chrono>
#include <sstream>

namespace sar_atr {

SarAtrService::SarAtrService(const ServiceConfig& config,
                             std::shared_ptr<InferenceEngine> inference_engine)
    : config_(config),
      inference_engine_(inference_engine),
      running_(false) {
    
    // Set up system info from config
    system_info_.system_uuid = config.system_uuid;
    system_info_.system_description = config.system_description;
    system_info_.service_version = config.service_version;
    
    // Create AMQ client
    amq_client_ = std::make_unique<AMQClient>();
}

void SarAtrService::start() {
    Logger::info("========================================");
    Logger::info("Starting SAR ATR UCI Service");
    Logger::info("========================================");
    Logger::info("Service Version: " + config_.service_version);
    Logger::info("System UUID: " + config_.system_uuid);
    Logger::info("Confidence Threshold: " + std::to_string(config_.confidence_threshold));
    
    try {
        // Connect to broker
        amq_client_->connect(config_.broker_address);
        Logger::info("Connected to message broker");
        
        // Subscribe to FileLocation_uci
        Logger::info("Subscribing to FileLocation_uci topic");
        amq_client_->subscribe("FileLocation_uci", 
            [this](const std::string& message) {
                this->handleFileLocationMessage(message);
            });
        
        running_ = true;
        
        Logger::info("========================================");
        Logger::info("Service initialized and ready");
        Logger::info("========================================");
        
        // Keep service running
        while (running_) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
        
    } catch (const std::exception& e) {
        Logger::error("Failed to start service: " + std::string(e.what()));
        throw;
    }
}

void SarAtrService::stop() {
    Logger::info("Stopping SAR ATR service");
    running_ = false;
    
    if (amq_client_) {
        amq_client_->disconnect();
    }
    
    Logger::info("Service stopped");
}

bool SarAtrService::isRunning() const {
    return running_;
}

void SarAtrService::handleFileLocationMessage(const std::string& message) {
    Logger::info("========================================");
    Logger::info("Received FileLocation_uci message");
    
    try {
        // Parse the message to extract file path
        std::string nitf_path = parseFileLocationMessage(message);
        Logger::info("Extracted NITF file path: " + nitf_path);
        
        // Process with inference engine
        Logger::info("Passing file to SAR ATR inference engine");
        auto start_time = std::chrono::high_resolution_clock::now();
        
        std::vector<DetectionResult> detections = inference_engine_->process(nitf_path);
        
        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
            end_time - start_time);
        
        Logger::info("Inference completed in " + std::to_string(duration.count()) + " ms");
        Logger::info("Received " + std::to_string(detections.size()) + " detections from inference engine");
        
        // Process and publish results
        processAndPublishResults(nitf_path, detections);
        
    } catch (const std::exception& e) {
        Logger::error("Error processing FileLocation message: " + std::string(e.what()));
    }
    
    Logger::info("========================================");
}

void SarAtrService::processAndPublishResults(const std::string& nitf_path,
                                              const std::vector<DetectionResult>& detections) {
    std::vector<std::string> entity_uuids;
    int published_count = 0;
    int filtered_count = 0;
    
    // Process each detection
    for (const auto& detection : detections) {
        std::stringstream ss;
        ss << "Detection: " << detection.classification 
           << " (confidence: " << std::fixed << std::setprecision(3) << detection.confidence << ")";
        
        // Check confidence threshold
        if (detection.confidence >= config_.confidence_threshold) {
            Logger::info(ss.str() + " - Publishing");
            
            try {
                // Create and send Entity message
                std::string entity_msg = createEntityMessage(detection, system_info_);
                
                // Extract the entity UUID from the message (we need it for AtrProcessingResult)
                Json::Value root;
                Json::CharReaderBuilder builder;
                std::string errs;
                std::istringstream iss(entity_msg);
                if (Json::parseFromStream(builder, iss, &root, &errs)) {
                    std::string entity_uuid = root["Entity"]["MessageData"]["EntityID"]["UUID"].asString();
                    entity_uuids.push_back(entity_uuid);
                }
                
                amq_client_->publish("Entity_uci", entity_msg);
                Logger::info("Published Entity_uci message for " + detection.classification);
                published_count++;
                
            } catch (const std::exception& e) {
                Logger::error("Failed to publish Entity message: " + std::string(e.what()));
            }
        } else {
            Logger::info(ss.str() + " - Below confidence threshold, not publishing");
            filtered_count++;
        }
    }
    
    // Publish AtrProcessingResult if we have any entities
    if (!entity_uuids.empty()) {
        try {
            std::string atr_result_msg = createAtrProcessingResultMessage(entity_uuids);
            amq_client_->publish("AtrProcessingResult_uci", atr_result_msg);
            Logger::info("Published AtrProcessingResult_uci message with " + 
                        std::to_string(entity_uuids.size()) + " entity references");
        } catch (const std::exception& e) {
            Logger::error("Failed to publish AtrProcessingResult message: " + 
                         std::string(e.what()));
        }
    }
    
    // Summary
    Logger::info("Processing summary:");
    Logger::info("  Total detections: " + std::to_string(detections.size()));
    Logger::info("  Published: " + std::to_string(published_count));
    Logger::info("  Filtered (below threshold): " + std::to_string(filtered_count));
}

} // namespace sar_atr
