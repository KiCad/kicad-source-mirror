/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2007 Dick Hollenbeck, dick@softplc.com
 * Copyright (C) 2018-2020 KiCad Developers, see AUTHORS.txt for contributors.
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

#ifndef DRC_ITEM_H
#define DRC_ITEM_H

#include <rc_item.h>

class PCB_BASE_FRAME;

class DRC_ITEM : public RC_ITEM
{
public:
    /**
     * Constructs a DRC_ITEM for the given error code
     * @see DRCE_T
     */
    static DRC_ITEM* Create( int aErrorCode );

    /**
     * Constructs a DRC item from a given error settings key
     * @param aErrorKey is a settings key for an error code (the untranslated string that is used
     * to represent a given error code in settings files and for storing ignored DRC items)
     * @return the created item
     */
    static DRC_ITEM* Create( const wxString& aErrorKey );

    static std::vector<std::reference_wrapper<RC_ITEM>> GetItemsWithSeverities()
    {
        return allItemTypes;
    }

    /**
     * Translates this object into a fragment of HTML suitable for the wxHtmlListBox class.
     * @return wxString - the html text.
     */
    wxString ShowHtml( PCB_BASE_FRAME* aFrame ) const;

private:
    DRC_ITEM( int aErrorCode = 0, const wxString& aTitle = "", const wxString& aSettingsKey = "" )
    {
        m_errorCode   = aErrorCode;
        m_errorTitle  = aTitle;
        m_settingsKey = aSettingsKey;
    }

    /// A list of all DRC_ITEM types which are valid error codes
    static std::vector<std::reference_wrapper<RC_ITEM>> allItemTypes;

    static DRC_ITEM unconnectedItems;
    static DRC_ITEM itemsNotAllowed;
    static DRC_ITEM clearance;
    static DRC_ITEM tracksCrossing;
    static DRC_ITEM copperEdgeClearance;
    static DRC_ITEM zonesIntersect;
    static DRC_ITEM zoneHasEmptyNet;
    static DRC_ITEM viaDangling;
    static DRC_ITEM trackDangling;
    static DRC_ITEM holeNearHole;
    static DRC_ITEM trackWidth;
    static DRC_ITEM viaTooSmall;
    static DRC_ITEM viaAnnulus;
    static DRC_ITEM drillTooSmall;
    static DRC_ITEM viaHoleLargerThanPad;
    static DRC_ITEM padstack;
    static DRC_ITEM microviaTooSmall;
    static DRC_ITEM microviaDrillTooSmall;
    static DRC_ITEM keepout;
    static DRC_ITEM courtyardsOverlap;
    static DRC_ITEM missingCourtyard;
    static DRC_ITEM malformedCourtyard;
    static DRC_ITEM pthInsideCourtyard;
    static DRC_ITEM npthInsideCourtyard;
    static DRC_ITEM itemOnDisabledLayer;
    static DRC_ITEM invalidOutline;
    static DRC_ITEM duplicateFootprints;
    static DRC_ITEM missingFootprint;
    static DRC_ITEM extraFootprint;
    static DRC_ITEM unresolvedVariable;
};


#endif      // DRC_ITEM_H
