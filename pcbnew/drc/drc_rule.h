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

#ifndef DRC_RULE_PROTO_H
#define DRC_RULE_PROTO_H

#include <core/typeinfo.h>
#include <core/optional.h>
#include <core/minoptmax.h>
#include <layers_id_colors_and_visibility.h>
#include <netclass.h>
#include <libeval_compiler/libeval_compiler.h>

class BOARD_ITEM;
class PCB_EXPR_UCODE;
class DRC_CONSTRAINT;
class DRC_RULE_CONDITION;


enum DRC_CONSTRAINT_TYPE_T
{
    DRC_CONSTRAINT_TYPE_UNKNOWN = -1,
    DRC_CONSTRAINT_TYPE_NULL = 0,
    DRC_CONSTRAINT_TYPE_CLEARANCE,
    DRC_CONSTRAINT_TYPE_HOLE_CLEARANCE,
    DRC_CONSTRAINT_TYPE_EDGE_CLEARANCE,
    DRC_CONSTRAINT_TYPE_HOLE_SIZE,
    DRC_CONSTRAINT_TYPE_COURTYARD_CLEARANCE,
    DRC_CONSTRAINT_TYPE_SILK_TO_MASK,
    DRC_CONSTRAINT_TYPE_SILK_TO_SILK,
    DRC_CONSTRAINT_TYPE_TRACK_WIDTH,
    DRC_CONSTRAINT_TYPE_ANNULAR_WIDTH,
    DRC_CONSTRAINT_TYPE_DISALLOW,
    DRC_CONSTRAINT_TYPE_VIA_DIAMETER,
    DRC_CONSTRAINT_TYPE_LENGTH,
    DRC_CONSTRAINT_TYPE_SKEW,
    DRC_CONSTRAINT_TYPE_DIFF_PAIR_GAP,
    DRC_CONSTRAINT_TYPE_DIFF_PAIR_MAX_UNCOUPLED,
    DRC_CONSTRAINT_TYPE_DIFF_PAIR_INTRA_SKEW,
    DRC_CONSTRAINT_TYPE_VIA_COUNT
};


enum DRC_DISALLOW_T
{
    DRC_DISALLOW_VIAS        = (1 << 0),
    DRC_DISALLOW_MICRO_VIAS  = (1 << 1),
    DRC_DISALLOW_BB_VIAS     = (1 << 2),
    DRC_DISALLOW_TRACKS      = (1 << 3),
    DRC_DISALLOW_PADS        = (1 << 4),
    DRC_DISALLOW_ZONES       = (1 << 5),
    DRC_DISALLOW_TEXTS       = (1 << 6),
    DRC_DISALLOW_GRAPHICS    = (1 << 7),
    DRC_DISALLOW_HOLES       = (1 << 8),
    DRC_DISALLOW_FOOTPRINTS  = (1 << 9)
};


class DRC_RULE
{
public:
    DRC_RULE();
    virtual ~DRC_RULE();

    virtual bool AppliesTo( const BOARD_ITEM* a, const BOARD_ITEM* b = nullptr ) const
    {
        return true;
    };

    void AddConstraint( DRC_CONSTRAINT& aConstraint );
    OPT<DRC_CONSTRAINT> FindConstraint( DRC_CONSTRAINT_TYPE_T aType );

public:
    bool                        m_Unary;
    bool                        m_Implicit;
    wxString                    m_Name;
    wxString                    m_LayerSource;
    LSET                        m_LayerCondition;
    DRC_RULE_CONDITION*         m_Condition;
    std::vector<DRC_CONSTRAINT> m_Constraints;
};


class DRC_CONSTRAINT
{
    public:
    DRC_CONSTRAINT( DRC_CONSTRAINT_TYPE_T aType = DRC_CONSTRAINT_TYPE_UNKNOWN,
                    const wxString& aName = wxEmptyString ) :
            m_Type( aType ),
            m_DisallowFlags( 0 ),
            m_name( aName ),
            m_parentRule( nullptr )
    {
    }

    bool IsNull() const
    {
        return m_Type == DRC_CONSTRAINT_TYPE_NULL;
    }

    const MINOPTMAX<int>& GetValue() const { return m_Value; }
    MINOPTMAX<int>& Value() { return m_Value; }

    void SetParentRule( DRC_RULE *aParentRule ) { m_parentRule = aParentRule; }
    DRC_RULE* GetParentRule() const { return m_parentRule; }

    wxString GetName() const
    {
        if( m_parentRule )
        {
            if( m_parentRule->m_Implicit )
                return m_parentRule->m_Name;
            else
                return wxString::Format( _( "rule %s" ), m_parentRule->m_Name );
        }

        return m_name;
    }

public:
    DRC_CONSTRAINT_TYPE_T  m_Type;
    MINOPTMAX<int>         m_Value;
    int                    m_DisallowFlags;

private:
    wxString               m_name;          // For just-in-time constraints
    DRC_RULE*              m_parentRule;    // For constraints found in rules
};


const DRC_CONSTRAINT* GetConstraint( const BOARD_ITEM* aItem, const BOARD_ITEM* bItem,
                                     int aConstraint, PCB_LAYER_ID aLayer,
                                     wxString* aRuleName = nullptr );


#endif // DRC_RULE_H
