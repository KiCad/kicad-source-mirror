/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
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

#pragma once

#include <stroke_params.h>
#include <stroke_params_lexer.h>

class STROKE_PARAMS_PARSER : public STROKE_PARAMS_LEXER
{
public:
    STROKE_PARAMS_PARSER( LINE_READER* aReader, int iuPerMM ) :
            STROKE_PARAMS_LEXER( aReader ), m_iuPerMM( iuPerMM )
    {
    }

    void ParseStroke( STROKE_PARAMS& aStroke );

private:
    int    parseInt( const char* aText );
    double parseDouble( const char* aText );

private:
    int m_iuPerMM;
};