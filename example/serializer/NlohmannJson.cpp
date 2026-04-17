// Copyright 2026 Christian Granzin
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

#include <boost/test/unit_test.hpp>
#include <stack>

// Deactivate the attributes container to showcase
// automatic detection of empty states with backmp11.
#define BOOST_MSM_FRONT_ATTRIBUTES_CONTAINER void

// Include headers for nlohmann json.
#include <nlohmann/json.hpp>

#define DIM_SWITCH_HAS_REFLECTION
#include "DimSwitch.hpp"

// Serializer for nlohmann json.
class nlohmann_json_serializer
{
    using json = nlohmann::json;

  public:
    nlohmann_json_serializer(json& json) : m_json({&json}) {}

    // Serialize a member.
    template <typename Member>
    void operator()(const char* key, Member& member)
    {
        top()[key] = member;
    }

    // Serialize a substate without reflection.
    template <typename State>
    void operator()(int state_id, State& state)
    {
        static_assert(std::is_empty_v<State>, "State must have reflection");
        top()["states"][state_id]["name"] = to_string<State>();
    }

    // Serialize a state with reflection.
    template <typename State, typename F>
    void operator()(int state_id, State& /*state*/, F&& f)
    {
        auto& json_state = top()["states"][state_id];
        json_state["name"] = to_string<State>();
        m_json.push(&json_state);
        f();
        m_json.pop();
    }

    // TODO:
    // For front-ends think about future tags.

    // Serialize a front-end without data.
    template <typename State>
    void operator()(front::composite_state_tag, State& state)
    {
        static_assert(std::is_empty_v<State>, "State must have reflection");
    }

    // Serialize a front-end with reflection.
    template <typename State, typename F>
    void operator()(front::composite_state_tag, State& /*state*/, F&& f)
    {
        auto& json_state = top()["front_end"];
        m_json.push(&json_state);
        f();
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

// Serializer for nlohmann json.
class nlohmann_json_deserializer
{
    using json = nlohmann::json;

  public:
    nlohmann_json_deserializer(const json& json) : m_json({&json}) {}

    // Deserialize a member.
    template <typename Member>
    void operator()(const char* key, Member& member)
    {
        member = top().at(key);
    }

    // Deserialize a substate without data.
    template <typename State>
    void operator()(size_t state_id, State& state)
    {
        static_assert(std::is_empty_v<State>, "State must have reflection");
    }

    // Deserialize a state with reflection.
    template <typename State, typename F>
    void operator()(size_t state_id, State& /*state*/, F&& f)
    {
        auto& json_state = top()["states"][state_id];
        m_json.push(&json_state);
        f();
        m_json.pop();
    }

    // TODO:
    // For front-ends think about future tags.

    // Serialize a front-end without data.
    template <typename State>
    void operator()(front::composite_state_tag, State& state)
    {
        static_assert(std::is_empty_v<State>, "State must have reflection");
    }

    // Serialize a front-end with reflection.
    template <typename State, typename F>
    void operator()(front::composite_state_tag, State& /*state*/, F&& f)
    {
        auto& json_state = top()["front_end"];
        m_json.push(&json_state);
        f();
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
    back::reflect(state_machine, nlohmann_json_serializer{json});
}

void from_json(const nlohmann::json& json, DimSwitch& state_machine)
{
    back::reflect(state_machine, nlohmann_json_deserializer{json});
}

BOOST_AUTO_TEST_CASE(serialize_example)
{
    DimSwitch dim_switch;

    // The initial state is Off.
    dim_switch.start();
    BOOST_REQUIRE(dim_switch.is_state_active<Off>());

    // Turn On and set brightness to 75.
    dim_switch.process_event(TurnOn{});
    dim_switch.process_event(Dim{75});
    BOOST_REQUIRE(dim_switch.is_state_active<On>());
    BOOST_REQUIRE(dim_switch.brightness = 75);

    // Serialize the state machine.
    nlohmann::json json = dim_switch;
    BOOST_REQUIRE(json.dump(4) == \
R"({
    "active_state_ids": [
        1
    ],
    "event_processing": false,
    "front_end": {
        "brightness": 75
    },
    "initial_state_ids": [
        0
    ],
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
