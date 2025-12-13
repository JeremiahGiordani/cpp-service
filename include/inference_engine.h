/**
 * @file inference_engine.h
 * @brief Interface contract for SAR ATR Inference Engine
 * 
 * This header defines the interface between the UCI service and the SAR ATR
 * inference engine. The inference engine is responsible for analyzing NITF
 * imagery and detecting/classifying targets.
 * 
 * INTEGRATION GUIDE:
 * -----------------
 * To integrate your inference engine implementation:
 * 1. Include this header file in your implementation
 * 2. Implement a class that provides the process() method with this signature
 * 3. The service will call process() with the NITF file path
 * 4. Return a vector of DetectionResult structures
 * 
 * THREAD SAFETY:
 * --------------
 * The process() method may be called from different threads. Ensure your
 * implementation is thread-safe or document any threading requirements.
 */

#ifndef INFERENCE_ENGINE_H
#define INFERENCE_ENGINE_H

#include <string>
#include <vector>

namespace sar_atr {

/**
 * @struct BoundingBox
 * @brief Represents a bounding box in normalized pixel coordinates (XYXY format)
 * 
 * Coordinates are normalized to [0.0, 1.0] range:
 * - (0, 0) represents top-left corner of image
 * - (1, 1) represents bottom-right corner of image
 * 
 * Format: (x1, y1, x2, y2) where:
 * - (x1, y1) is the top-left corner of the bounding box
 * - (x2, y2) is the bottom-right corner of the bounding box
 */
struct BoundingBox {
    float x1;  ///< Top-left X coordinate (normalized, 0.0 to 1.0)
    float y1;  ///< Top-left Y coordinate (normalized, 0.0 to 1.0)
    float x2;  ///< Bottom-right X coordinate (normalized, 0.0 to 1.0)
    float y2;  ///< Bottom-right Y coordinate (normalized, 0.0 to 1.0)
    
    /**
     * @brief Calculate the width of the bounding box
     * @return Width in normalized coordinates
     */
    float width() const { return x2 - x1; }
    
    /**
     * @brief Calculate the height of the bounding box
     * @return Height in normalized coordinates
     */
    float height() const { return y2 - y1; }
    
    /**
     * @brief Calculate the center X coordinate
     * @return Center X in normalized coordinates
     */
    float centerX() const { return (x1 + x2) / 2.0f; }
    
    /**
     * @brief Calculate the center Y coordinate
     * @return Center Y in normalized coordinates
     */
    float centerY() const { return (y1 + y2) / 2.0f; }
};

/**
 * @struct DetectionResult
 * @brief Single detection result from the inference engine
 * 
 * Represents one detected/classified target in the imagery with its
 * classification, confidence score, and location.
 */
struct DetectionResult {
    std::string classification;  ///< Target classification/type (e.g., "T-72", "BMP-2")
    float confidence;            ///< Confidence score [0.0, 1.0] where 1.0 is highest confidence
    BoundingBox bounding_box;    ///< Location of detection in normalized XYXY coordinates
};

/**
 * @class InferenceEngine
 * @brief Abstract interface for SAR ATR inference implementations
 * 
 * This is the contract that all inference engine implementations must follow.
 * The service will instantiate and call this interface to perform target
 * detection and classification on NITF imagery.
 */
class InferenceEngine {
public:
    virtual ~InferenceEngine() = default;
    
    /**
     * @brief Process a NITF file and return detection results
     * 
     * This is the main entry point for inference. The implementation should:
     * 1. Load and parse the NITF file at the given path
     * 2. Run the SAR ATR algorithm on the imagery
     * 3. Return all detections that meet internal quality thresholds
     * 
     * NOTE: The service applies its own confidence threshold filtering
     * after this method returns, so implementations should return all
     * reasonable detections and not apply aggressive filtering.
     * 
     * @param nitf_file_path Absolute path to the NITF file to process
     * @return Vector of detection results (may be empty if no targets found)
     * @throws std::runtime_error if file cannot be read or processing fails
     */
    virtual std::vector<DetectionResult> process(const std::string& nitf_file_path) = 0;
};

} // namespace sar_atr

#endif // INFERENCE_ENGINE_H
