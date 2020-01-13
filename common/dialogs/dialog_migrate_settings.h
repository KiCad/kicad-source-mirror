/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2020 Jon Evans <jon@craftyjon.com>
 * Copyright (C) 2020 KiCad Developers, see AUTHORS.txt for contributors.
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

#ifndef _DIALOG_MIGRATE_SETTINGS_H
#define _DIALOG_MIGRATE_SETTINGS_H

#include "dialog_migrate_settings_base.h"


class SETTINGS_MANAGER;


class DIALOG_MIGRATE_SETTINGS : public DIALOG_MIGRATE_SETTINGS_BASE
{
public:
    DIALOG_MIGRATE_SETTINGS( SETTINGS_MANAGER* aManager );

    ~DIALOG_MIGRATE_SETTINGS() override;

    bool TransferDataToWindow() override;

    bool TransferDataFromWindow() override;

protected:

    void OnPrevVerSelected( wxCommandEvent& event ) override;

    void OnPathChanged( wxCommandEvent& event ) override;

    void OnPathDefocused( wxFocusEvent& event ) override;

    void OnChoosePath( wxCommandEvent& event ) override;

    void OnDefaultSelected( wxCommandEvent& event ) override;

private:

    bool validatePath();

    void showPathError( bool aShow = true );

    SETTINGS_MANAGER* m_manager;
};

#endif
