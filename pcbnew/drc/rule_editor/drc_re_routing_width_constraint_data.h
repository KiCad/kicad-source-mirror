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


class DRC_RE_ROUTING_WIDTH_CONSTRAINT_DATA : public DRC_RE_BASE_CONSTRAINT_DATA
{
public:
    DRC_RE_ROUTING_WIDTH_CONSTRAINT_DATA() = default;

    explicit DRC_RE_ROUTING_WIDTH_CONSTRAINT_DATA( const DRC_RE_BASE_CONSTRAINT_DATA& aBaseData ) :
            DRC_RE_BASE_CONSTRAINT_DATA( aBaseData )
    {
    }

    explicit DRC_RE_ROUTING_WIDTH_CONSTRAINT_DATA( int aId, int aParentId, wxString aRuleName,
                                                   double aMinRoutingWidth,
                                                   double aPreferredRoutingWidth,
                                                   double aMaxRoutingWidth ) :
            DRC_RE_BASE_CONSTRAINT_DATA( aId, aParentId, aRuleName ),
            m_minRoutingWidth( aMinRoutingWidth ),
            m_preferredRoutingWidth( aPreferredRoutingWidth ), m_maxRoutingWidth( aMaxRoutingWidth )
    {
    }

    virtual ~DRC_RE_ROUTING_WIDTH_CONSTRAINT_DATA() = default;

    VALIDATION_RESULT Validate() const override
    {
        VALIDATION_RESULT result;

        // Validate routing width values are positive
        if( m_minRoutingWidth <= 0 )
            result.AddError( "Minimum Routing Width must be greater than 0" );

        if( m_preferredRoutingWidth <= 0 )
            result.AddError( "Preferred Routing Width must be greater than 0" );

        if( m_maxRoutingWidth <= 0 )
            result.AddError( "Maximum Routing Width must be greater than 0" );

        // Validate min <= preferred <= max
        if( m_minRoutingWidth > m_preferredRoutingWidth )
            result.AddError( "Minimum Routing Width cannot be greater than Preferred Routing Width" );

        if( m_preferredRoutingWidth > m_maxRoutingWidth )
            result.AddError( "Preferred Routing Width cannot be greater than Maximum Routing Width" );

        if( m_minRoutingWidth > m_maxRoutingWidth )
            result.AddError( "Minimum Routing Width cannot be greater than Maximum Routing Width" );

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
            code = wxS( "track_width" );

        wxString clause = wxString::Format(
                wxS( "(constraint %s (min %s) (opt %s) (max %s))" ),
                code,
                formatDistance( m_minRoutingWidth ),
                formatDistance( m_preferredRoutingWidth ),
                formatDistance( m_maxRoutingWidth ) );

        return buildRule( aContext, { clause } );
    }

    double GetMinRoutingWidth() { return m_minRoutingWidth; }

    void SetMinRoutingWidth( double aMinRoutingWidth ) { m_minRoutingWidth = aMinRoutingWidth; }

    double GetPreferredRoutingWidth() { return m_preferredRoutingWidth; }

    void SetPreferredRoutingWidth( double aPreferredRoutingWidth )
    {
        m_preferredRoutingWidth = aPreferredRoutingWidth;
    }

    double GetMaxRoutingWidth() { return m_maxRoutingWidth; }

    void SetMaxRoutingWidth( double aMaxRoutingWidth ) { m_maxRoutingWidth = aMaxRoutingWidth; }

    void CopyFrom( const ICopyable& aSource ) override
    {
        const auto& source = dynamic_cast<const DRC_RE_ROUTING_WIDTH_CONSTRAINT_DATA&>( aSource );

        DRC_RE_BASE_CONSTRAINT_DATA::CopyFrom( source );

        m_minRoutingWidth = source.m_minRoutingWidth;
        m_preferredRoutingWidth = source.m_preferredRoutingWidth;
        m_maxRoutingWidth = source.m_maxRoutingWidth;
    }

private:
    double m_minRoutingWidth{ 0 };
    double m_preferredRoutingWidth{ 0 };
    double m_maxRoutingWidth{ 0 };
};

#endif // DRC_RE_ROUTING_WIDTH_CONSTRAINT_DATA_H_