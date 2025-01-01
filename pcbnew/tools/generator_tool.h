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
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#ifndef GENERATOR_TOOL_H
#define GENERATOR_TOOL_H

#include <tools/generator_tool_pns_proxy.h>
#include <pcb_generator.h>


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

private:
};

#endif
