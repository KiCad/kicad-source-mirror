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

#include <kiway.h>
#include <tool/tool_manager.h>
#include <tool/actions.h>
#include <bitmap2cmp_frame.h>
#include <bitmap2cmp_control.h>


bool BITMAP2CMP_CONTROL::Init()
{
    Reset( MODEL_RELOAD );
    return true;
}


void BITMAP2CMP_CONTROL::Reset( RESET_REASON aReason )
{
    m_frame = getEditFrame<BITMAP2CMP_FRAME>();
}


int BITMAP2CMP_CONTROL::Open( const TOOL_EVENT& aEvent )
{
    m_frame->OnLoadFile();
    return 0;
}


void BITMAP2CMP_CONTROL::setTransitions()
{
    Go( &BITMAP2CMP_CONTROL::Open,                   ACTIONS::open.MakeEvent() );
}
