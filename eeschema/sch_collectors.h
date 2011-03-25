/*
 * This program source code file is part of KICAD, a free EDA CAD application.
 *
 * Copyright (C) 2011 Wayne Stambaugh <stambaughw@verizon.net>
 * Copyright (C) 2004-20011 Kicad Developers, see change_log.txt for contributors.
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

#ifndef _SCH_COLLECTORS_H_
#define _SCH_COLLECTORS_H_


#include "class_collector.h"
#include "sch_item_struct.h"


/**
 * Class SCH_COLLECTOR
 */
class SCH_COLLECTOR : public COLLECTOR
{
public:

    /**
     * A scan list for all schematic items.
     */
    static const KICAD_T AllItems[];

    /**
     * A scan list for all editable schematic items.
     */
    static const KICAD_T EditableItems[];

    /**
     * A scan list for all movable schematic items.
     */
    static const KICAD_T MovableItems[];

    /**
     * A scan list for all draggable schematic items.
     */
    static const KICAD_T DraggableItems[];

    /**
     * A scan list for all rotatable schematic items.
     */
    static const KICAD_T RotatableItems[];

    /**
     * A scan list for only parent schematic items.
     */
    static const KICAD_T ParentItems[];

    /**
     * A scan list for all schematic items except pins.
     */
    static const KICAD_T AllItemsButPins[];

    /**
     * A scan list for schematic component items only.
     */
    static const KICAD_T ComponentsOnly[];

    /**
     * A scan list for schematic sheet items only.
     */
    static const KICAD_T SheetsOnly[];

    /**
     * A scan list for schematic sheet and sheet label items.
     */
    static const KICAD_T SheetsAndSheetLabels[];

    /**
     * Constructor SCH_COLLECTOR
     */
    SCH_COLLECTOR( const KICAD_T* aScanTypes = SCH_COLLECTOR::AllItems )
    {
        SetScanTypes( aScanTypes );
    }

    /**
     * Operator []
     * overloads COLLECTOR::operator[](int) to return a SCH_ITEM* instead of
     * an EDA_ITEM* type.
     * @param aIndex The index into the list.
     * @return SCH_ITEM* at \a aIndex or NULL.
     */
    SCH_ITEM* operator[]( int aIndex ) const
    {
        if( (unsigned)aIndex < (unsigned)GetCount() )
            return (SCH_ITEM*) m_List[ aIndex ];

        return NULL;
    }

    /**
     * Function Inspect
     * is the examining function within the INSPECTOR which is passed to the
     * Iterate function.
     *
     * @param aItem An EDA_ITEM to examine.
     * @param aTestData is not used in this class.
     * @return SEARCH_RESULT #SEARCH_QUIT if the iterator is to stop the scan,
     *                       else #SEARCH_CONTINUE;
     */
    SEARCH_RESULT Inspect( EDA_ITEM* aItem, const void* aTestData = NULL );

    /**
     * Function Collect
     * scans a SCH_ITEM using this class's Inspector method, which does the collection.
     * @param aItem A SCH_ITEM to scan.
     * @param aFilterList A list of #KICAD_T types with a terminating #EOT, that determines
     *                    what is to be collected and the priority order of the resulting
     *                    collection.
     * @param aPosition A wxPoint to use in hit-testing.
     */
    void Collect( SCH_ITEM* aItem, const KICAD_T aScanList[], const wxPoint& aPositiion );

    /**
     * Function IsCorner
     * tests if the collected items forms as corner of two line segments.
     * @return True if the collected items form a corner of two line segments.
     */
    bool IsCorner() const;

    /**
     * Function IsNode
     * tests if the collected items form a node.
     *
     * @param aIncludePins Indicate if component pin items should be included in the test.
     * @return True if the collected items form a node.
     */
    bool IsNode( bool aIncludePins = true ) const;
};


#endif // _SCH_COLLECTORS_H_
