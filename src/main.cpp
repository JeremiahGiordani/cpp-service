#include "sar_atr_service.h"
#include "mock_inference_engine.h"
#include "config_manager.h"
#include "logger.h"
#include <csignal>
#include <memory>

namespace {
    sar_atr::SarAtrService* g_service = nullptr;
    
    void signalHandler(int signal) {
        if (g_service) {
            sar_atr::Logger::info("Received interrupt signal, shutting down...");
            g_service->stop();
        }
    }
}

int main(int argc, char* argv[]) {
    try {
        // Determine config file path
        std::string config_path = "config/service_config.yaml";
        if (argc > 1) {
            config_path = argv[1];
        }
        
        // Load configuration
        sar_atr::ServiceConfig config = sar_atr::ConfigManager::loadConfig(config_path);
        
        // Create inference engine (mock for now)
        auto inference_engine = std::make_shared<sar_atr::MockInferenceEngine>();
        
        // Create service
        sar_atr::SarAtrService service(config, inference_engine);
        g_service = &service;
        
        // Set up signal handlers for graceful shutdown
        std::signal(SIGINT, signalHandler);
        std::signal(SIGTERM, signalHandler);
        
        // Start service
        service.start();
        
        return 0;
        
    } catch (const std::exception& e) {
        sar_atr::Logger::error("Fatal error: " + std::string(e.what()));
        return 1;
    }
}
