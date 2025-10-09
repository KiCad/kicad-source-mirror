/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
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

#ifndef PCB_VIEWER_TOOLS_H_
#define PCB_VIEWER_TOOLS_H_

#include <board.h>
#include <tool/tool_interactive.h>
#include <pcbnew_settings.h>
#include <pcb_base_frame.h>
#include <pcb_view.h>


/**
 * Tool useful for viewing footprints.
 *
 * This tool is designed to be lighter-weight so that it doesn't bring in as many PcbNew
 * dependencies (since it is used in cvpcb).
 */
class PCB_VIEWER_TOOLS : public TOOL_INTERACTIVE
{
public:
    PCB_VIEWER_TOOLS() :
        TOOL_INTERACTIVE( "pcbnew.PCBViewerTools" ),
        m_footprintFrame( false ),
        m_isDefaultTool( false )
    {}

    ~PCB_VIEWER_TOOLS() override {}

    bool Init() override;

    /// @copydoc TOOL_BASE::Reset()
    void Reset( RESET_REASON aReason ) override;

    ///< Launch a tool to measure between points.
    int MeasureTool( const TOOL_EVENT& aEvent );
    int NextLineMode( const TOOL_EVENT& aEvent );

    // Display modes
    int ShowPadNumbers( const TOOL_EVENT& aEvent );
    int PadDisplayMode( const TOOL_EVENT& aEvent );
    int GraphicOutlines( const TOOL_EVENT& aEvent );
    int TextOutlines( const TOOL_EVENT& aEvent );

    /// Automatically zoom to fit on footprints
    int FootprintAutoZoom( const TOOL_EVENT& aEvent );

    /// Show the 3D viewer
    int Show3DViewer( const TOOL_EVENT& aEvent );

    ///< Set up handlers for various events.
    void setTransitions() override;

    /**
     * Toggle edit footprint mode.
     *
     * When enabled, one may select parts of footprints individually (graphics, pads, etc.),
     * so they can be modified.
     *
     * @param aEnabled decides if the mode should be enabled.
     */
    void SetFootprintFrame( bool aIsFrame )
    {
        m_footprintFrame = aIsFrame;
    }

    void SetIsDefaultTool( bool aIsDefaultTool )
    {
        m_isDefaultTool = aIsDefaultTool;
    }

    bool IsFootprintFrame() const
    {
        return m_footprintFrame;
    }

protected:
    PCB_BASE_FRAME* frame() const
    {
        return getEditFrame<PCB_BASE_FRAME>();
    }

    KIGFX::PCB_VIEW* view() const
    {
        return static_cast<KIGFX::PCB_VIEW*>( getView() );
    }

    PCBNEW_SETTINGS::DISPLAY_OPTIONS& displayOptions() const
    {
        return frame()->GetPcbNewSettings()->m_Display;
    }

    PCB_DRAW_PANEL_GAL* canvas() const
    {
        return static_cast<PCB_DRAW_PANEL_GAL*>( frame()->GetCanvas() );
    }

    BOARD* board() const
    {
        return getModel<BOARD>();
    }

    FOOTPRINT* footprint() const
    {
        return board()->GetFirstFootprint();
    }

protected:
    bool m_footprintFrame;  ///< Is this tool associated with a footprint frame
    bool m_isDefaultTool;   ///< Indicates no selection tool is present in the current toolset
};

#endif
