/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2017-2023 KiCad Developers, see AUTHORS.txt for contributors.
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
#include <widgets/wx_treebook.h>


class WX_INFOBAR;
class WX_TREEBOOK;

class PAGED_DIALOG : public DIALOG_SHIM
{
public:
    PAGED_DIALOG( wxWindow* aParent, const wxString& aTitle, bool aShowReset, bool aShowOpenFolder,
                  const wxString& aAuxiliaryAction = wxEmptyString,
                  const wxSize&   aInitialSize = wxDefaultSize );
    ~PAGED_DIALOG() override;

    WX_TREEBOOK* GetTreebook() { return m_treebook; }

    void SetInitialPage( const wxString& aPage, const wxString& aParentPage = wxEmptyString );

    void SetModified() { m_modified = true; }

    void SetError( const wxString& aMessage, const wxString& aPageName, int aCtrlId, int aRow = -1,
                   int aCol = -1 );

    void SetError( const wxString& aMessage, wxWindow* aPage, wxWindow* aCtrl, int aRow = -1,
                   int aCol = -1 );

    void UpdateResetButton( int aPage );

    static PAGED_DIALOG* GetDialog( wxWindow* aWindow );

protected:
    void finishInitialization();

    bool TransferDataToWindow() override;
    bool TransferDataFromWindow() override;

    virtual void onAuxiliaryAction( wxCommandEvent& aEvent ) { aEvent.Skip(); }
    virtual void onResetButton( wxCommandEvent& aEvent );
    virtual void onOpenPreferencesButton( wxCommandEvent& aEvent );
    virtual void onPageChanged( wxBookCtrlEvent& aEvent );
    virtual void onPageChanging( wxBookCtrlEvent& aEvent );
    virtual void onCharHook( wxKeyEvent& aEvent );

    WX_TREEBOOK* m_treebook;
    wxButton*    m_auxiliaryButton;
    wxButton*    m_resetButton;
    wxButton*    m_openPrefsDirButton;
    wxButton*    m_cancelButton;
    WX_INFOBAR*  m_infoBar;

private:
    wxString    m_title;

    wxBoxSizer* m_buttonsSizer;

    std::vector<bool> m_macHack;
};


#endif //PAGED_DIALOG_H
