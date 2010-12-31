/////////////////////////////////////////////////////////////////////////////
// Name:     build_BOM.cpp
// Purpose:
// Author:   jean-pierre Charras
// License:  GPL license
/////////////////////////////////////////////////////////////////////////////

#include <algorithm> // to use sort vector
#include <vector>

#include "fctsys.h"
#include "common.h"
#include "class_sch_screen.h"
#include "kicad_string.h"

#include "general.h"
#include "netlist.h"
#include "protos.h"
#include "sch_sheet.h"
#include "sch_component.h"
#include "template_fieldnames.h"


/**
 * @bug - Every instance of fprintf() and fputs() in this file fails to check
 *        the return value for an error.
 */


/* Fill aList  with Glabel info
 */
void GenListeGLabels( std::vector <LABEL_OBJECT>& aList )
{
    // Build the sheet list
    SCH_SHEET_LIST  sheetList;

    LABEL_OBJECT    lable;

    for( SCH_SHEET_PATH* path = sheetList.GetFirst();  path;  path = sheetList.GetNext() )
    {
        SCH_ITEM* schItem = (SCH_ITEM*) path->LastDrawList();

        while( schItem )
        {
            switch( schItem->Type() )
            {
            case SCH_HIERARCHICAL_LABEL_T:
            case SCH_GLOBAL_LABEL_T:
                lable.m_LabelType = schItem->Type();
                lable.m_SheetPath = *path;
                lable.m_Label     = schItem;
                aList.push_back( lable );
                break;

            case SCH_SHEET_T:
            {
                SCH_SHEET* sheet = (SCH_SHEET*) schItem;

                BOOST_FOREACH( SCH_SHEET_PIN sheetLabel, sheet->GetSheetPins() )
                {
                    lable.m_LabelType = SCH_SHEET_LABEL_T;
                    lable.m_SheetPath = *path;
                    lable.m_Label     = &sheetLabel;
                    aList.push_back( lable );
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


/* Compare function for sort()
 * components are sorted
 *    by value
 *    if same value: by reference
 *         if same reference: by unit number
 */
bool SortComponentsByValue( const SCH_REFERENCE& obj1, const SCH_REFERENCE& obj2 )
{
    int             ii;
    const wxString* Text1, * Text2;

    Text1 = &( obj1.m_RootCmp->GetField( VALUE )->m_Text );
    Text2 = &( obj2.m_RootCmp->GetField( VALUE )->m_Text );
    ii    = Text1->CmpNoCase( *Text2 );

    if( ii == 0 )
    {
        ii = RefDesStringCompare( obj1.GetRef(), obj2.GetRef() );
    }

    if( ii == 0 )
    {
        ii = obj1.m_Unit - obj2.m_Unit;
    }

    return ii < 0;
}


/* compare function for sorting
 * components are sorted
 *     by reference
 *     if same reference: by value
 *         if same value: by unit number
 */
bool SortComponentsByReference( const SCH_REFERENCE& obj1, const SCH_REFERENCE& obj2 )
{
    int             ii;
    const wxString* Text1, * Text2;

    ii = RefDesStringCompare( obj1.GetRef(), obj2.GetRef() );

    if( ii == 0 )
    {
        Text1 = &( obj1.m_RootCmp->GetField( VALUE )->m_Text );
        Text2 = &( obj2.m_RootCmp->GetField( VALUE )->m_Text );
        ii    = Text1->CmpNoCase( *Text2 );
    }

    if( ii == 0 )
    {
        ii = obj1.m_Unit - obj2.m_Unit;
    }

    return ii < 0;
}


/* compare function for sorting labels
 * sort by
 *     value
 *     if same value: by sheet
 */
bool SortLabelsByValue( const LABEL_OBJECT& obj1, const LABEL_OBJECT& obj2 )
{
    int       ii;
    wxString* Text1, * Text2;

    if( obj1.m_LabelType == SCH_SHEET_LABEL_T )
        Text1 = &( (SCH_SHEET_PIN*)(obj1.m_Label) )->m_Text;
    else
        Text1 = &( (SCH_TEXT*)(obj1.m_Label) )->m_Text;

    if( obj2.m_LabelType == SCH_SHEET_LABEL_T )
        Text2 = &( (SCH_SHEET_PIN*)(obj2.m_Label) )->m_Text;
    else
        Text2 = &( (SCH_TEXT*)(obj2.m_Label) )->m_Text;

    ii = Text1->CmpNoCase( *Text2 );

    if( ii == 0 )
    {
        ii = obj1.m_SheetPath.Cmp( obj2.m_SheetPath );
    }

    return ii < 0;
}


/* compare function for sorting labels
 *     by sheet
 *     in a sheet, by alphabetic order
 */
bool SortLabelsBySheet( const LABEL_OBJECT& obj1, const LABEL_OBJECT& obj2 )
{
    int      ii;
    wxString Text1, Text2;

    ii = obj1.m_SheetPath.Cmp( obj2.m_SheetPath );

    if( ii == 0 )
    {
        if( obj1.m_LabelType == SCH_SHEET_LABEL_T )
            Text1 = ( (SCH_SHEET_PIN*) obj1.m_Label )->m_Text;
        else
            Text1 = ( (SCH_TEXT*) obj1.m_Label )->m_Text;

        if( obj2.m_LabelType == SCH_SHEET_LABEL_T )
            Text2 = ( (SCH_SHEET_PIN*) obj2.m_Label )->m_Text;
        else
            Text2 = ( (SCH_TEXT*) obj2.m_Label )->m_Text;

        ii = Text1.CmpNoCase( Text2 );
    }

    return ii < 0;
}


/* Remove sub components from the list, when multiples parts per package are
 * found in this list
 * The component list **MUST** be sorted by reference and by unit number
 */
void DeleteSubCmp( std::vector< SCH_REFERENCE >& aList )
{
    SCH_COMPONENT* libItem;
    wxString       oldName;
    wxString       currName;


    for( unsigned ii = 0; ii < aList.size(); ii++ )
    {
        libItem = aList[ii].m_RootCmp;

        if( libItem == NULL )
            continue;

        currName = aList[ii].GetRef();

        if( !oldName.IsEmpty() )
        {
            if( oldName == currName )   // currName is a subpart of oldName: remove it
            {
                aList.erase( aList.begin() + ii );
                ii--;
            }
        }
        oldName = currName;
    }
}


int PrintListeGLabel( FILE* f, std::vector <LABEL_OBJECT>& aList )
{
    SCH_LABEL* DrawTextItem;
    SCH_SHEET_PIN* DrawSheetLabel;
    wxString msg, sheetpath;
    wxString labeltype;

    for( unsigned ii = 0; ii < aList.size(); ii++ )
    {
        switch( aList[ii].m_LabelType )
        {
        case SCH_HIERARCHICAL_LABEL_T:
        case SCH_GLOBAL_LABEL_T:
            DrawTextItem = (SCH_LABEL*)(aList[ii].m_Label);

            if( aList[ii].m_LabelType == SCH_HIERARCHICAL_LABEL_T )
                labeltype = wxT( "Hierarchical" );
            else
                labeltype = wxT( "Global      " );

            sheetpath = aList[ii].m_SheetPath.PathHumanReadable();
            msg.Printf( _( "> %-28.28s %s        (Sheet %s) pos: %3.3f, %3.3f\n" ),
                        GetChars( DrawTextItem->m_Text ),
                        GetChars( labeltype ),
                        GetChars( sheetpath ),
                        (float) DrawTextItem->m_Pos.x / 1000,
                        (float) DrawTextItem->m_Pos.y / 1000 );

            fputs( CONV_TO_UTF8( msg ), f );
            break;

        case SCH_SHEET_LABEL_T:
        {
            DrawSheetLabel = (SCH_SHEET_PIN*) aList[ii].m_Label;
            int jj = DrawSheetLabel->m_Shape;

            if( jj < 0 )
                jj = NET_TMAX;

            if( jj > NET_TMAX )
                jj = 4;

            wxString labtype = CONV_FROM_UTF8( SheetLabelType[jj] );

            msg.Printf( _( "> %-28.28s PinSheet %-7.7s (Sheet %s) pos: %3.3f, %3.3f\n" ),
                        GetChars( DrawSheetLabel->m_Text ),
                        GetChars( labtype ),
                        GetChars( aList[ii].m_SheetPath.PathHumanReadable() ),
                        (float) DrawSheetLabel->m_Pos.x / 1000,
                        (float) DrawSheetLabel->m_Pos.y / 1000 );

            fputs( CONV_TO_UTF8( msg ), f );
        }
        break;

        default:
            break;
        }
    }

    msg = _( "#End labels\n" );
    fputs( CONV_TO_UTF8( msg ), f );
    return 0;
}
