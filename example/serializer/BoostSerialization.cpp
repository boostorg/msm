// Copyright 2026 Christian Granzin
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

#include <boost/test/unit_test.hpp>

// Deactivate the attributes container to showcase
// automatic detection of empty states with backmp11.
// Without this setting all empty states need to be
// instrumented with no-op reflect methods.
#define BOOST_MSM_FRONT_DISABLE_STATE_ATTRIBUTES_CONTAINER

// Include headers that implement an archive in simple text format.
#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>
// We need serialization support for std::array.
#include <boost/serialization/array.hpp>

#include "DimSwitch.hpp"

// Serializer for Boost.Serialization.
template <typename Archive>
class boost_serializer
{
  public:
    boost_serializer(Archive& archive) : m_archive(archive) {}

    template <typename FrontEnd>
    void visit_front_end(FrontEnd&& front_end)
    {
        if constexpr (!std::is_empty_v<std::decay_t<FrontEnd>>)
        {
            m_archive & front_end;
        }
    }

    template <typename FrontEnd, typename Reflect>
    void visit_front_end(FrontEnd&& /*front_end*/, Reflect&& reflect)
    {
        reflect();
    }

    template <typename Member>
    void visit_member(const char* /*key*/, Member&& member)
    {
        m_archive & member;
    }

    template <typename State>
    void visit_state(size_t /*state_id*/, State&& state)
    {
        if constexpr (!std::is_empty_v<std::decay_t<State>>)
        {
            m_archive & state;
        }
    }

    template <typename State, typename Reflect>
    void visit_state(size_t /*state_id*/, State&& /*state*/, Reflect&& reflect)
    {
        reflect();
    }

  private:
    Archive& m_archive;
};

using StateMachine = DimSwitch;

namespace boost::serialization {

// TODO:
// Check how to make this definition generic.
template <typename Archive>
void serialize(Archive& archive, StateMachine& state_machine,
               const unsigned int /*version*/)
{
    backmp11::reflect(state_machine, boost_serializer<Archive>{archive});
}

} // boost::serialization

BOOST_AUTO_TEST_CASE(serialize_example)
{
    StateMachine dim_switch;

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
    StateMachine dim_switch_2;
    iarchive >> dim_switch_2;

    // We have the same state as before.
    BOOST_REQUIRE(dim_switch_2.is_state_active<On>());
    BOOST_REQUIRE(dim_switch.get_state<On>().times_pressed == 1);
    BOOST_REQUIRE(dim_switch_2.brightness = 75);
}
