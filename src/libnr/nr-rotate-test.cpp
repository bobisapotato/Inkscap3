#include <cmath>
#include <utest/utest.h>
#include <libnr/nr-matrix.h>
#include <libnr/nr-matrix-ops.h>
#include <libnr/nr-matrix-fns.h>        /* identity, matrix_equalp */
#include <libnr/nr-rotate.h>
#include <libnr/nr-rotate-ops.h>
using NR::X;
using NR::Y;

inline bool point_equalp(NR::Point const &a, NR::Point const &b)
{
    return ( NR_DF_TEST_CLOSE(a[X], b[X], 1e-5) &&
             NR_DF_TEST_CLOSE(a[Y], b[Y], 1e-5) );
}

static bool rotate_equalp(NR::rotate const &a, NR::rotate const &b, double const eps)
{
    return matrix_equalp(NR::Matrix(a), NR::Matrix(b), eps);
}

int main(int argc, char *argv[])
{
    utest_start("rotate");

    NR::Matrix const m_id(NR::identity());
    NR::rotate const r_id(0.0);
    NR::rotate const rot234(.234);
    UTEST_TEST("constructors, comparisons") {
        UTEST_ASSERT( r_id == r_id );
        UTEST_ASSERT( rot234 == rot234 );
        UTEST_ASSERT( rot234 != r_id );
        UTEST_ASSERT( r_id == NR::rotate(NR::Point(1.0, 0.0)) );
        UTEST_ASSERT( NR::Matrix(r_id) == m_id );
        UTEST_ASSERT( NR::Matrix(r_id).test_identity() );

        UTEST_ASSERT(rotate_equalp(rot234, NR::rotate(NR::Point(cos(.234), sin(.234))), 1e-12));
    }

    UTEST_TEST("operator=") {
        NR::rotate rot234_eq(r_id);
        rot234_eq = rot234;
        UTEST_ASSERT( rot234 == rot234_eq );
        UTEST_ASSERT( rot234_eq != r_id );
    }

    UTEST_TEST("inverse") {
        UTEST_ASSERT( r_id.inverse() == r_id );
        UTEST_ASSERT( rot234.inverse() == NR::rotate(-.234) );
    }

    NR::Point const b(-2.0, 3.0);
    NR::rotate const rot180(NR::Point(-1.0, 0.0));
    UTEST_TEST("operator*(Point, rotate)") {
        UTEST_ASSERT( b * r_id == b );
        UTEST_ASSERT( b * rot180 == -b );
        UTEST_ASSERT( b * rot234 == b * NR::Matrix(rot234) );
    }

    UTEST_TEST("operator*(rotate, rotate)") {
        UTEST_ASSERT( r_id * r_id == r_id );
        UTEST_ASSERT( rot180 * rot180 == r_id );
        UTEST_ASSERT( rot234 * r_id == rot234 );
        UTEST_ASSERT( r_id * rot234 == rot234 );
        UTEST_ASSERT(rotate_equalp(rot234 * rot234.inverse(), r_id, 1e-14));
        UTEST_ASSERT(rotate_equalp(rot234.inverse() * rot234, r_id, 1e-14));
        UTEST_ASSERT(point_equalp(( NR::rotate(0.25) * NR::rotate(.5) ).vec, NR::rotate(.75).vec));
    }

    UTEST_TEST("operator/(rotate, rotate)") {
        UTEST_ASSERT( rot234 / r_id == rot234 );
        UTEST_ASSERT( rot234 / rot180 == rot234 * rot180 );
        UTEST_ASSERT(rotate_equalp(rot234 / rot234, r_id, 1e-14));
        UTEST_ASSERT(rotate_equalp(r_id / rot234, rot234.inverse(), 1e-14));
    }

    return ( utest_end()
             ? EXIT_SUCCESS
             : EXIT_FAILURE );
}

/*
  Local Variables:
  mode:c++
  c-file-style:"stroustrup"
  c-file-offsets:((innamespace . 0)(inline-open . 0))
  indent-tabs-mode:nil
  fill-column:99
  End:
*/
// vim: filetype=c++:expandtab:shiftwidth=4:tabstop=8:softtabstop=4 :
