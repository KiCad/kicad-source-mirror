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
#include <gal/graphics_abstraction_layer.h>

#include <tool/tools_holder.h>

using std::unique_ptr;

class PCB_DRAW_PANEL_GAL;
class BOARD;

class TOOL_MANAGER;
class TOOL_DISPATCHER;
class ACTIONS;
class PCB_SELECTION;
class PCB_TEST_SELECTION_TOOL;

namespace KIGFX {
    class VIEW;
};

class PCB_TEST_FRAME_BASE : public TOOLS_HOLDER
{
public:
    PCB_TEST_FRAME_BASE();
    virtual ~PCB_TEST_FRAME_BASE();

    virtual void SetBoard( std::shared_ptr<BOARD> b);
    virtual BOARD* LoadAndDisplayBoard ( const std::string& filename );

    std::shared_ptr < PCB_DRAW_PANEL_GAL > GetPanel() { return m_galPanel; }
    std::shared_ptr < BOARD > GetBoard() { return m_board; }

    void LoadSettings();

    virtual wxWindow* GetToolCanvas() const override
    {
        return m_galPanel.get();
    }

    void SetSelectionHook( std::function<void(PCB_TEST_FRAME_BASE*, PCB_SELECTION*)> aHook );
    void SetSelectableItemTypes( const std::vector<KICAD_T> aTypes );
    std::shared_ptr< PCB_TEST_SELECTION_TOOL> GetSelectionTool() const { return m_selectionTool; }

protected:

    void createView( wxWindow *aParent, PCB_DRAW_PANEL_GAL::GAL_TYPE aGalType = PCB_DRAW_PANEL_GAL::GAL_TYPE_OPENGL );
    virtual void createUserTools() {};

    std::shared_ptr < PCB_DRAW_PANEL_GAL > m_galPanel;
    std::shared_ptr < BOARD > m_board;
    std::shared_ptr < PCB_TEST_SELECTION_TOOL> m_selectionTool;
    KIGFX::GAL_DISPLAY_OPTIONS m_displayOptions;
    wxString m_mruPath;
};

void SetTopFrame ( wxFrame* aFrame );

#endif
