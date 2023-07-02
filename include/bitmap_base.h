/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2018 jean-pierre.charras jp.charras at wanadoo.fr
 * Copyright (C) 2013-2023 KiCad Developers, see AUTHORS.txt for contributors.
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

#ifndef BITMAP_BASE_H
#define BITMAP_BASE_H

#include <wx/bitmap.h>
#include <wx/image.h>
#include <kiid.h>
#include <math/box2.h>
#include <gal/color4d.h>

class LINE_READER;
class PLOTTER;


/**
 * This class handle bitmap images in KiCad.
 *
 * It is not intended to be used alone, but inside another class so all methods are protected
 * or private.  It is used in #SCH_BITMAP class, #DS_DRAW_ITEM_BITMAP, and possibly others in
 * the future.
 *
 * @warning Not all plotters are able to plot a bitmap.  Mainly GERBER plotters cannot.
 */
class BITMAP_BASE
{
public:
    BITMAP_BASE( const VECTOR2I& pos = VECTOR2I( 0, 0 ) );

    BITMAP_BASE( const BITMAP_BASE& aSchBitmap );

    ~BITMAP_BASE()
    {
        delete m_bitmap;
        delete m_image;
        delete m_originalImage;
    }

    /*
     * Accessors:
     */
    double GetPixelSizeIu() const { return m_pixelSizeIu; }
    void SetPixelSizeIu( double aPixSize ) { m_pixelSizeIu = aPixSize; }

    wxImage* GetImageData() { return m_image; }
    const wxImage* GetImageData() const { return m_image; }

    const wxImage* GetOriginalImageData() const { return m_originalImage; }

    void SetImage( wxImage* aImage );

    double GetScale() const { return m_scale; }
    void SetScale( double aScale ) { m_scale = aScale; }

    KIID GetImageID() const { return m_imageId; }

    /**
     * Copy aItem image to this object and update #m_bitmap.
     */
    void ImportData( BITMAP_BASE* aItem );

    /**
     * This scaling factor depends on #m_pixelSizeIu and #m_scale.
     *
     * #m_pixelSizeIu gives the scaling factor between a pixel size and the internal units.
     * #m_scale is an user dependent value, and gives the "zoom" value.
     *  - #m_scale = 1.0 = original size of bitmap.
     *  - #m_scale < 1.0 = the bitmap is drawn smaller than its original size.
     *  - #m_scale > 1.0 = the bitmap is drawn bigger than its original size.
     *
     * @return The scaling factor from pixel size to actual draw size.
     */
    double GetScalingFactor() const
    {
        return m_pixelSizeIu * m_scale;
    }

    /**
     * @return the actual size (in user units, not in pixels) of the image
     */
    VECTOR2I GetSize() const;

    /**
     * @return the size in pixels of the image
     */
    VECTOR2I GetSizePixels() const
    {
        if( m_image )
            return VECTOR2I( m_image->GetWidth(), m_image->GetHeight() );
        else
            return VECTOR2I( 0, 0 );
    }

    /**
     * @return the bitmap definition in ppi, the default is 300 ppi.
     */
    int GetPPI() const
    {
        return m_ppi;
    }

    /**
     * Return the orthogonal, bounding box of this object for display purposes.
     *
     * This box should be an enclosing perimeter for visible components of this object,
     * and the units should be in the pcb or schematic coordinate system.  It is OK to
     * overestimate the size by a few counts.
     */
    const BOX2I GetBoundingBox() const;

    void DrawBitmap( wxDC* aDC, const VECTOR2I& aPos,
                     const KIGFX::COLOR4D& aBackgroundColor = KIGFX::COLOR4D::UNSPECIFIED );

    /**
     * Reads and stores in memory an image file.
     *
     * Initialize the bitmap format used to draw this item.  Supported images formats are
     * format supported by wxImage if all handlers are loaded.  By default, .png, .jpeg
     * are always loaded.
     *
     * @param aFullFilename The full filename of the image file to read.
     * @return  true if success reading else false.
     */
    bool ReadImageFile( const wxString& aFullFilename );

    /**
     * Reads and stores in memory an image file.
     *
     * Initialize the bitmap format used to draw this item.
     *
     * Supported images formats are format supported by wxImage if all handlers are loaded.
     * By default, .png, .jpeg are always loaded.
     *
     * @param aInStream an input stream containing the file data.
     * @return true if success reading else false.
     */
    bool ReadImageFile( wxInputStream& aInStream );

    /**
     * Write the bitmap data to \a aFile.
     *
     * The file format is png, in hexadecimal form.  If the hexadecimal data is converted to
     * binary it gives exactly a .png image data.
     *
     * @param aFile The FILE to write to.
     * @return true if success writing else false.
     */
    bool SaveData( FILE* aFile ) const;

    /**
     * Write the bitmap data to an array string.
     *
     * The format is png, in Hexadecimal form.  If the hexadecimal data is converted to binary
     * it gives exactly a .png image data.
     *
     * @param aPngStrings The wxArrayString to write to.
     */
    void SaveData( wxArrayString& aPngStrings ) const;

    /**
     * Load an image data saved by #SaveData.
     *
     * The file format must be png format in hexadecimal.
     *
     * @param aLine the LINE_READER used to read the data file.
     * @param aErrorMsg Description of the error if an error occurs while loading the
     *                  png bitmap data.
     * @return true if the bitmap loaded successfully.
     */
    bool LoadData( LINE_READER& aLine, wxString& aErrorMsg );

    /**
     * Mirror image vertically (i.e. relative to its horizontal X axis ) or horizontally (i.e
     * relative to its vertical Y axis).
     * @param aVertically false to mirror horizontally or true to mirror vertically.
     */
    void Mirror( bool aVertically );

    /**
     * Rotate image CW or CCW.
     *
     * @param aRotateCCW true to rotate CCW or false to rotate CW.
     */
    void Rotate( bool aRotateCCW );

    void ConvertToGreyscale();

    bool IsMirroredX() const { return m_isMirroredX; }
    bool IsMirroredY() const { return m_isMirroredY; }
    EDA_ANGLE Rotation() const { return m_rotation; }

    /**
     * Plot bitmap on plotter.
     *
     * If the plotter does not support bitmaps, plot a
     *
     * @param aPlotter the plotter to use.
     * @param aPos the position of the center of the bitmap.
     * @param aDefaultColor the color used to plot the rectangle when bitmap is not supported.
     * @param aDefaultPensize the pen size used to plot the rectangle when bitmap is not supported.
     */
    void PlotImage( PLOTTER* aPlotter, const VECTOR2I& aPos,
                    const KIGFX::COLOR4D& aDefaultColor, int aDefaultPensize ) const;

private:
    /*
     * Rebuild the internal bitmap used to draw/plot image.
     *
     * This must be called after a #m_image change.
     *
     * @param aResetID is used to reset the cache ID used for OpenGL rendering.
     */
    void rebuildBitmap( bool aResetID = true );

    void updatePPI();

    double    m_scale;              // The scaling factor of the bitmap
                                    // With m_pixelSizeIu, controls the actual draw size
    wxImage*  m_image;              // the raw image data (png format)
    wxImage*  m_originalImage;      // Raw image data, not transformed by rotate/mirror
    wxBitmap* m_bitmap;             // the bitmap used to draw/plot image
    double    m_pixelSizeIu;        // The scaling factor of the bitmap
                                    // to convert the bitmap size (in pixels)
                                    // to internal KiCad units
                                    // Usually does not change
    int       m_ppi;                // the bitmap definition. the default is 300PPI
    KIID      m_imageId;
    bool      m_isMirroredX;        // Used for OpenGL rendering only
    bool      m_isMirroredY;        // Used for OpenGL rendering only
    EDA_ANGLE m_rotation;           // Used for OpenGL rendering only
};


#endif    // BITMAP_BASE_H
