#include "shell.hpp"

using namespace aquarius::symmetry;

namespace aquarius
{
namespace molecule
{

Shell::Shell(const Center& pos, int L, int nprim, int ncontr, bool spherical, bool keep_contaminants,
             const vector<double>& exponents, const matrix<double>& coeffs)
: center(pos), L(L), spherical(spherical), keep_contaminants(keep_contaminants),
  exponents(exponents), coefficients(coeffs)
{
    const PointGroup& group = pos.getPointGroup();
    int nirrep = group.getNumIrreps();
    int order = group.getOrder();
    int nfunc = getNFunc();
    int ndegen = pos.getCenters().size();

    vector<int> proj(order);

    irreps.resize(nfunc, vector<int>(nirrep));
    func_irrep.resize(nfunc, vector<int>(nirrep));
    irrep_pos.resize(nfunc, vector<int>(nirrep));

    nfunc_per_irrep.resize(nirrep, 0);

    parity.resize(nfunc, vector<int>(order));

    for (int op = 0;op < order;op++)
    {
        if (spherical)
        {
            int f = 0;
            for (int l = L;l >= (keep_contaminants ? 0 : L);l -= 2)
            {
                for (int m = l;m > 0;m--)
                {
                    parity[f++][op] = (group.sphericalParity(l,  m, op) < 0 ? -1 : 1);
                    parity[f++][op] = (group.sphericalParity(l, -m, op) < 0 ? -1 : 1);
                }
                parity[f++][op] = (group.sphericalParity(l, 0, op) < 0 ? -1 : 1);
            }
        }
        else
        {
            int f = 0;
            for (int x = L;x >= 0;x--)
            {
                for (int y = L-x;y >= 0;y--)
                {
                    int z = L-x-y;
                    parity[f++][op] = (group.cartesianParity(x, y, z, op) < 0 ? -1 : 1);
                }
            }
        }
    }

    /*
     * the tricky part: determine the irrep of each final SO function
     * each AO function will give n SO functions, where n is the number of symmetry-equivalent atoms associated to this shell
     *
     * loop through the functions and for each irrep, determine the projection of this function onto the degenerate centers
     * if this is non-zero, then this is one of the irreps for this function
     *
     * this could potentially be simplified by using the DCR of the atom's stabilizer, but this works
     */
    for (int func = 0;func < nfunc;func++)
    {
        int i = 0;

        for (int irrep = 0;irrep < order;irrep++)
        {
            for (int j = 0;j < ndegen;j++)
            {
                proj[j] = 0;
            }

            /*
             * do the projection, using the characters of the irrep and the parity of either the cartesian or spherical functions
             */
            for (int op = 0;op < order;op++)
            {
                proj[pos.getCenterAfterOp(op)] += (group.character(irrep, op)*parity[func][op] < 0 ? -1 : 1);
            }

            for (int j = 0;j < ndegen;j++)
            {
                proj[j] /= (order/ndegen);
            }

            /*
             * check if the projection is non-zero
             */
            int nonzero = 0;
            for (int j = 0;j < ndegen;j++)
            {
                nonzero += abs(proj[j]);
            }
            if (nonzero > 0)
            {
                irrep_pos[func][irrep] = i;
                func_irrep[func][i] = nfunc_per_irrep[irrep];
                irreps[func][i] = irrep;
                nfunc_per_irrep[irrep]++;
                i++;
            }
        }
    }

    /*
     * Normalize the shell
     */
    const double PI2_N34 = 0.25197943553838073034791409490358;

    for (int i = 0;i < ncontr;i++)
    {
        double norm = 0.0;
        for (int j = 0;j < nprim;j++)
        {
            for (int k = 0;k < nprim;k++)
            {
                double zeta = sqrt(exponents[j]*exponents[k])/(exponents[j]+exponents[k]);
                norm += coefficients[j][i]*coefficients[k][i]*pow(2*zeta,(double)L+1.5);
            }
        }

        for (int j = 0;j < nprim;j++)
        {
            coefficients[j][i] *= PI2_N34*pow(4*exponents[j],((double)L+1.5)/2)/sqrt(norm);
        }
    }

    /*
     * Generate the cartesian -> spherical harmonic transformation
     */
    if (spherical)
    {
        cart2spher.resize((L+1)*(L+2)/2, nfunc);
        int sf = 0;
        for (int l = L;l >= (keep_contaminants ? 0 : L);l -= 2)
        {
            for (int m = l;m > 0;m--)
            {
                for (int x = L, cf = 0;x >= 0;x--)
                    for (int y = L-x;y >= 0;y--, cf++)
                        cart2spher[cf][sf] = cartcoef(l, m, x, y, L-x-y);
                sf++;
                for (int x = L, cf = 0;x >= 0;x--)
                    for (int y = L-x;y >= 0;y--, cf++)
                        cart2spher[cf][sf] = cartcoef(l, -m, x, y, L-x-y);
                sf++;
            }
            for (int x = L, cf = 0;x >= 0;x--)
                for (int y = L-x;y >= 0;y--, cf++)
                    cart2spher[cf][sf] = cartcoef(l, 0, x, y, L-x-y);
            sf++;
        }
        assert(sf == nfunc);
    }
    else
    {
        cart2spher.resize((L+1)*(L+2)/2, (L+1)*(L+2)/2);
        for (int i = 0;i < (L+1)*(L+2)/2;i++)
            cart2spher[i][i] = 1.0;
    }
}

double Shell::cartcoef(int l, int m, int lx, int ly, int lz)
{
    int am = abs(m);
    int j = lx+ly-am;
    if ((j&1) == 1) return 0.0;
    j /= 2;

    double c = sqrt((double)(binom(2*lx,lx)*binom(2*ly,ly)*binom(2*lz,lz)*binom(l+am,am))/
                    (double)(binom(2*l,l)*binom(l,am)*binom(lx+ly+lz,lx)*binom(ly+lz,ly))/
                    (double)(dfact(2*lx-1)*dfact(2*ly-1)*dfact(2*lz-1)))/pow(2.0,l);
    if (m != 0) c *= sqrt(2.0);

    if (m >= 0)
    {
        if (((am-lx)&1) == 1) return 0.0;
        if (((am-lx)&3) == 2) c = -c;
    }
    else
    {
        if (((am-lx)&1) == 0) return 0.0;
        if (((am-lx)&3) == 3) c = -c;
    }

    double sum = 0.0;
    for (int i = 0;i <= (l-am)/2;i++)
    {
        for (int k = 0;k <= j;k++)
        {
            double tmp = binom(2*l-2*i,l+am)*binom(l,i)*binom(i,j)*binom(j,k)*binom(am,lx-2*k);
            if (((i+k)&1) == 1) tmp = -tmp;
            sum += tmp;
        }
    }

    return sum*c;
}

}
}
