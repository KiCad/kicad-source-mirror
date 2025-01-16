/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2013 CERN
 * @author Tomasz Wlostowski <tomasz.wlostowski@cern.ch>
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

#include <tool/tool_event.h>
#include <tool/tool_manager.h>

#include <eda_draw_frame.h>
#include <eda_base_frame.h>


bool TOOL_BASE::IsToolActive() const
{
    return m_toolMgr->IsToolActive( m_toolId );
}


KIGFX::VIEW* TOOL_BASE::getView() const
{
    return m_toolMgr->GetView();
}


KIGFX::VIEW_CONTROLS* TOOL_BASE::getViewControls() const
{
    return m_toolMgr->GetViewControls();
}


TOOLS_HOLDER* TOOL_BASE::getToolHolderInternal() const
{
    return m_toolMgr->GetToolHolder();
}


EDA_ITEM* TOOL_BASE::getModelInternal() const
{
    return m_toolMgr->GetModel();
}


void TOOL_BASE::attachManager( TOOL_MANAGER* aManager )
{
    m_toolMgr = aManager;
}
