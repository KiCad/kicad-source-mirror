/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef GERBER_COLLECTORS_H
#define GERBER_COLLECTORS_H

#include <collector.h>

/**
 * Use when the right click button is pressed or when the select tool is in effect.
 */
class GERBER_COLLECTOR : public COLLECTOR
{
public:
    GERBER_COLLECTOR()
    {
        SetScanTypes( { GERBER_LAYOUT_T, GERBER_IMAGE_T, GERBER_DRAW_ITEM_T } );
    }

    /**
     * Overload the [](int) operator to return a EDA_ITEM* instead of an EDA_ITEM* type.
     *
     * @param ndx The index into the list.
     * @return the item collected or NULL.
     */
    EDA_ITEM* operator[]( int ndx ) const override
    {
        if( (unsigned)ndx < (unsigned)GetCount() )
            return (EDA_ITEM*) m_list[ ndx ];

        return nullptr;
    }

    /**
     * The examining function within the INSPECTOR which is passed to the Iterate function.
     *
     * @param testItem An EDA_ITEM to examine.
     * @param testData is not used in this class.
     * @return SEARCH_QUIT if the Iterator is to stop the scan else SCAN_CONTINUE.
     */
    INSPECT_RESULT Inspect( EDA_ITEM* testItem, void* testData )  override;

    /**
     * Scan an EDA_ITEM using this class's Inspector method, which does the collection.
     *
     * @param aItem An EDA_ITEM to scan
     * @param aScanTypes A list of KICAD_Ts that specs what is to be collected and the priority
     *                   order of the resultant collection in "m_list".
     * @param aRefPos A VECTOR2I to use in hit-testing.
     */
    void Collect( EDA_ITEM* aItem, const std::vector<KICAD_T>& aScanTypes,
                  const VECTOR2I& aRefPos );
};

#endif
