/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2025 KiCad Developers, see AUTHORS.txt for contributors.
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

#ifndef GERBER_TO_POLYSET_H
#define GERBER_TO_POLYSET_H

#include <geometry/shape_poly_set.h>

class GERBER_FILE_IMAGE;


/**
 * Convert a GERBER_FILE_IMAGE to a merged SHAPE_POLY_SET.
 *
 * All draw items in the Gerber image are converted to polygons and merged
 * using boolean union. This handles step-and-repeat expansion.
 *
 * Note: This function may populate cached polygon data in the draw items.
 *
 * @param aImage The Gerber file image to convert
 * @param aTolerance Optional inflation tolerance in nm for comparison purposes
 * @return A merged SHAPE_POLY_SET containing all copper areas
 */
SHAPE_POLY_SET ConvertGerberToPolySet( GERBER_FILE_IMAGE* aImage, int aTolerance = 0 );


#endif // GERBER_TO_POLYSET_H
