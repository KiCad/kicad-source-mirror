/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2007 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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

#include <confirm.h>

#include <functional>
#include <wx/app.h>
#include <wx/stockitem.h>
#include <wx/richmsgdlg.h>
#include <wx/msgdlg.h>
#include <wx/choicdlg.h>
#include <wx/crt.h>
/**
 * Flag to enable confirmation dialog debugging output.
 *
 * @ingroup trace_env_vars
 */
static const wxChar traceConfirm[] = wxT( "KICAD_CONFIRM" );


bool AskOverrideLock( wxWindow* aParent, const wxString& aMessage )
{
#ifdef __APPLE__
    // wxMessageDialog gets the button spacing wrong on Mac so we have to use wxRichMessageDialog.
    // Note that its warning icon is more like wxMessageDialog's error icon, so we use it instead
    // of wxICON_ERROR.
    wxRichMessageDialog dlg( aParent, aMessage, _( "File Open Warning" ),
                             wxYES_NO | wxICON_WARNING | wxCENTER );
    dlg.SetExtendedMessage( _( "Interleaved saves may produce very unexpected results." )
                                + wxS( "\n" ) );
    dlg.SetYesNoLabels( _( "&Cancel" ), _( "&Open Anyway" ) );
#else
    KICAD_MESSAGE_DIALOG_BASE dlg( aParent, aMessage, _( "File Open Warning" ),
                         wxYES_NO | wxICON_ERROR | wxCENTER );
    dlg.SetExtendedMessage( _( "Interleaved saves may produce very unexpected results." ) );
    dlg.SetYesNoLabels( _( "&Cancel" ), _( "&Open Anyway" ) );
#endif

    return dlg.ShowModal() == wxID_NO;
}


int UnsavedChangesDialog( wxWindow* parent, const wxString& aMessage, bool* aApplyToAll )
{
    static bool s_apply_to_all = false;

    KICAD_RICH_MESSAGE_DIALOG_BASE dlg( parent, aMessage, _( "Save Changes?" ),
                                        wxYES_NO | wxCANCEL |
                                        wxYES_DEFAULT | wxICON_WARNING | wxCENTER );
    dlg.SetExtendedMessage( _( "If you don't save, all your changes will be permanently lost." )
                                + wxS( "\n" ) );
    dlg.SetYesNoLabels( _( "&Save" ), _( "&Discard Changes" ) );

    if( aApplyToAll )
        dlg.ShowCheckBox( _( "&Apply to all" ), s_apply_to_all );

    int ret = dlg.ShowModal();

    if( aApplyToAll )
    {
        *aApplyToAll = dlg.IsCheckBoxChecked();
        s_apply_to_all = dlg.IsCheckBoxChecked();
    }

    // Returns wxID_YES, wxID_NO, or wxID_CANCEL
    return ret;
}


int UnsavedChangesDialog( wxWindow* parent, const wxString& aMessage )
{
#ifdef __APPLE__
    // wxMessageDialog gets the button order (and spacing) wrong on Mac so we have to use
    // wxRichMessageDialog.
    return UnsavedChangesDialog( parent, aMessage, nullptr );
#else
    #ifdef _WIN32
    // wxMessageDialog on windows invokes TaskDialogIndirect which is a native function for a dialog
    // As a result it skips wxWidgets for modal management...and we don't parent frames properly
    // among other things for Windows to do the right thing by default
    // Disable all the windows manually to avoid being able to hit this dialog from the tool frame
    // and kicad frame at the same time.
    wxWindowDisabler disable( true );
    #endif

    KICAD_MESSAGE_DIALOG_BASE dlg( parent, aMessage, _( "Save Changes?" ),
                         wxYES_NO | wxCANCEL | wxYES_DEFAULT | wxICON_WARNING | wxCENTER );
    dlg.SetExtendedMessage( _( "If you don't save, all your changes will be permanently lost." ) );
    dlg.SetYesNoLabels( _( "&Save" ), _( "&Discard Changes" ) );

    // Returns wxID_YES, wxID_NO, or wxID_CANCEL
    return dlg.ShowModal();
#endif
}


bool ConfirmRevertDialog( wxWindow* parent, const wxString& aMessage )
{
    KICAD_MESSAGE_DIALOG_BASE dlg( parent, aMessage, wxEmptyString,
                                   wxOK | wxCANCEL | wxOK_DEFAULT | wxICON_WARNING | wxCENTER );
    dlg.SetExtendedMessage( _( "Your current changes will be permanently lost." ) );
    dlg.SetOKCancelLabels( _( "&Revert" ), _( "&Cancel" ) );

    return dlg.ShowModal() == wxID_OK;
}


static int g_lastUnsavedChangesResult = -1;

bool HandleUnsavedChanges( wxWindow* aParent, const wxString& aMessage,
                           const std::function<bool()>& aSaveFunction )
{
    g_lastUnsavedChangesResult = UnsavedChangesDialog( aParent, aMessage );
    switch( g_lastUnsavedChangesResult )
    {
    case wxID_YES:    return aSaveFunction();
    case wxID_NO:     return true; // proceed without saving
    default:
    case wxID_CANCEL: return false;
    }
}

int GetLastUnsavedChangesResponse()
{
    return g_lastUnsavedChangesResult;
}


int OKOrCancelDialog( wxWindow* aParent, const wxString& aWarning, const wxString& aMessage,
                      const wxString& aDetailedMessage, const wxString& aOKLabel,
                      const wxString& aCancelLabel, bool* aApplyToAll )
{
    KICAD_RICH_MESSAGE_DIALOG_BASE dlg( aParent, aMessage, aWarning,
                                        wxOK | wxCANCEL | wxOK_DEFAULT | wxICON_WARNING | wxCENTER );

    dlg.SetOKCancelLabels( ( aOKLabel.IsEmpty() ) ? _( "&OK" ) : aOKLabel,
                           ( aCancelLabel.IsEmpty() ) ? _( "&Cancel" ) : aCancelLabel );

    if( !aDetailedMessage.IsEmpty() )
        dlg.SetExtendedMessage( aDetailedMessage );

    if( aApplyToAll )
        dlg.ShowCheckBox( _( "&Apply to all" ), true );

    int ret = dlg.ShowModal();

    if( aApplyToAll )
        *aApplyToAll = dlg.IsCheckBoxChecked();

    // Returns wxID_OK or wxID_CANCEL
    return ret;
}


// DisplayError should be deprecated, use DisplayErrorMessage instead
void DisplayError( wxWindow* aParent, const wxString& aText )
{
    if( !wxTheApp || !wxTheApp->IsMainLoopRunning() )
    {
        wxLogError( "%s", aText );
        return;
    }

    if( !wxTheApp->IsGUI() )
    {
        wxFprintf( stderr, aText );
        return;
    }

    KICAD_MESSAGE_DIALOG_BASE* dlg;

    dlg = new KICAD_MESSAGE_DIALOG_BASE( aParent, aText, _( "Error" ),
                                         wxOK | wxCENTRE | wxRESIZE_BORDER |
                                         wxICON_ERROR | wxSTAY_ON_TOP );

    dlg->ShowModal();
    dlg->Destroy();
}


void DisplayErrorMessage( wxWindow* aParent, const wxString& aText, const wxString& aExtraInfo )
{
    if( !wxTheApp || !wxTheApp->IsMainLoopRunning() )
    {
        wxLogError( "%s %s", aText, aExtraInfo );
        return;
    }

    if( !wxTheApp->IsGUI() )
    {
        wxFprintf( stderr, aText );
        return;
    }

    KICAD_MESSAGE_DIALOG_BASE* dlg;

    dlg = new KICAD_MESSAGE_DIALOG_BASE( aParent, aText, _( "Error" ),
                                         wxOK | wxCENTRE | wxRESIZE_BORDER |
                                         wxICON_ERROR | wxSTAY_ON_TOP );

    if( !aExtraInfo.IsEmpty() )
        dlg->SetExtendedMessage( aExtraInfo );

    dlg->ShowModal();
    dlg->Destroy();
}


void DisplayInfoMessage( wxWindow* aParent, const wxString& aMessage, const wxString& aExtraInfo )
{
    if( !wxTheApp || !wxTheApp->GetTopWindow() )
    {
        wxLogTrace( traceConfirm, wxS( "%s %s" ), aMessage, aExtraInfo );
        return;
    }

    if( !wxTheApp->IsGUI() )
    {
        wxFprintf( stdout, "%s %s", aMessage, aExtraInfo );
        return;
    }

    KICAD_MESSAGE_DIALOG_BASE* dlg;
    int              icon = wxICON_INFORMATION;

    dlg = new KICAD_MESSAGE_DIALOG_BASE( aParent, aMessage, _( "Information" ),
                                         wxOK | wxCENTRE | wxRESIZE_BORDER |
                                         icon | wxSTAY_ON_TOP );

    if( !aExtraInfo.IsEmpty() )
        dlg->SetExtendedMessage( aExtraInfo );

    dlg->ShowModal();
    dlg->Destroy();
}


bool IsOK( wxWindow* aParent, const wxString& aMessage )
{
    // wxMessageDialog no longer responds correctly to the <ESC> key (on at least OSX and MSW)
    // so we're now using wxRichMessageDialog.
    //
    // Note also that we have to repurpose an OK/Cancel version of it because otherwise wxWidgets
    // uses "destructive" spacing for the "No" button.

#ifdef __APPLE__
    // Why is wxICON_QUESTION a light-bulb on Mac?  That has more of a hint or info connotation.
    int icon = wxICON_WARNING;
#else
    int icon = wxICON_QUESTION;
#endif

#if !defined( __WXGTK__ )
    KICAD_RICH_MESSAGE_DIALOG_BASE dlg( aParent, aMessage, _( "Confirmation" ),
                                        wxOK | wxCANCEL | wxOK_DEFAULT |
                                        wxCENTRE | icon | wxSTAY_ON_TOP );
#else
    wxMessageDialog dlg( aParent, aMessage, _( "Confirmation" ),
                         wxOK | wxCANCEL | wxOK_DEFAULT | wxCENTRE | icon | wxSTAY_ON_TOP );
#endif

    dlg.SetOKCancelLabels( _( "&Yes" ), _( "&No" ) );

    return dlg.ShowModal() == wxID_OK;
}


int SelectSingleOption( wxWindow* aParent, const wxString& aTitle,
                        const wxString& aMessage, const wxArrayString& aOptions )
{
    wxSingleChoiceDialog dlg( aParent, aMessage, aTitle, aOptions );

    if( dlg.ShowModal() != wxID_OK )
        return -1;

    return dlg.GetSelection();
}

