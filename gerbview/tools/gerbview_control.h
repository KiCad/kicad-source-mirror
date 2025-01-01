/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2017 Jon Evans <jon@craftyjon.com>
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

#ifndef GERBVIEW_CONTROL_H
#define GERBVIEW_CONTROL_H

#include <tool/tool_interactive.h>

class EDA_DRAW_PANEL_GAL;
class GERBVIEW_FRAME;

/**
 * Handle actions that are shared between different frames in Pcbnew.
 */

class GERBVIEW_CONTROL : public TOOL_INTERACTIVE
{
public:
    GERBVIEW_CONTROL();
    ~GERBVIEW_CONTROL() override { }

    /// @copydoc TOOL_INTERACTIVE::Reset()
    void Reset( RESET_REASON aReason ) override;

    // Display modes
    int DisplayControl( const TOOL_EVENT& aEvent );

    // Layer control
    int LayerNext( const TOOL_EVENT& aEvent );
    int LayerPrev( const TOOL_EVENT& aEvent );
    int MoveLayerUp( const TOOL_EVENT& aEvent );
    int MoveLayerDown( const TOOL_EVENT& aEvent );
    int ClearLayer( const TOOL_EVENT& aEvent );
    int ClearAllLayers( const TOOL_EVENT& aEvent );
    int ReloadAllLayers( const TOOL_EVENT& aEvent );

    // Files
    int OpenAutodetected( const TOOL_EVENT& aEvent );
    int OpenGerber( const TOOL_EVENT& aEvent );
    int OpenDrillFile( const TOOL_EVENT& aEvent );
    int OpenJobFile( const TOOL_EVENT& aEvent );
    int OpenZipFile( const TOOL_EVENT& aEvent );

    int ToggleLayerManager( const TOOL_EVENT& aEvent );

    // Highlight control
    int HighlightControl( const TOOL_EVENT& aEvent );

    // Miscellaneous
    int ExportToPcbnew( const TOOL_EVENT& aEvent );
    int UpdateMessagePanel( const TOOL_EVENT& aEvent );
    int Print( const TOOL_EVENT& aEvent );

    // Drag and drop
    int LoadZipfile( const TOOL_EVENT& aEvent );
    int LoadGerbFiles( const TOOL_EVENT& aEvent );

    ///< Set up handlers for various events.
    void setTransitions() override;

protected:
    EDA_DRAW_PANEL_GAL* canvas()
    {
        return m_frame->GetCanvas();
    }

private:
    GERBVIEW_FRAME* m_frame;
};

#endif
