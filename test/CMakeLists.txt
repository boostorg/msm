file(GLOB SOURCES "*.cpp")
# TODO:
# Fully install boost to test Serialize
list(FILTER SOURCES EXCLUDE REGEX "Serialize")
# list(FILTER SOURCES INCLUDE REGEX "Anonymous")

add_executable(msm_test
    # ${SOURCES}
    # Anonymous.cpp
    # AnonymousAndGuard.cpp
    # AnonymousEuml.cpp
    # BigWithFunctors.cpp
    # CompositeEuml.cpp
    # CompositeMachine.cpp
    Constructor.cpp
    Entries.cpp
    EventQueue.cpp
    History.cpp
    main.cpp
)
target_include_directories(msm_test PRIVATE ../include)
target_compile_definitions(msm_test PRIVATE "BOOST_MSM_NONSTANDALONE_TEST")