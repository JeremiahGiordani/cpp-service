cmake_minimum_required(VERSION 3.15)
project(sar_atr_service VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Dependency Management
include(cmake/manage_jsoncpp.cmake)
include(cmake/manage_yamlcpp.cmake)

# Include directories
include_directories(
    ${PROJECT_SOURCE_DIR}/include
)

# Source files
set(SERVICE_SOURCES
    src/main.cpp
    src/amq_client.cpp
    src/uci_messages.cpp
    src/config_manager.cpp
    src/sar_atr_service.cpp
    src/mock_inference_engine.cpp
)

# Create executable
add_executable(sar_atr_service ${SERVICE_SOURCES})

# Link libraries
target_link_libraries(sar_atr_service
    jsoncpp_lib
    yaml-cpp::yaml-cpp
    pthread
)

# Install targets
install(TARGETS sar_atr_service DESTINATION bin)
install(FILES config/service_config.yaml DESTINATION etc)
