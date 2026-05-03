if(BOOST_MSM_IS_ROOT)
    find_package(Boost REQUIRED COMPONENTS unit_test_framework)
endif()

if(NOT TARGET examples)
    add_custom_target(examples)
endif()

link_libraries(Boost::msm Boost::unit_test_framework)
add_compile_definitions("BOOST_MSM_NONSTANDALONE_TEST")
if(BOOST_MSM_TEST_ONLY_BACKMP11)
    add_compile_definitions("BOOST_MSM_EXAMPLES_ONLY_BACKMP11")
endif()

add_subdirectory(serializer)
