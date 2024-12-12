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

    explicit DRC_RE_ROUTING_DIFF_PAIR_CONSTRAINT_DATA( int aId, int aParentId, wxString aRuleName,
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

    VALIDATION_RESULT Validate() const override
    {
        VALIDATION_RESULT result;

        // Validate max uncoupled length is positive
        if( m_maxUncoupledLength <= 0 )
            result.AddError( "Maximum Uncoupled Length must be greater than 0" );

        // Validate width values are positive
        if( m_minWidth <= 0 )
            result.AddError( "Minimum Width must be greater than 0" );

        if( m_preferredWidth <= 0 )
            result.AddError( "Preferred Width must be greater than 0" );

        if( m_maxWidth <= 0 )
            result.AddError( "Maximum Width must be greater than 0" );

        // Validate gap values are positive
        if( m_minGap <= 0 )
            result.AddError( "Minimum Gap must be greater than 0" );

        if( m_preferredGap <= 0 )
            result.AddError( "Preferred Gap must be greater than 0" );

        if( m_maxGap <= 0 )
            result.AddError( "Maximum Gap must be greater than 0" );

        // Validate min <= preferred <= max for width
        if( m_minWidth > m_preferredWidth )
            result.AddError( "Minimum Width cannot be greater than Preferred Width" );

        if( m_preferredWidth > m_maxWidth )
            result.AddError( "Preferred Width cannot be greater than Maximum Width" );

        if( m_minWidth > m_maxWidth )
            result.AddError( "Minimum Width cannot be greater than Maximum Width" );

        // Validate min <= preferred <= max for gap
        if( m_minGap > m_preferredGap )
            result.AddError( "Minimum Gap cannot be greater than Preferred Gap" );

        if( m_preferredGap > m_maxGap )
            result.AddError( "Preferred Gap cannot be greater than Maximum Gap" );

        if( m_minGap > m_maxGap )
            result.AddError( "Minimum Gap cannot be greater than Maximum Gap" );

        return result;
    }

    wxString GenerateRule( const RULE_GENERATION_CONTEXT& aContext ) override
    {
        auto formatDistance = [&]( double aValue )
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

        return buildRule( aContext, { widthClause, gapClause, uncoupledClause } );
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
    }

private:
    double m_maxUncoupledLength{ 0 };
    double m_minWidth{ 0 };
    double m_preferredWidth{ 0 };
    double m_maxWidth{ 0 };
    double m_minGap{ 0 };
    double m_preferredGap{ 0 };
    double m_maxGap{ 0 };
};

#endif // DRC_RE_ROUTING_DIFF_PAIR_CONSTRAINT_DATA_H_