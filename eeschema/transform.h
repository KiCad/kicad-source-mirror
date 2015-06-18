/**
 * @file transform.h
 */

/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2007-2010 Wayne Stambaugh <stambaughw@verizon.net>
 * Copyright (C) 2007 KiCad Developers, see change_log.txt for contributors.
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


#ifndef _TRANSFORM_H_
#define _TRANSFORM_H_

#include <wx/gdicmn.h>

class EDA_RECT;

/**
 * Class for tranforming drawing coordinates for a wxDC device context.
 *
 * This probably should be a base class with all pure methods and a derived class
 * named WXDC_TRANFORM be created.  Then in the future if some new device context
 * is used, a new transform could be derived from the base class and all the drawable
 * objects would have to do is provide overloaded draw methods to use the new transorm.
 */
class TRANSFORM
{
public:
    int x1;
    int y1;
    int x2;
    int y2;

    /**
     * The default construct creates a tranform that draws object is the normal orientation.
     */
    TRANSFORM() : x1( 1 ), y1( 0 ), x2( 0 ), y2( -1 ) {}

    TRANSFORM( int x1, int y1, int x2, int y2 ) : x1( x1 ), y1( y1 ), x2( x2 ), y2( y2 ) {}

    TRANSFORM& operator=( const TRANSFORM& aTransform );

    bool operator==( const TRANSFORM& aTransform ) const;

    bool operator!=( const TRANSFORM& aTransform ) const { return !( *this == aTransform ); }

   /**
    * Calculate a new coordinate according to the mirror/rotation transform.
    * Useful to calculate actual coordinates of a point
    * from coordinates relative to a component
    * which are given for a non rotated, non mirrored item
    * @param aPoint = The position to transform
    * @return The transformed coordinate.
    */
    wxPoint TransformCoordinate( const wxPoint& aPoint ) const;

   /**
    * Calculate a new rect according to the mirror/rotation transform.
    * Useful to calculate actual coordinates of a point
    * from coordinates relative to a component
    * which are given for a non rotated, non mirrored item
    * @param aRect = The rectangle to transform
    * @return The transformed rectangle.
    */
    EDA_RECT TransformCoordinate( const EDA_RECT& aRect ) const;

    /**
    * Calculate the Inverse mirror/rotation transform.
    * Useful to calculate coordinates relative to a component
    * which must be for a non rotated, non mirrored item
    * from the actual coordinate.
    * @return The inverse transform.
    */
    TRANSFORM InverseTransform( ) const;

   /**
    * Calculate new angles according to the transform.
    *
    * @param aAngle1 = The first angle to transform
    * @param aAngle2 = The second angle to transform
    * @return True if the angles were swapped during the transform.
    */
    bool MapAngles( int* aAngle1, int* aAngle2 ) const;
};


#endif    // _TRANSFORM_H_
