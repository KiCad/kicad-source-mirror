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

#ifndef DRC_RE_ROUTING_WIDTH_CONSTRAINT_DATA_H_
#define DRC_RE_ROUTING_WIDTH_CONSTRAINT_DATA_H_

#include "drc_re_base_constraint_data.h"
#include "drc_re_overlay_types.h"


class DRC_RE_ROUTING_WIDTH_CONSTRAINT_DATA : public DRC_RE_BASE_CONSTRAINT_DATA
{
public:
    DRC_RE_ROUTING_WIDTH_CONSTRAINT_DATA() = default;

    explicit DRC_RE_ROUTING_WIDTH_CONSTRAINT_DATA( const DRC_RE_BASE_CONSTRAINT_DATA& aBaseData ) :
            DRC_RE_BASE_CONSTRAINT_DATA( aBaseData )
    {
    }

    explicit DRC_RE_ROUTING_WIDTH_CONSTRAINT_DATA( int aId, int aParentId, const wxString& aRuleName, double aOptWidth,
                                                   double aWidthTolerance ) :
            DRC_RE_BASE_CONSTRAINT_DATA( aId, aParentId, aRuleName ),
            m_optWidth( aOptWidth ),
            m_widthTolerance( aWidthTolerance )
    {
    }

    virtual ~DRC_RE_ROUTING_WIDTH_CONSTRAINT_DATA() = default;

    BITMAPS GetOverlayBitmap() const override { return BITMAPS::constraint_routing_width; }

    std::vector<DRC_RE_FIELD_POSITION> GetFieldPositions() const override
    {
        // Positions measured from constraint_routing_width.png bitmap
        // Format: { xStart, xEnd, yTop, tabOrder }
        return {
            { 30 + DRC_RE_OVERLAY_XO, 70 + DRC_RE_OVERLAY_XO, 15 + DRC_RE_OVERLAY_YO, 1, wxS( "mm" ), LABEL_POSITION::RIGHT }, // opt_width
            { 110 + DRC_RE_OVERLAY_XO, 150 + DRC_RE_OVERLAY_XO, 15 + DRC_RE_OVERLAY_YO, 2, wxS( "mm" ), LABEL_POSITION::RIGHT }, // width_tolerance (±)
        };
    }


    VALIDATION_RESULT Validate() const override
    {
        VALIDATION_RESULT result;

        if( m_optWidth <= 0 )
            result.AddError( _( "Optimum Width must be greater than 0" ) );

        if( m_widthTolerance < 0 )
            result.AddError( _( "Width Tolerance must be greater than or equal to 0" ) );

        if( m_widthTolerance >= m_optWidth )
            result.AddError( _( "Width Tolerance must be less than Optimum Width" ) );

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
            code = wxS( "track_width" );

        double minWidth = m_optWidth - m_widthTolerance;
        double maxWidth = m_optWidth + m_widthTolerance;

        wxString clause =
                wxString::Format( wxS( "(constraint %s (min %s) (opt %s) (max %s))" ), code, formatDistance( minWidth ),
                                  formatDistance( m_optWidth ), formatDistance( maxWidth ) );

        return { clause };
    }

    wxString GenerateRule( const RULE_GENERATION_CONTEXT& aContext ) override
    {
        return buildRule( aContext, GetConstraintClauses( aContext ) );
    }

    double GetOptWidth() { return m_optWidth; }
    void   SetOptWidth( double aOptWidth ) { m_optWidth = aOptWidth; }

    double GetWidthTolerance() { return m_widthTolerance; }
    void   SetWidthTolerance( double aTolerance ) { m_widthTolerance = aTolerance; }

    void CopyFrom( const ICopyable& aSource ) override
    {
        const auto& source = dynamic_cast<const DRC_RE_ROUTING_WIDTH_CONSTRAINT_DATA&>( aSource );

        DRC_RE_BASE_CONSTRAINT_DATA::CopyFrom( source );

        m_optWidth = source.m_optWidth;
        m_widthTolerance = source.m_widthTolerance;
    }

private:
    double m_optWidth{ 0 };
    double m_widthTolerance{ 0 };
};

#endif // DRC_RE_ROUTING_WIDTH_CONSTRAINT_DATA_H_
