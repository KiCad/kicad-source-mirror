/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2019-2023 CERN
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
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#ifndef SCH_DRAWING_TOOLS_H
#define SCH_DRAWING_TOOLS_H

#include "sch_sheet_path.h"
#include <tools/sch_tool_base.h>
#include <sch_base_frame.h>
#include <sch_label.h>
#include <status_popup.h>
#include <sync_sheet_pin/dialog_sync_sheet_pins.h>

class SCH_SYMBOL;
class SCH_BUS_WIRE_ENTRY;
class SCH_EDIT_FRAME;
class SCH_SELECTION_TOOL;
class DIALOG_SYNC_SHEET_PINS;


/**
 * Tool responsible for drawing/placing items (symbols, wires, buses, labels, etc.).
 */

class SCH_DRAWING_TOOLS : public SCH_TOOL_BASE<SCH_EDIT_FRAME>
{
public:
    SCH_DRAWING_TOOLS();
    ~SCH_DRAWING_TOOLS() override { }

    /// @copydoc TOOL_INTERACTIVE::Init()
    bool Init() override;

    int PlaceSymbol( const TOOL_EVENT& aEvent );
    int PlaceNextSymbolUnit( const TOOL_EVENT& aEvent );
    int SingleClickPlace( const TOOL_EVENT& aEvent );
    int TwoClickPlace( const TOOL_EVENT& aEvent );
    int ImportSheet( const TOOL_EVENT& aEvent );
    int DrawShape( const TOOL_EVENT& aEvent );
    int DrawRuleArea( const TOOL_EVENT& aEvent );
    int DrawTable( const TOOL_EVENT& aEvent );
    int DrawSheet( const TOOL_EVENT& aEvent );
    int PlaceImage( const TOOL_EVENT& aEvent );
    int ImportGraphics( const TOOL_EVENT& aEvent );
    int SyncSheetsPins( const TOOL_EVENT& aEvent );
    int SyncAllSheetsPins( const TOOL_EVENT& aEvent );
    int AutoPlaceAllSheetPins( const TOOL_EVENT& aEvent );

private:
    SCH_LINE* findWire( const VECTOR2I& aPosition );

    ///< Gets the (global) label name driving this wire, if it is driven by a label
    wxString findWireLabelDriverName( SCH_LINE* aWire );

    SCH_TEXT* createNewText( const VECTOR2I& aPosition );

    bool createNewLabel( const VECTOR2I& aPosition, int aType,
                        std::list<std::unique_ptr<SCH_LABEL_BASE>>& aLabelList );

    SCH_SHEET_PIN* createNewSheetPin( SCH_SHEET* aSheet, const VECTOR2I& aPosition );

    SCH_SHEET_PIN* createNewSheetPinFromLabel( SCH_SHEET* aSheet, const VECTOR2I& aPosition,
                                               SCH_HIERLABEL* aLabel );

    void sizeSheet( SCH_SHEET* aSheet, const VECTOR2I& aPos );

    ///< Set up handlers for various events.
    void setTransitions() override;

    int doSyncSheetsPins( std::list<SCH_SHEET_PATH> aSheets );

    ///< Try finding any hierlabel that does not have a sheet pin associated with it
    SCH_HIERLABEL* importHierLabel( SCH_SHEET* aSheet );

    std::vector<SCH_HIERLABEL*> importHierLabels( SCH_SHEET* aSheet );

    std::vector<PICKED_SYMBOL> m_symbolHistoryList;
    std::vector<PICKED_SYMBOL> m_powerHistoryList;
    std::vector<LIB_ID>        m_designBlockHistoryList;

    LABEL_FLAG_SHAPE           m_lastSheetPinType;
    LABEL_FLAG_SHAPE           m_lastGlobalLabelShape;
    LABEL_FLAG_SHAPE           m_lastNetClassFlagShape;
    SPIN_STYLE                 m_lastTextOrientation;
    bool                       m_lastTextBold;
    bool                       m_lastTextItalic;
    EDA_ANGLE                  m_lastTextAngle;
    EDA_ANGLE                  m_lastTextboxAngle;
    GR_TEXT_H_ALIGN_T          m_lastTextHJustify;
    GR_TEXT_V_ALIGN_T          m_lastTextVJustify;
    GR_TEXT_H_ALIGN_T          m_lastTextboxHJustify;
    GR_TEXT_V_ALIGN_T          m_lastTextboxVJustify;
    FILL_T                     m_lastFillStyle;
    FILL_T                     m_lastTextboxFillStyle;
    COLOR4D                    m_lastFillColor;
    COLOR4D                    m_lastTextboxFillColor;
    STROKE_PARAMS              m_lastStroke;
    STROKE_PARAMS              m_lastTextboxStroke;
    wxString                   m_mruPath;
    bool                       m_lastAutoLabelRotateOnPlacement;
    bool                       m_drawingRuleArea;

    bool                                    m_inDrawingTool; // Re-entrancy guard
    std::unique_ptr<STATUS_TEXT_POPUP>      m_statusPopup;
    std::unique_ptr<DIALOG_SYNC_SHEET_PINS> m_dialogSyncSheetPin;
};

#endif /* SCH_DRAWING_TOOLS_H */
