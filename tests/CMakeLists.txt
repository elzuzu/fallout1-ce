cmake_minimum_required(VERSION 3.13)

add_executable(f1_tests
    fps_limiter_test.cpp
    vulkan_available_test.cpp
    test_batch.cpp
    test_cache.cpp
)

target_include_directories(f1_tests PRIVATE
    ${SDL2_INCLUDE_DIRS}
    ${CMAKE_SOURCE_DIR}/src
)

target_link_libraries(f1_tests
    ${SDL2_LIBRARIES}
    Vulkan::Vulkan
)

add_test(NAME f1_basic_tests COMMAND f1_tests)
