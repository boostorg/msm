file(GLOB SOURCES "*.cpp")
# TODO:
# Fully install boost to test Serialize
list(FILTER SOURCES EXCLUDE REGEX "Serialize|BigWithFunctors")
# list(FILTER SOURCES INCLUDE REGEX "Simple")

add_executable(msm_backmp11_test
    ${SOURCES}
    # Anonymous.cpp
    main.cpp
)
target_include_directories(msm_backmp11_test PRIVATE ../include)
target_compile_definitions(msm_backmp11_test PRIVATE "BOOST_MSM_NONSTANDALONE_TEST")