set(JSONCPP_VERSION 1.9.5)
message(STATUS "MANAGING DEPENDENCY: JsonCpp (Version ${JSONCPP_VERSION})\n")

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
    FetchContent_Declare(
        jsoncpp
        GIT_REPOSITORY https://github.com/open-source-parsers/jsoncpp.git
        GIT_TAG ${JSONCPP_VERSION}
        GIT_SHALLOW TRUE
        OVERRIDE_FIND_PACKAGE
    )
    
    # Configure jsoncpp build options
    set(JSONCPP_WITH_TESTS OFF CACHE BOOL "Build jsoncpp tests" FORCE)
    set(JSONCPP_WITH_POST_BUILD_UNITTEST OFF CACHE BOOL "Run jsoncpp tests" FORCE)
    set(JSONCPP_WITH_PKGCONFIG_SUPPORT OFF CACHE BOOL "Generate pkgconfig" FORCE)
    set(BUILD_SHARED_LIBS OFF CACHE BOOL "Build shared libraries" FORCE)
    
    FetchContent_MakeAvailable(jsoncpp)
    
    # Create interface library for consistent usage
    if(NOT TARGET jsoncpp_lib)
        add_library(jsoncpp_interface INTERFACE)
        target_link_libraries(jsoncpp_interface INTERFACE jsoncpp_static)
        add_library(jsoncpp_lib ALIAS jsoncpp_interface)
    endif()
else()
    message(STATUS "--> Found JsonCpp\n")
    
    # Create consistent interface if using system library
    if(NOT TARGET jsoncpp_lib)
        add_library(jsoncpp_interface INTERFACE)
        if(TARGET jsoncpp_lib)
            target_link_libraries(jsoncpp_interface INTERFACE jsoncpp_lib)
        else()
            target_include_directories(jsoncpp_interface INTERFACE ${JSONCPP_INCLUDE_DIRS})
            target_link_libraries(jsoncpp_interface INTERFACE ${JSONCPP_LIBRARIES})
        endif()
        add_library(jsoncpp_lib ALIAS jsoncpp_interface)
    endif()
endif()
