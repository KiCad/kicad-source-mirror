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
 */

#pragma once

#include <tools/pcb_tool_base.h>

/**
 * Tool that owns actions specific to via-stitch generators
 */
class VIA_STITCH_TOOL : public PCB_TOOL_BASE
{
public:
    VIA_STITCH_TOOL() : PCB_TOOL_BASE( "pcbnew.ViaStitch" ) {}
    ~VIA_STITCH_TOOL() override = default;

    bool Init() override;

    int ExcludeStitchVia( const TOOL_EVENT& aEvent );
    int ClearStitchViaExclusions( const TOOL_EVENT& aEvent );

private:
    void setTransitions() override;
};