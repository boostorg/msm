#include "common.hpp"
#include <boost/msm/backmp11/state_machine.hpp>

using fsm = msm::back::state_machine<fsm_>;

int main()
{
    test_fsm<fsm>();

    return 0;
}