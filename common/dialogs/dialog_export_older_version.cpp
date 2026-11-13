/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "dialog_export_older_version.h"

#include <confirm.h>
#include <downgrade_target.h>
#include <eda_base_frame.h>
#include <wx/msgdlg.h>

#include "dialog_downgrade_report.h"


DIALOG_EXPORT_OLDER_VERSION::DIALOG_EXPORT_OLDER_VERSION( wxWindow* aParent, const wxArrayString& aTargets,
                                                          const wxArrayString& aVariants,
                                                          const wxString&      aDefaultPath ) :
        DIALOG_EXPORT_OLDER_VERSION_BASE( aParent )
{
    m_targetChoice->Append( aTargets );
    m_targetChoice->SetSelection( 0 );

    m_variantChoice->Append( _( "Base design" ) );
    m_variantChoice->Append( aVariants );
    m_variantChoice->SetSelection( 0 );

    m_targetChoice->Bind( wxEVT_CHOICE,
                          [this]( wxCommandEvent& )
                          {
                              updateVariantChoice();
                          } );
    updateVariantChoice();

    m_destPicker->SetPath( aDefaultPath );

    SetupStandardButtons( { { wxID_OK, _( "Continue" ) } } );

    finishDialogSettings();
}


wxString DIALOG_EXPORT_OLDER_VERSION::GetTarget() const
{
    return m_targetChoice->GetStringSelection();
}


wxString DIALOG_EXPORT_OLDER_VERSION::GetVariant() const
{
    if( !m_variantChoice->IsEnabled() || m_variantChoice->GetSelection() <= 0 )
        return wxEmptyString;

    return m_variantChoice->GetStringSelection();
}


void DIALOG_EXPORT_OLDER_VERSION::updateVariantChoice()
{
    const DOWNGRADE_TARGET* target = FindDowngradeTarget( GetTarget() );
    bool                    supportsVariants = target && target->m_schVersion >= 20250922;
    bool                    canFlatten = target && !supportsVariants && m_variantChoice->GetCount() > 1;

    m_variantLabel->Enable( canFlatten );
    m_variantChoice->Enable( canFlatten );
    m_variantChoice->SetToolTip( supportsVariants ? _( "The target supports variants. All variants are exported." )
                                                  : _( "Choose the variant to flatten into the exported design." ) );
}


wxString DIALOG_EXPORT_OLDER_VERSION::GetPath() const
{
    return m_destPicker->GetPath();
}


bool DIALOG_EXPORT_OLDER_VERSION::GetDropInsteadOfApproximate() const
{
    return m_cbDropInsteadOfApproximate->GetValue();
}


bool DIALOG_EXPORT_OLDER_VERSION::TransferDataFromWindow()
{
    if( m_destPicker->GetPath().IsEmpty() )
    {
        DisplayErrorMessage( this, _( "Choose a destination folder." ) );
        return false;
    }

    return true;
}


void RunExportForOlderVersion( EDA_BASE_FRAME* aFrame, const DOWNGRADE_EXPORT_RUNNER& aRun )
{
    const wxString                       title = _( "Export for Older KiCad Version" );
    const std::vector<DOWNGRADE_TARGET>& targets = GetDowngradeTargets();
    wxArrayString                        names;

    for( const DOWNGRADE_TARGET& target : targets )
        names.Add( target.m_name );

    // Probe against the oldest target. It drops the most, so its variant list covers the
    // picker no matter which target the user chooses.
    DOWNGRADE_EXPORT_PASS   probe;
    DOWNGRADE_EXPORT_RESULT result;

    probe.m_target = targets.back().m_name;
    probe.m_dryRun = true;

    if( !aRun( probe, result ) )
    {
        wxMessageBox( result.m_error, title, wxOK | wxICON_ERROR, aFrame );
        return;
    }

    wxArrayString variantNames;

    for( const wxString& name : result.m_variantsFound )
        variantNames.Add( name );

    DIALOG_EXPORT_OLDER_VERSION setupDlg( aFrame, names, variantNames, aFrame->GetMruPath() );

    if( setupDlg.ShowModal() != wxID_OK )
        return;

    DOWNGRADE_EXPORT_PASS pass;
    pass.m_target = setupDlg.GetTarget();
    pass.m_variant = setupDlg.GetVariant();
    pass.m_folder = setupDlg.GetPath();
    pass.m_dropInsteadOfApproximate = setupDlg.GetDropInsteadOfApproximate();
    pass.m_dryRun = true;

    result = DOWNGRADE_EXPORT_RESULT();

    if( !aRun( pass, result ) )
    {
        wxMessageBox( result.m_error, title, wxOK | wxICON_ERROR, aFrame );
        return;
    }

    if( result.m_report.IsBlocked() || result.m_report.IsLossy() )
    {
        DIALOG_DOWNGRADE_REPORT reportDlg( aFrame, pass.m_target, result.m_report );

        if( reportDlg.ShowModal() != wxID_OK || result.m_report.IsBlocked() )
            return;
    }

    pass.m_dryRun = false;
    result = DOWNGRADE_EXPORT_RESULT();

    if( !aRun( pass, result ) )
    {
        wxMessageBox( result.m_error, title, wxOK | wxICON_ERROR, aFrame );
        return;
    }

    wxMessageBox( wxString::Format( _( "Exported to '%s'." ), pass.m_folder ), title, wxOK | wxICON_INFORMATION,
                  aFrame );
}
