/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 3 of the License, or (at your
 * option) any later version.
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

#include "dialog_remote_symbol_config.h"

#include <common.h>
#include <kiplatform/environment.h>
#include <pgm_base.h>
#include <settings/settings_manager.h>
#include <widgets/ui_common.h>

#include <wx/button.h>
#include <wx/dirdlg.h>
#include <wx/filename.h>
#include <wx/intl.h>
#include <wx/msgdlg.h>
#include <wx/radiobut.h>
#include <wx/sizer.h>
#include <wx/stattext.h>
#include <wx/textctrl.h>


DIALOG_REMOTE_SYMBOL_CONFIG::DIALOG_REMOTE_SYMBOL_CONFIG( wxWindow* aParent ) :
                DIALOG_SHIM( aParent, wxID_ANY, _( "Remote Symbol Settings" ), wxDefaultPosition,
                                          wxDefaultSize, wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER ),
                m_destinationCtrl( nullptr ),
                m_prefixCtrl( nullptr ),
                m_prefixHint( nullptr ),
                m_projectRadio( nullptr ),
                m_globalRadio( nullptr ),
                m_resetButton( nullptr ),
                m_browseButton( nullptr ),
                m_settings( GetAppSettings<EESCHEMA_SETTINGS>( "eeschema" ) )
{
        wxBoxSizer* topSizer = new wxBoxSizer( wxVERTICAL );

        wxStaticText* intro = new wxStaticText( this, wxID_ANY,
                        _( "Configure where downloaded remote libraries are stored and how they are named." ) );
        intro->Wrap( FromDIP( 420 ) );
        topSizer->Add( intro, 0, wxALL | wxEXPAND, FromDIP( 10 ) );

        wxFlexGridSizer* gridSizer = new wxFlexGridSizer( 2, FromDIP( 6 ), FromDIP( 6 ) );
        gridSizer->AddGrowableCol( 1, 1 );

        gridSizer->Add( new wxStaticText( this, wxID_ANY, _( "Destination directory:" ) ),
                                        0, wxALIGN_CENTER_VERTICAL );

        wxBoxSizer* destSizer = new wxBoxSizer( wxHORIZONTAL );
        m_destinationCtrl = new wxTextCtrl( this, wxID_ANY );
        m_destinationCtrl->SetMinSize( FromDIP( wxSize( 320, -1 ) ) );
        m_destinationCtrl->SetToolTip( _( "Directory where downloaded symbol, footprint, and 3D model data will be written." ) );
        destSizer->Add( m_destinationCtrl, 1, wxALIGN_CENTER_VERTICAL );

        m_browseButton = new wxButton( this, wxID_ANY, _( "Browse..." ) );
        destSizer->Add( m_browseButton, 0, wxLEFT, FromDIP( 4 ) );
        gridSizer->Add( destSizer, 1, wxEXPAND );

        gridSizer->Add( new wxStaticText( this, wxID_ANY, _( "Library prefix:" ) ),
                                        0, wxALIGN_CENTER_VERTICAL );
        m_prefixCtrl = new wxTextCtrl( this, wxID_ANY );
        m_prefixCtrl->SetToolTip( _( "Prefix that will be applied to the generated libraries." ) );
        gridSizer->Add( m_prefixCtrl, 0, wxEXPAND );

        gridSizer->AddSpacer( 0 );
        m_prefixHint = new wxStaticText( this, wxID_ANY, wxString() );
        m_prefixHint->SetFont( KIUI::GetSmallInfoFont( this ).Italic() );
        gridSizer->Add( m_prefixHint, 0, wxEXPAND );

        gridSizer->Add( new wxStaticText( this, wxID_ANY, _( "Add libraries to:" ) ),
                                        0, wxALIGN_CENTER_VERTICAL );

        wxBoxSizer* radioSizer = new wxBoxSizer( wxHORIZONTAL );
        m_projectRadio = new wxRadioButton( this, wxID_ANY, _( "Project library table" ),
                                                                                wxDefaultPosition, wxDefaultSize, wxRB_GROUP );
        m_projectRadio->SetToolTip( _( "Adds the generated libraries to the project's library tables." ) );
        radioSizer->Add( m_projectRadio, 0, wxRIGHT | wxALIGN_CENTER_VERTICAL, FromDIP( 12 ) );

        m_globalRadio = new wxRadioButton( this, wxID_ANY, _( "Global library table" ) );
        m_globalRadio->SetToolTip( _( "Adds the generated libraries to the global library tables." ) );
        radioSizer->Add( m_globalRadio, 0, wxALIGN_CENTER_VERTICAL );

        gridSizer->Add( radioSizer, 0, wxEXPAND );

        topSizer->Add( gridSizer, 0, wxALL | wxEXPAND, FromDIP( 10 ) );

        m_resetButton = new wxButton( this, wxID_ANY, _( "Reset to Defaults" ) );
        m_resetButton->SetToolTip( _( "Restore the default destination and prefix." ) );
        topSizer->Add( m_resetButton, 0, wxLEFT | wxBOTTOM, FromDIP( 10 ) );

        wxStdDialogButtonSizer* buttonSizer = CreateStdDialogButtonSizer( wxOK | wxCANCEL );
        topSizer->Add( buttonSizer, 0, wxALL | wxEXPAND, FromDIP( 10 ) );

        SetSizer( topSizer );
        topSizer->Fit( this );

        SetupStandardButtons();
        finishDialogSettings();

        SetInitialFocus( m_destinationCtrl );

        m_browseButton->Bind( wxEVT_BUTTON, &DIALOG_REMOTE_SYMBOL_CONFIG::onBrowseDestination, this );
        m_resetButton->Bind( wxEVT_BUTTON, &DIALOG_REMOTE_SYMBOL_CONFIG::onResetDefaults, this );
        m_prefixCtrl->Bind( wxEVT_TEXT, &DIALOG_REMOTE_SYMBOL_CONFIG::onPrefixChanged, this );
}


bool DIALOG_REMOTE_SYMBOL_CONFIG::TransferDataToWindow()
{
        if( m_settings )
                applyRemoteSettings( m_settings->m_RemoteSymbol );
        else
        {
                EESCHEMA_SETTINGS::REMOTE_SYMBOL_CONFIG defaults;
                applyRemoteSettings( defaults );
        }

        return true;
}


bool DIALOG_REMOTE_SYMBOL_CONFIG::TransferDataFromWindow()
{
        wxString destination = m_destinationCtrl->GetValue();
        wxString prefix = m_prefixCtrl->GetValue();

        destination.Trim( true ).Trim( false );
        prefix.Trim( true ).Trim( false );

        if( destination.IsEmpty() )
        {
                wxMessageBox( _( "Please choose a destination directory." ), GetTitle(), wxOK | wxICON_WARNING, this );
                m_destinationCtrl->SetFocus();
                return false;
        }

        if( prefix.IsEmpty() )
        {
                wxMessageBox( _( "Please enter a library prefix." ), GetTitle(), wxOK | wxICON_WARNING, this );
                m_prefixCtrl->SetFocus();
                return false;
        }

        if( !m_settings )
                m_settings = GetAppSettings<EESCHEMA_SETTINGS>( "eeschema" );

        if( !m_settings )
                return true;

        m_settings->m_RemoteSymbol.destination_dir = destination;
        m_settings->m_RemoteSymbol.library_prefix = prefix;
        m_settings->m_RemoteSymbol.add_to_global_table = m_globalRadio->GetValue();

        return true;
}


void DIALOG_REMOTE_SYMBOL_CONFIG::onBrowseDestination( wxCommandEvent& aEvent )
{
        wxUnusedVar( aEvent );

        wxString initialPath = ExpandEnvVarSubstitutions( m_destinationCtrl->GetValue(), &Prj() );

        if( initialPath.IsEmpty() )
        {
                if( Prj().IsNullProject() )
                        initialPath = KIPLATFORM::ENV::GetDocumentsPath();
                else
                        initialPath = Prj().GetProjectPath();
        }

        wxDirDialog dlg( this, _( "Select Destination Directory" ), initialPath );

        if( dlg.ShowModal() != wxID_OK )
                return;

        wxString path = dlg.GetPath();

        if( !Prj().IsNullProject() )
        {
                wxString projectRoot = Prj().GetProjectPath();

                wxFileName selectedDir = wxFileName::DirName( path );

                if( selectedDir.MakeRelativeTo( projectRoot ) )
                {
                        wxString relative = selectedDir.GetFullPath();

                        if( relative.IsEmpty() || relative == wxS( "." ) )
                                path = wxS( "${KIPRJMOD}" );
                        else
                                path = wxString::Format( wxS( "${KIPRJMOD}/%s" ), relative );
                }
        }

        path.Replace( wxS( "\\" ), wxS( "/" ) );
        m_destinationCtrl->ChangeValue( path );
}


void DIALOG_REMOTE_SYMBOL_CONFIG::onResetDefaults( wxCommandEvent& aEvent )
{
        wxUnusedVar( aEvent );

        EESCHEMA_SETTINGS::REMOTE_SYMBOL_CONFIG defaults;
        applyRemoteSettings( defaults );
}


void DIALOG_REMOTE_SYMBOL_CONFIG::onPrefixChanged( wxCommandEvent& aEvent )
{
        wxUnusedVar( aEvent );
        updatePrefixHint();
}


void DIALOG_REMOTE_SYMBOL_CONFIG::applyRemoteSettings(
                const EESCHEMA_SETTINGS::REMOTE_SYMBOL_CONFIG& aConfig )
{
        m_destinationCtrl->ChangeValue( aConfig.destination_dir );
        m_prefixCtrl->ChangeValue( aConfig.library_prefix );

        if( aConfig.add_to_global_table )
                m_globalRadio->SetValue( true );
        else
                m_projectRadio->SetValue( true );

        updatePrefixHint();
}


void DIALOG_REMOTE_SYMBOL_CONFIG::updatePrefixHint()
{
        const wxString prefix = m_prefixCtrl->GetValue();

        if( prefix.IsEmpty() )
        {
                m_prefixHint->SetLabel(
                                _( "Library names will be created with suffixes such as _symbols, _fp, and _3d." ) );
        }
        else
        {
                m_prefixHint->SetLabel( wxString::Format(
                                _( "Will create libraries like %1$s_symbols, %1$s_fp, %1$s_3d, etc." ), prefix ) );
        }

        m_prefixHint->Wrap( FromDIP( 360 ) );
}
