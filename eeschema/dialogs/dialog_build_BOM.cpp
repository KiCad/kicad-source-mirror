/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2008 Jean-Pierre Charras, jaen-pierre.charras@gipsa-lab.inpg.com
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
 * @file dialog_build_BOM.cpp
 */

#include <fctsys.h>
#include <appl_wxstruct.h>
#include <confirm.h>
#include <gestfich.h>
#include <kicad_string.h>
#include <class_sch_screen.h>
#include <wxstruct.h>
#include <build_version.h>

#include <general.h>
#include <netlist.h>
#include <template_fieldnames.h>
#include <sch_component.h>
#include <sch_sheet.h>
#include <class_library.h>

#include <wx/valgen.h>

#include <dialog_build_BOM.h>

#include <protos.h>


extern void GenListeGLabels( std::vector <BOM_LABEL>& aList );
extern bool SortLabelsByValue( const BOM_LABEL& obj1, const BOM_LABEL& obj2 );
extern bool SortLabelsBySheet( const BOM_LABEL& obj1, const BOM_LABEL& obj2 );
extern int  PrintListeGLabel( FILE* f, std::vector <BOM_LABEL>& aList );


/* Local variables */
static bool     s_ListByRef    = true;
static bool     s_ListByValue  = true;
static bool     s_ListWithSubCmponents;
static bool     s_ListHierarchicalPinByName;
static bool     s_ListHierarchicalPinBySheet;
static bool     s_BrowseCreatedList;
static int      s_OutputFormOpt;
static int      s_OutputSeparatorOpt;
static bool     s_Add_FpField_state = true;
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


const wxString OPTION_BOM_LIST_REF( wxT("BomListPerRef") );
const wxString OPTION_BOM_LIST_VALUE( wxT("BomListPerValue") );
const wxString OPTION_BOM_LIST_HPINS( wxT("BomListPerHPins") );
const wxString OPTION_BOM_LIST_HPINS_BY_SHEET( wxT("BomListHPinsPerSheet") );
const wxString OPTION_BOM_LIST_HPINS_BY_NAME_( wxT("BomListHPinsPerName") );
const wxString OPTION_BOM_LIST_SUB_CMP( wxT("BomListSubCmps") );

const wxString OPTION_BOM_FORMAT( wxT("BomFormat") );
const wxString OPTION_BOM_LAUNCH_BROWSER( wxT("BomLaunchBrowser") );
const wxString OPTION_BOM_SEPARATOR( wxT("BomExportSeparator") );
const wxString OPTION_BOM_ADD_FIELD ( wxT("BomAddField") );

/* list of separators used in bom export to spreadsheet
 * (selected by s_OutputSeparatorOpt, and s_OutputSeparatorOpt radiobox)
 */
static char s_ExportSeparator[] = ("\t;,.");

/*!
 * DIALOG_BUILD_BOM dialog type definition
 */


DIALOG_BUILD_BOM::DIALOG_BUILD_BOM( EDA_DRAW_FRAME* parent ) :
    DIALOG_BUILD_BOM_BASE( parent )
{
    m_Config = wxGetApp().GetSettings();
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
    m_Config->Read( OPTION_BOM_LIST_REF, &s_ListByRef );
    m_Config->Read( OPTION_BOM_LIST_VALUE , &s_ListByValue );
    m_Config->Read( OPTION_BOM_LIST_HPINS, &s_ListHierarchicalPinByName );
    m_Config->Read( OPTION_BOM_LIST_HPINS_BY_SHEET, &s_ListWithSubCmponents );
    m_Config->Read( OPTION_BOM_LIST_HPINS_BY_NAME_, &s_ListWithSubCmponents );
    m_Config->Read( OPTION_BOM_LIST_SUB_CMP, &s_ListWithSubCmponents );
    m_Config->Read( OPTION_BOM_LIST_HPINS_BY_SHEET, &s_ListHierarchicalPinBySheet );
    m_Config->Read( OPTION_BOM_LIST_HPINS_BY_NAME_, &s_ListHierarchicalPinByName );
    s_OutputFormOpt        = m_Config->Read( OPTION_BOM_FORMAT, (long) 0 );
    m_Config->Read( OPTION_BOM_LAUNCH_BROWSER, &s_BrowseCreatedList );
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
    m_GenListLabelsbySheet->SetValidator( wxGenericValidator( &s_ListHierarchicalPinBySheet ) );
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
    switch( m_OutputFormCtrl->GetSelection() )
    {
        case 0:
            m_OutputSeparatorCtrl->Enable( false );
            m_ListCmpbyRefItems->Enable( true );
            m_ListCmpbyValItems->Enable( true );
            m_GenListLabelsbyVal->Enable( true );
            m_GenListLabelsbySheet->Enable( true );
            m_ListSubCmpItems->Enable( true );
            break;

        case 1:
            m_OutputSeparatorCtrl->Enable( true );
            m_ListCmpbyRefItems->Enable( false );
            m_ListCmpbyValItems->Enable( false );
            m_GenListLabelsbyVal->Enable( false );
            m_GenListLabelsbySheet->Enable( false );
            m_ListSubCmpItems->Enable( true );
            break;

        case 2:
            m_OutputSeparatorCtrl->Enable( true );
            m_ListCmpbyRefItems->Enable( false );
            m_ListCmpbyValItems->Enable( false );
            m_GenListLabelsbyVal->Enable( false );
            m_GenListLabelsbySheet->Enable( false );
            m_ListSubCmpItems->Enable( false );
            break;
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
    s_ListByRef = m_ListCmpbyRefItems->GetValue();
    s_ListWithSubCmponents = m_ListSubCmpItems->GetValue();
    s_ListByValue = m_ListCmpbyValItems->GetValue();
    s_ListHierarchicalPinByName = m_GenListLabelsbyVal->GetValue();
    s_ListHierarchicalPinBySheet = m_GenListLabelsbySheet->GetValue();
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
    m_Config->Write( OPTION_BOM_LIST_REF, s_ListByRef );
    m_Config->Write( OPTION_BOM_LIST_VALUE , s_ListByValue );
    m_Config->Write( OPTION_BOM_LIST_HPINS, s_ListHierarchicalPinByName );
    m_Config->Write( OPTION_BOM_LIST_HPINS_BY_SHEET, s_ListHierarchicalPinBySheet );
    m_Config->Write( OPTION_BOM_LIST_HPINS_BY_NAME_, s_ListHierarchicalPinByName );
    m_Config->Write( OPTION_BOM_LIST_SUB_CMP, s_ListWithSubCmponents );

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
        GenereListeOfItems( aIncludeSubComponents );
        break;

    case 1: // spreadsheet, Single Part per line
        CreateExportList( aIncludeSubComponents );
        break;

    case 2: // spreadsheet, one value per line and no sub-component
        CreatePartsList();
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


/* Prints a list of components, in a form which can be imported by a spreadsheet.
 * components having the same value and the same footprint
 * are grouped on the same line
 * Form is:
 *  value; number of components; list of references; <footprint>; <field1>; ...;
 * list is sorted by values
 */
void DIALOG_BUILD_BOM::CreatePartsList( )
{
    FILE*    f;
    wxString msg;

    if( ( f = wxFopen( m_ListFileName, wxT( "wt" ) ) ) == NULL )
    {
        msg = _( "Failed to open file " );
        msg << m_ListFileName;
        DisplayError( this, msg );
        return;
    }

    SCH_REFERENCE_LIST cmplist;
    SCH_SHEET_LIST sheetList;

    sheetList.GetComponents( cmplist, false );

    // sort component list by ref and remove sub components
    cmplist.RemoveSubComponentsFromList();

    // sort component list by value
    cmplist.SortByValueOnly( );
    PrintComponentsListByPart( f, cmplist, false );

    fclose( f );
}


/*
 * Print a list of components, in a form which can be imported by a spreadsheet
 * form is:
 * cmp ref; cmp val; fields;
 * Components are sorted by reference
 */
void DIALOG_BUILD_BOM::CreateExportList( bool aIncludeSubComponents )
{
    FILE*    f;
    wxString msg;

    if( ( f = wxFopen( m_ListFileName, wxT( "wt" ) ) ) == NULL )
    {
        msg = _( "Failed to open file " );
        msg << m_ListFileName;
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
    PrintComponentsListByRef( f, cmplist, true, aIncludeSubComponents );

    fclose( f );
}


/*
 * GenereListeOfItems()
 * Main function to create the list of components and/or labels
 * (global labels and pin sheets" )
 */
void DIALOG_BUILD_BOM::GenereListeOfItems( bool aIncludeSubComponents )
{
    FILE*    f;
    int      itemCount;
    wxString msg;

    if( ( f = wxFopen( m_ListFileName, wxT( "wt" ) ) ) == NULL )
    {
        msg = _( "Failed to open file " );
        msg << m_ListFileName;
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
        wxString Title = wxGetApp().GetAppName() + wxT( " " ) + GetBuildVersion();

        fprintf( f, "%s  >> Creation date: %s\n", TO_UTF8( Title ), TO_UTF8( DateAndTime() ) );

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
    std::vector <BOM_LABEL> listOfLabels;

    GenListeGLabels( listOfLabels );

    if( ( itemCount = listOfLabels.size() ) > 0 )
    {
        if( m_GenListLabelsbySheet->GetValue() )
        {
            sort( listOfLabels.begin(), listOfLabels.end(), SortLabelsBySheet );

            msg.Printf( _( "\n#Global, Hierarchical Labels and PinSheets \
( order = Sheet Number ) count = %d\n" ),
                        itemCount );

            fprintf( f, "%s", TO_UTF8( msg ) );
            PrintListeGLabel( f, listOfLabels );
        }

        if( m_GenListLabelsbyVal->GetValue() )
        {
            sort( listOfLabels.begin(), listOfLabels.end(), SortLabelsByValue );

            msg.Printf( _( "\n#Global, Hierarchical Labels and PinSheets ( \
order = Alphab. ) count = %d\n\n" ),
                        itemCount );

            fprintf( f, "%s", TO_UTF8( msg ) );
            PrintListeGLabel( f, listOfLabels );
        }
    }

    msg = _( "\n#End List\n" );
    fprintf( f, "%s", TO_UTF8( msg ) );
    fclose( f );
}

wxString DIALOG_BUILD_BOM::PrintFieldData( SCH_COMPONENT* DrawLibItem,
                                       bool CompactForm )
{
    wxString outStr;
    wxString tmpStr;

    if( IsFieldChecked( FOOTPRINT ) )
    {
        if( CompactForm )
        {
            outStr.Printf( wxT( "%c%s" ), s_ExportSeparatorSymbol,
                           GetChars( DrawLibItem->GetField( FOOTPRINT )->m_Text ) );
        }
        else
        {
            outStr.Printf( wxT( "; %-12s" ),
                           GetChars( DrawLibItem->GetField( FOOTPRINT )->m_Text ) );
        }
    }

    for(int ii = FIELD1; ii < DrawLibItem->GetFieldCount(); ii++ )
    {
        if( ! IsFieldChecked( ii ) )
            continue;

        if( CompactForm )
        {
            tmpStr.Printf( wxT( "%c%s" ), s_ExportSeparatorSymbol,
                           GetChars( DrawLibItem->GetField( ii )->m_Text ) );
            outStr += tmpStr;
        }
        else
        {
            tmpStr.Printf( wxT( "; %-12s" ),
                           GetChars( DrawLibItem->GetField( ii )->m_Text ) );
            outStr += tmpStr;
        }
    }
    return outStr;
}


/* Print the B.O.M sorted by reference
 */
int DIALOG_BUILD_BOM::PrintComponentsListByRef( FILE*               f,
                                                SCH_REFERENCE_LIST& aList,
                                                bool                CompactForm,
                                                bool                aIncludeSubComponents )
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

            fprintf( f, "%c%s%d", s_ExportSeparatorSymbol, TO_UTF8( msg ), ii - FIELD1 + 1 );
        }

        fprintf( f, "\n" );
    }
    else
    {
        msg = _( "\n#Cmp ( order = Reference )" );

        if( aIncludeSubComponents )
            msg << _( " (with SubCmp)" );

        fprintf( f, "%s\n", TO_UTF8( msg ) );
    }

    std::string     CmpName;
    wxString        subRef;

#if defined(KICAD_GOST)
    wxString        strCur;
    wxString        strPred;
    int             amount = 0;
    std::string     CmpNameFirst;
    std::string     CmpNameLast;
#endif

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
            CmpName += TO_UTF8(subRef);

        if( CompactForm )
#if defined(KICAD_GOST)
            strCur.Printf( wxT( "%c%s%c%s" ), s_ExportSeparatorSymbol,
                           GetChars( comp->GetField( VALUE )->m_Text ), s_ExportSeparatorSymbol,
                           GetChars( comp->GetField( DATASHEET )->m_Text ) );
#else
            fprintf( f, "%s%c%s", CmpName.c_str(), s_ExportSeparatorSymbol,
                     TO_UTF8( comp->GetField( VALUE )->m_Text ) );
#endif

        else
#if defined(KICAD_GOST)
            fprintf( f, "| %-10s %-12s %-20s", CmpName.c_str(),
                     TO_UTF8( comp->GetField( VALUE )->m_Text ),
                     TO_UTF8( comp->GetField( DATASHEET )->m_Text ) );
#else
            fprintf( f, "| %-10s %-12s", CmpName.c_str(),
                     TO_UTF8( comp->GetField( VALUE )->m_Text ) );
#endif

        if( aIncludeSubComponents )
        {
            msg = aList[ii].GetSheetPath().PathHumanReadable();
            BASE_SCREEN * screen = (BASE_SCREEN*) comp->GetParent();

            if( screen )
            {
                if( CompactForm )
                {
#if defined(KICAD_GOST)
                    strCur.Printf( wxT( "%c%s" ), s_ExportSeparatorSymbol, GetChars( msg ) );
                    msg = m_Parent->GetXYSheetReferences( comp->GetPosition() );
                    strCur.Printf( wxT( "%c%s)" ), s_ExportSeparatorSymbol, GetChars( msg ) );
#else
                    fprintf( f, "%c%s", s_ExportSeparatorSymbol, TO_UTF8( msg ) );
                    msg = m_Parent->GetXYSheetReferences( comp->GetPosition() );
                    fprintf( f, "%c%s)", s_ExportSeparatorSymbol,
                             TO_UTF8( msg ) );
#endif
                }
                else
                {
                    fprintf( f, "   (Sheet %s)", TO_UTF8( msg ) );
                    msg = m_Parent->GetXYSheetReferences( comp->GetPosition() );
                    fprintf( f, "   (loc %s)", TO_UTF8( msg ) );
                }
            }
        }

#if defined(KICAD_GOST)
        wxString tmpStr = PrintFieldData( comp, CompactForm );
        strCur += tmpStr;

        if ( CompactForm )
        {
            if ( strPred.Len() == 0 )
            {
                CmpNameFirst = CmpName;
            }
            else
            {
                if ( !strCur.IsSameAs(strPred) )
                {
                    switch (amount)
                    {
                    case 1:
                        fprintf( f, "%s%s%c%d\n", CmpNameFirst.c_str(), TO_UTF8( strPred ),
                                 s_ExportSeparatorSymbol, amount );
                        break;

                    case 2:
                        fprintf( f, "%s,%s%s%c%d\n", CmpNameFirst.c_str(), CmpNameLast.c_str(),
                                 TO_UTF8(strPred), s_ExportSeparatorSymbol, amount );
                        break;

                    default:
                        fprintf( f, "%s..%s%s%c%d\n", CmpNameFirst.c_str(), CmpNameLast.c_str(),
                                 TO_UTF8( strPred ), s_ExportSeparatorSymbol, amount );
                        break;
                    }
                    CmpNameFirst = CmpName;
                    amount = 0;
                }
            }
            strPred = strCur;
            CmpNameLast = CmpName;
            amount++;
        }
        else
        {
            fprintf( f, "%s", TO_UTF8( tmpStr ) );
            fprintf( f, "\n" );
        }
#else
        wxString tmpStr = PrintFieldData( comp, CompactForm );
        fprintf( f, "%s\n", TO_UTF8( tmpStr ) );
#endif

    }

    if( !CompactForm )
    {
        msg = _( "#End Cmp\n" );
        fputs( TO_UTF8( msg ), f );
    }

#if defined(KICAD_GOST)
    else
    {
        switch (amount)
        {
        case 1:
            fprintf( f, "%s%s%c%d\n", CmpNameFirst.c_str(), TO_UTF8( strPred ),
                     s_ExportSeparatorSymbol, amount );
            break;

        case 2:
            fprintf( f, "%s,%s%s%c%d\n", CmpNameFirst.c_str(), CmpNameLast.c_str(),
                     TO_UTF8( strPred ), s_ExportSeparatorSymbol, amount );
            break;

        default:
            fprintf( f, "%s..%s%s%c%d\n", CmpNameFirst.c_str(), CmpNameLast.c_str(),
                     TO_UTF8( strPred ), s_ExportSeparatorSymbol, amount );
            break;
        }
    }
#endif

    return 0;
}


int DIALOG_BUILD_BOM::PrintComponentsListByPart( FILE* aFile, SCH_REFERENCE_LIST& aList,
                                                 bool aIncludeSubComponents )
{
    unsigned int index = 0;
    while( index < aList.GetCount() )
    {
        SCH_COMPONENT *component = aList[index].GetComponent();
        wxArrayString referenceStrList;
        int qty = 1;
        referenceStrList.Add( aList[index].GetRef() );
        for( unsigned int i = index+1; i < aList.GetCount(); )
        {
            if( *(aList[i].GetComponent()) == *component )
            {
                referenceStrList.Add( aList[i].GetRef() );
                aList.RemoveItem( i );
                qty++;
            }
            else
                i++; // Increment index only when current item is not removed from the list
        }

        referenceStrList.Sort( RefDesStringCompare ); // Sort references for this component

        // Write value, quantity
        fprintf( aFile, "%15s%c%3d", TO_UTF8( component->GetField( VALUE )->GetText() ),
                 s_ExportSeparatorSymbol, qty );

        // Write list of references
        for( int i = 0; i < referenceStrList.Count(); i++ )
        {
            if( i == 0 )
                fprintf( aFile, "%c\"%s", s_ExportSeparatorSymbol, TO_UTF8( referenceStrList[i] ) );
            else
                fprintf( aFile, " %s", TO_UTF8( referenceStrList[i] ) );
        }
        if( referenceStrList.Count() )
            fprintf( aFile, "\"" );

        // Write the rest of the fields if required
#if defined( KICAD_GOST )
        fprintf( aFile, "%c%20s", s_ExportSeparatorSymbol,
                 TO_UTF8( component->GetField( DATASHEET )->GetText() ) );
#endif
        for( int i = FOOTPRINT; i < component->GetFieldCount(); i++ )
            if( IsFieldChecked( i ) )
                fprintf( aFile, "%c%15s", s_ExportSeparatorSymbol,
                         TO_UTF8( component->GetField( i )->GetText() ) );
        fprintf( aFile, "\n" );
        index++;
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

    fputs( TO_UTF8( msg ), f );

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
        CmpName += TO_UTF8(subRef);

        fprintf( f, "| %-12s %-10s",
                 TO_UTF8( DrawLibItem->GetField( VALUE )->m_Text ),
                 CmpName.c_str() );

        // print the sheet path
        if( aIncludeSubComponents )
        {
            BASE_SCREEN * screen = (BASE_SCREEN*) DrawLibItem->GetParent();

            if( screen )
            {
                msg = aList[ii].GetSheetPath().PathHumanReadable();
                fprintf( f, "   (Sheet %s)", TO_UTF8( msg ) );
                msg = m_Parent->GetXYSheetReferences( DrawLibItem->GetPosition() );
                fprintf( f, "   (loc %s)", TO_UTF8( msg ) );
            }
        }

        fprintf( f, "%s\n", TO_UTF8( PrintFieldData( DrawLibItem ) ) );
   }

    msg = _( "#End Cmp\n" );
    fputs( TO_UTF8( msg ), f );
    return 0;
}
