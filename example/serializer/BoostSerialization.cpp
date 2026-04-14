// Copyright 2026 Christian Granzin
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

#include <boost/test/unit_test.hpp>

// Deactivate the attributes container to showcase
// automatic serialization of trivially copyable states with backmp11.
#define BOOST_MSM_FRONT_ATTRIBUTES_CONTAINER void

// Include headers that implement an archive in simple text format.
#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>
// We need serialization support for std::array.
#include <boost/serialization/array.hpp>

#include "DimSwitch.hpp"

// Serializer for Boost.Serialization.
//
// Can serialize trivially copyable states without instrumentation.
template <typename Archive>
class boost_serializer
{
  public:
    boost_serializer(Archive& archive) : m_archive(archive) {}

    // Serialize any member.
    template <typename Key, typename Member>
    void operator()(Key&&, Member& member)
    {
        if constexpr(has_serialize_member<Member>::value)
        {
            m_archive & member;
        }
        else if constexpr (std::is_trivially_copyable_v<Member>)
        {
            auto& bytes =
                reinterpret_cast<std::array<std::byte, sizeof(Member)>&>(member);
            m_archive & bytes;
        }
        else
        {
            static_assert(std::is_empty_v<Member>, "Member must have a serialize member");
        }
    }

    // Serialize a submachine.
    template <typename State, typename F>
    void operator()(int /*state_id*/, State& /*state*/, F&& f)
    {
        f();
    }

  private:
    template <typename State, typename = void>
    struct has_serialize_member : std::false_type {};
    template <typename State>
    struct has_serialize_member<
        State,
        std::void_t<decltype(std::declval<State&>().serialize(
            std::declval<Archive&>(), 0u))>>
        : std::true_type {};

    Archive& m_archive;
};

namespace boost::serialization {

template <typename Archive>
void serialize(Archive& archive, DimSwitch& state_machine,
               const unsigned int /*version*/)
{
    back::reflect(state_machine, boost_serializer<Archive>{archive});
}

} // boost::serialization

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
    BOOST_REQUIRE(dim_switch_2.brightness = 75);
}
