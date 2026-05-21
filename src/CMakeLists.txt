# STATIC: CMake компилирует .cpp сам и линкует в архив.
# INTERFACE нельзя — оно не компилирует .cpp, только пробрасывает их потребителям.
add_library(column_storage STATIC
    column/col_util.cpp
    column/batch.cpp
    reader/reader_CSV.cpp
    reader/parser.cpp
    reader/serialize.cpp
)

# PUBLIC: путь нужен и самой библиотеке (при компиляции .cpp),
# и любому исполняемому файлу, который её слинкует.
target_include_directories(column_storage PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

add_subdirectory(execute)
