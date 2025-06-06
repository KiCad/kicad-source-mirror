/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2024 Mark Roszko <mark.roszko@gmail.com>
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
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <dialogs/dialog_executecommand_job_settings_base.h>

class JOB_SPECIAL_EXECUTE;
class SCINTILLA_TRICKS;


class DIALOG_EXECUTECOMMAND_JOB_SETTINGS: public DIALOG_EXECUTECOMMAND_JOB_SETTINGS_BASE
{
public:
    DIALOG_EXECUTECOMMAND_JOB_SETTINGS( wxWindow* aParent, JOB_SPECIAL_EXECUTE* aJob );
    ~DIALOG_EXECUTECOMMAND_JOB_SETTINGS();

    bool TransferDataFromWindow() override;
    bool TransferDataToWindow() override;

private:
    void onSizeGrid( wxSizeEvent& event ) override;
    void adjustPathSubsGridColumns( int aWidth );
    void OnRecordOutputClicked( wxCommandEvent& event ) override;

    /// Populate the readonly environment variable table with names and values
    /// by examining the script and path.
    void populateEnvironReadOnlyTable();

private:
    JOB_SPECIAL_EXECUTE* m_job;

    SCINTILLA_TRICKS*    m_scintillaTricks;
};