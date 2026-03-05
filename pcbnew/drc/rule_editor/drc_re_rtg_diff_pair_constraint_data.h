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
                                                       double aOptWidth, double aWidthTolerance, double aOptGap,
                                                       double aGapTolerance, double aMaxUncoupledLength ) :
            DRC_RE_BASE_CONSTRAINT_DATA( aId, aParentId, aRuleName ),
            m_optWidth( aOptWidth ),
            m_widthTolerance( aWidthTolerance ),
            m_optGap( aOptGap ),
            m_gapTolerance( aGapTolerance ),
            m_maxUncoupledLength( aMaxUncoupledLength )
    {
    }

    virtual ~DRC_RE_ROUTING_DIFF_PAIR_CONSTRAINT_DATA() = default;

    BITMAPS GetOverlayBitmap() const override { return BITMAPS::constraint_routing_diff_pair; }

    std::vector<DRC_RE_FIELD_POSITION> GetFieldPositions() const override
    {
        // Positions measured from constraint_routing_diff_pair.png bitmap (~423x133)
        // Format: { xStart, xEnd, yTop, tabOrder }
        // TODO: measure actual positions from PNG
        return {
            { 0 + DRC_RE_OVERLAY_XO, 40 + DRC_RE_OVERLAY_XO, 135 + DRC_RE_OVERLAY_YO, 1, wxS( "mm" ),
              LABEL_POSITION::RIGHT }, // opt_width
            { 80 + DRC_RE_OVERLAY_XO, 120 + DRC_RE_OVERLAY_XO, 135 + DRC_RE_OVERLAY_YO, 2, wxS( "mm" ),
              LABEL_POSITION::RIGHT }, // width_tolerance (±)
            { 185 + DRC_RE_OVERLAY_XO, 225 + DRC_RE_OVERLAY_XO, 115 + DRC_RE_OVERLAY_YO, 3, wxS( "mm" ),
              LABEL_POSITION::RIGHT }, // opt_gap
            { 265 + DRC_RE_OVERLAY_XO, 305 + DRC_RE_OVERLAY_XO, 115 + DRC_RE_OVERLAY_YO, 4, wxS( "mm" ),
              LABEL_POSITION::RIGHT }, // gap_tolerance (±)
            { 95 + DRC_RE_OVERLAY_XO, 135 + DRC_RE_OVERLAY_XO, 10 + DRC_RE_OVERLAY_YO, 5, wxS( "mm" ),
              LABEL_POSITION::RIGHT }, // max_uncoupled
        };
    }

    VALIDATION_RESULT Validate() const override
    {
        VALIDATION_RESULT result;

        bool hasWidth = m_optWidth > 0;

        if( hasWidth )
        {
            if( m_widthTolerance < 0 )
                result.AddError( _( "Width Tolerance must be greater than or equal to 0" ) );

            if( m_widthTolerance >= m_optWidth )
                result.AddError( _( "Width Tolerance must be less than Optimum Width" ) );
        }

        bool hasGap = m_optGap > 0;

        if( hasGap )
        {
            if( m_gapTolerance < 0 )
                result.AddError( _( "Gap Tolerance must be greater than or equal to 0" ) );

            if( m_gapTolerance >= m_optGap )
                result.AddError( _( "Gap Tolerance must be less than Optimum Gap" ) );
        }

        if( m_maxUncoupledLength < 0 )
            result.AddError( _( "Maximum Uncoupled Length must be greater than or equal to 0" ) );

        if( !hasWidth && !hasGap && m_maxUncoupledLength <= 0 )
            result.AddError( _( "At least one constraint must be specified" ) );

        return result;
    }

    std::vector<wxString> GetConstraintClauses( const RULE_GENERATION_CONTEXT& aContext ) const override
    {
        auto formatDistance = []( double aValue )
        {
            return formatDouble( aValue ) + wxS( "mm" );
        };

        std::vector<wxString> clauses;

        if( m_optWidth > 0 )
        {
            double minWidth = m_optWidth - m_widthTolerance;
            double maxWidth = m_optWidth + m_widthTolerance;

            clauses.push_back( wxString::Format( wxS( "(constraint track_width (min %s) (opt %s) (max %s))" ),
                                                 formatDistance( minWidth ), formatDistance( m_optWidth ),
                                                 formatDistance( maxWidth ) ) );
        }

        if( m_optGap > 0 )
        {
            double minGap = m_optGap - m_gapTolerance;
            double maxGap = m_optGap + m_gapTolerance;

            clauses.push_back( wxString::Format( wxS( "(constraint diff_pair_gap (min %s) (opt %s) (max %s))" ),
                                                 formatDistance( minGap ), formatDistance( m_optGap ),
                                                 formatDistance( maxGap ) ) );
        }

        if( m_maxUncoupledLength > 0 )
        {
            clauses.push_back( wxString::Format( wxS( "(constraint diff_pair_uncoupled (max %s))" ),
                                                 formatDistance( m_maxUncoupledLength ) ) );
        }

        return clauses;
    }

    wxString GenerateRule( const RULE_GENERATION_CONTEXT& aContext ) override
    {
        return buildRule( aContext, GetConstraintClauses( aContext ) );
    }

    double GetOptWidth() { return m_optWidth; }
    void   SetOptWidth( double aOptWidth ) { m_optWidth = aOptWidth; }

    double GetWidthTolerance() { return m_widthTolerance; }
    void   SetWidthTolerance( double aTolerance ) { m_widthTolerance = aTolerance; }

    double GetOptGap() { return m_optGap; }
    void   SetOptGap( double aOptGap ) { m_optGap = aOptGap; }

    double GetGapTolerance() { return m_gapTolerance; }
    void   SetGapTolerance( double aTolerance ) { m_gapTolerance = aTolerance; }

    double GetMaxUncoupledLength() { return m_maxUncoupledLength; }
    void   SetMaxUncoupledLength( double aMaxUncoupledLength ) { m_maxUncoupledLength = aMaxUncoupledLength; }

    void CopyFrom( const ICopyable& aSource ) override
    {
        const auto& source =
                dynamic_cast<const DRC_RE_ROUTING_DIFF_PAIR_CONSTRAINT_DATA&>( aSource );

        DRC_RE_BASE_CONSTRAINT_DATA::CopyFrom( source );

        m_optWidth = source.m_optWidth;
        m_widthTolerance = source.m_widthTolerance;
        m_optGap = source.m_optGap;
        m_gapTolerance = source.m_gapTolerance;
        m_maxUncoupledLength = source.m_maxUncoupledLength;
    }

private:
    double m_optWidth{ 0 };
    double m_widthTolerance{ 0 };
    double m_optGap{ 0 };
    double m_gapTolerance{ 0 };
    double m_maxUncoupledLength{ 0 };
};

#endif // DRC_RE_ROUTING_DIFF_PAIR_CONSTRAINT_DATA_H_
