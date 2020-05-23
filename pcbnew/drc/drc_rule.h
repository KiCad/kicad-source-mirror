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


#define CLEARANCE_CONSTRAINT (1 << 0)
#define ANNULUS_CONSTRAINT   (1 << 1)
#define TRACK_CONSTRAINT     (1 << 2)
#define HOLE_CONSTRAINT      (1 << 3)

#define DISALLOW_VIAS        (1 << 0)
#define DISALLOW_MICRO_VIAS  (1 << 1)
#define DISALLOW_BB_VIAS     (1 << 2)
#define DISALLOW_TRACKS      (1 << 3)
#define DISALLOW_PADS        (1 << 4)
#define DISALLOW_ZONES       (1 << 5)
#define DISALLOW_TEXTS       (1 << 6)
#define DISALLOW_GRAPHICS    (1 << 7)
#define DISALLOW_HOLES       (1 << 8)
#define DISALLOW_FOOTPRINTS  (1 << 9)


class DRC_RULE
{
public:
    DRC_RULE() :
            m_ConstraintFlags( 0 ),
            m_DisallowFlags( 0 ),
            m_Clearance( { 0, 0, INT_MAX / 2 } ),
            m_MinAnnulusWidth( 0 ),
            m_TrackConstraint( { 0, 0, INT_MAX / 2 } ),
            m_MinHole( 0 )
    { }

    struct MINOPTMAX
    {
        int Min;
        int Opt;
        int Max;
    };

public:
    wxString  m_Name;
    int       m_ConstraintFlags;
    int       m_DisallowFlags;

    // A 0 value means the property is not modified by this rule.
    // A positive value is a minimum.
    MINOPTMAX m_Clearance;
    int       m_MinAnnulusWidth;
    MINOPTMAX m_TrackConstraint;
    int       m_MinHole;

    wxString  m_Condition;
};


class DRC_SELECTOR
{
public:
    DRC_SELECTOR() :
            m_Priority( 1 ),
            m_Rule( nullptr )
    { }

public:
    std::vector<NETCLASSPTR>  m_MatchNetclasses;
    std::vector<KICAD_T>      m_MatchTypes;
    std::vector<PCB_LAYER_ID> m_MatchLayers;
    std::vector<wxString>     m_MatchAreas;

    int                       m_Priority;       // 0 indicates automatic priority generation
    DRC_RULE*                 m_Rule;
};


DRC_RULE* GetRule( const BOARD_ITEM* aItem, const BOARD_ITEM* bItem, int aConstraint );



#endif      // DRC_RULE_H
