#include "mock_inference_engine.h"
#include "logger.h"
#include <random>
#include <thread>
#include <chrono>

namespace sar_atr {

const std::vector<std::string> MockInferenceEngine::classifications_ = {
    "class1", "class2", "class3"
};

MockInferenceEngine::MockInferenceEngine() 
    : rng_(std::random_device{}()),
      confidence_dist_(0.3f, 0.99f),
      coord_dist_(0.05f, 0.95f),
      count_dist_(0, 5) {
}

std::vector<DetectionResult> MockInferenceEngine::process(const std::string& nitf_file_path) {
    Logger::info("Mock inference engine processing: " + nitf_file_path);
    
    // Simulate processing time
    std::this_thread::sleep_for(std::chrono::milliseconds(100 + (rng_() % 400)));
    
    std::vector<DetectionResult> results;
    int num_detections = count_dist_(rng_);
    
    for (int i = 0; i < num_detections; ++i) {
        DetectionResult detection;
        
        // Random classification
        detection.classification = classifications_[rng_() % classifications_.size()];
        
        // Random confidence
        detection.confidence = confidence_dist_(rng_);
        
        // Random bounding box (ensure x1 < x2 and y1 < y2)
        float x1 = coord_dist_(rng_);
        float y1 = coord_dist_(rng_);
        float width = std::uniform_real_distribution<float>(0.05f, 0.3f)(rng_);
        float height = std::uniform_real_distribution<float>(0.05f, 0.3f)(rng_);
        
        detection.bounding_box.x1 = x1;
        detection.bounding_box.y1 = y1;
        detection.bounding_box.x2 = std::min(1.0f, x1 + width);
        detection.bounding_box.y2 = std::min(1.0f, y1 + height);
        
        // 50% chance of generating an output file path
        if (rng_() % 2 == 0) {
            detection.output_file_path = "/output/chips/chip_" + std::to_string(rng_() % 10000) + ".nitf";
        } else {
            detection.output_file_path = "";  // No output file for this detection
        }
        
        results.push_back(detection);
    }
    
    Logger::info("Mock inference generated " + std::to_string(num_detections) + " detections");
    
    return results;
}

} // namespace sar_atr
