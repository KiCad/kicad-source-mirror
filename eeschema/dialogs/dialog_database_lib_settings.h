/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2023 Jon Evans <jon@craftyjon.com>
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

#ifndef KICAD_DIALOG_DATABASE_LIB_SETTINGS_H
#define KICAD_DIALOG_DATABASE_LIB_SETTINGS_H

#include "dialog_database_lib_settings_base.h"

class SCH_IO_DATABASE;

class DIALOG_DATABASE_LIB_SETTINGS : public DIALOG_DATABASE_LIB_SETTINGS_BASE
{
public:
    DIALOG_DATABASE_LIB_SETTINGS( wxWindow* aParent, SCH_IO_DATABASE* aPlugin );

    virtual ~DIALOG_DATABASE_LIB_SETTINGS() {}

    bool TransferDataFromWindow() override;
    bool TransferDataToWindow() override;

protected:
    void OnDSNSelected( wxCommandEvent& aEvent ) override;
    void OnConnectionStringSelected( wxCommandEvent& aEvent ) override;
    void OnBtnTest( wxCommandEvent& aEvent ) override;
    void OnBtnReloadConfig( wxCommandEvent& aEvent ) override;

private:
    bool hasPotentiallyValidConfig();

    SCH_IO_DATABASE* m_plugin;
};

#endif //KICAD_DIALOG_DATABASE_LIB_SETTINGS_H
