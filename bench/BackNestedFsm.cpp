#define TRANSITION_TABLE_TYPE boost::mp11::mp_list
#include <boost/mp11.hpp>
#include <boost/mp11/mpl.hpp>
#include "nested_fsm.hpp"
#include <boost/msm/back/state_machine.hpp>

using fsm2 = msm::back::state_machine<fsm_<PROBLEM_SIZE/2>>;
using fsm1 = msm::back::state_machine<fsm_<PROBLEM_SIZE/3, fsm2>>;
using fsm0 = msm::back::state_machine<fsm_<0, fsm1>>;

int main()
{
    test_fsm<fsm0>();

    return 0;
}
