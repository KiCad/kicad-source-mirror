/*
* This program source code file is part of KiCad, a free EDA CAD application.
*
* Copyright (C) 2009 Jean-Pierre Charras, jp.charras at wanadoo.fr
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
* along with this program; if not, you may find one here:
* http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
* or you may search the http://www.gnu.org website for the version 2 license,
* or you may write to the Free Software Foundation, Inc.,
* 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
*/

/**
 * @file erc_sch_pin_context.h
 */

#ifndef _ERC_ITEM_CONTEXT_H
#define _ERC_ITEM_CONTEXT_H

#include <sch_pin.h>

/**
 * A class used to associate a #SCH_PIN with its owning #SCH_SHEET_PATH, in order to handle ERC
 * checks across connected common hierarchical schematics/
 */
class ERC_SCH_PIN_CONTEXT
{
public:
    ERC_SCH_PIN_CONTEXT() :
            m_pin( nullptr ),
            m_sheet(),
            m_hash( 0 )
    {}

    ERC_SCH_PIN_CONTEXT( SCH_PIN* pin, const SCH_SHEET_PATH& sheet ) :
            m_pin( pin ),
            m_sheet( sheet )
    {
        rehash();
    }

    ERC_SCH_PIN_CONTEXT( const ERC_SCH_PIN_CONTEXT& other ) = default;

    ERC_SCH_PIN_CONTEXT& operator=( const ERC_SCH_PIN_CONTEXT& other ) = default;

    ~ERC_SCH_PIN_CONTEXT() = default;

    /**
     * Get the SCH_PIN for this context.
     */
    SCH_PIN* Pin() const;

    /**
     * Get the #SCH_SHEET_PATH context for the paired #SCH_PIN.
     */
    const SCH_SHEET_PATH& Sheet() const;

    /**
     * Test two pin contexts for equality based on the deterministic hash.
     */
    bool operator==( const ERC_SCH_PIN_CONTEXT& other ) const;

    /**
     * Provide a deterministic ordering for item contexts based on hash value.
     */
    bool operator<( const ERC_SCH_PIN_CONTEXT& other ) const;

protected:
    /**
     * Calculate the deterministic hash for this context.
     */
    void rehash();

    SCH_PIN*       m_pin;
    SCH_SHEET_PATH m_sheet;
    size_t         m_hash;
};

#endif //_ERC_ITEM_CONTEXT_H
