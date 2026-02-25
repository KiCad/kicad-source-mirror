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

#ifndef DRC_RE_ALLOWED_ORIENTATION_CONSTRAINT_DATA_H_
#define DRC_RE_ALLOWED_ORIENTATION_CONSTRAINT_DATA_H_

#include "drc_re_base_constraint_data.h"


class DRC_RE_ALLOWED_ORIENTATION_CONSTRAINT_DATA : public DRC_RE_BASE_CONSTRAINT_DATA
{
public:
    DRC_RE_ALLOWED_ORIENTATION_CONSTRAINT_DATA() = default;

    explicit DRC_RE_ALLOWED_ORIENTATION_CONSTRAINT_DATA(
            const DRC_RE_BASE_CONSTRAINT_DATA& aBaseData ) :
            DRC_RE_BASE_CONSTRAINT_DATA( aBaseData )
    {
    }

    explicit DRC_RE_ALLOWED_ORIENTATION_CONSTRAINT_DATA(
            int aId, int aParentId, bool aAllowZeroDegreess, bool aAllowNinetyDegreess,
            bool aAllowOneEightyDegreess, bool aAllowTwoSeventyDegreess, bool aAllowAllDegreess,
            const wxString& aRuleName ) :
            DRC_RE_BASE_CONSTRAINT_DATA( aId, aParentId, aRuleName ),
            m_allowZeroDegreess( aAllowZeroDegreess ), m_allowNinetyDegrees( aAllowNinetyDegreess ),
            m_allowOneEightyDegrees( aAllowOneEightyDegreess ),
            m_allowTwoSeventyDegrees( aAllowTwoSeventyDegreess ),
            m_allowAllDegrees( aAllowAllDegreess )
    {
    }

    virtual ~DRC_RE_ALLOWED_ORIENTATION_CONSTRAINT_DATA() = default;

    BITMAPS GetOverlayBitmap() const override { return BITMAPS::constraint_allowed_orientation; }

    std::vector<DRC_RE_FIELD_POSITION> GetFieldPositions() const override
    {
        // Positions measured from constraint_allowed_orientation.png (~280x180)
        // Format: { xStart, xEnd, yTop, tabOrder }
        // Checkboxes stacked vertically on the right side
        return {
            { 400 + DRC_RE_OVERLAY_XO, 414 + DRC_RE_OVERLAY_XO, 40 + DRC_RE_OVERLAY_YO, 1, _( "Allow 0°" ), LABEL_POSITION::RIGHT },      // 0 degrees checkbox
            { 400 + DRC_RE_OVERLAY_XO, 414 + DRC_RE_OVERLAY_XO, 80 + DRC_RE_OVERLAY_YO, 2, _( "Allow 90°" ), LABEL_POSITION::RIGHT },     // 90 degrees checkbox
            { 400 + DRC_RE_OVERLAY_XO, 414 + DRC_RE_OVERLAY_XO, 120 + DRC_RE_OVERLAY_YO, 3, _( "Allow 180°" ), LABEL_POSITION::RIGHT },   // 180 degrees checkbox
            { 400 + DRC_RE_OVERLAY_XO, 414 + DRC_RE_OVERLAY_XO, 160 + DRC_RE_OVERLAY_YO, 4, _( "Allow 270°" ), LABEL_POSITION::RIGHT },   // 270 degrees checkbox
            { 400 + DRC_RE_OVERLAY_XO, 414 + DRC_RE_OVERLAY_XO, 200 + DRC_RE_OVERLAY_YO, 5, _( "Allow All" ), LABEL_POSITION::RIGHT },    // all degrees checkbox
        };
    }

    std::vector<wxString> GetConstraintClauses( const RULE_GENERATION_CONTEXT& aContext ) const override
    {
        if( m_allowAllDegrees )
            return { wxS( "(constraint assertion \"A.Orientation == A.Orientation\")" ) };

        wxArrayString terms;

        if( m_allowZeroDegreess )
            terms.Add( wxS( "A.Orientation == 0 deg" ) );

        if( m_allowNinetyDegrees )
            terms.Add( wxS( "A.Orientation == 90 deg" ) );

        if( m_allowOneEightyDegrees )
            terms.Add( wxS( "A.Orientation == 180 deg" ) );

        if( m_allowTwoSeventyDegrees )
            terms.Add( wxS( "A.Orientation == 270 deg" ) );

        if( terms.IsEmpty() )
            return {};

        wxString expr = wxJoin( terms, '|' );
        expr.Replace( wxS( "|" ), wxS( " || " ) );

        wxString clause = wxString::Format( wxS( "(constraint assertion \"%s\")" ), expr );

        return { clause };
    }

    wxString GenerateRule( const RULE_GENERATION_CONTEXT& aContext ) override
    {
        return buildRule( aContext, GetConstraintClauses( aContext ) );
    }

    VALIDATION_RESULT Validate() const override
    {
        VALIDATION_RESULT result;

        // At least one orientation must be selected
        if( !m_allowZeroDegreess && !m_allowNinetyDegrees && !m_allowOneEightyDegrees
            && !m_allowTwoSeventyDegrees && !m_allowAllDegrees )
        {
            result.AddError( _( "At least one orientation must be selected" ) );
        }

        return result;
    }

    bool GetIsZeroDegreesAllowed() { return m_allowZeroDegreess; }

    void SetIsZeroDegreesAllowed( bool aAllowZeroDegreess )
    {
        m_allowZeroDegreess = aAllowZeroDegreess;
    }

    bool GetIsNinetyDegreesAllowed() { return m_allowNinetyDegrees; }

    void SetIsNinetyDegreesAllowed( bool aAllowNinetyDegrees )
    {
        m_allowNinetyDegrees = aAllowNinetyDegrees  ;
    }

    bool GetIsOneEightyDegreesAllowed() { return m_allowOneEightyDegrees; }

    void SetIsOneEightyDegreesAllowed( bool aAllowOneEightyDegrees )
    {
        m_allowOneEightyDegrees = aAllowOneEightyDegrees;
    }

    bool GetIsTwoSeventyDegreesAllowed() { return m_allowTwoSeventyDegrees; }

    void SetIsTwoSeventyDegreesAllowed( bool aAllowTwoSeventyDegrees )
    {
        m_allowTwoSeventyDegrees = aAllowTwoSeventyDegrees;
    }

    bool GetIsAllDegreesAllowed() { return m_allowAllDegrees; }

    void SetIsAllDegreesAllowed( bool aAllowAllDegrees )
    {
        m_allowAllDegrees = aAllowAllDegrees;
    }

    void CopyFrom( const ICopyable& aSource ) override
    {
        const auto& source =
                dynamic_cast<const DRC_RE_ALLOWED_ORIENTATION_CONSTRAINT_DATA&>( aSource );

        DRC_RE_BASE_CONSTRAINT_DATA::CopyFrom( source );

        m_allowZeroDegreess = source.m_allowZeroDegreess;
        m_allowNinetyDegrees = source.m_allowNinetyDegrees;
        m_allowOneEightyDegrees = source.m_allowOneEightyDegrees;
        m_allowTwoSeventyDegrees = source.m_allowTwoSeventyDegrees;
        m_allowAllDegrees = source.m_allowAllDegrees;
    }

private:
    bool m_allowZeroDegreess{ false };
    bool m_allowNinetyDegrees{ false };
    bool m_allowOneEightyDegrees{ false };
    bool m_allowTwoSeventyDegrees{ false };
    bool m_allowAllDegrees{ false };
};

#endif // DRC_RE_ALLOWED_ORIENTATION_CONSTRAINT_DATA_H_
