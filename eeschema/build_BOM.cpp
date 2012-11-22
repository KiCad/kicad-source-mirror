/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2012 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2012 Wayne Stambaugh <stambaughw@verizon.net>
 * Copyright (C) 1992-2012 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <fctsys.h>
#include <class_sch_screen.h>
#include <sch_sheet.h>
#include <sch_component.h>
#include <template_fieldnames.h>

#include <BOM_lister.h>


void BOM_LISTER::CreateCsvBOMList( char aSeparator, FILE * aFile )
{
    m_outFile = aFile;
    m_separatorSymbol = aSeparator;

    SCH_REFERENCE_LIST cmplist;
    SCH_SHEET_LIST sheetList;

    sheetList.GetComponents( cmplist, false );

    // sort component list by ref and remove sub components
    cmplist.RemoveSubComponentsFromList();

    // sort component list by value
    cmplist.SortByValueOnly( );
    PrintComponentsListByPart( cmplist );

    fclose( m_outFile );
    m_outFile = NULL;
}


void BOM_LISTER::PrintComponentsListByPart( SCH_REFERENCE_LIST& aList )
{
    unsigned int index = 0;
    while( index < aList.GetCount() )
    {
        SCH_COMPONENT *component = aList[index].GetComponent();
        wxString referenceListStr;
        int qty = 1;
        referenceListStr.append( aList[index].GetRef() );
        for( unsigned int i = index+1; i < aList.GetCount(); )
        {
            if( *(aList[i].GetComponent()) == *component )
            {
                referenceListStr.append( wxT( " " ) + aList[i].GetRef() );
                aList.RemoveItem( i );
                qty++;
            }
            else
                i++; // Increment index only when current item is not removed from the list
        }

        // Write value, quantity and list of references
        fprintf( m_outFile, "%s%c%d%c\"%s\"", TO_UTF8( component->GetField( VALUE )->GetText() ),
                 m_separatorSymbol, qty,
                 m_separatorSymbol, TO_UTF8( referenceListStr ) );

        for( int i = FOOTPRINT; i < component->GetFieldCount(); i++ )
        {
            if( isFieldPrintable( i ) )
                fprintf( m_outFile, "%c%s", m_separatorSymbol,
                         TO_UTF8( component->GetField( i )->GetText() ) );
        }
        fprintf( m_outFile, "\n" );
        index++;
    }
}

bool BOM_LISTER::isFieldPrintable( int aFieldId )
{
    for( unsigned ii = 0; ii < m_fieldIDactive.size(); ii ++ )
        if( m_fieldIDactive[ii] == aFieldId )
            return true;

    return false;
}

void BOM_LISTER::AddFieldIdToPrintList( int aFieldId )
{
    for( unsigned ii = 0; ii < m_fieldIDactive.size(); ii ++ )
        if( m_fieldIDactive[ii] == aFieldId )
            return;

    m_fieldIDactive.push_back( aFieldId );
}


/* compare function for sorting labels
 * sort by
 *     value
 *     if same value: by sheet
 */
static bool SortLabelsByValue( const BOM_LABEL& obj1, const BOM_LABEL& obj2 )
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
static bool SortLabelsBySheet( const BOM_LABEL& obj1, const BOM_LABEL& obj2 )
{
    int      ii;

    ii = obj1.GetSheetPath().Cmp( obj2.GetSheetPath() );

    if( ii == 0 )
    {
        ii = obj1.GetText().CmpNoCase( obj2.GetText() );
    }

    return ii < 0;
}

void BOM_LISTER::buildGlobalAndHierarchicalLabelsList()
{
    m_labelList.clear();

    // Explore the flat sheet list
    SCH_SHEET_LIST sheetList;
    for( SCH_SHEET_PATH* path = sheetList.GetFirst(); path; path = sheetList.GetNext() )
    {
        SCH_ITEM* schItem = (SCH_ITEM*) path->LastDrawList();
        for( ; schItem; schItem = schItem->Next() )
        {
            switch( schItem->Type() )
            {
                case SCH_HIERARCHICAL_LABEL_T:
                case SCH_GLOBAL_LABEL_T:
                    m_labelList.push_back( BOM_LABEL( schItem->Type(), schItem, *path ) );
                    break;

                case SCH_SHEET_T:
                {
                    SCH_SHEET* sheet = (SCH_SHEET*) schItem;

                    BOOST_FOREACH( SCH_SHEET_PIN& sheetPin, sheet->GetPins() )
                    {
                        m_labelList.push_back( BOM_LABEL( SCH_SHEET_PIN_T,
                                                          &sheetPin, *path ) );
                    }
                }
                break;

                default:
                    break;
            }
        }
    }
}

void BOM_LISTER::PrintGlobalAndHierarchicalLabelsList( FILE * aFile, bool aSortBySheet )
{
    m_outFile = aFile;

    buildGlobalAndHierarchicalLabelsList();

    wxString msg;

    if( aSortBySheet )
    {
        sort( m_labelList.begin(), m_labelList.end(), SortLabelsBySheet );
        msg.Printf( _( "\n#Global, Hierarchical Labels and PinSheets \
( order = Sheet Number ) count = %d\n" ),
                    m_labelList.size() );
    }

    else
    {
        sort( m_labelList.begin(), m_labelList.end(), SortLabelsByValue );
        msg.Printf( _( "\n#Global, Hierarchical Labels and PinSheets ( \
order = Alphab. ) count = %d\n\n" ),
                     m_labelList.size() );

    }

    fprintf( m_outFile, "%s", TO_UTF8( msg ) );

    SCH_LABEL* label;
    SCH_SHEET_PIN* pinsheet;
    wxString sheetpath;
    wxString labeltype;

    for( unsigned ii = 0; ii < m_labelList.size(); ii++ )
    {
        switch( m_labelList[ii].GetType() )
        {
        case SCH_HIERARCHICAL_LABEL_T:
        case SCH_GLOBAL_LABEL_T:
            label = (SCH_LABEL*)(m_labelList[ii].GetLabel());

            if( m_labelList[ii].GetType() == SCH_HIERARCHICAL_LABEL_T )
                labeltype = wxT( "Hierarchical" );
            else
                labeltype = wxT( "Global      " );

            sheetpath = m_labelList[ii].GetSheetPath().PathHumanReadable();
            msg.Printf( _( "> %-28.28s %s        (Sheet %s) pos: %3.3f, %3.3f\n" ),
                        GetChars( label->GetText() ),
                        GetChars( labeltype ), GetChars( sheetpath ),
                        (float) label->m_Pos.x / 1000,
                        (float) label->m_Pos.y / 1000 );

            fputs( TO_UTF8( msg ), m_outFile );
            break;

        case SCH_SHEET_PIN_T:
        {
            pinsheet = (SCH_SHEET_PIN*) m_labelList[ii].GetLabel();
            int jj = pinsheet->GetShape();

            if( jj < 0 )
                jj = NET_TMAX;

            if( jj > NET_TMAX )
                jj = 4;

            wxString labtype = FROM_UTF8( SheetLabelType[jj] );

            msg.Printf( _( "> %-28.28s PinSheet %-7.7s (Sheet %s) pos: %3.3f, %3.3f\n" ),
                        GetChars( pinsheet->GetText() ),
                        GetChars( labtype ),
                        GetChars( m_labelList[ii].GetSheetPath().PathHumanReadable() ),
                        (float) pinsheet->m_Pos.x / 1000,
                        (float) pinsheet->m_Pos.y / 1000 );

            fputs( TO_UTF8( msg ), m_outFile );
        }

        break;

        default:
            break;
        }
    }

    msg = _( "#End labels\n" );
    fputs( TO_UTF8( msg ), m_outFile );
}
