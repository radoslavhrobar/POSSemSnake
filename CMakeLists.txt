add_executable(had)
target_sources(had PRIVATE main.c buffer.c syn_buffer.c snake.c shm.c)
target_link_libraries(had PRIVATE sockets-lib pthread rt)
target_compile_definitions(had PRIVATE _XOPEN_SOURCE=500)
