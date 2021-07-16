/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2017-2021 KiCad Developers, see AUTHORS.txt for contributors.
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

#include "gerber_collectors.h"

const KICAD_T GERBER_COLLECTOR::AllItems[] = {
    GERBER_LAYOUT_T,
    GERBER_IMAGE_T,
    GERBER_DRAW_ITEM_T,
    EOT
};


/**
 * The examining function within the INSPECTOR which is passed to the iterate function.
 *
 * @param testItem is an EDA_ITEM to examine.
 * @param testData is not used here.
 * @return SEARCH_QUIT if the iterator is to stop the scan, else SCAN_CONTINUE.
 */
SEARCH_RESULT GERBER_COLLECTOR::Inspect( EDA_ITEM* testItem, void* testData )
{
    if( testItem->HitTest( m_refPos ) )
        Append( testItem );

    return SEARCH_RESULT::CONTINUE;
}


void GERBER_COLLECTOR::Collect( EDA_ITEM* aItem, const KICAD_T aScanList[],
                                const wxPoint& aRefPos/*, const COLLECTORS_GUIDE& aGuide*/ )
{
    Empty();        // empty the collection, primary criteria list

    // remember guide, pass it to Inspect()
    //SetGuide( &aGuide );

    SetScanTypes( aScanList );

    // remember where the snapshot was taken from and pass refPos to
    // the Inspect() function.
    SetRefPos( aRefPos );

    aItem->Visit( m_inspector, nullptr, m_scanTypes );

    // record the length of the primary list before concatenating on to it.
    m_PrimaryLength = m_list.size();
}
