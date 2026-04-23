#include <boost/msm/backmp11/state_machine.hpp>
#include <boost/msm/front/state_machine_def.hpp>
#include <boost/msm/front/functor_row.hpp>

namespace backmp11 = boost::msm::backmp11;
namespace front = boost::msm::front;
using front::Row;
using front::Internal;
namespace mp11 = boost::mp11;

// States.
struct Off : front::state<> {};
struct On : front::state<> {};

// Events.
struct TurnOn {};
struct TurnOff {};
struct Dim
{
    uint8_t brightness;
};

// State machine.
struct DimSwitch_;
using DimSwitch = backmp11::state_machine<DimSwitch_>;

struct DimSwitch_ : front::state_machine_def<DimSwitch_>
{
    // Actions.
    struct SetDimValue
    {
        void operator()(const Dim& event, DimSwitch& self)
        {
            self.brightness = event.brightness;
        }
    };

    using initial_state = Off;

    using transition_table = mp11::mp_list<
        front::Row<Off, TurnOn, On>,
        front::Row<On, TurnOff, Off>
        >;

    using internal_transition_table = mp11::mp_list<
        front::Internal<Dim, SetDimValue>
        >;

#ifdef DIM_SWITCH_FRONT_END_HAS_REFLECTION
    template <typename Visitor>
    void reflect(Visitor&& visitor)
    {
        visitor.visit_member("brightness", brightness);
    }

    template <typename Visitor>
    void reflect(Visitor&& visitor) const
    {
        visitor.visit_member("brightness", brightness);
    }
#endif

    uint8_t brightness;
};
