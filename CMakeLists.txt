cmake_minimum_required(VERSION 3.5)

# Set extension name here
set(TARGET_NAME sqlmud)

# DuckDB's extension distribution supports vcpkg. As such, dependencies can be added in ./vcpkg.json and then
# used in cmake with find_package. Feel free to remove or replace with other dependencies.
# Note that it should also be removed from vcpkg.json to prevent needlessly installing it..
find_package(OpenSSL REQUIRED)

set(EXTENSION_NAME ${TARGET_NAME}_extension)
set(LOADABLE_EXTENSION_NAME ${TARGET_NAME}_loadable_extension)

project(${TARGET_NAME})
include_directories(src/third_party/transform/include)
include_directories(src/include)

add_subdirectory(src/third_party/transform)

set(EXTENSION_SOURCES 
  src/sqlmud_extension.cpp
  src/mutation_generator.cpp
  src/third_party/transform/transformer.cpp
  )

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

# cmake_minimum_required(VERSION 3.5)

# # Project name
# project(DuckDBExtensionProject)

# # Set the C++ standard
# set(CMAKE_CXX_STANDARD 14)
# set(CMAKE_CXX_STANDARD_REQUIRED True)

# # Add the path to the duckdb_static.a file
# # Make sure this path is correct
# set(DUCKDB_STATIC_LIB "${CMAKE_SOURCE_DIR}/build/release/src/libduckdb_static.a")
# set(FASTPFORLIB_STATIC_LIB "${CMAKE_SOURCE_DIR}/build/release/third_party/fastpforlib/libduckdb_fastpforlib.a")
# set(FMT_STATIC_LIB "${CMAKE_SOURCE_DIR}/build/release/third_party/fmt/libduckdb_fmt.a")
# set(FSST_STATIC_LIB "${CMAKE_SOURCE_DIR}/build/release/third_party/fsst/libduckdb_fsst.a")
# set(HYPERLOGLOG_STATIC_LIB "${CMAKE_SOURCE_DIR}/build/release/third_party/hyperloglog/libduckdb_hyperloglog.a")
# set(IMDB_STATIC_LIB "${CMAKE_SOURCE_DIR}/build/release/third_party/imdb/libimdb.a")
# set(LIBPG_QUERY_STATIC_LIB "${CMAKE_SOURCE_DIR}/build/release/third_party/libpg_query/libduckdb_pg_query.a")
# set(MBEDTLS_STATIC_LIB "${CMAKE_SOURCE_DIR}/build/release/third_party/mbedtls/libduckdb_mbedtls.a")
# set(MINIZ_STATIC_LIB "${CMAKE_SOURCE_DIR}/build/release/third_party/miniz/libduckdb_miniz.a")
# set(RE2_STATIC_LIB "${CMAKE_SOURCE_DIR}/build/release/third_party/re2/libduckdb_re2.a")
# set(SKIPLIST_STATIC_LIB "${CMAKE_SOURCE_DIR}/build/release/third_party/skiplist/libduckdb_skiplistlib.a")
# set(UTF8PROC_STATIC_LIB "${CMAKE_SOURCE_DIR}/build/release/third_party/utf8proc/libduckdb_utf8proc.a")
# set(YYJSON "${CMAKE_SOURCE_DIR}/build/release/third_party/yyjson/libduckdb_yyjson.a")
# set(CRYPTO_STATIC_LIB "${CMAKE_SOURCE_DIR}/build/release/vcpkg_installed/x64-linux/lib/libcrypto.a")
# set(SSL_STATIC_LIB "${CMAKE_SOURCE_DIR}/build/release/vcpkg_installed/x64-linux/lib/libssl.a")
# set(JEMALLOC_STATIC_LIB "${CMAKE_SOURCE_DIR}/build/release/extension/jemalloc/libjemalloc_extension.a")
# set(PARQUET_STATIC_LIB "${CMAKE_SOURCE_DIR}/build/release/extension/parquet/libparquet_extension.a")
# set(QUACK_STATIC_LIB "${CMAKE_SOURCE_DIR}/build/release/extension/quack/libquack_extension.a")

# # Include DuckDB's headers
# include_directories(
#   ${CMAKE_SOURCE_DIR}/duckdb/src/include
#   ${CMAKE_SOURCE_DIR}/duckdb/src/include/duckdb
# )

# # Add your src directory and the source files inside it (like main.cpp)
# add_executable(DuckDBExtension src/main.cpp)

# # Link the static library
# target_link_libraries(DuckDBExtension 
#   ${DUCKDB_STATIC_LIB}
#   ${FASTPFORLIB_STATIC_LIB}
#   ${FMT_STATIC_LIB}
#   ${FSST_STATIC_LIB}
#   ${HYPERLOGLOG_STATIC_LIB}
#   ${IMDB_STATIC_LIB}
#   ${LIBPG_QUERY_STATIC_LIB}
#   ${MBEDTLS_STATIC_LIB}
#   ${MINIZ_STATIC_LIB}
#   ${RE2_STATIC_LIB}
#   ${SKIPLIST_STATIC_LIB}
#   ${UTF8PROC_STATIC_LIB}
#   ${YYJSON}
#   ${CRYPTO_STATIC_LIB}
#   ${SSL_STATIC_LIB}
#   ${JEMALLOC_STATIC_LIB}
#   ${PARQUET_STATIC_LIB}
#   ${QUACK_STATIC_LIB}
# )
