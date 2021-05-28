/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2020 KiCad Developers.
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


// WARNING - this Tom's crappy PNS hack tool code. Please don't complain about its quality
// (unless you want to improve it).

#ifndef __PNS_LOG_VIEWER_FRAME_H
#define __PNS_LOG_VIEWER_FRAME_H

#include <pcb_painter.h>
#include <pcb_test_frame.h>

#include "pns_log.h"
#include "pns_log_viewer_frame_base.h"

#define ID_LIST_COPY 10001
#define ID_LIST_SHOW_ALL 10002
#define ID_LIST_SHOW_NONE 10003

class PNS_LOG_VIEWER_FRAME : public PNS_LOG_VIEWER_FRAME_BASE, public PCB_TEST_FRAME_BASE
{
public:
    PNS_LOG_VIEWER_FRAME( wxFrame* frame ) : PNS_LOG_VIEWER_FRAME_BASE( frame )
    {
        LoadSettings();
        createView( this, PCB_DRAW_PANEL_GAL::GAL_TYPE_OPENGL );

        m_viewSizer->Add( m_galPanel.get(), 1, wxEXPAND, 5 );

        Layout();

        Show( true );
        Maximize();
        Raise();

        auto settings = static_cast<KIGFX::PCB_RENDER_SETTINGS*>(
                m_galPanel->GetView()->GetPainter()->GetSettings() );

        settings->SetZoneDisplayMode( ZONE_DISPLAY_MODE::SHOW_ZONE_OUTLINE );

        m_listPopupMenu = new wxMenu( wxT( "" ) );
        m_listPopupMenu->Append( ID_LIST_COPY, wxT( "Copy selected geometry" ), wxT( "" ),
                                 wxITEM_NORMAL );
        m_listPopupMenu->Append( ID_LIST_SHOW_ALL, wxT( "Show all" ), wxT( "" ), wxITEM_NORMAL );
        m_listPopupMenu->Append( ID_LIST_SHOW_NONE, wxT( "Show none" ), wxT( "" ), wxITEM_NORMAL );

        m_itemList->Connect( m_itemList->GetId(), wxEVT_TREELIST_ITEM_CONTEXT_MENU,
                             wxMouseEventHandler( PNS_LOG_VIEWER_FRAME::onListRightClick ), NULL,
                             this );
        //m_itemList->Connect(m_itemList->GetId(),wxEVT_LISTBOX,wxCommandEventHandler(PNS_LOG_VIEWER_FRAME::onListSelect),NULL,this);
        m_itemList->Connect( m_itemList->GetId(), wxEVT_TREELIST_SELECTION_CHANGED,
                             wxCommandEventHandler( PNS_LOG_VIEWER_FRAME::onListSelect ), NULL,
                             this );
        m_itemList->Connect( m_itemList->GetId(), wxEVT_TREELIST_ITEM_CHECKED,
                             wxCommandEventHandler( PNS_LOG_VIEWER_FRAME::onListChecked ), NULL,
                             this );

        m_itemList->AppendColumn( "Type" );
        m_itemList->AppendColumn( "Value" );
        m_itemList->AppendColumn( "File" );
        m_itemList->AppendColumn( "Method" );
        m_itemList->AppendColumn( "Line" );
    }

    virtual ~PNS_LOG_VIEWER_FRAME() 
    {
        m_overlay = nullptr;
    }

    void SetLogFile( PNS_LOG_FILE* aLog );

private:
    void         drawLoggedItems( int iter );
    void         updateDumpPanel( int iter );
    virtual void createUserTools() override;
    void         buildListTree( wxTreeListItem item, PNS_TEST_DEBUG_DECORATOR::DEBUG_ENT* ent,
                                int depth = 0 );
    void         syncModel();
    PNS_TEST_DEBUG_DECORATOR::STAGE* getCurrentStage();

    virtual void onReload( wxCommandEvent& event ) override;
    virtual void onExit( wxCommandEvent& event ) override;
    virtual void onRewindScroll( wxScrollEvent& event ) override;
    virtual void onRewindCountText( wxCommandEvent& event ) override;
    virtual void onListRightClick( wxMouseEvent& event );
    virtual void onListSelect( wxCommandEvent& event );
    virtual void onBtnRewindLeft( wxCommandEvent& event ) override;
    virtual void onBtnRewindRight( wxCommandEvent& event ) override;
    virtual void onListChecked( wxCommandEvent& event );

    std::shared_ptr<KIGFX::VIEW_OVERLAY>  m_overlay;
    std::shared_ptr<PNS_LOG_FILE>         m_logFile;
    std::shared_ptr<PNS_TEST_ENVIRONMENT> m_env;
    int                                   m_rewindIter;
    wxMenu*                               m_listPopupMenu;
};

#endif
