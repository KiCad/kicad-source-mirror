/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#pragma once

#include <kiid.h>
#include <dialogs/dialog_book_reporter_base.h>

class KIWAY_PLAYER;
class WX_HTML_REPORT_BOX;
class wxHtmlLinkEvent;

/**
 * Event sent to parent when dialog is mode-less.
 */
wxDECLARE_EVENT( EDA_EVT_CLOSE_DIALOG_BOOK_REPORTER, wxCommandEvent );

class DIALOG_BOOK_REPORTER : public DIALOG_BOOK_REPORTER_BASE
{
public:
    DIALOG_BOOK_REPORTER( KIWAY_PLAYER* aParent, const wxString& aName,
                          const wxString& aDialogTitle );

    void OnClose( wxCloseEvent& aEvent ) override;
    void OnApply( wxCommandEvent& event ) override;
  	void OnOK( wxCommandEvent& event ) override;

    void OnErrorLinkClicked( wxHtmlLinkEvent& aEvent );

    void DeleteAllPages();

    WX_HTML_REPORT_BOX* AddHTMLPage( const wxString& aTitle );

    wxPanel* AddBlankPage( const wxString& aTitle );

    int GetPageCount() const;

    KIID GetUserItemID() const { return m_userItemID; }
    void SetUserItemID( const KIID& aID ) { m_userItemID = aID; }

protected:
    KIWAY_PLAYER* m_frame;
    KIID          m_userItemID;
};
