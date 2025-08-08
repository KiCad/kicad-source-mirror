/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2017 CERN
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * @author Maciej Suminski <maciej.suminski@cern.ch>
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

#include "dialog_gencad_export_options.h"

#include <wx/anybutton.h>
#include <wx/filedlg.h>
#include <wx/checkbox.h>
#include <wx/filepicker.h>
#include <wx/sizer.h>
#include <wx/stattext.h>
#include <wx/textctrl.h>

#include <jobs/job_export_pcb_gencad.h>

#include <pcb_edit_frame.h>
#include <kidialog.h>
#include <wildcards_and_files_ext.h>
#include <widgets/std_bitmap_button.h>
#include <string_utils.h>
#include <board.h>


DIALOG_GENCAD_EXPORT_OPTIONS::DIALOG_GENCAD_EXPORT_OPTIONS( PCB_EDIT_FRAME* aParent, const wxString& aTitle,
                                                            JOB_EXPORT_PCB_GENCAD* aJob ) :
        DIALOG_SHIM( aParent, wxID_ANY, aTitle, wxDefaultPosition, wxDefaultSize,
                     wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER ),
        m_frame( aParent ),
        m_job( aJob )
{
    wxBoxSizer* m_mainSizer = new wxBoxSizer( wxVERTICAL );

    m_fileSizer = new wxBoxSizer( wxHORIZONTAL );

    m_textFile = new wxStaticText( this, wxID_ANY, _( "Output File:" ), wxDefaultPosition, wxDefaultSize, 0 );
    m_textFile->Wrap( -1 );
    m_fileSizer->Add( m_textFile, 0, wxALIGN_CENTER_VERTICAL | wxLEFT, 5 );

	m_outputFileName = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
    m_outputFileName->SetToolTip( _( "Enter a filename if you do not want to use default file names" ) );
    m_outputFileName->SetMinSize( wxSize( 350, -1 ) );
    m_fileSizer->Add( m_outputFileName, 1, wxALL | wxEXPAND, 5 );

    m_browseButton = new STD_BITMAP_BUTTON( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxSize( -1, -1 ),
                                            wxBU_AUTODRAW | 0 );
    m_fileSizer->Add( m_browseButton, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 5 );

    m_mainSizer->Add( m_fileSizer, 0, wxEXPAND | wxALL, 5 );

    m_optsSizer = new wxGridSizer( 0, 1, 3, 3 );
    createOptCheckboxes();
    m_mainSizer->Add( m_optsSizer, 1, wxEXPAND | wxALL, 5 );

    wxSizer* stdButtons = CreateSeparatedButtonSizer( wxOK | wxCANCEL );
    m_mainSizer->Add( stdButtons, 0, wxALL | wxEXPAND, 5 );

    SetSizer( m_mainSizer );

    if( !aTitle.IsEmpty() )
        SetTitle( aTitle );

    if( aJob )
        m_browseButton->Hide();

    // DIALOG_SHIM needs a unique hash_key because classname will be the same for both job and
    // non-job versions.
    m_hash_key = TO_UTF8( GetTitle() );

    // Now all widgets have the size fixed, call FinishDialogSettings
    finishDialogSettings();

    Layout();
    Fit();

    m_browseButton->SetBitmap( KiBitmapBundle( BITMAPS::small_folder ) );
	m_browseButton->Connect( wxEVT_COMMAND_BUTTON_CLICKED,
                             wxCommandEventHandler( DIALOG_GENCAD_EXPORT_OPTIONS::onBrowseClicked ),
                             nullptr, this );
}


DIALOG_GENCAD_EXPORT_OPTIONS::~DIALOG_GENCAD_EXPORT_OPTIONS()
{
    m_browseButton->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED,
                                wxCommandEventHandler( DIALOG_GENCAD_EXPORT_OPTIONS::onBrowseClicked ),
                                nullptr, this );
}


void DIALOG_GENCAD_EXPORT_OPTIONS::onBrowseClicked( wxCommandEvent& aEvent )
{
    wxFileName brdFile( m_frame->GetBoard()->GetFileName() );
    wxString   fileDialogName( wxString::Format( wxS( "%s-gencad" ), brdFile.GetName() ) );

    wxString   path = ExpandEnvVarSubstitutions( m_outputFileName->GetValue(), &Prj() );
    wxFileName fn( Prj().AbsolutePath( path ) );

    wxFileDialog dlg( this, _( "Export GenCAD File" ), fn.GetPath(), fileDialogName,
                      FILEEXT::GencadFileWildcard(), wxFD_SAVE | wxFD_OVERWRITE_PROMPT );
    if( dlg.ShowModal() == wxID_OK )
        m_outputFileName->SetValue( dlg.GetPath() );
}


bool DIALOG_GENCAD_EXPORT_OPTIONS::GetOption( GENCAD_EXPORT_OPT aOption ) const
{
    auto it = m_options.find( aOption );

    if( it == m_options.end() )
    {
        wxASSERT_MSG( false, wxT( "Missing checkbox for an option" ) );
        return false;
    }

    return it->second->IsChecked();
}


wxString DIALOG_GENCAD_EXPORT_OPTIONS::GetFileName() const
{
    return m_outputFileName->GetValue();
}


bool DIALOG_GENCAD_EXPORT_OPTIONS::TransferDataToWindow()
{
    if( !wxDialog::TransferDataToWindow() )
        return false;

    if( !m_job )
    {
        if( m_outputFileName->GetValue().IsEmpty() )
        {
            wxFileName brdFile = m_frame->GetBoard()->GetFileName();
            brdFile.SetExt( wxT( "cad" ) );
            m_outputFileName->SetValue( brdFile.GetFullPath() );
        }
    }
    else
    {
        m_outputFileName->SetValue( m_job->GetConfiguredOutputPath() );
        m_options[FLIP_BOTTOM_PADS]->SetValue( m_job->m_flipBottomPads );
        m_options[UNIQUE_PIN_NAMES]->SetValue( m_job->m_useUniquePins );
        m_options[INDIVIDUAL_SHAPES]->SetValue( m_job->m_useIndividualShapes );
        m_options[USE_AUX_ORIGIN]->SetValue( m_job->m_useDrillOrigin );
        m_options[STORE_ORIGIN_COORDS]->SetValue( m_job->m_storeOriginCoords );
    }

    return true;
}


bool DIALOG_GENCAD_EXPORT_OPTIONS::TransferDataFromWindow()
{
    if( !wxDialog::TransferDataFromWindow() )
        return false;

    if( m_job )
    {
        m_job->SetConfiguredOutputPath( GetFileName() );
        m_job->m_flipBottomPads = GetOption( FLIP_BOTTOM_PADS );
        m_job->m_useUniquePins = GetOption( UNIQUE_PIN_NAMES );
        m_job->m_useIndividualShapes = GetOption( INDIVIDUAL_SHAPES );
        m_job->m_useDrillOrigin = GetOption( USE_AUX_ORIGIN );
        m_job->m_storeOriginCoords = GetOption( STORE_ORIGIN_COORDS );
    }
    else
    {
        wxString fn = GetFileName();

        if( wxFile::Exists( fn ) )
        {
            wxString msg = wxString::Format( _( "File %s already exists." ), fn );
            KIDIALOG dlg( this, msg, _( "Confirmation" ), wxOK | wxCANCEL | wxICON_WARNING );
            dlg.SetOKLabel( _( "Overwrite" ) );
            dlg.DoNotShowCheckbox( __FILE__, __LINE__ );

            return ( dlg.ShowModal() == wxID_OK );
        }
    }

    return true;
}


void DIALOG_GENCAD_EXPORT_OPTIONS::createOptCheckboxes()
{
    std::map<GENCAD_EXPORT_OPT, wxString> opts =
    {
        { FLIP_BOTTOM_PADS,    _( "Flip bottom footprint padstacks" ) },
        { UNIQUE_PIN_NAMES,    _( "Generate unique pin names" ) },
        { INDIVIDUAL_SHAPES,   _( "Generate a new shape for each footprint instance (do not reuse shapes)" ) },
        { USE_AUX_ORIGIN,      _( "Use drill/place file origin as origin" ) },
        { STORE_ORIGIN_COORDS, _( "Save the origin coordinates in the file" ) }
    };

    for( const auto& option : opts )
    {
        wxCheckBox* chkbox = new wxCheckBox( this, wxID_ANY, option.second );
        m_options[option.first] = chkbox;
        m_optsSizer->Add( chkbox );
    }
}
