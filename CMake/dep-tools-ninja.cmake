include_guard(GLOBAL)

option(BOOTSTRAP_NINJA "Download and extract Ninja build system" ON)

set(NINJA_VERSION "1.13.2" CACHE STRING "Ninja build system version")

# Detect host platform for archive selection
if(CMAKE_HOST_SYSTEM_NAME STREQUAL "Darwin")
    set(_ninja_host_string "mac")
elseif(CMAKE_HOST_SYSTEM_NAME STREQUAL "Linux")
    set(_ninja_host_string "linux")
else()
    message(FATAL_ERROR "[ninja] Unsupported host: ${CMAKE_HOST_SYSTEM_NAME}")
endif()

set(NINJA_ARCHIVE_NAME "ninja-${_ninja_host_string}.tar.xz")
set(NINJA_RELEASE_TAG "ninja-v${NINJA_VERSION}")

set(NINJA_BASE_URL
    "https://github.com/embedom/tools/releases/download/${NINJA_RELEASE_TAG}")

set(NINJA_INSTALL_DIR "${CMAKE_SOURCE_DIR}/Tools/ninja")
set(NINJA_EXECUTABLE "${NINJA_INSTALL_DIR}/ninja")

if(BOOTSTRAP_NINJA)
    if(NOT EXISTS "${NINJA_EXECUTABLE}")
        set(_deps_dir "${CMAKE_BINARY_DIR}/_deps")
        set(_archive_path "${_deps_dir}/${NINJA_ARCHIVE_NAME}")
        set(_sha256sums_path "${_deps_dir}/ninja-SHA256SUMS")

        file(MAKE_DIRECTORY "${_deps_dir}")

        # 1. Download SHA256SUMS
        message(STATUS "[ninja] Downloading SHA256SUMS ...")
        file(DOWNLOAD
            "${NINJA_BASE_URL}/SHA256SUMS"
            "${_sha256sums_path}"
            STATUS _sha_status
            TLS_VERIFY ON
        )
        list(GET _sha_status 0 _sha_code)
        if(NOT _sha_code EQUAL 0)
            list(GET _sha_status 1 _sha_msg)
            message(FATAL_ERROR
                "[ninja] SHA256SUMS download failed: ${_sha_code} ${_sha_msg}")
        endif()

        # 2. Parse expected hash from SHA256SUMS
        file(STRINGS "${_sha256sums_path}" _sha_file_lines)
        set(_expected_hash "")
        foreach(_line IN LISTS _sha_file_lines)
            if(_line MATCHES "${NINJA_ARCHIVE_NAME}")
                string(REGEX MATCH "^([a-fA-F0-9]+)" _hash_match "${_line}")
                set(_expected_hash "${CMAKE_MATCH_1}")
                break()
            endif()
        endforeach()

        if("${_expected_hash}" STREQUAL "")
            message(FATAL_ERROR
                "[ninja] Hash for '${NINJA_ARCHIVE_NAME}' not found in SHA256SUMS")
        endif()
        message(STATUS "[ninja] Expected SHA256: ${_expected_hash}")

        # 3. Download archive (verified against SHA256SUMS)
        message(STATUS "[ninja] Downloading ${NINJA_ARCHIVE_NAME} ...")
        file(DOWNLOAD
            "${NINJA_BASE_URL}/${NINJA_ARCHIVE_NAME}"
            "${_archive_path}"
            SHOW_PROGRESS
            STATUS _download_status
            TLS_VERIFY ON
            EXPECTED_HASH "SHA256=${_expected_hash}"
        )
        list(GET _download_status 0 _download_code)
        if(NOT _download_code EQUAL 0)
            list(GET _download_status 1 _download_msg)
            file(REMOVE "${_archive_path}")
            message(FATAL_ERROR
                "[ninja] Archive download failed: ${_download_code} ${_download_msg}")
        endif()

        # 4. Extract archive
        file(REMOVE_RECURSE "${NINJA_INSTALL_DIR}")
        file(MAKE_DIRECTORY "${NINJA_INSTALL_DIR}")
        file(ARCHIVE_EXTRACT INPUT "${_archive_path}" DESTINATION "${NINJA_INSTALL_DIR}")
        message(STATUS "[ninja] Installed to ${NINJA_INSTALL_DIR}")
    else()
        message(STATUS "[ninja] Found at ${NINJA_INSTALL_DIR}")
    endif()
endif()

# Point CMake to the bootstrapped ninja
if(EXISTS "${NINJA_EXECUTABLE}")
    set(CMAKE_MAKE_PROGRAM "${NINJA_EXECUTABLE}" CACHE FILEPATH "Path to Ninja" FORCE)
endif()
