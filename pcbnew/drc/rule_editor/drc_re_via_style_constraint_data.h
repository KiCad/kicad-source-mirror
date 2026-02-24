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

#ifndef DRC_RE_VIA_STYLE_CONSTRAINT_DATA_H_
#define DRC_RE_VIA_STYLE_CONSTRAINT_DATA_H_

#include "drc_re_base_constraint_data.h"


class DRC_RE_VIA_STYLE_CONSTRAINT_DATA : public DRC_RE_BASE_CONSTRAINT_DATA
{
public:
    DRC_RE_VIA_STYLE_CONSTRAINT_DATA() = default;

    explicit DRC_RE_VIA_STYLE_CONSTRAINT_DATA( const DRC_RE_BASE_CONSTRAINT_DATA& aBaseData ) :
            DRC_RE_BASE_CONSTRAINT_DATA( aBaseData )
    {
    }

    explicit DRC_RE_VIA_STYLE_CONSTRAINT_DATA( int aId, int aParentId, wxString aRuleName,
                                               double aMinViaDiameter, double aMaxViaDiameter,
                                               double aPreferredViaDiameter, double aMinViaHoleSize,
                                               double aMaxViaHoleSize,
                                               double aPreferredViaHoleSize ) :
            DRC_RE_BASE_CONSTRAINT_DATA( aId, aParentId, aRuleName ),
            m_minViaDiameter( aMinViaDiameter ), m_preferredViaDiameter( aPreferredViaDiameter ),
            m_maxViaDiameter( aMaxViaDiameter ), m_minViaHoleSize( aMinViaHoleSize ),
            m_preferredViaHoleSize( aPreferredViaHoleSize ), m_maxViaHoleSize( aMaxViaHoleSize )
    {
    }

    virtual ~DRC_RE_VIA_STYLE_CONSTRAINT_DATA() = default;

    BITMAPS GetOverlayBitmap() const override { return BITMAPS::constraint_via_style; }

    std::vector<DRC_RE_FIELD_POSITION> GetFieldPositions() const override
    {
        // Positions measured from constraint_via_style.png bitmap
        // Format: { xStart, xEnd, yTop, tabOrder }
        return {
            { 300 + DRC_RE_OVERLAY_XO, 340 + DRC_RE_OVERLAY_XO, 15 + DRC_RE_OVERLAY_YO, 1, wxS( "mm" ), LABEL_POSITION::RIGHT },  // min_via_diameter (right side, top)
            { 300 + DRC_RE_OVERLAY_XO, 340 + DRC_RE_OVERLAY_XO, 45 + DRC_RE_OVERLAY_YO, 2, wxS( "mm" ), LABEL_POSITION::RIGHT },  // opt_via_diameter (right side, middle)
            { 300 + DRC_RE_OVERLAY_XO, 340 + DRC_RE_OVERLAY_XO, 75 + DRC_RE_OVERLAY_YO, 3, wxS( "mm" ), LABEL_POSITION::RIGHT },  // max_via_diameter (right side, bottom)
            { 300 + DRC_RE_OVERLAY_XO, 340 + DRC_RE_OVERLAY_XO, 125 + DRC_RE_OVERLAY_YO, 4, wxS( "mm" ), LABEL_POSITION::RIGHT }, // min_via_hole (right side, below diameter)
            { 300 + DRC_RE_OVERLAY_XO, 340 + DRC_RE_OVERLAY_XO, 155 + DRC_RE_OVERLAY_YO, 5, wxS( "mm" ), LABEL_POSITION::RIGHT }, // opt_via_hole
            { 300 + DRC_RE_OVERLAY_XO, 340 + DRC_RE_OVERLAY_XO, 185 + DRC_RE_OVERLAY_YO, 6, wxS( "mm" ), LABEL_POSITION::RIGHT }, // max_via_hole
        };
    }

    VALIDATION_RESULT Validate() const override
    {
        VALIDATION_RESULT result;

        // Validate via diameter values are positive
        if( m_minViaDiameter <= 0 )
            result.AddError( _( "Minimum Via Diameter must be greater than 0" ) );

        if( m_preferredViaDiameter <= 0 )
            result.AddError( _( "Preferred Via Diameter must be greater than 0" ) );

        if( m_maxViaDiameter <= 0 )
            result.AddError( _( "Maximum Via Diameter must be greater than 0" ) );

        // Validate via hole size values are positive
        if( m_minViaHoleSize <= 0 )
            result.AddError( _( "Minimum Via Hole Size must be greater than 0" ) );

        if( m_preferredViaHoleSize <= 0 )
            result.AddError( _( "Preferred Via Hole Size must be greater than 0" ) );

        if( m_maxViaHoleSize <= 0 )
            result.AddError( _( "Maximum Via Hole Size must be greater than 0" ) );

        // Validate min <= preferred <= max for diameter
        if( m_minViaDiameter > m_preferredViaDiameter )
            result.AddError( _( "Minimum Via Diameter cannot be greater than Preferred Via Diameter" ) );

        if( m_preferredViaDiameter > m_maxViaDiameter )
            result.AddError( _( "Preferred Via Diameter cannot be greater than Maximum Via Diameter" ) );

        if( m_minViaDiameter > m_maxViaDiameter )
            result.AddError( _( "Minimum Via Diameter cannot be greater than Maximum Via Diameter" ) );

        // Validate min <= preferred <= max for hole size
        if( m_minViaHoleSize > m_preferredViaHoleSize )
            result.AddError( _( "Minimum Via Hole Size cannot be greater than Preferred Via Hole Size" ) );

        if( m_preferredViaHoleSize > m_maxViaHoleSize )
            result.AddError( _( "Preferred Via Hole Size cannot be greater than Maximum Via Hole Size" ) );

        if( m_minViaHoleSize > m_maxViaHoleSize )
            result.AddError( _( "Minimum Via Hole Size cannot be greater than Maximum Via Hole Size" ) );

        return result;
    }

    std::vector<wxString> GetConstraintClauses( const RULE_GENERATION_CONTEXT& aContext ) const override
    {
        auto formatDimension = []( double aValue )
        {
            return formatDouble( aValue ) + wxS( "mm" );
        };

        wxString diaClause = wxString::Format(
                wxS( "(constraint via_diameter (min %s) (opt %s) (max %s))" ),
                formatDimension( m_minViaDiameter ),
                formatDimension( m_preferredViaDiameter ),
                formatDimension( m_maxViaDiameter ) );

        wxString drillClause = wxString::Format(
                wxS( "(constraint hole_size (min %s) (opt %s) (max %s))" ),
                formatDimension( m_minViaHoleSize ),
                formatDimension( m_preferredViaHoleSize ),
                formatDimension( m_maxViaHoleSize ) );

        return { diaClause, drillClause };
    }

    wxString GenerateRule( const RULE_GENERATION_CONTEXT& aContext ) override
    {
        return buildRule( aContext, GetConstraintClauses( aContext ) );
    }

    double GetMinViaDiameter() { return m_minViaDiameter; }

    void SetMinViaDiameter( double aMinViaDiameter ) { m_minViaDiameter = aMinViaDiameter; }

    double GetMaxViaDiameter() { return m_maxViaDiameter; }

    void SetMaxViaDiameter( double aMaxViaDiameter ) { m_maxViaDiameter = aMaxViaDiameter; }

    double GetPreferredViaDiameter() { return m_preferredViaDiameter; }

    void SetPreferredViaDiameter( double aPreferredViaDiameter )
    {
        m_preferredViaDiameter = aPreferredViaDiameter;
    }

    double GetMinViaHoleSize() { return m_minViaHoleSize; }

    void SetMinViaHoleSize( double aMinViaHoleSize ) { m_minViaHoleSize = aMinViaHoleSize; }

    double GetMaxViaHoleSize() { return m_maxViaHoleSize; }

    void SetMaxViaHoleSize( double aMaxViaHoleSize ) { m_maxViaHoleSize = aMaxViaHoleSize; }

    double GetPreferredViaHoleSize() { return m_preferredViaHoleSize; }

    void SetPreferredViaHoleSize( double aPreferredViaHoleSize )
    {
        m_preferredViaHoleSize = aPreferredViaHoleSize;
    }

    void CopyFrom( const ICopyable& aSource ) override
    {
        const auto& source = dynamic_cast<const DRC_RE_VIA_STYLE_CONSTRAINT_DATA&>( aSource );

        DRC_RE_BASE_CONSTRAINT_DATA::CopyFrom( source );

        m_minViaDiameter = source.m_minViaDiameter;
        m_maxViaDiameter = source.m_maxViaDiameter;
        m_preferredViaDiameter = source.m_preferredViaDiameter;
        m_minViaHoleSize = source.m_minViaHoleSize;
        m_maxViaHoleSize = source.m_maxViaHoleSize;
        m_preferredViaHoleSize = source.m_preferredViaHoleSize;
    }

private:
    double m_minViaDiameter{ 0 };
    double m_preferredViaDiameter{ 0 };
    double m_maxViaDiameter{ 0 };
    double m_minViaHoleSize{ 0 };
    double m_preferredViaHoleSize{ 0 };
    double m_maxViaHoleSize{ 0 };
};

#endif // DRC_RE_VIA_STYLE_CONSTRAINT_DATA_H_