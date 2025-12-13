#include "uci_messages.h"
#include "logger.h"
#include <random>
#include <sstream>
#include <iomanip>
#include <chrono>
#include <stdexcept>

namespace sar_atr {

std::string generateUUID() {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_int_distribution<> dis(0, 15);
    static std::uniform_int_distribution<> dis2(8, 11);
    
    std::stringstream ss;
    int i;
    ss << std::hex;
    for (i = 0; i < 8; i++) {
        ss << dis(gen);
    }
    ss << "-";
    for (i = 0; i < 4; i++) {
        ss << dis(gen);
    }
    ss << "-4";
    for (i = 0; i < 3; i++) {
        ss << dis(gen);
    }
    ss << "-";
    ss << dis2(gen);
    for (i = 0; i < 3; i++) {
        ss << dis(gen);
    }
    ss << "-";
    for (i = 0; i < 12; i++) {
        ss << dis(gen);
    }
    return ss.str();
}

std::string getCurrentTimestamp() {
    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()) % 1000;
    
    std::stringstream ss;
    ss << std::put_time(std::gmtime(&time), "%Y-%m-%dT%H:%M:%S");
    ss << "." << std::setfill('0') << std::setw(3) << ms.count() << "Z";
    return ss.str();
}

std::string parseFileLocationMessage(const std::string& json_message) {
    Json::Value root;
    Json::CharReaderBuilder builder;
    std::string errs;
    std::istringstream iss(json_message);
    
    if (!Json::parseFromStream(builder, iss, &root, &errs)) {
        throw std::runtime_error("Failed to parse FileLocation message: " + errs);
    }
    
    try {
        const Json::Value& file_location = root["FileLocation"];
        const Json::Value& message_data = file_location["MessageData"];
        const Json::Value& location = message_data["LocationAndStatus"]["Location"];
        const Json::Value& network = location["Network"];
        
        std::string address = network["Address"].asString();
        
        if (address.empty()) {
            throw std::runtime_error("Address field is empty in FileLocation message");
        }
        
        return address;
    } catch (const std::exception& e) {
        throw std::runtime_error("Failed to extract file path from FileLocation message: " + 
                                std::string(e.what()));
    }
}

std::string createEntityMessage(const DetectionResult& detection, const SystemInfo& system_info) {
    Json::Value root;
    Json::Value& entity = root["Entity"];
    
    // Namespace
    entity["@xmlns"] = "namespace";
    
    // Security Information
    Json::Value& security = entity["SecurityInformation"];

    
    // Message Header
    Json::Value& header = entity["MessageHeader"];
    header["SystemID"]["UUID"] = system_info.system_uuid;
    header["SystemID"]["DescriptiveLabel"] = system_info.system_description;
    header["Timestamp"] = getCurrentTimestamp();
    header["SchemaVersion"] = "002.3";
    header["Mode"] = "SIMULATION";
    header["ServiceID"]["UUID"] = system_info.system_uuid;
    header["ServiceID"]["DescriptiveLabel"] = system_info.system_description;
    header["ServiceID"]["ServiceVersion"] = system_info.service_version;
    
    // Message Data
    Json::Value& data = entity["MessageData"];
    std::string entity_uuid = generateUUID();
    data["EntityID"]["UUID"] = entity_uuid;
    data["CreationTimestamp"] = getCurrentTimestamp();
    
    // Identity
    data["Identity"]["Platform"]["ThreatType"] = detection.classification;
    
    // Kinematics - Position with bounding box
    Json::Value& rectangle = data["Kinematics"]["Position"]["Zone"]["Shape"]["Rectangle"];
    rectangle["Width"] = detection.bounding_box.width();
    rectangle["Height"] = detection.bounding_box.height();
    
    Json::Value& relative_offset = rectangle["CenterPositionChoice"]["RelativePoint"]["RelativeOffset"];
    relative_offset["X"] = detection.bounding_box.centerX();
    relative_offset["Y"] = detection.bounding_box.centerY();
    
    // Convert to string
    Json::StreamWriterBuilder writer;
    writer["indentation"] = "";
    return Json::writeString(writer, root);
}

std::string createAtrProcessingResultMessage(const std::vector<std::string>& entity_uuids) {
    Json::Value root;
    Json::Value& atr_result = root["ATR_ProcessingResultsType"];
    
    atr_result["@xmlns"] = "";
    
    Json::Value entity_ids(Json::arrayValue);
    for (const auto& uuid : entity_uuids) {
        Json::Value entity_id;
        entity_id["@xmlns"] = "namespace";
        entity_id["ns1:UUID"] = uuid;
        entity_ids.append(entity_id);
    }
    
    atr_result["ns1:EntityId"] = entity_ids;
    
    // Convert to string
    Json::StreamWriterBuilder writer;
    writer["indentation"] = "";
    return Json::writeString(writer, root);
}

} // namespace sar_atr
