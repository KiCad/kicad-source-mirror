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
#include <layers_id_colors_and_visibility.h>
#include <netclass.h>
#include <libeval_compiler/libeval_compiler.h>


class BOARD_ITEM;
class PCB_EXPR_UCODE;


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


enum DRC_CONSTRAINT_TYPE_T
{
    DRC_RULE_ID_UNKNOWN = -1,
    DRC_RULE_ID_CLEARANCE = 0,
    DRC_RULE_ID_HOLE_SIZE,
    DRC_RULE_ID_ANNULUS,
    DRC_RULE_ID_TRACK,
    DRC_RULE_ID_DISALLOW
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
        m_Type( DRC_RULE_ID_UNKNOWN ),
        m_DisallowFlags( 0 ),
        m_LayerCondition( LSET::AllLayersMask() )
    {}

    const MINOPTMAX<int>& GetValue() const { return m_Value; }
    MINOPTMAX<int>& Value() { return m_Value; }

public:
    DRC_CONSTRAINT_TYPE_T  m_Type;
    MINOPTMAX<int> m_Value;
    int            m_DisallowFlags;
    LSET           m_LayerCondition;
};


class DRC_RULE_CONDITION
{
public:
    DRC_RULE_CONDITION();
    ~DRC_RULE_CONDITION();

    bool EvaluateFor( const BOARD_ITEM* aItemA, const BOARD_ITEM* aItemB, PCB_LAYER_ID aLayer );
    bool Compile( REPORTER* aReporter, int aSourceLine, int aSourceOffset );

public:
    wxString  m_Expression;

private:
    PCB_EXPR_UCODE*       m_ucode;
};


class DRC_RULE
{
public:
    DRC_RULE();
    virtual ~DRC_RULE();

public:
    wxString                    m_Name;
    LSET                        m_LayerCondition;
    wxString                    m_TestProviderName;
    DRC_RULE_CONDITION          m_Condition;
    std::vector<DRC_CONSTRAINT> m_Constraints;
};


const DRC_CONSTRAINT* GetConstraint( const BOARD_ITEM* aItem, const BOARD_ITEM* bItem,
                                     int aConstraint, PCB_LAYER_ID aLayer, wxString* aRuleName );


#endif      // DRC_RULE_H
