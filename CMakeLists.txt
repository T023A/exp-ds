cmake_minimum_required(VERSION 3.15)
project(exp_ds1)

set(CMAKE_CXX_STANDARD 17)

set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -ggdb -O3 -march=native -DNDEBUG -Wno-interference-size")

add_subdirectory(abseil-cpp/)
include_directories(abseil-cpp src)
add_executable(ds_profile1 profile/profile_1.cpp abseil-cpp/absl/container/internal/raw_hash_set.cc)
#target_link_libraries(ds_profile1 absl::strings absl::memory absl::base absl::hash)
target_link_libraries(ds_profile1 absl::base absl::hash)
