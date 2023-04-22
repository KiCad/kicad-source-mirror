/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2011 jean-pierre.charras
 * Copyright (C) 2022 Mike Williams
 * Copyright (C) 2011-2022 KiCad Developers, see AUTHORS.txt for contributors.
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
 * @file pcb_bitmap.h
 */

#ifndef _PCB_BITMAP_H_
#define _PCB_BITMAP_H_


#include <board_item.h>
#include <bitmap_base.h>


/**
 * Object to handle a bitmap image that can be inserted in a PCB.
 */
class PCB_BITMAP : public BOARD_ITEM
{
public:
    PCB_BITMAP( BOARD_ITEM* aParent, const VECTOR2I& pos = VECTOR2I( 0, 0 ),
                PCB_LAYER_ID aLayer = F_Cu );

    PCB_BITMAP( const PCB_BITMAP& aPcbBitmap );

    ~PCB_BITMAP() { delete m_bitmapBase; }

    PCB_BITMAP& operator=( const BOARD_ITEM& aItem );

    const BITMAP_BASE* GetImage() const
    {
        wxCHECK_MSG( m_bitmapBase != nullptr, nullptr,
                     "Invalid PCB_BITMAP init, m_bitmapBase is NULL." );
        return m_bitmapBase;
    }

    /**
     * Only use this if you really need to modify the underlying image
     */
    BITMAP_BASE* MutableImage() const
    {
        return m_bitmapBase;
    }

    void SetImage( wxImage* aImage );

    /**
     * @return the image "zoom" value.
     *  scale = 1.0 = original size of bitmap.
     *  scale < 1.0 = the bitmap is drawn smaller than its original size.
     *  scale > 1.0 = the bitmap is drawn bigger than its original size.
     */
    double GetImageScale() const { return m_bitmapBase->GetScale(); }

    void SetImageScale( double aScale ) { m_bitmapBase->SetScale( aScale ); }

    static inline bool ClassOf( const EDA_ITEM* aItem )
    {
        return aItem && PCB_BITMAP_T == aItem->Type();
    }

    wxString GetClass() const override { return wxT( "PCB_BITMAP" ); }

    /**
     * @return the actual size (in user units, not in pixels) of the image.
     */
    const VECTOR2I GetSize() const;

    double ViewGetLOD( int aLayer, KIGFX::VIEW* aView ) const override;

    const BOX2I GetBoundingBox() const override;

    std::shared_ptr<SHAPE> GetEffectiveShape( PCB_LAYER_ID aLayer = UNDEFINED_LAYER,
                                              FLASHING aFlash = FLASHING::DEFAULT ) const override;

    //void Print( const RENDER_SETTINGS* aSettings, const VECTOR2I& aOffset ) override;

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

    void Move( const VECTOR2I& aMoveVector ) override { m_pos += aMoveVector; }

    void Flip( const VECTOR2I& aCentre, bool aFlipLeftRight ) override;
    void Rotate( const VECTOR2I& aCenter, const EDA_ANGLE& aAngle ) override;

    wxString GetItemDescription( UNITS_PROVIDER* aUnitsProvider ) const override
    {
        return wxString( _( "Image" ) );
    }

    BITMAPS GetMenuImage() const override;

    void GetMsgPanelInfo( EDA_DRAW_FRAME* aFrame, std::vector<MSG_PANEL_ITEM>& aList ) override;

    VECTOR2I GetPosition() const override { return m_pos; }
    void     SetPosition( const VECTOR2I& aPosition ) override { m_pos = aPosition; }

    bool HitTest( const VECTOR2I& aPosition, int aAccuracy = 0 ) const override;
    bool HitTest( const BOX2I& aRect, bool aContained, int aAccuracy = 0 ) const override;

    EDA_ITEM* Clone() const override;

#if defined( DEBUG )
    void Show( int nestLevel, std::ostream& os ) const override;
#endif

protected:
    void swapData( BOARD_ITEM* aItem ) override;

private:
    VECTOR2I     m_pos;   // XY coordinates of center of the bitmap
    BITMAP_BASE* m_bitmapBase; // the BITMAP_BASE item
};

#endif // _PCB_BITMAP_H_
