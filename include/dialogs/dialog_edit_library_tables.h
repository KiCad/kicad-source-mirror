/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2018 KiCad Developers, see AUTHORS.txt for contributors.
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


#ifndef DIALOG_EDIT_LIBRARY_TABLES_H
#define DIALOG_EDIT_LIBRARY_TABLES_H

#include <dialog_shim.h>


class DIALOG_EDIT_LIBRARY_TABLES : public DIALOG_SHIM
{
public:
    bool m_GlobalTableChanged;
    bool m_ProjectTableChanged;

public:
    DIALOG_EDIT_LIBRARY_TABLES( wxWindow* aParent, const wxString& aTitle );

    void InstallPanel( wxPanel* aPanel );

    bool TransferDataToWindow() override;
    bool TransferDataFromWindow() override;

protected:
    wxPanel* m_contentPanel;
};


#endif //DIALOG_EDIT_LIBRARY_TABLES_H
