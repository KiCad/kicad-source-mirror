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

#ifndef DRC_RE_NUMERIC_INPUT_CONSTRAINT_DATA_H_
#define DRC_RE_NUMERIC_INPUT_CONSTRAINT_DATA_H_

#include "drc_re_base_constraint_data.h"


class DRC_RE_NUMERIC_INPUT_CONSTRAINT_DATA : public DRC_RE_BASE_CONSTRAINT_DATA
{
public:
    DRC_RE_NUMERIC_INPUT_CONSTRAINT_DATA() = default;

    explicit DRC_RE_NUMERIC_INPUT_CONSTRAINT_DATA( const DRC_RE_BASE_CONSTRAINT_DATA& aBaseData ) :
            DRC_RE_BASE_CONSTRAINT_DATA( aBaseData )
    {
    }

    explicit DRC_RE_NUMERIC_INPUT_CONSTRAINT_DATA( int aId, int aParentId,
                                                   double aNumericInputValue, const wxString& aRuleName ) :
            DRC_RE_BASE_CONSTRAINT_DATA( aId, aParentId, aRuleName ),
            m_numericInputValue( aNumericInputValue )
    {
    }

    virtual ~DRC_RE_NUMERIC_INPUT_CONSTRAINT_DATA() = default;

    BITMAPS GetOverlayBitmap() const override { return BITMAPS::constraint_minimum_track_width; }

    std::vector<DRC_RE_FIELD_POSITION> GetFieldPositions() const override
    {
        return { { 20 + DRC_RE_OVERLAY_XO, 40 + DRC_RE_OVERLAY_XO, 20 + DRC_RE_OVERLAY_YO, 1 } };
    }

    std::vector<wxString> GetConstraintClauses( const RULE_GENERATION_CONTEXT& aContext ) const override
    {
        wxString code = GetConstraintCode();
        wxString valueStr;

        if( code == "via_count" || code == "min_resolved_spokes" )
            valueStr = wxString::Format( "%d", (int) m_numericInputValue );
        else
            valueStr = formatDouble( m_numericInputValue );

        if( code == "via_count" )
        {
            return { wxString::Format( "(constraint %s (max %s))", code, valueStr ) };
        }
        else if( code == "min_resolved_spokes" )
        {
            return { wxString::Format( "(constraint %s %s)", code, valueStr ) };
        }
        else if( code == "track_angle" )
        {
            return { wxString::Format( "(constraint %s (min %sdeg))", code, valueStr ) };
        }
        else
        {
            return { wxString::Format( "(constraint %s (min %smm))", code, valueStr ) };
        }
    }

    wxString GenerateRule( const RULE_GENERATION_CONTEXT& aContext ) override
    {
        return buildRule( aContext, GetConstraintClauses( aContext ) );
    }

    double GetNumericInputValue() { return m_numericInputValue; }

    void SetNumericInputValue( double aNumericInput ) { m_numericInputValue = aNumericInput; }

    virtual bool IsIntegerOnly() const { return false; }

    VALIDATION_RESULT Validate() const override
    {
        VALIDATION_RESULT result;

        if( m_numericInputValue <= 0 )
            result.AddError( _( "Numeric input value must be greater than 0" ));

        return result;
    }

    void CopyFrom( const ICopyable& aSource ) override
    {
        const auto& source = dynamic_cast<const DRC_RE_NUMERIC_INPUT_CONSTRAINT_DATA&>( aSource );

        DRC_RE_BASE_CONSTRAINT_DATA::CopyFrom( source );

        m_numericInputValue = source.m_numericInputValue;
    }

private:
    double m_numericInputValue{ 0 };
};

#endif // DRC_RE_NUMERIC_INPUT_CONSTRAINT_DATA_H_
