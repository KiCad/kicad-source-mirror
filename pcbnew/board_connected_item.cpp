/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2015 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2012 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
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

#include <board.h>
#include <board_connected_item.h>
#include <board_design_settings.h>
#include <drc/drc_engine.h>
#include <connectivity/connectivity_data.h>
#include <lset.h>
#include <properties/property_validators.h>
#include <string_utils.h>
#include <i18n_utility.h>
#include <netinfo.h>
#include <api/board/board_types.pb.h>
#include <shared_mutex>

using namespace std::placeholders;

BOARD_CONNECTED_ITEM::BOARD_CONNECTED_ITEM( BOARD_ITEM* aParent, KICAD_T idtype ) :
    BOARD_ITEM( aParent, idtype ),
    m_netinfo( NETINFO_LIST::OrphanedItem() )
{
    m_localRatsnestVisible = true;
}


void BOARD_CONNECTED_ITEM::SetLayer( PCB_LAYER_ID aLayer )
{
    BOARD_ITEM::SetLayer( aLayer );

    // Invalidate clearance cache since layer can affect clearance rules
    if( BOARD* board = GetBoard() )
        board->InvalidateClearanceCache( m_Uuid );
}


void BOARD_CONNECTED_ITEM::UnpackNet( const kiapi::board::types::Net& aProto )
{
    if( BOARD* board = GetBoard() )
    {
        wxString name = wxString::FromUTF8( aProto.name() );

        if( NETINFO_ITEM* net = board->FindNet( name ) )
        {
            m_netinfo = net;
        }
        else
        {
            NETINFO_ITEM* newnet = new NETINFO_ITEM( board, name, 0 );
            board->Add( newnet );
            m_netinfo = newnet;
        }
    }
}


void BOARD_CONNECTED_ITEM::PackNet( kiapi::board::types::Net* aProto ) const
{
    aProto->set_name( GetNetname().ToUTF8() );
}


bool BOARD_CONNECTED_ITEM::SetNetCode( int aNetCode, bool aNoAssert )
{
    if( !IsOnCopperLayer() )
        aNetCode = 0;

    // if aNetCode < 0 (typically NETINFO_LIST::FORCE_ORPHANED) or no parent board,
    // set the m_netinfo to the dummy NETINFO_LIST::ORPHANED

    BOARD* board = GetBoard();

    if( ( aNetCode >= 0 ) && board )
        m_netinfo = board->FindNet( aNetCode );
    else
        m_netinfo = NETINFO_LIST::OrphanedItem();

    if( !aNoAssert )
        wxASSERT( m_netinfo );

    // Invalidate clearance cache since net can affect clearance rules
    if( board )
    {
        board->InvalidateClearanceCache( m_Uuid );

        std::unique_lock<std::shared_mutex> writeLock( board->m_CachesMutex );
        board->m_ItemNetclassCache.erase( this );
    }

    return ( m_netinfo != nullptr );
}


int BOARD_CONNECTED_ITEM::GetOwnClearance( PCB_LAYER_ID aLayer, wxString* aSource ) const
{
    if( GetBoard() && GetBoard()->GetDesignSettings().m_DRCEngine )
    {
        BOARD_DESIGN_SETTINGS& bds = GetBoard()->GetDesignSettings();
        return bds.m_DRCEngine->GetCachedOwnClearance( this, aLayer, aSource );
    }

    return 0;
}


int BOARD_CONNECTED_ITEM::GetNetCode() const
{
    return m_netinfo ? m_netinfo->GetNetCode() : -1;
}


// Note: do NOT return a std::shared_ptr from this.  It is used heavily in DRC, and the
// std::shared_ptr stuff shows up large in performance profiling.
NETCLASS* BOARD_CONNECTED_ITEM::GetEffectiveNetClass() const
{
    // Static fallback netclass for items without a valid board (e.g., during DRC evaluation
    // of dummy items, or items not yet added to a board).
    static std::shared_ptr<NETCLASS> fallbackNetclass = std::make_shared<NETCLASS>( NETCLASS::Default );

    if( m_netinfo && m_netinfo->GetNetClass() )
        return m_netinfo->GetNetClass();

    if( const BOARD* board = GetBoard() )
    {
        if( board->GetDesignSettings().m_NetSettings )
            return board->GetDesignSettings().m_NetSettings->GetDefaultNetclass().get();
    }

    return fallbackNetclass.get();
}


wxString BOARD_CONNECTED_ITEM::GetNetClassName() const
{
    return GetEffectiveNetClass()->GetName();
}


wxString BOARD_CONNECTED_ITEM::GetNetname() const
{
    return m_netinfo ? m_netinfo->GetNetname() : wxString();
}


wxString BOARD_CONNECTED_ITEM::GetNetnameMsg() const
{
    if( !GetBoard() )
        return wxT( "[** NO BOARD DEFINED **]" );

    wxString netname = GetNetname();

    if( !netname.length() )
        return wxT( "[<no net>]" );
    else if( GetNetCode() < 0 )
        return wxT( "[" ) + UnescapeString( netname ) + wxT( "](" ) + _( "Not Found" ) + wxT( ")" );
    else
        return wxT( "[" ) + UnescapeString( netname ) + wxT( "]" );
}


const wxString& BOARD_CONNECTED_ITEM::GetShortNetname() const
{
    static wxString emptyString;

    return m_netinfo ? m_netinfo->GetShortNetname() : emptyString;
}


const wxString& BOARD_CONNECTED_ITEM::GetDisplayNetname() const
{
    static wxString emptyString;

    if( !m_netinfo )
        return emptyString;

    if( const BOARD* board = GetBoard() )
    {
        if( board->GetNetInfo().m_DisplayNetnamesDirty )
            board->GetNetInfo().RebuildDisplayNetnames();
    }

    return m_netinfo->GetDisplayNetname();
}


static struct BOARD_CONNECTED_ITEM_DESC
{
    BOARD_CONNECTED_ITEM_DESC()
    {
        ENUM_MAP<PCB_LAYER_ID>& layerEnum = ENUM_MAP<PCB_LAYER_ID>::Instance();

        if( layerEnum.Choices().GetCount() == 0 )
        {
            layerEnum.Undefined( UNDEFINED_LAYER );

            for( PCB_LAYER_ID layer : LSET::AllLayersMask() )
                layerEnum.Map( layer, LSET::Name( layer ) );
        }

        PROPERTY_MANAGER& propMgr = PROPERTY_MANAGER::Instance();
        REGISTER_TYPE( BOARD_CONNECTED_ITEM );
        propMgr.InheritsAfter( TYPE_HASH( BOARD_CONNECTED_ITEM ), TYPE_HASH( BOARD_ITEM ) );

        // Replace layer property as the properties panel will set a restriction for copper layers
        // only for BOARD_CONNECTED_ITEM that we don't want to apply to BOARD_ITEM
        auto layer = new PROPERTY_ENUM<BOARD_CONNECTED_ITEM, PCB_LAYER_ID>(
                _HKI( "Layer" ),
                &BOARD_CONNECTED_ITEM::SetLayer, &BOARD_CONNECTED_ITEM::GetLayer );
        layer->SetChoices( layerEnum.Choices() );
        propMgr.ReplaceProperty( TYPE_HASH( BOARD_ITEM ), _HKI( "Layer" ), layer );

        propMgr.AddProperty( new PROPERTY_ENUM<BOARD_CONNECTED_ITEM, int>( _HKI( "Net" ),
                             &BOARD_CONNECTED_ITEM::SetNetCode,
                             &BOARD_CONNECTED_ITEM::GetNetCode, PT_NET ) )
                .SetIsHiddenFromRulesEditor()
                .SetIsHiddenFromLibraryEditors();

        /**
         * This property should just be an alias for the one below, it only exists so that we
         * maintain compatibility with both `NetClass` and `Net_Class` in custom rules.
         * It has the name we would show in the GUI if we wanted to show this in the GUI, but we
         * don't at the moment because there is no way to edit the netclass of a net from a selected
         * connected item, and showing it makes users think they can change it.
         */
        propMgr.AddProperty( new PROPERTY<BOARD_CONNECTED_ITEM, wxString>( _HKI( "Net Class" ),
                             NO_SETTER( BOARD_CONNECTED_ITEM, wxString ),
                             &BOARD_CONNECTED_ITEM::GetNetClassName ) )
                .SetIsHiddenFromRulesEditor()
                .SetIsHiddenFromPropertiesManager()
                .SetIsHiddenFromLibraryEditors();

        // Compatibility alias for DRC engine
        propMgr.AddProperty( new PROPERTY<BOARD_CONNECTED_ITEM, wxString>( _HKI( "NetClass" ),
                             NO_SETTER( BOARD_CONNECTED_ITEM, wxString ),
                             &BOARD_CONNECTED_ITEM::GetNetClassName ) )
                .SetIsHiddenFromPropertiesManager()
                .SetIsHiddenFromLibraryEditors();

        // Used only in DRC engine
        propMgr.AddProperty( new PROPERTY<BOARD_CONNECTED_ITEM, wxString>( _HKI( "NetName" ),
                             NO_SETTER( BOARD_CONNECTED_ITEM, wxString ),
                             &BOARD_CONNECTED_ITEM::GetNetname ) )
                .SetIsHiddenFromPropertiesManager()
                .SetIsHiddenFromLibraryEditors();

        auto supportsTeardrops =
                []( INSPECTABLE* aItem ) -> bool
                {
                    if( BOARD_CONNECTED_ITEM* bci = dynamic_cast<BOARD_CONNECTED_ITEM*>( aItem ) )
                    {
                        if( !bci->GetBoard() || bci->GetBoard()->LegacyTeardrops() )
                            return false;

                        return bci->Type() == PCB_PAD_T || bci->Type() == PCB_VIA_T;
                    }

                    return false;
                };

        auto supportsTeardropPreferZoneSetting =
                []( INSPECTABLE* aItem ) -> bool
                {
                    if( BOARD_CONNECTED_ITEM* bci = dynamic_cast<BOARD_CONNECTED_ITEM*>( aItem ) )
                    {
                        if( !bci->GetBoard() || bci->GetBoard()->LegacyTeardrops() )
                            return false;

                        return bci->Type() == PCB_PAD_T;
                    }

                    return false;
                };

        const wxString groupTeardrops = _HKI( "Teardrops" );

        auto enableTeardrops = new PROPERTY<BOARD_CONNECTED_ITEM, bool>( _HKI( "Enable Teardrops" ),
                         &BOARD_CONNECTED_ITEM::SetTeardropsEnabled,
                         &BOARD_CONNECTED_ITEM::GetTeardropsEnabled );
        enableTeardrops->SetAvailableFunc( supportsTeardrops );
        propMgr.AddProperty( enableTeardrops, groupTeardrops );

        auto bestLength = new PROPERTY<BOARD_CONNECTED_ITEM, double>( _HKI( "Best Length Ratio" ),
                         &BOARD_CONNECTED_ITEM::SetTeardropBestLengthRatio,
                         &BOARD_CONNECTED_ITEM::GetTeardropBestLengthRatio );
        bestLength->SetAvailableFunc( supportsTeardrops );
        bestLength->SetValidator( PROPERTY_VALIDATORS::PositiveRatioValidator );
        propMgr.AddProperty( bestLength, groupTeardrops );

        auto maxLength = new PROPERTY<BOARD_CONNECTED_ITEM, int>( _HKI( "Max Length" ),
                         &BOARD_CONNECTED_ITEM::SetTeardropMaxLength,
                         &BOARD_CONNECTED_ITEM::GetTeardropMaxLength, PROPERTY_DISPLAY::PT_SIZE );
        maxLength->SetAvailableFunc( supportsTeardrops );
        propMgr.AddProperty( maxLength, groupTeardrops );

        auto bestWidth = new PROPERTY<BOARD_CONNECTED_ITEM, double>( _HKI( "Best Width Ratio" ),
                         &BOARD_CONNECTED_ITEM::SetTeardropBestWidthRatio,
                         &BOARD_CONNECTED_ITEM::GetTeardropBestWidthRatio );
        bestWidth->SetAvailableFunc( supportsTeardrops );
        bestWidth->SetValidator( PROPERTY_VALIDATORS::PositiveRatioValidator );
        propMgr.AddProperty( bestWidth, groupTeardrops );

        auto maxWidth = new PROPERTY<BOARD_CONNECTED_ITEM, int>( _HKI( "Max Width" ),
                         &BOARD_CONNECTED_ITEM::SetTeardropMaxWidth,
                         &BOARD_CONNECTED_ITEM::GetTeardropMaxWidth, PROPERTY_DISPLAY::PT_SIZE );
        maxWidth->SetAvailableFunc( supportsTeardrops );
        propMgr.AddProperty( maxWidth, groupTeardrops );

        auto curvePts = new PROPERTY<BOARD_CONNECTED_ITEM, bool>( _HKI( "Curved Teardrops" ),
                         &BOARD_CONNECTED_ITEM::SetTeardropCurved,
                         &BOARD_CONNECTED_ITEM::GetTeardropCurved );
        curvePts->SetAvailableFunc( supportsTeardrops );
        propMgr.AddProperty( curvePts, groupTeardrops );

        auto preferZones = new PROPERTY<BOARD_CONNECTED_ITEM, bool>( _HKI( "Prefer Zone Connections" ),
                         &BOARD_CONNECTED_ITEM::SetTeardropPreferZoneConnections,
                         &BOARD_CONNECTED_ITEM::GetTeardropPreferZoneConnections );
        preferZones->SetAvailableFunc( supportsTeardropPreferZoneSetting );
        propMgr.AddProperty( preferZones, groupTeardrops );

        auto twoTracks = new PROPERTY<BOARD_CONNECTED_ITEM, bool>( _HKI( "Allow Teardrops To Span Two Tracks" ),
                         &BOARD_CONNECTED_ITEM::SetTeardropAllowSpanTwoTracks,
                         &BOARD_CONNECTED_ITEM::GetTeardropAllowSpanTwoTracks );
        twoTracks->SetAvailableFunc( supportsTeardrops );
        propMgr.AddProperty( twoTracks, groupTeardrops );

        auto maxTrackWidth = new PROPERTY<BOARD_CONNECTED_ITEM, double>( _HKI( "Max Width Ratio" ),
                         &BOARD_CONNECTED_ITEM::SetTeardropMaxTrackWidth,
                         &BOARD_CONNECTED_ITEM::GetTeardropMaxTrackWidth );
        maxTrackWidth->SetAvailableFunc( supportsTeardrops );
        propMgr.AddProperty( maxTrackWidth, groupTeardrops );
    }
} _BOARD_CONNECTED_ITEM_DESC;
