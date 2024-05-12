/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2011 jean-pierre.charras
 * Copyright (C) 2011-2023 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <sch_draw_panel.h>
#include <plotters/plotter.h>
#include <settings/color_settings.h>
#include <bitmaps.h>
#include <base_units.h>
#include <common.h>
#include <eda_draw_frame.h>
#include <core/mirror.h>
#include <sch_bitmap.h>
#include <trigo.h>

#include <wx/mstream.h>


SCH_BITMAP::SCH_BITMAP( const VECTOR2I& pos ) :
    SCH_ITEM( nullptr, SCH_BITMAP_T )
{
    m_pos   = pos;
    m_layer = LAYER_NOTES;              // used only to draw/plot a rectangle,
                                        // when a bitmap cannot be drawn or plotted
    m_bitmapBase = new BITMAP_BASE();
    m_bitmapBase->SetPixelSizeIu( (double) schIUScale.MilsToIU( 1000 ) / m_bitmapBase->GetPPI() );
}


SCH_BITMAP::SCH_BITMAP( const SCH_BITMAP& aSchBitmap ) :
    SCH_ITEM( aSchBitmap )
{
    m_pos   = aSchBitmap.m_pos;
    m_layer = aSchBitmap.m_layer;
    m_bitmapBase = new BITMAP_BASE( *aSchBitmap.m_bitmapBase );
}


SCH_BITMAP& SCH_BITMAP::operator=( const SCH_ITEM& aItem )
{
    wxCHECK_MSG( Type() == aItem.Type(), *this,
                 wxT( "Cannot assign object type " ) + aItem.GetClass() + wxT( " to type " ) +
                 GetClass() );

    if( &aItem != this )
    {
        SCH_ITEM::operator=( aItem );

        SCH_BITMAP* bitmap = (SCH_BITMAP*) &aItem;

        delete m_bitmapBase;
        m_bitmapBase = new BITMAP_BASE( *bitmap->m_bitmapBase );
        m_pos = bitmap->m_pos;
    }

    return *this;
}


bool SCH_BITMAP::ReadImageFile( const wxString& aFullFilename )
{
    if( m_bitmapBase->ReadImageFile( aFullFilename ) )
    {
        m_bitmapBase->SetPixelSizeIu( (double) schIUScale.MilsToIU( 1000 ) / m_bitmapBase->GetPPI() );
        return true;
    }

    return false;
}


bool SCH_BITMAP::ReadImageFile( wxMemoryBuffer& aBuffer )
{
    if( m_bitmapBase->ReadImageFile( aBuffer ) )
    {
        m_bitmapBase->SetPixelSizeIu( (double) schIUScale.MilsToIU( 1000 ) / m_bitmapBase->GetPPI() );
        return true;
    }

    return false;
}


EDA_ITEM* SCH_BITMAP::Clone() const
{
    return new SCH_BITMAP( *this );
}


void SCH_BITMAP::SwapData( SCH_ITEM* aItem )
{
    SCH_ITEM::SwapFlags( aItem );

    wxCHECK_RET( aItem->Type() == SCH_BITMAP_T,
                 wxString::Format( wxT( "SCH_BITMAP object cannot swap data with %s object." ),
                                   aItem->GetClass() ) );

    SCH_BITMAP* item = (SCH_BITMAP*) aItem;
    std::swap( m_pos, item->m_pos );
    std::swap( m_bitmapBase, item->m_bitmapBase );
}


const BOX2I SCH_BITMAP::GetBoundingBox() const
{
    BOX2I bbox = m_bitmapBase->GetBoundingBox();
    bbox.Move( m_pos );

    return bbox;
}


void SCH_BITMAP::Print( const SCH_RENDER_SETTINGS* aSettings, int aUnit, int aBodyStyle,
                        const VECTOR2I& aOffset, bool aForceNoFill, bool aDimmed )
{
    VECTOR2I pos = m_pos + aOffset;

    m_bitmapBase->DrawBitmap( aSettings->GetPrintDC(), pos, aSettings->GetBackgroundColor() );
}


VECTOR2I SCH_BITMAP::GetSize() const
{
    return m_bitmapBase->GetSize();
}


void SCH_BITMAP::MirrorVertically( int aCenter )
{
    MIRROR( m_pos.y, aCenter );
    m_bitmapBase->Mirror( true );
}


void SCH_BITMAP::MirrorHorizontally( int aCenter )
{
    MIRROR( m_pos.x, aCenter );
    m_bitmapBase->Mirror( false );
}


void SCH_BITMAP::Rotate( const VECTOR2I& aCenter, bool aRotateCCW )
{
    RotatePoint( m_pos, aCenter, aRotateCCW ? ANGLE_90 : ANGLE_270 );
    m_bitmapBase->Rotate( aRotateCCW );
}


#if defined(DEBUG)
void SCH_BITMAP::Show( int nestLevel, std::ostream& os ) const
{
    // XML output:
    wxString s = GetClass();

    NestedSpace( nestLevel, os ) << '<' << s.Lower().mb_str() << m_pos << "/>\n";
}
#endif


bool SCH_BITMAP::HitTest( const VECTOR2I& aPosition, int aAccuracy ) const
{
    BOX2I rect = GetBoundingBox();

    rect.Inflate( aAccuracy );

    return rect.Contains( aPosition );
}


bool SCH_BITMAP::HitTest( const BOX2I& aRect, bool aContained, int aAccuracy ) const
{
    BOX2I rect = aRect;

    rect.Inflate( aAccuracy );

    if( aContained )
        return rect.Contains( GetBoundingBox() );

    return rect.Intersects( GetBoundingBox() );
}


void SCH_BITMAP::Plot( PLOTTER* aPlotter, bool aBackground, const SCH_PLOT_OPTS& aPlotOpts,
                       int aUnit, int aBodyStyle, const VECTOR2I& aOffset, bool aDimmed )
{
    if( aBackground )
    {
        m_bitmapBase->PlotImage( aPlotter, m_pos,
                                 aPlotter->RenderSettings()->GetLayerColor( GetLayer() ),
                                 aPlotter->RenderSettings()->GetDefaultPenWidth() );
    }
}


BITMAPS SCH_BITMAP::GetMenuImage() const
{
    return BITMAPS::image;
}


void SCH_BITMAP::GetMsgPanelInfo( EDA_DRAW_FRAME* aFrame, std::vector<MSG_PANEL_ITEM>& aList )
{
    aList.emplace_back( _( "Bitmap" ), wxEmptyString );

    aList.emplace_back( _( "PPI" ), wxString::Format( wxT( "%d "), GetImage()->GetPPI() ) );
    aList.emplace_back( _( "Scale" ), wxString::Format( wxT( "%f "), GetImageScale() ) );

    aList.emplace_back( _( "Width" ), aFrame->MessageTextFromValue( GetSize().x ) );
    aList.emplace_back( _( "Height" ), aFrame->MessageTextFromValue( GetSize().y ) );
}


void SCH_BITMAP::ViewGetLayers( int aLayers[], int& aCount ) const
{
    aCount = 2;
    aLayers[0] = LAYER_DRAW_BITMAPS;
    aLayers[1] = LAYER_SELECTION_SHADOWS;
}


static struct SCH_BITMAP_DESC
{
    SCH_BITMAP_DESC()
    {
        PROPERTY_MANAGER& propMgr = PROPERTY_MANAGER::Instance();
        REGISTER_TYPE( SCH_BITMAP );
        propMgr.InheritsAfter( TYPE_HASH( SCH_BITMAP ), TYPE_HASH( SCH_ITEM ) );
    }
} _SCH_BITMAP_DESC;


bool SCH_BITMAP::operator==( const SCH_ITEM& aItem ) const
{
    if( Type() != aItem.Type() )
        return false;

    const SCH_BITMAP* bitmap = static_cast<const SCH_BITMAP*>( &aItem );

    if( GetPosition() != bitmap->GetPosition() )
        return false;

    if( GetSize() != bitmap->GetSize() )
        return false;

    if( GetImage() != bitmap->GetImage() )
        return false;

    return true;
}


double SCH_BITMAP::Similarity( const SCH_ITEM& aItem ) const
{
    if( Type() != aItem.Type() )
        return 0.0;

    if( m_Uuid == aItem.m_Uuid )
        return 1.0;

    const SCH_BITMAP* bitmap = static_cast<const SCH_BITMAP*>( &aItem );

    if( GetImage() != bitmap->GetImage() )
        return 0.0;

    // If it is the same image but a different UUID and a different size,
    // then it _might be different_.
    if( GetSize() != bitmap->GetSize() )
        return 0.5;

    return 1.0;
}