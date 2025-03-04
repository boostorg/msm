add_executable(bench_back Back.cpp)
add_executable(bench_back_favor_compile_time BackFavorCompileTime.cpp)
add_executable(bench_back11 Back11.cpp)
add_executable(bench_backmp11 Backmp11.cpp)
add_executable(bench_backmp11_favor_compile_time Backmp11FavorCompileTime.cpp)

add_executable(bench_back_nested_fsm BackNestedFsm.cpp)
add_executable(bench_back_nested_fsm_favor_compile_time BackNestedFsmFavorCompileTime.cpp)
add_executable(bench_backmp11_nested_fsm Backmp11NestedFsm.cpp)
add_executable(bench_backmp11_nested_fsm_favor_compile_time Backmp11NestedFsmFavorCompileTime.cpp)
add_executable(bench_backmp11_nestesd_fsm_multi_cu
    backmp11_nested_fsm_multi_cu/fsm0.cpp
    backmp11_nested_fsm_multi_cu/fsm1.cpp
    backmp11_nested_fsm_multi_cu/fsm2.cpp
    backmp11_nested_fsm_multi_cu/test.cpp
    )

add_test(bench_back bench_back)
add_test(bench_back_favor_compile_time bench_back_favor_compile_time)
add_test(bench_back11 bench_back11)
add_test(bench_backmp11 bench_backmp11)
add_test(bench_backmp11_favor_compile_time bench_backmp11_favor_compile_time)

add_test(bench_back_nested_fsm bench_back_nested_fsm)
add_test(bench_back_nested_fsm_favor_compile_time bench_back_nested_fsm_favor_compile_time)
add_test(bench_backmp11_nested_fsm bench_backmp11_nested_fsm)
add_test(bench_backmp11_nested_fsm_favor_compile_time bench_backmp11_nested_fsm_favor_compile_time)
add_test(bench_backmp11_nestesd_fsm_multi_cu bench_backmp11_nestesd_fsm_multi_cu)


add_custom_target(benchmarks_backmp11)
add_dependencies(benchmarks_backmp11
    bench_backmp11
    bench_backmp11_favor_compile_time
    bench_backmp11_nested_fsm
    bench_backmp11_nested_fsm_favor_compile_time
    # bench_backmp11_nestesd_fsm_multi_cu
)

add_custom_target(benchmarks)
add_dependencies(benchmarks
    # bench_back
    # bench_back_favor_compile_time
    # bench_back11
    bench_backmp11
    bench_backmp11_favor_compile_time
)

add_custom_target(benchmarks_nested_fsm)
add_dependencies(benchmarks_nested_fsm
    # bench_back_nested_fsm
    # bench_back_nested_fsm_favor_compile_time
    bench_backmp11_nested_fsm
    bench_backmp11_nested_fsm_favor_compile_time
)