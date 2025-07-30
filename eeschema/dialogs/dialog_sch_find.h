/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2010 Wayne Stambaugh <stambaughw@gmail.com>
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

#ifndef DIALOG_SCH_FIND_H
#define DIALOG_SCH_FIND_H

#include "dialog_sch_find_base.h"

#include <wx/fdrepdlg.h>          // Use the wxFindReplaceDialog events, data, and enums.

class SCH_BASE_FRAME;
class SCH_FIND_REPLACE_TOOL;
struct SCH_SEARCH_DATA;


class DIALOG_SCH_FIND : public DIALOG_SCH_FIND_BASE
{
public:
    DIALOG_SCH_FIND( SCH_BASE_FRAME* aParent, SCH_SEARCH_DATA* aData,
                     const wxPoint& aPosition = wxDefaultPosition,
                     const wxSize& aSize = wxDefaultSize, int aStyle = 0 );
    ~DIALOG_SCH_FIND() override = default;

    void SetFindEntries( const wxArrayString& aEntries, const wxString& aFindString );
    wxArrayString GetFindEntries() const;

    void SetReplaceEntries( const wxArrayString& aEntries );
    wxArrayString GetReplaceEntries() const { return m_comboReplace->GetStrings(); }

protected:
    // Handlers for DIALOG_SCH_FIND_BASE events.
    void OnClose( wxCloseEvent& aEvent ) override;
    void OnCancel( wxCommandEvent& aEvent ) override;
    void OnSearchForSelect( wxCommandEvent& aEvent ) override;
    void OnSearchForText( wxCommandEvent& aEvent ) override;
    void OnSearchForEnter( wxCommandEvent& event ) override;
    void OnReplaceWithSelect( wxCommandEvent& aEvent ) override;
    void OnReplaceWithText( wxCommandEvent& aEvent ) override;
    void OnReplaceWithEnter( wxCommandEvent& event ) override;
    void OnOptions( wxCommandEvent& event ) override;
    void OnUpdateReplaceUI( wxUpdateUIEvent& aEvent ) override;
    void OnUpdateReplaceAllUI( wxUpdateUIEvent& aEvent ) override;
    void OnIdle( wxIdleEvent& event ) override;
    void OnFind( wxCommandEvent& aEvent ) override;
    void OnReplace( wxCommandEvent& aEvent ) override;
    void onShowSearchPanel( wxHyperlinkEvent& event ) override;

    // Rebuild the search flags from dialog settings
    void updateFlags();

protected:
    SCH_BASE_FRAME*        m_frame;
    SCH_FIND_REPLACE_TOOL* m_findReplaceTool;
    SCH_SEARCH_DATA*       m_findReplaceData;
    bool                   m_findDirty;

    DECLARE_NO_COPY_CLASS( DIALOG_SCH_FIND )
};


#endif // DIALOG_SCH_FIND_H
