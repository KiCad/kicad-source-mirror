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
#define SCALING_FACTOR  Millimeter2iu( 0.1625 )



PCB_MARKER::PCB_MARKER( std::shared_ptr<RC_ITEM> aItem, const VECTOR2I& aPosition,
                        PCB_LAYER_ID aLayer ) :
    BOARD_ITEM( nullptr, PCB_MARKER_T, aLayer ),  // parent set during BOARD::Add()
    MARKER_BASE( SCALING_FACTOR, aItem )
{
    if( m_rcItem )
    {
        m_rcItem->SetParent( this );

        switch( m_rcItem->GetErrorCode() )
        {
        case DRCE_UNCONNECTED_ITEMS:
            SetMarkerType( MARKER_BASE::MARKER_RATSNEST );
            break;

        case DRCE_MISSING_FOOTPRINT:
        case DRCE_DUPLICATE_FOOTPRINT:
        case DRCE_EXTRA_FOOTPRINT:
        case DRCE_NET_CONFLICT:
            SetMarkerType( MARKER_BASE::MARKER_PARITY );
            break;

        default:
            SetMarkerType( MARKER_BASE::MARKER_DRC );
            break;
        }
    }

    m_Pos = aPosition;
}


/* destructor */
PCB_MARKER::~PCB_MARKER()
{
    if( m_rcItem )
        m_rcItem->SetParent( nullptr );
}


wxString PCB_MARKER::Serialize() const
{
    wxString lastItem;

    if( m_rcItem->GetErrorCode() == DRCE_COPPER_SLIVER )
        lastItem = LayerName( m_layer );
    else
        lastItem = m_rcItem->GetAuxItemID().AsString();

    return wxString::Format( wxT( "%s|%d|%d|%s|%s" ),
                             m_rcItem->GetSettingsKey(),
                             m_Pos.x,
                             m_Pos.y,
                             m_rcItem->GetMainItemID().AsString(),
                             lastItem );
}


PCB_MARKER* PCB_MARKER::Deserialize( const wxString& data )
{
    wxArrayString props = wxSplit( data, '|' );
    PCB_LAYER_ID  markerLayer = F_Cu;
    VECTOR2I      markerPos( (int) strtol( props[1].c_str(), nullptr, 10 ),
                             (int) strtol( props[2].c_str(), nullptr, 10 ) );

    std::shared_ptr<DRC_ITEM> drcItem =  DRC_ITEM::Create( props[0] );

    if( !drcItem )
        return nullptr;

    if( drcItem->GetErrorCode() == DRCE_COPPER_SLIVER )
    {
        drcItem->SetItems( KIID( props[3] ) );

        for( int layer = 0; layer < PCB_LAYER_ID_COUNT; ++layer )
        {
            if( LayerName( ToLAYER_ID( layer ) ) == props[4] )
            {
                markerLayer = ToLAYER_ID( layer );
                break;
            }
        }
    }
    else
    {
        drcItem->SetItems( KIID( props[3] ), KIID( props[4] ) );
    }

    return new PCB_MARKER( drcItem, markerPos, markerLayer );
}


void PCB_MARKER::GetMsgPanelInfo( EDA_DRAW_FRAME* aFrame, std::vector<MSG_PANEL_ITEM>& aList )
{
    aList.emplace_back( _( "Type" ), _( "Marker" ) );
    aList.emplace_back( _( "Violation" ), m_rcItem->GetErrorMessage() );
    aList.emplace_back( _( "Severity" ), GetSeverity() == RPT_SEVERITY_ERROR ? _( "Error" )
                                                                             : _( "Warning" ) );

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


void PCB_MARKER::Rotate( const VECTOR2I& aRotCentre, double aAngle )
{
    // Marker geometry isn't user-editable
}


void PCB_MARKER::Flip( const VECTOR2I& aCentre, bool aFlipLeftRight )
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


SEVERITY PCB_MARKER::GetSeverity() const
{
    if( IsExcluded() )
        return RPT_SEVERITY_EXCLUSION;

    DRC_ITEM* item = static_cast<DRC_ITEM*>( m_rcItem.get() );
    DRC_RULE* rule = item->GetViolatingRule();

    if( rule && rule->m_Severity != RPT_SEVERITY_UNDEFINED )
        return rule->m_Severity;

    return GetBoard()->GetDesignSettings().GetSeverity( item->GetErrorCode() );
}


void PCB_MARKER::ViewGetLayers( int aLayers[], int& aCount ) const
{
    if( GetMarkerType() == MARKER_RATSNEST )
    {
        aCount = 0;
        return;
    }

    aCount = 2;

    aLayers[1] = LAYER_MARKER_SHADOWS;

    switch( GetSeverity() )
    {
    default:
    case SEVERITY::RPT_SEVERITY_ERROR:     aLayers[0] = LAYER_DRC_ERROR;     break;
    case SEVERITY::RPT_SEVERITY_WARNING:   aLayers[0] = LAYER_DRC_WARNING;   break;
    case SEVERITY::RPT_SEVERITY_EXCLUSION: aLayers[0] = LAYER_DRC_EXCLUSION; break;
    }
}


GAL_LAYER_ID PCB_MARKER::GetColorLayer() const
{
    switch( GetSeverity() )
    {
    default:
    case SEVERITY::RPT_SEVERITY_ERROR:     return LAYER_DRC_ERROR;
    case SEVERITY::RPT_SEVERITY_WARNING:   return LAYER_DRC_WARNING;
    case SEVERITY::RPT_SEVERITY_EXCLUSION: return LAYER_DRC_EXCLUSION;
    }
}


KIGFX::COLOR4D PCB_MARKER::getColor() const
{
    COLOR_SETTINGS* colors = Pgm().GetSettingsManager().GetColorSettings();
    return colors->GetColor( GetColorLayer() );
}


void PCB_MARKER::SetZoom( double aZoomFactor )
{
    SetMarkerScale( SCALING_FACTOR * aZoomFactor );
}


const EDA_RECT PCB_MARKER::GetBoundingBox() const
{
    EDA_RECT bbox = m_shapeBoundingBox;

    VECTOR2I pos = m_Pos;
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


