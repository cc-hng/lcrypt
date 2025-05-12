# Add executable
file(GLOB_RECURSE sources CONFIGURE_DEPENDS "${CMAKE_CURRENT_LIST_DIR}/*.cpp")
set(bench_name "${PROJECT_NAME}-bench")
add_executable(${bench_name} ${sources})
target_link_libraries(${bench_name} PRIVATE ${PROJECT_NAME} nanobench)
target_compile_features(${bench_name} PRIVATE cxx_std_20)
