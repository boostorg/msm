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

# Build tests standalone
get_filename_component(PARENT_DIR ${CMAKE_CURRENT_SOURCE_DIR} DIRECTORY)
if(PARENT_DIR STREQUAL CMAKE_SOURCE_DIR)
    target_include_directories(boost_msm_tests PRIVATE ../include)
    find_package(Boost COMPONENTS serialization REQUIRED)
    target_link_libraries(boost_msm_tests Boost::serialization)
# Build tests as part of the super-project
else()    
    target_link_libraries(boost_msm_tests Boost::msm)
endif()

target_compile_definitions(boost_msm_tests PRIVATE "BOOST_MSM_NONSTANDALONE_TEST")

add_test(boost_msm_tests boost_msm_tests)
