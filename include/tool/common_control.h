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

#ifndef COMMON_CONTROL_H
#define COMMON_CONTROL_H

#include <tool/tool_interactive.h>

class EDA_BASE_FRAME;

/**
 * Handle actions that are shared between different applications
 */

class COMMON_CONTROL : public TOOL_INTERACTIVE
{
public:
    COMMON_CONTROL() :
        TOOL_INTERACTIVE( "common.SuiteControl" ),
        m_frame( nullptr )
    { }

    ~COMMON_CONTROL() override { }

    /// @copydoc TOOL_BASE::Reset()
    void Reset( RESET_REASON aReason ) override;

    int OpenPreferences( const TOOL_EVENT& aEvent );
    int ConfigurePaths( const TOOL_EVENT& aEvent );
    int ShowLibraryTable( const TOOL_EVENT& aEvent );

    int ShowPlayer( const TOOL_EVENT& aEvent );
    int Quit( const TOOL_EVENT& aEvent );
    int Execute( const TOOL_EVENT& aEvent );
    int ShowProjectManager( const TOOL_EVENT& aEvent );

    int ShowHelp( const TOOL_EVENT& aEvent );
    int About( const TOOL_EVENT& aEvent );
    int ListHotKeys( const TOOL_EVENT& aEvent );
    int GetInvolved( const TOOL_EVENT& aEvent );
    int Donate( const TOOL_EVENT& aEvent );
    int ReportBug( const TOOL_EVENT& aEvent );

    ///< Sets up handlers for various events.
    void setTransitions() override;

    int Execute( const wxString& aExecutible, const wxString& aParam );

private:
    ///< Pointer to the currently used edit frame.
    EDA_BASE_FRAME* m_frame;

    static wxString m_bugReportUrl;
    static wxString m_bugReportTemplate;
};

#endif
