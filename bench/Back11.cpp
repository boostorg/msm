#define TRANSITION_TABLE_TYPE boost::fusion::vector
#include "common.hpp"
#include <boost/msm/back11/state_machine.hpp>

using fsm0 = msm::back11::state_machine<fsm_>;

int main()
{
    test_fsm<fsm0>();

    return 0;
}