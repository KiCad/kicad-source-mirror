/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2017-2020 KiCad Developers, see AUTHORS.txt for contributors.
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


#ifndef PAGED_DIALOG_H
#define PAGED_DIALOG_H

#include <dialog_shim.h>
#include <wx/treebook.h>


class WX_INFOBAR;


class PAGED_DIALOG : public DIALOG_SHIM
{
private:
    wxString    m_title;

    bool        m_dirty;

    wxString    m_errorMessage;
    wxWindow*   m_errorCtrl;    // the control associated with m_errorMessage
    int         m_errorRow;     // the row if m_errorCtrl is a grid
    int         m_errorCol;     // the column if m_errorCtrl is a grid

    wxBoxSizer* m_buttonsSizer;

    std::vector<bool> m_macHack;

public:
    PAGED_DIALOG( wxWindow* aParent, const wxString& aTitle, bool aUseReset = false,
                  const wxString& aAuxiliaryAction = wxEmptyString );
    ~PAGED_DIALOG() override;

    wxTreebook* GetTreebook() { return m_treebook; }

    void SetInitialPage( const wxString& aPage, const wxString& aParentPage = wxEmptyString );

    void SetModified() { m_modified = true; }

    void SetError( const wxString& aMessage, const wxString& aPageName, int aCtrlId, int aRow = -1,
                   int aCol = -1 );

    void SetError( const wxString& aMessage, wxWindow* aPage, wxWindow* aCtrl, int aRow = -1,
                   int aCol = -1 );

    void AddAuxiliaryAction( const wxString& aTitle, const wxString& aTooltip,
                             std::function<void( wxCommandEvent& )> aHandler );

protected:
    void finishInitialization();

    bool TransferDataToWindow() override;
    bool TransferDataFromWindow() override;

    void OnClose( wxCloseEvent& event );
    void OnCancel( wxCommandEvent& event );
    virtual void OnAuxiliaryAction( wxCommandEvent& event ) { event.Skip(); }
    void OnResetButton( wxCommandEvent& aEvent );
    void OnUpdateUI( wxUpdateUIEvent& event );
    void OnPageChange( wxBookCtrlEvent& event );
    void OnValidate( wxCommandEvent& aEvent );

    wxTreebook* m_treebook;
    wxButton*   m_auxiliaryButton;
    wxButton*   m_resetButton;
    wxButton*   m_cancelButton;
    WX_INFOBAR* m_infoBar;
};


#endif //PAGED_DIALOG_H
