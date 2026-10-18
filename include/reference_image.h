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

#include <memory>

#include <base_units.h>
#include <core/mirror.h>
#include <gal/color4d.h>
#include <geometry/eda_angle.h>
#include <math/box2.h>
#include <math/vector2d.h>

class BITMAP_BASE;
class PLOTTER;

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

    /**
     *
     * @param aCentre
     * @param aFlipDirection LEFT_RIGHT or TOP_BOTTOM
     * @return false if the flip would put the coordinates out-of-bounds
     */
    bool Flip( const VECTOR2I& aCentre, FLIP_DIRECTION aFlipDirection );

    /**
     * Rotate the image about \a aCenter.
     *
     * The position and the transform origin follow \a aAngle exactly.  The rotation is stored
     * rather than turned into the raster, so the image keeps its resolution and a save followed
     * by a load is lossless for the formats that carry the rotation.
     */
    void Rotate( const VECTOR2I& aCenter, const EDA_ANGLE& aAngle );

    /**
     * @return the rotation of the image content relative to the stored raster.
     */
    EDA_ANGLE GetOrientation() const;

    /**
     * Set the rotation of the image content relative to the stored raster.
     */
    void SetOrientation( const EDA_ANGLE& aAngle );

    double GetOpacity() const { return m_opacity; }
    void   SetOpacity( double aOpacity );

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

    void PackToBytes( std::string& aOutputBytes ) const;
    bool UnpackFromBytes( const std::string& aInputBytes );

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
     * Plot the image on a plotter, turned to its stored orientation.
     *
     * This may resample the image depending on the orientation.
     *
     * @param aPlotter the plotter to use.
     * @param aPos the position of the center of the image.
     * @param aDefaultColor the color used to plot the rectangle when bitmap is not supported.
     * @param aDefaultPensize the pen size used to plot the rectangle when bitmap is not supported.
     */
    void PlotImage( PLOTTER* aPlotter, const VECTOR2I& aPos, const KIGFX::COLOR4D& aDefaultColor,
                    int aDefaultPensize ) const;


    /**
     * Get the center of scaling, etc, relative to the image center (GetPosition()).
     */
    VECTOR2I GetTransformOriginOffset() const;
    void     SetTransformOriginOffset( const VECTOR2I& aCenter );

private:
    void scaleBy( double ratio );

    void updatePixelSizeInIU();

    /**
     * @return the size of the axis-aligned bounding box that contains the rotated image.
     */
    VECTOR2I boundingSize() const;

    const EDA_IU_SCALE& m_iuScale;

    VECTOR2I m_pos; ///< XY coordinates of center of the bitmap.

    /// Center of scaling, etc, relative to the image center.
    VECTOR2I m_transformOriginOffset;

    /// Rotation of the image content relative to the raster, in [0, 360).
    EDA_ORIENTATION m_orientation;

    /// The opacity of the image, in the range [0, 1].
    double m_opacity;

    std::unique_ptr<BITMAP_BASE> m_bitmapBase;
};
