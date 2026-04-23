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

// Include headers for Boost.JSON.
#define BOOST_JSON_NO_LIB
#include <boost/json/src.hpp>
// #include <boost/json.hpp>

#include "DimSwitch.hpp"

// Serializer for nlohmann json.
class boost_json_serializer
{
    using json = boost::json::object;

  public:
    boost_json_serializer(boost::json::value& json) : m_json({&json.emplace_object()}) {}

    template <typename FrontEnd>
    void visit_front_end(FrontEnd&& /*front_end*/)
    {
        static_assert(std::is_empty_v<std::decay_t<FrontEnd>>,
                      "FrontEnd must implement reflect(...)");
    }

    template <typename FrontEnd, typename Reflect>
    void visit_front_end(FrontEnd&& /*front_end*/, Reflect&& reflect)
    {
        auto& json_front_end = top()["front_end"].emplace_object();
        m_json.push(&json_front_end);
        reflect();
        m_json.pop();
    }

    template <typename Member>
    void visit_member(const char* key, Member&& member)
    {
        top()[key] = member;
    }

    template <typename T, std::size_t N>
    void visit_member(const char* key, const std::array<T, N>& member)
    {
        boost::json::array& arr = top()[key].emplace_array();
        for (const auto& elem : member)
        {
            arr.push_back(elem);
        }
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
        boost::json::object* states_json;
        if (!top().contains("states"))
        {
            top()["states"].emplace_object();
        }
        auto& json_state =
            top()["states"].as_object()[std::to_string(state_id)].emplace_object();
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
class boost_json_deserializer
{
    using json = boost::json::object;

  public:
    boost_json_deserializer(const boost::json::value& json) : m_json({&json.as_object()}) {}

    template <typename FrontEnd>
    void visit_front_end(FrontEnd&& /*front_end*/)
    {
        static_assert(std::is_empty_v<std::decay_t<FrontEnd>>,
                      "FrontEnd must implement reflect(...)");
    }

    template <typename FrontEnd, typename Reflect>
    void visit_front_end(FrontEnd&& /*front_end*/, Reflect&& reflect)
    {
        auto& json_state = top().at("front_end").as_object();
        m_json.push(&json_state);
        reflect();
        m_json.pop();
    }

    template <typename Member>
    void visit_member(const char* key, Member&& member)
    {
        member = boost::json::value_to<std::decay_t<Member>>(top().at(key));
    }

    template <typename T, std::size_t N>
    void visit_member(const char* key, std::array<T, N>& member)
    {
        const auto& arr = top().at(key).as_array();
        for (std::size_t i = 0; i < N; ++i)
            member[i] = boost::json::value_to<T>(arr[i]);
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
        auto& json_state =
            top().at("states").at(std::to_string(state_id)).as_object();
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

void tag_invoke(const boost::json::value_from_tag&, boost::json::value& json, const StateMachine& state_machine)
{
    backmp11::reflect(state_machine, boost_json_serializer{json});
}

StateMachine tag_invoke(const boost::json::value_to_tag<StateMachine>&, const boost::json::value& json)
{
    StateMachine state_machine;
    backmp11::reflect(state_machine, boost_json_deserializer{json});
    return state_machine;
}

// Convert all state ids to a human-readable JSON array
// to understand which states the ids refer to.
template <typename StateMachine>
boost::json::array state_names_to_json(const StateMachine& sm)
{
    boost::json::array json;
    sm.template visit<backmp11::visit_mode::all_states>(
        [&sm, &json](auto& state)
        {
            using State = std::decay_t<decltype(state)>;
            json.push_back(boost::json::string{
                boost::core::demangled_name(typeid(State))});
        });
    return json;
}

BOOST_AUTO_TEST_CASE(serialize_example)
{
    StateMachine dim_switch;

    // Convert all state ids to a human-readable JSON array
    // to understand which states the ids refer to.
    auto state_names_json = state_names_to_json(dim_switch);
    BOOST_REQUIRE(boost::json::serialize(state_names_json) ==
                  R"(["Off","On"])");

        // The initial state is Off (state id 0).
        dim_switch.start();
        BOOST_REQUIRE(dim_switch.is_state_active<Off>());

        boost::json::value json = boost::json::value_from(dim_switch);
        BOOST_REQUIRE(boost::json::serialize(json) == \
R"({"front_end":{"brightness":0},"active_state_ids":[0],"event_processing":false,"states":{"1":{"times_pressed":0}},"running":true})");

        // Turn On (state id 1) and set brightness to 75.
        dim_switch.process_event(TurnOn{});
        BOOST_REQUIRE(dim_switch.is_state_active<On>());
        BOOST_REQUIRE(dim_switch.get_state<On>().times_pressed == 1);
        dim_switch.process_event(Dim{75});
        BOOST_REQUIRE(dim_switch.brightness = 75);

        json = boost::json::value_from(dim_switch);
        BOOST_REQUIRE(boost::json::serialize(json) == \
R"({"front_end":{"brightness":75},"active_state_ids":[1],"event_processing":false,"states":{"1":{"times_pressed":1}},"running":true})");

        // Deserialize the json into a new state machine.
        auto dim_switch_2 = boost::json::value_to<StateMachine>(json);

        // We have the same state as before.
        BOOST_REQUIRE(dim_switch_2.is_state_active<On>());
        BOOST_REQUIRE(dim_switch.get_state<On>().times_pressed == 1);
        BOOST_REQUIRE(dim_switch_2.brightness = 75);
}
