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

#include <fctsys.h>
#include <gr_basic.h>
#include <trigo.h>
#include <msgpanel.h>
#include <bitmaps.h>
#include <base_units.h>
#include <pcbnew.h>
#include <class_board.h>
#include <class_board_item.h>
#include <class_marker_pcb.h>
#include <board_design_settings.h>
#include <layers_id_colors_and_visibility.h>
#include <settings/color_settings.h>
#include <settings/settings_manager.h>
#include <widgets/ui_common.h>
#include <pgm_base.h>


/// Factor to convert the maker unit shape to internal units:
#define SCALING_FACTOR  Millimeter2iu( 0.1 )

MARKER_PCB::MARKER_PCB( BOARD_ITEM* aParent ) :
        BOARD_ITEM( aParent, PCB_MARKER_T ),
        MARKER_BASE( SCALING_FACTOR ), m_item( nullptr )
{
}


MARKER_PCB::MARKER_PCB( EDA_UNITS aUnits, int aErrorCode, const wxPoint& aMarkerPos,
                        BOARD_ITEM* aItem,
                        BOARD_ITEM* bItem ) :
        BOARD_ITEM( nullptr, PCB_MARKER_T ), // parent set during BOARD::Add()
        MARKER_BASE( aUnits, aErrorCode, aMarkerPos, aItem, bItem, SCALING_FACTOR ),
        m_item( nullptr )
{
}


MARKER_PCB::MARKER_PCB( EDA_UNITS aUnits, int aErrorCode, const wxPoint& aMarkerPos,
                        BOARD_ITEM* aItem, const wxPoint& aPos,
                        BOARD_ITEM* bItem, const wxPoint& bPos ) :
        BOARD_ITEM( nullptr, PCB_MARKER_T ), // parent set during BOARD::Add()
        MARKER_BASE( aUnits, aErrorCode, aMarkerPos, aItem, aPos, bItem, bPos, SCALING_FACTOR ),
        m_item( nullptr )
{
}


MARKER_PCB::MARKER_PCB( int aErrorCode, const wxPoint& aMarkerPos,
                        const wxString& aText, const wxPoint& aPos,
                        const wxString& bText, const wxPoint& bPos ) :
        BOARD_ITEM( nullptr, PCB_MARKER_T ),  // parent set during BOARD::Add()
        MARKER_BASE( aErrorCode, aMarkerPos, aText, aPos, bText, bPos, SCALING_FACTOR ), m_item( nullptr )
{
}


MARKER_PCB::MARKER_PCB( int aErrorCode,
                        const wxString& aText,
                        const wxString& bText) :
        BOARD_ITEM( nullptr, PCB_MARKER_T ),  // parent set during BOARD::Add()
        MARKER_BASE( aErrorCode, aText, bText, SCALING_FACTOR ), m_item( nullptr )
{
}


/* destructor */
MARKER_PCB::~MARKER_PCB()
{
}


wxString MARKER_PCB::Serialize() const
{
    return wxString::Format( wxT( "%d|%d|%d|%s|%s|%s|%s" ),
                             m_drc.GetErrorCode(),
                             m_Pos.x,
                             m_Pos.y,
                             m_drc.GetMainText(),
                             m_drc.GetMainItemID().AsString(),
                             m_drc.GetAuxiliaryText(),
                             m_drc.GetAuxItemID().AsString() );
}


MARKER_PCB* MARKER_PCB::Deserialize( const wxString& data )
{
    wxArrayString props = wxSplit( data, '|' );
    int           errorCode = (int) strtol( props[0].c_str(), nullptr, 10 );
    MARKER_PCB*   marker = new MARKER_PCB( nullptr );   // parent set during BOARD::Add()

    marker->m_Pos.x = (int) strtol( props[1].c_str(), nullptr, 10 );
    marker->m_Pos.y = (int) strtol( props[2].c_str(), nullptr, 10 );
    marker->m_drc.SetData( errorCode, props[3], KIID( props[4] ), props[5], KIID( props[6] ) );
    marker->m_drc.SetParent( marker );
    return marker;
}


/* tests to see if this object is on the given layer.
 * DRC markers are not really on a copper layer, but
 * MARKER_PCB::IsOnCopperLayer return true if aLayer is a cooper layer,
 * because this test is often used to locad a marker
 * param aLayer The layer to test for.
 * return bool - true if on given layer, else false.
 */
bool MARKER_PCB::IsOnLayer( PCB_LAYER_ID aLayer ) const
{
    return IsCopperLayer( aLayer );
}


void MARKER_PCB::GetMsgPanelInfo( EDA_UNITS aUnits, std::vector<MSG_PANEL_ITEM>& aList )
{
    aList.emplace_back( MSG_PANEL_ITEM( _( "Type" ), _( "Marker" ), DARKCYAN ) );

    aList.emplace_back( MSG_PANEL_ITEM( _( "Violation" ), m_drc.GetErrorText(), RED ) );

    aList.emplace_back( MSG_PANEL_ITEM( m_drc.GetTextA(), m_drc.GetTextB(), DARKBROWN ) );
}


void MARKER_PCB::Rotate(const wxPoint& aRotCentre, double aAngle)
{
    RotatePoint( &m_Pos, aRotCentre, aAngle );
}


void MARKER_PCB::Flip(const wxPoint& aCentre, bool aFlipLeftRight )
{
    if( aFlipLeftRight )
        m_Pos.x = aCentre.x - ( m_Pos.x - aCentre.x );
    else
        m_Pos.y = aCentre.y - ( m_Pos.y - aCentre.y );
}


wxString MARKER_PCB::GetSelectMenuText( EDA_UNITS aUnits ) const
{
    return wxString::Format( _( "Marker (%s)" ), GetReporter().GetErrorText() );
}


BITMAP_DEF MARKER_PCB::GetMenuImage() const
{
    return drc_xpm;
}


void MARKER_PCB::ViewGetLayers( int aLayers[], int& aCount ) const
{
    aCount = 1;

    BOARD_ITEM_CONTAINER* ancestor = GetParent();

    while( ancestor->GetParent() )
        ancestor = ancestor->GetParent();

    BOARD* board = static_cast<BOARD*>( ancestor );

    switch( board->GetDesignSettings().GetSeverity( m_drc.GetErrorCode() ) )
    {
    default:
    case SEVERITY::RPT_SEVERITY_ERROR:   aLayers[0] = LAYER_DRC_ERROR;   break;
    case SEVERITY::RPT_SEVERITY_WARNING: aLayers[0] = LAYER_DRC_WARNING; break;
    }
}


GAL_LAYER_ID MARKER_PCB::GetColorLayer() const
{
    if( IsExcluded() )
        return LAYER_AUX_ITEMS;

    BOARD_ITEM_CONTAINER* ancestor = GetParent();

    while( ancestor->GetParent() )
        ancestor = ancestor->GetParent();

    BOARD* board = static_cast<BOARD*>( ancestor );

    switch( board->GetDesignSettings().GetSeverity( m_drc.GetErrorCode() ) )
    {
    default:
    case SEVERITY::RPT_SEVERITY_ERROR:   return LAYER_DRC_ERROR;
    case SEVERITY::RPT_SEVERITY_WARNING: return LAYER_DRC_WARNING;
    }
}


KIGFX::COLOR4D MARKER_PCB::getColor() const
{
    COLOR_SETTINGS* colors = Pgm().GetSettingsManager().GetColorSettings();
    return colors->GetColor( GetColorLayer() );
}


const EDA_RECT MARKER_PCB::GetBoundingBox() const
{
    EDA_RECT bbox = m_ShapeBoundingBox;

    wxPoint pos = m_Pos;
    pos.x += int( bbox.GetOrigin().x * MarkerScale() );
    pos.y += int( bbox.GetOrigin().y * MarkerScale() );

    return EDA_RECT( pos, wxSize( int( bbox.GetWidth() * MarkerScale() ),
                                  int( bbox.GetHeight() * MarkerScale() ) ) );
}


const BOX2I MARKER_PCB::ViewBBox() const
{
    EDA_RECT bbox = GetBoundingBox();
    return BOX2I( bbox.GetOrigin(), VECTOR2I( bbox.GetWidth(), bbox.GetHeight() ) );
}
