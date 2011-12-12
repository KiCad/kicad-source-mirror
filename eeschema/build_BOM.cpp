/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2009 Jean-Pierre Charras, jaen-pierre.charras@gipsa-lab.inpg.com
 * Copyright (C) 2011 Wayne Stambaugh <stambaughw@verizon.net>
 * Copyright (C) 1992-2011 KiCad Developers, see AUTHORS.txt for contributors.
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

/**
 * @file build_BOM.cpp
 * @brief Code used to generate bill of materials.
 */

#include <algorithm> // to use sort vector
#include <vector>

#include "fctsys.h"
#include "class_sch_screen.h"
#include "kicad_string.h"

#include "general.h"
#include "sch_sheet.h"
#include "sch_component.h"
#include "template_fieldnames.h"
#include "netlist.h"


/* Fill aList  with labels
 */
void GenListeGLabels( BOM_LABEL_LIST& aList )
{
    // Build the sheet list
    SCH_SHEET_LIST sheetList;
    BOM_LABEL      label;

    for( SCH_SHEET_PATH* path = sheetList.GetFirst(); path; path = sheetList.GetNext() )
    {
        SCH_ITEM* schItem = (SCH_ITEM*) path->LastDrawList();

        while( schItem )
        {
            switch( schItem->Type() )
            {
            case SCH_HIERARCHICAL_LABEL_T:
            case SCH_GLOBAL_LABEL_T:
                aList.push_back( BOM_LABEL( schItem->Type(), schItem, *path ) );
                break;

            case SCH_SHEET_T:
            {
                SCH_SHEET* sheet = (SCH_SHEET*) schItem;

                BOOST_FOREACH( SCH_SHEET_PIN& sheetPin, sheet->GetPins() )
                {
                    aList.push_back( BOM_LABEL( SCH_SHEET_PIN_T, &sheetPin, *path ) );
                }
            }
            break;

            default:
                break;
            }

            schItem = schItem->Next();
        }
    }
}


/* compare function for sorting labels
 * sort by
 *     value
 *     if same value: by sheet
 */
bool SortLabelsByValue( const BOM_LABEL& obj1, const BOM_LABEL& obj2 )
{
    int       ii;

    ii = obj1.GetText().CmpNoCase( obj2.GetText() );

    if( ii == 0 )
    {
        ii = obj1.GetSheetPath().Cmp( obj2.GetSheetPath() );
    }

    return ii < 0;
}


/* compare function for sorting labels
 *     by sheet
 *     in a sheet, by alphabetic order
 */
bool SortLabelsBySheet( const BOM_LABEL& obj1, const BOM_LABEL& obj2 )
{
    int      ii;

    ii = obj1.GetSheetPath().Cmp( obj2.GetSheetPath() );

    if( ii == 0 )
    {
        ii = obj1.GetText().CmpNoCase( obj2.GetText() );
    }

    return ii < 0;
}


int PrintListeGLabel( FILE* f, BOM_LABEL_LIST& aList )
{
    SCH_LABEL* label;
    SCH_SHEET_PIN* pinsheet;
    wxString msg, sheetpath;
    wxString labeltype;

    for( unsigned ii = 0; ii < aList.size(); ii++ )
    {
        switch( aList[ii].GetType() )
        {
        case SCH_HIERARCHICAL_LABEL_T:
        case SCH_GLOBAL_LABEL_T:
            label = (SCH_LABEL*)(aList[ii].GetLabel());

            if( aList[ii].GetType() == SCH_HIERARCHICAL_LABEL_T )
                labeltype = wxT( "Hierarchical" );
            else
                labeltype = wxT( "Global      " );

            sheetpath = aList[ii].GetSheetPath().PathHumanReadable();
            msg.Printf( _( "> %-28.28s %s        (Sheet %s) pos: %3.3f, %3.3f\n" ),
                        GetChars( label->GetText() ),
                        GetChars( labeltype ),
                        GetChars( sheetpath ),
                        (float) label->m_Pos.x / 1000,
                        (float) label->m_Pos.y / 1000 );

            fputs( TO_UTF8( msg ), f );
            break;

        case SCH_SHEET_PIN_T:
        {
            pinsheet = (SCH_SHEET_PIN*) aList[ii].GetLabel();
            int jj = pinsheet->GetShape();

            if( jj < 0 )
                jj = NET_TMAX;

            if( jj > NET_TMAX )
                jj = 4;

            wxString labtype = FROM_UTF8( SheetLabelType[jj] );

            msg.Printf( _( "> %-28.28s PinSheet %-7.7s (Sheet %s) pos: %3.3f, %3.3f\n" ),
                        GetChars( pinsheet->GetText() ),
                        GetChars( labtype ),
                        GetChars( aList[ii].GetSheetPath().PathHumanReadable() ),
                        (float) pinsheet->m_Pos.x / 1000,
                        (float) pinsheet->m_Pos.y / 1000 );

            fputs( TO_UTF8( msg ), f );
        }

        break;

        default:
            break;
        }
    }

    msg = _( "#End labels\n" );
    fputs( TO_UTF8( msg ), f );
    return 0;
}
