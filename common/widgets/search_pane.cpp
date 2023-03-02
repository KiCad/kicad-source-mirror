/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2022-2023 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <widgets/search_pane.h>
#include <widgets/search_pane_tab.h>
#include <eda_draw_frame.h>
#include <kiway.h>


SEARCH_PANE::SEARCH_PANE( EDA_DRAW_FRAME* aFrame ) :
	SEARCH_PANE_BASE( aFrame )
{
}


SEARCH_PANE::~SEARCH_PANE()
{
}


void SEARCH_PANE::OnLanguageChange()
{
    for( size_t i = 0; i < m_notebook->GetPageCount(); ++i )
    {
        wxWindow* page = m_notebook->GetPage( i );
        SEARCH_PANE_TAB* tab = dynamic_cast<SEARCH_PANE_TAB*>( page );

        wxCHECK( tab, /* void */ );

        tab->RefreshColumnNames();
        m_notebook->SetPageText( i, wxGetTranslation( tab->GetSearchHandler()->GetName() ) );
    }
}


void SEARCH_PANE::AddSearcher( SEARCH_HANDLER* aHandler )
{
    SEARCH_PANE_TAB* tab = new SEARCH_PANE_TAB( aHandler, m_notebook );

    m_notebook->AddPage( tab, wxGetTranslation( aHandler->GetName() ) );
    m_handlers.push_back( aHandler );
    m_tabs.push_back( tab );
}


void SEARCH_PANE::RefreshSearch()
{
    SEARCH_PANE_TAB* tab = GetCurrentTab();

    if( tab )
        tab->Search( m_lastQuery );
}


void SEARCH_PANE::ClearAllResults()
{
    for( SEARCH_PANE_TAB* tab : m_tabs )
    {
        tab->Clear();
    }
}


void SEARCH_PANE::OnSearchTextEntry( wxCommandEvent& aEvent )
{
    wxString query = m_searchCtrl1->GetValue();
    m_lastQuery = query;

    RefreshSearch();
}


void SEARCH_PANE::FocusSearch()
{
    m_searchCtrl1->SetFocus();
}


void SEARCH_PANE::OnNotebookPageChanged( wxBookCtrlEvent& aEvent )
{
    SEARCH_PANE_TAB* tab = GetCurrentTab();

    if( tab )
        tab->Search( m_lastQuery );
}


SEARCH_PANE_TAB* SEARCH_PANE::GetCurrentTab() const
{
    return dynamic_cast<SEARCH_PANE_TAB*>( m_notebook->GetCurrentPage() );
}
