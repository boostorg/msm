#define TRANSITION_TABLE_TYPE boost::mp11::mp_list
#include <boost/mp11.hpp>
#include <boost/mp11/mpl.hpp>
#include "nested_fsm.hpp"
#include <boost/msm/backmp11/state_machine.hpp>
#include <boost/msm/backmp11/favor_compile_time.hpp>

using fsm2 = msm::back::state_machine<fsm_<PROBLEM_SIZE/2>, msm::back::favor_compile_time>;
using fsm1 = msm::back::state_machine<fsm_<PROBLEM_SIZE/3, fsm2>, msm::back::favor_compile_time>;
using fsm0 = msm::back::state_machine<fsm_<0, fsm1>, msm::back::favor_compile_time>;

int main()
{
    test_fsm<fsm0>();

    return 0;
}

BOOST_MSM_BACK_GENERATE_FSM(fsm2);
BOOST_MSM_BACK_GENERATE_FSM(fsm1);
BOOST_MSM_BACK_GENERATE_FSM(fsm0);
