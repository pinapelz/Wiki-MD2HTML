cmake_minimum_required(VERSION 3.16)
project(FFXIV-MD-GENERATOR VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if (MSVC)
    add_compile_options(/W4 /permissive-)
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

message(STATUS "Source directory: ${CMAKE_SOURCE_DIR}")
message(STATUS "Binary directory: ${CMAKE_BINARY_DIR}")

# Find OpenSSL for AES-GCM / PBKDF2 usage
find_package(OpenSSL REQUIRED)

add_executable(${PROJECT_NAME}
    src/main.cpp
    src/markdown_translator.cpp
    src/utils/rclone_uploader.cpp
)

target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_SOURCE_DIR}/include)

# Link OpenSSL libraries
target_link_libraries(${PROJECT_NAME} PRIVATE OpenSSL::Crypto OpenSSL::SSL)

add_custom_target(copy_styles ALL
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_SOURCE_DIR}/styles
            ${CMAKE_BINARY_DIR}/styles
    COMMENT "Copying styles directory to build directory"
    VERBATIM
)

add_dependencies(${PROJECT_NAME} copy_styles)
file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/styles)
