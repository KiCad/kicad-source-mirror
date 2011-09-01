/**
 * @file class_bitmap_base.h
 *
 */

/*
 * This program source code file is part of KICAD, a free EDA CAD application.
 *
 * Copyright (C) 2011 jean-pierre.charras
 * Copyright (C) 2011 Kicad Developers, see change_log.txt for contributors.
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

#ifndef _BITMAP_BASE_H_
#define _BITMAP_BASE_H_


#include "sch_item_struct.h"

/**
 * This class handle bitmap images in kicad.
 * It is not intended to be used alone, but inside an other class,
 * so all methods are protected ( or private )
 * It is used in SCH_BITMAP class  (and other in futute)
 *
 * Remember not all plotters are able to plot a bitmap
 * Mainly GERBER plotters cannot.
 */
class BITMAP_BASE
{
public:
    double    m_Scale;              // The scaling factor of the bitmap
                                    // With m_pixelScaleFactor, controls the actual draw size
private:
    wxImage*  m_image;              // the raw image data (png format)
    wxBitmap* m_bitmap;             // the bitmap used to draw/plot image
    double    m_pixelScaleFactor;   // The scaling factor of the bitmap
                                    // to convert the bitmap size (in pixels)
                                    // to internal Kicad units
                                    // Usually does not change


public: BITMAP_BASE( const wxPoint& pos = wxPoint( 0, 0 ) );

    BITMAP_BASE( const BITMAP_BASE& aSchBitmap );

    ~BITMAP_BASE()
    {
        delete m_bitmap;
        delete m_image;
    }


    /*
     * Accessors:
     */
    double GetPixelScaleFactor() { return m_pixelScaleFactor; }
    void SetPixelScaleFactor( double aSF ) { m_pixelScaleFactor = aSF; }

    /**
     * Function ImportData
     * Copy aItem image to me and update m_bitmap
     */
    void ImportData( BITMAP_BASE* aItem );

    /**
     * Function GetScalingFactor
     * @return the scaling factor from pixel size to actual draw size
     * this scaling factor  depend on m_pixelScaleFactor and m_Scale
     * m_pixelScaleFactor gives the scaling factor between a pixel size and
     * the internal schematic units
     * m_Scale is an user dependant value, and gives the "zoom" value
     *  m_Scale = 1.0 = original size of bitmap.
     *  m_Scale < 1.0 = the bitmap is drawn smaller than its original size.
     *  m_Scale > 1.0 = the bitmap is drawn bigger than its original size.
     */
    double GetScalingFactor() const
    {
        return m_pixelScaleFactor * m_Scale;
    }


    /**
     * Function GetSize
     * @returns the actual size (in user units, not in pixels) of the image
     */
    wxSize   GetSize() const;

    /**
     * Function GetBoundingBox
     * returns the orthogonal, bounding box of this object for display
     * purposes.  This box should be an enclosing perimeter for visible
     * components of this object, and the units should be in the pcb or
     * schematic coordinate system.  It is OK to overestimate the size
     * by a few counts.
     */
    EDA_RECT GetBoundingBox() const;

    void     DrawBitmap( EDA_DRAW_PANEL* aPanel, wxDC* aDC, const wxPoint& aPos );

    /**
     * Function ReadImageFile
     * Reads and stores an image file. Init the bitmap used to draw this item
     * format.
     * supported images formats are format supported by wxImage
     * if all handlers are loaded
     * by default, .png, .jpeg are alway loaded
     * @param aFullFilename The full filename of the image file to read.
     * @return bool - true if success reading else false.
     */
    bool     ReadImageFile( const wxString& aFullFilename );

    /**
     * Function
     * writes the bitmap data to aFile
     * The format is png, in Hexadecimal form:
     * If the hexadecimal data is converted to binary it gives exactly a .png image data
     * @param aFile The FILE to write to.
     * @return bool - true if success writing else false.
     */
    bool     SaveData( FILE* aFile ) const;

    /**
     * Load an image data saved by SaveData (png, in Hexadecimal form)
     * @param aLine - Essentially this is file to read schematic junction from.
     * @param aErrorMsg - Description of the error if an error occurs while loading the
     *                    png bimap data.
     * @return true if the bitmap loaded successfully.
     */
    bool     LoadData( LINE_READER& aLine, wxString& aErrorMsg );


    /**
     * Function Mirror
     * Mirror image vertically (i.e. relative to its horizontal X axis )
     *  or horizontally (i.e relative to its vertical Y axis)
     * @param aVertically = false to mirror horizontally
     *                      or true to mirror vertically
     */
    void     Mirror( bool aVertically );

    /**
     * Function Rotate
     * Rotate image CW or CCW.
     * @param aRotateCCW = true to rotate CCW
     */
    void     Rotate( bool aRotateCCW );

    /**
     * Function PlotImage
     * Plot bitmap on plotter.
     * If the plotter does not support bitmaps, plot a
     * @param aPlotter = the plotter to use
     * @param aPos = the position od the center of the bitmap
     * @param aDefaultColor = the color used to plot the rectangle when bitmap is not supported
     * @param aDefaultPensize = the pen size used to plot the rectangle when bitmap is not supported
     */
    void     PlotImage( PLOTTER* aPlotter, wxPoint aPos, int aDefaultColor, int aDefaultPensize );
};


#endif    // _BITMAP_BASE_H_
