set(JSONCPP_VERSION 1.9.5)
message(STATUS "MANAGING DEPENDENCY: JsonCpp (Version ${JSONCPP_VERSION})\n")

# Save the current C++ standard
set(SAVED_CMAKE_CXX_STANDARD ${CMAKE_CXX_STANDARD})

# Try to find JsonCpp using pkg-config first
find_package(PkgConfig QUIET)
if(PKG_CONFIG_FOUND)
    pkg_check_modules(JSONCPP jsoncpp)
endif()

# If not found via pkg-config, try standard CMake find
if(NOT JSONCPP_FOUND)
    if(DEFINED ENV{SDKTARGETSYSROOT})
        find_package(jsoncpp ${JSONCPP_VERSION} QUIET PATHS $ENV{SDKTARGETSYSROOT}/usr/lib/cmake/jsoncpp NO_DEFAULT_PATH)
    else()
        find_package(jsoncpp ${JSONCPP_VERSION} QUIET PATHS /usr/lib/cmake/jsoncpp NO_DEFAULT_PATH)
    endif()
endif()

if(NOT JSONCPP_FOUND AND NOT jsoncpp_FOUND)
    message(STATUS "--> Unable to Find JsonCpp - Building from Source\n")

    include(FetchContent)
    
    # Configure jsoncpp build options BEFORE declaring
    set(JSONCPP_WITH_TESTS OFF CACHE BOOL "Build jsoncpp tests" FORCE)
    set(JSONCPP_WITH_POST_BUILD_UNITTEST OFF CACHE BOOL "Run jsoncpp tests" FORCE)
    set(JSONCPP_WITH_PKGCONFIG_SUPPORT OFF CACHE BOOL "Generate pkgconfig" FORCE)
    set(BUILD_SHARED_LIBS OFF CACHE BOOL "Build shared libraries" FORCE)
    
    FetchContent_Declare(
        jsoncpp
        GIT_REPOSITORY https://github.com/open-source-parsers/jsoncpp.git
        GIT_TAG ${JSONCPP_VERSION}
        GIT_SHALLOW TRUE
    )
    
    FetchContent_MakeAvailable(jsoncpp)
    
    # Set C++ standard for jsoncpp targets after they're created
    if(TARGET jsoncpp_static)
        target_compile_features(jsoncpp_static PUBLIC cxx_std_17)
        set_target_properties(jsoncpp_static PROPERTIES
            CXX_STANDARD 17
            CXX_STANDARD_REQUIRED ON
            POSITION_INDEPENDENT_CODE ON
        )
    endif()
    if(TARGET jsoncpp_lib_static)
        target_compile_features(jsoncpp_lib_static PUBLIC cxx_std_17)
        set_target_properties(jsoncpp_lib_static PROPERTIES
            CXX_STANDARD 17
            CXX_STANDARD_REQUIRED ON
            POSITION_INDEPENDENT_CODE ON
        )
    endif()
    
    # JsonCpp may create different targets depending on version
    # Check what's available and create a consistent jsoncpp_lib alias
    if(NOT TARGET jsoncpp_lib)
        if(TARGET jsoncpp_static)
            add_library(jsoncpp_lib ALIAS jsoncpp_static)
            message(STATUS "  Using jsoncpp_static as jsoncpp_lib")
        elseif(TARGET jsoncpp_lib_static)
            add_library(jsoncpp_lib ALIAS jsoncpp_lib_static)
            message(STATUS "  Using jsoncpp_lib_static as jsoncpp_lib")
        elseif(TARGET JsonCpp::JsonCpp)
            add_library(jsoncpp_lib ALIAS JsonCpp::JsonCpp)
            message(STATUS "  Using JsonCpp::JsonCpp as jsoncpp_lib")
        else()
            message(FATAL_ERROR "Could not find jsoncpp target after building from source")
        endif()
    endif()
else()
    message(STATUS "--> Found JsonCpp\n")
    
    # Create consistent interface if using system library
    if(NOT TARGET jsoncpp_lib)
        if(TARGET JsonCpp::JsonCpp)
            add_library(jsoncpp_lib ALIAS JsonCpp::JsonCpp)
        elseif(TARGET jsoncpp_lib_static)
            add_library(jsoncpp_lib ALIAS jsoncpp_lib_static)
        else()
            add_library(jsoncpp_interface INTERFACE)
            target_include_directories(jsoncpp_interface INTERFACE ${JSONCPP_INCLUDE_DIRS})
            target_link_libraries(jsoncpp_interface INTERFACE ${JSONCPP_LIBRARIES})
            add_library(jsoncpp_lib ALIAS jsoncpp_interface)
        endif()
    endif()
endif()
