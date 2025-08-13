/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2024 KiCad Developers
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

#ifndef DIALOG_GENERATE_DATABASE_CONNECTION_H
#define DIALOG_GENERATE_DATABASE_CONNECTION_H

#include <vector>

#include <dialog_shim.h>
#include <database/database_lib_settings.h>

class wxChoice;
class wxTextCtrl;
class wxSpinCtrl;
class wxButton;

/**
 * Dialog for generating database connection settings.
 */
class DIALOG_GENERATE_DATABASE_CONNECTION : public DIALOG_SHIM
{
public:
    DIALOG_GENERATE_DATABASE_CONNECTION( wxWindow* aParent );

    DATABASE_SOURCE GetSource() const;

private:
    void OnDSNChanged( wxCommandEvent& aEvent );
    void OnTest( wxCommandEvent& aEvent );
    void UpdateControls();

private:
    wxChoice*  m_dsnChoice;
    wxTextCtrl* m_userCtrl;
    wxTextCtrl* m_passCtrl;
    wxSpinCtrl* m_timeoutCtrl;
    wxTextCtrl* m_connStrCtrl;
    wxButton*   m_testButton;
    wxChoice*   m_tableChoice;
};

#endif // DIALOG_GENERATE_DATABASE_CONNECTION_H
