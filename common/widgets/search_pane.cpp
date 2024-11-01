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

#include "widgets/search_pane.h"


#include <tool/action_menu.h>
#include <settings/app_settings.h>
#include <eda_draw_frame.h>
#include <bitmaps.h>
#include <kiway.h>
#include <widgets/search_pane_tab.h>


#define ID_TOGGLE_ZOOM_TO_SELECTION 14000
#define ID_TOGGLE_PAN_TO_SELECTION 14001


class SEARCH_PANE_MENU : public ACTION_MENU
{
public:
    SEARCH_PANE_MENU( EDA_DRAW_FRAME& aFrame ) : ACTION_MENU( true, nullptr ), m_frame( aFrame )
    {
        Add( _( "Zoom to Selection" ), _( "Toggle zooming to selections in the search pane" ),
             ID_TOGGLE_ZOOM_TO_SELECTION, BITMAPS::zoom_fit_to_objects, true );
        Add( _( "Pan to Selection" ), _( "Toggle panning to selections in the search pane" ),
             ID_TOGGLE_PAN_TO_SELECTION, BITMAPS::zoom_center_on_screen, true );

        updateZoomPanCheckboxes();
    }


    OPT_TOOL_EVENT eventHandler( const wxMenuEvent& aEvent ) override
    {
        APP_SETTINGS_BASE::SEARCH_PANE& settings = m_frame.config()->m_SearchPane;
        const int                       id = aEvent.GetId();
        const wxMenuItem*               item = FindItem( id );

        switch( id )
        {
        case ID_TOGGLE_ZOOM_TO_SELECTION:
            settings.selection_zoom =
                    item->IsChecked() ? APP_SETTINGS_BASE::SEARCH_PANE::SELECTION_ZOOM::ZOOM
                                      : APP_SETTINGS_BASE::SEARCH_PANE::SELECTION_ZOOM::NONE;
            updateZoomPanCheckboxes();
            break;
        case ID_TOGGLE_PAN_TO_SELECTION:
            settings.selection_zoom =
                    item->IsChecked() ? APP_SETTINGS_BASE::SEARCH_PANE::SELECTION_ZOOM::PAN
                                      : APP_SETTINGS_BASE::SEARCH_PANE::SELECTION_ZOOM::NONE;
            updateZoomPanCheckboxes();
            break;
        }
        return OPT_TOOL_EVENT();
    }

private:
    void updateZoomPanCheckboxes()
    {
        APP_SETTINGS_BASE::SEARCH_PANE& settings = m_frame.config()->m_SearchPane;

        wxMenuItem* zoomCb = FindItem( ID_TOGGLE_ZOOM_TO_SELECTION );
        wxMenuItem* panCb = FindItem( ID_TOGGLE_PAN_TO_SELECTION );

        zoomCb->Check( settings.selection_zoom
                       == APP_SETTINGS_BASE::SEARCH_PANE::SELECTION_ZOOM::ZOOM );
        panCb->Check( settings.selection_zoom
                      == APP_SETTINGS_BASE::SEARCH_PANE::SELECTION_ZOOM::PAN );
    }

    EDA_DRAW_FRAME& m_frame;
};


SEARCH_PANE::SEARCH_PANE( EDA_DRAW_FRAME* aFrame ) :
	SEARCH_PANE_BASE( aFrame ),
    m_frame( aFrame )
{
    m_frame->Bind( EDA_LANG_CHANGED, &SEARCH_PANE::OnLanguageChange, this );

    m_menu = new SEARCH_PANE_MENU( *m_frame );

    m_menuButton->SetBitmap( KiBitmapBundle( BITMAPS::config ) );
    m_menuButton->Bind( wxEVT_LEFT_DOWN,
            [&]( wxMouseEvent& event )
            {
                PopupMenu( m_menu );
            } );
}


SEARCH_PANE::~SEARCH_PANE()
{
    m_frame->Unbind( EDA_LANG_CHANGED, &SEARCH_PANE::OnLanguageChange, this );

    delete m_menu;
}


void SEARCH_PANE::OnLanguageChange( wxCommandEvent& aEvent )
{
    m_searchCtrl1->SetDescriptiveText( _( "Search" ) );

    for( size_t i = 0; i < m_notebook->GetPageCount(); ++i )
    {
        wxWindow* page = m_notebook->GetPage( i );
        SEARCH_PANE_TAB* tab = dynamic_cast<SEARCH_PANE_TAB*>( page );

        wxCHECK( tab, /* void */ );

        tab->RefreshColumnNames();
        m_notebook->SetPageText( i, wxGetTranslation( tab->GetSearchHandler()->GetName() ) );
    }

    aEvent.Skip();
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
