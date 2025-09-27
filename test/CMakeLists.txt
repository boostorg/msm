file(GLOB SOURCES CONFIGURE_DEPENDS "*.cpp")
if(CMAKE_CXX_STANDARD LESS 20)
    message(STATUS "MSM: CXX standard is below 20, excluding puml tests")
    list(FILTER SOURCES EXCLUDE REGEX "puml|Puml")
endif()

add_executable(boost_msm_tests
    EXCLUDE_FROM_ALL
    ${SOURCES}
    main.cpp
)
if(BOOST_MSM_IS_ROOT)
    find_package(Boost REQUIRED COMPONENTS unit_test_framework)
endif()
target_link_libraries(boost_msm_tests Boost::msm Boost::unit_test_framework)
target_compile_definitions(boost_msm_tests PRIVATE "BOOST_MSM_NONSTANDALONE_TEST")

add_test(NAME boost_msm_tests COMMAND boost_msm_tests)

if(NOT TARGET tests)
    add_custom_target(tests)
endif()
add_dependencies(tests boost_msm_tests)
