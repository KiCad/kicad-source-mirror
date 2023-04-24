/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2019 KiCad Developers, see AUTHORS.TXT for contributors.
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

#ifndef QA_COMMON_WXIMAGE_TEST_UTILS__H
#define QA_COMMON_WXIMAGE_TEST_UTILS__H

#include <qa_utils/wx_utils/unit_test_utils.h>

#include <wx/image.h>

#include <gal/color4d.h>

namespace KI_TEST
{
/**
 * Predicate to check an image pixel matches color and alpha
 *
 * @param  aImage  the image to check
 * @param  aX      pixel x-coordinate
 * @param  aY      pixel y-coordinate
 * @param  aColor  expected color (alpha is 1.0 if image doesn't support alpha)
 * @return         true if colour match
 */
bool IsImagePixelOfColor( const wxImage& aImage, int aX, int aY, const KIGFX::COLOR4D& aColor );

} // namespace KI_TEST


namespace BOOST_TEST_PRINT_NAMESPACE_OPEN
{
template <>
struct print_log_value<wxImage>
{
    void operator()( std::ostream& os, wxImage const& aImage );
};
} // namespace BOOST_TEST_PRINT_NAMESPACE_OPEN
BOOST_TEST_PRINT_NAMESPACE_CLOSE

#endif
