/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2019 CERN
 * Copyright (C) 2019 KiCad Developers, see AUTHORS.txt for contributors.
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

#ifndef SYMBOL_EDITOR_DRAWING_TOOLS_H
#define SYMBOL_EDITOR_DRAWING_TOOLS_H

#include <tools/ee_tool_base.h>


class SYMBOL_EDIT_FRAME;


/**
 * SYMBOL_EDITOR_DRAWING_TOOLS
 *
 * Tool responsible for drawing/placing items (body outlines, pins, etc.)
 */

class SYMBOL_EDITOR_DRAWING_TOOLS : public EE_TOOL_BASE<SYMBOL_EDIT_FRAME>
{
public:
    SYMBOL_EDITOR_DRAWING_TOOLS();
    ~SYMBOL_EDITOR_DRAWING_TOOLS() override { }

    /// @copydoc TOOL_INTERACTIVE::Init()
    bool Init() override;

    int TwoClickPlace( const TOOL_EVENT& aEvent );
    int DrawShape( const TOOL_EVENT& aEvent );
    int PlaceAnchor( const TOOL_EVENT& aEvent );

    int RepeatDrawItem( const TOOL_EVENT& aEvent );

    void SetLastTextAngle( double aAngle ) { m_lastTextAngle = aAngle; }
    double GetLastTextAngle() const { return m_lastTextAngle; }

    void SetDrawSpecificConvert( bool aSpecific ) { m_drawSpecificConvert = aSpecific; }
    bool GetDrawSpecificConvert() const { return m_drawSpecificConvert; }

    void SetDrawSpecificUnit( bool aSpecific ) { m_drawSpecificUnit = aSpecific; }
    bool GetDrawSpecificUnit() const { return m_drawSpecificUnit; }

private:
    void setTransitions() override;

private:
    double    m_lastTextAngle;
    FILL_T    m_lastFillStyle;
    bool      m_drawSpecificConvert;
    bool      m_drawSpecificUnit;
};

#endif /* SYMBOL_EDITOR_DRAWING_TOOLS_H */
