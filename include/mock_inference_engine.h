#ifndef MOCK_INFERENCE_ENGINE_H
#define MOCK_INFERENCE_ENGINE_H

#include "inference_engine.h"
#include <random>

namespace sar_atr {

/**
 * @class MockInferenceEngine
 * @brief Mock implementation of the SAR ATR inference engine for testing
 * 
 * This mock generates random detection results without actually processing
 * the NITF file. Used for testing the service architecture.
 */
class MockInferenceEngine : public InferenceEngine {
public:
    MockInferenceEngine();
    
    /**
     * @brief Generate mock detection results
     * @param nitf_file_path Path to NITF file (not actually used in mock)
     * @return Vector of randomly generated detection results
     */
    std::vector<DetectionResult> process(const std::string& nitf_file_path) override;
    
private:
    std::mt19937 rng_;
    std::uniform_real_distribution<float> confidence_dist_;
    std::uniform_real_distribution<float> coord_dist_;
    std::uniform_int_distribution<int> count_dist_;
    
    static const std::vector<std::string> classifications_;
};

} // namespace sar_atr

#endif // MOCK_INFERENCE_ENGINE_H
