file(GLOB SOURCES "*.cpp")
# TODO:
# Fully install boost to test Serialize
list(FILTER SOURCES EXCLUDE REGEX "Serialize|BigWithFunctors")
# list(FILTER SOURCES INCLUDE REGEX "Anonymous")

add_executable(msm_test
    ${SOURCES}
    # Anonymous.cpp
    # AnonymousEuml.cpp
    # AnonymousAndGuard.cpp
    # BigWithFunctors.cpp
    # CompositeMachine.cpp
    # Entries.cpp
    main.cpp
)
target_include_directories(msm_test PRIVATE ../include)
target_compile_definitions(msm_test PRIVATE "BOOST_MSM_NONSTANDALONE_TEST")