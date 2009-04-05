/////////////////////////////////////////////////////////////////////////////

// Name:        dialog_build_BOM.cpp
// Author:      jean-pierre Charras
// Modified by:
// Licence:     GPL
/////////////////////////////////////////////////////////////////////////////


#include "fctsys.h"
#include "appl_wxstruct.h"

#include "common.h"
#include "program.h"
#include "libcmp.h"
#include "general.h"
#include "netlist.h"

#include "wx/valgen.h"

#include "dialog_build_BOM.h"


#include "protos.h"


/* Local variables */
static bool  s_ListByRef   = TRUE;
static bool  s_ListByValue = TRUE;
static bool  s_ListWithSubCmponents;
static bool  s_ListHierarchicalPinByName;
static bool  s_ListBySheet;
static bool  s_BrowseCreatedList;
static int   s_OutputFormOpt;
static int   s_OutputSeparatorOpt;
static bool  s_Add_FpField_state;
static bool  s_Add_F1_state;
static bool  s_Add_F2_state;
static bool  s_Add_F3_state;
static bool  s_Add_F4_state;
static bool  s_Add_F5_state;
static bool  s_Add_F6_state;
static bool  s_Add_F7_state;
static bool  s_Add_F8_state;
static bool  s_Add_Alls_state;

static bool* s_AddFieldList[] = {
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


#define OPTION_BOM_FORMAT    wxT( "BomFormat" )
#define OPTION_BOM_LAUNCH_BROWSER    wxT( "BomLaunchBrowser" )
#define OPTION_BOM_SEPARATOR wxT( "BomExportSeparator" )
#define OPTION_BOM_ADD_FIELD wxT( "BomAddField" )

/* list of separators used in bom export to spreadsheet
 * (selected by s_OutputSeparatorOpt, and s_OutputSeparatorOpt radiobox)
 */
static char s_ExportSeparator[] = ("\t;,.");

/*!
 * DIALOG_BUILD_BOM dialog type definition
 */


DIALOG_BUILD_BOM::DIALOG_BUILD_BOM( WinEDA_DrawFrame* parent ):
    DIALOG_BUILD_BOM_BASE(parent)
{
    m_Config = wxGetApp().m_EDA_Config;
    wxASSERT( m_Config != NULL );

    m_Parent = parent;

    Init( );

    if (GetSizer())
    {
        GetSizer()->SetSizeHints(this);
    }
    Centre();
}


/*!
 * Init Controls for DIALOG_BUILD_BOM
 */

void DIALOG_BUILD_BOM::Init()
{
    SetFont( *g_DialogFont );

    SetFocus();

    /* Get options */
    s_OutputFormOpt      = m_Config->Read( OPTION_BOM_FORMAT, (long) 0 );
    s_BrowseCreatedList      = m_Config->Read( OPTION_BOM_LAUNCH_BROWSER, (long) 0 );
    s_OutputSeparatorOpt = m_Config->Read( OPTION_BOM_SEPARATOR, (long) 0 );
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
    m_ListCmpbyRefItems->SetValidator( wxGenericValidator(& s_ListByRef) );
    m_ListSubCmpItems->SetValidator( wxGenericValidator(& s_ListWithSubCmponents) );
    m_ListCmpbyValItems->SetValidator( wxGenericValidator(& s_ListByValue) );
    m_GenListLabelsbyVal->SetValidator( wxGenericValidator(& s_ListHierarchicalPinByName) );
    m_GenListLabelsbySheet->SetValidator( wxGenericValidator(& s_ListBySheet) );
    m_OutputFormCtrl->SetValidator( wxGenericValidator(& s_OutputFormOpt) );
    m_OutputSeparatorCtrl->SetValidator( wxGenericValidator(& s_OutputSeparatorOpt) );
    m_GetListBrowser->SetValidator( wxGenericValidator(& s_BrowseCreatedList) );
    m_AddFootprintField->SetValidator( wxGenericValidator(& s_Add_FpField_state) );
    m_AddField1->SetValidator( wxGenericValidator(& s_Add_F1_state) );
    m_AddField2->SetValidator( wxGenericValidator(& s_Add_F2_state) );
    m_AddField3->SetValidator( wxGenericValidator(& s_Add_F3_state) );
    m_AddField4->SetValidator( wxGenericValidator(& s_Add_F4_state) );
    m_AddField5->SetValidator( wxGenericValidator(& s_Add_F5_state) );
    m_AddField6->SetValidator( wxGenericValidator(& s_Add_F6_state) );
    m_AddField7->SetValidator( wxGenericValidator(& s_Add_F7_state) );
    m_AddField8->SetValidator( wxGenericValidator(& s_Add_F8_state) );
    m_AddAllFields->SetValidator( wxGenericValidator(& s_Add_Alls_state) );

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
    if( m_OutputFormCtrl->GetSelection() == 1 )
    {
        m_OutputSeparatorCtrl->Enable( true );
        m_ListCmpbyValItems->Enable( false );
        m_GenListLabelsbyVal->Enable( false );
        m_GenListLabelsbySheet->Enable( false );
    }
    else
    {
        m_OutputSeparatorCtrl->Enable( false );
        m_ListCmpbyValItems->Enable( true );
        m_GenListLabelsbyVal->Enable( true );
        m_GenListLabelsbySheet->Enable( true );
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

    bool ExportFileType = m_OutputFormCtrl->GetSelection() == 0 ? false : true;

    SavePreferences();

    Create_BOM_Lists( ExportFileType, m_ListSubCmpItems->GetValue(),
                      ExportSeparatorSymbol, m_GetListBrowser->GetValue());
}



/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_CANCEL
 */

void DIALOG_BUILD_BOM::OnCancelClick( wxCommandEvent& event )
{
    EndModal( -1 );
}


/**************************************************/
void DIALOG_BUILD_BOM::SavePreferences()
/**************************************************/
{
    wxConfig* config = wxGetApp().m_EDA_Config;
    wxASSERT( config != NULL );

    // Determine current settings of "List items" and "Options" checkboxes
    // (NOTE: These 6 settings are restored when the dialog box is next
    // invoked, but are *not* still saved after EESchema is next shut down.)
    s_ListByRef = m_ListCmpbyRefItems->GetValue();
    s_ListWithSubCmponents = m_ListSubCmpItems->GetValue();
    s_ListByValue = m_ListCmpbyValItems->GetValue();
    s_ListHierarchicalPinByName = m_GenListLabelsbyVal->GetValue();
    s_ListBySheet = m_GenListLabelsbySheet->GetValue();
    s_BrowseCreatedList   = m_GetListBrowser->GetValue();

    // (aved in config ):

    // Determine current settings of both radiobutton groups
    s_OutputFormOpt      = m_OutputFormCtrl->GetSelection();
    s_OutputSeparatorOpt = m_OutputSeparatorCtrl->GetSelection();
    if( s_OutputSeparatorOpt < 0 )
        s_OutputSeparatorOpt = 0;

    // Determine current settings of all "Fields to add" checkboxes
    s_Add_FpField_state = m_AddFootprintField->GetValue();
    s_Add_F1_state = m_AddField1->GetValue();
    s_Add_F2_state = m_AddField2->GetValue();
    s_Add_F3_state = m_AddField3->GetValue();
    s_Add_F4_state = m_AddField4->GetValue();
    s_Add_F5_state = m_AddField5->GetValue();
    s_Add_F6_state = m_AddField6->GetValue();
    s_Add_F7_state = m_AddField7->GetValue();
    s_Add_F8_state = m_AddField8->GetValue();
    s_Add_Alls_state = m_AddAllFields->GetValue();

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
