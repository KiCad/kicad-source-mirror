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

#ifndef DRC_RE_MINIMUM_TEXT_HEIGHT_THICKNESS_CONSTRAINT_DATA_H_
#define DRC_RE_MINIMUM_TEXT_HEIGHT_THICKNESS_CONSTRAINT_DATA_H_

#include "drc_re_base_constraint_data.h"


class DRC_RE_MINIMUM_TEXT_HEIGHT_THICKNESS_CONSTRAINT_DATA : public DRC_RE_BASE_CONSTRAINT_DATA
{
public:
    DRC_RE_MINIMUM_TEXT_HEIGHT_THICKNESS_CONSTRAINT_DATA() = default;

    explicit DRC_RE_MINIMUM_TEXT_HEIGHT_THICKNESS_CONSTRAINT_DATA(
            const DRC_RE_BASE_CONSTRAINT_DATA& aBaseData ) :
            DRC_RE_BASE_CONSTRAINT_DATA( aBaseData )
    {
    }

    explicit DRC_RE_MINIMUM_TEXT_HEIGHT_THICKNESS_CONSTRAINT_DATA( int aId, int aParentId,
                                                                   wxString aRuleName,
                                                                   double   aMinTextHeight,
                                                                   double   aMinTextThickness ) :
            DRC_RE_BASE_CONSTRAINT_DATA( aId, aParentId, aRuleName ),
            m_minTextHeight( aMinTextHeight ), m_minTextThickness( aMinTextThickness )
    {
    }

    virtual ~DRC_RE_MINIMUM_TEXT_HEIGHT_THICKNESS_CONSTRAINT_DATA() = default;

    double GetMinTextHeight() { return m_minTextHeight; }

    void SetMinTextHeight( double aMinTextHeight ) { m_minTextHeight = aMinTextHeight; }

    double GetMinTextThickness() { return m_minTextThickness; }

    void SetMinTextThickness( double aMinTextThickness ) { m_minTextThickness = aMinTextThickness; }

    VALIDATION_RESULT Validate() const override
    {
        VALIDATION_RESULT result;

        if( m_minTextHeight <= 0 )
            result.AddError( "Minimum Text Height must be greater than 0" );

        if( m_minTextThickness <= 0 )
            result.AddError( "Minimum Text Thickness must be greater than 0" );

        return result;
    }

    void CopyFrom( const ICopyable& aSource ) override
    {
        const auto& source =
                dynamic_cast<const DRC_RE_MINIMUM_TEXT_HEIGHT_THICKNESS_CONSTRAINT_DATA&>(
                        aSource );

        DRC_RE_BASE_CONSTRAINT_DATA::CopyFrom( source );

        m_minTextHeight = source.m_minTextHeight;
        m_minTextThickness = source.m_minTextThickness;
    }

private:
    double m_minTextHeight{ 0 };
    double m_minTextThickness{ 0 };
};

#endif // DRC_RE_MINIMUM_TEXT_HEIGHT_THICKNESS_CONSTRAINT_DATA_H_