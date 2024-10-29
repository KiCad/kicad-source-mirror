/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2020-2023 KiCad Developers, see change_log.txt for contributors.
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

#include <kiid.h>
#include <core/typeinfo.h>
#include <optional>
#include <core/minoptmax.h>
#include <layer_ids.h>
#include <netclass.h>
#include <zones.h>
#include <libeval_compiler/libeval_compiler.h>
#include <wx/intl.h>
#include <widgets/report_severity.h>

class BOARD_ITEM;
class PCBEXPR_UCODE;
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
    TEXT_HEIGHT_CONSTRAINT,
    TEXT_THICKNESS_CONSTRAINT,
    TRACK_WIDTH_CONSTRAINT,
    ANNULAR_WIDTH_CONSTRAINT,
    ZONE_CONNECTION_CONSTRAINT,
    THERMAL_RELIEF_GAP_CONSTRAINT,
    THERMAL_SPOKE_WIDTH_CONSTRAINT,
    MIN_RESOLVED_SPOKES_CONSTRAINT,
    DISALLOW_CONSTRAINT,
    VIA_DIAMETER_CONSTRAINT,
    LENGTH_CONSTRAINT,
    SKEW_CONSTRAINT,
    DIFF_PAIR_GAP_CONSTRAINT,
    MAX_UNCOUPLED_CONSTRAINT,
    DIFF_PAIR_INTRA_SKEW_CONSTRAINT,
    VIA_COUNT_CONSTRAINT,
    PHYSICAL_CLEARANCE_CONSTRAINT,
    PHYSICAL_HOLE_CLEARANCE_CONSTRAINT,
    ASSERTION_CONSTRAINT,
    CONNECTION_WIDTH_CONSTRAINT
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
    DRC_RULE( const wxString& aName );

    virtual ~DRC_RULE();

    virtual bool AppliesTo( const BOARD_ITEM* a, const BOARD_ITEM* b = nullptr ) const
    {
        return true;
    };

    void AddConstraint( DRC_CONSTRAINT& aConstraint );
    std::optional<DRC_CONSTRAINT> FindConstraint( DRC_CONSTRAINT_T aType );

public:
    bool                        m_Unary;
    bool                        m_Implicit;
    KIID                        m_ImplicitItemId;
    wxString                    m_Name;
    wxString                    m_LayerSource;
    LSET                        m_LayerCondition;
    DRC_RULE_CONDITION*         m_Condition;
    std::vector<DRC_CONSTRAINT> m_Constraints;
    SEVERITY                    m_Severity;
};


class DRC_CONSTRAINT
{
public:
    DRC_CONSTRAINT( DRC_CONSTRAINT_T aType = NULL_CONSTRAINT,
                    const wxString& aName = wxEmptyString ) :
            m_Type( aType ),
            m_Value(),
            m_DisallowFlags( 0 ),
            m_ZoneConnection( ZONE_CONNECTION::INHERITED ),
            m_Test( nullptr ),
            m_ImplicitMin( false ),
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
                return wxString::Format( _( "rule '%s'" ), m_parentRule->m_Name );
        }

        return m_name;
    }

    SEVERITY GetSeverity() const
    {
        if( m_parentRule )
            return m_parentRule->m_Severity;
        else
            return RPT_SEVERITY_UNDEFINED;
    }

public:
    DRC_CONSTRAINT_T    m_Type;
    MINOPTMAX<int>      m_Value;
    int                 m_DisallowFlags;
    ZONE_CONNECTION     m_ZoneConnection;
    DRC_RULE_CONDITION* m_Test;
    bool                m_ImplicitMin;

private:
    wxString            m_name;          // For just-in-time constraints
    DRC_RULE*           m_parentRule;    // For constraints found in rules
};


const DRC_CONSTRAINT* GetConstraint( const BOARD_ITEM* aItem, const BOARD_ITEM* bItem,
                                     int aConstraint, PCB_LAYER_ID aLayer,
                                     wxString* aRuleName = nullptr );


#endif // DRC_RULE_H
