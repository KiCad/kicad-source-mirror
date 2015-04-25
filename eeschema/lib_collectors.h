/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2011 Wayne Stambaugh <stambaughw@verizon.net>
 * Copyright (C) 2011 KiCad Developers, see change_log.txt for contributors.
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

#ifndef _LIB_COLLECTORS_H_
#define _LIB_COLLECTORS_H_


#include <class_collector.h>
#include <lib_draw_item.h>


class LIB_COLLECTOR;


class LIB_COLLECTOR_DATA
{
    int m_unit;
    int m_convert;

    friend class LIB_COLLECTOR;

public:
    LIB_COLLECTOR_DATA() :
        m_unit( 0 ),
        m_convert( 0 ) {}
};


/**
 * Class LIB_COLLECTOR
 */
class LIB_COLLECTOR : public COLLECTOR
{
    LIB_COLLECTOR_DATA m_data;

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
     * A scan list for all rotatable schematic items.
     */
    static const KICAD_T RotatableItems[];

    /**
     * A scan list for all schematic items except pins.
     */
    static const KICAD_T AllItemsButPins[];

    /**
     * Constructor LIB_COLLECTOR
     */
    LIB_COLLECTOR( const KICAD_T* aScanTypes = LIB_COLLECTOR::AllItems )
    {
        SetScanTypes( aScanTypes );
    }

    /**
     * Operator []
     * overloads COLLECTOR::operator[](int) to return a LIB_ITEM* instead of
     * an EDA_ITEM* type.
     * @param aIndex The index into the list.
     * @return LIB_ITEM* at \a aIndex or NULL.
     */
    LIB_ITEM* operator[]( int aIndex ) const
    {
        if( (unsigned)aIndex < (unsigned)GetCount() )
            return (LIB_ITEM*) m_List[ aIndex ];

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
     * @param aUnit The unit of the items to collect or zero if all units.
     * @param aConvert The convert of the items to collect or zero if all conversions.
     */
    void Collect( LIB_ITEMS& aItem, const KICAD_T aFilterList[], const wxPoint& aPosition,
                  int aUnit = 0, int aConvert = 0 );
};


#endif // _LIB_COLLECTORS_H_
