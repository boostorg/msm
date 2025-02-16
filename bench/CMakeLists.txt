add_executable(bench_back Back.cpp)
add_executable(bench_back_favor_compile_time BackFavorCompileTime.cpp)
add_executable(bench_back11 Back11.cpp)
add_executable(bench_backmp11 Backmp11.cpp)
add_executable(bench_backmp11_favor_compile_time Backmp11FavorCompileTime.cpp)

add_test(bench_back bench_back)
add_test(bench_back_favor_compile_time bench_back_favor_compile_time)
add_test(bench_back11 bench_back11)
add_test(bench_backmp11 bench_backmp11)
add_test(bench_backmp11_favor_compile_time bench_backmp11_favor_compile_time)


add_custom_target(benchmarks)
add_dependencies(benchmarks
    bench_back
    bench_back_favor_compile_time
    bench_back11
    bench_backmp11
    bench_backmp11_favor_compile_time
)