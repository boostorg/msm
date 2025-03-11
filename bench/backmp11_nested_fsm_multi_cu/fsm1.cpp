#define TRANSITION_TABLE_TYPE boost::mp11::mp_list
#include <boost/mp11.hpp>
#include <boost/mp11/mpl.hpp>
#include "../nested_fsm.hpp"
#include <boost/msm/backmp11/state_machine.hpp>
#include <boost/msm/backmp11/favor_compile_time.hpp>

using fsm2 = msm::back::state_machine<fsm_<PROBLEM_SIZE/2>, msm::back::favor_compile_time>;
using fsm1 = msm::back::state_machine<fsm_<PROBLEM_SIZE/3, fsm2>, msm::back::favor_compile_time>;

BOOST_MSM_BACKMP11_GENERATE_FSM(fsm1);
