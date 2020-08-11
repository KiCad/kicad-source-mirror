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
#include <layers_id_colors_and_visibility.h>
#include <netclass.h>
#include <libeval_compiler/libeval_compiler.h>

class BOARD_ITEM;

namespace LIBEVAL
{
class UCODE;
class ERROR_STATUS;
};

class PCB_EXPR_UCODE;

namespace test
{

class DRC_RULE;
class DRC_RULE_CONDITION;

enum DRC_CONSTRAINT_TYPE_T
{
    DRC_CONSTRAINT_TYPE_UNKNOWN = -1,
    DRC_CONSTRAINT_TYPE_CLEARANCE = 0,
    DRC_CONSTRAINT_TYPE_HOLE_CLEARANCE,
    DRC_CONSTRAINT_TYPE_EDGE_CLEARANCE,
    DRC_CONSTRAINT_TYPE_HOLE_SIZE,
    DRC_CONSTRAINT_TYPE_COURTYARD_CLEARANCE,
    DRC_CONSTRAINT_TYPE_SILK_TO_PAD,
    DRC_CONSTRAINT_TYPE_SILK_TO_SILK,
    DRC_CONSTRAINT_TYPE_TRACK_WIDTH,
    DRC_CONSTRAINT_TYPE_ANNULUS_WIDTH,
    DRC_CONSTRAINT_TYPE_DISALLOW
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

enum DRC_RULE_SEVERITY_T
{
    DRC_SEVERITY_IGNORE = 0,
    DRC_SEVERITY_WARNING,
    DRC_SEVERITY_ERROR
};

template<class T=int>
class MINOPTMAX
{
public:
    T Min() const { assert( m_hasMin ); return m_min; };
    T Max() const { assert( m_hasMax ); return m_max; };
    T Opt() const { assert( m_hasOpt ); return m_opt; };

    bool HasMin() const { return m_hasMin; }
    bool HasMax() const { return m_hasMax; }
    bool HasOpt() const { return m_hasOpt; }

    void SetMin( T v ) { m_min = v; m_hasMin = true; }
    void SetMax( T v ) { m_max = v; m_hasMax = true; }
    void SetOpt( T v ) { m_opt = v; m_hasOpt = true; }

private:
    T m_min;
    T m_opt;
    T m_max;
    bool m_hasMin = false;
    bool m_hasOpt = false;
    bool m_hasMax = false;
};

class DRC_CONSTRAINT
{
    public:
    DRC_CONSTRAINT() :
        m_Type( DRC_CONSTRAINT_TYPE_T::DRC_CONSTRAINT_TYPE_UNKNOWN ),
        m_DisallowFlags( 0 ),
        m_LayerCondition( LSET::AllLayersMask() ),
        m_parentRule( nullptr ) // fixme
    {
    }

    const MINOPTMAX<int>& GetValue() const { return m_Value; }
    MINOPTMAX<int>& Value() { return m_Value; }

    // fixme: needed?
    bool Allowed() const { return m_Allow; }
    DRC_RULE* GetParentRule() const { return m_parentRule; }

    DRC_CONSTRAINT_TYPE_T GetType() const { return m_Type; }

    public:
        DRC_CONSTRAINT_TYPE_T  m_Type;
        int            m_DisallowFlags;
    	LSET           m_LayerCondition;

    private:
        DRC_RULE *m_parentRule;
        MINOPTMAX<int> m_Value;
    	bool      m_Allow;
};

class DRC_RULE
{
public:
    DRC_RULE();
    virtual ~DRC_RULE();

    virtual bool IsImplicit() const { return false; };
    virtual bool AppliesTo( const BOARD_ITEM* a, const BOARD_ITEM* b = nullptr ) const { return true; };
    virtual bool IsEnabled() const { return m_Enabled; }
    
    virtual bool HasSpecificItemSet() const { return false; };
    virtual void FillSpecificItemSet( std::vector<BOARD_ITEM*> specificItems ) { };

    int GetPriority() const { return m_Priority; }
    DRC_RULE_SEVERITY_T GetSeverity() const { return m_Severity; }

    const wxString GetName() const { return m_Name; }
    
    std::vector<DRC_CONSTRAINT>& Constraints()
    {
        return m_constraints;
    }

    void AddConstraint( const DRC_CONSTRAINT& aConstraint );

    bool IsConditional() const
    {
        return m_condition != nullptr;
    }

    void SetCondition( test::DRC_RULE_CONDITION* aCondition )
    {
        m_condition = aCondition;
    }

    test::DRC_RULE_CONDITION* Condition()
    {
         return m_condition;
    }

    void SetLayerCondition( LSET aLayerCondition )
    {
        m_layerCondition = aLayerCondition;
    }

public:
    bool m_Unary;

    wxString m_Name;
    LSET                        m_layerCondition;
    DRC_RULE_CONDITION*         m_condition; // fixme: consider unique_ptr
    std::vector<DRC_CONSTRAINT> m_constraints;

    DRC_RULE_SEVERITY_T m_Severity;
    bool m_Enabled;
    int  m_Priority; // 0 indicates automatic priority generation
};


class DRC_RULE_CONDITION
{
public:
    DRC_RULE_CONDITION();
    ~DRC_RULE_CONDITION();

    bool EvaluateFor( const BOARD_ITEM* aItemA, const BOARD_ITEM* aItemB,
                                            PCB_LAYER_ID aLayer );

    bool Compile( REPORTER* aReporter, int aSourceLine = 0, int aSourceOffset = 0 );

    void SetLayerCondition( LSET aLayerCondition )
    {
        m_layerCondition = aLayerCondition;
    }

    void SetExpression( const wxString& aExpression )
    {
        m_expression = aExpression;
    }

    const wxString& GetExpression() const
    {
        return m_expression;
    }

private:
    LSET                                  m_layerCondition;
    wxString                              m_expression;
    std::unique_ptr<PCB_EXPR_UCODE>       m_ucode;
};


//DRC_RULE* GetRule( const BOARD_ITEM* aItem, const BOARD_ITEM* bItem, int aConstraint );

}; // namespace test

#endif // DRC_RULE_H
