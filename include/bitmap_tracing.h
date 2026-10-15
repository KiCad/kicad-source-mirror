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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#pragma once

#include <geometry/shape_poly_set.h>
#include <wx/image.h>

class REPORTER;


/**
 * Reduce an image to pure black and white.
 *
 * A pixel becomes black when it is darker than the threshold and opaque enough.
 * Masked areas become white.
 *
 * @param aImage is the source image.
 * @param aThreshold is the brightness cut, from 0 to 1.
 * @param aInvert inverts the greyscale before the cut is applied.
 * @return a new image holding only black and white pixels.
 */
wxImage ConvertImageToBilevel( const wxImage& aImage, double aThreshold, bool aInvert );


/**
 * Produce the black and white view of an image, shrunk to fit a preview area.
 *
 * The aspect ratio is kept, and an image already smaller than the area is left alone.
 *
 * @param aImage is the source image.
 * @param aThreshold is the brightness cut, from 0 to 1.
 * @param aInvert inverts the greyscale before the cut is applied.
 * @param aRoom is the area the result has to fit inside.
 * @return the black and white image, scaled down if it did not fit.
 */
wxImage MakeBilevelPreview( const wxImage& aImage, double aThreshold, bool aInvert, const wxSize& aRoom );


/**
 * Trace the black regions of a black and white image into filled polygons.
 *
 * Holes are fractured into their outlines, so every returned outline is a simple polygon
 * that can be drawn filled.
 *
 * @param aBilevelImage is an image produced by ConvertImageToBilevel.
 * @param aScaleX converts pixels to caller units on X.
 * @param aScaleY converts pixels to caller units on Y. A negative value flips the Y axis.
 * @param aReporter receives errors.
 * @return the traced polygons, empty if nothing was traced.
 */
SHAPE_POLY_SET TraceBilevelImage( const wxImage& aBilevelImage, double aScaleX, double aScaleY, REPORTER& aReporter );
