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

#ifndef SEARCH_PANE_H
#define SEARCH_PANE_H

#include <memory>
#include <vector>

#include <widgets/search_pane_base.h>
#include <wx/listbase.h>


class wxAuiManagerEvent;
class ACTION_MENU;
class EDA_DRAW_FRAME;
class SEARCH_PANE_TAB;

class SEARCH_HANDLER
{
public:
    SEARCH_HANDLER( const wxString& aName ) :
            m_name( aName )
    {}

    virtual ~SEARCH_HANDLER()
    {}

    wxString GetName() const { return m_name; }

    std::vector<std::tuple<wxString, int, wxListColumnFormat>> GetColumns() const
    {
        return m_columns;
    }

    virtual int Search( const wxString& string ) = 0;
    virtual wxString GetResultCell( int row, int col ) = 0;
    virtual void Sort( int aCol, bool aAscending, std::vector<long>* aSelection ) = 0;

    virtual void SelectItems( std::vector<long>& aItemRows ) {}
    virtual void ActivateItem( long aItemRow ) {}

protected:
    wxString                                                   m_name;
    std::vector<std::tuple<wxString, int, wxListColumnFormat>> m_columns;
};


class SEARCH_PANE : public SEARCH_PANE_BASE
{
public:
    SEARCH_PANE( EDA_DRAW_FRAME* aFrame );
    virtual ~SEARCH_PANE();

    // We own at least one list of raw pointers.  Don't let the compiler fill in copy c'tors that
    // will only land us in trouble.
    SEARCH_PANE( const SEARCH_PANE& ) = delete;
    SEARCH_PANE& operator=( const SEARCH_PANE& ) = delete;

    void AddSearcher( const std::shared_ptr<SEARCH_HANDLER>& aHandler );
    void OnSearchTextEntry( wxCommandEvent& aEvent ) override;
    void OnNotebookPageChanged( wxBookCtrlEvent& aEvent ) override;

    void RefreshSearch();
    void FocusSearch();
    void ClearAllResults();

    void OnCharHook( wxKeyEvent& aEvent );

protected:
    void             OnLanguageChange( wxCommandEvent& aEvent );
    SEARCH_PANE_TAB* GetCurrentTab() const;
    void             OnClosed( wxAuiManagerEvent& aEvent );

private:
    std::vector<std::shared_ptr<SEARCH_HANDLER>> m_handlers;
    std::vector<SEARCH_PANE_TAB*>                m_tabs;
    wxString                                     m_lastQuery;
    EDA_DRAW_FRAME*                              m_frame;
    ACTION_MENU*                                 m_menu;
};

#endif
