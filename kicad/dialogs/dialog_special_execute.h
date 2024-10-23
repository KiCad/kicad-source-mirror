/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2024 Mark Roszko <mark.roszko@gmail.com>
 * Copyright (C) 2024 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <dialogs/panel_jobs_base.h>

class JOB_SPECIAL_EXECUTE;

class DIALOG_SPECIAL_EXECUTE: public DIALOG_SPECIAL_EXECUTE_BASE
{
public:
    DIALOG_SPECIAL_EXECUTE( wxWindow* aParent, JOB_SPECIAL_EXECUTE* aJob );

    bool TransferDataFromWindow() override;
    bool TransferDataToWindow() override;

private:
    JOB_SPECIAL_EXECUTE* m_job;

    void OnRecordOutputClicked( wxCommandEvent& event ) override;
};