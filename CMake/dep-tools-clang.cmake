include_guard(GLOBAL)

option(BOOTSTRAP_CLANG_TOOLS "Download and extract clang tools archive" ON)
option(ENABLE_CLANG_TIDY "Run clang-tidy during build" ON)
option(ENABLE_CLANG_FORMAT "Run clang-format during build" ON)
option(CLANG_FORMAT_ON_BUILD "Auto-format sources on every build" ON)

set(CLANG_VERSION "22.1.3" CACHE STRING "Clang tools version")

# Detect host platform for archive selection
if(CMAKE_HOST_SYSTEM_NAME STREQUAL "Darwin")
    set(_host_string "mac")
elseif(CMAKE_HOST_SYSTEM_NAME STREQUAL "Linux")
    set(_host_string "linux")
else()
    message(FATAL_ERROR "[clang-tools] Unsupported host: ${CMAKE_HOST_SYSTEM_NAME}")
endif()

set(CLANG_TOOLS_DIR "${CMAKE_SOURCE_DIR}/Tools")
set(CLANG_TOOLS_ARCHIVE_NAME "clang-${_host_string}.tar.xz")
set(CLANG_TOOLS_RELEASE_TAG "llvm-clang-tools-v${CLANG_VERSION}")
set(CLANG_TOOLS_BASE_URL
    "https://github.com/embedom/tools/releases/download/${CLANG_TOOLS_RELEASE_TAG}")
set(CLANG_TOOLS_ARCHIVE "${CMAKE_BINARY_DIR}/_deps/${CLANG_TOOLS_ARCHIVE_NAME}")

if(BOOTSTRAP_CLANG_TOOLS)
    # Only download/extract if clang-tidy or clang-format not already found
    if(NOT EXISTS "${CLANG_TOOLS_DIR}/clang/bin/clang-tidy" OR
        NOT EXISTS "${CLANG_TOOLS_DIR}/clang/bin/clang-format")

        set(_deps_dir "${CMAKE_BINARY_DIR}/_deps")
        set(_sha256sums_path "${_deps_dir}/clang-tools-SHA256SUMS")
        file(MAKE_DIRECTORY "${_deps_dir}")

        # 1. Download SHA256SUMS
        message(STATUS "[clang-tools] Downloading SHA256SUMS ...")
        file(DOWNLOAD
            "${CLANG_TOOLS_BASE_URL}/SHA256SUMS"
            "${_sha256sums_path}"
            STATUS _sha_status
            TLS_VERIFY ON
        )
        list(GET _sha_status 0 _sha_code)
        if(NOT _sha_code EQUAL 0)
            list(GET _sha_status 1 _sha_msg)
            message(FATAL_ERROR
                "[clang-tools] SHA256SUMS download failed: ${_sha_code} ${_sha_msg}")
        endif()

        # 2. Parse expected hash from SHA256SUMS
        file(STRINGS "${_sha256sums_path}" _sha_file_lines)
        set(_expected_hash "")
        foreach(_line IN LISTS _sha_file_lines)
            if(_line MATCHES "${CLANG_TOOLS_ARCHIVE_NAME}")
                string(REGEX MATCH "^([a-fA-F0-9]+)" _hash_match "${_line}")
                set(_expected_hash "${CMAKE_MATCH_1}")
                break()
            endif()
        endforeach()

        if("${_expected_hash}" STREQUAL "")
            message(FATAL_ERROR "[clang-tools] Failed to parse hash from SHA256SUMS")
        endif()
        message(STATUS "[clang-tools] Expected SHA256: ${_expected_hash}")

        # 3. Download archive (verified against SHA256SUMS)
        message(STATUS "[clang-tools] Downloading ${CLANG_TOOLS_ARCHIVE_NAME} ...")
        file(DOWNLOAD
            "${CLANG_TOOLS_BASE_URL}/${CLANG_TOOLS_ARCHIVE_NAME}"
            "${CLANG_TOOLS_ARCHIVE}"
            SHOW_PROGRESS
            STATUS _download_status
            TLS_VERIFY ON
            EXPECTED_HASH "SHA256=${_expected_hash}"
        )
        list(GET _download_status 0 _download_code)
        if(NOT _download_code EQUAL 0)
            list(GET _download_status 1 _download_msg)
            file(REMOVE "${CLANG_TOOLS_ARCHIVE}")
            message(FATAL_ERROR
                "[clang-tools] Archive download failed: ${_download_code} ${_download_msg}")
        endif()

        # 4. Extract archive to Tools/clang
        file(REMOVE_RECURSE "${CLANG_TOOLS_DIR}/clang")
        file(MAKE_DIRECTORY "${CLANG_TOOLS_DIR}")
        file(ARCHIVE_EXTRACT INPUT "${CLANG_TOOLS_ARCHIVE}" DESTINATION "${CLANG_TOOLS_DIR}")
        message(STATUS "[clang-tools] Extracted to: ${CLANG_TOOLS_DIR}/clang")
    endif()
endif()

set(CLANG_TIDY_EXE "${CLANG_TOOLS_DIR}/clang/bin/clang-tidy")
set(CLANG_FORMAT_EXE "${CLANG_TOOLS_DIR}/clang/bin/clang-format")

# Fallback: try to find in PATH if not bootstrapped
if(NOT EXISTS "${CLANG_TIDY_EXE}")
    find_program(CLANG_TIDY_EXE clang-tidy)
endif()
if(NOT EXISTS "${CLANG_FORMAT_EXE}")
    find_program(CLANG_FORMAT_EXE clang-format)
endif()

# Auto-detect ARM GCC system include paths for clang-tidy
if(ENABLE_CLANG_TIDY AND EXISTS "${CLANG_TIDY_EXE}")
    # Detect include paths from arm-none-eabi-g++ (works on any machine/CI)
    execute_process(
        COMMAND ${CMAKE_CXX_COMPILER} -E -x c++ -Wp,-v /dev/null
        OUTPUT_QUIET
        ERROR_VARIABLE _gcc_verbose_output
        ERROR_STRIP_TRAILING_WHITESPACE
    )

    set(ARM_GCC_INCLUDE_PATHS "")
    set(_is_in_list FALSE)
    string(REPLACE "\n" ";" _gcc_output_list "${_gcc_verbose_output}")

    # Parse the output to extract include paths between the markers
    foreach(_line IN LISTS _gcc_output_list)
        if(_line MATCHES "#include <\\.\\.\\.> search starts here:")
            set(_is_in_list TRUE)
            continue()
        endif()
        if(_line MATCHES "End of search list\\.")
            set(_is_in_list FALSE)
            continue()
        endif()
        if(_is_in_list)
            string(STRIP "${_line}" _line) # Remove leading/trailing whitespace
            # Resolve relative paths (../) to absolute
            get_filename_component(_abs_path "${_line}" REALPATH)
            list(APPEND ARM_GCC_INCLUDE_PATHS "${_abs_path}") # Add to list of include paths
        endif()
    endforeach()

    if(NOT ARM_GCC_INCLUDE_PATHS)
        message(WARNING "Could not detect ARM GCC include paths for clang-tidy")
    endif()

    # Build clang-tidy extra args
    set(CLANG_TIDY_ARGS
        "--extra-arg=--target=arm-none-eabi"
        "--extra-arg=-nostdinc"
    )
    foreach(INC_PATH IN LISTS ARM_GCC_INCLUDE_PATHS)
        list(APPEND CLANG_TIDY_ARGS "--extra-arg=-isystem${INC_PATH}")
    endforeach()

    list(JOIN CLANG_TIDY_ARGS ";" _CLANG_TIDY_ARGS_STR)
    set(CMAKE_C_CLANG_TIDY   "${CLANG_TIDY_EXE};${_CLANG_TIDY_ARGS_STR}")
    set(CMAKE_CXX_CLANG_TIDY "${CLANG_TIDY_EXE};${_CLANG_TIDY_ARGS_STR}")
    message(STATUS "clang-tidy enabled: ${CLANG_TIDY_EXE}")
endif()

# Skip clang-tidy for library/vendor sources (Drivers, Middlewares, SeggerRTT)
if(ENABLE_CLANG_TIDY)
    file(GLOB_RECURSE _SKIP_SOURCES
        ${CMAKE_SOURCE_DIR}/Drivers/*.c    ${CMAKE_SOURCE_DIR}/Drivers/*.cpp
        ${CMAKE_SOURCE_DIR}/Middlewares/*.c ${CMAKE_SOURCE_DIR}/Middlewares/*.cpp
        ${CMAKE_SOURCE_DIR}/SeggerRTT/*.c  ${CMAKE_SOURCE_DIR}/SeggerRTT/*.cpp
    )
    set_source_files_properties(${_SKIP_SOURCES} PROPERTIES SKIP_LINTING ON)
endif()

# clang-format: custom target 'format' to format all project sources in-place
if(ENABLE_CLANG_FORMAT AND EXISTS "${CLANG_FORMAT_EXE}")
    file(GLOB_RECURSE ALL_FORMAT_SOURCES
        ${CMAKE_SOURCE_DIR}/Core/*.cpp  ${CMAKE_SOURCE_DIR}/Core/*.hpp
        ${CMAKE_SOURCE_DIR}/Core/*.c    ${CMAKE_SOURCE_DIR}/Core/*.h
        ${CMAKE_SOURCE_DIR}/Config/*.h  ${CMAKE_SOURCE_DIR}/Config/*.hpp
    )
    
    if(CLANG_FORMAT_ON_BUILD)
        # Add clang-format as a pre-build step for the main target
        add_custom_target(format
            COMMAND ${CLANG_FORMAT_EXE} -i ${ALL_FORMAT_SOURCES}
            WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
            COMMENT "Running clang-format on project sources"
            VERBATIM
        )
        # check-format: dry-run that fails on unformatted files (useful in CI)
        # add_custom_target(check-format
        #     COMMAND ${CLANG_FORMAT_EXE} --dry-run --Werror ${ALL_FORMAT_SOURCES}
        #     WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        #     COMMENT "Checking clang-format compliance"
        #     VERBATIM
        # )
    endif()
    message(STATUS "clang-format targets available: 'format', 'check-format'")
endif()
