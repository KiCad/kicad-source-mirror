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

#ifndef DRC_RE_NUMERIC_INPUT_CONSTRAINT_DATA_H_
#define DRC_RE_NUMERIC_INPUT_CONSTRAINT_DATA_H_

#include "drc_re_base_constraint_data.h"


class DRC_RE_NUMERIC_INPUT_CONSTRAINT_DATA : public DRC_RE_BASE_CONSTRAINT_DATA
{
public:
    DRC_RE_NUMERIC_INPUT_CONSTRAINT_DATA() = default;

    explicit DRC_RE_NUMERIC_INPUT_CONSTRAINT_DATA( const DRC_RE_BASE_CONSTRAINT_DATA& aBaseData ) :
            DRC_RE_BASE_CONSTRAINT_DATA( aBaseData )
    {
    }

    explicit DRC_RE_NUMERIC_INPUT_CONSTRAINT_DATA( int aId, int aParentId,
                                                   double aNumericInputValue, wxString aRuleName ) :
            DRC_RE_BASE_CONSTRAINT_DATA( aId, aParentId, aRuleName ),
            m_numericInputValue( aNumericInputValue )
    {
    }

    virtual ~DRC_RE_NUMERIC_INPUT_CONSTRAINT_DATA() = default;

    wxString GenerateRule( const RULE_GENERATION_CONTEXT& aContext ) override
    {
        wxString code = GetConstraintCode();
        wxString valueStr = formatDouble( m_numericInputValue );

        if( code == "via_count" )
        {
            return buildRule( aContext, { wxString::Format( "(constraint %s (max %s))", code, valueStr ) } );
        }
        else if( code == "track_angle" )
        {
            return buildRule( aContext, { wxString::Format( "(constraint %s (min %sdeg))", code, valueStr ) } );
        }
        else if( code == "maximum_allowed_deviation" )
        {
            return buildRule( aContext, { wxString::Format( "(constraint %s (max %smm))", code, valueStr ) } );
        }
        else
        {
            return buildRule( aContext, { wxString::Format( "(constraint %s (min %smm))", code, valueStr ) } );
        }
    }

    double GetNumericInputValue() { return m_numericInputValue; }

    void SetNumericInputValue( double aNumericInput ) { m_numericInputValue = aNumericInput; }

    VALIDATION_RESULT Validate() const override
    {
        VALIDATION_RESULT result;

        if( m_numericInputValue <= 0 )
            result.AddError( "Numeric input value must be greater than 0" );

        return result;
    }

    void CopyFrom( const ICopyable& aSource ) override
    {
        const auto& source = dynamic_cast<const DRC_RE_NUMERIC_INPUT_CONSTRAINT_DATA&>( aSource );

        DRC_RE_BASE_CONSTRAINT_DATA::CopyFrom( source );

        m_numericInputValue = source.m_numericInputValue;
    }

private:
    double m_numericInputValue{ 0 };
};

#endif // DRC_RE_NUMERIC_INPUT_CONSTRAINT_DATA_H_