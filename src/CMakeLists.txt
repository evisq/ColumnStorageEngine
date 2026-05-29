add_library(column_storage STATIC
    column/col_util.cpp
    column/batch.cpp
    reader/csv_reader.cpp
    reader/parser.cpp
    reader/serialize.cpp
)

target_include_directories(column_storage PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

add_subdirectory(execute)
