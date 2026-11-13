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

#pragma once

#include <functional>
#include <vector>

#include <compatibility_report.h>
#include <kiway.h>
#include <reporter.h>
#include <dialogs/dialog_export_older_version_base.h>

class EDA_BASE_FRAME;

/// What one pass of an export is asked to do.
struct DOWNGRADE_EXPORT_PASS
{
    wxString m_target;  ///< Target release name
    wxString m_variant; ///< Variant to flatten, empty for the base design
    wxString m_folder;  ///< Destination, empty on a dry run
    bool     m_dropInsteadOfApproximate = false;
    bool     m_dryRun = false; ///< Classify only, write nothing
};


/// What the flow needs back from a pass.
struct DOWNGRADE_EXPORT_RESULT
{
    COMPATIBILITY_REPORT  m_report;
    std::vector<wxString> m_variantsFound; ///< A dry run fills these so the picker can offer them
    wxString              m_error;
};


/// Fill in the parts of a downgrade job that every editor sets the same way, run it, and hand
/// back its report. Consent for any loss was already taken through the report dialog.
template <typename JOB_T>
bool RunDowngradeJob( KIWAY& aKiway, KIWAY::FACE_T aFace, JOB_T& aJob, const DOWNGRADE_EXPORT_PASS& aPass,
                      DOWNGRADE_EXPORT_RESULT& aResult )
{
    aJob.m_target = aPass.m_target;
    aJob.m_dropInsteadOfApproximate = aPass.m_dropInsteadOfApproximate;
    aJob.m_dryRun = aPass.m_dryRun;
    aJob.m_force = true;

    WX_STRING_REPORTER reporter;

    if( aKiway.ProcessJob( aFace, &aJob, &reporter ) != 0 )
    {
        aResult.m_error = reporter.GetMessages();
        return false;
    }

    aResult.m_report = aJob.m_report;
    return true;
}


using DOWNGRADE_EXPORT_RUNNER = std::function<bool( const DOWNGRADE_EXPORT_PASS&, DOWNGRADE_EXPORT_RESULT& )>;


void RunExportForOlderVersion( EDA_BASE_FRAME* aFrame, const DOWNGRADE_EXPORT_RUNNER& aRun );

class DIALOG_EXPORT_OLDER_VERSION : public DIALOG_EXPORT_OLDER_VERSION_BASE
{
public:
    DIALOG_EXPORT_OLDER_VERSION( wxWindow* aParent, const wxArrayString& aTargets, const wxArrayString& aVariants,
                                 const wxString& aDefaultPath );

    wxString GetTarget() const;

    /// The variant to flatten, or empty for the base design.
    wxString GetVariant() const;

    wxString GetPath() const;

    bool GetDropInsteadOfApproximate() const;

private:
    void updateVariantChoice();

    bool TransferDataFromWindow() override;
};
