/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2011 jean-pierre.charras
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

/**
 * @file sch_bitmap.h
 */

#pragma once


#include <sch_item.h>
#include <reference_image.h>


/**
 * Object to handle a bitmap image that can be inserted in a schematic.
 */
class SCH_BITMAP : public SCH_ITEM
{
public:
    SCH_BITMAP( const VECTOR2I& pos = VECTOR2I( 0, 0 ) );

    SCH_BITMAP( const SCH_BITMAP& aSchBitmap );

    SCH_BITMAP& operator=( const SCH_ITEM& aItem );

    /**
     * @return the underlying reference image object.
     */
    REFERENCE_IMAGE&       GetReferenceImage() { return m_referenceImage; }
    const REFERENCE_IMAGE& GetReferenceImage() const { return m_referenceImage; }

    int  GetX() const { return GetPosition().x; };
    void SetX( int aX ) { SetPosition( VECTOR2I( aX, GetY() ) ); }

    int  GetY() const { return GetPosition().y; }
    void SetY( int aY ) { SetPosition( VECTOR2I( GetX(), aY ) ); }

    static inline bool ClassOf( const EDA_ITEM* aItem )
    {
        return aItem && SCH_BITMAP_T == aItem->Type();
    }

    wxString GetClass() const override
    {
        return wxT( "SCH_BITMAP" );
    }

    const BOX2I GetBoundingBox() const override;

    /// @copydoc VIEW_ITEM::ViewGetLayers()
    virtual std::vector<int> ViewGetLayers() const override;

    void Move( const VECTOR2I& aMoveVector ) override;

    /**
     * Return true for items which are moved with the anchor point at mouse cursor and false
     * for items moved with no reference to anchor.
     *
     * @return false for a bus entry.
     */
    bool IsMovableFromAnchorPoint() const override { return false; }

    void MirrorHorizontally( int aCenter ) override;
    void MirrorVertically( int aCenter ) override;
    void Rotate( const VECTOR2I& aCenter, bool aRotateCCW ) override;

    wxString GetItemDescription( UNITS_PROVIDER* aUnitsProvider, bool aFull ) const override
    {
        return wxString( _( "Image" ) );
    }

    BITMAPS GetMenuImage() const override;

    void GetMsgPanelInfo( EDA_DRAW_FRAME* aFrame, std::vector<MSG_PANEL_ITEM>& aList ) override;

    VECTOR2I GetPosition() const override;
    void     SetPosition( const VECTOR2I& aPosition ) override;

    bool HitTest( const VECTOR2I& aPosition, int aAccuracy = 0 ) const override;
    bool HitTest( const BOX2I& aRect, bool aContained, int aAccuracy = 0 ) const override;
    bool HitTest( const SHAPE_LINE_CHAIN& aPoly, bool aContained ) const override;

    void Plot( PLOTTER* aPlotter, bool aBackground, const SCH_PLOT_OPTS& aPlotOpts,
               int aUnit, int aBodyStyle, const VECTOR2I& aOffset, bool aDimmed ) override;

    EDA_ITEM* Clone() const override;

    double Similarity( const SCH_ITEM& aOther ) const override;

    bool operator==( const SCH_ITEM& aOther ) const override;

#if defined(DEBUG)
    void Show( int nestLevel, std::ostream& os ) const override;
#endif

protected:
    void swapData( SCH_ITEM* aItem ) override;

private:
    friend struct SCH_BITMAP_DESC;

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
