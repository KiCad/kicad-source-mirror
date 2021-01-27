/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2017-2021 KiCad Developers, see AUTHORS.txt for contributors.
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

#ifndef __PAD_TOOL_H
#define __PAD_TOOL_H


#include <tools/pcb_tool_base.h>

class ACTION_MENU;

/**
 * Tool relating to pads and pad settings.
 */
class PAD_TOOL : public PCB_TOOL_BASE
{
public:
    PAD_TOOL();
    ~PAD_TOOL();

    ///< React to model/view changes
    void Reset( RESET_REASON aReason ) override;

    ///< Basic initialization
    bool Init() override;

    /**
     * Tool for quick pad enumeration.
     */
    int EnumeratePads( const TOOL_EVENT& aEvent );

    /**
     * Place a pad in footprint editor.
     */
    int PlacePad( const TOOL_EVENT& aEvent );

    /**
     * Enter/exit WYSIWYG pad shape editing.
     */
    int EditPad( const TOOL_EVENT& aEvent );

    wxString GetLastPadName() const { return m_lastPadName; }
    void SetLastPadName( const wxString& aPadName ) { m_lastPadName = aPadName; }

private:
    ///< Bind handlers to corresponding TOOL_ACTIONs.
    void setTransitions() override;

    ///< Apply pad settings from board design settings to a pad.
    int pastePadProperties( const TOOL_EVENT& aEvent );

    ///< Copy pad settings from a pad to the board design settings.
    int copyPadSettings( const TOOL_EVENT& aEvent );

    ///< Push pad settings from a pad to other pads on board or footprint.
    int pushPadSettings( const TOOL_EVENT& aEvent );

    PCB_LAYER_ID explodePad( PAD* aPad );
    void recombinePad( PAD* aPad );

    wxString       m_lastPadName;
    bool           m_padCopied;    // Indicates there are valid settings in the Master Pad object

    bool           m_wasHighContrast;
    KIID           m_editPad;
};

#endif // __PAD_TOOL_H
