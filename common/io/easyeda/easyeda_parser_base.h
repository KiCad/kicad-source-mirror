/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2023 Alex Shvartzkop <dudesuchamazing@gmail.com>
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef EASYEDA_PARSER_BASE_H_
#define EASYEDA_PARSER_BASE_H_

#include <geometry/shape_poly_set.h>
#include <math/vector2d.h>
#include <wx/string.h>

class EDA_TEXT;

class EASYEDA_PARSER_BASE
{
public:
    static double Convert( const wxString& aValue );

    double ConvertSize( const wxString& aValue ) { return ScaleSize( Convert( aValue ) ); }

    virtual double ScaleSize( double aValue ) = 0;

    double ScaleSize( const wxString& aValue ) { return ScaleSize( Convert( aValue ) ); }

    template <typename T>
    VECTOR2<T> ScalePos( const VECTOR2<T>& aValue )
    {
        return VECTOR2<T>( ScaleSize( aValue.x ), ScaleSize( aValue.y ) );
    }

    double RelPosX( double aValue );
    double RelPosY( double aValue );

    double RelPosX( const wxString& aValue );
    double RelPosY( const wxString& aValue );

    template <typename T>
    VECTOR2<T> RelPos( const VECTOR2<T>& aVec )
    {
        return ScalePos( aVec - m_relOrigin );
    }

    void TransformTextToBaseline( EDA_TEXT* textItem, const wxString& baselineAlign );

    std::vector<SHAPE_LINE_CHAIN> ParseLineChains( const wxString& aData, int aMaxError, bool aForceClosed );

protected:
    VECTOR2D m_relOrigin;
};


#endif // EASYEDA_PARSER_BASE_H_
