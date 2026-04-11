/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2023 Alex Shvartzkop <dudesuchamazing@gmail.com>
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

#ifndef GENERATOR_TOOL_H
#define GENERATOR_TOOL_H

#include <tools/generator_tool_pns_proxy.h>
#include <pcb_generator.h>
#include <properties/property_mgr.h>


class DIALOG_GENERATORS;
class PCB_EDIT_FRAME;
class PROGRESS_REPORTER;
class WX_PROGRESS_REPORTER;


/**
 * Handle actions specific to filling copper zones.
 */
class GENERATOR_TOOL : public GENERATOR_TOOL_PNS_PROXY
{
public:
    GENERATOR_TOOL();
    ~GENERATOR_TOOL();

    /// @copydoc TOOL_INTERACTIVE::Reset()
    void Reset( RESET_REASON aReason ) override;

    /// @copydoc TOOL_INTERACTIVE::Init()
    bool Init() override;

    void DestroyManagerDialog();

    int ShowGeneratorsManager( const TOOL_EVENT& aEvent );

    int RegenerateSelected( const TOOL_EVENT& aEvent );
    int RegenerateAllOfType( const TOOL_EVENT& aEvent );
    int RegenerateOutdated( const TOOL_EVENT& aEvent );
    int RegenerateItem( const TOOL_EVENT& aEvent );
    int GenEditAction( const TOOL_EVENT& aEvent );

private:
    ///< Set up handlers for various events.
    void setTransitions() override;

    DIALOG_GENERATORS* m_mgrDialog;

    /// RAII PROPERTY_MANAGER listener subscriptions; auto-unregister on
    /// destruction so this tool composes cleanly with other listeners.
    PROPERTY_LISTENER_SUBSCRIPTION m_boardItemListener;
    PROPERTY_LISTENER_SUBSCRIPTION m_generatorListener;
};

#endif
