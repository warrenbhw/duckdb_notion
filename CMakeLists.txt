cmake_minimum_required(VERSION 3.5)

# Set extension name here
set(TARGET_NAME notion)

# DuckDB's extension distribution supports vcpkg. As such, dependencies can be added in ./vcpkg.json and then
# used in cmake with find_package. Feel free to remove or replace with other dependencies.
find_package(OpenSSL REQUIRED)

set(EXTENSION_NAME ${TARGET_NAME}_extension)
set(LOADABLE_EXTENSION_NAME ${TARGET_NAME}_loadable_extension)

project(${TARGET_NAME})
include_directories(src/include)
include_directories(third_party)

# Extension sources
set(EXTENSION_SOURCES 
    src/notion_extension.cpp
    src/notion_requests.cpp
    src/notion_utils.cpp
    src/notion_auth.cpp
    # src/notion_copy.cpp
    # src/notion_read.cpp
)

# Build extension
build_static_extension(${TARGET_NAME} ${EXTENSION_SOURCES})
build_loadable_extension(${TARGET_NAME} " " ${EXTENSION_SOURCES})

# Link OpenSSL in both the static library as the loadable extension
target_link_libraries(${EXTENSION_NAME} OpenSSL::SSL OpenSSL::Crypto)
target_link_libraries(${LOADABLE_EXTENSION_NAME} OpenSSL::SSL OpenSSL::Crypto)

install(
  TARGETS ${EXTENSION_NAME}
  EXPORT "${DUCKDB_EXPORT_SET}"
  LIBRARY DESTINATION "${INSTALL_LIB_DIR}"
  ARCHIVE DESTINATION "${INSTALL_LIB_DIR}")

# Unit tests
if (NOT CLANG_TIDY)
    find_package(Catch2 3 REQUIRED)

    # Test sources
    set(TEST_SOURCES
        test/test_main.cpp
        test/notion_test.cpp
        ${EXTENSION_SOURCES}
    )

    # Create test executable
    add_executable(notion_test test/notion_test.cpp)

    # Link test dependencies
    target_link_libraries(notion_test PRIVATE
        duckdb_static
        OpenSSL::SSL 
        OpenSSL::Crypto
        Catch2::Catch2WithMain
    )

    # Include directories for tests
    target_include_directories(${TARGET_NAME}_test PRIVATE
        ${CMAKE_SOURCE_DIR}/src/include
        ${OPENSSL_INCLUDE_DIR}
        ${DUCKDB_INCLUDE_DIRS}
    )
endif()
