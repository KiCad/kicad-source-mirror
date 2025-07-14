/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2011 jean-pierre.charras
 * Copyright (C) 2022 Mike Williams
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

#include <board_item.h>
#include <bitmap_base.h>
#include <reference_image.h>


/**
 * Object to handle a bitmap image that can be inserted in a PCB.
 */
class PCB_REFERENCE_IMAGE : public BOARD_ITEM
{
public:
    PCB_REFERENCE_IMAGE( BOARD_ITEM* aParent, const VECTOR2I& pos = VECTOR2I( 0, 0 ),
                         PCB_LAYER_ID aLayer = F_Cu );

    PCB_REFERENCE_IMAGE( const PCB_REFERENCE_IMAGE& aPcbBitmap );

    ~PCB_REFERENCE_IMAGE();

    void CopyFrom( const BOARD_ITEM* aOther ) override;

    PCB_REFERENCE_IMAGE& operator=( const BOARD_ITEM& aItem );

    /**
     * @return the underlying reference image object.
     */
    REFERENCE_IMAGE&       GetReferenceImage() { return m_referenceImage; }
    const REFERENCE_IMAGE& GetReferenceImage() const { return m_referenceImage; }

    static inline bool ClassOf( const EDA_ITEM* aItem )
    {
        return aItem && PCB_REFERENCE_IMAGE_T == aItem->Type();
    }

    wxString GetClass() const override { return wxT( "PCB_REFERENCE_IMAGE" ); }

    double ViewGetLOD( int aLayer, const KIGFX::VIEW* aView ) const override;

    const BOX2I GetBoundingBox() const override;

    std::shared_ptr<SHAPE> GetEffectiveShape( PCB_LAYER_ID aLayer = UNDEFINED_LAYER,
                                              FLASHING aFlash = FLASHING::DEFAULT ) const override;

    //void Print( const RENDER_SETTINGS* aSettings, const VECTOR2I& aOffset ) override;

    /// @copydoc VIEW_ITEM::ViewGetLayers()
    virtual std::vector<int> ViewGetLayers() const override;

    void Move( const VECTOR2I& aMoveVector ) override;

    void Flip( const VECTOR2I& aCentre, FLIP_DIRECTION aFlipDirection ) override;
    void Rotate( const VECTOR2I& aCenter, const EDA_ANGLE& aAngle ) override;

    wxString GetItemDescription( UNITS_PROVIDER* aUnitsProvider, bool aFull ) const override
    {
        return wxString( _( "Reference Image" ) );
    }

    BITMAPS GetMenuImage() const override;

    void GetMsgPanelInfo( EDA_DRAW_FRAME* aFrame, std::vector<MSG_PANEL_ITEM>& aList ) override;

    /**
     * Get the position of the image (this is the center of the image).
     */
    VECTOR2I GetPosition() const override;

    /**
     * Set the position of the image.
     *
     * If this results in the image overflowing the coordinate system, nothing is updated.
     */
    void SetPosition( const VECTOR2I& aPosition ) override;

    bool HitTest( const VECTOR2I& aPosition, int aAccuracy = 0 ) const override;
    bool HitTest( const BOX2I& aRect, bool aContained, int aAccuracy = 0 ) const override;
    bool HitTest( const SHAPE_LINE_CHAIN& aPoly, bool aContained ) const override;

    EDA_ITEM* Clone() const override;

    double Similarity( const BOARD_ITEM& aBoardItem ) const override;

    bool operator==( const PCB_REFERENCE_IMAGE& aOther ) const;
    bool operator==( const BOARD_ITEM& aBoardItem ) const override;

#if defined( DEBUG )
    void Show( int nestLevel, std::ostream& os ) const override;
#endif

protected:
    void swapData( BOARD_ITEM* aItem ) override;

private:
    friend struct PCB_REFERENCE_IMAGE_DESC;

    // Property manager interfaces
    int  GetWidth() const;
    void SetWidth( int aWidth );
    int  GetHeight() const;
    void SetHeight( int aHeight );
    int  GetTransformOriginOffsetX() const;
    void SetTransformOriginOffsetX( int aX );
    int  GetTransformOriginOffsetY() const;
    void SetTransformOriginOffsetY( int aY );

    double GetImageScale() const;
    void   SetImageScale( double aScale );

    REFERENCE_IMAGE m_referenceImage;
};
