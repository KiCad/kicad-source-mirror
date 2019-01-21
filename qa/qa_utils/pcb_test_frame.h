/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2013-2017 CERN
 * @author Tomasz Wlostowski <tomasz.wlostowski@cern.ch>
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

#ifndef __PCB_TEST_FRAME_H
#define __PCB_TEST_FRAME_H

#include <wx/wx.h>
#include <wx/app.h>

#include <memory>

#include <pcb_draw_panel_gal.h>

using std::unique_ptr;

class PCB_DRAW_PANEL_GAL;
class BOARD;

class TOOL_MANAGER;
class TOOL_DISPATCHER;
class ACTIONS;


namespace KIGFX {
    class VIEW;
};

// Define a new application type
class GAL_TEST_APP : public wxApp
{
public:
    virtual bool OnInit() override;

    virtual void OnInitCmdLine( wxCmdLineParser& parser ) override;
    virtual bool OnCmdLineParsed( wxCmdLineParser& parser ) override;

private:
    wxString m_filename;
};

class PCB_TEST_FRAME : public wxFrame
{
public:
    PCB_TEST_FRAME(wxFrame *frame,
            const wxString& title,
            const wxPoint& pos = wxDefaultPosition,
            const wxSize& size = wxDefaultSize,
            long style = wxDEFAULT_FRAME_STYLE,
            PCB_DRAW_PANEL_GAL::GAL_TYPE aGalType = PCB_DRAW_PANEL_GAL::GAL_TYPE_OPENGL );

    virtual ~PCB_TEST_FRAME();

    void SetBoard( BOARD * b);
    BOARD* LoadAndDisplayBoard ( const std::string& filename );

protected:

    virtual void OnExit(wxCommandEvent& event);
    virtual void OnMotion( wxMouseEvent& aEvent );
    virtual void OnMenuFileOpen( wxCommandEvent& WXUNUSED( event ) );

    void buildView();

    unique_ptr < PCB_DRAW_PANEL_GAL > m_galPanel;
    unique_ptr < BOARD > m_board;
#ifdef USE_TOOL_MANAGER
    unique_ptr < TOOL_MANAGER > m_toolManager;
    unique_ptr < TOOL_DISPATCHER > m_toolDispatcher;
    unique_ptr < ACTIONS > m_pcbActions;
#endif
};

wxFrame* CreateMainFrame( const std::string& aFileName );

#endif
