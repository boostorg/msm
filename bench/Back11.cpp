#include "common.hpp"
#include <boost/msm/back11/state_machine.hpp>

using fsm = msm::back11::state_machine<fsm_>;

int main()
{
    test_fsm<fsm>();

    return 0;
}