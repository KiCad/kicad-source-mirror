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
#include <layer_ids.h>
#include <netclass.h>
#include <libeval_compiler/libeval_compiler.h>
#include <wx/intl.h>

class BOARD_ITEM;
class PCB_EXPR_UCODE;
class DRC_CONSTRAINT;
class DRC_RULE_CONDITION;


enum DRC_CONSTRAINT_T
{
    NULL_CONSTRAINT = 0,
    CLEARANCE_CONSTRAINT,
    HOLE_CLEARANCE_CONSTRAINT,
    HOLE_TO_HOLE_CONSTRAINT,
    EDGE_CLEARANCE_CONSTRAINT,
    HOLE_SIZE_CONSTRAINT,
    COURTYARD_CLEARANCE_CONSTRAINT,
    SILK_CLEARANCE_CONSTRAINT,
    TRACK_WIDTH_CONSTRAINT,
    ANNULAR_WIDTH_CONSTRAINT,
    DISALLOW_CONSTRAINT,
    VIA_DIAMETER_CONSTRAINT,
    LENGTH_CONSTRAINT,
    SKEW_CONSTRAINT,
    DIFF_PAIR_GAP_CONSTRAINT,
    DIFF_PAIR_MAX_UNCOUPLED_CONSTRAINT,
    DIFF_PAIR_INTRA_SKEW_CONSTRAINT,
    VIA_COUNT_CONSTRAINT
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
    OPT<DRC_CONSTRAINT> FindConstraint( DRC_CONSTRAINT_T aType );

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
    DRC_CONSTRAINT( DRC_CONSTRAINT_T aType = NULL_CONSTRAINT,
                    const wxString& aName = wxEmptyString ) :
            m_Type( aType ),
            m_Value(),
            m_DisallowFlags( 0 ),
            m_name( aName ),
            m_parentRule( nullptr )
    {
    }

    bool IsNull() const
    {
        return m_Type == NULL_CONSTRAINT;
    }

    const MINOPTMAX<int>& GetValue() const { return m_Value; }
    MINOPTMAX<int>& Value() { return m_Value; }

    void SetParentRule( DRC_RULE *aParentRule ) { m_parentRule = aParentRule; }
    DRC_RULE* GetParentRule() const { return m_parentRule; }

    void SetName( const wxString& aName ) { m_name = aName; }

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
    DRC_CONSTRAINT_T  m_Type;
    MINOPTMAX<int>    m_Value;
    int               m_DisallowFlags;

private:
    wxString          m_name;          // For just-in-time constraints
    DRC_RULE*         m_parentRule;    // For constraints found in rules
};


const DRC_CONSTRAINT* GetConstraint( const BOARD_ITEM* aItem, const BOARD_ITEM* bItem,
                                     int aConstraint, PCB_LAYER_ID aLayer,
                                     wxString* aRuleName = nullptr );


#endif // DRC_RULE_H
