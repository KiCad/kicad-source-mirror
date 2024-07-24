/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2024 KiCad Developers, see AUTHORS.txt for contributors.
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

#include "layer_pairs.h"

wxDEFINE_EVENT( PCB_LAYER_PAIR_PRESETS_CHANGED, wxCommandEvent );
wxDEFINE_EVENT( PCB_CURRENT_LAYER_PAIR_CHANGED, wxCommandEvent );


LAYER_PAIR_SETTINGS::LAYER_PAIR_SETTINGS( const LAYER_PAIR_SETTINGS& aOther )
{
    m_pairs = aOther.m_pairs;
    m_currentPair = aOther.m_currentPair;
}


bool LAYER_PAIR_SETTINGS::addLayerPairInternal( LAYER_PAIR_INFO aPairInfo )
{
    const LAYER_PAIR& newPair = aPairInfo.GetLayerPair();
    const auto        pairMatcher = [&]( const LAYER_PAIR_INFO& aExistingPair )
    {
        return newPair.HasSameLayers( aExistingPair.GetLayerPair() );
    };

    const bool alreadyExists = std::any_of( m_pairs.begin(), m_pairs.end(), pairMatcher );

    if( alreadyExists )
    {
        return false;
    }

    m_pairs.push_back( std::move( aPairInfo ) );
    return true;
}


bool LAYER_PAIR_SETTINGS::AddLayerPair( LAYER_PAIR_INFO aPair )
{
    bool ret = addLayerPairInternal( std::move( aPair ) );

    wxCommandEvent* evt = new wxCommandEvent( PCB_LAYER_PAIR_PRESETS_CHANGED, wxID_ANY );
    QueueEvent( evt );
    return ret;
}


bool LAYER_PAIR_SETTINGS::removeLayerPairInternal( const LAYER_PAIR& aPair )
{
    const auto pairMatcher = [&aPair]( const LAYER_PAIR_INFO& aPairInfo )
    {
        return aPairInfo.GetLayerPair().HasSameLayers( aPair );
    };

    const auto pairToRemoveIt = std::find_if( m_pairs.begin(), m_pairs.end(), pairMatcher );

    if( pairToRemoveIt == m_pairs.end() )
    {
        return false;
    }

    m_pairs.erase( pairToRemoveIt );
    return true;
}


bool LAYER_PAIR_SETTINGS::RemoveLayerPair( const LAYER_PAIR& aPair )
{
    bool ret = removeLayerPairInternal( aPair );

    wxCommandEvent* evt = new wxCommandEvent( PCB_LAYER_PAIR_PRESETS_CHANGED, wxID_ANY );
    QueueEvent( evt );
    return ret;
}


std::span<const LAYER_PAIR_INFO> LAYER_PAIR_SETTINGS::GetLayerPairs() const
{
    return m_pairs;
}

std::span<LAYER_PAIR_INFO> LAYER_PAIR_SETTINGS::GetLayerPairs()
{
    return m_pairs;
}

void LAYER_PAIR_SETTINGS::SetLayerPairs( std::span<const LAYER_PAIR_INFO> aPairs )
{
    // Replace all pairs with the given list
    m_pairs.clear();
    for( const LAYER_PAIR_INFO& pair : aPairs )
    {
        // Skip dupes and other
        addLayerPairInternal( pair );
    }

    {
        wxCommandEvent* evt = new wxCommandEvent( PCB_LAYER_PAIR_PRESETS_CHANGED, wxID_ANY );
        QueueEvent( evt );
    }
}

void LAYER_PAIR_SETTINGS::SetCurrentLayerPair( const LAYER_PAIR& aPair )
{
    m_currentPair = aPair;
    wxCommandEvent* evt = new wxCommandEvent( PCB_CURRENT_LAYER_PAIR_CHANGED, wxID_ANY );
    QueueEvent( evt );
}
