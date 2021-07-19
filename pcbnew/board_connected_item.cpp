/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2015 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2012 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright (C) 1992-2021 KiCad Developers, see AUTHORS.txt for contributors.
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
#include <connectivity/connectivity_data.h>
#include <drc/drc_engine.h>
#include <kicad_string.h>
#include <i18n_utility.h>
#include <netinfo.h>

using namespace std::placeholders;

BOARD_CONNECTED_ITEM::BOARD_CONNECTED_ITEM( BOARD_ITEM* aParent, KICAD_T idtype ) :
    BOARD_ITEM( aParent, idtype ),
    m_netinfo( NETINFO_LIST::OrphanedItem() )
{
    m_localRatsnestVisible = true;
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

    return ( m_netinfo != nullptr );
}


NETCLASS* BOARD_CONNECTED_ITEM::GetEffectiveNetclass() const
{
    // NB: we must check the net first, as when it is 0 GetNetClass() will return the
    // orphaned net netclass, not the default netclass.
    if( !m_netinfo || m_netinfo->GetNetCode() == 0 )
        return GetBoard()->GetDesignSettings().GetDefault();
    else
        return GetNetClass();
}


int BOARD_CONNECTED_ITEM::GetOwnClearance( PCB_LAYER_ID aLayer, wxString* aSource ) const
{
    DRC_CONSTRAINT constraint;

    if( GetBoard() && GetBoard()->GetDesignSettings().m_DRCEngine )
    {
        BOARD_DESIGN_SETTINGS& bds = GetBoard()->GetDesignSettings();

        constraint = bds.m_DRCEngine->EvalRules( CLEARANCE_CONSTRAINT, this, nullptr, aLayer );
    }

    if( constraint.Value().HasMin() )
    {
        if( aSource )
            *aSource = constraint.GetName();

        return constraint.Value().Min();
    }

    return 0;
}


int BOARD_CONNECTED_ITEM::GetNetCode() const
{
    return m_netinfo ? m_netinfo->GetNetCode() : -1;
}


// Note: do NOT return a std::shared_ptr from this.  It is used heavily in DRC, and the
// std::shared_ptr stuff shows up large in performance profiling.
NETCLASS* BOARD_CONNECTED_ITEM::GetNetClass() const
{
    if( m_netinfo && m_netinfo->GetNetClass() )
        return m_netinfo->GetNetClass();
    else
        return GetBoard()->GetDesignSettings().GetDefault();
}


wxString BOARD_CONNECTED_ITEM::GetNetClassName() const
{
    if( m_netinfo )
        return m_netinfo->GetNetClassName();
    else
        return wxEmptyString;
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
        return wxT( "[" + UnescapeString( netname ) + "](" + _( "Not Found" ) + ")" );
    else
        return wxT( "[" + UnescapeString( netname ) + "]" );
}


wxString BOARD_CONNECTED_ITEM::GetShortNetname() const
{
    return m_netinfo->GetShortNetname();
}


static struct BOARD_CONNECTED_ITEM_DESC
{
    BOARD_CONNECTED_ITEM_DESC()
    {
        ENUM_MAP<PCB_LAYER_ID>& layerEnum = ENUM_MAP<PCB_LAYER_ID>::Instance();

        if( layerEnum.Choices().GetCount() == 0 )
        {
            layerEnum.Undefined( UNDEFINED_LAYER );

            for( LSEQ seq = LSET::AllLayersMask().Seq(); seq; ++seq )
                layerEnum.Map( *seq, LSET::Name( *seq ) );
        }

        PROPERTY_MANAGER& propMgr = PROPERTY_MANAGER::Instance();
        REGISTER_TYPE( BOARD_CONNECTED_ITEM );
        propMgr.InheritsAfter( TYPE_HASH( BOARD_CONNECTED_ITEM ), TYPE_HASH( BOARD_ITEM ) );

        propMgr.ReplaceProperty( TYPE_HASH( BOARD_ITEM ), _HKI( "Layer" ),
                new PROPERTY_ENUM<BOARD_CONNECTED_ITEM, PCB_LAYER_ID, BOARD_ITEM>( _HKI( "Layer" ),
                    &BOARD_CONNECTED_ITEM::SetLayer, &BOARD_CONNECTED_ITEM::GetLayer ) );

        propMgr.AddProperty( new PROPERTY_ENUM<BOARD_CONNECTED_ITEM, int>( _HKI( "Net" ),
                    &BOARD_CONNECTED_ITEM::SetNetCode, &BOARD_CONNECTED_ITEM::GetNetCode ) );
        propMgr.AddProperty( new PROPERTY<BOARD_CONNECTED_ITEM, wxString>( _HKI( "NetName" ),
                    NO_SETTER( BOARD_CONNECTED_ITEM, wxString ), &BOARD_CONNECTED_ITEM::GetNetname ) );
        propMgr.AddProperty( new PROPERTY<BOARD_CONNECTED_ITEM, wxString>( _HKI( "NetClass" ),
                    NO_SETTER( BOARD_CONNECTED_ITEM, wxString ), &BOARD_CONNECTED_ITEM::GetNetClassName ) );
    }
} _BOARD_CONNECTED_ITEM_DESC;
