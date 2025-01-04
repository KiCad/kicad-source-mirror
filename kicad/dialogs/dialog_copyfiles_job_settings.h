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

#include "dialog_copyfiles_job_settings_base.h"

class JOB_SPECIAL_COPYFILES;

class DIALOG_COPYFILES_JOB_SETTINGS : public DIALOG_COPYFILES_JOB_SETTINGS_BASE
{
public:
    DIALOG_COPYFILES_JOB_SETTINGS( wxWindow* aParent, JOB_SPECIAL_COPYFILES* aJob );


    bool TransferDataToWindow() override;
    bool TransferDataFromWindow() override;

private:
    JOB_SPECIAL_COPYFILES* m_job;
};


