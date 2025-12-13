#include "config_manager.h"
#include "logger.h"
#include <fstream>
#include <stdexcept>

namespace sar_atr {

ServiceConfig ConfigManager::loadConfig(const std::string& config_path) {
    Logger::info("Loading configuration from: " + config_path);
    
    try {
        YAML::Node config = YAML::LoadFile(config_path);
        
        ServiceConfig service_config;
        
        // Required fields
        if (!config["broker_address"]) {
            throw std::runtime_error("Missing required field: broker_address");
        }
        service_config.broker_address = config["broker_address"].as<std::string>();
        
        if (!config["confidence_threshold"]) {
            throw std::runtime_error("Missing required field: confidence_threshold");
        }
        service_config.confidence_threshold = config["confidence_threshold"].as<float>();
        
        // Validate confidence threshold
        if (service_config.confidence_threshold < 0.0f || service_config.confidence_threshold > 1.0f) {
            throw std::runtime_error("confidence_threshold must be between 0.0 and 1.0");
        }
        
        // Optional fields with defaults
        service_config.system_uuid = config["system_uuid"] 
            ? config["system_uuid"].as<std::string>() 
            : "00000000-0000-0000-0000-000000000000";
            
        service_config.system_description = config["system_description"]
            ? config["system_description"].as<std::string>()
            : "SAR ATR Service";
            
        service_config.service_version = config["service_version"]
            ? config["service_version"].as<std::string>()
            : "1.0.0";
        
        Logger::info("Configuration loaded successfully");
        Logger::info("  Broker: " + service_config.broker_address);
        Logger::info("  Confidence Threshold: " + std::to_string(service_config.confidence_threshold));
        Logger::info("  System UUID: " + service_config.system_uuid);
        
        return service_config;
        
    } catch (const YAML::Exception& e) {
        throw std::runtime_error("Failed to parse YAML config: " + std::string(e.what()));
    }
}

} // namespace sar_atr
