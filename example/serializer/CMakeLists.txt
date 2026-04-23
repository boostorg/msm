add_executable(boost_msm_example_serializer_boost_serialization
    EXCLUDE_FROM_ALL
    BoostSerialization.cpp
    main.cpp
)
target_compile_features(boost_msm_example_serializer_boost_serialization PRIVATE cxx_std_17)
if(((DEFINED CMAKE_CXX_STANDARD)     AND (CMAKE_CXX_STANDARD GREATER_EQUAL 17)) OR
((NOT DEFINED CMAKE_CXX_STANDARD) AND ("cxx_std_17" IN_LIST CMAKE_CXX_COMPILE_FEATURES)))
    add_test(NAME boost_msm_example_serializer_boost_serialization COMMAND boost_msm_example_serializer_boost_serialization)
    add_dependencies(tests boost_msm_example_serializer_boost_serialization)
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
