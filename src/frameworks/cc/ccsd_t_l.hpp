#ifndef _AQUARIUS_CC_CCSD_T_L_HPP_
#define _AQUARIUS_CC_CCSD_T_L_HPP_

#include "../../frameworks/cc/ccsd.hpp"
#include "../../frameworks/operator/2eoperator.hpp"
#include "../../frameworks/operator/excitationoperator.hpp"
#include "../../frameworks/task/task.hpp"
#include "../../frameworks/time/time.hpp"
#include "../../frameworks/util/global.hpp"

namespace aquarius
{
namespace cc
{

template <typename U>
class CCSD_T_L : public task::Task
{
    public:
        CCSD_T_L(const string& name, input::Config& config);

        bool run(task::TaskDAG& dag, const Arena& arena);
};

}
}

#endif
