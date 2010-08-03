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
#include "confirm.h"
#include "kicad_string.h"
#include "gestfich.h"
#include "appl_wxstruct.h"

#include "program.h"
#include "general.h"
#include "class_library.h"
#include "netlist.h"
#include "protos.h"

#include "build_version.h"

#include "dialog_build_BOM.h"


/**
 * @bug - Every instance of fprintf() and fputs() in this file fails to check
 *        the return value for an error.
 */


/**
 * Class LABEL_OBJECT
 * is used in build BOM to handle the list of labels in schematic
 * because in a complex hierarchy, a label is used more than once,
 * and had more than one sheet path, so we must create a flat list of labels
 */
class LABEL_OBJECT
{
public:
    int            m_LabelType;
    SCH_ITEM*      m_Label;

    //have to store it here since the object references will be duplicated.
    SCH_SHEET_PATH m_SheetPath;  //composed of UIDs

public: LABEL_OBJECT()
    {
        m_Label     = NULL;
        m_LabelType = 0;
    }
};


static void BuildComponentsListFromSchematic(
    std::vector <OBJ_CMP_TO_LIST>& aList );

static void GenListeGLabels( std::vector <LABEL_OBJECT>& aList );
static bool SortComponentsByReference(  const OBJ_CMP_TO_LIST& obj1,
                                        const OBJ_CMP_TO_LIST& obj2 );
static bool SortComponentsByValue(  const OBJ_CMP_TO_LIST& obj1,
                                    const OBJ_CMP_TO_LIST& obj2 );
static bool SortLabelsByValue( const LABEL_OBJECT& obj1,
                               const LABEL_OBJECT& obj2 );
static bool SortLabelsBySheet( const LABEL_OBJECT& obj1,
                               const LABEL_OBJECT& obj2 );
static void DeleteSubCmp( std::vector <OBJ_CMP_TO_LIST>& aList );
static int  PrintListeGLabel( FILE* f, std::vector <LABEL_OBJECT>& aList );


// separator used in bom export to spreadsheet
static char s_ExportSeparatorSymbol;


void DIALOG_BUILD_BOM::Create_BOM_Lists( int  aTypeFile,
                                         bool aIncludeSubComponents,
                                         char aExportSeparatorSymbol,
                                         bool aRunBrowser )
{
    wxString    wildcard;

    static wxFileName fn;

    wxFileName  current = g_RootSheet->m_AssociatedScreen->m_FileName;

    s_ExportSeparatorSymbol = aExportSeparatorSymbol;

    if( !fn.HasName() || fn.GetName()==NAMELESS_PROJECT )
    {
        fn.SetName( current.GetName() );
    }
    // else use a previous run's name, because fn was set before and user
    // is probably just iteratively refining the BOM.

    if( fn.GetPath().IsEmpty() )
    {
        fn.SetPath( current.GetPath() );
    }
    // else use a previous run's path, because fn was set before and user
    // is probably just iteratively refining the BOM.

    wxString bomDesc = _( "Bill of Materials" );    // translate once, use twice.

    if( aTypeFile == 0 )
    {
        fn.SetExt( wxT( "lst" ) );
        wildcard = bomDesc + wxT( " (*.lst)|*.lst" );
    }
    else
    {
        fn.SetExt( wxT( "csv" ) );
        wildcard = bomDesc + wxT( " (*.csv)|*.csv" );
    }

    wxFileDialog dlg( this, bomDesc, fn.GetPath(),
                      fn.GetFullName(), wildcard,
                      wxFD_SAVE | wxFD_OVERWRITE_PROMPT );

    if( dlg.ShowModal() == wxID_CANCEL )
        return;

    fn = dlg.GetPath();        // remember path+filename+ext for subsequent runs.

    m_ListFileName = dlg.GetPath();

    // Close dialog, then show the list (if so requested)

    switch( aTypeFile )
    {
    case 0: // list
        GenereListeOfItems( m_ListFileName, aIncludeSubComponents );
        break;

    case 1: // speadsheet
        CreateExportList( m_ListFileName, aIncludeSubComponents );
        break;

    case 2: // Single Part per line
        CreatePartsList( m_ListFileName, aIncludeSubComponents );
        break;
    }

    EndModal( 1 );

    if( aRunBrowser )
    {
        wxString editorname = wxGetApp().GetEditorName();
        wxString filename   = m_ListFileName;
        AddDelimiterString( filename );
        ExecuteFile( this, editorname, filename );
    }
}

/** Helper function IsFieldChecked
 * return the state of the wxCheckbox corresponding to the
 * field aFieldId (FOOTPRINT and FIELD1 to FIELD8
 * if the option "All user fields" is checked, return always true
 * for fileds ids >= FIELD1
 * @param aFieldId = the field id : FOOTPRINT to FIELD8
 */
bool DIALOG_BUILD_BOM::IsFieldChecked(int aFieldId)
{
    if( m_AddAllFields->IsChecked() && (aFieldId>= FIELD1) )
        return true;

    switch ( aFieldId )
    {
        case FIELD1:
            return m_AddField1->IsChecked();
        case FIELD2:
            return m_AddField2->IsChecked();
        case FIELD3:
            return m_AddField3->IsChecked();
        case FIELD4:
            return m_AddField4->IsChecked();
        case FIELD5:
            return m_AddField5->IsChecked();
        case FIELD6:
            return m_AddField6->IsChecked();
        case FIELD7:
            return m_AddField7->IsChecked();
        case FIELD8:
            return m_AddField8->IsChecked();
        case FOOTPRINT:
            return m_AddFootprintField->IsChecked();
    }

    return false;
}


void DIALOG_BUILD_BOM::CreatePartsList( const wxString& aFullFileName, bool aIncludeSubComponents )
{
    FILE*    f;
    wxString msg;

    if( ( f = wxFopen( aFullFileName, wxT( "wt" ) ) ) == NULL )
    {
        msg = _( "Failed to open file " );
        msg << aFullFileName;
        DisplayError( this, msg );
        return;
    }

    std::vector <OBJ_CMP_TO_LIST> cmplist;

    BuildComponentsListFromSchematic( cmplist );

    // sort component list by ref and remove sub components
    if( !aIncludeSubComponents )
    {
        sort( cmplist.begin(), cmplist.end(), SortComponentsByReference );
        DeleteSubCmp( cmplist );
    }

    // sort component list by value
    sort( cmplist.begin(), cmplist.end(), SortComponentsByValue );
    PrintComponentsListByPart( f, cmplist,aIncludeSubComponents );

    fclose( f );
}


/*
 * Print a list of components, in a form which can be imported by a spreadsheet
 * form is:
 * cmp name; cmp val; fields;
 */
void DIALOG_BUILD_BOM::CreateExportList( const wxString& aFullFileName,
                                         bool            aIncludeSubComponents )
{
    FILE*    f;
    wxString msg;

    if( ( f = wxFopen( aFullFileName, wxT( "wt" ) ) ) == NULL )
    {
        msg = _( "Failed to open file " );
        msg << aFullFileName;
        DisplayError( this, msg );
        return;
    }

    std::vector <OBJ_CMP_TO_LIST> cmplist;
    BuildComponentsListFromSchematic( cmplist );

    // sort component list
    sort( cmplist.begin(), cmplist.end(), SortComponentsByReference );

    if( !aIncludeSubComponents )
        DeleteSubCmp( cmplist );

    // create the file
    PrintComponentsListByRef( f, cmplist, TRUE, aIncludeSubComponents );

    fclose( f );
}


/** GenereListeOfItems()
 * Main function to create the list of components and/or labels
 * (global labels and pin sheets" )
 */
void DIALOG_BUILD_BOM::GenereListeOfItems( const wxString& aFullFileName,
                                           bool            aIncludeSubComponents )
{
    FILE*    f;
    int      itemCount;
    char     Line[1024];
    wxString msg;

    if( ( f = wxFopen( aFullFileName, wxT( "wt" ) ) ) == NULL )
    {
        msg = _( "Failed to open file " );
        msg << aFullFileName;
        DisplayError( this, msg );
        return;
    }

    std::vector <OBJ_CMP_TO_LIST> cmplist;
    BuildComponentsListFromSchematic( cmplist );

    itemCount = cmplist.size();
    if( itemCount )
    {
        // creates the list file
        DateAndTime( Line );

        wxString Title = wxGetApp().GetAppName() + wxT( " " ) +
                         GetBuildVersion();

        fprintf( f, "%s  >> Creation date: %s\n", CONV_TO_UTF8( Title ), Line );

        // sort component list
        sort( cmplist.begin(), cmplist.end(), SortComponentsByReference );

        if( !aIncludeSubComponents )
            DeleteSubCmp( cmplist );

        if( m_ListCmpbyRefItems->GetValue() )
            PrintComponentsListByRef( f, cmplist, false, aIncludeSubComponents );

        if( m_ListCmpbyValItems->GetValue() )
        {
            sort( cmplist.begin(), cmplist.end(), SortComponentsByValue );
            PrintComponentsListByVal( f, cmplist, aIncludeSubComponents );
        }
    }

    /*************************************************/
    /* Create list of global labels and pins sheets */
    /*************************************************/
    std::vector <LABEL_OBJECT> listOfLabels;

    GenListeGLabels( listOfLabels );
    if( ( itemCount = listOfLabels.size() ) > 0 )
    {
        if( m_GenListLabelsbySheet->GetValue() )
        {
            sort( listOfLabels.begin(), listOfLabels.end(), SortLabelsBySheet );

            msg.Printf( _(
                            "\n#Global, Hierarchical Labels and PinSheets "
                            "( order = Sheet Number ) count = %d\n" ),
                        itemCount );

            fprintf( f, "%s", CONV_TO_UTF8( msg ) );
            PrintListeGLabel( f, listOfLabels );
        }

        if( m_GenListLabelsbyVal->GetValue() )
        {
            sort( listOfLabels.begin(), listOfLabels.end(), SortLabelsByValue );

            msg.Printf( _(
                            "\n#Global, Hierarchical Labels and PinSheets ( "
                            "order = Alphab. ) count = %d\n\n" ),
                        itemCount );

            fprintf( f, "%s", CONV_TO_UTF8( msg ) );
            PrintListeGLabel( f, listOfLabels );
        }
    }

    msg = _( "\n#End List\n" );
    fprintf( f, "%s", CONV_TO_UTF8( msg ) );
    fclose( f );
}


/**
 * Function BuildComponentsListFromSchematic
 * creates the list of components found in the whole schematic.
 *
 * Goes through the 'sheets', not the screens, so that we account for
 * multiple instances of a given screen.
 */
void BuildComponentsListFromSchematic( std::vector <OBJ_CMP_TO_LIST>& aList )
{
    // Build the sheet list (which is not screen a screen list)
    SCH_SHEET_LIST  sheetList;  // uses a global

    for( SCH_SHEET_PATH* path = sheetList.GetFirst();  path;  path = sheetList.GetNext() )
    {
        for( EDA_BaseStruct* schItem = path->LastDrawList();  schItem;  schItem = schItem->Next() )
        {
            if( schItem->Type() != TYPE_SCH_COMPONENT )
                continue;

            SCH_COMPONENT* comp = (SCH_COMPONENT*) schItem;

            comp->SetParent( path->LastScreen() );

            OBJ_CMP_TO_LIST item;

            item.m_RootCmp   = comp;
            item.m_SheetPath = *path;
            item.m_Unit      = comp->GetUnitSelection( path );

            item.SetRef( comp->GetRef( path ) );

            // skip pseudo components, which have a reference starting
            // with #, mainly power symbols
            if( item.GetRefStr()[0] == '#' )
                continue;

            // Real component found, keep it
            aList.push_back( item );
        }
    }
}


/* Fill aList  with Glabel info
 */
static void GenListeGLabels( std::vector <LABEL_OBJECT>& aList )
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
            case TYPE_SCH_HIERLABEL:
            case TYPE_SCH_GLOBALLABEL:
                lable.m_LabelType = schItem->Type();
                lable.m_SheetPath = *path;
                lable.m_Label     = schItem;
                aList.push_back( lable );
                break;

            case DRAW_SHEET_STRUCT_TYPE:
            {
                SCH_SHEET* sheet = (SCH_SHEET*) schItem;

                BOOST_FOREACH( SCH_SHEET_PIN sheetLabel, sheet->GetSheetPins() )
                {
                    lable.m_LabelType = DRAW_HIERARCHICAL_PIN_SHEET_STRUCT_TYPE;
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
bool SortComponentsByValue( const OBJ_CMP_TO_LIST& obj1,
                            const OBJ_CMP_TO_LIST& obj2 )
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
bool SortComponentsByReference( const OBJ_CMP_TO_LIST& obj1,
                                const OBJ_CMP_TO_LIST& obj2 )
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

    if( obj1.m_LabelType == DRAW_HIERARCHICAL_PIN_SHEET_STRUCT_TYPE )
        Text1 = &( (SCH_SHEET_PIN*)(obj1.m_Label) )->m_Text;
    else
        Text1 = &( (SCH_TEXT*)(obj1.m_Label) )->m_Text;

    if( obj2.m_LabelType == DRAW_HIERARCHICAL_PIN_SHEET_STRUCT_TYPE )
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
        if( obj1.m_LabelType == DRAW_HIERARCHICAL_PIN_SHEET_STRUCT_TYPE )
            Text1 = ( (SCH_SHEET_PIN*) obj1.m_Label )->m_Text;
        else
            Text1 = ( (SCH_TEXT*) obj1.m_Label )->m_Text;

        if( obj2.m_LabelType == DRAW_HIERARCHICAL_PIN_SHEET_STRUCT_TYPE )
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
static void DeleteSubCmp( std::vector <OBJ_CMP_TO_LIST>& aList )
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
            if( oldName == currName )   // currName is a subpart of oldName:
                                        // remove it
            {
                aList.erase( aList.begin() + ii );
                ii--;
            }
        }
        oldName = currName;
    }
}


void DIALOG_BUILD_BOM::PrintFieldData( FILE* f, SCH_COMPONENT* DrawLibItem,
                                       bool CompactForm )
{
    int ii;

    if( IsFieldChecked( FOOTPRINT ) )
    {
        if( CompactForm )
        {
            fprintf( f, "%c%s", s_ExportSeparatorSymbol,
                    CONV_TO_UTF8( DrawLibItem->GetField( FOOTPRINT )->m_Text ) );
        }
        else
        {
            fprintf( f, "; %-12s",
                    CONV_TO_UTF8( DrawLibItem->GetField( FOOTPRINT )->m_Text ) );
        }
    }

    for( ii = FIELD1; ii < DrawLibItem->GetFieldCount(); ii++ )
    {
        if( ! IsFieldChecked( ii ) )
            continue;

        if( CompactForm )
            fprintf( f, "%c%s", s_ExportSeparatorSymbol,
                    CONV_TO_UTF8( DrawLibItem->GetField( ii )->m_Text ) );
        else
            fprintf( f, "; %-12s",
                    CONV_TO_UTF8( DrawLibItem->GetField( ii )->m_Text ) );
    }
}


/* Print the B.O.M sorted by reference
 */
int DIALOG_BUILD_BOM::PrintComponentsListByRef(
    FILE*                          f,
    std::vector <OBJ_CMP_TO_LIST>& aList,
    bool                           CompactForm,
    bool                           aIncludeSubComponents )
{
    wxString        msg;

    if( CompactForm )
    {
        // Print comment line:
#if defined(KICAD_GOST)
        fprintf( f, "ref%cvalue%cdatasheet", s_ExportSeparatorSymbol, s_ExportSeparatorSymbol );
#else
        fprintf( f, "ref%cvalue", s_ExportSeparatorSymbol );
#endif

        if( aIncludeSubComponents )
        {
            fprintf( f, "%csheet path", s_ExportSeparatorSymbol );
            fprintf( f, "%clocation", s_ExportSeparatorSymbol );
        }

        if( IsFieldChecked( FOOTPRINT ) )
            fprintf( f, "%cfootprint", s_ExportSeparatorSymbol );

        for( int ii = FIELD1; ii <= FIELD8; ii++ )
        {
            if( !IsFieldChecked( ii ) )
                continue;

            msg = _( "Field" );

            fprintf( f, "%c%s%d", s_ExportSeparatorSymbol, CONV_TO_UTF8(
                         msg ), ii - FIELD1 + 1 );
        }

        fprintf( f, "\n" );
    }
    else
    {
        msg = _( "\n#Cmp ( order = Reference )" );

        if( aIncludeSubComponents )
            msg << _( " (with SubCmp)" );

        fprintf( f, "%s\n", CONV_TO_UTF8( msg ) );
    }

    std::string     CmpName;
    wxString        subRef;

    // Print list of items
    for( unsigned ii = 0; ii < aList.size(); ii++ )
    {
        EDA_BaseStruct* item = aList[ii].m_RootCmp;
        if( item == NULL )
            continue;

        if( item->Type() != TYPE_SCH_COMPONENT )
            continue;

        SCH_COMPONENT*  comp = (SCH_COMPONENT*) item;

        bool isMulti = false;

        LIB_COMPONENT*  entry = CMP_LIBRARY::FindLibraryComponent( comp->m_ChipName );
        if( entry )
            isMulti = entry->IsMulti();

        if( isMulti && aIncludeSubComponents )
            subRef = LIB_COMPONENT::ReturnSubReference( aList[ii].m_Unit );
        else
            subRef.Empty();

        CmpName = aList[ii].GetRefStr();

        if( !CompactForm )
            CmpName += CONV_TO_UTF8(subRef);

        if( CompactForm )
#if defined(KICAD_GOST)
            fprintf( f, "%s%c%s%c%s", CmpName.c_str(), s_ExportSeparatorSymbol,
                    CONV_TO_UTF8( comp->GetField(
                                      VALUE )->m_Text ), s_ExportSeparatorSymbol,
                    CONV_TO_UTF8( comp->GetField( DATASHEET )->m_Text ) );
#else
            fprintf( f, "%s%c%s", CmpName.c_str(), s_ExportSeparatorSymbol,
                    CONV_TO_UTF8( comp->GetField( VALUE )->m_Text ) );
#endif

        else
#if defined(KICAD_GOST)
            fprintf( f, "| %-10s %-12s %-20s", CmpName.c_str(),
                    CONV_TO_UTF8( comp->GetField( VALUE )->m_Text ),
                    CONV_TO_UTF8( comp->GetField( DATASHEET )->m_Text ) );
#else
            fprintf( f, "| %-10s %-12s", CmpName.c_str(),
                    CONV_TO_UTF8( comp->GetField( VALUE )->m_Text ) );
#endif

        if( aIncludeSubComponents )
        {
            msg = aList[ii].m_SheetPath.PathHumanReadable();

            if( CompactForm )
            {
                fprintf( f, "%c%s", s_ExportSeparatorSymbol,
                        CONV_TO_UTF8( msg ) );

                msg = m_Parent->GetXYSheetReferences(
                    (BASE_SCREEN*) comp->GetParent(),
                    comp->m_Pos );

                fprintf( f, "%c%s)", s_ExportSeparatorSymbol,
                        CONV_TO_UTF8( msg ) );
            }
            else
            {
                fprintf( f, "   (Sheet %s)", CONV_TO_UTF8( msg ) );

                msg = m_Parent->GetXYSheetReferences(
                    (BASE_SCREEN*) comp->GetParent(),
                    comp->m_Pos );

                fprintf( f, "   (loc %s)", CONV_TO_UTF8( msg ) );
            }
        }

        PrintFieldData( f, comp, CompactForm );

        fprintf( f, "\n" );
    }

    if( !CompactForm )
    {
        msg = _( "#End Cmp\n" );
        fputs( CONV_TO_UTF8( msg ), f );
    }

    return 0;
}


/* Bom Output format option - single part per line
 *  a common part being defined as have a common value.
 *  This is true for most designs but will produce an
 *  incorrect output if two or more parts with the same
 *  value have different footprints, tolerances, voltage
 *  rating, etc.  Also usefull if the following fields
 *  are edited:
 *   FIELD1 - manufacture
 *   FIELD2 - manufacture part number
 *   FIELD3 - distributor part number
 */
int DIALOG_BUILD_BOM::PrintComponentsListByPart(
    FILE*                          f,
    std::vector <OBJ_CMP_TO_LIST>& aList,
    bool aIncludeSubComponents)
{
    int             qty = 0;
    wxString        refName;
    wxString        fullRefName;        // reference + part Id (for multiple parts per package
    wxString        valName;
    wxString        refNames;
    wxString        lastRef;
    wxString        unitId;
    SCH_COMPONENT*  currCmp;
    SCH_COMPONENT*  nextCmp;
    SCH_COMPONENT   dummyCmp;           // A dummy component, to store fields

    for( unsigned ii = 0; ii < aList.size(); ii++ )
    {
        currCmp = (SCH_COMPONENT*) aList[ii].m_RootCmp;

        if( ii < aList.size() -1 )
            nextCmp = aList[ii+1].m_RootCmp;
        else
            nextCmp = NULL;

        // Store fields. Store non empty fields only.
        for( int jj = FOOTPRINT; jj < currCmp->GetFieldCount(); jj++ )
        {
            // Ensure fields exists in dummy component
            if( dummyCmp.GetFieldCount() <= jj )
                dummyCmp.AddField( *currCmp->GetField( jj ) );

            // store useful data
            if( !currCmp->GetField( jj )->m_Text.IsEmpty() )
                dummyCmp.GetField( jj )->m_Text = currCmp->GetField( jj )->m_Text;
        }

        refName = aList[ii].GetRef();
        valName = currCmp->GetField( VALUE )->m_Text;

        int multi = 0;
        if( aIncludeSubComponents )
        {
            LIB_COMPONENT* entry = CMP_LIBRARY::FindLibraryComponent( currCmp->m_ChipName );
            if( entry )
                multi = entry->GetPartCount();
            if ( multi <= 1 )
                multi = 0;
        }

        if ( multi && aList[ii].m_Unit > 0 )
            unitId.Printf( wxT("%c"), 'A' -1 + aList[ii].m_Unit );
        else
            unitId.Empty();

        fullRefName = refName + unitId;

        if( refNames.IsEmpty() )
            refNames = fullRefName;
        else
            refNames << wxT( ", " ) << fullRefName;

        // In multi parts per package, we have the reference more than once
        // but we must count only one package
        if( lastRef != refName )
            qty++;

        lastRef = refName;

        // if the next cmoponent has same value the line will be printed after.
        if( nextCmp && nextCmp->GetField( VALUE )->m_Text.CmpNoCase( valName ) == 0 )
            continue;

       // Print line for the current component value:
        fprintf( f, "%15s%c%3d", CONV_TO_UTF8( valName ), s_ExportSeparatorSymbol, qty );

        if( IsFieldChecked(FOOTPRINT ) )
            fprintf( f, "%c%15s", s_ExportSeparatorSymbol,
                    CONV_TO_UTF8( currCmp->GetField( FOOTPRINT )->m_Text ) );

#if defined(KICAD_GOST)
            fprintf( f, "%c%20s", s_ExportSeparatorSymbol,
                    CONV_TO_UTF8( currCmp->GetField( DATASHEET )->m_Text ) );
#endif

        // wrap the field in quotes, since it has commas in it.
        fprintf( f, "%c\"%s\"", s_ExportSeparatorSymbol,
                CONV_TO_UTF8( refNames ) );

        // print fields, on demand
        int last_nonempty_field_idx = 0;

        for( int jj = FOOTPRINT; jj < dummyCmp.GetFieldCount(); jj++ )
            if ( !dummyCmp.GetField( jj )->m_Text.IsEmpty() )
                last_nonempty_field_idx = jj;

        for( int jj = FIELD1; jj <= last_nonempty_field_idx ; jj++ )
        {
            if ( IsFieldChecked( jj ) )
                fprintf( f, "%c%4s", s_ExportSeparatorSymbol,
                        CONV_TO_UTF8( dummyCmp.GetField( jj )->m_Text ) );
        }

        fprintf( f, "\n" );

        // Clear strings and values, to prepare next component
        qty = 0;
        refNames.Empty();

        for( int jj = FOOTPRINT; jj < dummyCmp.GetFieldCount(); jj++ )
            dummyCmp.GetField( jj )->m_Text.Empty();
    }

    return 0;
}


int DIALOG_BUILD_BOM::PrintComponentsListByVal(
    FILE*                          f,
    std::vector <OBJ_CMP_TO_LIST>& aList,
    bool                           aIncludeSubComponents )
{
    EDA_BaseStruct* schItem;
    SCH_COMPONENT*  DrawLibItem;
    LIB_COMPONENT*  entry;
    std::string     CmpName;
    wxString        msg;

    msg = _( "\n#Cmp ( order = Value )" );

    if( aIncludeSubComponents )
        msg << _( " (with SubCmp)" );
    msg << wxT( "\n" );

    fputs( CONV_TO_UTF8( msg ), f );

    for( unsigned ii = 0; ii < aList.size(); ii++ )
    {
        schItem = aList[ii].m_RootCmp;

        if( schItem == NULL )
            continue;

        if( schItem->Type() != TYPE_SCH_COMPONENT )
            continue;

        DrawLibItem = (SCH_COMPONENT*) schItem;

        bool isMulti = false;
        entry = CMP_LIBRARY::FindLibraryComponent( DrawLibItem->m_ChipName );
        if( entry )
            isMulti = entry->IsMulti();

        wxString subRef;
        if( isMulti && aIncludeSubComponents )
            subRef = LIB_COMPONENT::ReturnSubReference( aList[ii].m_Unit );
        else
            subRef.Empty();

        CmpName = aList[ii].GetRefStr();
        CmpName += CONV_TO_UTF8(subRef);

        fprintf( f, "| %-12s %-10s",
                 CONV_TO_UTF8( DrawLibItem->GetField( VALUE )->m_Text ),
                 CmpName.c_str() );

        // print the sheet path
        if( aIncludeSubComponents )
        {
            msg = aList[ii].m_SheetPath.PathHumanReadable();
            fprintf( f, "   (Sheet %s)", CONV_TO_UTF8( msg ) );
            msg = m_Parent->GetXYSheetReferences(
                (BASE_SCREEN*) DrawLibItem->GetParent(), DrawLibItem->m_Pos );
            fprintf( f, "   (loc %s)", CONV_TO_UTF8( msg ) );
        }

        PrintFieldData( f, DrawLibItem );

        fputs( "\n", f );
    }

    msg = _( "#End Cmp\n" );
    fputs( CONV_TO_UTF8( msg ), f );
    return 0;
}


static int PrintListeGLabel( FILE* f, std::vector <LABEL_OBJECT>& aList )
{
    SCH_LABEL* DrawTextItem;
    SCH_SHEET_PIN* DrawSheetLabel;
    wxString msg, sheetpath;
    wxString labeltype;

    for( unsigned ii = 0; ii < aList.size(); ii++ )
    {
        switch( aList[ii].m_LabelType )
        {
        case TYPE_SCH_HIERLABEL:
        case TYPE_SCH_GLOBALLABEL:
            DrawTextItem = (SCH_LABEL*)(aList[ii].m_Label);

            if( aList[ii].m_LabelType == TYPE_SCH_HIERLABEL )
                labeltype = wxT( "Hierarchical" );
            else
                labeltype = wxT( "Global      " );

            sheetpath = aList[ii].m_SheetPath.PathHumanReadable();
            msg.Printf(
                _( "> %-28.28s %s        (Sheet %s) pos: %3.3f, %3.3f\n" ),
                GetChars( DrawTextItem->m_Text ),
                GetChars( labeltype ),
                GetChars( sheetpath ),
                (float) DrawTextItem->m_Pos.x / 1000,
                (float) DrawTextItem->m_Pos.y / 1000 );

            fputs( CONV_TO_UTF8( msg ), f );
            break;

        case DRAW_HIERARCHICAL_PIN_SHEET_STRUCT_TYPE:
        {
            DrawSheetLabel = (SCH_SHEET_PIN*) aList[ii].m_Label;
            int jj = DrawSheetLabel->m_Shape;

            if( jj < 0 )
                jj = NET_TMAX;

            if( jj > NET_TMAX )
                jj = 4;

            wxString labtype = CONV_FROM_UTF8( SheetLabelType[jj] );

            msg.Printf(
                _( "> %-28.28s PinSheet %-7.7s (Sheet %s) pos: %3.3f, %3.3f\n" ),
                GetChars( DrawSheetLabel->m_Text ),
                GetChars( labtype ),
                GetChars( aList[ii].m_SheetPath.PathHumanReadable()),
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


