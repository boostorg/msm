add_executable(boost_msm_example_serializer_boost
    EXCLUDE_FROM_ALL
    BoostSerialization.cpp
    main.cpp
)
target_compile_features(boost_msm_example_serializer_boost PRIVATE cxx_std_17)
if(((DEFINED CMAKE_CXX_STANDARD)     AND (CMAKE_CXX_STANDARD GREATER_EQUAL 17)) OR
((NOT DEFINED CMAKE_CXX_STANDARD) AND ("cxx_std_17" IN_LIST CMAKE_CXX_COMPILE_FEATURES)))
    add_test(NAME boost_msm_example_serializer_boost COMMAND boost_msm_example_serializer_boost)
    add_dependencies(tests boost_msm_example_serializer_boost)
endif()

find_package(nlohmann_json)

if(nlohmann_json_FOUND)
    add_executable(boost_msm_example_serializer_nlohmann
        EXCLUDE_FROM_ALL
        NlohmannJson.cpp
        main.cpp
    )
    target_compile_features(boost_msm_example_serializer_nlohmann PRIVATE cxx_std_17)
    if(((DEFINED CMAKE_CXX_STANDARD)     AND (CMAKE_CXX_STANDARD GREATER_EQUAL 17)) OR
    ((NOT DEFINED CMAKE_CXX_STANDARD) AND ("cxx_std_17" IN_LIST CMAKE_CXX_COMPILE_FEATURES)))
        add_test(NAME boost_msm_example_serializer_nlohmann COMMAND boost_msm_example_serializer_nlohmann)
        add_dependencies(tests boost_msm_example_serializer_nlohmann)
    endif()
endif()
