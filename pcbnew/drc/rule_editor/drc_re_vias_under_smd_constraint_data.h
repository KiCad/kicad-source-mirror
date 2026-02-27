/*                                                                                                                    
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2026 KiCad Developers, see AUTHORS.txt for contributors.
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

#ifndef DRC_RE_VIAS_UNDER_SMD_CONSTRAINT_DATA_H_
#define DRC_RE_VIAS_UNDER_SMD_CONSTRAINT_DATA_H_

#include "drc_re_base_constraint_data.h"
#include <drc/drc_rule.h>


class DRC_RE_VIAS_UNDER_SMD_CONSTRAINT_DATA : public DRC_RE_BASE_CONSTRAINT_DATA
{
public:
    DRC_RE_VIAS_UNDER_SMD_CONSTRAINT_DATA() = default;

    explicit DRC_RE_VIAS_UNDER_SMD_CONSTRAINT_DATA( const DRC_RE_BASE_CONSTRAINT_DATA& aBaseData ) :
            DRC_RE_BASE_CONSTRAINT_DATA( aBaseData )
    {
    }

    explicit DRC_RE_VIAS_UNDER_SMD_CONSTRAINT_DATA( int aId, int aParentId, const wxString& aRuleName,
                                                    bool aDisallowThroughVias = false, bool aDisallowMicroVias = false,
                                                    bool aDisallowBlindVias = false,
                                                    bool aDisallowBuriedVias = false ) :
            DRC_RE_BASE_CONSTRAINT_DATA( aId, aParentId, aRuleName ),
            m_disallowThroughVias( aDisallowThroughVias ),
            m_disallowMicroVias( aDisallowMicroVias ),
            m_disallowBlindVias( aDisallowBlindVias ),
            m_disallowBuriedVias( aDisallowBuriedVias )
    {
    }

    virtual ~DRC_RE_VIAS_UNDER_SMD_CONSTRAINT_DATA() = default;

    BITMAPS GetOverlayBitmap() const override { return BITMAPS::constraint_vias_under_smd; }

    std::vector<DRC_RE_FIELD_POSITION> GetFieldPositions() const override
    {
        return {
            { 400 + DRC_RE_OVERLAY_XO, 414 + DRC_RE_OVERLAY_XO, 40 + DRC_RE_OVERLAY_YO, 1, wxS( "Disallow Through Via" ), LABEL_POSITION::RIGHT },
            { 400 + DRC_RE_OVERLAY_XO, 414 + DRC_RE_OVERLAY_XO, 80 + DRC_RE_OVERLAY_YO, 2, wxS( "Disallow Micro Via" ), LABEL_POSITION::RIGHT },
            { 400 + DRC_RE_OVERLAY_XO, 414 + DRC_RE_OVERLAY_XO, 120 + DRC_RE_OVERLAY_YO, 3, wxS( "Disallow Blind Via" ), LABEL_POSITION::RIGHT },
            { 400 + DRC_RE_OVERLAY_XO, 414 + DRC_RE_OVERLAY_XO, 160 + DRC_RE_OVERLAY_YO, 4, wxS( "Disallow Buried Via" ), LABEL_POSITION::RIGHT },
        };
    }

    VALIDATION_RESULT Validate() const override
    {
        VALIDATION_RESULT result;

        if( !m_disallowThroughVias && !m_disallowMicroVias && !m_disallowBlindVias && !m_disallowBuriedVias )
        {
            result.AddError( _( "At least one via type must be selected" ) );
        }

        return result;
    }

    std::vector<wxString> GetConstraintClauses( const RULE_GENERATION_CONTEXT& aContext ) const override
    {
        if( !m_disallowThroughVias && !m_disallowMicroVias && !m_disallowBlindVias && !m_disallowBuriedVias )
            return {};

        if( m_disallowThroughVias && m_disallowMicroVias && m_disallowBlindVias && m_disallowBuriedVias )
        {
            return { wxS( "(constraint disallow via)" ) };
        }

        wxString items;

        if( m_disallowThroughVias )
            items += wxS( " through_via" );
        if( m_disallowMicroVias )
            items += wxS( " micro_via" );
        if( m_disallowBlindVias )
            items += wxS( " blind_via" );
        if( m_disallowBuriedVias )
            items += wxS( " buried_via" );

        items.Trim( false );
        return { wxString::Format( wxS( "(constraint disallow %s)" ), items ) };
    }

    wxString GenerateRule( const RULE_GENERATION_CONTEXT& aContext ) override
    {
        return buildRule( aContext, GetConstraintClauses( aContext ) );
    }

    bool GetDisallowThroughVias() const { return m_disallowThroughVias; }
    void SetDisallowThroughVias( bool aValue ) { m_disallowThroughVias = aValue; }

    bool GetDisallowMicroVias() const { return m_disallowMicroVias; }
    void SetDisallowMicroVias( bool aValue ) { m_disallowMicroVias = aValue; }

    bool GetDisallowBlindVias() const { return m_disallowBlindVias; }
    void SetDisallowBlindVias( bool aValue ) { m_disallowBlindVias = aValue; }

    bool GetDisallowBuriedVias() const { return m_disallowBuriedVias; }
    void SetDisallowBuriedVias( bool aValue ) { m_disallowBuriedVias = aValue; }

    void CopyFrom( const ICopyable& aSource ) override
    {
        const auto& source = dynamic_cast<const DRC_RE_VIAS_UNDER_SMD_CONSTRAINT_DATA&>( aSource );

        DRC_RE_BASE_CONSTRAINT_DATA::CopyFrom( source );

        m_disallowThroughVias = source.m_disallowThroughVias;
        m_disallowMicroVias = source.m_disallowMicroVias;
        m_disallowBlindVias = source.m_disallowBlindVias;
        m_disallowBuriedVias = source.m_disallowBuriedVias;
    }

private:
    bool m_disallowThroughVias{ true };
    bool m_disallowMicroVias{ true };
    bool m_disallowBlindVias{ true };
    bool m_disallowBuriedVias{ true };
};

#endif // DRC_RE_VIAS_UNDER_SMD_CONSTRAINT_DATA_H_
