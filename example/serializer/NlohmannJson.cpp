// Copyright 2026 Christian Granzin
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

#include <boost/test/unit_test.hpp>
#include <stack>

// Deactivate the attributes container to showcase
// automatic detection of empty states with backmp11.
// Without this setting all empty states need to be
// instrumented with no-op reflect methods.
#define BOOST_MSM_FRONT_ATTRIBUTES_CONTAINER void

// Include headers for nlohmann json.
#include <nlohmann/json.hpp>

// JSON requires reflection.
#define DIM_SWITCH_FRONT_END_HAS_REFLECTION
#include "DimSwitch.hpp"

// Serializer for nlohmann json.
class nlohmann_json_serializer
{
    using json = nlohmann::json;

  public:
    nlohmann_json_serializer(json& json) : m_json({&json}) {}

    template <typename FrontEnd>
    void visit_front_end(FrontEnd& /*front_end*/)
    {
        static_assert(std::is_empty_v<FrontEnd>, "FrontEnd must have reflection");
    }

    template <typename FrontEnd, typename Reflect>
    void visit_front_end(FrontEnd& /*front_end*/, Reflect&& reflect)
    {
        auto& json_front_end = top()["front_end"];
        m_json.push(&json_front_end);
        reflect();
        m_json.pop();
    }

    template <typename Member>
    void visit_member(const char* key, Member& member)
    {
        top()[key] = member;
    }

    template <typename State>
    void visit_state(size_t state_id, State& /*state*/)
    {
        static_assert(std::is_empty_v<State>, "State must have reflection");
        auto& json_state = top()["states"][state_id];
        // Name is not required for serialization, but useful to
        // print a machine's state in a human-readable JSON string.
        json_state["name"] = to_string<State>();
    }

    template <typename State, typename Reflect>
    void visit_state(size_t state_id, State& /*state*/, Reflect&& reflect)
    {
        auto& json_state = top()["states"][state_id];
        // Name is not required for serialization, but useful to
        // print a machine's state in a human-readable JSON string.
        json_state["name"] = to_string<State>();
        m_json.push(&json_state);
        reflect();
        m_json.pop();
    }

  private:
    json& top()
    {
        return *m_json.top();
    }

    template <typename T>
    static std::string to_string()
    {
        return boost::core::demangled_name(typeid(T));
    }

    std::stack<json*> m_json;
};

// Deserializer for nlohmann json.
class nlohmann_json_deserializer
{
    using json = nlohmann::json;

  public:
    nlohmann_json_deserializer(const json& json) : m_json({&json}) {}

    template <typename FrontEnd>
    void visit_front_end(FrontEnd& /*front_end*/)
    {
        static_assert(std::is_empty_v<FrontEnd>, "FrontEnd must have reflection");
    }

    template <typename FrontEnd, typename Reflect>
    void visit_front_end(FrontEnd& /*front_end*/, Reflect&& reflect)
    {
        auto& json_state = top()["front_end"];
        m_json.push(&json_state);
        reflect();
        m_json.pop();
    }

    template <typename Member>
    void visit_member(const char* key, Member& member)
    {
        member = top().at(key);
    }

    template <typename State>
    void visit_state(size_t /*state_id*/, State& /*state*/)
    {
        static_assert(std::is_empty_v<State>, "State must have reflection");
    }

    template <typename State, typename Reflect>
    void visit_state(size_t state_id, State& /*state*/, Reflect&& reflect)
    {
        auto& json_state = top()["states"][state_id];
        m_json.push(&json_state);
        reflect();
        m_json.pop();
    }

  private:
    const json& top()
    {
        return *m_json.top();
    }

    std::stack<const json*> m_json;
};


void to_json(nlohmann::json& json, const DimSwitch& state_machine)
{
    backmp11::reflect(state_machine, nlohmann_json_serializer{json});
}

void from_json(const nlohmann::json& json, DimSwitch& state_machine)
{
    backmp11::reflect(state_machine, nlohmann_json_deserializer{json});
}

BOOST_AUTO_TEST_CASE(serialize_example)
{
    DimSwitch dim_switch;

    // The initial state is Off.
    dim_switch.start();
    BOOST_REQUIRE(dim_switch.is_state_active<Off>());

    nlohmann::json json = dim_switch;
    BOOST_REQUIRE(json.dump(4) == \
R"({
    "active_state_ids": [
        0
    ],
    "event_processing": false,
    "front_end": {
        "brightness": 0
    },
    "running": true,
    "states": [
        {
            "name": "Off"
        },
        {
            "name": "On"
        }
    ]
})");

    // Turn On and set brightness to 75.
    dim_switch.process_event(TurnOn{});
    BOOST_REQUIRE(dim_switch.is_state_active<On>());
    dim_switch.process_event(Dim{75});
    BOOST_REQUIRE(dim_switch.brightness = 75);

    json = dim_switch;
    BOOST_REQUIRE(json.dump(4) == \
R"({
    "active_state_ids": [
        1
    ],
    "event_processing": false,
    "front_end": {
        "brightness": 75
    },
    "running": true,
    "states": [
        {
            "name": "Off"
        },
        {
            "name": "On"
        }
    ]
})");
    
    // Deserialize the json into a new state machine.
    auto dim_switch_2 = json.get<DimSwitch>();

    // We have the same state as before.
    BOOST_REQUIRE(dim_switch_2.is_state_active<On>());
    BOOST_REQUIRE(dim_switch_2.brightness = 75);
}
