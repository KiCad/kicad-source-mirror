/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.TXT for contributors.
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

/**
 * @file
 * Test utilities for COLOUR4D objects
 */

#ifndef QA_COMMON_COLOR4D_TEST_UTILS__H
#define QA_COMMON_COLOR4D_TEST_UTILS__H

#include <gal/color4d.h>

#include <qa_utils/numeric.h>

namespace KI_TEST
{
/**
 * Checks if a COLOR4D is close enough to another
 */
inline bool IsColorNear( const KIGFX::COLOR4D& aCol, const KIGFX::COLOR4D aOther, double aTol )
{
    return KI_TEST::IsWithin<double>( aCol.r, aOther.r, aTol )
           && KI_TEST::IsWithin<double>( aCol.g, aOther.g, aTol )
           && KI_TEST::IsWithin<double>( aCol.b, aOther.b, aTol )
           && KI_TEST::IsWithin<double>( aCol.a, aOther.a, aTol );
}

/**
 * Checks if a COLOR4D is close enough to a given RGB char value
 */
inline bool IsColorNearHex( const KIGFX::COLOR4D& aCol, unsigned char r, unsigned char g,
        unsigned char b, unsigned char a )
{
    const double tol = 0.5 / 255.0; // One bit of quantised error

    return KI_TEST::IsWithin<double>( aCol.r, r / 255.0, tol )
           && KI_TEST::IsWithin<double>( aCol.g, g / 255.0, tol )
           && KI_TEST::IsWithin<double>( aCol.b, b / 255.0, tol )
           && KI_TEST::IsWithin<double>( aCol.a, a / 255.0, tol );
}
} // namespace KI_TEST

#endif // QA_COMMON_COLOR4D_TEST_UTILS__H
