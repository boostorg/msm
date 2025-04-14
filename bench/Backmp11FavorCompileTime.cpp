#define TRANSITION_TABLE_TYPE boost::mp11::mp_list
#include <boost/mp11.hpp>
#include "common.hpp"
#include <boost/msm/backmp11/state_machine.hpp>
#include <boost/msm/backmp11/favor_compile_time.hpp>

using fsm0 = msm::back::state_machine<fsm_, msm::back::favor_compile_time>;

int main()
{
    test_fsm<fsm0>();

    return 0;
}

BOOST_MSM_BACKMP11_GENERATE_DISPATCH_TABLE(fsm0);