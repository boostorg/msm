file(GLOB SOURCES "*.cpp")
if(CMAKE_CXX_STANDARD LESS 20)
    message(STATUS "MSM: CXX standard is below 20, excluding puml tests")
    list(FILTER SOURCES EXCLUDE REGEX "puml|Puml")
endif()

add_executable(boost_msm_tests
    EXCLUDE_FROM_ALL
    ${SOURCES}
    main.cpp
)
target_include_directories(boost_msm_tests PRIVATE ../include)
find_package(boost_serialization)
target_link_libraries(boost_msm_tests Boost::serialization)
target_compile_definitions(boost_msm_tests PRIVATE "BOOST_MSM_NONSTANDALONE_TEST")

add_test(boost_msm_tests boost_msm_tests)
