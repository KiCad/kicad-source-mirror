/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2018 KiCad Developers, see AUTHORS.txt for contributors.
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


#ifndef DRC_DRC_MARKER_FACTORY__H
#define DRC_DRC_MARKER_FACTORY__H


#include <class_marker_pcb.h>
#include <common.h> // for EDA_UNITS_T


class ZONE_CONTAINER;
class TRACK;
class D_PAD;
class SEG;

/**
 * Class that constructs DRC markers of various kinds
 * with messages according to items and error code
 */
class DRC_MARKER_FACTORY
{
public:
    using UNITS_PROVIDER = std::function<EDA_UNITS_T()>;

    DRC_MARKER_FACTORY();

    /**
     * Set the provider for the units (this will be called for each new
     * marker constructed)
     * @param aUnitsProvider a callable that returns the current units to use
     */
    void SetUnitsProvider( UNITS_PROVIDER aUnitsProvider );

    /**
     * Set the units provider to a function returning a constant value.
     * This is a shorthand for #SetUnitProvider.
     */
    void SetUnits( EDA_UNITS_T aUnits );

    /**
     * Creates a marker on a track, via or pad.
     *
     * @param aTrack/aPad The reference item.
     * @param aConflitItem  Another item on the board which is in conflict with the
     *                       reference item.
     * @param aErrorCode An ID for the particular type of error that is being reported.
     */
    MARKER_PCB* NewMarker( TRACK* aTrack, BOARD_ITEM* aConflitItem, const SEG& aConflictSeg,
                           int aErrorCode ) const;

    MARKER_PCB* NewMarker( TRACK* aTrack, ZONE_CONTAINER* aConflictZone, int aErrorCode ) const;

    MARKER_PCB* NewMarker( D_PAD* aPad, BOARD_ITEM* aConflictItem, int aErrorCode ) const;

    /**
     * Creates a marker at a given location.
     *
     * @param aItem The reference item.
     * @param aPos Usually the position of the item, but could be more specific for a zone.
     * @param aErrorCode An ID for the particular type of error that is being reported.
     */
    MARKER_PCB* NewMarker( const wxPoint& aPos, BOARD_ITEM* aItem, int aErrorCode ) const;

    MARKER_PCB* NewMarker( const wxPoint& aPos, BOARD_ITEM* aItem, BOARD_ITEM* bItem,
                           int aErrorCode ) const;

    /**
     * Create a MARKER which will report on a generic problem with the board which is
     * not geographically locatable.
     */
    MARKER_PCB* NewMarker( int aErrorCode, const wxString& aMessage ) const;

private:
    EDA_UNITS_T getCurrentUnits() const
    {
        return m_units_provider();
    }

    UNITS_PROVIDER m_units_provider;
};

#endif // DRC_DRC_MARKER_FACTORY__H