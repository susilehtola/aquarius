#ifndef _AQUARIUS_SCF_UHF_LOCAL_HPP_
#define _AQUARIUS_SCF_UHF_LOCAL_HPP_

#include "../../frameworks/scf/uhf.hpp"
#include "../../frameworks/util/global.hpp"

namespace aquarius
{
namespace scf
{

template <typename T>
class LocalUHF : public UHF<T>
{
    public:
        LocalUHF(const string& name, input::Config& config);

    protected:
        using UHF<T>::E_alpha;
        using UHF<T>::E_beta;

        void calcSMinusHalf();

        void diagonalizeFock();
};

}
}

#endif
