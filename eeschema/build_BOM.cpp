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

/* object used in build BOM to handle the list of labels in schematic
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


// Filename extension for BOM list
static const wxString BomFileExtension( wxT( "lst" ) );
static const wxString CsvFileExtension( wxT( "csv" ) );
#define BomFileWildcard _( "Bill of Materials file (*.lst)|*.lst" )


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

int         RefDesStringCompare( const char* obj1, const char* obj2 );
int         SplitString( wxString  strToSplit,
                         wxString* strBeginning,
                         wxString* strDigits,
                         wxString* strEnd );


/* separator used in bom export to spreadsheet */
static char s_ExportSeparatorSymbol;


void DIALOG_BUILD_BOM::Create_BOM_Lists( int  aTypeFile,
                                         bool aIncludeSubComponents,
                                         char aExportSeparatorSymbol,
                                         bool aRunBrowser )
{
    wxFileName fn;

    s_ExportSeparatorSymbol = aExportSeparatorSymbol;

    m_ListFileName = g_RootSheet->m_AssociatedScreen->m_FileName;
    fn = m_ListFileName;
    if( aTypeFile == 2 )
        fn.SetExt( CsvFileExtension );
    else
        fn.SetExt( BomFileExtension );

    wxFileDialog dlg( this, _( "Bill of Materials" ), fn.GetPath(),
                      fn.GetFullName(), BomFileWildcard,
                      wxFD_SAVE | wxFD_OVERWRITE_PROMPT );

    if( dlg.ShowModal() == wxID_CANCEL )
        return;

    m_ListFileName = dlg.GetPath();

    /* Close dialog, then show the list (if so requested) */

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


/*
 * Print a list of components, in a form which can be imported by a spreadsheet
 * form is:
 * cmp value; number of components; <footprint>; field1; field2; field3; list of references having the same value
 */
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

    /*  sort component list by ref and remove sub components*/
    if( !aIncludeSubComponents )
    {
        sort( cmplist.begin(), cmplist.end(), SortComponentsByReference );
        DeleteSubCmp( cmplist );
    }

    /*  sort component list by value*/
    sort( cmplist.begin(), cmplist.end(), SortComponentsByValue );
    PrintComponentsListByPart( f, cmplist );

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

    /*  sort component list */
    sort( cmplist.begin(), cmplist.end(), SortComponentsByReference );

    if( !aIncludeSubComponents )
        DeleteSubCmp( cmplist );

    /* create the file */
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
        /* creates the list file */
        DateAndTime( Line );
        wxString Title = wxGetApp().GetAppName() + wxT( " " ) +
                         GetBuildVersion();
        fprintf( f, "%s  >> Creation date: %s\n", CONV_TO_UTF8( Title ), Line );

        /*  sort component list */
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
                            "\n#Global, Hierarchical Labels and PinSheets \
( order = Sheet Number ) count = %d\n"                                                                              ),
                        itemCount );
            fprintf( f, "%s", CONV_TO_UTF8( msg ) );
            PrintListeGLabel( f, listOfLabels );
        }

        if( m_GenListLabelsbyVal->GetValue() )
        {
            sort( listOfLabels.begin(), listOfLabels.end(), SortLabelsByValue );

            msg.Printf( _(
                            "\n#Global, Hierarchical Labels and PinSheets ( \
order = Alphab. ) count = %d\n\n"                                                                                ),
                        itemCount );
            fprintf( f, "%s", CONV_TO_UTF8( msg ) );
            PrintListeGLabel( f, listOfLabels );
        }
    }

    msg = _( "\n#End List\n" );
    fprintf( f, "%s", CONV_TO_UTF8( msg ) );
    fclose( f );
}


/* Creates the list of components found in the whole schematic
 *
 * if List == null, just returns the count. if not, fills the list.
 * goes through the sheets, not the screens, so that we account for
 * multiple instances of a given screen.
 * Also Initialize m_Father as pointer of the SCH_SCREEN parent
 */
void BuildComponentsListFromSchematic( std::vector <OBJ_CMP_TO_LIST>& aList )
{
    EDA_BaseStruct* SchItem;
    SCH_COMPONENT*  DrawLibItem;
    SCH_SHEET_PATH* sheet;

    /* Build the sheet (not screen) list */
    SCH_SHEET_LIST  SheetList;

    for( sheet = SheetList.GetFirst();
        sheet != NULL;
        sheet = SheetList.GetNext() )
    {
        for( SchItem = sheet->LastDrawList(); SchItem;
            SchItem = SchItem->Next() )
        {
            if( SchItem->Type() != TYPE_SCH_COMPONENT )
                continue;

            DrawLibItem = (SCH_COMPONENT*) SchItem;
            DrawLibItem->SetParent( sheet->LastScreen() );
            OBJ_CMP_TO_LIST item;
            item.m_RootCmp   = DrawLibItem;
            item.m_SheetPath = *sheet;
            item.m_Unit = DrawLibItem->GetUnitSelection( sheet );

            strncpy( item.m_Reference,
                    CONV_TO_UTF8( DrawLibItem->GetRef( sheet ) ),
                    sizeof( item.m_Reference ) );

            // Ensure always null terminate m_Ref.
            item.m_Reference[sizeof( item.m_Reference ) - 1 ] = 0;
            aList.push_back( item );
        }
    }
}


/* Fill aList  with Glabel info
 */
static void GenListeGLabels( std::vector <LABEL_OBJECT>& aList )
{
    SCH_ITEM*       DrawList;
    SCH_SHEET_PIN*  PinLabel;
    SCH_SHEET_PATH* sheet;

    /* Build the sheet list */
    SCH_SHEET_LIST  SheetList;

    LABEL_OBJECT    labet_object;

    for( sheet = SheetList.GetFirst();
        sheet != NULL;
        sheet = SheetList.GetNext() )
    {
        DrawList = (SCH_ITEM*) sheet->LastDrawList();
        while( DrawList )
        {
            switch( DrawList->Type() )
            {
            case TYPE_SCH_HIERLABEL:
            case TYPE_SCH_GLOBALLABEL:
                labet_object.m_LabelType = DrawList->Type();
                labet_object.m_SheetPath = *sheet;
                labet_object.m_Label     = DrawList;
                aList.push_back( labet_object );
                break;

            case DRAW_SHEET_STRUCT_TYPE:
            {
                PinLabel = ( (SCH_SHEET*) DrawList )->m_Label;
                while( PinLabel != NULL )
                {
                    labet_object.m_LabelType =
                        DRAW_HIERARCHICAL_PIN_SHEET_STRUCT_TYPE;
                    labet_object.m_SheetPath = *sheet;
                    labet_object.m_Label     = PinLabel;
                    aList.push_back( labet_object );
                    PinLabel = PinLabel->Next();
                }
            }
            break;

            default:
                break;
            }

            DrawList = DrawList->Next();
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
        ii = RefDesStringCompare( obj1.m_Reference, obj2.m_Reference );
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

    ii = RefDesStringCompare( obj1.m_Reference, obj2.m_Reference );

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

        currName = CONV_FROM_UTF8( aList[ii].m_Reference );

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
    // @todo make this variable length
    const wxCheckBox* FieldListCtrl[] =
    {
        m_AddField1,
        m_AddField2,
        m_AddField3,
        m_AddField4,
        m_AddField5,
        m_AddField6,
        m_AddField7,
        m_AddField8
    };

    int ii;
    const wxCheckBox* FieldCtrl = FieldListCtrl[0];

    if( m_AddFootprintField->IsChecked() )
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
        if( ii <= FIELD8 )    // see users fields 1 to 8
        {
            FieldCtrl = FieldListCtrl[ii - FIELD1];
            if( FieldCtrl == NULL )
                continue;

            if( !FieldCtrl->IsChecked() && !m_AddAllFields->IsChecked() )
                continue;
        }

        if( !m_AddAllFields->IsChecked() )
            break;


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
    int             Multi, Unit;
    EDA_BaseStruct* DrawList;
    SCH_COMPONENT*  DrawLibItem;
    LIB_COMPONENT*  Entry;
    char            CmpName[80];
    wxString        msg;

    if( CompactForm )
    {
        // @todo make this variable length
        const wxCheckBox* FieldListCtrl[FIELD8 - FIELD1 + 1] =
        {
            m_AddField1,
            m_AddField2,
            m_AddField3,
            m_AddField4,
            m_AddField5,
            m_AddField6,
            m_AddField7,
            m_AddField8
        };

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

        if( m_AddFootprintField->IsChecked() )
            fprintf( f, "%cfootprint", s_ExportSeparatorSymbol );

        for( int ii = FIELD1; ii <= FIELD8; ii++ )
        {
            const wxCheckBox* FieldCtrl = FieldListCtrl[ii - FIELD1];
            if( FieldCtrl == NULL )
                continue;

            if( !FieldCtrl->IsChecked() )
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

    // Print list of items
    for( unsigned ii = 0; ii < aList.size(); ii++ )
    {
        DrawList = aList[ii].m_RootCmp;

        if( DrawList == NULL )
            continue;
        if( DrawList->Type() != TYPE_SCH_COMPONENT )
            continue;

        DrawLibItem = (SCH_COMPONENT*) DrawList;
        if( aList[ii].m_Reference[0] == '#' )
            continue;

        Multi = 0;
        Unit  = ' ';
        Entry = CMP_LIBRARY::FindLibraryComponent( DrawLibItem->m_ChipName );
        if( Entry )
            Multi = Entry->GetPartCount();

        if( ( Multi > 1 ) && aIncludeSubComponents )
#if defined(KICAD_GOST)


            Unit = aList[ii].m_Unit + '1' - 1;
#else


            Unit = aList[ii].m_Unit + 'A' - 1;
#endif

        sprintf( CmpName, "%s", aList[ii].m_Reference );
        if( !CompactForm || Unit != ' ' )
            sprintf( CmpName + strlen( CmpName ), "%c", Unit );

        if( CompactForm )
#if defined(KICAD_GOST)


            fprintf( f, "%s%c%s%c%s", CmpName, s_ExportSeparatorSymbol,
                    CONV_TO_UTF8( DrawLibItem->GetField(
                                      VALUE )->m_Text ), s_ExportSeparatorSymbol,
                    CONV_TO_UTF8( DrawLibItem->GetField( DATASHEET )->m_Text ) );
#else


            fprintf( f, "%s%c%s", CmpName, s_ExportSeparatorSymbol,
                    CONV_TO_UTF8( DrawLibItem->GetField( VALUE )->m_Text ) );
#endif

        else
#if defined(KICAD_GOST)


            fprintf( f, "| %-10s %-12s %-20s", CmpName,
                    CONV_TO_UTF8( DrawLibItem->GetField( VALUE )->m_Text ),
                    CONV_TO_UTF8( DrawLibItem->GetField( DATASHEET )->m_Text ) );
#else


            fprintf( f, "| %-10s %-12s", CmpName,
                    CONV_TO_UTF8( DrawLibItem->GetField( VALUE )->m_Text ) );
#endif


        if( aIncludeSubComponents )
        {
            msg = aList[ii].m_SheetPath.PathHumanReadable();
            if( CompactForm )
            {
                fprintf( f, "%c%s", s_ExportSeparatorSymbol,
                        CONV_TO_UTF8( msg ) );
                msg = m_Parent->GetXYSheetReferences(
                    (BASE_SCREEN*) DrawLibItem->GetParent(),
                    DrawLibItem->m_Pos );
                fprintf( f, "%c%s)", s_ExportSeparatorSymbol,
                        CONV_TO_UTF8( msg ) );
            }
            else
            {
                fprintf( f, "   (Sheet %s)", CONV_TO_UTF8( msg ) );
                msg = m_Parent->GetXYSheetReferences(
                    (BASE_SCREEN*) DrawLibItem->GetParent(),
                    DrawLibItem->m_Pos );
                fprintf( f, "   (loc %s)", CONV_TO_UTF8( msg ) );
            }
        }

        PrintFieldData( f, DrawLibItem, CompactForm );

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
    std::vector <OBJ_CMP_TO_LIST>& aList )
{
    int             qty = 1;
    char            RefName[256];
	wxString		ValName, NxtName;
    char            RNames[2000];
    EDA_BaseStruct* DrawList;
    EDA_BaseStruct* NxtList;
    SCH_COMPONENT*  DrawLibItem;
    SCH_COMPONENT*  NxtLibItem;
	SCH_COMPONENT*  m_Cmp;
	SCH_COMPONENT   Cmp;

	NxtName = wxT ("");
    strcpy( RNames, "" );
	m_Cmp = &Cmp;

    for( unsigned ii = 0; ii < aList.size(); ii++ )
    {
        DrawList = aList[ii].m_RootCmp;
        if( DrawList == NULL )
            continue;
        if( DrawList->Type() != TYPE_SCH_COMPONENT )
            continue;
        if( aList[ii].m_Reference[0] == '#' )
            continue;
        DrawLibItem = (SCH_COMPONENT*) DrawList;
		if( (DrawLibItem->GetField( VALUE )->m_Text.IsEmpty()) )
			continue;
		// m_Cmp = DrawLibItem->GenCopy();

        // Store fields. try to store non empty fields.
        for( int jj = FOOTPRINT; jj < DrawLibItem->GetFieldCount(); jj++ )
        {
			if( ! DrawLibItem->GetField( jj )->m_Text.IsEmpty() )
                m_Cmp->GetField( jj )->m_Text = DrawLibItem->GetField( jj )->m_Text;
        }

        NxtLibItem = NULL;
        for( unsigned ij = ii + 1; ij < aList.size(); ij++ )
        {
            NxtList = aList[ij].m_RootCmp;
            if( NxtList == NULL )
                continue;
            if( NxtList->Type() != TYPE_SCH_COMPONENT )
                continue;
            if( aList[ij].m_Reference[0] == '#' )
                continue;
            NxtLibItem = (SCH_COMPONENT*) NxtList;
			if( (NxtLibItem->GetField( VALUE )->m_Text.IsEmpty()) ) {
				continue;
			}
            break;
        }

        if( NxtLibItem != NULL )
			NxtName =  NxtLibItem->GetField( VALUE )->m_Text;
        else
			NxtName = wxT ("");

        sprintf( RefName, "%s", aList[ii].m_Reference );
        ValName = DrawLibItem->GetField( VALUE )->m_Text ;

		if( !NxtName.CmpNoCase(ValName )) {
            qty++;
            strcat( RNames, ", " );
            strcat( RNames, RefName );
			continue;
        }

        fprintf( f, "%15s%c%3d", CONV_TO_UTF8(ValName), s_ExportSeparatorSymbol, qty );
        qty = 1;

        if( m_AddFootprintField->IsChecked() )
            fprintf( f, "%c%15s", s_ExportSeparatorSymbol,
                    CONV_TO_UTF8( DrawLibItem->GetField( FOOTPRINT )->m_Text ) );

        // for( int jj = FIELD1; jj < DrawLibItem->GetFieldCount(); jj++ )
        for( int jj = FIELD1; jj < FIELD5 ; jj++ )
        	fprintf( f, "%c%12s", s_ExportSeparatorSymbol, 
                    CONV_TO_UTF8( m_Cmp->GetField( jj )->m_Text ) );

        fprintf( f, "%c%s%s", s_ExportSeparatorSymbol, RefName, RNames );
        fputs( "\n", f );

        strcpy( RNames, "" );
        for( int jj = FOOTPRINT; jj < DrawLibItem->GetFieldCount(); jj++ )
			m_Cmp->GetField( jj )->m_Text = wxT ("");
    }

    return 0;
}


int DIALOG_BUILD_BOM::PrintComponentsListByVal(
    FILE*                          f,
    std::vector <OBJ_CMP_TO_LIST>& aList,
    bool                           aIncludeSubComponents )
{
    int             Multi;
    wxChar          Unit;
    EDA_BaseStruct* DrawList;
    SCH_COMPONENT*  DrawLibItem;
    LIB_COMPONENT*  Entry;
    char            CmpName[80];
    wxString        msg;

    msg = _( "\n#Cmp ( order = Value )" );

    if( aIncludeSubComponents )
        msg << _( " (with SubCmp)" );
    msg << wxT( "\n" );
    fputs( CONV_TO_UTF8( msg ), f );

    for( unsigned ii = 0; ii < aList.size(); ii++ )
    {
        DrawList = aList[ii].m_RootCmp;

        if( DrawList == NULL )
            continue;

        if( DrawList->Type() != TYPE_SCH_COMPONENT )
            continue;

        DrawLibItem = (SCH_COMPONENT*) DrawList;
        if( aList[ii].m_Reference[0] == '#' )
            continue;

        Multi = 0;
        Unit  = ' ';
        Entry = CMP_LIBRARY::FindLibraryComponent( DrawLibItem->m_ChipName );
        if( Entry )
            Multi = Entry->GetPartCount();

        if( ( Multi > 1 ) && aIncludeSubComponents )
        {
#if defined(KICAD_GOST)
            Unit = aList[ii].m_Unit + '1' - 1;
        }

        sprintf( CmpName, "%s.%c", aList[ii].m_Reference, Unit );
#else
            Unit = aList[ii].m_Unit + 'A' - 1;
        }

        sprintf( CmpName, "%s%c", aList[ii].m_Reference, Unit );
#endif
        fprintf( f, "| %-12s %-10s",
                 CONV_TO_UTF8( DrawLibItem->GetField( VALUE )->m_Text ),
                 CmpName );

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
                DrawTextItem->m_Text.GetData(),
                labeltype.GetData(),
                sheetpath.GetData(),
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
                DrawSheetLabel->m_Text.GetData(),
                labtype.GetData(),
                aList[ii].m_SheetPath.PathHumanReadable().GetData(),
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


/* This function will act just like the strcmp function but correctly sort
 * the numerical order in the string
 * return -1 if first string is less than the second
 * return 0 if the strings are equal
 * return 1 if the first string is greater than the second
 */
int RefDesStringCompare( const char* obj1, const char* obj2 )
{
    /* The strings we are going to compare */
    wxString strFWord;
    wxString strSWord;

    /* The different sections of the first string */
    wxString strFWordBeg, strFWordMid, strFWordEnd;

    /* The different sections of the second string */
    wxString strSWordBeg, strSWordMid, strSWordEnd;

    int isEqual = 0;            /* The numerical results of a string compare */
    int iReturn = 0;            /* The variable that is being returned */

    long lFirstDigit  = 0;      /* The converted middle section of the first
                                 *string */
    long lSecondDigit = 0;      /* The converted middle section of the second
                                 *string */

    /* Since m_Ref is a char * it is ASCII */
    strFWord = wxString::FromAscii( obj1 );
    strSWord = wxString::FromAscii( obj2 );

    /* Split the two string into separate parts */
    SplitString( strFWord, &strFWordBeg, &strFWordMid, &strFWordEnd );
    SplitString( strSWord, &strSWordBeg, &strSWordMid, &strSWordEnd );

    /* Compare the Beginning section of the strings */
    isEqual = strFWordBeg.CmpNoCase( strSWordBeg );
    if( isEqual > 0 )
        iReturn = 1;
    else if( isEqual < 0 )
        iReturn = -1;
    else
    {
        /* If the first sections are equal compare there digits */
        strFWordMid.ToLong( &lFirstDigit );
        strSWordMid.ToLong( &lSecondDigit );
        if( lFirstDigit > lSecondDigit )
            iReturn = 1;
        else if( lFirstDigit < lSecondDigit )
            iReturn = -1;
        else
        {
            /* If the first two sections are equal compare the endings */
            isEqual = strFWordEnd.CmpNoCase( strSWordEnd );
            if( isEqual > 0 )
                iReturn = 1;
            else if( isEqual < 0 )
                iReturn = -1;
            else
                iReturn = 0;
        }
    }

    return iReturn;
}


/* This is the function that breaks a string into three parts.
 * The alphabetic preamble
 * The numeric part
 * Any alphabetic ending
 * For example C10A is split to C 10 A
 */
int SplitString( wxString  strToSplit,
                 wxString* strBeginning,
                 wxString* strDigits,
                 wxString* strEnd )
{
    /* Clear all the return strings */
    strBeginning->Clear();
    strDigits->Clear();
    strEnd->Clear();

    /* There no need to do anything if the string is empty */
    if( strToSplit.length() == 0 )
        return 0;

    /* Starting at the end of the string look for the first digit */
    int ii;
    for( ii = (strToSplit.length() - 1); ii >= 0; ii-- )
    {
        if( isdigit( strToSplit[ii] ) )
            break;
    }

    /* If there were no digits then just set the single string */
    if( ii < 0 )
        *strBeginning = strToSplit;
    else
    {
        /* Since there is at least one digit this is the trailing string */
        *strEnd = strToSplit.substr( ii + 1 );

        /* Go to the end of the digits */
        int position = ii + 1;
        for( ; ii >= 0; ii-- )
        {
            if( !isdigit( strToSplit[ii] ) )
                break;
        }

        /* If all that was left was digits, then just set the digits string */
        if( ii < 0 )
            *strDigits = strToSplit.substr( 0, position );

        /* We were only looking for the last set of digits everything else is
         *part of the preamble */
        else
        {
            *strDigits    = strToSplit.substr( ii + 1, position - ii - 1 );
            *strBeginning = strToSplit.substr( 0, ii + 1 );
        }
    }

    return 0;
}
