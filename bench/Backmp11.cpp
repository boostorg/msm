#define TRANSITION_TABLE_TYPE boost::mp11::mp_list
#include <boost/mp11.hpp>
#include "common.hpp"
#include <boost/msm/backmp11/state_machine.hpp>

using fsm0 = msm::backmp11::state_machine<fsm_>;

int main()
{
    test_fsm<fsm0>();

    return 0;
}