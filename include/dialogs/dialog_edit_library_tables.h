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
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */


#ifndef DIALOG_EDIT_LIBRARY_TABLES_H
#define DIALOG_EDIT_LIBRARY_TABLES_H

#include <dialog_shim.h>
#include <wx/panel.h>
#include <widgets/wx_infobar.h>


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

    void ShowInfoBarError( const wxString& aErrorMsg, bool aShowCloseButton = false,
                           WX_INFOBAR::MESSAGE_TYPE aType = WX_INFOBAR::MESSAGE_TYPE::GENERIC )
    {
        m_infoBar->RemoveAllButtons();

        if( aShowCloseButton )
            m_infoBar->AddCloseButton();

        m_infoBar->ShowMessageFor( aErrorMsg, 8000, wxICON_ERROR, aType );
    }

protected:
    WX_INFOBAR* m_infoBar;
    wxPanel*    m_contentPanel;
};


#endif //DIALOG_EDIT_LIBRARY_TABLES_H
