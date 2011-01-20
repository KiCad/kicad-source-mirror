/////////////////////////////////////////////////////////////////////////////
// Name:        dialog_build_BOM.cpp
// Author:      jean-pierre Charras
// Modified by:
// Licence:     GPL
/////////////////////////////////////////////////////////////////////////////


#include "fctsys.h"
#include "appl_wxstruct.h"
#include "common.h"
#include "confirm.h"
#include "gestfich.h"
#include "kicad_string.h"
#include "class_sch_screen.h"
#include "wxstruct.h"
#include "build_version.h"

#include "general.h"
#include "netlist.h"
#include "template_fieldnames.h"
#include "sch_component.h"
#include "sch_sheet.h"
#include "class_library.h"

#include "wx/valgen.h"

#include "dialog_build_BOM.h"

#include "protos.h"


extern void GenListeGLabels( std::vector <LABEL_OBJECT>& aList );
extern bool SortLabelsByValue( const LABEL_OBJECT& obj1, const LABEL_OBJECT& obj2 );
extern bool SortLabelsBySheet( const LABEL_OBJECT& obj1, const LABEL_OBJECT& obj2 );
extern int  PrintListeGLabel( FILE* f, std::vector <LABEL_OBJECT>& aList );


/* Local variables */
static bool     s_ListByRef    = TRUE;
static bool     s_ListByValue  = TRUE;
static bool     s_ListWithSubCmponents;
static bool     s_ListHierarchicalPinByName;
static bool     s_ListBySheet;
static bool     s_BrowseCreatedList;
static int      s_OutputFormOpt;
static int      s_OutputSeparatorOpt;
static bool     s_Add_FpField_state;
static bool     s_Add_F1_state;
static bool     s_Add_F2_state;
static bool     s_Add_F3_state;
static bool     s_Add_F4_state;
static bool     s_Add_F5_state;
static bool     s_Add_F6_state;
static bool     s_Add_F7_state;
static bool     s_Add_F8_state;
static bool     s_Add_Alls_state;
static char     s_ExportSeparatorSymbol;


static bool*    s_AddFieldList[] =
{
    &s_Add_FpField_state,
    &s_Add_F1_state,
    &s_Add_F2_state,
    &s_Add_F3_state,
    &s_Add_F4_state,
    &s_Add_F5_state,
    &s_Add_F6_state,
    &s_Add_F7_state,
    &s_Add_F8_state,
    &s_Add_Alls_state,
    NULL
};


#define OPTION_BOM_FORMAT           wxT( "BomFormat" )
#define OPTION_BOM_LAUNCH_BROWSER   wxT( "BomLaunchBrowser" )
#define OPTION_BOM_SEPARATOR        wxT( "BomExportSeparator" )
#define OPTION_BOM_ADD_FIELD        wxT( "BomAddField" )

/* list of separators used in bom export to spreadsheet
 * (selected by s_OutputSeparatorOpt, and s_OutputSeparatorOpt radiobox)
 */
static char s_ExportSeparator[] = ("\t;,.");

/*!
 * DIALOG_BUILD_BOM dialog type definition
 */


DIALOG_BUILD_BOM::DIALOG_BUILD_BOM( WinEDA_DrawFrame* parent ) :
    DIALOG_BUILD_BOM_BASE( parent )
{
    m_Config = wxGetApp().m_EDA_Config;
    wxASSERT( m_Config != NULL );

    m_Parent = parent;

    Init();

    if( GetSizer() )
    {
        GetSizer()->SetSizeHints( this );
    }
    Centre();
}


/*!
 * Init Controls for DIALOG_BUILD_BOM
 */

void DIALOG_BUILD_BOM::Init()
{
    SetFocus();

    /* Get options */
    s_OutputFormOpt        = m_Config->Read( OPTION_BOM_FORMAT, (long) 0 );
    s_BrowseCreatedList    = m_Config->Read( OPTION_BOM_LAUNCH_BROWSER, (long) 0 );
    s_OutputSeparatorOpt   = m_Config->Read( OPTION_BOM_SEPARATOR, (long) 0 );
    long addfields = m_Config->Read( OPTION_BOM_ADD_FIELD, (long) 0 );

    for( int ii = 0, bitmask = 1; s_AddFieldList[ii] != NULL; ii++ )
    {
        if( (addfields & bitmask) )
            *s_AddFieldList[ii] = true;
        else
            *s_AddFieldList[ii] = false;

        bitmask <<= 1;
    }

    // Set validators
    m_ListCmpbyRefItems->SetValidator( wxGenericValidator( &s_ListByRef ) );
    m_ListSubCmpItems->SetValidator( wxGenericValidator( &s_ListWithSubCmponents ) );
    m_ListCmpbyValItems->SetValidator( wxGenericValidator( &s_ListByValue ) );
    m_GenListLabelsbyVal->SetValidator( wxGenericValidator( &s_ListHierarchicalPinByName ) );
    m_GenListLabelsbySheet->SetValidator( wxGenericValidator( &s_ListBySheet ) );
    m_OutputFormCtrl->SetValidator( wxGenericValidator( &s_OutputFormOpt ) );
    m_OutputSeparatorCtrl->SetValidator( wxGenericValidator( &s_OutputSeparatorOpt ) );
    m_GetListBrowser->SetValidator( wxGenericValidator( &s_BrowseCreatedList ) );
    m_AddFootprintField->SetValidator( wxGenericValidator( &s_Add_FpField_state ) );
    m_AddField1->SetValidator( wxGenericValidator( &s_Add_F1_state ) );
    m_AddField2->SetValidator( wxGenericValidator( &s_Add_F2_state ) );
    m_AddField3->SetValidator( wxGenericValidator( &s_Add_F3_state ) );
    m_AddField4->SetValidator( wxGenericValidator( &s_Add_F4_state ) );
    m_AddField5->SetValidator( wxGenericValidator( &s_Add_F5_state ) );
    m_AddField6->SetValidator( wxGenericValidator( &s_Add_F6_state ) );
    m_AddField7->SetValidator( wxGenericValidator( &s_Add_F7_state ) );
    m_AddField8->SetValidator( wxGenericValidator( &s_Add_F8_state ) );
    m_AddAllFields->SetValidator( wxGenericValidator( &s_Add_Alls_state ) );

    m_OutputFormCtrl->SetSelection( s_OutputFormOpt );
    m_OutputSeparatorCtrl->SetSelection( s_OutputSeparatorOpt );

    // Enable/disable options:
    wxCommandEvent dummy;
    OnRadioboxSelectFormatSelected( dummy );
}


/*!
 * wxEVT_COMMAND_RADIOBOX_SELECTED event handler for ID_RADIOBOX_SELECT_FORMAT
 */

void DIALOG_BUILD_BOM::OnRadioboxSelectFormatSelected( wxCommandEvent& event )
{
    if( m_OutputFormCtrl->GetSelection() == 0 )
    {
        m_OutputSeparatorCtrl->Enable( false );
        m_ListCmpbyValItems->Enable( true );
        m_GenListLabelsbyVal->Enable( true );
        m_GenListLabelsbySheet->Enable( true );
    }
    else
    {
        m_OutputSeparatorCtrl->Enable( true );
        m_ListCmpbyValItems->Enable( false );
        m_GenListLabelsbyVal->Enable( false );
        m_GenListLabelsbySheet->Enable( false );
    }
}


/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_OK
 */

void DIALOG_BUILD_BOM::OnOkClick( wxCommandEvent& event )
{
    char ExportSeparatorSymbol = s_ExportSeparator[0];

    if( m_OutputSeparatorCtrl->GetSelection() > 0 )
        ExportSeparatorSymbol = s_ExportSeparator[m_OutputSeparatorCtrl->GetSelection()];

    int ExportFileType = m_OutputFormCtrl->GetSelection();

    SavePreferences();

    Create_BOM_Lists( ExportFileType, m_ListSubCmpItems->GetValue(),
                      ExportSeparatorSymbol, m_GetListBrowser->GetValue() );
}


/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_CANCEL
 */

void DIALOG_BUILD_BOM::OnCancelClick( wxCommandEvent& event )
{
    EndModal( -1 );
}


void DIALOG_BUILD_BOM::SavePreferences()
{
    wxASSERT( m_Config != NULL );

    // Determine current settings of "List items" and "Options" checkboxes
    // (NOTE: These 6 settings are restored when the dialog box is next
    // invoked, but are *not* still saved after EESchema is next shut down.)
    s_ListByRef = m_ListCmpbyRefItems->GetValue();
    s_ListWithSubCmponents = m_ListSubCmpItems->GetValue();
    s_ListByValue = m_ListCmpbyValItems->GetValue();
    s_ListHierarchicalPinByName = m_GenListLabelsbyVal->GetValue();
    s_ListBySheet = m_GenListLabelsbySheet->GetValue();
    s_BrowseCreatedList = m_GetListBrowser->GetValue();

    // (saved in config ):

    // Determine current settings of both radiobutton groups
    s_OutputFormOpt = m_OutputFormCtrl->GetSelection();
    s_OutputSeparatorOpt = m_OutputSeparatorCtrl->GetSelection();

    if( s_OutputSeparatorOpt < 0 )
        s_OutputSeparatorOpt = 0;

    // Determine current settings of all "Fields to add" checkboxes
    s_Add_FpField_state = m_AddFootprintField->GetValue();
    s_Add_F1_state     = m_AddField1->GetValue();
    s_Add_F2_state     = m_AddField2->GetValue();
    s_Add_F3_state     = m_AddField3->GetValue();
    s_Add_F4_state     = m_AddField4->GetValue();
    s_Add_F5_state     = m_AddField5->GetValue();
    s_Add_F6_state     = m_AddField6->GetValue();
    s_Add_F7_state     = m_AddField7->GetValue();
    s_Add_F8_state     = m_AddField8->GetValue();
    s_Add_Alls_state   = m_AddAllFields->GetValue();

    // Now save current settings of both radiobutton groups
    m_Config->Write( OPTION_BOM_FORMAT, (long) s_OutputFormOpt );
    m_Config->Write( OPTION_BOM_SEPARATOR, (long) s_OutputSeparatorOpt );
    m_Config->Write( OPTION_BOM_LAUNCH_BROWSER, (long) s_BrowseCreatedList );

    // Now save current settings of all "Fields to add" checkboxes
    long addfields = 0;

    for( int ii = 0, bitmask = 1; s_AddFieldList[ii] != NULL; ii++ )
    {
        if( *s_AddFieldList[ii] )
            addfields |= bitmask;

        bitmask <<= 1;
    }

    m_Config->Write( OPTION_BOM_ADD_FIELD, addfields );
}


void DIALOG_BUILD_BOM::Create_BOM_Lists( int  aTypeFile,
                                         bool aIncludeSubComponents,
                                         char aExportSeparatorSymbol,
                                         bool aRunBrowser )
{
    wxString    wildcard;

    static wxFileName fn;

    wxFileName  current = g_RootSheet->GetScreen()->GetFileName();

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

    case 1: // spreadsheet
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

    SCH_REFERENCE_LIST cmplist;
    SCH_SHEET_LIST sheetList;              // uses a global

    sheetList.GetComponents( cmplist, false );

    // sort component list by ref and remove sub components
    if( !aIncludeSubComponents )
        cmplist.RemoveSubComponentsFromList();

    // sort component list by value
    cmplist.SortByValueOnly( );
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

    SCH_REFERENCE_LIST cmplist;
    SCH_SHEET_LIST sheetList;              // uses a global

    sheetList.GetComponents( cmplist, false );

    // sort component list
    cmplist.SortByReferenceOnly( );

    if( !aIncludeSubComponents )
        cmplist.RemoveSubComponentsFromList();

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

    SCH_REFERENCE_LIST cmplist;
    SCH_SHEET_LIST sheetList;

    sheetList.GetComponents( cmplist, false );

    itemCount = cmplist.GetCount();

    if( itemCount )
    {
        // creates the list file
        DateAndTime( Line );

        wxString Title = wxGetApp().GetAppName() + wxT( " " ) + GetBuildVersion();

        fprintf( f, "%s  >> Creation date: %s\n", CONV_TO_UTF8( Title ), Line );

        // sort component list
        cmplist.SortByReferenceOnly();

        if( !aIncludeSubComponents )
            cmplist.RemoveSubComponentsFromList();

        if( m_ListCmpbyRefItems->GetValue() )
            PrintComponentsListByRef( f, cmplist, false, aIncludeSubComponents );

        if( m_ListCmpbyValItems->GetValue() )
        {
            cmplist.SortByValueOnly();
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

            msg.Printf( _( "\n#Global, Hierarchical Labels and PinSheets \
( order = Sheet Number ) count = %d\n" ),
                        itemCount );

            fprintf( f, "%s", CONV_TO_UTF8( msg ) );
            PrintListeGLabel( f, listOfLabels );
        }

        if( m_GenListLabelsbyVal->GetValue() )
        {
            sort( listOfLabels.begin(), listOfLabels.end(), SortLabelsByValue );

            msg.Printf( _( "\n#Global, Hierarchical Labels and PinSheets ( \
order = Alphab. ) count = %d\n\n" ),
                        itemCount );

            fprintf( f, "%s", CONV_TO_UTF8( msg ) );
            PrintListeGLabel( f, listOfLabels );
        }
    }

    msg = _( "\n#End List\n" );
    fprintf( f, "%s", CONV_TO_UTF8( msg ) );
    fclose( f );
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
int DIALOG_BUILD_BOM::PrintComponentsListByRef( FILE*                    f,
                                                SCH_REFERENCE_LIST& aList,
                                                bool                     CompactForm,
                                                bool                     aIncludeSubComponents )
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

            fprintf( f, "%c%s%d", s_ExportSeparatorSymbol, CONV_TO_UTF8( msg ), ii - FIELD1 + 1 );
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
    for( unsigned ii = 0; ii < aList.GetCount(); ii++ )
    {
        EDA_ITEM* item = aList[ii].GetComponent();

        if( item == NULL )
            continue;

        if( item->Type() != SCH_COMPONENT_T )
            continue;

        SCH_COMPONENT*  comp = (SCH_COMPONENT*) item;

        bool isMulti = false;

        LIB_COMPONENT*  entry = CMP_LIBRARY::FindLibraryComponent( comp->GetLibName() );
        if( entry )
            isMulti = entry->IsMulti();

        if( isMulti && aIncludeSubComponents )
            subRef = LIB_COMPONENT::ReturnSubReference( aList[ii].GetUnit() );
        else
            subRef.Empty();

        CmpName = aList[ii].GetRefStr();

        if( !CompactForm )
            CmpName += CONV_TO_UTF8(subRef);

        if( CompactForm )
#if defined(KICAD_GOST)
            fprintf( f, "%s%c%s%c%s", CmpName.c_str(), s_ExportSeparatorSymbol,
                     CONV_TO_UTF8( comp->GetField( VALUE )->m_Text ), s_ExportSeparatorSymbol,
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
            msg = aList[ii].GetSheetPath().PathHumanReadable();
            BASE_SCREEN * screen = (BASE_SCREEN*) comp->GetParent();

            if( screen )
            {
                if( CompactForm )
                {
                    fprintf( f, "%c%s", s_ExportSeparatorSymbol, CONV_TO_UTF8( msg ) );
                    msg = m_Parent->GetXYSheetReferences( screen, comp->m_Pos );
                    fprintf( f, "%c%s)", s_ExportSeparatorSymbol,
                             CONV_TO_UTF8( msg ) );
                }
                else
                {
                    fprintf( f, "   (Sheet %s)", CONV_TO_UTF8( msg ) );
                    msg = m_Parent->GetXYSheetReferences( screen, comp->m_Pos );
                    fprintf( f, "   (loc %s)", CONV_TO_UTF8( msg ) );
                }
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
 *  rating, etc.  Also useful if the following fields
 *  are edited:
 *   FIELD1 - manufacture
 *   FIELD2 - manufacture part number
 *   FIELD3 - distributor part number
 */
int DIALOG_BUILD_BOM::PrintComponentsListByPart( FILE* f, SCH_REFERENCE_LIST& aList,
                                                 bool aIncludeSubComponents )
{
    int             qty = 0;
    wxString        refName;
    wxString        fullRefName;        // reference + part Id (for multiple parts per package
    wxString        valName;
#if defined(KICAD_GOST)
    wxString        footName;
    wxString        datsName;
#endif
    wxString        refNames;
    wxString        lastRef;
    wxString        unitId;
    SCH_COMPONENT*  currCmp;
    SCH_COMPONENT*  nextCmp;
    SCH_COMPONENT   dummyCmp;           // A dummy component, to store fields

    for( unsigned ii = 0; ii < aList.GetCount(); ii++ )
    {
        currCmp = aList[ii].GetComponent();

        if( ii < aList.GetCount() -1 )
            nextCmp = aList[ii+1].GetComponent();
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
#if defined(KICAD_GOST)
        footName = currCmp->GetField( FOOTPRINT )->m_Text;
        datsName = currCmp->GetField( DATASHEET )->m_Text;
#endif

        int multi = 0;

        if( aIncludeSubComponents )
        {
            LIB_COMPONENT* entry = CMP_LIBRARY::FindLibraryComponent( currCmp->GetLibName() );

            if( entry )
                multi = entry->GetPartCount();

            if ( multi <= 1 )
                multi = 0;
        }

        if ( multi && aList[ii].GetUnit() > 0 )
            unitId.Printf( wxT("%c"), 'A' -1 + aList[ii].GetUnit() );
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

        // if the next component has same value the line will be printed after.
#if defined(KICAD_GOST)
        if( nextCmp && nextCmp->GetField( VALUE )->m_Text.CmpNoCase( valName ) == 0
            && nextCmp->GetField( FOOTPRINT )->m_Text.CmpNoCase( footName ) == 0
            && nextCmp->GetField( DATASHEET )->m_Text.CmpNoCase( datsName ) == 0 )
#else
        if( nextCmp && nextCmp->GetField( VALUE )->m_Text.CmpNoCase( valName ) == 0 )
#endif
            continue;

       // Print line for the current component value:
        fprintf( f, "%15s%c%3d", CONV_TO_UTF8( valName ), s_ExportSeparatorSymbol, qty );

        if( IsFieldChecked(FOOTPRINT ) )
            fprintf( f, "%c%15s", s_ExportSeparatorSymbol,
#if defined(KICAD_GOST)
                     CONV_TO_UTF8( footName ) );
#else
                     CONV_TO_UTF8( currCmp->GetField( FOOTPRINT )->m_Text ) );
#endif

#if defined(KICAD_GOST)
            fprintf( f, "%c%20s", s_ExportSeparatorSymbol,CONV_TO_UTF8( datsName ) );
#endif

        // wrap the field in quotes, since it has commas in it.
        fprintf( f, "%c\"%s\"", s_ExportSeparatorSymbol, CONV_TO_UTF8( refNames ) );

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


int DIALOG_BUILD_BOM::PrintComponentsListByVal( FILE*               f,
                                                SCH_REFERENCE_LIST& aList,
                                                bool                aIncludeSubComponents )
{
    EDA_ITEM*      schItem;
    SCH_COMPONENT* DrawLibItem;
    LIB_COMPONENT* entry;
    std::string    CmpName;
    wxString       msg;

    msg = _( "\n#Cmp ( order = Value )" );

    if( aIncludeSubComponents )
        msg << _( " (with SubCmp)" );

    msg << wxT( "\n" );

    fputs( CONV_TO_UTF8( msg ), f );

    for( unsigned ii = 0; ii < aList.GetCount(); ii++ )
    {
        schItem = aList[ii].GetComponent();

        if( schItem == NULL )
            continue;

        if( schItem->Type() != SCH_COMPONENT_T )
            continue;

        DrawLibItem = (SCH_COMPONENT*) schItem;

        bool isMulti = false;
        entry = CMP_LIBRARY::FindLibraryComponent( DrawLibItem->GetLibName() );

        if( entry )
            isMulti = entry->IsMulti();

        wxString subRef;

        if( isMulti && aIncludeSubComponents )
            subRef = LIB_COMPONENT::ReturnSubReference( aList[ii].GetUnit() );
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
            BASE_SCREEN * screen = (BASE_SCREEN*) DrawLibItem->GetParent();
            if( screen )
            {
                msg = aList[ii].GetSheetPath().PathHumanReadable();
                fprintf( f, "   (Sheet %s)", CONV_TO_UTF8( msg ) );
                msg = m_Parent->GetXYSheetReferences( screen, DrawLibItem->m_Pos );
                fprintf( f, "   (loc %s)", CONV_TO_UTF8( msg ) );
            }
        }

        PrintFieldData( f, DrawLibItem );

        fputs( "\n", f );
    }

    msg = _( "#End Cmp\n" );
    fputs( CONV_TO_UTF8( msg ), f );
    return 0;
}
