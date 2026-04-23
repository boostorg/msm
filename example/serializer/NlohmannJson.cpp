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
#define BOOST_MSM_FRONT_DISABLE_STATE_ATTRIBUTES_CONTAINER

// Include headers for nlohmann json.
#include <nlohmann/json.hpp>

#include "DimSwitch.hpp"

// Serializer for nlohmann json.
class nlohmann_json_serializer
{
    using json = nlohmann::json;

  public:
    nlohmann_json_serializer(json& json) : m_json({&json}) {}

    template <typename FrontEnd>
    void visit_front_end(FrontEnd&& /*front_end*/)
    {
        static_assert(std::is_empty_v<std::decay_t<FrontEnd>>,
                      "FrontEnd must implement reflect(...)");
    }

    template <typename FrontEnd, typename Reflect>
    void visit_front_end(FrontEnd&& /*front_end*/, Reflect&& reflect)
    {
        auto& json_front_end = top()["front_end"];
        m_json.push(&json_front_end);
        reflect();
        m_json.pop();
    }

    template <typename Member>
    void visit_member(const char* key, Member&& member)
    {
        top()[key] = member;
    }

    template <typename State>
    void visit_state(size_t state_id, State&& /*state*/)
    {
        static_assert(std::is_empty_v<std::decay_t<State>>,
                      "State must implement reflect(...)");
    }

    template <typename State, typename Reflect>
    void visit_state(size_t state_id, State&& /*state*/, Reflect&& reflect)
    {
        auto& json_state = top()["states"][std::to_string(state_id)];
        m_json.push(&json_state);
        reflect();
        m_json.pop();
    }

  private:
    json& top()
    {
        return *m_json.top();
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
    void visit_front_end(FrontEnd&& /*front_end*/)
    {
        static_assert(std::is_empty_v<std::decay_t<FrontEnd>>,
                      "FrontEnd must implement reflect(...)");
    }

    template <typename FrontEnd, typename Reflect>
    void visit_front_end(FrontEnd&& /*front_end*/, Reflect&& reflect)
    {
        auto& json_state = top().at("front_end");
        m_json.push(&json_state);
        reflect();
        m_json.pop();
    }

    template <typename Member>
    void visit_member(const char* key, Member&& member)
    {
        member = top().at(key);
    }

    template <typename State>
    void visit_state(size_t /*state_id*/, State&& /*state*/)
    {
        static_assert(std::is_empty_v<std::decay_t<State>>,
                      "State must implement reflect(...)");
    }

    template <typename State, typename Reflect>
    void visit_state(size_t state_id, State& /*state*/, Reflect&& reflect)
    {
        auto& json_state = top().at("states").at(std::to_string(state_id));
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

using StateMachine = DimSwitch;

// TODO:
// Check how to make these definitions generic.

void to_json(nlohmann::json& json, const StateMachine& state_machine)
{
    backmp11::reflect(state_machine, nlohmann_json_serializer{json});
}

void from_json(const nlohmann::json& json, StateMachine& state_machine)
{
    backmp11::reflect(state_machine, nlohmann_json_deserializer{json});
}

// Convert all state ids to a human-readable JSON array
// to understand which states the ids refer to.
template <typename StateMachine>
nlohmann::json state_names_to_json(const StateMachine& sm)
{
    nlohmann::json json;
    sm.template visit<backmp11::visit_mode::all_states>(
        [&sm, &json](auto& state)
        {
            using State = std::decay_t<decltype(state)>;
            json[sm.template get_state_id<State>()] =
                boost::core::demangled_name(typeid(State));
        });
    return json;
}

BOOST_AUTO_TEST_CASE(serialize_example)
{
    StateMachine dim_switch;

    auto state_names_json = state_names_to_json(dim_switch);
    BOOST_REQUIRE(state_names_json.dump(4) == \
R"([
    "Off",
    "On"
])");

    // The initial state is Off (state id 0).
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
    "states": {
        "1": {
            "times_pressed": 0
        }
    }
})");

    // Turn On (state id 1) and set brightness to 75.
    dim_switch.process_event(TurnOn{});
    BOOST_REQUIRE(dim_switch.is_state_active<On>());
    BOOST_REQUIRE(dim_switch.get_state<On>().times_pressed == 1);
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
    "states": {
        "1": {
            "times_pressed": 1
        }
    }
})");
    
    // Deserialize the json into a new state machine.
    auto dim_switch_2 = json.get<StateMachine>();

    // We have the same state as before.
    BOOST_REQUIRE(dim_switch_2.is_state_active<On>());
    BOOST_REQUIRE(dim_switch.get_state<On>().times_pressed == 1);
    BOOST_REQUIRE(dim_switch_2.brightness = 75);
}
