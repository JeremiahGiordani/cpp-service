#ifndef UCI_MESSAGES_H
#define UCI_MESSAGES_H

#include <string>
#include <vector>
#include <json/json.h>
#include "inference_engine.h"

namespace sar_atr {

/**
 * @brief Generate a UUID v4 string
 */
std::string generateUUID();

/**
 * @brief Get current timestamp in ISO 8601 format
 */
std::string getCurrentTimestamp();

/**
 * @struct SystemInfo
 * @brief System identification information from config
 */
struct SystemInfo {
    std::string system_uuid;
    std::string system_description;
    std::string service_version;
};

/**
 * @brief Parse FileLocation UCI message to extract NITF file path
 * @param json_message The JSON string of the FileLocation message
 * @return The file path extracted from the message
 * @throws std::runtime_error if parsing fails
 */
std::string parseFileLocationMessage(const std::string& json_message);

/**
 * @brief Create Entity UCI message from detection result
 * @param detection The detection result from inference engine
 * @param system_info System identification information
 * @return JSON string of the Entity message
 */
std::string createEntityMessage(const DetectionResult& detection, const SystemInfo& system_info);

/**
 * @brief Create AtrProcessingResult UCI message
 * @param entity_uuids Vector of entity UUIDs that were created
 * @return JSON string of the AtrProcessingResult message
 */
std::string createAtrProcessingResultMessage(const std::vector<std::string>& entity_uuids);

} // namespace sar_atr

#endif // UCI_MESSAGES_H
