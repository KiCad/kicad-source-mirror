/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2011 Wayne Stambaugh <stambaughw@gmail.com>
 * Copyright (C) 2011-2021 KiCad Developers, see AUTHORS.txt for contributors.
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

#ifndef EE_COLLECTORS_H
#define EE_COLLECTORS_H

#include <lib_symbol.h>
#include <collector.h>
#include <dialogs/dialog_schematic_find.h>
#include <sch_item.h>


class SCH_SHEET_PATH;
class SCH_COMPONENT;


/**
 * EE_COLLECTOR
 */
class EE_COLLECTOR : public COLLECTOR
{
public:
    static const KICAD_T AllItems[];
    static const KICAD_T EditableItems[];
    static const KICAD_T MovableItems[];
    static const KICAD_T ComponentsOnly[];
    static const KICAD_T SheetsOnly[];
    static const KICAD_T WiresOnly[];
    static const KICAD_T FieldOwners[];

    EE_COLLECTOR( const KICAD_T* aScanTypes = EE_COLLECTOR::AllItems ) :
        m_Unit( 0 ),
        m_Convert( 0 )
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

        return NULL;
    }

    SEARCH_RESULT Inspect( EDA_ITEM* aItem, void* aTestData ) override;

    /**
     * Scan a #EDA_ITEM using this class's Inspector method which does the collection.
     *
     * @param aScreen The eeschema screen to use for scanning
     * @param aFilterList A list of #KICAD_T types with a terminating #EOT, that determines
     *                    what is to be collected and the priority order of the resulting
     *                    collection.
     * @param aPos are the coordinates to use in hit testing.
     * @param aUnit is the symbol unit filter (for symbol editor).
     * @param aConvert is the DeMorgan filter (for symbol editor)
     */
    void Collect( SCH_SCREEN* aScreen, const KICAD_T aFilterList[], const wxPoint& aPos,
                  int aUnit = 0, int aConvert = 0 );

    /**
     * Scan an #EDA_ITEM using this class's Inspector method which does the collection.
     *
     * @param aItems is a LIB_PART multivector holding the part items.
     * @param aFilterList is a list of #KICAD_T types with a terminating #EOT, that determines
     *                    what is to be collected and the priority order of the resulting
     *                    collection.
     * @param aPos are the coordinates to use in hit testing.
     * @param aUnit is the symbol unit filter (for symbol editor).
     * @param aConvert is the DeMorgan filter (for symbol editor).
     */
    void Collect( LIB_ITEMS_CONTAINER& aItems, const KICAD_T aFilterList[], const wxPoint& aPos,
                  int aUnit = 0, int aConvert = 0 );

    /**
     * Test if the collected items form a corner of two line segments.
     *
     * @return True if the collected items form a corner of two line segments.
     */
    bool IsCorner() const;

public:
    int      m_Unit;            // Fixed symbol unit filter (for symbol editor)
    int      m_Convert;         // Fixed DeMorgan filter (for symbol editor)
};


void CollectOtherUnits( const wxString& thisRef, int thisUnit, const LIB_ID& aLibId,
                        SCH_SHEET_PATH& aSheet, std::vector<SCH_COMPONENT*>* otherUnits );

#endif // EE_COLLECTORS_H
