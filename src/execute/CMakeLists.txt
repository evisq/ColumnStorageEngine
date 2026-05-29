add_executable(hits_bench main.cpp query_builder.cpp query_registry.cpp)
target_link_libraries(hits_bench PRIVATE column_storage)
