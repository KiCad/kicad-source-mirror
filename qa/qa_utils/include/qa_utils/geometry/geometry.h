/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#ifndef QA_UNIT_TEST_UTILS_GEOM__H
#define QA_UNIT_TEST_UTILS_GEOM__H

#include <qa_utils/numeric.h>
#include <qa_utils/wx_utils/unit_test_utils.h>

#include <math/box2.h>
#include <math/vector2d.h>


/**
 * Define a stream function for logging this type.
 */
template <typename T>
std::ostream& boost_test_print_type( std::ostream& os, const BOX2<T>& aBox )
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
