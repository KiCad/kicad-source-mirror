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

#include "widgets/search_pane.h"

#include <memory>
#include <tool/action_menu.h>
#include <tool/tool_dispatcher.h>
#include <tool/tool_manager.h>
#include <settings/app_settings.h>
#include <eda_draw_frame.h>
#include <bitmaps.h>
#include <kiway.h>
#include <widgets/search_pane_tab.h>
#include <widgets/bitmap_button.h>


#define ID_TOGGLE_ZOOM_TO_SELECTION    14000
#define ID_TOGGLE_PAN_TO_SELECTION     14001
#define ID_TOGGLE_SEARCH_HIDDEN_FIELDS 14002
#define ID_TOGGLE_SEARCH_METADATA      14003


class SEARCH_PANE_MENU : public ACTION_MENU
{
public:
    SEARCH_PANE_MENU( SEARCH_PANE* aSearchPane, EDA_DRAW_FRAME& aFrame ) :
            ACTION_MENU( true, nullptr ),
            m_frame( aFrame ),
            m_searchPane( aSearchPane )
    {
        Add( _( "Zoom to Selection" ), _( "Toggle zooming to selections in the search pane" ),
             ID_TOGGLE_ZOOM_TO_SELECTION, BITMAPS::zoom_fit_to_objects, true );
        Add( _( "Pan to Selection" ), _( "Toggle panning to selections in the search pane" ),
             ID_TOGGLE_PAN_TO_SELECTION, BITMAPS::zoom_center_on_screen, true );

        AppendSeparator();
        Add( _( "Search Hidden Fields" ), wxEmptyString,
             ID_TOGGLE_SEARCH_HIDDEN_FIELDS, BITMAPS::invisible_text, true );
        Add( _( "Search Metadata" ), _( "Search library links, descriptions and keywords" ),
             ID_TOGGLE_SEARCH_METADATA, BITMAPS::library, true );

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
            settings.selection_zoom = item->IsChecked() ? APP_SETTINGS_BASE::SEARCH_PANE::SELECTION_ZOOM::ZOOM
                                                        : APP_SETTINGS_BASE::SEARCH_PANE::SELECTION_ZOOM::NONE;
            updateZoomPanCheckboxes();
            break;

        case ID_TOGGLE_PAN_TO_SELECTION:
            settings.selection_zoom = item->IsChecked() ? APP_SETTINGS_BASE::SEARCH_PANE::SELECTION_ZOOM::PAN
                                                        : APP_SETTINGS_BASE::SEARCH_PANE::SELECTION_ZOOM::NONE;
            updateZoomPanCheckboxes();
            break;

        case ID_TOGGLE_SEARCH_HIDDEN_FIELDS:
            settings.search_hidden_fields = item->IsChecked();
            updateZoomPanCheckboxes();
            m_searchPane->RefreshSearch();
            break;

        case ID_TOGGLE_SEARCH_METADATA:
            settings.search_metadata = item->IsChecked();
            updateZoomPanCheckboxes();
            m_searchPane->RefreshSearch();
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
        wxMenuItem* hiddenFieldsCb = FindItem( ID_TOGGLE_SEARCH_HIDDEN_FIELDS );
        wxMenuItem* metadataCb = FindItem( ID_TOGGLE_SEARCH_METADATA );

        zoomCb->Check( settings.selection_zoom == APP_SETTINGS_BASE::SEARCH_PANE::SELECTION_ZOOM::ZOOM );
        panCb->Check( settings.selection_zoom == APP_SETTINGS_BASE::SEARCH_PANE::SELECTION_ZOOM::PAN );
        hiddenFieldsCb->Check( settings.search_hidden_fields );
        metadataCb->Check( settings.search_metadata );
    }

private:
    EDA_DRAW_FRAME& m_frame;
    SEARCH_PANE*    m_searchPane;
};


SEARCH_PANE::SEARCH_PANE( EDA_DRAW_FRAME* aFrame ) :
        SEARCH_PANE_BASE( aFrame ),
        m_frame( aFrame )
{
    m_frame->Bind( EDA_LANG_CHANGED, &SEARCH_PANE::OnLanguageChange, this );

    m_menu = new SEARCH_PANE_MENU( this, *m_frame );

    m_menuButton->SetBitmap( KiBitmapBundle( BITMAPS::config ) );
    m_menuButton->Bind( wxEVT_LEFT_DOWN,
            [&]( wxMouseEvent& event )
            {
                PopupMenu( m_menu );
            } );

    m_frame->Bind( wxEVT_AUI_PANE_CLOSE, &SEARCH_PANE::OnClosed, this );
    Bind( wxEVT_CHAR_HOOK, &SEARCH_PANE::OnCharHook, this );
}


SEARCH_PANE::~SEARCH_PANE()
{
    m_frame->Unbind( wxEVT_AUI_PANE_CLOSE, &SEARCH_PANE::OnClosed, this );
    m_frame->Unbind( EDA_LANG_CHANGED, &SEARCH_PANE::OnLanguageChange, this );
    Unbind( wxEVT_CHAR_HOOK, &SEARCH_PANE::OnCharHook, this );

    m_handlers.clear();

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


void SEARCH_PANE::AddSearcher( const std::shared_ptr<SEARCH_HANDLER>& aHandler )
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
        tab->Clear();
}


void SEARCH_PANE::OnSearchTextEntry( wxCommandEvent& aEvent )
{
    m_lastQuery = m_searchCtrl1->GetValue();

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


void SEARCH_PANE::OnClosed( wxAuiManagerEvent& aEvent )
{
    if( APP_SETTINGS_BASE* cfg = m_frame->config() )
        m_frame->SaveSettings( cfg );

    aEvent.Skip();
}


SEARCH_PANE_TAB* SEARCH_PANE::GetCurrentTab() const
{
    return dynamic_cast<SEARCH_PANE_TAB*>( m_notebook->GetCurrentPage() );
}


void SEARCH_PANE::OnCharHook( wxKeyEvent& aEvent )
{
    // Check if the event is from a child window of the search pane
    wxWindow* eventObject = dynamic_cast<wxWindow*>( aEvent.GetEventObject() );

    if( !eventObject || !IsDescendant( eventObject ) )
    {
        aEvent.Skip();
        return;
    }

    // Try to let the tool framework handle the event
    if( m_frame->GetToolDispatcher() )
    {
        m_frame->GetToolDispatcher()->DispatchWxEvent( aEvent );
        return;
    }

    aEvent.Skip();
}
