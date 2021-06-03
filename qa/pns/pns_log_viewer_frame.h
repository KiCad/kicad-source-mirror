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

class PNS_LOG_VIEWER_OVERLAY;

class PNS_LOG_VIEWER_FRAME : public PNS_LOG_VIEWER_FRAME_BASE, public PCB_TEST_FRAME_BASE
{
public:
    PNS_LOG_VIEWER_FRAME( wxFrame* frame );
    virtual ~PNS_LOG_VIEWER_FRAME();

    void SetLogFile( PNS_LOG_FILE* aLog );
    std::shared_ptr<PNS_LOG_VIEWER_OVERLAY> GetOverlay() const { return m_overlay; }

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

    std::shared_ptr<PNS_LOG_VIEWER_OVERLAY>  m_overlay;
    std::shared_ptr<PNS_LOG_FILE>         m_logFile;
    std::shared_ptr<PNS_TEST_ENVIRONMENT> m_env;
    int                                   m_rewindIter;
    wxMenu*                               m_listPopupMenu;
};

class LABEL_MANAGER;

class PNS_LOG_VIEWER_OVERLAY : public KIGFX::VIEW_OVERLAY
{
    public:
        PNS_LOG_VIEWER_OVERLAY( KIGFX::GAL* aGal );
        void AnnotatedPolyline( const SHAPE_LINE_CHAIN& aL, std::string name, bool aShowVertexNumbers = false );
        void DrawAnnotations();

    private:
        std::unique_ptr<LABEL_MANAGER> m_labelMgr;
};

#endif
