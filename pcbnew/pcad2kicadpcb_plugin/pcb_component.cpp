/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2007, 2008 Lubo Racko <developer@lura.sk>
 * Copyright (C) 2007, 2008, 2012-2013 Alexander Lunev <al.lunev@yahoo.com>
 * Copyright (C) 2012 KiCad Developers, see CHANGELOG.TXT for contributors.
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

/**
 * @file pcb_component.cpp
 */

#include <wx/wx.h>
#include <wx/config.h>

#include <common.h>

#include <pcb_component.h>

namespace PCAD2KICAD {

PCB_COMPONENT::PCB_COMPONENT( PCB_CALLBACKS*    aCallbacks,
                              BOARD*            aBoard ) : m_callbacks( aCallbacks ),
    m_board( aBoard )
{
    m_tag       = 0;
    m_objType   = wxT( '?' );
    m_PCadLayer     = 0;
    m_KiCadLayer    = F_Cu; // It *has* to be somewhere...
    m_timestamp     = 0;
    m_positionX     = 0;
    m_positionY     = 0;
    m_rotation      = 0;
    InitTTextValue( &m_name );
    m_net       = wxEmptyString;
    m_netCode   = 0;
    m_compRef   = wxEmptyString;
    m_patGraphRefName = wxEmptyString;
}


PCB_COMPONENT::~PCB_COMPONENT()
{
}


void PCB_COMPONENT::AddToModule( MODULE* aModule )
{
}


void PCB_COMPONENT::SetPosOffset( int aX_offs, int aY_offs )
{
    m_positionX += aX_offs;
    m_positionY += aY_offs;
}

void PCB_COMPONENT::Flip()
{
    m_positionX = -m_positionX;
}

} // namespace PCAD2KICAD
