/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2011 jean-pierre.charras
 * Copyright (C) 2011 KiCad Developers, see change_log.txt for contributors.
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
 * @file sch_bitmap.h
 *
 */

#ifndef _SCH_BITMAP_H_
#define _SCH_BITMAP_H_


#include <sch_item_struct.h>
#include <class_bitmap_base.h>


class SCH_BITMAP : public SCH_ITEM
{
    wxPoint      m_Pos;                 // XY coordinates of center of the bitmap

public:
    BITMAP_BASE* m_Image;               // the BITMAP_BASE item


public:
    SCH_BITMAP( const wxPoint& pos = wxPoint( 0, 0 ) );

    SCH_BITMAP( const SCH_BITMAP& aSchBitmap );

    ~SCH_BITMAP()
    {
        delete m_Image;
    }

    SCH_ITEM& operator=( const SCH_ITEM& aItem );

    /*
     * Accessors:
     */
    double GetPixelScaleFactor() { return m_Image->GetPixelScaleFactor(); }
    void SetPixelScaleFactor( double aSF ) { m_Image->SetPixelScaleFactor( aSF ); }

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
        return m_Image->GetScalingFactor();
    }


    virtual wxString GetClass() const
    {
        return wxT( "SCH_BITMAP" );
    }


    /**
     * Function GetSize
     * @returns the actual size (in user units, not in pixels) of the image
     */
    wxSize GetSize() const;

    /**
     * Function GetBoundingBox
     * returns the orthogonal, bounding box of this object for display
     * purposes.  This box should be an enclosing perimeter for visible
     * components of this object, and the units should be in the pcb or
     * schematic coordinate system.  It is OK to overestimate the size
     * by a few counts.
     */
    EDA_RECT GetBoundingBox() const;

    virtual void SwapData( SCH_ITEM* aItem );

    virtual void Draw( EDA_DRAW_PANEL* aPanel, wxDC* aDC, const wxPoint& aOffset,
                       int aDrawMode, int aColor = -1 );

    /**
     * Function ReadImageFile
     * Reads and stores an image file. Init the bitmap used to draw this item
     * format.
     * @param aFullFilename The full filename of the image file to read.
     * @return bool - true if success reading else false.
     */
    bool ReadImageFile( const wxString& aFullFilename );

    /**
     * Function Save
     * writes the data structures for this object out to a FILE in "*.sch"
     * format.
     * @param aFile The FILE to write to.
     * @return bool - true if success writing else false.
     */
    bool Save( FILE* aFile ) const;

    /**
     * Load schematic junction entry from \a aLine in a .sch file.
     *
     * @param aLine - Essentially this is file to read schematic junction from.
     * @param aErrorMsg - Description of the error if an error occurs while loading the
     *                    schematic junction.
     * @return True if the schematic junction loaded successfully.
     */
    virtual bool Load( LINE_READER& aLine, wxString& aErrorMsg );

    /**
     * Function Move
     * moves then item to a new position by \a aMoveVector.
     * @param aMoveVector The displacement vector.
     */
    virtual void Move( const wxPoint& aMoveVector )
    {
        m_Pos += aMoveVector;
    }


    /**
     * Function Mirror_Y
     * mirrors the item relative to \a aYaxis_position.
     * @param aYaxis_position = the y axis position
     */
    virtual void Mirror_Y( int aYaxis_position );

    virtual void Mirror_X( int aXaxis_position );

    virtual void Rotate( wxPoint rotationPoint );

    virtual bool IsSelectStateChanged( const wxRect& aRect );

    virtual wxString GetSelectMenuText() const { return wxString( _( "Image" ) ); }

    virtual BITMAP_DEF GetMenuImage() const { return image_xpm; }

#if defined(DEBUG)
    void Show( int nestLevel, std::ostream& os ) const;     // override
#endif

private:
    virtual bool doHitTest( const wxPoint& aPoint, int aAccuracy ) const;
    virtual bool doHitTest( const EDA_RECT& aRect, bool aContained, int aAccuracy ) const;
    virtual bool doIsConnected( const wxPoint& aPosition ) const;
    virtual EDA_ITEM* doClone() const;
    virtual void doPlot( PLOTTER* aPlotter );
    virtual wxPoint doGetPosition() const { return m_Pos; }
    virtual void doSetPosition( const wxPoint& aPosition ) { m_Pos = aPosition; }
};


#endif    // _SCH_BITMAP_H_
