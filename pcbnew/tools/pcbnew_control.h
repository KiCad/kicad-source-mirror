/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2014 CERN
 * @author Maciej Suminski <maciej.suminski@cern.ch>
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

#ifndef PCBNEW_CONTROL_H
#define PCBNEW_CONTROL_H

#include <tool/tool_interactive.h>

class PCB_BASE_FRAME;

/**
 * Class PCBNEW_CONTROL
 *
 * Handles hot keys that are not accepted by any other tool.
 */

class PCBNEW_CONTROL : public TOOL_INTERACTIVE
{
public:
    PCBNEW_CONTROL();

    /// @copydoc TOOL_INTERACTIVE::Reset()
    void Reset( RESET_REASON aReason );

    /// @copydoc TOOL_INTERACTIVE::Init()
    bool Init();

    // View controls
    int ZoomInOut( TOOL_EVENT& aEvent );
    int ZoomInOutCenter( TOOL_EVENT& aEvent );
    int ZoomCenter( TOOL_EVENT& aEvent );
    int ZoomFitScreen( TOOL_EVENT& aEvent );

    // Display modes
    int TrackDisplayMode( TOOL_EVENT& aEvent );
    int PadDisplayMode( TOOL_EVENT& aEvent );
    int ViaDisplayMode( TOOL_EVENT& aEvent );
    int HighContrastMode( TOOL_EVENT& aEvent );
    int HighContrastInc( TOOL_EVENT& aEvent );
    int HighContrastDec( TOOL_EVENT& aEvent );

    // Layer control
    int LayerTop( TOOL_EVENT& aEvent );
    int LayerInner1( TOOL_EVENT& aEvent );
    int LayerInner2( TOOL_EVENT& aEvent );
    int LayerInner3( TOOL_EVENT& aEvent );
    int LayerInner4( TOOL_EVENT& aEvent );
    int LayerInner5( TOOL_EVENT& aEvent );
    int LayerInner6( TOOL_EVENT& aEvent );
    int LayerBottom( TOOL_EVENT& aEvent );
    int LayerNext( TOOL_EVENT& aEvent );
    int LayerPrev( TOOL_EVENT& aEvent );
    int LayerAlphaInc( TOOL_EVENT& aEvent );
    int LayerAlphaDec( TOOL_EVENT& aEvent );

    // Grid control
    int GridFast1( TOOL_EVENT& aEvent );
    int GridFast2( TOOL_EVENT& aEvent );
    int GridNext( TOOL_EVENT& aEvent );
    int GridPrev( TOOL_EVENT& aEvent );
    int GridSetOrigin( TOOL_EVENT& aEvent );

    // Track & via size control
    int TrackWidthInc( TOOL_EVENT& aEvent );
    int TrackWidthDec( TOOL_EVENT& aEvent );
    int ViaSizeInc( TOOL_EVENT& aEvent );
    int ViaSizeDec( TOOL_EVENT& aEvent );

    // Miscellaneous
    int ResetCoords( TOOL_EVENT& aEvent );
    int SwitchUnits( TOOL_EVENT& aEvent );
    int ShowHelp( TOOL_EVENT& aEvent );
    int ToBeDone( TOOL_EVENT& aEvent );

private:
    ///> Sets up handlers for various events.
    void setTransitions();

    ///> Pointerto the currently used edit frame.
    PCB_BASE_FRAME* m_frame;
};

#endif
