#pragma once

/**
 * @file version.hpp
 * @brief Version information for Vector Studio
 */

namespace vdb {

// Version information
#define VDB_VERSION_MAJOR 2
#define VDB_VERSION_MINOR 0
#define VDB_VERSION_PATCH 0

// String representation
#define VDB_VERSION "2.0.0"
#define VDB_VERSION_STRING "Vector Studio v2.0.0"

// API level (incremented on breaking changes)
#define VDB_API_LEVEL 2

// Build information (can be set by CMake)
#ifndef VDB_BUILD_DATE
#define VDB_BUILD_DATE "2026-01-04"
#endif

#ifndef VDB_GIT_COMMIT
#define VDB_GIT_COMMIT "unknown"
#endif

/**
 * @brief Get the version string
 * @return Version string in format "major.minor.patch"
 */
inline const char* get_version() {
    return VDB_VERSION;
}

/**
 * @brief Get the full version string with product name
 * @return Full version string
 */
inline const char* get_version_string() {
    return VDB_VERSION_STRING;
}

/**
 * @brief Get the API level
 * @return API level number
 */
inline int get_api_level() {
    return VDB_API_LEVEL;
}

/**
 * @brief Get the build date
 * @return Build date string
 */
inline const char* get_build_date() {
    return VDB_BUILD_DATE;
}

/**
 * @brief Get the git commit hash
 * @return Git commit hash string
 */
inline const char* get_git_commit() {
    return VDB_GIT_COMMIT;
}

/**
 * @brief Check if version is at least the specified version
 * @param major Major version number
 * @param minor Minor version number
 * @param patch Patch version number
 * @return true if current version >= specified version
 */
inline bool version_at_least(int major, int minor, int patch) {
    if (VDB_VERSION_MAJOR > major) return true;
    if (VDB_VERSION_MAJOR < major) return false;
    if (VDB_VERSION_MINOR > minor) return true;
    if (VDB_VERSION_MINOR < minor) return false;
    return VDB_VERSION_PATCH >= patch;
}

} // namespace vdb
