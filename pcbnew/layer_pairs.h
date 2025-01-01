/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
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

#ifndef LAYER_PAIRS_H
#define LAYER_PAIRS_H

#include <memory>
#include <span>
#include <string>
#include <vector>
#include <optional>

#include <wx/event.h>

#include <project/board_project_settings.h>


class wxBitmap;

wxDECLARE_EVENT( PCB_LAYER_PAIR_PRESETS_CHANGED, wxCommandEvent );
wxDECLARE_EVENT( PCB_CURRENT_LAYER_PAIR_CHANGED, wxCommandEvent );

/**
 * Management class for layer pairs in a PCB
 */
class LAYER_PAIR_SETTINGS: public wxEvtHandler
{
public:
    /**
     * Construct a new layer pair manager.
     */
    LAYER_PAIR_SETTINGS() :
            m_currentPair( F_Cu, B_Cu )
    {
    }

    LAYER_PAIR_SETTINGS( const LAYER_PAIR_SETTINGS& aOther );

    bool AddLayerPair( LAYER_PAIR_INFO aPair );

    /**
     * Remove the matching layer pair from the store, if present.
     */
    bool RemoveLayerPair( const LAYER_PAIR& aPair );

    /**
     * Returns a span of all stored layer pairs.
     */
    std::span<const LAYER_PAIR_INFO> GetLayerPairs() const;
    std::span<LAYER_PAIR_INFO> GetLayerPairs();

    /**
     * Get a vector of all enabled layer pairs, in order.
     *
     * This includes a "manual" pair, if one is set
     * and isn't in the list of presets.
     */
    std::vector<LAYER_PAIR_INFO> GetEnabledLayerPairs( int& aCurrentIndex ) const;

    /**
     * Replace the stored layer pairs with the given list.
     *
     * The same conditions are maintained as for AddLayerPair
     */
    void SetLayerPairs( std::span<const LAYER_PAIR_INFO> aPairs );

    const LAYER_PAIR& GetCurrentLayerPair() const { return m_currentPair; }

    /**
     * Set the "active" layer pair. This doesn't have to be a preset pair.
     */
    void SetCurrentLayerPair( const LAYER_PAIR& aPair );

private:
    bool addLayerPairInternal( LAYER_PAIR_INFO aPair );
    bool removeLayerPairInternal( const LAYER_PAIR& aPair );

    // Ordered store of all preset layer pairs
    std::vector<LAYER_PAIR_INFO> m_pairs;

    // Keep track of the last manual pair (set, but not a preset)
    // for quick switching back
    std::optional<LAYER_PAIR> m_lastManualPair;

    LAYER_PAIR m_currentPair;
};

#endif // LAYER_PAIRS_H