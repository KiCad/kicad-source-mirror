/*
* This program source code file is part of KiCad, a free EDA CAD application.
*
* Copyright (C) 2015 Jean-Pierre Charras, jp.charras at wanadoo.fr
* Copyright (C) 2011 Wayne Stambaugh <stambaughw@gmail.com>
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

/**
 * @file erc_sch_pin_context.cpp
 */

#include "erc/erc_sch_pin_context.h"

#include <hash.h>


SCH_PIN* ERC_SCH_PIN_CONTEXT::Pin() const
{
    return m_pin;
}


const SCH_SHEET_PATH& ERC_SCH_PIN_CONTEXT::Sheet() const
{
    return m_sheet;
}


bool ERC_SCH_PIN_CONTEXT::operator==( const ERC_SCH_PIN_CONTEXT& other ) const
{
    return m_hash == other.m_hash;
}


bool ERC_SCH_PIN_CONTEXT::operator<( const ERC_SCH_PIN_CONTEXT& other ) const
{
    return m_hash < other.m_hash;
}


void ERC_SCH_PIN_CONTEXT::rehash()
{
    m_hash = 0;
    hash_combine( m_hash, m_pin, m_sheet.GetCurrentHash() );
}
