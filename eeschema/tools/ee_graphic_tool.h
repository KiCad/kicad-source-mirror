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

#include <tools/sch_tool_base.h>
#include <sch_base_frame.h>
#include <sch_shape.h>

class SCH_EDIT_FRAME;
class SYMBOL_EDIT_FRAME;


/**
 * Tool responsible for drawing graphical shapes (rectangles, circles, arcs,
 * beziers, text boxes, etc.) in both the schematic and symbol editors.
 */
class EE_GRAPHIC_TOOL : public SCH_TOOL_BASE<SCH_BASE_FRAME>
{
public:
    enum class MODE
    {
        NONE,
        ARC,
    };

    EE_GRAPHIC_TOOL();

    bool Init() override;

    int DrawShape( const TOOL_EVENT& aEvent );
    int DrawArc( const TOOL_EVENT& aEvent );

private:
    void setTransitions() override;

    ///< The layer to use for new shapes in the current editor.
    SCH_LAYER_ID getShapeLayer() const;

    ///< Commit a completed item.
    void commitItem( SCH_COMMIT& aCommit, std::unique_ptr<SCH_ITEM> aItem, const wxString& aDescription );

    FILL_T        m_lastFillStyle;
    COLOR4D       m_lastFillColor;
    STROKE_PARAMS m_lastStroke;

    FILL_T        m_lastTextboxFillStyle;
    COLOR4D       m_lastTextboxFillColor;
    STROKE_PARAMS m_lastTextboxStroke;

    bool              m_lastTextBold;
    bool              m_lastTextItalic;
    EDA_ANGLE         m_lastTextboxAngle;
    GR_TEXT_H_ALIGN_T m_lastTextboxHJustify;
    GR_TEXT_V_ALIGN_T m_lastTextboxVJustify;

    MODE m_mode;

    // Re-entrancy guards
    bool m_inDrawingTool;
};
