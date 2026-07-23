/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
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

#ifndef DRC_RULE_CONDITION_H
#define DRC_RULE_CONDITION_H

#include <core/typeinfo.h>
#include <layer_ids.h>

class BOARD_ITEM;
class PCBEXPR_UCODE;
class REPORTER;


class DRC_RULE_CONDITION
{
public:
    DRC_RULE_CONDITION( const wxString& aExpression = "" );
    ~DRC_RULE_CONDITION();

    bool EvaluateFor( const BOARD_ITEM* aItemA, const BOARD_ITEM* aItemB, int aConstraint,
                      PCB_LAYER_ID aLayer, REPORTER* aReporter = nullptr );

    bool Compile( REPORTER* aReporter, int aSourceLine = 0, int aSourceOffset = 0 );

    void SetExpression( const wxString& aExpression ) { m_expression = aExpression; }
    wxString GetExpression() const { return m_expression; }

    bool HasGeometryDependentFunctions() const;
    bool RequiresPairItems() const;

private:
    wxString                       m_expression;
    std::unique_ptr<PCBEXPR_UCODE> m_ucode;
};


#endif // DRC_RULE_CONDITION_H
