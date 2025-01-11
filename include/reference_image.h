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

#pragma once

#include <memory>

#include <base_units.h>
#include <core/mirror.h>
#include <math/box2.h>
#include <math/vector2d.h>

class BITMAP_BASE;

class wxMemoryBuffer;
class wxImage;

/**
 * A REFERENCE_IMAGE is a wrapper around a BITMAP_IMAGE that is
 * displayed in an editor as a reference for the user.
 */
class REFERENCE_IMAGE
{
public:
    REFERENCE_IMAGE( const EDA_IU_SCALE& aIuScale );
    REFERENCE_IMAGE( const REFERENCE_IMAGE& aOther );
    ~REFERENCE_IMAGE();

    REFERENCE_IMAGE& operator=( const REFERENCE_IMAGE& aOther );

    bool operator==( const REFERENCE_IMAGE& aOther ) const;

    double Similarity( const REFERENCE_IMAGE& aOther ) const;

    BOX2I GetBoundingBox() const;

    bool HitTest( const BOX2I& aRect, bool aContained, int aAccuracy ) const;

    VECTOR2I GetPosition() const;
    void     SetPosition( const VECTOR2I& aPos );

    VECTOR2I GetSize() const;

    /**
     * @return the image "zoom" value.
     *  scale = 1.0 = original size of bitmap.
     *  scale < 1.0 = the bitmap is drawn smaller than its original size.
     *  scale > 1.0 = the bitmap is drawn bigger than its original size.
     */
    double GetImageScale() const;

    /**
     * Set the image "zoom" value.
     *
     * The image is scaled such that the position of the image's
     * transform origin is unchanged.
     *
     * If the scale is negative or the image would overflow the
     * the coordinate system, nothing is updated.
     */
    void SetImageScale( double aScale );

    void SetWidth( int aWidth );
    void SetHeight( int aHeight );

    void Flip( const VECTOR2I& aCentre, FLIP_DIRECTION aFlipDirection );

    void Rotate( const VECTOR2I& aCenter, const EDA_ANGLE& aAngle );

    /**
     * Read and store an image file.
     *
     * Initialize the bitmap used to draw this item format.
     *
     * @param aFullFilename is the full filename of the image file to read.
     * @return true if success reading else false.
     */
    bool ReadImageFile( const wxString& aFullFilename );

    /**
     * Read and store an image file.
     *
     * Initialize the bitmap used to draw this item format.
     *
     * @param aBuf is the memory buffer containing the image file to read.
     * @return true if success reading else false.
     */
    bool ReadImageFile( wxMemoryBuffer& aBuf );

    /**
     * Set the image from an existing wxImage.
     */
    bool SetImage( const wxImage& aImage );

    void SwapData( REFERENCE_IMAGE& aItem );

    /**
     * Get the underlying image.
     *
     * This will always return a valid reference, but it may be to an empty image.
     */
    const BITMAP_BASE& GetImage() const;

    /**
     * Only use this if you really need to modify the underlying image
     */
    BITMAP_BASE& MutableImage() const;


    /**
     * Get the center of scaling, etc, relative to the image center (GetPosition()).
     */
    VECTOR2I GetTransformOriginOffset() const;
    void     SetTransformOriginOffset( const VECTOR2I& aCenter );

private:
    void scaleBy( double ratio );

    void updatePixelSizeInIU();

    const EDA_IU_SCALE& m_iuScale;

    VECTOR2I m_pos; ///< XY coordinates of center of the bitmap.

    /// Center of scaling, etc, relative to the image center.
    VECTOR2I m_transformOriginOffset;

    std::unique_ptr<BITMAP_BASE> m_bitmapBase;
};
