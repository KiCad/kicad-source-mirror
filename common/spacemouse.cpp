/*
 *
 * This file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include "spacemouse.h"
#include <wx/event.h>

#include <class_draw_panel_gal.h>

LINUX_SPACEMOUSE::LINUX_SPACEMOUSE(EDA_DRAW_PANEL_GAL* parent)
    : m_parent(parent)
{
    m_parent->Bind( EVT_SPACEMOUSE_MOTION, &LINUX_SPACEMOUSE::OnSpaceMouseMotion );
    m_parent->Bind( EVT_SPACEMOUSE_BUTTON, &LINUX_SPACEMOUSE::OnSpaceMouseButton );
}

LINUX_SPACEMOUSE::~LINUX_SPACEMOUSE()
{
    m_parent->Unbind( EVT_SPACEMOUSE_MOTION, &LINUX_SPACEMOUSE::OnSpaceMouseMotion );
    m_parent->Unbind( EVT_SPACEMOUSE_BUTTON, &LINUX_SPACEMOUSE::OnSpaceMouseButton );
}

void LINUX_SPACEMOUSE::OnSpaceMouseMotion()
{
    // Handle SpaceMouse motion event
}

void LINUX_SPACEMOUSE::OnSpaceMouseButton()
{
    // Handle SpaceMouse button event
}

