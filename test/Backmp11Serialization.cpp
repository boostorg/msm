// Copyright 2026 Christian Granzin
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_MSM_NONSTANDALONE_TEST
#define BOOST_TEST_MODULE backmp11_serialization_test
#endif
#include <boost/test/unit_test.hpp>

// back-end
#include <boost/msm/backmp11/state_machine.hpp>
// front-end
#include <boost/msm/front/state_machine_def.hpp>
#include <boost/msm/front/functor_row.hpp>

// Boost.Serialization.
// Include headers for a simple text archive format.
#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>
#include <boost/msm/backmp11/serialization/boost_serialization.hpp>

// Boost.JSON.
#define BOOST_JSON_NO_LIB
#include <boost/json/src.hpp>
#include <boost/msm/backmp11/serialization/boost_json.hpp>

// nlohmann/json.
#include <boost/msm/backmp11/serialization/nlohmann_json.hpp>

using namespace boost::msm;
namespace mp11 = boost::mp11;

namespace
{

// States.

// An empty state (doesn't need reflection).
struct Off : front::state<> {};

// A state with a reflect free function.
struct On : front::state<>
{
    template <typename Event, typename Fsm>
    void on_entry(const Event&, Fsm&)
    {
        times_pressed += 1;
    }

// ADL with MSVC does not work correctly.
#ifdef BOOST_MSVC
    template <typename Visitor>
    void reflect(Visitor&& visitor)
    {
        visitor.visit_member("times_pressed", times_pressed);
    }
    template <typename Visitor>
    void reflect(Visitor&& visitor) const
    {
        visitor.visit_member("times_pressed", times_pressed);
    }
#endif 

    uint32_t times_pressed{};
};
template <typename Visitor>
void reflect(On& on, Visitor&& visitor)
{
    visitor.visit_member("times_pressed", on.times_pressed);
}
template <typename Visitor>
void reflect(const On& on, Visitor&& visitor)
{
    visitor.visit_member("times_pressed", on.times_pressed);
}

// Events.
struct TurnOn {};
struct TurnOff {};
struct Dim
{
    uint8_t brightness;
};

// State machine front-end with a reflect member function.
struct DimSwitch_ : front::state_machine_def<DimSwitch_>
{
    // Actions.
    struct SetDimValue
    {
        void operator()(const Dim& event, DimSwitch_& self)
        {
            self.brightness = event.brightness;
        }
    };

    using initial_state = Off;

    using transition_table = mp11::mp_list<
        front::Row<Off, TurnOn , On >,
        front::Row<On , TurnOff, Off>
        >;

    using internal_transition_table = mp11::mp_list<
        front::Internal<Dim, SetDimValue>
        >;

    template <typename Visitor>
    void reflect(Visitor&& visitor)
    {
        visitor.visit_member("brightness", this->brightness);
    }

    template <typename Visitor>
    void reflect(Visitor&& visitor) const
    {
        visitor.visit_member("brightness", brightness);
    }

    uint8_t brightness;
};

using DimSwitch = backmp11::state_machine<DimSwitch_>;

} // namespace

// Add a serialize free function to support Boost.Serialization for DimSwitch.
// We can integrate Boost.Serialization with this mechanism or by
// adding a serialize member function to the state machine.
namespace boost::serialization {

template <typename Archive>
void serialize(Archive& archive, DimSwitch& state_machine,
               const unsigned int /*version*/)
{
    backmp11::reflect(
        state_machine,
        backmp11::serialization::boost_serialization_serializer<Archive>{
            archive});
}

} // boost::serialization

namespace
{

BOOST_AUTO_TEST_CASE(boost_serialization)
{
    DimSwitch dim_switch;

    // The initial state is Off.
    dim_switch.start();
    BOOST_REQUIRE(dim_switch.is_state_active<Off>());

    // Turn On and set brightness to 75.
    dim_switch.process_event(TurnOn{});
    BOOST_REQUIRE(dim_switch.is_state_active<On>());
    BOOST_REQUIRE(dim_switch.get_state<On>().times_pressed == 1);
    dim_switch.process_event(Dim{75});
    BOOST_REQUIRE(dim_switch.brightness = 75);

    // Serialize the state machine.
    std::ostringstream ostream;
    boost::archive::text_oarchive oarchive{ostream};
    oarchive << dim_switch;

    // Deserialize the archive into a new state machine.
    std::istringstream istream{ostream.str()};
    boost::archive::text_iarchive iarchive{istream};
    DimSwitch dim_switch_2;
    iarchive >> dim_switch_2;

    // We have the same state as before.
    BOOST_REQUIRE(dim_switch_2.is_state_active<On>());
    BOOST_REQUIRE(dim_switch.get_state<On>().times_pressed == 1);
    BOOST_REQUIRE(dim_switch_2.brightness = 75);
}

// Helper for convenience:
// Convert all state ids to a human-readable JSON array
// to understand which states the ids refer to.
boost::json::array state_names_to_boost_json(const DimSwitch& sm)
{
    boost::json::array json;
    sm.template visit<backmp11::visit_mode::all_states>(
        [&json](auto& state)
        {
            using State = std::decay_t<decltype(state)>;
            const auto demangled = boost::core::demangled_name(typeid(State));
            const auto short_name = demangled.substr(demangled.rfind(':') + 1);
            json.push_back(boost::json::string{short_name});
        });
    return json;
}

BOOST_AUTO_TEST_CASE(boost_json)
{
    DimSwitch dim_switch;

    // Convert all state ids to a human-readable JSON array
    // to understand which states the ids refer to.
    auto state_names_json = state_names_to_boost_json(dim_switch);
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
        auto dim_switch_2 = boost::json::value_to<DimSwitch>(json);

        // We have the same state as before.
        BOOST_REQUIRE(dim_switch_2.is_state_active<On>());
        BOOST_REQUIRE(dim_switch.get_state<On>().times_pressed == 1);
        BOOST_REQUIRE(dim_switch_2.brightness = 75);
}

// Helper for convenience:
// Convert all state ids to a human-readable JSON array
// to understand which states the ids refer to.
nlohmann::json state_names_to_nlohmann_json(const DimSwitch& sm)
{
    nlohmann::json json;
    sm.template visit<backmp11::visit_mode::all_states>(
        [&sm, &json](auto& state)
        {
            using State = std::decay_t<decltype(state)>;
            const auto demangled = boost::core::demangled_name(typeid(State));
            const auto short_name = demangled.substr(demangled.rfind(':') + 1);
            json[sm.template get_state_id<State>()] = short_name;
        });
    return json;
}

BOOST_AUTO_TEST_CASE(nlohmann_json)
{
    DimSwitch dim_switch;

    auto state_names_json = state_names_to_nlohmann_json(dim_switch);
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
    auto dim_switch_2 = json.get<DimSwitch>();

    // We have the same state as before.
    BOOST_REQUIRE(dim_switch_2.is_state_active<On>());
    BOOST_REQUIRE(dim_switch.get_state<On>().times_pressed == 1);
    BOOST_REQUIRE(dim_switch_2.brightness = 75);
}

} // namespace
