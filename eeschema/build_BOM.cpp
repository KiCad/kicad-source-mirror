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

#include <algorithm>    // to use sort vector
#include <vector>

#include <fctsys.h>
#include <sch_sheet.h>
#include <sch_component.h>
#include <template_fieldnames.h>
#include <class_library.h>
#include <base_units.h>

#include <BOM_lister.h>

/* Creates the list of components, grouped by values:
 * One line by value. The format is something like:
 *   value;quantity;references;other fields
 *   18pF;2;"C404 C405";SM0402
 *   22nF/25V;4;"C128 C168 C228 C268";SM0402
 * param aFile = the file to write to (will be closed)
 */
void BOM_LISTER::CreateCsvBOMListByValues( FILE* aFile )
{
    m_outFile = aFile;

    SCH_SHEET_LIST sheetList;

    sheetList.GetComponents( m_cmplist, false );

    // sort component list by ref and remove sub components
    m_cmplist.RemoveSubComponentsFromList();

    // sort component list by value
    m_cmplist.SortByValueOnly();

    unsigned int index = 0;

    while( index < m_cmplist.GetCount() )
    {
        SCH_COMPONENT*  component = m_cmplist[index].GetComponent();
        wxString        referenceListStr;
        int             qty = 1;
        referenceListStr.append( m_cmplist[index].GetRef() );

        for( unsigned int ii = index + 1; ii < m_cmplist.GetCount(); )
        {
            if( *( m_cmplist[ii].GetComponent() ) == *component )
            {
                referenceListStr.append( wxT( " " ) + m_cmplist[ii].GetRef() );
                m_cmplist.RemoveItem( ii );
                qty++;
            }
            else
                ii++; // Increment index only when current item is not removed from the list
        }

        // Write value, quantity and list of references
        fprintf( m_outFile, "%s%c%d%c\"%s\"",
                 TO_UTF8( component->GetField( VALUE )->GetText() ),
                 m_separatorSymbol, qty,
                 m_separatorSymbol, TO_UTF8( referenceListStr ) );

        for( int ii = FOOTPRINT; ii < component->GetFieldCount(); ii++ )
        {
            if( isFieldPrintable( ii ) )
                fprintf( m_outFile, "%c%s", m_separatorSymbol,
                         TO_UTF8( component->GetField( ii )->GetText() ) );
        }

        fprintf( m_outFile, "\n" );
        index++;
    }

    fclose( m_outFile );
    m_outFile = NULL;
}


bool BOM_LISTER::isFieldPrintable( int aFieldId )
{
    for( unsigned ii = 0; ii < m_fieldIDactive.size(); ii++ )
        if( m_fieldIDactive[ii] == aFieldId )
            return true;




    return false;
}


void BOM_LISTER::AddFieldIdToPrintList( int aFieldId )
{
    for( unsigned ii = 0; ii < m_fieldIDactive.size(); ii++ )
        if( m_fieldIDactive[ii] == aFieldId )
            return;




    m_fieldIDactive.push_back( aFieldId );
}


/* compare function for sorting labels by value, then by sheet
 */
static bool SortLabelsByValue( const BOM_LABEL& obj1, const BOM_LABEL& obj2 )
{
    int ii = obj1.GetText().CmpNoCase( obj2.GetText() );

    if( ii == 0 )
        ii = obj1.GetSheetPath().Cmp( obj2.GetSheetPath() );

    return ii < 0;
}


/* compare function for sorting labels by sheet, then by alphabetic order
 */
static bool SortLabelsBySheet( const BOM_LABEL& obj1, const BOM_LABEL& obj2 )
{
    int ii = obj1.GetSheetPath().Cmp( obj2.GetSheetPath() );

    if( ii == 0 )
        ii = obj1.GetText().CmpNoCase( obj2.GetText() );

    return ii < 0;
}


// Creates the flat list of global, hierachycal labels and pin sheets
// and populate m_labelList
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

                    BOOST_FOREACH( SCH_SHEET_PIN & sheetPin, sheet->GetPins() ) {
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


// Print the flat list of global, hierachycal labels and pin sheets
// contained by m_labelList
void BOM_LISTER::PrintGlobalAndHierarchicalLabelsList( FILE* aFile, bool aSortBySheet )
{
    m_outFile = aFile;

    buildGlobalAndHierarchicalLabelsList();

    wxString msg;

    if( aSortBySheet )
    {
        sort( m_labelList.begin(), m_labelList.end(), SortLabelsBySheet );
        msg.Printf( _(
                        "\n#Global, Hierarchical Labels and PinSheets \
( order = Sheet Number ) count = %d\n"                                                                         ),
                    m_labelList.size() );
    }
    else
    {
        sort( m_labelList.begin(), m_labelList.end(), SortLabelsByValue );
        msg.Printf( _(
                        "\n#Global, Hierarchical Labels and PinSheets ( \
order = Alphab. ) count = %d\n\n"                                                                            ),
                    m_labelList.size() );
    }

    fprintf( m_outFile, "%s", TO_UTF8( msg ) );

    SCH_LABEL*      label;
    SCH_SHEET_PIN*  pinsheet;
    wxString        sheetpath;
    wxString        labeltype;

    for( unsigned ii = 0; ii < m_labelList.size(); ii++ )
    {
        switch( m_labelList[ii].GetType() )
        {
        case SCH_HIERARCHICAL_LABEL_T:
        case SCH_GLOBAL_LABEL_T:
            label = (SCH_LABEL*) ( m_labelList[ii].GetLabel() );

            if( m_labelList[ii].GetType() == SCH_HIERARCHICAL_LABEL_T )
                labeltype = wxT( "Hierarchical" );
            else
                labeltype = wxT( "Global      " );

            sheetpath = m_labelList[ii].GetSheetPath().PathHumanReadable();
            msg.Printf( wxT( "> %-28.28s %s        %s\n" ),
                        GetChars( label->GetText() ),
                        GetChars( labeltype ),
                        GetChars( returnURLItemLocation( sheetpath, label->m_Pos ) ) );

            fputs( TO_UTF8( msg ), m_outFile );
            break;

        case SCH_SHEET_PIN_T:
            pinsheet    = (SCH_SHEET_PIN*) m_labelList[ii].GetLabel();
            labeltype   = FROM_UTF8( SheetLabelType[pinsheet->GetShape()] );

            msg.Printf( _( "> %-28.28s PinSheet %-7.7s %s\n" ),
                        GetChars( pinsheet->GetText() ),
                        GetChars( labeltype ),
                        GetChars( returnURLItemLocation( m_labelList[ii].GetSheetPath().
                                                         PathHumanReadable(),
                                                         pinsheet->m_Pos ) ) );

            fputs( TO_UTF8( msg ), m_outFile );
            break;

        default:
            break;
        }
    }

    msg = _( "#End labels\n" );
    fputs( TO_UTF8( msg ), m_outFile );
}


/*
 * Helper function
 * returns a string containing all selected fields texts,
 * separated by the csv separator symbol (csv form) or a ;
 */
const wxString BOM_LISTER::returnFieldsString( SCH_COMPONENT* aComponent )
{
    wxString    outStr;
    wxString    tmpStr;
    wxString    text;

    for( int ii = FOOTPRINT; ii <= FIELD8; ii++ )
    {
        if( !isFieldPrintable( ii ) )
            continue;

        if( aComponent->GetFieldCount() > ii )
            text = aComponent->GetField( ii )->m_Text;
        else
            text = wxEmptyString;

        if( m_csvForm )
            tmpStr.Printf( wxT( "%c%s" ), m_separatorSymbol, GetChars( text ) );
        else
            tmpStr.Printf( wxT( "; %-12s" ), GetChars( text ) );

        outStr += tmpStr;
    }

    return outStr;
}


/* print the list of components ordered by references,
 * full component list in human readable form
 * param aFile = the file to write to (will be NOT closed)
 */

/* full list in human readable form sample:
 * #Cmp ( order = Reference )with sub-composants
 * | C101       47pF        Loc /(X=344,170 mm, Y=116,840 mm); C1          ; field1      ;
 * | C102       47pF        Loc /(X=364,490 mm, Y=116,840 mm); C1          ;             ;
 * | C103       47uF        Loc /(X=66,040 mm, Y=231,140 mm); CP6         ;             ;
 */

bool BOM_LISTER::PrintComponentsListByReferenceHumanReadable( FILE* aFile )
{
    m_outFile = aFile;
    bool addDatasheet = isFieldPrintable( DATASHEET );

    // Print component location if needed, but only when
    // include sub component option is enabled, because for multiple
    // parts per package there are more than one location per reference
    bool        printLocCmp = m_printLocation && m_includeSubComponents;

    wxString    msg;

    if( m_cmplist.GetCount() == 0 )    // Build component list
    {
        SCH_SHEET_LIST sheetList;
        sheetList.GetComponents( m_cmplist, false );

        // sort component list
        m_cmplist.SortByReferenceOnly();

        if( !m_includeSubComponents )
            m_cmplist.RemoveSubComponentsFromList();
    }
    else
        m_cmplist.SortByReferenceOnly();

    // Print comment line:
    msg = _( "#Cmp ( order = Reference )" );

    if( m_includeSubComponents )
        msg << _( " (with SubCmp)" );

    fprintf( m_outFile, "%s\n", TO_UTF8( msg ) );

    wxString    subReference;   // Unit ident, for mutiple parts per package
    std::string CmpName;

    // Print list of items
    for( unsigned ii = 0; ii < m_cmplist.GetCount(); ii++ )
    {
        EDA_ITEM* item = m_cmplist[ii].GetComponent();

        if( item == NULL )
            continue;

        if( item->Type() != SCH_COMPONENT_T )
            continue;

        SCH_COMPONENT*  comp = (SCH_COMPONENT*) item;

        bool            isMulti = false;

        LIB_COMPONENT*  entry = CMP_LIBRARY::FindLibraryComponent( comp->GetLibName() );

        if( entry )
            isMulti = entry->IsMulti();

        CmpName = m_cmplist[ii].GetRefStr();

        if( isMulti && m_includeSubComponents )
        {
            subReference = LIB_COMPONENT::ReturnSubReference( m_cmplist[ii].GetUnit() );
            CmpName += TO_UTF8( subReference );
        }

        fprintf( m_outFile, "| %-10s %-12s", CmpName.c_str(),
                 TO_UTF8( comp->GetField( VALUE )->m_Text ) );

        if( addDatasheet )
            fprintf( m_outFile, "%-20s",
                     TO_UTF8( comp->GetField( DATASHEET )->m_Text ) );

        if( m_includeSubComponents )
        {
            if( printLocCmp )
            {
                msg = returnURLItemLocation( m_cmplist[ii].GetSheetPath().PathHumanReadable(),
                                             comp->GetPosition() );
                fprintf( m_outFile, "%s", TO_UTF8( msg ) );
            }
        }

        wxString tmpStr = returnFieldsString( comp );
        fprintf( m_outFile, "%s\n", TO_UTF8( tmpStr ) );
    }

    // Print the last line:
    fputs(  "#End Cmp\n", m_outFile );

    return true;
}


/* print the list of components ordered by references. Generate 2 formats:
 * - full component list in csv form
 * - "short" component list in csv form, grouped by common fields values
 *          (mainly component value)
 * param aFile = the file to write to (will be NOT closed)
 */

/* full csv format sample:
 *  ref;value;sheet path(location);footprint;field1;field2
 *  C101;47pF;Loc /(X=57,150 mm, Y=74,930 mm);Loc /(X=344,170 mm, Y=116,840 mm));C1;field1;
 *  C102;47pF;Loc /(X=344,170 mm, Y=116,840 mm);Loc /(X=364,490 mm, Y=116,840 mm));C1;;
 *  C103;47uF;Loc /(X=364,490 mm, Y=116,840 mm);Loc /(X=66,040 mm, Y=231,140 mm));CP6;;
 *  C104;47uF;Loc /(X=66,040 mm, Y=231,140 mm);Loc /(X=82,550 mm, Y=231,140 mm));CP6;;
 */
/* short csv format sample:
 *  ref;value;footprint;Champ1;Champ2
 *  C101;47pF;C1;field1;;1
 *  C102;47pF;C1;;;1
 *  C103..C106;47uF;CP6;;;4
 */

bool BOM_LISTER::PrintComponentsListByReferenceCsvForm( FILE* aFile )
{
    m_outFile = aFile;
    bool addDatasheet = isFieldPrintable( DATASHEET );

    // Set option group references, for components having same field values
    // (same value, same footprint ...)
    // obviously, this is possible only when print location
    // and include Sub Components are not enabled.
    bool groupRefs = m_groupReferences;
    bool includeSubComponents = m_includeSubComponents && !groupRefs;

    // Print component location if needed, but only when
    // include sub component option is enabled, because for multiple
    // parts per package there are more than one location per reference
    bool        printLocCmp = m_printLocation && !groupRefs && m_includeSubComponents;

    wxString    msg;

    if( m_cmplist.GetCount() == 0 )    // Build component list
    {
        SCH_SHEET_LIST sheetList;
        sheetList.GetComponents( m_cmplist, false );

        // sort component list
        m_cmplist.SortByReferenceOnly();

        if( !includeSubComponents )
            m_cmplist.RemoveSubComponentsFromList();
    }
    else
        m_cmplist.SortByReferenceOnly();

    // Print comment line:
    msg = wxT( "ref" );
    msg << (wxChar)m_separatorSymbol << wxT( "value" );

    if( addDatasheet )
        msg << (wxChar)m_separatorSymbol << wxT( "datasheet" );

    if( printLocCmp )
        msg << (wxChar)m_separatorSymbol << wxT( "sheet path(location)" );

    if( isFieldPrintable( FOOTPRINT ) )
        msg << (wxChar)m_separatorSymbol << wxT( "footprint" );

    for( int ii = FIELD1; ii <= FIELD8; ii++ )
    {
        if( isFieldPrintable( ii ) )
            msg << (wxChar)m_separatorSymbol << _( "Field" ) << ii - FIELD1 + 1;
    }

    if( groupRefs )
        msg << (wxChar)m_separatorSymbol << _( "Item count" );

    fprintf( m_outFile, "%s\n", TO_UTF8( msg ) );

    // Print BOM list
    wxString    strCur;
    wxString    strPred;
    int         amount = 0;     // number of items, on the same line
    wxString    cmpName;
    wxString    cmpNameFirst;
    wxString    cmpNameLast;

    // Print list of items, by reference
    for( unsigned ii = 0; ii < m_cmplist.GetCount(); ii++ )
    {
        EDA_ITEM* item = m_cmplist[ii].GetComponent();

        if( item == NULL )
            continue;

        if( item->Type() != SCH_COMPONENT_T )
            continue;

        SCH_COMPONENT*  comp = (SCH_COMPONENT*) item;

        LIB_COMPONENT*  entry = CMP_LIBRARY::FindLibraryComponent( comp->GetLibName() );

        bool            isMulti = false;

        if( entry )
            isMulti = entry->IsMulti();

        cmpName = m_cmplist[ii].GetRef();

        if( isMulti && includeSubComponents )
            // Add unit ident, for mutiple parts per package
            cmpName += LIB_COMPONENT::ReturnSubReference( m_cmplist[ii].GetUnit() );

        if( groupRefs )
        {
            // Store value and datasheet (will be printed later)
            strCur.Empty();
            strCur << (wxChar)m_separatorSymbol << comp->GetField( VALUE )->m_Text;

            if( addDatasheet )
                strCur << (wxChar)m_separatorSymbol << comp->GetField( DATASHEET )->m_Text;
        }
        else
        {
            // Print the current component reference, value and datasheet
            msg = cmpName;
            msg << (wxChar)m_separatorSymbol << comp->GetField( VALUE )->m_Text;

            if( addDatasheet )
                msg << (wxChar)m_separatorSymbol << comp->GetField( DATASHEET )->m_Text;

            fprintf( m_outFile, "%s",  TO_UTF8( msg ) );
        }

        if( printLocCmp )   // Is allowed only for full list (not grouped)
        {
            msg = returnURLItemLocation(
                m_cmplist[ii].GetSheetPath().PathHumanReadable(),
                comp->GetPosition() );

            fprintf( m_outFile, "%c%s", m_separatorSymbol, TO_UTF8( msg ) );
        }

        if( groupRefs )
        {
            wxString tmpStr = returnFieldsString( comp );
            strCur += tmpStr;

            if( strPred.Len() == 0 )
                cmpNameFirst = cmpName;
            else
            {
                // print a BOM line
                msg.Empty();
                if( !strCur.IsSameAs( strPred ) )
                {
                    switch( amount )
                    {
                    case 1:     // One reference to print
                                // format C103;47uF;CP6;;;1
                        msg << cmpNameFirst <<strPred << (wxChar)m_separatorSymbol << amount;
                        break;

                    case 2:     // 2 references to print
                                // format C103,C104;47uF;CP6;;;2
                        msg << cmpNameFirst  << wxT(",") << cmpNameLast
                            << strPred << (wxChar)m_separatorSymbol << amount;
                        break;

                    default:    // Many references to print :
                                // format: C103..C106;47uF;CP6;;;4
                        msg << cmpNameFirst << wxT("..") << cmpNameLast
                            << strPred << (wxChar)m_separatorSymbol << amount;
                        break;
                    }
                    fprintf( m_outFile, "%s\n", TO_UTF8( msg ) );

                    cmpNameFirst = cmpName;
                    amount = 0;
                }
            }

            strPred     = strCur;
            cmpNameLast = cmpName;
            amount++;
        }
        else
        {
            msg = returnFieldsString( comp );
            fprintf( m_outFile, "%s\n", TO_UTF8( msg ) );
        }
    }

    // Print the last line:
    if( groupRefs )
    {
        msg.Empty();
        switch( amount )
        {
        case 1:
            msg << cmpNameFirst << strPred << (wxChar)m_separatorSymbol << amount;
            break;

        case 2:
            msg << cmpNameFirst  << wxT(",") << cmpNameLast
                << strPred << (wxChar)m_separatorSymbol << amount;
            break;

        default:
           msg << cmpNameFirst << wxT("..") << cmpNameFirst << cmpNameLast
               << strPred << (wxChar)m_separatorSymbol << amount;
            break;
        }
        fprintf( m_outFile, "%s\n", TO_UTF8( msg ) );
    }

    return true;
}


/* PrintComponentsListByValue
 * print the list of components, sorted by value, one line per component
 * param aFile = the file to write to (will be NOT closed)
 * not useable for csv format (use CreateCsvBOMListByValues instead)
 * format:
 *   | 10pF         C15       Loc /controle/(X=48,260 mm, Y=83,820 mm); <fields>
 *   | 10pF         C16       Loc /controle/(X=68,580 mm, Y=83,820 mm); <fields>
 */
int BOM_LISTER::PrintComponentsListByValue( FILE* aFile )
{
    m_outFile = aFile;

    if( m_cmplist.GetCount() == 0 )    // Build component list
    {
        SCH_SHEET_LIST sheetList;
        sheetList.GetComponents( m_cmplist, false );

        if( !m_includeSubComponents )
        {
            // sort component list
            m_cmplist.SortByReferenceOnly();
            m_cmplist.RemoveSubComponentsFromList();
        }
    }

    m_cmplist.SortByValueOnly();

    wxString    msg;

    msg = _( "\n#Cmp ( order = Value )" );

    if( m_includeSubComponents )
        msg << _( " (with SubCmp)" );

    msg << wxT( "\n" );

    fputs( TO_UTF8( msg ), m_outFile );

    std::string cmpName;
    for( unsigned ii = 0; ii < m_cmplist.GetCount(); ii++ )
    {
        EDA_ITEM* schItem = m_cmplist[ii].GetComponent();

        if( schItem == NULL )
            continue;

        if( schItem->Type() != SCH_COMPONENT_T )
            continue;

        SCH_COMPONENT*  drawLibItem = (SCH_COMPONENT*) schItem;

        bool            isMulti = false;
        LIB_COMPONENT*  entry   = CMP_LIBRARY::FindLibraryComponent( drawLibItem->GetLibName() );

        if( entry )
            isMulti = entry->IsMulti();

        cmpName = m_cmplist[ii].GetRefStr();

        if( isMulti && m_includeSubComponents )
            // Add unit ident, for mutiple parts per package
            cmpName += TO_UTF8( LIB_COMPONENT::ReturnSubReference( m_cmplist[ii].GetUnit() ) );

        fprintf( m_outFile, "| %-12s %-10s",
                 TO_UTF8( drawLibItem->GetField( VALUE )->m_Text ),
                 cmpName.c_str() );

        // print the sheet path and location
        if( m_includeSubComponents )
        {
            msg = returnURLItemLocation( m_cmplist[ii].GetSheetPath().PathHumanReadable(),
                                         drawLibItem->GetPosition() );
            fprintf( m_outFile, "%s", TO_UTF8( msg ) );
        }

        fprintf( m_outFile, "%s\n", TO_UTF8( returnFieldsString( drawLibItem ) ) );
    }

    msg = _( "#End Cmp\n" );
    fputs( TO_UTF8( msg ), m_outFile );
    return 0;
}


/* returnURLItemLocation
 * return a formated string to print the full location:
 * <sheet name>/( X Y position)
 * param aPathName = the full sheet name of item
 * param aPosition = a position (in internal units) to print
 */
const wxString BOM_LISTER::returnURLItemLocation( const wxString&   aPathName,
                                                  wxPoint           aPosition )
{
    wxString text;

    text.Printf( wxT( "Loc %s(X=%s, Y=%s)" ), GetChars( aPathName ),
                 GetChars( ReturnStringFromValue( g_UserUnit, aPosition.x, true ) ),
                 GetChars( ReturnStringFromValue( g_UserUnit, aPosition.y, true ) ) );
    return text;
}
