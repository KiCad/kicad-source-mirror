/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2009 Jean-Pierre Charras, jaen-pierre.charras@gipsa-lab.inpg.com
 * Copyright (C) 1992-2019 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <sch_draw_panel.h>
#include <trigo.h>
#include <widgets/msgpanel.h>
#include <bitmaps.h>
#include <base_units.h>
#include <erc_settings.h>
#include <sch_marker.h>
#include <schematic.h>
#include <widgets/ui_common.h>
#include <pgm_base.h>
#include <settings/settings_manager.h>
#include <settings/color_settings.h>
#include <erc_item.h>

/// Factor to convert the maker unit shape to internal units:
#define SCALING_FACTOR  Millimeter2iu( 0.15 )


SCH_MARKER::SCH_MARKER( std::shared_ptr<ERC_ITEM> aItem, const wxPoint& aPos ) :
        SCH_ITEM( nullptr, SCH_MARKER_T ),
        MARKER_BASE( SCALING_FACTOR, aItem, MARKER_BASE::MARKER_ERC )
{
    if( m_rcItem )
        m_rcItem->SetParent( this );

    m_Pos = aPos;
}


EDA_ITEM* SCH_MARKER::Clone() const
{
    return new SCH_MARKER( *this );
}


void SCH_MARKER::SwapData( SCH_ITEM* aItem )
{
    std::swap( *((SCH_MARKER*) this), *((SCH_MARKER*) aItem ) );
}


wxString SCH_MARKER::Serialize() const
{
    return wxString::Format( wxT( "%s|%d|%d|%s|%s" ),
                             m_rcItem->GetSettingsKey(),
                             m_Pos.x,
                             m_Pos.y,
                             m_rcItem->GetMainItemID().AsString(),
                             m_rcItem->GetAuxItemID().AsString() );
}


SCH_MARKER* SCH_MARKER::Deserialize( const wxString& data )
{
    wxArrayString props = wxSplit( data, '|' );
    wxPoint       markerPos( (int) strtol( props[1].c_str(), nullptr, 10 ),
                             (int) strtol( props[2].c_str(), nullptr, 10 ) );

    std::shared_ptr<ERC_ITEM> ercItem = ERC_ITEM::Create( props[0] );

    if( !ercItem )
        return nullptr;

    ercItem->SetItems( KIID( props[3] ), KIID( props[4] ) );

    return new SCH_MARKER( ercItem, markerPos );
}


#if defined(DEBUG)

void SCH_MARKER::Show( int nestLevel, std::ostream& os ) const
{
    // for now, make it look like XML:
    NestedSpace( nestLevel, os ) << '<' << GetClass().Lower().mb_str() << GetPos() << "/>\n";
}

#endif


void SCH_MARKER::ViewGetLayers( int aLayers[], int& aCount ) const
{
    aCount = 2;

    wxCHECK_RET( Schematic(), "No SCHEMATIC set for SCH_MARKER!" );

    switch( Schematic()->ErcSettings().GetSeverity( m_rcItem->GetErrorCode() ) )
    {
    default:
    case SEVERITY::RPT_SEVERITY_ERROR:   aLayers[0] = LAYER_ERC_ERR;  break;
    case SEVERITY::RPT_SEVERITY_WARNING: aLayers[0] = LAYER_ERC_WARN; break;
    }

    aLayers[1] = LAYER_SELECTION_SHADOWS;
}


SCH_LAYER_ID SCH_MARKER::GetColorLayer() const
{
    if( IsExcluded() )
        return LAYER_HIDDEN;

    wxCHECK_MSG( Schematic(), LAYER_ERC_ERR, "No SCHEMATIC set for SCH_MARKER!" );

    switch( Schematic()->ErcSettings().GetSeverity( m_rcItem->GetErrorCode() ) )
    {
    default:
    case SEVERITY::RPT_SEVERITY_ERROR:   return LAYER_ERC_ERR;
    case SEVERITY::RPT_SEVERITY_WARNING: return LAYER_ERC_WARN;
    }
}


KIGFX::COLOR4D SCH_MARKER::getColor() const
{
    COLOR_SETTINGS* colors = Pgm().GetSettingsManager().GetColorSettings();
    return colors->GetColor( GetColorLayer() );
}


void SCH_MARKER::Print( const RENDER_SETTINGS* aSettings, const wxPoint& aOffset )
{
    PrintMarker( aSettings, aOffset );
}


bool SCH_MARKER::Matches( wxFindReplaceData& aSearchData, void* aAuxData )
{
    return SCH_ITEM::Matches( m_rcItem->GetErrorMessage(), aSearchData );
}


const EDA_RECT SCH_MARKER::GetBoundingBox() const
{
    return GetBoundingBoxMarker();
}


void SCH_MARKER::GetMsgPanelInfo( EDA_DRAW_FRAME* aFrame, MSG_PANEL_ITEMS& aList )
{
    aList.push_back( MSG_PANEL_ITEM( _( "Electronics Rule Check Error" ),
                                     m_rcItem->GetErrorMessage() ) );
}


BITMAP_DEF SCH_MARKER::GetMenuImage() const
{
    return erc_xpm;
}


void SCH_MARKER::Rotate( wxPoint aPosition )
{
    // Marker geometry isn't user-editable
}


void SCH_MARKER::MirrorX( int aXaxis_position )
{
    // Marker geometry isn't user-editable
}


void SCH_MARKER::MirrorY( int aYaxis_position )
{
    // Marker geometry isn't user-editable
}


bool SCH_MARKER::HitTest( const wxPoint& aPosition, int aAccuracy ) const
{
    return HitTestMarker( aPosition, aAccuracy );
}
