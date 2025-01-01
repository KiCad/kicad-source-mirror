/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2007, 2008 Lubo Racko <developer@lura.sk>
 * Copyright (C) 2007, 2008, 2012-2013 Alexander Lunev <al.lunev@yahoo.com>
 * Copyright The KiCad Developers, see AUTHORS.TXT for contributors.
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

#include <pcad/pcad_pcb_component.h>

#include <board.h>
#include <common.h>
#include <footprint.h>

#include <wx/string.h>


namespace PCAD2KICAD {

PCAD_PCB_COMPONENT::PCAD_PCB_COMPONENT( PCAD_CALLBACKS* aCallbacks, BOARD* aBoard ) :
        m_Uuid(),
        m_callbacks( aCallbacks ),
        m_board( aBoard )
{
    m_ObjType = wxT( '?' );
    m_PCadLayer       = 0;
    m_KiCadLayer      = F_Cu; // It *has* to be somewhere...
    m_PositionX       = 0;
    m_PositionY       = 0;
    m_Rotation        = ANGLE_0;
    InitTTextValue( &m_Name );
    m_Net             = wxEmptyString;
    m_NetCode         = 0;
    m_CompRef         = wxEmptyString;
    m_PatGraphRefName = wxEmptyString;
}


PCAD_PCB_COMPONENT::~PCAD_PCB_COMPONENT()
{
}


void PCAD_PCB_COMPONENT::SetPosOffset( int aX_offs, int aY_offs )
{
    m_PositionX += aX_offs;
    m_PositionY += aY_offs;
}

void PCAD_PCB_COMPONENT::Flip()
{
    m_PositionX = -m_PositionX;
}

} // namespace PCAD2KICAD
