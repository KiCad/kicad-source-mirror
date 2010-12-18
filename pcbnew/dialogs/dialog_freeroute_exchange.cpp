/////////////////////////////////////////////////////////////////////////////
// Name:        dialog_freeroute.cpp
/////////////////////////////////////////////////////////////////////////////

#include "fctsys.h"
#include "appl_wxstruct.h"
#include "common.h"
#include "confirm.h"
#include "gestfich.h"
#include "pcbnew.h"
#include "wxPcbStruct.h"
#include "../common/dialogs/dialog_display_info_HTML_base.h"

#include "dialog_freeroute_exchange.h"


#define FREEROUTE_URL_KEY wxT( "freeroute_url" )
#define FREEROUTE_RUN_KEY wxT( "freeroute_command" )


/**********************************************************************/
void WinEDA_PcbFrame::Access_to_External_Tool( wxCommandEvent& event )
/**********************************************************************/

/* Run an external tool (currently, only freeroute)
 */
{
    DIALOG_FREEROUTE dialog( this );
    dialog.ShowModal();
}



DIALOG_FREEROUTE::DIALOG_FREEROUTE( WinEDA_PcbFrame* parent ):
    DIALOG_FREEROUTE_BASE( parent )
{
    m_Parent = parent;
    MyInit();

    GetSizer()->SetSizeHints( this );
    Centre();
}



/* Specific data initialisation
 */

void DIALOG_FREEROUTE::MyInit()
{
    SetFocus();
    m_FreeRouteSetupChanged = false;

    wxString msg;
    wxGetApp().m_EDA_Config->Read( FREEROUTE_URL_KEY, &msg );
    if( msg.IsEmpty() )
        m_FreerouteURLName->SetValue( wxT( "http://www.freerouting.net/" ) );
    else
        m_FreerouteURLName->SetValue( msg );
}

const char * s_FreeRouteHelpInfo =
#include "dialog_freeroute_exchange_help_html.h"
;
void DIALOG_FREEROUTE::OnHelpButtonClick( wxCommandEvent& event )
{
    DIALOG_DISPLAY_HTML_TEXT_BASE help_Dlg( this, wxID_ANY,
        _("Freeroute Help"),wxDefaultPosition, wxSize( 650,550 ) );

    wxString msg = CONV_FROM_UTF8(s_FreeRouteHelpInfo);
    help_Dlg.m_htmlWindow->AppendToPage( msg );
    help_Dlg.ShowModal();
}

/*  wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_CREATE_EXPORT_DSN_FILE
 */
void DIALOG_FREEROUTE::OnExportButtonClick( wxCommandEvent& event )
{
    m_Parent->ExportToSpecctra( event );
}


/*  wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_IMPORT_FREEROUTE_DSN_FILE
 */
void DIALOG_FREEROUTE::OnImportButtonClick( wxCommandEvent& event )
{
    m_Parent->ImportSpecctraSession(  event );

    /* Connectivity inf must be rebuild.
     * because for large board it can take some time, this is made only on demand
     */
    if( IsOK( this, _("Do you want to rebuild connectivity data ?" ) ) )
        m_Parent->Compile_Ratsnest( NULL, true );
}


/* wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_RUN_FREEROUTE
 */
void DIALOG_FREEROUTE::OnLaunchButtonClick( wxCommandEvent& event )
{
    wxString FullFileName = FindKicadFile( wxT( "freeroute.jnlp" ) );
    wxString command;

    if( wxFileExists( FullFileName ) )
    {
        // Wrap FullFileName in double quotes in case it has C:\Program Files in it.
        // The space is interpreted as an argument separator.
        command << wxT("javaws") << wxChar(' ') << wxChar('"') << FullFileName << wxChar('"');
        ProcessExecute( command );
        return;
    }

    command = m_FreerouteURLName->GetValue() + wxT( "/java/freeroute.jnlp" );

    wxLaunchDefaultBrowser( command );
}


/* wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON
 */
void DIALOG_FREEROUTE::OnVisitButtonClick( wxCommandEvent& event )
{
    wxString command = m_FreerouteURLName->GetValue();

    wxLaunchDefaultBrowser( command );
}


/*  wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_CLOSE
 */
void DIALOG_FREEROUTE::OnCancelButtonClick( wxCommandEvent& event )
{
    EndModal(wxID_CANCEL);
}


void DIALOG_FREEROUTE::OnOKButtonClick( wxCommandEvent& event )
{
    if( m_FreeRouteSetupChanged )  // Save new config
    {
        wxGetApp().m_EDA_Config->Write( FREEROUTE_URL_KEY,
                                        m_FreerouteURLName->GetValue() );
    }

    EndModal(wxID_OK);
}


/* wxEVT_COMMAND_TEXT_UPDATED event handler for ID_TEXT_EDIT_FR_URL
 */
void DIALOG_FREEROUTE::OnTextEditFrUrlUpdated( wxCommandEvent& event )
{
    m_FreeRouteSetupChanged = true;
}


