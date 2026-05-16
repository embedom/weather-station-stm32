include_guard(GLOBAL)

option(BOOTSTRAP_ARM_TOOLCHAIN "Download and extract ARM GNU Toolchain" ON)

set(ARM_TOOLCHAIN_VERSION "15.2.rel1" CACHE STRING "ARM GNU Toolchain version")

# Detect host platform for archive selection
if(CMAKE_HOST_SYSTEM_NAME STREQUAL "Darwin")
    set(_host_string "darwin-arm64")
elseif(CMAKE_HOST_SYSTEM_NAME STREQUAL "Linux")
    set(_host_string "x86_64")
else()
    message(FATAL_ERROR "[arm-toolchain] Unsupported host: ${CMAKE_HOST_SYSTEM_NAME}")
endif()

set(ARM_TOOLCHAIN_ARCHIVE_NAME
    "arm-gnu-toolchain-${ARM_TOOLCHAIN_VERSION}-${_host_string}-arm-none-eabi.tar.xz")
set(ARM_TOOLCHAIN_RELEASE_TAG "arm-gnu-toolchain-${ARM_TOOLCHAIN_VERSION}")

set(ARM_TOOLCHAIN_BASE_URL
    "https://github.com/embedom/tools/releases/download/${ARM_TOOLCHAIN_RELEASE_TAG}")

set(ARM_TOOLCHAIN_INSTALL_DIR
    "${CMAKE_SOURCE_DIR}/Tools/arm-none-eabi/${ARM_TOOLCHAIN_VERSION}")
set(ARM_TOOLCHAIN_BIN_DIR "${ARM_TOOLCHAIN_INSTALL_DIR}/bin")

if(BOOTSTRAP_ARM_TOOLCHAIN)
    if(NOT EXISTS "${ARM_TOOLCHAIN_BIN_DIR}/arm-none-eabi-gcc")
        set(_deps_dir "${CMAKE_BINARY_DIR}/_deps")
        set(_archive_path "${_deps_dir}/${ARM_TOOLCHAIN_ARCHIVE_NAME}")
        set(_sha256sums_path "${_deps_dir}/arm-toolchain-SHA256SUMS")

        file(MAKE_DIRECTORY "${_deps_dir}")

        # 1. Download SHA256SUMS
        message(STATUS "[arm-toolchain] Downloading SHA256SUMS ...")
        file(DOWNLOAD
            "${ARM_TOOLCHAIN_BASE_URL}/SHA256SUMS"
            "${_sha256sums_path}"
            STATUS _sha_status
            TLS_VERIFY ON
        )
        list(GET _sha_status 0 _sha_code)
        if(NOT _sha_code EQUAL 0)
            list(GET _sha_status 1 _sha_msg)
            message(FATAL_ERROR
                "[arm-toolchain] SHA256SUMS download failed: ${_sha_code} ${_sha_msg}")
        endif()

        # 2. Parse expected hash from SHA256SUMS
        file(STRINGS "${_sha256sums_path}" _sha_file_lines)
        set(_expected_hash "")
        foreach(_line IN LISTS _sha_file_lines)
            if(_line MATCHES "${ARM_TOOLCHAIN_ARCHIVE_NAME}")
                string(REGEX MATCH "^([a-fA-F0-9]+)" _hash_match "${_line}")
                set(_expected_hash "${CMAKE_MATCH_1}")
                break()
            endif()
        endforeach()

        if("${_expected_hash}" STREQUAL "")
            message(FATAL_ERROR
                "[arm-toolchain] Hash for '${ARM_TOOLCHAIN_ARCHIVE_NAME}' not found in SHA256SUMS")
        endif()
        message(STATUS "[arm-toolchain] Expected SHA256: ${_expected_hash}")

        # 3. Download archive (verified against SHA256SUMS)
        message(STATUS "[arm-toolchain] Downloading ${ARM_TOOLCHAIN_ARCHIVE_NAME} ...")
        file(DOWNLOAD
            "${ARM_TOOLCHAIN_BASE_URL}/${ARM_TOOLCHAIN_ARCHIVE_NAME}"
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
                "[arm-toolchain] Archive download failed: ${_download_code} ${_download_msg}")
        endif()

        # 4. Extract archive to temporary directory
        set(_extract_tmp "${_deps_dir}/arm-toolchain-extract")
        file(REMOVE_RECURSE "${_extract_tmp}")
        file(MAKE_DIRECTORY "${_extract_tmp}")

        message(STATUS "[arm-toolchain] Extracting archive ...")
        file(ARCHIVE_EXTRACT INPUT "${_archive_path}" DESTINATION "${_extract_tmp}")

        # 5. Move into final location (Tools/arm-none-eabi/${ARM_TOOLCHAIN_VERSION})
        # Archive contains a single top-level directory – relocate it.
        file(GLOB _extracted_children "${_extract_tmp}/*")
        list(LENGTH _extracted_children _num_children)
        if(_num_children EQUAL 1)
            list(GET _extracted_children 0 _src_dir)
        else()
            set(_src_dir "${_extract_tmp}")
        endif()

        file(REMOVE_RECURSE "${ARM_TOOLCHAIN_INSTALL_DIR}")
        file(MAKE_DIRECTORY "${CMAKE_SOURCE_DIR}/Tools/arm-none-eabi")
        file(RENAME "${_src_dir}" "${ARM_TOOLCHAIN_INSTALL_DIR}")
        file(REMOVE_RECURSE "${_extract_tmp}")

        message(STATUS "[arm-toolchain] Installed to ${ARM_TOOLCHAIN_INSTALL_DIR}")
    else()
        message(STATUS "[arm-toolchain] Found at ${ARM_TOOLCHAIN_INSTALL_DIR}")
    endif()
endif()