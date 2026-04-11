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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef FOOTPRINT_WIZARD_TOOLS_H
#define FOOTPRINT_WIZARD_TOOLS_H

#include <tool/tool_interactive.h>
#include <footprint_wizard_frame.h>

class FOOTPRINT_WIZARD_FRAME;

/**
 * Tool useful for viewing footprints.
 *
 * This tool is designed to be lighter-weight so that it doesn't bring in as many PcbNew
 * dependencies (since it is used in cvpcb).
 */
class FOOTPRINT_WIZARD_TOOLS : public TOOL_INTERACTIVE
{
public:
    FOOTPRINT_WIZARD_TOOLS() :
        TOOL_INTERACTIVE( "pcbnew.FpWizard" ),
        m_wizardFrame( nullptr )
    {}

    ~FOOTPRINT_WIZARD_TOOLS() override {}

    bool Init() override;

    /// @copydoc TOOL_BASE::Reset()
    void Reset( RESET_REASON aReason ) override;

    ///< Set up handlers for various events.
    void setTransitions() override;

protected:
    int ShowWizards( const TOOL_EVENT& aEvent );
    int ResetWizardPrms( const TOOL_EVENT& aEvent );
    int ExportFpToEditor( const TOOL_EVENT& aEvent );

    FOOTPRINT_WIZARD_FRAME* frame() const
    {
        return getEditFrame<FOOTPRINT_WIZARD_FRAME>();
    }

    /*FOOTPRINT_WIZARD_FRAME* frame() const
    {
        return m_wizardFrame;
    }*/

protected:
    FOOTPRINT_WIZARD_FRAME* m_wizardFrame;  ///< the associated footprint wizard frame
};

#endif
