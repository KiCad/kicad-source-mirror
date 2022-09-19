/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2021 KiCad Developers, see AUTHORS.txt for contributors.
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

#ifndef STROKE_PARAMS_H
#define STROKE_PARAMS_H

#include <map>
#include <bitmaps.h>
#include <units_provider.h>
#include <gal/color4d.h>
#include <wx/translation.h>
#include <geometry/shape.h>
#include <stroke_params_lexer.h>

class STROKE_PARAMS_LEXER;
class MSG_PANEL_ITEM;

namespace KIGFX
{
class RENDER_SETTINGS;
}


/**
 * Dashed line types.
 */
enum class PLOT_DASH_TYPE
{
    DEFAULT    = -1,
    SOLID      = 0,
    FIRST_TYPE = SOLID,
    DASH,
    DOT,
    DASHDOT,
    DASHDOTDOT,
    LAST_TYPE = DASHDOTDOT
};


struct lineTypeStruct
{
    wxString      name;
    const BITMAPS bitmap;
};


/*
 * Conversion map between PLOT_DASH_TYPE values and style names displayed
 */
const std::map<PLOT_DASH_TYPE, struct lineTypeStruct> lineTypeNames =
{
    { PLOT_DASH_TYPE::SOLID,      { _( "Solid" ),        BITMAPS::stroke_solid      } },
    { PLOT_DASH_TYPE::DASH,       { _( "Dashed" ),       BITMAPS::stroke_dash       } },
    { PLOT_DASH_TYPE::DOT,        { _( "Dotted" ),       BITMAPS::stroke_dot        } },
    { PLOT_DASH_TYPE::DASHDOT,    { _( "Dash-Dot" ),     BITMAPS::stroke_dashdot    } },
    { PLOT_DASH_TYPE::DASHDOTDOT, { _( "Dash-Dot-Dot" ), BITMAPS::stroke_dashdotdot } }
};


#define DEFAULT_STYLE _( "Default" )
#define INDETERMINATE_STYLE _( "Leave unchanged" )


/**
 * Simple container to manage line stroke parameters.
 */
class STROKE_PARAMS
{
public:
    STROKE_PARAMS( int aWidth = 0, PLOT_DASH_TYPE aPlotStyle = PLOT_DASH_TYPE::DEFAULT,
                   const KIGFX::COLOR4D& aColor = KIGFX::COLOR4D::UNSPECIFIED ) :
            m_width( aWidth ),
            m_plotstyle( aPlotStyle ),
            m_color( aColor )
    {
    }

    int GetWidth() const { return m_width; }
    void SetWidth( int aWidth ) { m_width = aWidth; }

    PLOT_DASH_TYPE GetPlotStyle() const { return m_plotstyle; }
    void SetPlotStyle( PLOT_DASH_TYPE aPlotStyle ) { m_plotstyle = aPlotStyle; }

    KIGFX::COLOR4D GetColor() const { return m_color; }
    void SetColor( const KIGFX::COLOR4D& aColor ) { m_color = aColor; }

    bool operator!=( const STROKE_PARAMS& aOther )
    {
        return m_width != aOther.m_width
                || m_plotstyle != aOther.m_plotstyle
                || m_color != aOther.m_color;
    }

    void Format( OUTPUTFORMATTER* out, const EDA_IU_SCALE& aIuScale, int nestLevel ) const;

    void GetMsgPanelInfo( UNITS_PROVIDER* aUnitsProvider, std::vector<MSG_PANEL_ITEM>& aList,
                          bool aIncludeStyle = true, bool aIncludeWidth = true );

    // Helper functions

    static wxString GetLineStyleToken( PLOT_DASH_TYPE aStyle );

    static void Stroke( const SHAPE* aShape, PLOT_DASH_TYPE aLineStyle, int aWidth,
                        const KIGFX::RENDER_SETTINGS* aRenderSettings,
                        std::function<void( const VECTOR2I& a, const VECTOR2I& b )> aStroker );

private:
    int            m_width;
    PLOT_DASH_TYPE m_plotstyle;
    KIGFX::COLOR4D m_color;
};


class STROKE_PARAMS_PARSER : public STROKE_PARAMS_LEXER
{
public:
    STROKE_PARAMS_PARSER( LINE_READER* aReader, int iuPerMM ) :
            STROKE_PARAMS_LEXER( aReader ),
            m_iuPerMM( iuPerMM )
    {
    }

    void ParseStroke( STROKE_PARAMS& aStroke );

private:
    int parseInt( const char* aText );
    double parseDouble( const char* aText );

private:
    int  m_iuPerMM;
};


#endif  // STROKE_PARAMS_H
