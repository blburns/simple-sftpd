# CMakeLists.txt template for SimpleDaemons projects
# Simple Secure FTP Daemon - A secure, configurable, and feature-rich FTP server
# Copyright 2024 SimpleDaemons

cmake_minimum_required(VERSION 3.16)
project(simple-sftpd VERSION 0.3.0 LANGUAGES CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Platform detection
if(WIN32)
    set(PLATFORM_NAME "Windows")
elseif(APPLE)
    set(PLATFORM_NAME "macOS")
elseif(CMAKE_SYSTEM_NAME STREQUAL "FreeBSD")
    set(PLATFORM_NAME "FreeBSD")
elseif(UNIX)
    set(PLATFORM_NAME "Linux")
endif()

# Build options
option(BUILD_SHARED_LIBS "Build shared libraries" OFF)
option(ENABLE_TESTS "Enable tests" ON)
option(ENABLE_PACKAGING "Enable package generation" ON)
option(ENABLE_SSL "Enable SSL/TLS support" ON)
option(ENABLE_JSON "Enable JSON support" ON)
option(ENABLE_COMPRESSION "Enable compression support (gzip, bzip2)" ON)
option(ENABLE_STATIC_LINKING "Enable static linking for self-contained binaries" OFF)

# Product Version Selection
# Options: production (default), enterprise, datacenter
set(BUILD_VERSION "production" CACHE STRING "Product version to build (production, enterprise, datacenter)")
set_property(CACHE BUILD_VERSION PROPERTY STRINGS production enterprise datacenter)

# Validate BUILD_VERSION
if(NOT BUILD_VERSION MATCHES "^(production|enterprise|datacenter)$")
    message(FATAL_ERROR "BUILD_VERSION must be one of: production, enterprise, datacenter")
endif()

# Set version-specific defines and main source
if(BUILD_VERSION STREQUAL "production")
    set(PRODUCT_VERSION_NAME "production")
    set(PRODUCT_VERSION_DISPLAY "Production")
    set(MAIN_SOURCE "main/production.cpp")
    add_definitions(-DSIMPLE_SFTPD_PRODUCTION)
    message(STATUS "Building Production Version (Apache 2.0)")
elseif(BUILD_VERSION STREQUAL "enterprise")
    set(PRODUCT_VERSION_NAME "enterprise")
    set(PRODUCT_VERSION_DISPLAY "Enterprise")
    set(MAIN_SOURCE "main/enterprise.cpp")
    add_definitions(-DSIMPLE_SFTPD_ENTERPRISE)
    message(STATUS "Building Enterprise Version (BSL 1.1)")
elseif(BUILD_VERSION STREQUAL "datacenter")
    set(PRODUCT_VERSION_NAME "datacenter")
    set(PRODUCT_VERSION_DISPLAY "Datacenter")
    set(MAIN_SOURCE "main/datacenter.cpp")
    add_definitions(-DSIMPLE_SFTPD_DATACENTER)
    message(STATUS "Building Datacenter Version (BSL 1.1)")
endif()

# Find required packages
find_package(Threads REQUIRED)

if(ENABLE_SSL)
    find_package(OpenSSL REQUIRED)
    add_definitions(-DSIMPLE_SFTPD_SSL_ENABLED)
endif()

if(ENABLE_JSON)
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(JSONCPP REQUIRED jsoncpp)
    add_definitions(-DENABLE_JSON)
endif()

if(ENABLE_COMPRESSION)
    find_package(ZLIB REQUIRED)
    find_library(BZIP2_LIB NAMES bz2 PATHS /usr/lib /usr/lib64 /usr/local/lib)
    find_path(BZIP2_INCLUDE_DIR NAMES bzlib.h PATHS /usr/include /usr/local/include)
    if(BZIP2_LIB AND BZIP2_INCLUDE_DIR)
        add_definitions(-DHAVE_BZIP2)
    else()
        message(WARNING "bzip2 not found (need library and bzlib.h); bzip2 compression disabled. "
                        "Install bzip2-devel (RHEL) or libbz2-dev (Debian).")
    endif()
    add_definitions(-DENABLE_COMPRESSION)
endif()

# Optional PAM on Linux
if(UNIX AND NOT APPLE)
    find_library(PAM_LIBRARY NAMES pam PATHS /usr/lib /usr/lib64 /usr/local/lib)
    find_path(PAM_INCLUDE_DIR NAMES security/pam_appl.h PATHS /usr/include /usr/local/include)
    if(PAM_LIBRARY AND PAM_INCLUDE_DIR)
        set(HAVE_PAM TRUE)
        message(STATUS "PAM support: ON")
    else()
        message(WARNING "libpam not found; PAM authentication disabled. "
                        "Install pam-devel (RHEL) or libpam0g-dev (Debian).")
    endif()
endif()

# Include directories
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

# Source files - exclude src/core/ to avoid duplicates with src/simple-sftpd/
file(GLOB_RECURSE SOURCES 
    "src/simple-sftpd/*.cpp"
    "src/production/*.cpp"
    "src/*.c"
)

# Add version-specific sources
if(BUILD_VERSION STREQUAL "enterprise")
    # Enterprise includes all production features plus enterprise-specific
    file(GLOB_RECURSE ENTERPRISE_SOURCES "src/enterprise/*.cpp")
    list(APPEND SOURCES ${ENTERPRISE_SOURCES})
elseif(BUILD_VERSION STREQUAL "datacenter")
    # Datacenter includes all enterprise and production features plus datacenter-specific
    file(GLOB_RECURSE ENTERPRISE_SOURCES "src/enterprise/*.cpp")
    file(GLOB_RECURSE DATACENTER_SOURCES "src/datacenter/*.cpp")
    list(APPEND SOURCES ${ENTERPRISE_SOURCES} ${DATACENTER_SOURCES})
endif()

file(GLOB_RECURSE HEADERS "include/*.hpp" "include/*.h")

# Create executable with version-specific main
add_executable(${PROJECT_NAME} ${SOURCES} ${MAIN_SOURCE} ${HEADERS})

target_compile_definitions(${PROJECT_NAME} PRIVATE
    SIMPLE_SFTPD_VERSION="${PROJECT_VERSION}"
)

# Link libraries
target_link_libraries(${PROJECT_NAME} Threads::Threads)

if(ENABLE_SSL)
    target_link_libraries(${PROJECT_NAME} OpenSSL::SSL OpenSSL::Crypto)
endif()

if(ENABLE_COMPRESSION)
    target_link_libraries(${PROJECT_NAME} ZLIB::ZLIB)
    if(BZIP2_LIB)
        target_link_libraries(${PROJECT_NAME} ${BZIP2_LIB})
    endif()
endif()

if(HAVE_PAM)
    target_compile_definitions(${PROJECT_NAME} PRIVATE HAVE_PAM)
    target_include_directories(${PROJECT_NAME} PRIVATE ${PAM_INCLUDE_DIR})
    target_link_libraries(${PROJECT_NAME} ${PAM_LIBRARY})
endif()

if(ENABLE_JSON)
    if(ENABLE_STATIC_LINKING)
        # For static linking, find the static library directly
        find_library(JSONCPP_STATIC_LIB jsoncpp PATHS /opt/local/lib /usr/local/lib /usr/lib)
        if(JSONCPP_STATIC_LIB)
            target_link_libraries(${PROJECT_NAME} ${JSONCPP_STATIC_LIB})
        else()
            target_link_libraries(${PROJECT_NAME} ${JSONCPP_LIBRARIES})
        endif()
    else()
        target_link_libraries(${PROJECT_NAME} ${JSONCPP_LIBRARIES})
    endif()
    target_include_directories(${PROJECT_NAME} PRIVATE ${JSONCPP_INCLUDE_DIRS})
    target_compile_options(${PROJECT_NAME} PRIVATE ${JSONCPP_CFLAGS_OTHER})
    target_link_directories(${PROJECT_NAME} PRIVATE ${JSONCPP_LIBRARY_DIRS})
endif()

# Compiler-specific options
if(MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE /W3 /WX)
    if(ENABLE_STATIC_LINKING)
        target_compile_options(${PROJECT_NAME} PRIVATE /MT)
    endif()
else()
    target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra -Wpedantic -Wno-unused-parameter)
    if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        target_compile_options(${PROJECT_NAME} PRIVATE -Wno-unused-private-field)
    endif()
    if(ENABLE_STATIC_LINKING)
        # Only apply static linking flags for GCC, not clang
        if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
            target_compile_options(${PROJECT_NAME} PRIVATE -static-libgcc -static-libstdc++)
            target_link_options(${PROJECT_NAME} PRIVATE -static-libgcc -static-libstdc++)
        endif()
    endif()
endif()

# Static linking configuration
if(ENABLE_STATIC_LINKING)
    set(BUILD_SHARED_LIBS OFF)
    if(ENABLE_SSL)
        # Force static linking for OpenSSL
        set(OPENSSL_USE_STATIC_LIBS TRUE)
    endif()
    if(ENABLE_JSON)
        # Force static linking for jsoncpp
        set(JSONCPP_USE_STATIC_LIBS TRUE)
    endif()
endif()

# Install targets
install(TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
)

install(DIRECTORY include/
    DESTINATION include
    FILES_MATCHING PATTERN "*.h" PATTERN "*.hpp"
)

# Install configuration files
install(DIRECTORY config/
    DESTINATION etc/${PROJECT_NAME}
    FILES_MATCHING PATTERN "*.conf" PATTERN "*.example"
)

# Install service files
if(UNIX AND NOT APPLE)
    install(FILES deployment/systemd/${PROJECT_NAME}.service
        DESTINATION lib/systemd/system
    )
elseif(APPLE)
    install(FILES deployment/launchd/com.${PROJECT_NAME}.${PROJECT_NAME}.plist
        DESTINATION Library/LaunchDaemons
    )
endif()

# Tests
if(ENABLE_TESTS)
    enable_testing()
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/tests/CMakeLists.txt")
        add_subdirectory(tests)
    endif()
endif()

# Package generation
if(ENABLE_PACKAGING)
    include(CPack)
    
    # Package information
    set(CPACK_PACKAGE_NAME "${PROJECT_NAME}")
    set(CPACK_PACKAGE_VENDOR "SimpleDaemons")
    set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "${PROJECT_DESCRIPTION}")
    set(CPACK_PACKAGE_VERSION "${PROJECT_VERSION}")
    set(CPACK_PACKAGE_CONTACT "info@simpledaemons.com")
    set(CPACK_PACKAGE_HOMEPAGE_URL "https://github.com/simpledaemons/${PROJECT_NAME}")
    
    # Package files
    set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE")
    set(CPACK_RESOURCE_FILE_README "${CMAKE_CURRENT_SOURCE_DIR}/README.md")
    
    # Platform-specific package settings
    if(WIN32)
        # Windows MSI and ZIP packages
        set(CPACK_GENERATOR "WIX;ZIP")
        
        # WIX (MSI) settings
        set(CPACK_WIX_PRODUCT_GUID "12345678-1234-1234-1234-123456789012")
        set(CPACK_WIX_UPGRADE_GUID "87654321-4321-4321-4321-210987654321")
        set(CPACK_WIX_PROPERTY_ARPPRODUCTICON "${CMAKE_CURRENT_SOURCE_DIR}/assets/icon.ico")
        set(CPACK_WIX_PROPERTY_ARPHELPLINK "https://github.com/simpledaemons/${PROJECT_NAME}")
        set(CPACK_WIX_PROPERTY_ARPURLINFOABOUT "https://github.com/simpledaemons/${PROJECT_NAME}")
        set(CPACK_WIX_PROPERTY_ARPNOREPAIR "1")
        set(CPACK_WIX_PROPERTY_ARPNOMODIFY "1")
        
        # ZIP package settings
        set(CPACK_ZIP_COMPONENT_INSTALL ON)
        
    elseif(APPLE)
        # macOS DMG and PKG packages
        set(CPACK_GENERATOR "DragNDrop;productbuild")
        
        # DMG settings
        set(CPACK_DMG_VOLUME_NAME "${PROJECT_NAME}")
        set(CPACK_DMG_FORMAT "UDBZ")
        set(CPACK_DMG_BACKGROUND_IMAGE "${CMAKE_CURRENT_SOURCE_DIR}/assets/dmg_background.png")
        set(CPACK_DMG_DS_STORE "${CMAKE_CURRENT_SOURCE_DIR}/assets/DS_Store")
        
        # PKG settings
        set(CPACK_PRODUCTBUILD_IDENTIFIER "com.simpledaemons.${PROJECT_NAME}")
        set(CPACK_PRODUCTBUILD_PACKAGE_NAME "${PROJECT_NAME}")
        
    elseif(CMAKE_SYSTEM_NAME STREQUAL "FreeBSD")
        # FreeBSD binary tarball (install tree)
        set(CPACK_GENERATOR "TGZ")
        set(PLATFORM_ID "freebsd")

        if(CMAKE_SYSTEM_PROCESSOR MATCHES "amd64|x86_64")
            set(PACKAGE_ARCH "amd64")
        elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "aarch64|arm64")
            set(PACKAGE_ARCH "arm64")
        else()
            set(PACKAGE_ARCH "${CMAKE_SYSTEM_PROCESSOR}")
        endif()

    elseif(UNIX)
        # Linux DEB and RPM packages
        set(CPACK_GENERATOR "DEB;RPM")
        
        # DEB package settings
        set(CPACK_DEBIAN_PACKAGE_MAINTAINER "SimpleDaemons <info@simpledaemons.com>")
        set(CPACK_DEBIAN_PACKAGE_SECTION "net")
        set(CPACK_DEBIAN_PACKAGE_PRIORITY "optional")
        set(CPACK_DEBIAN_PACKAGE_DEPENDS "libssl1.1, libjsoncpp25")
        set(CPACK_DEBIAN_PACKAGE_RECOMMENDS "systemd")
        set(CPACK_DEBIAN_PACKAGE_SUGGESTS "logrotate")
        set(CPACK_DEBIAN_PACKAGE_CONFLICTS "ntp")
        set(CPACK_DEBIAN_PACKAGE_PROVIDES "${PROJECT_NAME}")
        set(CPACK_DEBIAN_PACKAGE_HOMEPAGE "https://github.com/simpledaemons/${PROJECT_NAME}")
        set(CPACK_DEBIAN_PACKAGE_DESCRIPTION "${PROJECT_DESCRIPTION}")
        set(CPACK_DEBIAN_PACKAGE_LONG_DESCRIPTION "${PROJECT_DESCRIPTION}")
        
        # RPM package settings
        set(CPACK_RPM_PACKAGE_LICENSE "Apache-2.0")
        set(CPACK_RPM_PACKAGE_GROUP "Applications/System")
        set(CPACK_RPM_PACKAGE_URL "https://github.com/simpledaemons/${PROJECT_NAME}")
        set(CPACK_RPM_PACKAGE_DESCRIPTION "${PROJECT_DESCRIPTION}")
        set(CPACK_RPM_PACKAGE_REQUIRES "openssl-libs, jsoncpp")
        set(CPACK_RPM_PACKAGE_SUGGESTS "systemd")
        set(CPACK_RPM_PACKAGE_CONFLICTS "ntp")
        set(CPACK_RPM_PACKAGE_PROVIDES "${PROJECT_NAME}")
        
        # Set package architecture and determine architecture string
        if(CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64")
            set(CPACK_DEBIAN_PACKAGE_ARCHITECTURE "amd64")
            set(CPACK_RPM_PACKAGE_ARCHITECTURE "x86_64")
            set(PACKAGE_ARCH "amd64")
        elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "aarch64")
            set(CPACK_DEBIAN_PACKAGE_ARCHITECTURE "arm64")
            set(CPACK_RPM_PACKAGE_ARCHITECTURE "aarch64")
            set(PACKAGE_ARCH "arm64")
        elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "armv7l")
            set(CPACK_DEBIAN_PACKAGE_ARCHITECTURE "armhf")
            set(CPACK_RPM_PACKAGE_ARCHITECTURE "armv7hl")
            set(PACKAGE_ARCH "armhf")
        else()
            set(PACKAGE_ARCH "${CMAKE_SYSTEM_PROCESSOR}")
        endif()
        
        # Determine distribution/platform identifier for package naming
        if(APPLE)
            # Detect Intel vs Apple Silicon for macOS
            if(CMAKE_SYSTEM_PROCESSOR MATCHES "arm64|aarch64")
                set(PLATFORM_ID "macos-apple")
            else()
                set(PLATFORM_ID "macos-intel")
            endif()
        else()
            # Try to detect Linux distribution for proper naming
            set(PLATFORM_ID "linux-generic")  # Default for RPM-based
            if(EXISTS "/etc/os-release")
                file(READ "/etc/os-release" OS_RELEASE)
                if(OS_RELEASE MATCHES "ID=([^\n]+)")
                    string(REGEX REPLACE ".*ID=([^\n]+).*" "\\1" DISTRO_ID "${OS_RELEASE}")
                    string(STRIP "${DISTRO_ID}" DISTRO_ID)
                    string(TOLOWER "${DISTRO_ID}" DISTRO_ID)
                    string(REPLACE "\"" "" DISTRO_ID "${DISTRO_ID}")
                    
                    # Map common distro IDs to package names
                    if(DISTRO_ID MATCHES "debian|ubuntu")
                        set(PLATFORM_ID "linux-debian")
                    endif()
                endif()
            endif()
        endif()
    endif()
    
    # Set package file names with standardized format: {project}-{version}-{product}-{platform}-{distro}-{arch}.{ext}
    # Format matches: simple-sftpd-0.2.1-production-linux-debian-amd64.deb
    if(APPLE)
        # macOS packages: simple-sftpd-0.2.1-production-macos-intel.dmg / .pkg
        set(CPACK_PACKAGE_FILE_NAME "${PROJECT_NAME}-${PROJECT_VERSION}-${PRODUCT_VERSION_NAME}-${PLATFORM_ID}")
    elseif(UNIX)
        # Linux/FreeBSD packages: simple-sftpd-0.3.0-production-linux-debian-amd64.deb / .rpm
        # FreeBSD TGZ: simple-sftpd-0.3.0-production-freebsd-amd64.tar.gz
        set(CPACK_PACKAGE_FILE_NAME "${PROJECT_NAME}-${PROJECT_VERSION}-${PRODUCT_VERSION_NAME}-${PLATFORM_ID}-${PACKAGE_ARCH}")
        
        if(NOT CMAKE_SYSTEM_NAME STREQUAL "FreeBSD")
            # Override DEB and RPM file names to match our format
            set(CPACK_DEBIAN_FILE_NAME "${PROJECT_NAME}-${PROJECT_VERSION}-${PRODUCT_VERSION_NAME}-${PLATFORM_ID}-${PACKAGE_ARCH}")
            set(CPACK_RPM_FILE_NAME "${PROJECT_NAME}-${PROJECT_VERSION}-${PRODUCT_VERSION_NAME}-${PLATFORM_ID}-${PACKAGE_ARCH}")
        endif()
    else()
        # Windows and others
        set(CPACK_PACKAGE_FILE_NAME "${PROJECT_NAME}-${PROJECT_VERSION}-${PRODUCT_VERSION_NAME}-${PLATFORM_NAME}-${PACKAGE_ARCH}")
    endif()
    set(CPACK_PACKAGE_INSTALL_DIRECTORY "${PROJECT_NAME}")
    
    # Component installation
    set(CPACK_COMPONENT_APPLICATIONS_DISPLAY_NAME "Application")
    set(CPACK_COMPONENT_APPLICATIONS_DESCRIPTION "The ${PROJECT_NAME} executable and related files")
    
    set(CPACK_COMPONENT_DEVELOPMENT_DISPLAY_NAME "Development")
    set(CPACK_COMPONENT_DEVELOPMENT_DESCRIPTION "Header files for developing with ${PROJECT_NAME}")
    
    set(CPACK_COMPONENT_CONFIG_DISPLAY_NAME "Configuration")
    set(CPACK_COMPONENT_CONFIG_DESCRIPTION "Configuration files and examples")
    
    set(CPACK_COMPONENT_SERVICE_DISPLAY_NAME "Service")
    set(CPACK_COMPONENT_SERVICE_DESCRIPTION "System service files")
    
    # Set component dependencies
    set(CPACK_COMPONENT_DEVELOPMENT_DEPENDS applications)
    set(CPACK_COMPONENT_CONFIG_DEPENDS applications)
    set(CPACK_COMPONENT_SERVICE_DEPENDS applications)
    
    # Source package generation
    set(CPACK_SOURCE_GENERATOR "TGZ;ZIP")
    set(CPACK_SOURCE_PACKAGE_FILE_NAME "${PROJECT_NAME}-${PROJECT_VERSION}-src")
    set(CPACK_SOURCE_IGNORE_FILES
        "/build/"
        "/dist/"
        "/.git/"
        "/.gitignore"
        "/.vscode/"
        "/.vs/"
        "/CMakeCache.txt"
        "/CMakeFiles/"
        "/cmake_install.cmake"
        "/Makefile"
        "/install_manifest.txt"
        "/compile_commands.json"
        "/CTestTestfile.cmake"
        "/_CPack_Packages/"
        "/.DS_Store"
        "/Thumbs.db"
        "*.o"
        "*.a"
        "*.so"
        "*.dylib"
        "*.dll"
        "*.exe"
        "*.pdb"
        "*.ilk"
        "*.exp"
        "*.lib"
    )
endif()

# Print configuration summary
message(STATUS "")
message(STATUS "Configuration Summary:")
message(STATUS "")
message(STATUS "Configuration Summary:")
message(STATUS "  Project: ${PROJECT_NAME} ${PROJECT_VERSION}")
message(STATUS "  Product Version: ${PRODUCT_VERSION_DISPLAY} (${PRODUCT_VERSION_NAME})")
message(STATUS "  Platform: ${PLATFORM_NAME}")
message(STATUS "  Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "  C++ standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "  Shared libraries: ${BUILD_SHARED_LIBS}")
message(STATUS "  Tests enabled: ${ENABLE_TESTS}")
message(STATUS "  Packaging enabled: ${ENABLE_PACKAGING}")
message(STATUS "  SSL support: ${ENABLE_SSL}")
message(STATUS "  JSON support: ${ENABLE_JSON}")
message(STATUS "")
