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
 * @file sch_bitmap.cpp
 */
#include "sch_bitmap.h"

#include <bitmap_base.h>
#include <bitmaps.h>
#include <base_units.h>
#include <common.h>
#include <core/mirror.h>
#include <eda_draw_frame.h>
#include <geometry/geometry_utils.h>
#include <plotters/plotter.h>
#include <sch_draw_panel.h>
#include <settings/color_settings.h>
#include <trigo.h>

#include <wx/mstream.h>


SCH_BITMAP::SCH_BITMAP( const VECTOR2I& pos ) :
    SCH_ITEM( nullptr, SCH_BITMAP_T ),
    m_referenceImage( schIUScale)
{
    m_referenceImage.SetPosition( pos );
    m_layer = LAYER_NOTES;              // used only to draw/plot a rectangle,
                                        // when a bitmap cannot be drawn or plotted
}


SCH_BITMAP::SCH_BITMAP( const SCH_BITMAP& aSchBitmap ) :
    SCH_ITEM( aSchBitmap ),
    m_referenceImage( aSchBitmap.m_referenceImage )
{
}


SCH_BITMAP& SCH_BITMAP::operator=( const SCH_ITEM& aItem )
{
    wxCHECK_MSG( Type() == aItem.Type(), *this,
                 wxT( "Cannot assign object type " ) + aItem.GetClass() + wxT( " to type " ) +
                 GetClass() );

    if( &aItem != this )
    {
        SCH_ITEM::operator=( aItem );

        const SCH_BITMAP& bitmap = static_cast<const SCH_BITMAP&>( aItem );
        m_referenceImage = bitmap.m_referenceImage;
    }

    return *this;
}


EDA_ITEM* SCH_BITMAP::Clone() const
{
    return new SCH_BITMAP( *this );
}


void SCH_BITMAP::swapData( SCH_ITEM* aItem )
{
    wxCHECK_RET( aItem->Type() == SCH_BITMAP_T,
                 wxString::Format( wxT( "SCH_BITMAP object cannot swap data with %s object." ),
                                   aItem->GetClass() ) );

    SCH_BITMAP* item = (SCH_BITMAP*) aItem;
    m_referenceImage.SwapData( item->m_referenceImage );
}


const BOX2I SCH_BITMAP::GetBoundingBox() const
{
    return m_referenceImage.GetBoundingBox();
}


VECTOR2I SCH_BITMAP::GetPosition() const
{
    return m_referenceImage.GetPosition();
}


void SCH_BITMAP::SetPosition( const VECTOR2I& aPosition )
{
    m_referenceImage.SetPosition( aPosition );
}


void SCH_BITMAP::Move( const VECTOR2I& aMoveVector )
{
    SetPosition( GetPosition() + aMoveVector );
}


void SCH_BITMAP::MirrorVertically( int aCenter )
{
    m_referenceImage.Flip( VECTOR2I( 0, aCenter ), FLIP_DIRECTION::TOP_BOTTOM );
}


void SCH_BITMAP::MirrorHorizontally( int aCenter )
{
    m_referenceImage.Flip( VECTOR2I( aCenter, 0 ), FLIP_DIRECTION::LEFT_RIGHT );
}


void SCH_BITMAP::Rotate( const VECTOR2I& aCenter, bool aRotateCCW )
{
    m_referenceImage.Rotate( aCenter, aRotateCCW ? ANGLE_90 : ANGLE_270 );
}


#if defined(DEBUG)
void SCH_BITMAP::Show( int nestLevel, std::ostream& os ) const
{
    // XML output:
    wxString s = GetClass();

    NestedSpace( nestLevel, os ) << '<' << s.Lower().mb_str() << GetPosition() << "/>\n";
}
#endif


bool SCH_BITMAP::HitTest( const VECTOR2I& aPosition, int aAccuracy ) const
{
    return KIGEOM::BoxHitTest( aPosition, GetBoundingBox(), aAccuracy );
}


bool SCH_BITMAP::HitTest( const BOX2I& aRect, bool aContained, int aAccuracy ) const
{
    return KIGEOM::BoxHitTest( aRect, GetBoundingBox(), aContained, aAccuracy );
}


bool SCH_BITMAP::HitTest( const SHAPE_LINE_CHAIN& aPoly, bool aContained ) const
{
    return KIGEOM::BoxHitTest( aPoly, GetBoundingBox(), aContained );
}



void SCH_BITMAP::Plot( PLOTTER* aPlotter, bool aBackground, const SCH_PLOT_OPTS& aPlotOpts,
                       int aUnit, int aBodyStyle, const VECTOR2I& aOffset, bool aDimmed )
{
    if( aBackground )
    {
        RENDER_SETTINGS* cfg = aPlotter->RenderSettings();

        m_referenceImage.GetImage().PlotImage( aPlotter, GetPosition(),
                                               cfg->GetLayerColor( GetLayer() ),
                                               cfg->GetDefaultPenWidth() );
    }
}


BITMAPS SCH_BITMAP::GetMenuImage() const
{
    return BITMAPS::image;
}


void SCH_BITMAP::GetMsgPanelInfo( EDA_DRAW_FRAME* aFrame, std::vector<MSG_PANEL_ITEM>& aList )
{
    aList.emplace_back( _( "Bitmap" ), wxEmptyString );

    aList.emplace_back( _( "PPI" ),
                        wxString::Format( wxT( "%d " ), m_referenceImage.GetImage().GetPPI() ) );
    aList.emplace_back( _( "Scale" ),
                        wxString::Format( wxT( "%f " ), m_referenceImage.GetImageScale() ) );

    aList.emplace_back( _( "Width" ),
                        aFrame->MessageTextFromValue( m_referenceImage.GetSize().x ) );
    aList.emplace_back( _( "Height" ),
                        aFrame->MessageTextFromValue( m_referenceImage.GetSize().y ) );
}


std::vector<int> SCH_BITMAP::ViewGetLayers() const
{
    return { LAYER_DRAW_BITMAPS, LAYER_SELECTION_SHADOWS };
}


bool SCH_BITMAP::operator==( const SCH_ITEM& aItem ) const
{
    if( Type() != aItem.Type() )
        return false;

    const SCH_BITMAP& bitmap = static_cast<const SCH_BITMAP&>( aItem );
    return m_referenceImage == bitmap.m_referenceImage;
}


double SCH_BITMAP::Similarity( const SCH_ITEM& aItem ) const
{
    if( Type() != aItem.Type() )
        return 0.0;

    if( m_Uuid == aItem.m_Uuid )
        return 1.0;

    const SCH_BITMAP& bitmap = static_cast<const SCH_BITMAP&>( aItem );
    return m_referenceImage.Similarity( bitmap.m_referenceImage );
}


int SCH_BITMAP::GetTransformOriginOffsetX() const
{
    return m_referenceImage.GetTransformOriginOffset().x;
}


void SCH_BITMAP::SetTransformOriginOffsetX( int aX )
{
    VECTOR2I offset = m_referenceImage.GetTransformOriginOffset();
    offset.x = aX;
    m_referenceImage.SetTransformOriginOffset( offset );
}


int SCH_BITMAP::GetTransformOriginOffsetY() const
{
    return m_referenceImage.GetTransformOriginOffset().y;
}


void SCH_BITMAP::SetTransformOriginOffsetY( int aY )
{
    VECTOR2I offset = m_referenceImage.GetTransformOriginOffset();
    offset.y = aY;
    m_referenceImage.SetTransformOriginOffset( offset );
}


double SCH_BITMAP::GetImageScale() const
{
    return m_referenceImage.GetImageScale();
}


void SCH_BITMAP::SetImageScale( double aScale )
{
    m_referenceImage.SetImageScale( aScale );
}


int SCH_BITMAP::GetWidth() const
{
    return m_referenceImage.GetImage().GetSize().x;
}


void SCH_BITMAP::SetWidth( int aWidth )
{
    m_referenceImage.SetWidth( aWidth );
}


int SCH_BITMAP::GetHeight() const
{
    return m_referenceImage.GetImage().GetSize().y;
}


void SCH_BITMAP::SetHeight( int aHeight )
{
    m_referenceImage.SetHeight( aHeight );
}


static struct SCH_BITMAP_DESC
{
    SCH_BITMAP_DESC()
    {
        PROPERTY_MANAGER& propMgr = PROPERTY_MANAGER::Instance();
        REGISTER_TYPE( SCH_BITMAP );
        propMgr.InheritsAfter( TYPE_HASH( SCH_BITMAP ), TYPE_HASH( SCH_ITEM ) );

        propMgr.AddProperty( new PROPERTY<SCH_BITMAP, int>( _HKI( "Position X" ),
                                     &SCH_BITMAP::SetX, &SCH_BITMAP::GetX,
                                     PROPERTY_DISPLAY::PT_COORD ) );


        propMgr.AddProperty( new PROPERTY<SCH_BITMAP, int>( _HKI( "Position Y" ),
                                     &SCH_BITMAP::SetY, &SCH_BITMAP::GetY,
                                     PROPERTY_DISPLAY::PT_COORD ) );

        const wxString groupImage = _HKI( "Image Properties" );

        propMgr.AddProperty( new PROPERTY<SCH_BITMAP, double>( _HKI( "Scale" ),
                                     &SCH_BITMAP::SetImageScale, &SCH_BITMAP::GetImageScale ),
                             groupImage );

        propMgr.AddProperty( new PROPERTY<SCH_BITMAP, int>( _HKI( "Transform Offset X" ),
                                     &SCH_BITMAP::SetTransformOriginOffsetX,
                                     &SCH_BITMAP::GetTransformOriginOffsetX,
                                     PROPERTY_DISPLAY::PT_COORD, ORIGIN_TRANSFORMS::ABS_X_COORD ),
                             groupImage );

        propMgr.AddProperty( new PROPERTY<SCH_BITMAP, int>( _HKI( "Transform Offset Y" ),
                                     &SCH_BITMAP::SetTransformOriginOffsetY,
                                     &SCH_BITMAP::GetTransformOriginOffsetY,
                                     PROPERTY_DISPLAY::PT_COORD, ORIGIN_TRANSFORMS::ABS_Y_COORD ),
                             groupImage );

        propMgr.AddProperty( new PROPERTY<SCH_BITMAP, int>( _HKI( "Width" ),
                                     &SCH_BITMAP::SetWidth, &SCH_BITMAP::GetWidth,
                                     PROPERTY_DISPLAY::PT_COORD ),
                             groupImage );

        propMgr.AddProperty( new PROPERTY<SCH_BITMAP, int>( _HKI( "Height" ),
                                     &SCH_BITMAP::SetHeight, &SCH_BITMAP::GetHeight,
                                     PROPERTY_DISPLAY::PT_COORD ),
                             groupImage );
    }
} _SCH_BITMAP_DESC;
