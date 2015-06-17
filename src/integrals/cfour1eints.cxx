#include "cfour1eints.hpp"

using namespace aquarius::input;
using namespace aquarius::symmetry;
using namespace aquarius::task;
using namespace aquarius::tensor;

namespace aquarius
{
namespace integrals
{

CFOUROneElectronIntegralsTask::CFOUROneElectronIntegralsTask(const string& name, Config& config)
: Task(name, config)
{
    vector<Requirement> reqs;
    reqs.push_back(Requirement("molecule", "molecule"));
    addProduct(Product("ovi", "S", reqs));
    addProduct(Product("kei", "T", reqs));
    addProduct(Product("nai", "G", reqs));
    addProduct(Product("1ehamiltonian", "H", reqs));
}

bool CFOUROneElectronIntegralsTask::run(TaskDAG& dag, const Arena& arena)
{
    const Molecule& molecule = get<Molecule>("molecule");

    const vector<int>& N = molecule.getNumOrbitals();
    int n = molecule.getGroup().getNumIrreps();

    ifstream ifs("IIII");

    int intsize = 8;
    int32_t junk;

    ifs.read((char*)&junk, 4);
    assert(junk == 224);

    ifs.read((char*)&junk, 4);
    if (junk != 0) intsize = 4;

    int batchsize = 600;
    vector<double> ints(batchsize);
    vector<int64_t> idxs(batchsize);
    KeyValueVector pairs(Field::DOUBLE);

    auto init = TensorInitializer<>("S", tensor::Field::DOUBLE) <<
                TensorInitializer<PGSYMMETRIC|BOUNDED>(molecule.getGroup(), {N,N});
    Tensor<BOUNDED|PGSYMMETRIC> S = put<Tensor<>>("S", Tensor<PGSYMMETRIC|BOUNDED>::construct(init));
    Tensor<BOUNDED|PGSYMMETRIC> T = put<Tensor<>>("T", S.construct("T"));
    Tensor<BOUNDED|PGSYMMETRIC> G = put<Tensor<>>("G", S.construct("G"));
    Tensor<BOUNDED|PGSYMMETRIC> H = put<Tensor<>>("H", S.construct("H"));

    ifs.seekg(0);
    while (ifs)
    {
        int64_t recsize = 0;
        ifs.read((char*)&recsize, intsize);
        if (recsize != 8)
        {
            ifs.seekg(recsize+intsize, ifstream::cur);
            continue;
        }

        char label[9] = {};
        ifs.read(label, 8);
        ifs.seekg(intsize, ifstream::cur);
        auto& tensor = strcmp(label, "OVERLAP ") == 0 ? S :
                       strcmp(label, "ONEHAMIL") == 0 ? H :
                       strcmp(label, "KINETINT") == 0 ? T : G;
        if (&tensor == &G) continue;

        while (true)
        {
            int64_t numints;
            ifs.read((char*)&recsize, intsize);
            assert(recsize == 600*16+8);
            ifs.read((char*)ints.data(), 600*8);
            ifs.read((char*)idxs.data(), 600*8);
            ifs.read((char*)&numints, 8);
            ifs.seekg(intsize, ifstream::cur);

            if (numints < 0) break;

            pairs.clear();
            pairs.reserve(2*numints);
            for (int i = 0;i < numints;i++)
            {
                idxs[i]--;
                int p, q, pq = 0;
                for (p = 0;p < N[0];p++)
                {
                    if (pq+p+1 > idxs[i]) break;
                    pq += p+1;
                }
                q = idxs[i]-pq;
                pairs.push_back(p+q*N[0], ints[i]);
                if (p != q)
                {
                    pairs.push_back(q+p*N[0], ints[i]);
                }
            }

            tensor.setDataByIrrep({0,0}, pairs);
        }
    }

    G["PQ"]  = H["PQ"];
    G["PQ"] -= T["PQ"];

    return true;
}

}
}

REGISTER_TASK(aquarius::integrals::CFOUROneElectronIntegralsTask,"cfour1eints");
