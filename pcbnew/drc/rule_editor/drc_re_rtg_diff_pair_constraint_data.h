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

#ifndef DRC_RE_ROUTING_DIFF_PAIR_CONSTRAINT_DATA_H_
#define DRC_RE_ROUTING_DIFF_PAIR_CONSTRAINT_DATA_H_

#include "drc_re_base_constraint_data.h"


class DRC_RE_ROUTING_DIFF_PAIR_CONSTRAINT_DATA : public DRC_RE_BASE_CONSTRAINT_DATA
{
public:
    DRC_RE_ROUTING_DIFF_PAIR_CONSTRAINT_DATA() = default;

    explicit DRC_RE_ROUTING_DIFF_PAIR_CONSTRAINT_DATA(
            const DRC_RE_BASE_CONSTRAINT_DATA& aBaseData ) :
            DRC_RE_BASE_CONSTRAINT_DATA( aBaseData )
    {
    }

    explicit DRC_RE_ROUTING_DIFF_PAIR_CONSTRAINT_DATA( int aId, int aParentId, const wxString& aRuleName,
                                                       double aMaxUncoupledLength, double aMinWidth,
                                                       double aPreferredWidth, double aMaxWidth,
                                                       double aMinGap, double aPreferredGap,
                                                       double aMaxGap ) :
            DRC_RE_BASE_CONSTRAINT_DATA( aId, aParentId, aRuleName ),
            m_maxUncoupledLength( aMaxUncoupledLength ), m_minWidth( aMinWidth ),
            m_preferredWidth( aPreferredWidth ), m_maxWidth( aMaxWidth ), m_minGap( aMinGap ),
            m_preferredGap( aPreferredGap ), m_maxGap( aMaxGap )
    {
    }

    virtual ~DRC_RE_ROUTING_DIFF_PAIR_CONSTRAINT_DATA() = default;

    BITMAPS GetOverlayBitmap() const override { return BITMAPS::constraint_routing_diff_pair; }

    std::vector<DRC_RE_FIELD_POSITION> GetFieldPositions() const override
    {
        // Positions measured from constraint_routing_diff_pair.png bitmap (~570x160)
        // Format: { xStart, xEnd, yTop, tabOrder }
        return {
            { 162 + DRC_RE_OVERLAY_XO, 202 + DRC_RE_OVERLAY_XO, 39 + DRC_RE_OVERLAY_YO, 1, wxS( "mm" ), LABEL_POSITION::RIGHT },  // min_width (left side, top arrow)
            { 302 + DRC_RE_OVERLAY_XO, 342 + DRC_RE_OVERLAY_XO, 39 + DRC_RE_OVERLAY_YO, 2, wxS( "mm" ), LABEL_POSITION::RIGHT },  // opt_width (left side, middle)
            { 480 + DRC_RE_OVERLAY_XO, 520 + DRC_RE_OVERLAY_XO, 39 + DRC_RE_OVERLAY_YO, 3, wxS( "mm" ), LABEL_POSITION::RIGHT },  // max_width (left side, bottom)
            { 213 + DRC_RE_OVERLAY_XO, 253 + DRC_RE_OVERLAY_XO, 215 + DRC_RE_OVERLAY_YO, 4, wxS( "mm" ), LABEL_POSITION::RIGHT }, // min_gap (center-left, top)
            { 386 + DRC_RE_OVERLAY_XO, 426 + DRC_RE_OVERLAY_XO, 215 + DRC_RE_OVERLAY_YO, 5, wxS( "mm" ), LABEL_POSITION::RIGHT }, // opt_gap (center-left, middle)
            { 600 + DRC_RE_OVERLAY_XO, 640 + DRC_RE_OVERLAY_XO, 215 + DRC_RE_OVERLAY_YO, 6, wxS( "mm" ), LABEL_POSITION::RIGHT }, // max_gap (center-left, bottom)
            { 120 + DRC_RE_OVERLAY_XO, 160 + DRC_RE_OVERLAY_XO, 204 + DRC_RE_OVERLAY_YO, 7, wxS( "mm" ), LABEL_POSITION::RIGHT }, // max_uncoupled (right side)
            { 798 + DRC_RE_OVERLAY_XO, 838 + DRC_RE_OVERLAY_XO, 168 + DRC_RE_OVERLAY_YO, 8, wxS( "mm" ), LABEL_POSITION::RIGHT }, // max_skew (to be added, right side)
        };
    }

    VALIDATION_RESULT Validate() const override
    {
        VALIDATION_RESULT result;

        if( m_minWidth <= 0 )
            result.AddError( _( "Minimum Width must be greater than 0" ) );

        if( m_preferredWidth <= 0 )
            result.AddError( _( "Preferred Width must be greater than 0" ) );

        if( m_maxWidth <= 0 )
            result.AddError( _( "Maximum Width must be greater than 0" ) );

        if( m_minGap <= 0 )
            result.AddError( _( "Minimum Gap must be greater than 0" ) );

        if( m_preferredGap <= 0 )
            result.AddError( _( "Preferred Gap must be greater than 0" ) );

        if( m_maxGap <= 0 )
            result.AddError( _( "Maximum Gap must be greater than 0" ) );

        if( m_maxUncoupledLength <= 0 )
            result.AddError( _( "Maximum Uncoupled Length must be greater than 0" ) );

        if( m_maxSkew < 0 )
            result.AddError( _( "Maximum Skew must be greater than or equal to 0" ) );

        if( result.isValid )
        {
            if( m_minWidth > m_preferredWidth )
                result.AddError( _( "Minimum Width cannot be greater than Preferred Width" ) );

            if( m_preferredWidth > m_maxWidth )
                result.AddError( _( "Preferred Width cannot be greater than Maximum Width" ) );

            if( m_minWidth > m_maxWidth )
                result.AddError( _( "Minimum Width cannot be greater than Maximum Width" ) );

            if( m_minGap > m_preferredGap )
                result.AddError( _( "Minimum Gap cannot be greater than Preferred Gap" ) );

            if( m_preferredGap > m_maxGap )
                result.AddError( _( "Preferred Gap cannot be greater than Maximum Gap" ) );

            if( m_minGap > m_maxGap )
                result.AddError( _( "Minimum Gap cannot be greater than Maximum Gap" ) );
        }

        return result;
    }

    std::vector<wxString> GetConstraintClauses( const RULE_GENERATION_CONTEXT& aContext ) const override
    {
        auto formatDistance = []( double aValue )
        {
            return formatDouble( aValue ) + wxS( "mm" );
        };

        wxString widthClause = wxString::Format(
                wxS( "(constraint track_width (min %s) (opt %s) (max %s))" ),
                formatDistance( m_minWidth ),
                formatDistance( m_preferredWidth ),
                formatDistance( m_maxWidth ) );

        wxString gapClause = wxString::Format(
                wxS( "(constraint diff_pair_gap (min %s) (opt %s) (max %s))" ),
                formatDistance( m_minGap ),
                formatDistance( m_preferredGap ),
                formatDistance( m_maxGap ) );

        wxString uncoupledClause = wxString::Format(
                wxS( "(constraint diff_pair_uncoupled (max %s))" ),
                formatDistance( m_maxUncoupledLength ) );

        std::vector<wxString> clauses = { widthClause, gapClause, uncoupledClause };

        if( m_maxSkew > 0 )
        {
            wxString skewClause = wxString::Format( wxS( "(constraint skew (max %s))" ), formatDistance( m_maxSkew ) );
            clauses.push_back( skewClause );
        }

        return clauses;
    }

    wxString GenerateRule( const RULE_GENERATION_CONTEXT& aContext ) override
    {
        return buildRule( aContext, GetConstraintClauses( aContext ) );
    }

    double GetMaxUncoupledLength() { return m_maxUncoupledLength; }

    void SetMaxUncoupledLength( double aMaxUncoupledLength )
    {
        m_maxUncoupledLength = aMaxUncoupledLength;
    }

    double GetMinWidth() { return m_minWidth; }

    void SetMinWidth( double aMinWidth ) { m_minWidth = aMinWidth; }

    double GetPreferredWidth() { return m_preferredWidth; }

    void SetPreferredWidth( double aPreferredWidth ) { m_preferredWidth = aPreferredWidth; }

    double GetMaxWidth() { return m_maxWidth; }

    void SetMaxWidth( double aMaxWidth ) { m_maxWidth = aMaxWidth; }

    double GetMinGap() { return m_minGap; }

    void SetMinGap( double aMinGap ) { m_minGap = aMinGap; }

    double GetPreferredGap() { return m_preferredGap; }

    void SetPreferredGap( double aPreferredGap ) { m_preferredGap = aPreferredGap; }

    double GetMaxGap() { return m_maxGap; }

    void SetMaxGap( double aMaxGap ) { m_maxGap = aMaxGap; }

    double GetMaxSkew() { return m_maxSkew; }

    void SetMaxSkew( double aMaxSkew ) { m_maxSkew = aMaxSkew; }

    void CopyFrom( const ICopyable& aSource ) override
    {
        const auto& source =
                dynamic_cast<const DRC_RE_ROUTING_DIFF_PAIR_CONSTRAINT_DATA&>( aSource );

        DRC_RE_BASE_CONSTRAINT_DATA::CopyFrom( source );

        m_maxUncoupledLength = source.m_maxUncoupledLength;
        m_minWidth = source.m_minWidth;
        m_preferredWidth = source.m_preferredWidth;
        m_maxWidth = source.m_maxWidth;
        m_minGap = source.m_minGap;
        m_preferredGap = source.m_preferredGap;
        m_maxGap = source.m_maxGap;
        m_maxSkew = source.m_maxSkew;
    }

private:
    double m_maxUncoupledLength{ 0 };
    double m_minWidth{ 0 };
    double m_preferredWidth{ 0 };
    double m_maxWidth{ 0 };
    double m_minGap{ 0 };
    double m_preferredGap{ 0 };
    double m_maxGap{ 0 };
    double m_maxSkew{ 0 };
};

#endif // DRC_RE_ROUTING_DIFF_PAIR_CONSTRAINT_DATA_H_
