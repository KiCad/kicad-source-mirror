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
#include <pcb_base_frame.h>
#include <class_board.h>
#include <class_board_item.h>
#include <class_marker_pcb.h>
#include <layers_id_colors_and_visibility.h>
#include <settings/color_settings.h>
#include <settings/settings_manager.h>
#include <widgets/ui_common.h>
#include <pgm_base.h>
#include <drc/drc_item.h>


/// Factor to convert the maker unit shape to internal units:
#define SCALING_FACTOR  Millimeter2iu( 0.1 )


MARKER_PCB::MARKER_PCB( DRC_ITEM* aItem, const wxPoint& aPosition ) :
        BOARD_ITEM( nullptr, PCB_MARKER_T ),  // parent set during BOARD::Add()
        MARKER_BASE( SCALING_FACTOR, aItem )
{
    if( m_rcItem )
        m_rcItem->SetParent( this );

    m_Pos = aPosition;
}


/* destructor */
MARKER_PCB::~MARKER_PCB()
{
}


wxString MARKER_PCB::Serialize() const
{
    return wxString::Format( wxT( "%s|%d|%d|%s|%s" ),
                             m_rcItem->GetErrorText( m_rcItem->GetErrorCode(), false ),
                             m_Pos.x,
                             m_Pos.y,
                             m_rcItem->GetMainItemID().AsString(),
                             m_rcItem->GetAuxItemID().AsString() );
}


MARKER_PCB* MARKER_PCB::Deserialize( const wxString& data )
{
    wxArrayString props = wxSplit( data, '|' );
    wxPoint       markerPos( (int) strtol( props[1].c_str(), nullptr, 10 ),
                             (int) strtol( props[2].c_str(), nullptr, 10 ) );

    DRC_ITEM* drcItem = new DRC_ITEM( props[0] );
    drcItem->SetItems( KIID( props[3] ), KIID( props[4] ) );

    return new MARKER_PCB( drcItem, markerPos );
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


void MARKER_PCB::GetMsgPanelInfo( EDA_DRAW_FRAME* aFrame, std::vector<MSG_PANEL_ITEM>& aList )
{
    aList.emplace_back( _( "Type" ), _( "Marker" ), DARKCYAN );
    aList.emplace_back( _( "Violation" ), m_rcItem->GetErrorMessage(), RED );

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

    aList.emplace_back( mainText, auxText, DARKBROWN );
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
    // m_rcItem->GetErrorMessage() could be used instead, but is probably too long
    // for menu duty.
    return wxString::Format( _( "Marker (%s)" ),
                             m_rcItem->GetErrorText( m_rcItem->GetErrorCode() ) );
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

    switch( board->GetDesignSettings().GetSeverity( m_rcItem->GetErrorCode() ) )
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

    switch( board->GetDesignSettings().GetSeverity( m_rcItem->GetErrorCode() ) )
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
    EDA_RECT bbox = m_shapeBoundingBox;

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


