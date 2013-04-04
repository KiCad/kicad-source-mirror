/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2012 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright (C) 2013 CERN
 * @author Tomasz Wlostowski <tomasz.wlostowski@cern.ch>
 * @author Maciej Suminski <maciej.suminski@cern.ch>
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

#include <painter.h>
#include <gal/stroke_font.h>
#include <newstroke_font.h>

using namespace KiGfx;

RENDER_SETTINGS::RENDER_SETTINGS()
{
    // Set the default initial values
    m_selectionBorderColor = COLOR4D( 1.0, 1.0, 1.0, 1.0 );
    m_netLabelColor = COLOR4D( 1.0, 1.0, 1.0, 0.7 );

    m_highlightFactor   = 0.5;
    m_selectFactor      = 0.5;
    m_layerOpacity      = 0.8;
    m_highlightEnabled  = false;
    m_hiContrastEnabled = false;
    m_hiContrastFactor  = 0.2;
    m_activeLayer       = 0;

    // Store the predefined colors used in KiCad in format used by GAL
    for( int i = 0; i < NBCOLOR; i++ )
    {
        m_legacyColorMap[ColorRefs[i].m_Numcolor] = COLOR4D( (double) ColorRefs[i].m_Red / 255.0,
                                                             (double) ColorRefs[i].m_Green / 255.0,
                                                             (double) ColorRefs[i].m_Blue / 255.0,
                                                             m_layerOpacity );
    }
}


RENDER_SETTINGS::~RENDER_SETTINGS()
{
}


void RENDER_SETTINGS::Update()
{
    m_hiContrastColor = COLOR4D( m_hiContrastFactor, m_hiContrastFactor, m_highlightFactor,
                                 m_layerOpacity );
}


PAINTER::PAINTER( GAL* aGal ) :
    m_gal( aGal ), m_settings( NULL )
{
    m_stroke_font = new STROKE_FONT( aGal );
    m_stroke_font->LoadNewStrokeFont( newstroke_font, newstroke_font_bufsize );
}


PAINTER::~PAINTER()
{
    delete m_stroke_font;
}
