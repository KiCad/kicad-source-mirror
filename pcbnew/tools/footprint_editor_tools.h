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

#ifndef FOOTPRINT_EDITOR_TOOLS_H
#define FOOTPRINT_EDITOR_TOOLS_H

#include <tools/pcb_tool_base.h>


class FOOTPRINT_EDIT_FRAME;
class DIALOG_FOOTPRINT_CHECKER;

/**
 * FOOTPRINT_EDITOR_TOOLS
 *
 * Module editor specific tools.
 */
class FOOTPRINT_EDITOR_TOOLS : public PCB_TOOL_BASE
{
public:
    FOOTPRINT_EDITOR_TOOLS();
    ~FOOTPRINT_EDITOR_TOOLS() override;

    /// @copydoc TOOL_INTERACTIVE::Reset()
    void Reset( RESET_REASON aReason ) override;

    /// @copydoc TOOL_INTERACTIVE::Init()
    bool Init() override;

    int NewFootprint( const TOOL_EVENT& aEvent );
    int CreateFootprint( const TOOL_EVENT& aEvent );
    
    int Save( const TOOL_EVENT& aEvent );
    int SaveAs( const TOOL_EVENT& aEvent );
    int Revert( const TOOL_EVENT& aEvent );

    int EditFootprint( const TOOL_EVENT& aEvent );
    int CutCopyFootprint( const TOOL_EVENT& aEvent );
    int PasteFootprint( const TOOL_EVENT& aEvent );
    int DeleteFootprint( const TOOL_EVENT& aEvent );
    int ImportFootprint( const TOOL_EVENT& aEvent );
    int ExportFootprint( const TOOL_EVENT& aEvent );

    int PinLibrary( const TOOL_EVENT& aEvent );
    int UnpinLibrary( const TOOL_EVENT& aEvent );
    int ToggleFootprintTree( const TOOL_EVENT& aEvent );
    int Properties( const TOOL_EVENT& aEvent );

    int CleanupGraphics( const TOOL_EVENT& aEvent );

    int CheckFootprint( const TOOL_EVENT& aEvent );
    void DestroyCheckerDialog();

    /**
     * Edit the properties used for new pad creation.
     */
    int DefaultPadProperties( const TOOL_EVENT& aEvent );

private:
    ///> Sets up handlers for various events.
    void setTransitions() override;

private:
    FOOTPRINT_EDIT_FRAME*      m_frame;
    DIALOG_FOOTPRINT_CHECKER*  m_checkerDialog;

    // A private clipboard for cut/copy/past of an entire footprint
    std::unique_ptr<FOOTPRINT> m_copiedFootprint;
};

#endif
