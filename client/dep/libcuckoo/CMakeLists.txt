# we require the use of threads
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

# libcuckoo is an interface (all headers) library target
add_library(libcuckoo INTERFACE)
add_library(libcuckoo::libcuckoo ALIAS libcuckoo)

# tag libcuckoo target with a c++11 feature so that libcuckoo users
# will have c++11 turned on in their compile when they use this target.
# XXX: newer cmakes have a "cxx_std_11" feature that could be used
target_compile_features(libcuckoo INTERFACE cxx_constexpr)

# Include relative to the base directory
target_include_directories(libcuckoo INTERFACE .)

# switch on threading for all targets that link with libcuckoo
target_link_libraries(libcuckoo INTERFACE Threads::Threads)