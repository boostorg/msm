file(GLOB SOURCES "*.cpp")
# list(FILTER SOURCES EXCLUDE REGEX "foo")
# list(FILTER SOURCES INCLUDE REGEX "foo")

add_executable(msm_test
    ${SOURCES}
    main.cpp
)
target_include_directories(msm_test PRIVATE ../include)
find_package(boost_serialization)
target_link_libraries(msm_test Boost::serialization)
target_compile_definitions(msm_test PRIVATE "BOOST_MSM_NONSTANDALONE_TEST")