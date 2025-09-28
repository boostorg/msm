file(GLOB SOURCES CONFIGURE_DEPENDS "*.cpp")

set(PUML_SOURCES "")
if(NOT DEFINED CMAKE_CXX_STANDARD)
    # If standard is not set on configure use separate target if C++20 is supported at all
    if("cxx_std_20" IN_LIST CMAKE_CXX_COMPILE_FEATURES)
        set(PUML_SOURCES ${SOURCES})
        list(FILTER PUML_SOURCES INCLUDE REGEX "puml|Puml")
    else()
        message(STATUS "MSM: Compiler doesn't support CXX 20 standard, excluding puml tests")
    endif()
    list(FILTER SOURCES EXCLUDE REGEX "puml|Puml")
elseif(CMAKE_CXX_STANDARD LESS 20)
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

if(PUML_SOURCES)
    add_executable(boost_msm_puml_tests
      EXCLUDE_FROM_ALL
      ${PUML_SOURCES}
      main.cpp
    )
    target_link_libraries(boost_msm_puml_tests Boost::msm Boost::unit_test_framework)
    target_compile_definitions(boost_msm_puml_tests PRIVATE "BOOST_MSM_NONSTANDALONE_TEST")
    target_compile_features(boost_msm_puml_tests PRIVATE cxx_std_20)

    add_test(NAME boost_msm_puml_tests COMMAND boost_msm_puml_tests)
    add_dependencies(tests boost_msm_puml_tests)
endif()
