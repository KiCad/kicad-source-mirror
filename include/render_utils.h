/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#pragma once

#include <tl/expected.hpp>

#include <wx/colour.h>
#include <wx/image.h>


/**
 * Combine two opaque renders of the same content into a single image with an alpha
 * channel.
 *
 * Given two renders of the same content, one composited over a pure white background
 * and one over a pure black background, the per-pixel alpha can be recovered
 * from the difference between the renders, and the straight (un-premultiplied) colour
 * from the black-background render:
 *
 *     B = ( c * alpha ) / 255
 *     W = B + ( 255 - alpha )
 *
 * where @c c is the straight colour and @c alpha the per-pixel alpha, both in the
 * interval 0..255. Solving gives:
 *
 *     alpha = 255 - ( W - B )
 *     c     = ( B * 255 ) / alpha
 *
 * The returned image reproduces the same appearance over any background, which is
 * useful where transparent output is needed from an opaque renderer (for example
 * clipboard images).
 *
 * @param aOnWhite is a render of the content over an opaque pure-white background.
 * @param aOnBlack is a render of the same content over an opaque pure-black background.
 * @return the combined image with an alpha channel, or an unexpected value containing
 *         an error message when the inputs are not valid images of the same size.
 */
tl::expected<wxImage, std::string> CreateAlphaImageFromTwoRenders( const wxImage& aOnWhite, const wxImage& aOnBlack );


/**
 * Apply the "Color to Alpha" algorithm in place, converting \a aColour to transparent.
 *
 * Pixels matching \a aColour exactly become fully transparent; pixels further from it
 * keep proportionally more opacity. An alpha channel is added if the image does not
 * have one.
 *
 * This is a port of the GEGL color-to-alpha operation used by GIMP; see the
 * implementation file for the full attribution.
 *
 * @param aImage the image to modify in place.
 * @param aColour the colour to make transparent.
 */
void ConvertColourToAlphaInPlace( wxImage& aImage, const wxColour& aColour );
