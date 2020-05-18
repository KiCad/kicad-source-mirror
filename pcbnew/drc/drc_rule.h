/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2020 KiCad Developers, see change_log.txt for contributors.
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

#ifndef DRC_RULE_H
#define DRC_RULE_H

#include <core/typeinfo.h>
#include <netclass.h>
#include <layers_id_colors_and_visibility.h>


class BOARD_ITEM;


class DRC_RULE
{
public:
    DRC_RULE() :
            m_Clearance( 0 ),
            m_AnnulusWidth( 0 ),
            m_TrackWidth( 0 ),
            m_Hole( 0 )
    { }

public:
    wxString m_Name;

    // A 0 value means the property is not modified by this rule.
    // A positive value is a minimum.  A negative value is a relaxed constraint: the minimum
    // is reduced to the absolute value of the constraint.
    int      m_Clearance;
    int      m_AnnulusWidth;
    int      m_TrackWidth;
    int      m_Hole;
};


class DRC_SELECTOR
{
public:
    DRC_SELECTOR() :
            m_Priority( 1 )
    { }

public:
    std::vector<NETCLASSPTR>  m_MatchNetclasses;
    std::vector<KICAD_T>      m_MatchTypes;
    std::vector<PCB_LAYER_ID> m_MatchLayers;
    std::vector<wxString>     m_MatchAreas;

    int                       m_Priority;       // 0 indicates automatic priority generation
    DRC_RULE*                 m_Rule;
};


void MatchSelectors( const std::vector<DRC_SELECTOR*>& aSelectors,
                     const BOARD_ITEM* aItem, const NETCLASS* aNetclass,
                     const BOARD_ITEM* bItem, const NETCLASS* bNetclass,
                     std::vector<DRC_SELECTOR*>* aSelected );



#endif      // DRC_RULE_H
