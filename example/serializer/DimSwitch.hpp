#include <boost/msm/backmp11/state_machine.hpp>
#include <boost/msm/front/state_machine_def.hpp>
#include <boost/msm/front/functor_row.hpp>

namespace backmp11 = boost::msm::backmp11;
namespace front = boost::msm::front;
using front::Row;
using front::Internal;
namespace mp11 = boost::mp11;

// States.

// A state without reflection.
struct Off : front::state<> {};

// A state with a reflect free function.
struct On : front::state<>
{
    template <typename Event, typename Fsm>
    void on_entry(const Event&, Fsm&)
    {
        times_pressed += 1;
    }

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
