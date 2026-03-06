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

    explicit DRC_RE_ABSOLUTE_LENGTH_TWO_CONSTRAINT_DATA( int aId, int aParentId, double aOptLength, double aTolerance,
                                                         const wxString& aRuleName ) :
            DRC_RE_BASE_CONSTRAINT_DATA( aId, aParentId, aRuleName ),
            m_optLength( aOptLength ),
            m_tolerance( aTolerance )
    {
    }

    virtual ~DRC_RE_ABSOLUTE_LENGTH_TWO_CONSTRAINT_DATA() = default;

    BITMAPS GetOverlayBitmap() const override { return BITMAPS::constraint_absolute_length_2; }

    std::vector<DRC_RE_FIELD_POSITION> GetFieldPositions() const override
    {
        // Format: { xStart, xEnd, yTop, tabOrder }
        // Two fields side-by-side, opt_length and tolerance
        return {
            { 80 + DRC_RE_OVERLAY_XO, 120 + DRC_RE_OVERLAY_XO, 115 + DRC_RE_OVERLAY_YO, 1, wxS( "mm" ),
              LABEL_POSITION::RIGHT }, // opt_length
            { 160 + DRC_RE_OVERLAY_XO, 200 + DRC_RE_OVERLAY_XO, 115 + DRC_RE_OVERLAY_YO, 2, wxS( "mm" ),
              LABEL_POSITION::RIGHT }, // tolerance
        };
    }

    double GetOptimumLength() const { return m_optLength; }

    void SetOptimumLength( double aLength ) { m_optLength = aLength; }

    double GetTolerance() const { return m_tolerance; }

    void SetTolerance( double aTolerance ) { m_tolerance = aTolerance; }

    // Computed from opt +/- tolerance
    double GetMinimumLength() const { return m_optLength - m_tolerance; }

    double GetMaximumLength() const { return m_optLength + m_tolerance; }

    VALIDATION_RESULT Validate() const override
    {
        VALIDATION_RESULT result;

        if( m_optLength <= 0 )
            result.AddError( _( "Optimum Length must be greater than 0" ) );

        if( m_tolerance < 0 )
            result.AddError( _( "Tolerance must be greater than or equal to 0" ) );

        if( result.isValid )
        {
            if( GetMinimumLength() <= 0 )
                result.AddError( _( "Tolerance is too large: resulting minimum length is not positive" ) );
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

        wxString clause = wxString::Format( wxS( "(constraint %s (min %s) (opt %s) (max %s))" ), code,
                                            formatDistance( GetMinimumLength() ), formatDistance( m_optLength ),
                                            formatDistance( GetMaximumLength() ) );

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

        m_optLength = source.m_optLength;
        m_tolerance = source.m_tolerance;
    }

private:
    double m_optLength{ 0 };
    double m_tolerance{ 0 };
};

class DRC_RE_MATCHED_LENGTH_DIFF_PAIR_CONSTRAINT_DATA : public DRC_RE_ABSOLUTE_LENGTH_TWO_CONSTRAINT_DATA
{
public:
    using DRC_RE_ABSOLUTE_LENGTH_TWO_CONSTRAINT_DATA::DRC_RE_ABSOLUTE_LENGTH_TWO_CONSTRAINT_DATA;

    BITMAPS GetOverlayBitmap() const override { return BITMAPS::constraint_matched_length_diff_pair_v2; }

    std::vector<DRC_RE_FIELD_POSITION> GetFieldPositions() const override
    {
        return {
            { 80 + DRC_RE_OVERLAY_XO, 120 + DRC_RE_OVERLAY_XO, 130 + DRC_RE_OVERLAY_YO, 1, wxS( "mm" ),
              LABEL_POSITION::RIGHT }, // opt_length
            { 160 + DRC_RE_OVERLAY_XO, 200 + DRC_RE_OVERLAY_XO, 130 + DRC_RE_OVERLAY_YO, 2, wxS( "mm" ),
              LABEL_POSITION::RIGHT }, // tolerance
            { 25 + DRC_RE_OVERLAY_XO, 65 + DRC_RE_OVERLAY_XO, 0 + DRC_RE_OVERLAY_YO, 3, wxS( "mm" ),
              LABEL_POSITION::RIGHT }, // max_skew
        };
    }

    double GetMaxSkew() const { return m_maxSkew; }

    void SetMaxSkew( double aSkew ) { m_maxSkew = aSkew; }

    bool GetWithinDiffPairs() const { return m_withinDiffPairs; }

    void SetWithinDiffPairs( bool aWithinDiffPairs ) { m_withinDiffPairs = aWithinDiffPairs; }

    std::vector<wxString> GetConstraintClauses( const RULE_GENERATION_CONTEXT& aContext ) const override
    {
        std::vector<wxString> clauses;

        if( GetOptimumLength() > 0 )
            clauses = DRC_RE_ABSOLUTE_LENGTH_TWO_CONSTRAINT_DATA::GetConstraintClauses( aContext );

        if( m_maxSkew > 0 )
        {
            wxString skewClause;

            if( m_withinDiffPairs )
            {
                skewClause = wxString::Format( wxS( "(constraint skew (max %smm) (within_diff_pairs))" ),
                                               formatDouble( m_maxSkew ) );
            }
            else
            {
                skewClause = wxString::Format( wxS( "(constraint skew (max %smm))" ), formatDouble( m_maxSkew ) );
            }

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
        m_withinDiffPairs = source.m_withinDiffPairs;
    }

    VALIDATION_RESULT Validate() const override
    {
        VALIDATION_RESULT result;

        bool hasLength = ( GetOptimumLength() > 0 || GetTolerance() > 0 );
        bool hasSkew = ( m_maxSkew != 0 );

        if( !hasLength && !hasSkew )
            result.AddError( _( "At least one constraint must be defined" ) );

        if( hasLength )
            result = DRC_RE_ABSOLUTE_LENGTH_TWO_CONSTRAINT_DATA::Validate();

        if( m_maxSkew < 0 )
            result.AddError( _( "Maximum Skew must be greater than or equal to 0" ) );

        return result;
    }

private:
    double m_maxSkew{ 0 };
    bool   m_withinDiffPairs{ false };
};

#endif // DRC_RE_ABSOLUTE_LENGTH_TWO_CONSTRAINT_DATA_H_
