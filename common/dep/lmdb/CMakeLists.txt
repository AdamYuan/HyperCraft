cmake_minimum_required(VERSION 3.15)

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

add_library(lmdb STATIC src/mdb.c src/midl.c)
target_include_directories(lmdb PUBLIC include)
target_link_libraries(lmdb PRIVATE Threads::Threads)

