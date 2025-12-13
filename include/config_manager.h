#ifndef CONFIG_MANAGER_H
#define CONFIG_MANAGER_H

#include <string>
#include <yaml-cpp/yaml.h>

namespace sar_atr {

/**
 * @struct ServiceConfig
 * @brief Configuration parameters for the SAR ATR service
 */
struct ServiceConfig {
    std::string broker_address;        ///< AMQ broker WebSocket address
    float confidence_threshold;        ///< Minimum confidence to publish results
    std::string system_uuid;           ///< System UUID for UCI messages
    std::string system_description;    ///< System description for UCI messages
    std::string service_version;       ///< Service version string
};

/**
 * @class ConfigManager
 * @brief Manages service configuration from YAML file
 */
class ConfigManager {
public:
    /**
     * @brief Load configuration from YAML file
     * @param config_path Path to the YAML configuration file
     * @return ServiceConfig structure with loaded values
     * @throws std::runtime_error if file cannot be read or is invalid
     */
    static ServiceConfig loadConfig(const std::string& config_path);
};

} // namespace sar_atr

#endif // CONFIG_MANAGER_H
