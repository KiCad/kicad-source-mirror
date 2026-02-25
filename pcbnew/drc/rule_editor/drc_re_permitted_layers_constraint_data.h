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

#ifndef DRC_RE_PERMITTED_LAYERS_CONSTRAINT_DATA_H_
#define DRC_RE_PERMITTED_LAYERS_CONSTRAINT_DATA_H_

#include "drc_re_base_constraint_data.h"


class DRC_RE_PERMITTED_LAYERS_CONSTRAINT_DATA : public DRC_RE_BASE_CONSTRAINT_DATA
{
public:
    DRC_RE_PERMITTED_LAYERS_CONSTRAINT_DATA() = default;

    explicit DRC_RE_PERMITTED_LAYERS_CONSTRAINT_DATA(
            const DRC_RE_BASE_CONSTRAINT_DATA& aBaseData ) :
            DRC_RE_BASE_CONSTRAINT_DATA( aBaseData )
    {
    }

    explicit DRC_RE_PERMITTED_LAYERS_CONSTRAINT_DATA( int aId, int aParentId, const wxString& aRuleName,
                                                      bool aTopLayer, bool aBottomLayer ) :
            DRC_RE_BASE_CONSTRAINT_DATA( aId, aParentId, aRuleName ), m_topLayer( aTopLayer ),
            m_bottomLayer( aBottomLayer )
    {
    }

    virtual ~DRC_RE_PERMITTED_LAYERS_CONSTRAINT_DATA() = default;

    BITMAPS GetOverlayBitmap() const override { return BITMAPS::constraint_permitted_layers; }

    std::vector<DRC_RE_FIELD_POSITION> GetFieldPositions() const override
    {
        // Positions measured from constraint_permitted_layers.png (~300x170)
        // Format: { xStart, xEnd, yTop, tabOrder }
        return {
            { 150 + DRC_RE_OVERLAY_XO, 164 + DRC_RE_OVERLAY_XO, 40 + DRC_RE_OVERLAY_YO, 1, _( "Allow top Layer" ), LABEL_POSITION::RIGHT },      // top layer checkbox (upper left)
            { 150 + DRC_RE_OVERLAY_XO, 164 + DRC_RE_OVERLAY_XO, 200 + DRC_RE_OVERLAY_YO, 2, _( "Allow bottom Layer" ), LABEL_POSITION::RIGHT },  // bottom layer checkbox (lower left)
        };
    }

    std::vector<wxString> GetConstraintClauses( const RULE_GENERATION_CONTEXT& aContext ) const override
    {
        wxArrayString terms;

        if( m_topLayer )
            terms.Add( wxS( "A.Layer == 'F.Cu'" ) );

        if( m_bottomLayer )
            terms.Add( wxS( "A.Layer == 'B.Cu'" ) );

        if( terms.IsEmpty() )
            return {};

        wxString expr = wxJoin( terms, '|' );
        expr.Replace( wxS( "|" ), wxS( " || " ) );

        return { wxString::Format( wxS( "(constraint assertion \"%s\")" ), expr ) };
    }

    wxString GenerateRule( const RULE_GENERATION_CONTEXT& aContext ) override
    {
        return buildRule( aContext, GetConstraintClauses( aContext ) );
    }

    VALIDATION_RESULT Validate() const override
    {
        VALIDATION_RESULT result;

        if( !m_topLayer && !m_bottomLayer )
            result.AddError( _( "At least one layer must be selected" ) );

        return result;
    }

    bool GetTopLayerEnabled() { return m_topLayer; }

    void SetTopLayerEnabled( bool aTopLayer ) { m_topLayer = aTopLayer; }

    bool GetBottomLayerEnabled() { return m_bottomLayer; }

    void SetBottomLayerEnabled( bool aBottomLayer ) { m_bottomLayer = aBottomLayer; }

    void CopyFrom( const ICopyable& aSource ) override
    {
        const auto& source =
                dynamic_cast<const DRC_RE_PERMITTED_LAYERS_CONSTRAINT_DATA&>( aSource );

        DRC_RE_BASE_CONSTRAINT_DATA::CopyFrom( source );

        m_topLayer = source.m_topLayer;
        m_bottomLayer = source.m_bottomLayer;
    }

private:
    bool m_topLayer{ false };
    bool m_bottomLayer{ false };
};

#endif // DRC_RE_PERMITTED_LAYERS_CONSTRAINT_DATA_H_
