# set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE "time -v")

add_executable(msm_bench
    # Back11LargeMachine10.cpp
    # Back11LargeMachine20.cpp
    # BackMp11LargeMachine10.cpp
    BackMp11LargeMachine15.cpp
    # BackMp11CompositeMachine.cpp
    # LargeMachine10.cpp
    # LargeMachine10OptCompileTime.cpp
    LargeMachine15.cpp
    main.cpp
)
target_include_directories(msm_bench PRIVATE ../include)
target_compile_options(msm_bench PRIVATE "-ftime-trace")
set_target_properties(msm_bench PROPERTIES
    RULE_LAUNCH_COMPILE "time -v"
)