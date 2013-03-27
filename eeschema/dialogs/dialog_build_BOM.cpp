/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2012 Jean-Pierre Charras, jp.charras at wanadoo.fr
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

#include <netlist.h>
#include <template_fieldnames.h>
#include <sch_component.h>
#include <sch_sheet.h>
#include <class_library.h>

#include <wx/valgen.h>

#include <dialog_build_BOM.h>
#include <BOM_lister.h>


/* Local variables */
static bool     s_ListByRef    = true;
static bool     s_ListByValue  = true;
static bool     s_ListWithSubCmponents;
static bool     s_ListHierarchicalPinByName;
static bool     s_ListHierarchicalPinBySheet;
static bool     s_BrowseCreatedList;
static int      s_OutputFormOpt;
static int      s_OutputSeparatorOpt;
static bool     s_Add_Location = false;
static bool     s_Add_FpField_state = true;
static bool     s_Add_DatasheetField_state;
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
    &s_Add_DatasheetField_state,
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
const wxString OPTION_BOM_ADD_LOCATION ( wxT("BomAddLocation") );

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
    m_config = wxGetApp().GetSettings();
    wxASSERT( m_config != NULL );

    m_parent = parent;

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
    m_config->Read( OPTION_BOM_LIST_REF, &s_ListByRef );
    m_config->Read( OPTION_BOM_LIST_VALUE , &s_ListByValue );
    m_config->Read( OPTION_BOM_LIST_HPINS, &s_ListHierarchicalPinByName );
    m_config->Read( OPTION_BOM_LIST_HPINS_BY_SHEET, &s_ListWithSubCmponents );
    m_config->Read( OPTION_BOM_LIST_HPINS_BY_NAME_, &s_ListWithSubCmponents );
    m_config->Read( OPTION_BOM_LIST_SUB_CMP, &s_ListWithSubCmponents );
    m_config->Read( OPTION_BOM_LIST_HPINS_BY_SHEET, &s_ListHierarchicalPinBySheet );
    m_config->Read( OPTION_BOM_LIST_HPINS_BY_NAME_, &s_ListHierarchicalPinByName );
    s_OutputFormOpt = m_config->Read( OPTION_BOM_FORMAT, 0l );
    m_config->Read( OPTION_BOM_LAUNCH_BROWSER, &s_BrowseCreatedList );
    s_OutputSeparatorOpt   = m_config->Read( OPTION_BOM_SEPARATOR, 0l );
    m_config->Read( OPTION_BOM_ADD_LOCATION, &s_Add_Location );

    long addfields = m_config->Read( OPTION_BOM_ADD_FIELD, 0l );
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

    m_AddLocationField->SetValidator( wxGenericValidator( &s_Add_Location ) );
    m_AddFootprintField->SetValidator( wxGenericValidator( &s_Add_FpField_state ) );
    m_AddDatasheetField->SetValidator( wxGenericValidator( &s_Add_DatasheetField_state ) );
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


/*
 * Called on BOM format selection:
 * Enable/disable options in dialog
 */
void DIALOG_BUILD_BOM::OnRadioboxSelectFormatSelected( wxCommandEvent& event )
{
    switch( m_OutputFormCtrl->GetSelection() )
    {
        case 0:     // Human readable text full report
            m_OutputSeparatorCtrl->Enable( false );
            m_ListCmpbyRefItems->Enable( true );
            m_ListCmpbyValItems->Enable( true );
            m_GenListLabelsbyVal->Enable( true );
            m_GenListLabelsbySheet->Enable( true );
            m_ListSubCmpItems->Enable( true );
            m_AddLocationField->Enable( true );
            break;

        case 1:     // Csv format, full list by reference
            m_OutputSeparatorCtrl->Enable( true );
            m_ListCmpbyRefItems->Enable( false );
            m_ListCmpbyValItems->Enable( false );
            m_GenListLabelsbyVal->Enable( false );
            m_GenListLabelsbySheet->Enable( false );
            m_ListSubCmpItems->Enable( true );
            m_AddLocationField->Enable( true );
            break;

        case 2:     // Csv format, grouped list by reference
            m_OutputSeparatorCtrl->Enable( true );
            m_ListCmpbyRefItems->Enable( false );
            m_ListCmpbyValItems->Enable( false );
            m_GenListLabelsbyVal->Enable( false );
            m_GenListLabelsbySheet->Enable( false );
            m_ListSubCmpItems->Enable( false );
            m_AddLocationField->Enable( false );
            break;

        case 3:     // Csv format, short list by values
            m_OutputSeparatorCtrl->Enable( true );
            m_ListCmpbyRefItems->Enable( false );
            m_ListCmpbyValItems->Enable( false );
            m_GenListLabelsbyVal->Enable( false );
            m_GenListLabelsbySheet->Enable( false );
            m_ListSubCmpItems->Enable( false );
            m_AddLocationField->Enable( false );
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
    s_Add_Location = m_AddLocationField->GetValue();
    s_Add_FpField_state = m_AddFootprintField->GetValue();
    s_Add_DatasheetField_state = m_AddDatasheetField->GetValue();
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
    m_config->Write( OPTION_BOM_LIST_REF, s_ListByRef );
    m_config->Write( OPTION_BOM_LIST_VALUE , s_ListByValue );
    m_config->Write( OPTION_BOM_LIST_HPINS, s_ListHierarchicalPinByName );
    m_config->Write( OPTION_BOM_LIST_HPINS_BY_SHEET, s_ListHierarchicalPinBySheet );
    m_config->Write( OPTION_BOM_LIST_HPINS_BY_NAME_, s_ListHierarchicalPinByName );
    m_config->Write( OPTION_BOM_LIST_SUB_CMP, s_ListWithSubCmponents );

    m_config->Write( OPTION_BOM_FORMAT, (long) s_OutputFormOpt );
    m_config->Write( OPTION_BOM_SEPARATOR, (long) s_OutputSeparatorOpt );
    m_config->Write( OPTION_BOM_LAUNCH_BROWSER, (long) s_BrowseCreatedList );

    // Now save current settings of all "Fields to add" checkboxes
    m_config->Write( OPTION_BOM_ADD_LOCATION, s_Add_Location );

    long addfields = 0;
    for( int ii = 0, bitmask = 1; s_AddFieldList[ii] != NULL; ii++ )
    {
        if( *s_AddFieldList[ii] )
            addfields |= bitmask;

        bitmask <<= 1;
    }

    m_config->Write( OPTION_BOM_ADD_FIELD, addfields );
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
                      fn.GetFullName(), wildcard, wxFD_SAVE );

    if( dlg.ShowModal() == wxID_CANCEL )
        return;

    fn = dlg.GetPath();        // remember path+filename+ext for subsequent runs.

    m_listFileName = dlg.GetPath();

    // Close dialog, then show the list (if so requested)

    switch( aTypeFile )
    {
    case 0: // list
        CreatePartsAndLabelsFullList( aIncludeSubComponents );
        break;

    case 1: // spreadsheet, Single Part per line
        CreateSpreadSheetPartsFullList( aIncludeSubComponents, s_Add_Location, false );
        break;

    case 2: // spreadsheet, group Part with same fields per line
        CreateSpreadSheetPartsFullList( aIncludeSubComponents, s_Add_Location, true );
        break;

    case 3: // spreadsheet, one value per line and no sub-component
        CreateSpreadSheetPartsShortList();
        break;
    }

    EndModal( 1 );

    if( aRunBrowser )
    {
        wxString editorname = wxGetApp().GetEditorName();
        wxString filename   = m_listFileName;
        AddDelimiterString( filename );
        ExecuteFile( this, editorname, filename );
    }
}

/** Helper function IsFieldChecked
 * return the state of the wxCheckbox corresponding to the
 * field aFieldId (FOOTPRINT, DATASHEET and FIELD1 to FIELD8
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
        case DATASHEET:
            return m_AddDatasheetField->IsChecked();
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
void DIALOG_BUILD_BOM::CreateSpreadSheetPartsShortList( )
{
    FILE* f;

    if( ( f = wxFopen( m_listFileName, wxT( "wt" ) ) ) == NULL )
    {
        wxString msg;
        msg.Printf( _( "Failed to open file '%s'" ), GetChars(m_listFileName) );
        DisplayError( this, msg );
        return;
    }

    BOM_LISTER bom_lister;
    bom_lister.SetCvsFormOn( s_ExportSeparatorSymbol );

    // Set the list of fields to add to list
    for( int ii = FOOTPRINT; ii <= FIELD8; ii++ )
        if( IsFieldChecked( ii ) )
            bom_lister.AddFieldIdToPrintList( ii );
    // Write the list of components grouped by values:
    bom_lister.CreateCsvBOMListByValues( f );
}


/*
 * Print a list of components, in a form which can be imported by a spreadsheet
 * form is:
 * cmp ref; cmp val; fields;
 * Components are sorted by reference
 * param aIncludeSubComponents = true to print sub components
 * param aPrintLocation = true to print components location
 *        (only possible when aIncludeSubComponents == true)
 * param aGroupRefs = true to group components references, when other fieds
 *          have the same value
 */
void DIALOG_BUILD_BOM::CreateSpreadSheetPartsFullList( bool aIncludeSubComponents,
                                                       bool aPrintLocation,
                                                       bool aGroupRefs )
{
    FILE*    f;
    wxString msg;

    if( ( f = wxFopen( m_listFileName, wxT( "wt" ) ) ) == NULL )
    {
        msg = _( "Failed to open file " );
        msg << m_listFileName;
        DisplayError( this, msg );
        return;
    }

    BOM_LISTER bom_lister;
    bom_lister.SetCvsFormOn( s_ExportSeparatorSymbol );

    // Set group refs option (hight priority):
    // Obvioulsy only useful when not including sub-components
    bom_lister.SetGroupReferences( aGroupRefs );
    bom_lister.SetIncludeSubCmp( aIncludeSubComponents && !aGroupRefs );

    // Set print location option:
    // Obvioulsy only possible when including sub components
    // and not grouping references
    bom_lister.SetPrintLocation( aPrintLocation && !aGroupRefs &&
                                 aIncludeSubComponents );

    // Set the list of fields to add to list
    for( int ii = FOOTPRINT; ii <= FIELD8; ii++ )
        if( IsFieldChecked( ii ) )
            bom_lister.AddFieldIdToPrintList( ii );

    // create the file
    bom_lister.PrintComponentsListByReferenceCsvForm( f );

    fclose( f );
}


/*
 * CreatePartsAndLabelsFullList()
 * Main function to create the list of components and/or labels
 * (global labels, hierarchical labels and pin sheets )
 */
void DIALOG_BUILD_BOM::CreatePartsAndLabelsFullList( bool aIncludeSubComponents )
{
    FILE*    f;
    wxString msg;

    if( ( f = wxFopen( m_listFileName, wxT( "wt" ) ) ) == NULL )
    {
        msg = _( "Failed to open file " );
        msg << m_listFileName;
        DisplayError( this, msg );
        return;
    }

    BOM_LISTER bom_lister;
    bom_lister.SetIncludeSubCmp( aIncludeSubComponents );
    bom_lister.SetCvsFormOff();
    bom_lister.SetPrintLocation( s_Add_Location );
    // Set the list of fields to add to list
    for( int ii = FOOTPRINT; ii <= FIELD8; ii++ )
        if( IsFieldChecked( ii ) )
            bom_lister.AddFieldIdToPrintList( ii );

    // creates the list file
    wxString Title = wxGetApp().GetAppName() + wxT( " " ) + GetBuildVersion();

    fprintf( f, "%s  >> Creation date: %s\n", TO_UTF8( Title ), TO_UTF8( DateAndTime() ) );

    if( m_ListCmpbyRefItems->GetValue() )
        bom_lister.PrintComponentsListByReferenceHumanReadable( f );

    if( m_ListCmpbyValItems->GetValue() )
        bom_lister.PrintComponentsListByValue( f );

    // Create list of global labels, hierachical labels and pins sheets

    if( m_GenListLabelsbySheet->GetValue() )
        bom_lister.PrintGlobalAndHierarchicalLabelsList( f, true );

    if( m_GenListLabelsbyVal->GetValue() )
        bom_lister.PrintGlobalAndHierarchicalLabelsList( f, false );

    msg = _( "\n#End List\n" );
    fprintf( f, "%s", TO_UTF8( msg ) );
    fclose( f );
}
