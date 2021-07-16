/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2011 jean-pierre.charras
 * Copyright (C) 2011-2021 KiCad Developers, see AUTHORS.txt for contributors.
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
 */

#ifndef _SCH_BITMAP_H_
#define _SCH_BITMAP_H_


#include <sch_item.h>
#include <bitmap_base.h>


/**
 * Object to handle a bitmap image that can be inserted in a schematic.
 */
class SCH_BITMAP : public SCH_ITEM
{
public:
    SCH_BITMAP( const wxPoint& pos = wxPoint( 0, 0 ) );

    SCH_BITMAP( const SCH_BITMAP& aSchBitmap );

    ~SCH_BITMAP()
    {
        delete m_image;
    }

    SCH_BITMAP& operator=( const SCH_ITEM& aItem );

    BITMAP_BASE* GetImage() const
    {
        wxCHECK_MSG( m_image != nullptr, nullptr, "Invalid SCH_BITMAP init, m_image is NULL." );
        return m_image;
    }

    /**
     * @return the image "zoom" value.
     *  scale = 1.0 = original size of bitmap.
     *  scale < 1.0 = the bitmap is drawn smaller than its original size.
     *  scale > 1.0 = the bitmap is drawn bigger than its original size.
     */
    double GetImageScale() const
    {
        return m_image->GetScale();
    }

    void SetImageScale( double aScale )
    {
        m_image->SetScale( aScale );
    }

    static inline bool ClassOf( const EDA_ITEM* aItem )
    {
        return aItem && SCH_BITMAP_T == aItem->Type();
    }

    wxString GetClass() const override
    {
        return wxT( "SCH_BITMAP" );
    }

    /**
     * @return the actual size (in user units, not in pixels) of the image.
     */
    wxSize GetSize() const;

    const EDA_RECT GetBoundingBox() const override;

    void SwapData( SCH_ITEM* aItem ) override;

    void Print( const RENDER_SETTINGS* aSettings, const wxPoint& aOffset ) override;

    /// @copydoc VIEW_ITEM::ViewGetLayers()
    virtual void ViewGetLayers( int aLayers[], int& aCount ) const override;

   /**
     * Read and store an image file.
     *
     * Initialize the bitmap used to draw this item format.
     *
     * @param aFullFilename is the full filename of the image file to read.
     * @return true if success reading else false.
     */
    bool ReadImageFile( const wxString& aFullFilename );

    void Move( const wxPoint& aMoveVector ) override
    {
        m_pos += aMoveVector;
    }

    /**
     * Return true for items which are moved with the anchor point at mouse cursor and false
     * for items moved with no reference to anchor.
     *
     * @return false for a bus entry.
     */
    bool IsMovableFromAnchorPoint() const override { return false; }

    void MirrorHorizontally( int aCenter ) override;
    void MirrorVertically( int aCenter ) override;
    void Rotate( const wxPoint& aCenter ) override;

    wxString GetSelectMenuText( EDA_UNITS aUnits ) const override
    {
        return wxString( _( "Image" ) );
    }

    BITMAPS GetMenuImage() const override;

    void GetMsgPanelInfo( EDA_DRAW_FRAME* aFrame, std::vector<MSG_PANEL_ITEM>& aList ) override;

    wxPoint GetPosition() const override { return m_pos; }
    void SetPosition( const wxPoint& aPosition ) override { m_pos = aPosition; }

    bool HitTest( const wxPoint& aPosition, int aAccuracy = 0 ) const override;
    bool HitTest( const EDA_RECT& aRect, bool aContained, int aAccuracy = 0 ) const override;

    void Plot( PLOTTER* aPlotter ) const override;

    EDA_ITEM* Clone() const override;

#if defined(DEBUG)
    void Show( int nestLevel, std::ostream& os ) const override;
#endif

private:
    wxPoint      m_pos;                 // XY coordinates of center of the bitmap
    BITMAP_BASE* m_image;               // the BITMAP_BASE item
};


#endif    // _SCH_BITMAP_H_
