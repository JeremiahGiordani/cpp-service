set(YAMLCPP_VERSION 0.8.0)
message(STATUS "MANAGING DEPENDENCY: yaml-cpp (Version ${YAMLCPP_VERSION})\n")

# Try to find yaml-cpp using pkg-config first
find_package(PkgConfig QUIET)
if(PKG_CONFIG_FOUND)
    pkg_check_modules(YAMLCPP yaml-cpp)
endif()

# If not found via pkg-config, try standard CMake find
if(NOT YAMLCPP_FOUND)
    if(DEFINED ENV{SDKTARGETSYSROOT})
        find_package(yaml-cpp ${YAMLCPP_VERSION} QUIET PATHS $ENV{SDKTARGETSYSROOT}/usr/lib/cmake/yaml-cpp NO_DEFAULT_PATH)
    else()
        find_package(yaml-cpp ${YAMLCPP_VERSION} QUIET PATHS /usr/lib/cmake/yaml-cpp NO_DEFAULT_PATH)
    endif()
endif()

if(NOT YAMLCPP_FOUND AND NOT yaml-cpp_FOUND)
    message(STATUS "--> Unable to Find yaml-cpp - Building from Source\n")

    include(FetchContent)
    FetchContent_Declare(
        yaml-cpp
        GIT_REPOSITORY https://github.com/jbeder/yaml-cpp.git
        GIT_TAG ${YAMLCPP_VERSION}
        GIT_SHALLOW TRUE
        OVERRIDE_FIND_PACKAGE
    )
    
    # Configure yaml-cpp build options
    set(YAML_CPP_BUILD_TESTS OFF CACHE BOOL "Build yaml-cpp tests" FORCE)
    set(YAML_CPP_BUILD_TOOLS OFF CACHE BOOL "Build yaml-cpp tools" FORCE)
    set(YAML_CPP_BUILD_CONTRIB OFF CACHE BOOL "Build yaml-cpp contrib" FORCE)
    set(YAML_BUILD_SHARED_LIBS OFF CACHE BOOL "Build shared libraries" FORCE)
    
    FetchContent_MakeAvailable(yaml-cpp)
    
    # yaml-cpp already creates yaml-cpp target, just ensure alias exists
    if(NOT TARGET yaml-cpp::yaml-cpp)
        add_library(yaml-cpp::yaml-cpp ALIAS yaml-cpp)
    endif()
else()
    message(STATUS "--> Found yaml-cpp\n")
    
    # Create consistent interface if using system library
    if(NOT TARGET yaml-cpp::yaml-cpp)
        if(TARGET yaml-cpp)
            add_library(yaml-cpp::yaml-cpp ALIAS yaml-cpp)
        else()
            add_library(yaml-cpp-interface INTERFACE)
            target_include_directories(yaml-cpp-interface INTERFACE ${YAMLCPP_INCLUDE_DIRS})
            target_link_libraries(yaml-cpp-interface INTERFACE ${YAMLCPP_LIBRARIES})
            add_library(yaml-cpp::yaml-cpp ALIAS yaml-cpp-interface)
        endif()
    endif()
endif()
