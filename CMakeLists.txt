cmake_minimum_required(VERSION 3.8)
project(wordle)

set(CMAKE_CXX_STANDARD 20)

macro(replace_cxx_flag pattern text)
    foreach (flag
        CMAKE_CXX_FLAGS CMAKE_CXX_FLAGS_DEBUG CMAKE_CXX_FLAGS_RELEASE
        CMAKE_CXX_FLAGS_MINSIZEREL CMAKE_CXX_FLAGS_RELWITHDEBINFO)

        string(REGEX REPLACE "${pattern}" "${text}" ${flag} "${${flag}}")

    endforeach()
endmacro()

macro(append_cxx_flag text)
    foreach (flag
        CMAKE_CXX_FLAGS CMAKE_CXX_FLAGS_DEBUG CMAKE_CXX_FLAGS_RELEASE
        CMAKE_CXX_FLAGS_MINSIZEREL CMAKE_CXX_FLAGS_RELWITHDEBINFO)

        string(APPEND ${flag} " ${text}")

    endforeach()
endmacro()

# Compiler options
if (MSVC)
    # Warning levels
    replace_cxx_flag("/W[0-4]" "/W4")
    append_cxx_flag("/WX")
    append_cxx_flag("/permissive-")
else()
    # TODO
endif()

if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
    # append_cxx_flag("-Wno-switch")
    # append_cxx_flag("-Wno-missing-field-initializers")
    append_cxx_flag("-fno-delayed-template-parsing")
    append_cxx_flag("-fno-ms-compatibility")
endif()

add_subdirectory(src)

# Custom target for running clang-format
add_custom_target(format
    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
    COMMAND scripts/run_clang_format.cmd)
