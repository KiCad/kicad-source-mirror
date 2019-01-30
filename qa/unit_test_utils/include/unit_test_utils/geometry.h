
#ifndef QA_UNIT_TEST_UTILS_GEOM__H
#define QA_UNIT_TEST_UTILS_GEOM__H

#include <unit_test_utils/numeric.h>
#include <unit_test_utils/unit_test_utils.h>

#include <math/box2.h>
#include <math/vector2d.h>


/**
 * Define a stream function for logging this type.
 *
 * TODO: convert to boost_test_print_type when Boost minver > 1.64
 */
inline std::ostream& operator<<( std::ostream& os, const BOX2I& aBox )
{
    os << "BOX[ " << aBox.GetOrigin() << " + " << aBox.GetSize() << " ]";
    return os;
}

namespace KI_TEST
{

/**
 * Check that both x and y of a vector are within expected error
 */
template <typename VEC>
bool IsVecWithinTol( const VEC& aVec, const VEC& aExp, typename VEC::coord_type aTol )
{
    return IsWithin<typename VEC::coord_type>( aVec.x, aExp.x, aTol )
           && IsWithin<typename VEC::coord_type>( aVec.y, aExp.y, aTol );
}

/**
 * Check that a box is close enough to another box
 */
template <typename BOX>
bool IsBoxWithinTol( const BOX& aBox, const BOX& aExp, typename BOX::coord_type aTol )
{
    using VEC = VECTOR2<typename BOX::coord_type>;
    return IsVecWithinTol<VEC>( aBox.GetPosition(), aExp.GetPosition(), aTol )
           && IsVecWithinTol<VEC>( aBox.GetSize(), aExp.GetSize(), aTol * 2 );
}

} // namespace KI_TEST

#endif // QA_UNIT_TEST_UTILS_GEOM__H