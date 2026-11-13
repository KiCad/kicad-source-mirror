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

#include <compatibility_report.h>
#include <dialogs/dialog_downgrade_report_base.h>

/// Shows what an export will approximate, drop, or refuse. When the report blocks,
/// the export button is not offered at all.
class DIALOG_DOWNGRADE_REPORT : public DIALOG_DOWNGRADE_REPORT_BASE
{
public:
    DIALOG_DOWNGRADE_REPORT( wxWindow* aParent, const wxString& aTargetName, const COMPATIBILITY_REPORT& aReport );

private:
    void OnSaveReport( wxCommandEvent& aEvent ) override;

    const wxString m_reportText;
    wxString       m_reportFileName;
};
