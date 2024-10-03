cmake_minimum_required(VERSION 3.5)

project(SQLMudExtension)

include_directories(include)
include_directories(third_party/transform/include)
add_subdirectory(third_party)

set(SQLMUD_SOURCES
    sqlmud_extension.cpp mutation_generator.cpp
    ${SQLMUD_OBJECT_FILES})

build_static_extension(sqlmud ${SQLMUD_SOURCES})
set(PARAMETERS "-warnings")
build_loadable_extension(sqlmud ${PARAMETERS} ${SQLMUD_SOURCES})

install(
  TARGETS sqlmud_extension
  EXPORT "${DUCKDB_EXPORT_SET}"
  LIBRARY DESTINATION "${INSTALL_LIB_DIR}"
  ARCHIVE DESTINATION "${INSTALL_LIB_DIR}")