#include "sar_atr_service.h"
#include "logger.h"
#include <chrono>
#include <sstream>
#include <algorithm>
#include <cctype>

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
    
    // Retry connection logic
    const int max_retries = 5;
    const int retry_delay_ms = 2000; // 2 seconds
    
    for (int attempt = 1; attempt <= max_retries; ++attempt) {
        try {
            Logger::info("Connection attempt " + std::to_string(attempt) + " of " + std::to_string(max_retries));
            
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
            
            // If we get here, service stopped normally
            return;
            
        } catch (const std::exception& e) {
            Logger::error("Connection attempt " + std::to_string(attempt) + " failed: " + std::string(e.what()));
            
            if (attempt < max_retries) {
                Logger::info("Retrying in " + std::to_string(retry_delay_ms / 1000) + " seconds...");
                std::this_thread::sleep_for(std::chrono::milliseconds(retry_delay_ms));
            } else {
                Logger::error("Failed to connect after " + std::to_string(max_retries) + " attempts");
                throw std::runtime_error("Failed to start service after multiple connection attempts");
            }
        }
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
        
        Logger::info("========================================");
        Logger::info("Inference Results");
        Logger::info("========================================");
        Logger::info("Total inference time: " + std::to_string(duration.count()) + " ms");
        Logger::info("Total detections found: " + std::to_string(detections.size()));
        
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
    
    Logger::info("========================================");
    Logger::info("Detection Results");
    Logger::info("========================================");
    
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
                Logger::info("  └─ Published Entity_uci message for " + detection.classification);
                published_count++;
                
            } catch (const std::exception& e) {
                Logger::error("Failed to publish Entity message: " + std::string(e.what()));
            }
        } else {
            Logger::info(ss.str() + " - Below threshold, not publishing");
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
    
    // Calculate bandwidth savings
    calculateBandwidthSavings(nitf_path, detections, published_count);
    
    // Summary
    Logger::info("========================================");
    Logger::info("Processing Summary");
    Logger::info("========================================");
    Logger::info("Total detections: " + std::to_string(detections.size()));
    Logger::info("Published: " + std::to_string(published_count));
    Logger::info("Filtered (below threshold): " + std::to_string(filtered_count));
}

void SarAtrService::calculateBandwidthSavings(const std::string& nitf_path,
                                               const std::vector<DetectionResult>& detections, 
                                               int published_count) {
    // Default SAR image dimensions (used as fallback)
    int image_width = 4096;
    int image_height = 4096;
    const int bytes_per_pixel = 2; // 16-bit SAR data
    
    // Try to extract actual dimensions from NITF file path
    // For mock files or if file doesn't exist, we'll use defaults
    bool using_actual_dimensions = false;
    
    // Simple heuristic: try to extract dimensions from filename patterns
    // Real implementation would parse NITF headers, but for demo we'll use heuristics
    size_t last_slash = nitf_path.find_last_of("/\\");
    std::string filename = (last_slash != std::string::npos) ? nitf_path.substr(last_slash + 1) : nitf_path;
    
    // Look for dimension patterns in filename like "2048x2048" or "_4096_4096"
    std::string fname_lower = filename;
    std::transform(fname_lower.begin(), fname_lower.end(), fname_lower.begin(), ::tolower);
    
    // Try pattern: NNNNxNNNN
    size_t x_pos = fname_lower.find('x');
    if (x_pos != std::string::npos && x_pos > 0) {
        // Look backwards for digits
        size_t start = x_pos;
        while (start > 0 && std::isdigit(fname_lower[start - 1])) {
            start--;
        }
        // Look forwards for digits
        size_t end = x_pos + 1;
        while (end < fname_lower.length() && std::isdigit(fname_lower[end])) {
            end++;
        }
        
        if (start < x_pos && end > x_pos + 1) {
            try {
                int w = std::stoi(fname_lower.substr(start, x_pos - start));
                int h = std::stoi(fname_lower.substr(x_pos + 1, end - x_pos - 1));
                if (w > 0 && h > 0 && w < 100000 && h < 100000) {
                    image_width = w;
                    image_height = h;
                    using_actual_dimensions = true;
                }
            } catch (...) {
                // Parsing failed, use defaults
            }
        }
    }
    
    // Calculate original file size
    long long original_pixels = static_cast<long long>(image_width) * image_height;
    long long original_bytes = original_pixels * bytes_per_pixel;
    double original_mb = original_bytes / (1024.0 * 1024.0);
    
    // Calculate actual chip sizes from detections
    long long total_chip_pixels = 0;
    
    for (const auto& detection : detections) {
        // Calculate chip dimensions in pixels from normalized bounding box
        int chip_width = static_cast<int>((detection.bounding_box.x2 - detection.bounding_box.x1) * image_width);
        int chip_height = static_cast<int>((detection.bounding_box.y2 - detection.bounding_box.y1) * image_height);
        
        // Add some padding around detection (typical 20% on each side)
        chip_width = static_cast<int>(chip_width * 1.4);
        chip_height = static_cast<int>(chip_height * 1.4);
        
        // Ensure minimum chip size of 64x64 and maximum of 512x512
        chip_width = std::max(64, std::min(512, chip_width));
        chip_height = std::max(64, std::min(512, chip_height));
        
        long long chip_pixels = static_cast<long long>(chip_width) * chip_height;
        total_chip_pixels += chip_pixels;
    }
    
    // Only count published detections for bandwidth calculation
    // Scale by the ratio of published to total
    if (detections.size() > 0) {
        total_chip_pixels = total_chip_pixels * published_count / detections.size();
    }
    
    long long total_chip_bytes = total_chip_pixels * bytes_per_pixel;
    double chip_mb = total_chip_bytes / (1024.0 * 1024.0);
    
    // Calculate savings
    double saved_mb = original_mb - chip_mb;
    double saved_percent = (saved_mb / original_mb) * 100.0;
    
    Logger::info("========================================");
    Logger::info("Bandwidth Savings Estimate");
    Logger::info("========================================");
    
    std::stringstream ss;
    ss << std::fixed << std::setprecision(2);
    
    // Show if using actual or estimated dimensions
    std::string dim_source = using_actual_dimensions ? " (from filename)" : " (estimated)";
    Logger::info("Original full image: ~" + std::to_string(static_cast<int>(original_mb)) + " MB " +
                 "(" + std::to_string(image_width) + "x" + std::to_string(image_height) + " pixels" + dim_source + ")");
    
    if (published_count > 0) {
        Logger::info("Detections to transmit: " + std::to_string(published_count) + " chips " +
                     "(variable sizes based on actual detections)");
        
        ss.str("");
        ss << "Total chip data: ~" << chip_mb << " MB";
        Logger::info(ss.str());
        
        ss.str("");
        ss << "Data NOT transmitted: ~" << saved_mb << " MB";
        Logger::info(ss.str());
        
        ss.str("");
        ss << "Bandwidth savings: " << saved_percent << "%";
        Logger::info(ss.str());
        
        if (saved_percent > 95.0) {
            Logger::info("  └─ Excellent bandwidth optimization!");
        } else if (saved_percent > 80.0) {
            Logger::info("  └─ Good bandwidth savings");
        } else if (saved_percent > 50.0) {
            Logger::info("  └─ Moderate bandwidth savings");
        } else {
            Logger::info("  └─ Limited bandwidth savings (large detections)");
        }
    } else {
        Logger::info("No detections published - no chip data transmitted");
        Logger::info("Bandwidth savings: 100% (no data sent)");
    }
}

} // namespace sar_atr
