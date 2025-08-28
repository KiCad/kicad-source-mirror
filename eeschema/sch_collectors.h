/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2011 Wayne Stambaugh <stambaughw@gmail.com>
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 * Copyright (C) 2019 CERN
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

#pragma once


#include <lib_symbol.h>
#include <collector.h>
#include <sch_item.h>


class SCH_SHEET_PATH;
class SCH_SYMBOL;


class SCH_COLLECTOR : public COLLECTOR
{
public:
    static const std::vector<KICAD_T> EditableItems;
    static const std::vector<KICAD_T> MovableItems;
    static const std::vector<KICAD_T> FieldOwners;
    static const std::vector<KICAD_T> DeletableItems;

    SCH_COLLECTOR( const std::vector<KICAD_T>& aScanTypes = { SCH_LOCATE_ANY_T } ) :
            m_Unit( 0 ),
            m_BodyStyle( 0 ),
            m_ShowPinElectricalTypes( false )
    {
        SetScanTypes( aScanTypes );
    }

    /**
     * Overload COLLECTOR::operator[](int) to return a SCH_ITEM instead of an EDA_ITEM.
     *
     * @param aIndex The index into the list.
     * @return SCH_ITEM* at \a aIndex or NULL.
     */
    SCH_ITEM* operator[]( int aIndex ) const override
    {
        if( (unsigned)aIndex < (unsigned)GetCount() )
            return (SCH_ITEM*) m_list[ aIndex ];

        return nullptr;
    }

    INSPECT_RESULT Inspect( EDA_ITEM* aItem, void* aTestData ) override;

    /**
     * Scan a #EDA_ITEM using this class's Inspector method which does the collection.
     *
     * @param aScreen The eeschema screen to use for scanning
     * @param aScanTypes A list of #KICAD_T types that determines what is to be collected and
     *                   the priority order of the resulting collection.
     * @param aPos are the coordinates to use in hit testing.
     * @param aUnit is the symbol unit filter (for symbol editor).
     * @param aBodyStyle is the body style filter (for symbol editor)
     */
    void Collect( SCH_SCREEN* aScreen, const std::vector<KICAD_T>& aScanTypes,
                  const VECTOR2I& aPos, int aUnit = 0, int aBodyStyle = 0 );

    /**
     * Scan an #EDA_ITEM using this class's Inspector method which does the collection.
     *
     * @param aItems is a LIB_SYMBOL multivector holding the symbol items.
     * @param aScanTypes is a list of #KICAD_T types that determines what is to be collected
     *                   and the priority order of the resulting collection.
     * @param aPos are the coordinates to use in hit testing.
     * @param aUnit is the symbol unit filter (for symbol editor).
     * @param aBodyStyle is the body style filter (for symbol editor).
     */
    void Collect( LIB_ITEMS_CONTAINER& aItems, const std::vector<KICAD_T>& aScanTypes,
                  const VECTOR2I& aPos, int aUnit = 0, int aBodyStyle = 0 );

    /**
     * Test if the collected items form a corner of two line segments.
     *
     * @return True if the collected items form a corner of two line segments.
     */
    bool IsCorner() const;

public:
    int      m_Unit;            // Fixed symbol unit filter (for symbol editor)
    int      m_BodyStyle;       // Fixed body style filter (for symbol editor)

    bool     m_ShowPinElectricalTypes;
};


void CollectOtherUnits( const wxString& thisRef, int thisUnit, const LIB_ID& aLibId,
                        SCH_SHEET_PATH& aSheet, std::vector<SCH_SYMBOL*>* otherUnits );
