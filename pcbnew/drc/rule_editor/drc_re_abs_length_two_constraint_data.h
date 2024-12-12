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

#ifndef DRC_RE_ABSOLUTE_LENGTH_TWO_CONSTRAINT_DATA_H_
#define DRC_RE_ABSOLUTE_LENGTH_TWO_CONSTRAINT_DATA_H_

#include "drc_re_base_constraint_data.h"


class DRC_RE_ABSOLUTE_LENGTH_TWO_CONSTRAINT_DATA : public DRC_RE_BASE_CONSTRAINT_DATA
{
public:
    DRC_RE_ABSOLUTE_LENGTH_TWO_CONSTRAINT_DATA() = default;

    explicit DRC_RE_ABSOLUTE_LENGTH_TWO_CONSTRAINT_DATA(
            const DRC_RE_BASE_CONSTRAINT_DATA& aBaseData ) :
            DRC_RE_BASE_CONSTRAINT_DATA( aBaseData )
    {
    }

    explicit DRC_RE_ABSOLUTE_LENGTH_TWO_CONSTRAINT_DATA( int aId, int aParentId,
                                                         double   aMinLength,
                                                         double   aOptLength,
                                                         double   aMaxLength,
                                                         wxString aRuleName ) :
            DRC_RE_BASE_CONSTRAINT_DATA( aId, aParentId, aRuleName ),
            m_minLength( aMinLength ),
            m_optLength( aOptLength ),
            m_maxLength( aMaxLength )
    {
    }

    virtual ~DRC_RE_ABSOLUTE_LENGTH_TWO_CONSTRAINT_DATA() = default;

    double GetMinimumLength() const { return m_minLength; }

    void SetMinimumLength( double aLength ) { m_minLength = aLength; }

    double GetOptimumLength() const { return m_optLength; }

    void SetOptimumLength( double aLength ) { m_optLength = aLength; }

    double GetMaximumLength() const { return m_maxLength; }

    void SetMaximumLength( double aLength ) { m_maxLength = aLength; }

    VALIDATION_RESULT Validate() const override
    {
        VALIDATION_RESULT result;

        // Validate length values are positive
        if( m_minLength <= 0 )
            result.AddError( "Minimum Length must be greater than 0" );

        if( m_optLength <= 0 )
            result.AddError( "Optimum Length must be greater than 0" );

        if( m_maxLength <= 0 )
            result.AddError( "Maximum Length must be greater than 0" );

        // Validate min <= opt <= max
        if( m_minLength > m_optLength )
            result.AddError( "Minimum Length cannot be greater than Optimum Length" );

        if( m_optLength > m_maxLength )
            result.AddError( "Optimum Length cannot be greater than Maximum Length" );

        if( m_minLength > m_maxLength )
            result.AddError( "Minimum Length cannot be greater than Maximum Length" );

        return result;
    }

    wxString GenerateRule( const RULE_GENERATION_CONTEXT& aContext ) override
    {
        auto formatDistance = [&]( double aValue )
        {
            return formatDouble( aValue ) + wxS( "mm" );
        };

        wxString code = GetConstraintCode();

        if( code.IsEmpty() )
            code = wxS( "length" );

        wxString clause = wxString::Format(
                wxS( "(constraint %s (min %s) (opt %s) (max %s))" ),
                code,
                formatDistance( m_minLength ),
                formatDistance( m_optLength ),
                formatDistance( m_maxLength ) );

        return buildRule( aContext, { clause } );
    }

    void CopyFrom( const ICopyable& aSource ) override
    {
        const auto& source =
                dynamic_cast<const DRC_RE_ABSOLUTE_LENGTH_TWO_CONSTRAINT_DATA&>( aSource );

        DRC_RE_BASE_CONSTRAINT_DATA::CopyFrom( source );

        m_minLength = source.m_minLength;
        m_optLength = source.m_optLength;
        m_maxLength = source.m_maxLength;
    }

private:
    double m_minLength{ 0 };
    double m_optLength{ 0 };
    double m_maxLength{ 0 };
};

#endif // DRC_RE_ABSOLUTE_LENGTH_TWO_CONSTRAINT_DATA_H_
