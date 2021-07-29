/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2018 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2012 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright (C) 1992-2018 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <bitmaps.h>
#include <base_units.h>
#include <eda_draw_frame.h>
#include <board.h>
#include <board_design_settings.h>
#include <pcb_marker.h>
#include <layer_ids.h>
#include <settings/color_settings.h>
#include <settings/settings_manager.h>
#include <geometry/shape_null.h>
#include <widgets/ui_common.h>
#include <pgm_base.h>
#include <drc/drc_item.h>
#include <trigo.h>


/// Factor to convert the maker unit shape to internal units:
#define SCALING_FACTOR  Millimeter2iu( 0.075 )



PCB_MARKER::PCB_MARKER( std::shared_ptr<RC_ITEM> aItem, const wxPoint& aPosition ) :
    BOARD_ITEM( nullptr, PCB_MARKER_T ),  // parent set during BOARD::Add()
    MARKER_BASE( SCALING_FACTOR, aItem )
{
    if( m_rcItem )
        m_rcItem->SetParent( this );

    m_Pos = aPosition;
}


/* destructor */
PCB_MARKER::~PCB_MARKER()
{
}


wxString PCB_MARKER::Serialize() const
{
    return wxString::Format( wxT( "%s|%d|%d|%s|%s" ),
                             m_rcItem->GetSettingsKey(),
                             m_Pos.x,
                             m_Pos.y,
                             m_rcItem->GetMainItemID().AsString(),
                             m_rcItem->GetAuxItemID().AsString() );
}


PCB_MARKER* PCB_MARKER::Deserialize( const wxString& data )
{
    wxArrayString props = wxSplit( data, '|' );
    wxPoint       markerPos( (int) strtol( props[1].c_str(), nullptr, 10 ),
                             (int) strtol( props[2].c_str(), nullptr, 10 ) );

    std::shared_ptr<DRC_ITEM> drcItem =  DRC_ITEM::Create( props[0] );

    if( !drcItem )
        return nullptr;

    drcItem->SetItems( KIID( props[3] ), KIID( props[4] ) );

    return new PCB_MARKER( drcItem, markerPos );
}


void PCB_MARKER::GetMsgPanelInfo( EDA_DRAW_FRAME* aFrame, std::vector<MSG_PANEL_ITEM>& aList )
{
    aList.emplace_back( _( "Type" ), _( "Marker" ) );
    aList.emplace_back( _( "Violation" ), m_rcItem->GetErrorMessage() );

    wxString  mainText;
    wxString  auxText;
    EDA_ITEM* mainItem = nullptr;
    EDA_ITEM* auxItem = nullptr;

    if( m_rcItem->GetMainItemID() != niluuid )
        mainItem = aFrame->GetItem( m_rcItem->GetMainItemID() );

    if( m_rcItem->GetAuxItemID() != niluuid )
        auxItem = aFrame->GetItem( m_rcItem->GetAuxItemID() );

    if( mainItem )
        mainText = mainItem->GetSelectMenuText( aFrame->GetUserUnits() );

    if( auxItem )
        auxText = auxItem->GetSelectMenuText( aFrame->GetUserUnits() );

    aList.emplace_back( mainText, auxText );
}


void PCB_MARKER::Rotate( const wxPoint& aRotCentre, double aAngle )
{
    // Marker geometry isn't user-editable
}


void PCB_MARKER::Flip( const wxPoint& aCentre, bool aFlipLeftRight )
{
    // Marker geometry isn't user-editable
}


std::shared_ptr<SHAPE> PCB_MARKER::GetEffectiveShape( PCB_LAYER_ID aLayer ) const
{
    // Markers do not participate in the board geometry space, and therefore have no
    // effectiven shape.
    return std::make_shared<SHAPE_NULL>();
}


wxString PCB_MARKER::GetSelectMenuText( EDA_UNITS aUnits ) const
{
    // m_rcItem->GetErrorMessage() could be used instead, but is probably too long
    // for menu duty.
    return wxString::Format( _( "Marker (%s)" ), m_rcItem->GetErrorText() );
}


BITMAPS PCB_MARKER::GetMenuImage() const
{
    return BITMAPS::drc;
}


void PCB_MARKER::ViewGetLayers( int aLayers[], int& aCount ) const
{
    aCount = 2;

    aLayers[1] = LAYER_MARKER_SHADOWS;

    if( IsExcluded() )
    {
        aLayers[0] = LAYER_DRC_EXCLUSION;
        return;
    }

    BOARD_ITEM_CONTAINER* ancestor = GetParent();

    while( ancestor->GetParent() )
        ancestor = ancestor->GetParent();

    BOARD* board = static_cast<BOARD*>( ancestor );

    switch( board->GetDesignSettings().GetSeverity( m_rcItem->GetErrorCode() ) )
    {
    default:
    case SEVERITY::RPT_SEVERITY_ERROR:   aLayers[0] = LAYER_DRC_ERROR;   break;
    case SEVERITY::RPT_SEVERITY_WARNING: aLayers[0] = LAYER_DRC_WARNING; break;
    }
}


GAL_LAYER_ID PCB_MARKER::GetColorLayer() const
{
    if( IsExcluded() )
        return LAYER_DRC_EXCLUSION;

    BOARD_ITEM_CONTAINER* ancestor = GetParent();

    while( ancestor->GetParent() )
        ancestor = ancestor->GetParent();

    BOARD* board = static_cast<BOARD*>( ancestor );

    switch( board->GetDesignSettings().GetSeverity( m_rcItem->GetErrorCode() ) )
    {
    default:
    case SEVERITY::RPT_SEVERITY_ERROR:   return LAYER_DRC_ERROR;
    case SEVERITY::RPT_SEVERITY_WARNING: return LAYER_DRC_WARNING;
    }
}


KIGFX::COLOR4D PCB_MARKER::getColor() const
{
    COLOR_SETTINGS* colors = Pgm().GetSettingsManager().GetColorSettings();
    return colors->GetColor( GetColorLayer() );
}


const EDA_RECT PCB_MARKER::GetBoundingBox() const
{
    EDA_RECT bbox = m_shapeBoundingBox;

    wxPoint pos = m_Pos;
    pos.x += int( bbox.GetOrigin().x * MarkerScale() );
    pos.y += int( bbox.GetOrigin().y * MarkerScale() );

    return EDA_RECT( pos, wxSize( int( bbox.GetWidth() * MarkerScale() ),
                                  int( bbox.GetHeight() * MarkerScale() ) ) );
}


const BOX2I PCB_MARKER::ViewBBox() const
{
    EDA_RECT bbox = GetBoundingBox();
    return BOX2I( bbox.GetOrigin(), VECTOR2I( bbox.GetWidth(), bbox.GetHeight() ) );
}


