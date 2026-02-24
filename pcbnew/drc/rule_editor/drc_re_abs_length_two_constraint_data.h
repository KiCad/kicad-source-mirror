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

    BITMAPS GetOverlayBitmap() const override { return BITMAPS::constraint_absolute_length_2; }

    std::vector<DRC_RE_FIELD_POSITION> GetFieldPositions() const override
    {
        // Positions measured from constraint_absolute_lenght_2.png (~340x130)
        // Format: { xStart, xEnd, yTop, tabOrder }
        return {
            { 285 + DRC_RE_OVERLAY_XO, 315 + DRC_RE_OVERLAY_XO, 62 + DRC_RE_OVERLAY_YO, 1, wxS( "mm" ), LABEL_POSITION::RIGHT },  // min_length (left arrow)
            { 245 + DRC_RE_OVERLAY_XO, 275 + DRC_RE_OVERLAY_XO, 131 + DRC_RE_OVERLAY_YO, 2, wxS( "mm" ), LABEL_POSITION::RIGHT }, // opt_length (bottom center arrow)
            { 15 + DRC_RE_OVERLAY_XO, 45 + DRC_RE_OVERLAY_XO, 37 + DRC_RE_OVERLAY_YO, 3, wxS( "mm" ), LABEL_POSITION::RIGHT },    // max_length (right arrow)
        };
    }

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
            result.AddError( _( "Minimum Length must be greater than 0" ) );

        if( m_optLength <= 0 )
            result.AddError( _( "Optimum Length must be greater than 0" ) );

        if( m_maxLength <= 0 )
            result.AddError( _( "Maximum Length must be greater than 0" ) );

        if( result.isValid )
        {
            // Validate min <= opt <= max
            if( m_minLength > m_optLength )
                result.AddError( _( "Minimum Length cannot be greater than Optimum Length" ) );

            if( m_optLength > m_maxLength )
                result.AddError( _( "Optimum Length cannot be greater than Maximum Length" ) );

            if( m_minLength > m_maxLength )
                result.AddError( _( "Minimum Length cannot be greater than Maximum Length" ) );
        }

        return result;
    }

    std::vector<wxString> GetConstraintClauses( const RULE_GENERATION_CONTEXT& aContext ) const override
    {
        auto formatDistance = []( double aValue )
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

        return { clause };
    }

    wxString GenerateRule( const RULE_GENERATION_CONTEXT& aContext ) override
    {
        return buildRule( aContext, GetConstraintClauses( aContext ) );
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

class DRC_RE_MATCHED_LENGTH_DIFF_PAIR_CONSTRAINT_DATA : public DRC_RE_ABSOLUTE_LENGTH_TWO_CONSTRAINT_DATA
{
public:
    using DRC_RE_ABSOLUTE_LENGTH_TWO_CONSTRAINT_DATA::DRC_RE_ABSOLUTE_LENGTH_TWO_CONSTRAINT_DATA;

    BITMAPS GetOverlayBitmap() const override { return BITMAPS::constraint_matched_length_diff_pair; }

    std::vector<DRC_RE_FIELD_POSITION> GetFieldPositions() const override
    {
        return {
            { 335 + DRC_RE_OVERLAY_XO, 365 + DRC_RE_OVERLAY_XO, 33 + DRC_RE_OVERLAY_YO, 1, wxS( "mm" ), LABEL_POSITION::RIGHT },  // min_length
            { 335 + DRC_RE_OVERLAY_XO, 365 + DRC_RE_OVERLAY_XO, 103 + DRC_RE_OVERLAY_YO, 2, wxS( "mm" ), LABEL_POSITION::RIGHT }, // opt_length
            { 335 + DRC_RE_OVERLAY_XO, 365 + DRC_RE_OVERLAY_XO, 173 + DRC_RE_OVERLAY_YO, 3, wxS( "mm" ), LABEL_POSITION::RIGHT }, // max_length
        };
    }

    double GetMaxSkew() const { return m_maxSkew; }

    void SetMaxSkew( double aSkew ) { m_maxSkew = aSkew; }

    std::vector<wxString> GetConstraintClauses( const RULE_GENERATION_CONTEXT& aContext ) const override
    {
        std::vector<wxString> clauses = DRC_RE_ABSOLUTE_LENGTH_TWO_CONSTRAINT_DATA::GetConstraintClauses( aContext );

        if( m_maxSkew > 0 )
        {
            wxString skewClause = wxString::Format(
                    wxS( "(constraint skew (max %smm))" ), formatDouble( m_maxSkew ) );
            clauses.push_back( skewClause );
        }

        return clauses;
    }

    void CopyFrom( const ICopyable& aSource ) override
    {
        const auto& source =
                dynamic_cast<const DRC_RE_MATCHED_LENGTH_DIFF_PAIR_CONSTRAINT_DATA&>( aSource );

        DRC_RE_ABSOLUTE_LENGTH_TWO_CONSTRAINT_DATA::CopyFrom( source );

        m_maxSkew = source.m_maxSkew;
    }

private:
    double m_maxSkew{ 0 };
};

#endif // DRC_RE_ABSOLUTE_LENGTH_TWO_CONSTRAINT_DATA_H_
