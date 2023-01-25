/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2022 KiCad Developers, see AUTHORS.txt for contributors.
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

#ifndef SEARCH_PANE_TAB_H
#define SEARCH_PANE_TAB_H

#include <vector>
#include <wx/listctrl.h>
#include <wx/sizer.h>
#include <wx/panel.h>

class SEARCH_HANDLER;


class SEARCH_PANE_LISTVIEW : public wxListView
{
public:
    SEARCH_PANE_LISTVIEW( SEARCH_HANDLER* handler,
                          wxWindow* parent, wxWindowID winid = wxID_ANY,
                          const wxPoint& pos = wxDefaultPosition,
                          const wxSize& size = wxDefaultSize );

    virtual ~SEARCH_PANE_LISTVIEW();

    void RefreshColumnNames();

protected:
    wxString OnGetItemText( long item, long column ) const override;
    void     OnItemSelected( wxListEvent& aEvent );
    void     OnItemActivated( wxListEvent& aEvent );
    void     OnItemDeselected( wxListEvent& aEvent );

    void GetSelectRowsList( std::vector<long>& aSelectedList );

private:
    SEARCH_HANDLER*       m_handler;
};


class SEARCH_PANE_TAB : public wxPanel
{
public:
    SEARCH_PANE_TAB( SEARCH_HANDLER* handler, wxWindow* parent, wxWindowID aId = wxID_ANY,
                     const wxPoint& aLocation = wxDefaultPosition,
                     const wxSize&  aSize = wxDefaultSize );

    void Search( wxString& query );
    void Clear();
    void RefreshColumnNames();

    SEARCH_HANDLER* GetSearchHandler() const { return m_handler; }

private:
    SEARCH_PANE_LISTVIEW* m_listView;
    SEARCH_HANDLER*       m_handler;
};

#endif
