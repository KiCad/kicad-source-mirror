/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2024 KiCad Developers, see AUTHORS.txt for contributors.
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

#ifndef DRC_RE_BOOL_INPUT_CONSTRAINT_DATA_H_
#define DRC_RE_BOOL_INPUT_CONSTRAINT_DATA_H_

#include "drc_re_base_constraint_data.h"


class DRC_RE_BOOL_INPUT_CONSTRAINT_DATA : public DRC_RE_BASE_CONSTRAINT_DATA
{
public:
    DRC_RE_BOOL_INPUT_CONSTRAINT_DATA() = default;

    explicit DRC_RE_BOOL_INPUT_CONSTRAINT_DATA( const DRC_RE_BASE_CONSTRAINT_DATA& aBaseData ) :
            DRC_RE_BASE_CONSTRAINT_DATA( aBaseData )
    {
    }

    explicit DRC_RE_BOOL_INPUT_CONSTRAINT_DATA( int aId, int aParentId, bool aBoolInputValue,
                                                wxString aRuleName ) :
            DRC_RE_BASE_CONSTRAINT_DATA( aId, aParentId, aRuleName ),
            m_boolInputValue( aBoolInputValue )
    {
    }

    virtual ~DRC_RE_BOOL_INPUT_CONSTRAINT_DATA() = default;

    VALIDATION_RESULT Validate() const override
    {
        // Boolean inputs are always valid - any true/false value is acceptable
        return VALIDATION_RESULT();
    }

    wxString GenerateRule( const RULE_GENERATION_CONTEXT& aContext ) override
    {
        if( !m_boolInputValue )
            return wxEmptyString;

        wxString code = GetConstraintCode();
        return buildRule( aContext, { wxString::Format( "(constraint %s)", code ) } );
    }

    bool GetBoolInputValue() { return m_boolInputValue; }

    void SetBoolInputValue( bool aBoolInputValue ) { m_boolInputValue = aBoolInputValue; }

    void CopyFrom( const ICopyable& aSource ) override
    {
        const auto& source = dynamic_cast<const DRC_RE_BOOL_INPUT_CONSTRAINT_DATA&>( aSource );

        DRC_RE_BASE_CONSTRAINT_DATA::CopyFrom( source );

        m_boolInputValue = source.m_boolInputValue;
    }

private:
    bool m_boolInputValue{ false };
};

#endif // DRC_RE_BOOL_INPUT_CONSTRAINT_DATA_H_