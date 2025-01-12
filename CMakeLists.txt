add_executable(had)
target_sources(had PRIVATE names.c buffer.c shm.c syn_buffer.c pipe.c snake.c server.c client.c main.c)
target_link_libraries(had PRIVATE sockets-lib pthread rt)
target_compile_definitions(had PRIVATE _XOPEN_SOURCE=500)
