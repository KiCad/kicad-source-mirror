/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2019-2023 CERN
 * Copyright (C) 2019-2024 KiCad Developers, see AUTHORS.txt for contributors.
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

#pragma once

#include <tool/tool_action.h>
#include <tool/actions.h>

class SCH_SYMBOL;
class TOOL_EVENT;
class TOOL_MANAGER;

/**
 * Gather all the actions that are shared by tools. The instance of SCH_ACTIONS is created
 * inside of ACTION_MANAGER object that registers the actions.
 */
class EE_ACTIONS : public ACTIONS
{
public:
    // Menu bar save curr sheet as command
    static TOOL_ACTION saveCurrSheetCopyAs;

    // Selection Tool
    /// Activation of the selection tool
    static TOOL_ACTION selectionActivate;

    /// Select the junction, wire or bus segment under the cursor.
    static TOOL_ACTION selectNode;

    /// If current selection is a wire or bus, expand to entire connection.
    /// Otherwise, select connection under cursor.
    static TOOL_ACTION selectConnection;

    /// Clears the current selection
    static TOOL_ACTION clearSelection;

    /// Selects an item (specified as the event parameter).
    static TOOL_ACTION addItemToSel;
    static TOOL_ACTION removeItemFromSel;

    /// Selects a list of items (specified as the event parameter)
    static TOOL_ACTION addItemsToSel;
    static TOOL_ACTION removeItemsFromSel;

    /// Runs a selection menu to select from a list of items
    static TOOL_ACTION selectionMenu;

    /// Selection synchronization (PCB -> SCH)
    static TOOL_ACTION syncSelection;

    // Locking
    static TOOL_ACTION toggleLock;
    static TOOL_ACTION lock;
    static TOOL_ACTION unlock;

    // Schematic Tools
    static TOOL_ACTION pickerTool;
    static TOOL_ACTION placeSymbol;
    static TOOL_ACTION placeNextSymbolUnit;
    static TOOL_ACTION placePower;
    static TOOL_ACTION placeDesignBlock;
    static TOOL_ACTION drawWire;
    static TOOL_ACTION drawBus;
    static TOOL_ACTION unfoldBus;
    static TOOL_ACTION placeNoConnect;
    static TOOL_ACTION placeJunction;
    static TOOL_ACTION placeBusWireEntry;
    static TOOL_ACTION placeLabel;
    static TOOL_ACTION placeClassLabel;
    static TOOL_ACTION placeGlobalLabel;
    static TOOL_ACTION placeHierLabel;
    static TOOL_ACTION drawSheet;
    static TOOL_ACTION drawSheetFromFile;
    static TOOL_ACTION drawSheetFromDesignBlock;
    static TOOL_ACTION placeSheetPin;
    static TOOL_ACTION importSheet;
    // Sync sheet pins for selected sheet symbol
    static TOOL_ACTION syncSheetPins;
    // Sync sheet pins for all sheet symbols
    static TOOL_ACTION syncAllSheetsPins;
    static TOOL_ACTION placeSchematicText;
    static TOOL_ACTION drawTextBox;
    static TOOL_ACTION drawTable;
    static TOOL_ACTION drawRectangle;
    static TOOL_ACTION drawCircle;
    static TOOL_ACTION drawArc;
    static TOOL_ACTION drawBezier;
    static TOOL_ACTION drawLines;
    static TOOL_ACTION placeImage;
    static TOOL_ACTION undoLastSegment;
    static TOOL_ACTION switchSegmentPosture;
    static TOOL_ACTION drawRuleArea;
    static TOOL_ACTION deleteLastPoint;
    static TOOL_ACTION closeOutline;

    // Symbol Tools
    static TOOL_ACTION placeSymbolPin;
    static TOOL_ACTION placeSymbolText;
    static TOOL_ACTION drawSymbolTextBox;
    static TOOL_ACTION drawSymbolLines;
    static TOOL_ACTION drawSymbolPolygon;
    static TOOL_ACTION placeSymbolAnchor;

    // Interactive Editing
    static TOOL_ACTION alignToGrid;
    static TOOL_ACTION move;
    static TOOL_ACTION drag;
    static TOOL_ACTION repeatDrawItem;
    static TOOL_ACTION rotateCW;
    static TOOL_ACTION rotateCCW;
    static TOOL_ACTION mirrorV;
    static TOOL_ACTION mirrorH;
    static TOOL_ACTION swap;
    static TOOL_ACTION properties;
    static TOOL_ACTION editReference;
    static TOOL_ACTION editValue;
    static TOOL_ACTION editFootprint;
    static TOOL_ACTION autoplaceFields;
    static TOOL_ACTION toggleDeMorgan;
    static TOOL_ACTION showDeMorganStandard;
    static TOOL_ACTION showDeMorganAlternate;
    static TOOL_ACTION editSymbolUnit;
    static TOOL_ACTION toLabel;
    static TOOL_ACTION toCLabel;
    static TOOL_ACTION toHLabel;
    static TOOL_ACTION toGLabel;
    static TOOL_ACTION toText;
    static TOOL_ACTION toTextBox;
    static TOOL_ACTION breakWire;
    static TOOL_ACTION slice;
    static TOOL_ACTION pointEditorAddCorner;
    static TOOL_ACTION pointEditorRemoveCorner;

    /// Inspection and Editing
    static TOOL_ACTION runERC;
    static TOOL_ACTION annotate;
    static TOOL_ACTION incrementAnnotations;
    static TOOL_ACTION editSymbolFields;
    static TOOL_ACTION editSymbolLibraryLinks;
    static TOOL_ACTION symbolProperties;
    static TOOL_ACTION pinTable;
    static TOOL_ACTION changeSymbols;
    static TOOL_ACTION updateSymbols;
    static TOOL_ACTION changeSymbol;
    static TOOL_ACTION updateSymbol;
    static TOOL_ACTION assignFootprints;
    static TOOL_ACTION assignNetclass;
    static TOOL_ACTION schematicSetup;
    static TOOL_ACTION editPageNumber;
    static TOOL_ACTION checkSymbol;
    static TOOL_ACTION diffSymbol;
    static TOOL_ACTION showBusSyntaxHelp;

    static TOOL_ACTION rescueSymbols;
    static TOOL_ACTION remapSymbols;

    // Suite operations
    static TOOL_ACTION editWithLibEdit;
    static TOOL_ACTION editLibSymbolWithLibEdit;
    static TOOL_ACTION showPcbNew;
    static TOOL_ACTION importFPAssignments;
    static TOOL_ACTION exportNetlist;
    static TOOL_ACTION generateBOM;
    static TOOL_ACTION generateBOMLegacy;
    static TOOL_ACTION generateBOMExternal;
    static TOOL_ACTION addSymbolToSchematic;
    static TOOL_ACTION exportSymbolsToLibrary;
    static TOOL_ACTION exportSymbolsToNewLibrary;

    // Attribute Toggles
    static TOOL_ACTION setExcludeFromBOM;
    static TOOL_ACTION unsetExcludeFromBOM;
    static TOOL_ACTION toggleExcludeFromBOM;
    static TOOL_ACTION setExcludeFromSimulation;
    static TOOL_ACTION unsetExcludeFromSimulation;
    static TOOL_ACTION toggleExcludeFromSimulation;
    static TOOL_ACTION setExcludeFromBoard;
    static TOOL_ACTION unsetExcludeFromBoard;
    static TOOL_ACTION toggleExcludeFromBoard;
    static TOOL_ACTION setDNP;
    static TOOL_ACTION unsetDNP;
    static TOOL_ACTION toggleDNP;

    // Design Block management
    static TOOL_ACTION showDesignBlockPanel;
    static TOOL_ACTION saveSheetAsDesignBlock;
    static TOOL_ACTION saveSelectionAsDesignBlock;
    static TOOL_ACTION deleteDesignBlock;
    static TOOL_ACTION editDesignBlockProperties;

    // Library management
    static TOOL_ACTION saveLibraryAs;
    static TOOL_ACTION saveSymbolAs;
    static TOOL_ACTION saveSymbolCopyAs;
    static TOOL_ACTION newSymbol;
    static TOOL_ACTION deriveFromExistingSymbol;
    static TOOL_ACTION editSymbol;
    static TOOL_ACTION duplicateSymbol;
    static TOOL_ACTION renameSymbol;
    static TOOL_ACTION deleteSymbol;
    static TOOL_ACTION cutSymbol;
    static TOOL_ACTION copySymbol;
    static TOOL_ACTION pasteSymbol;
    static TOOL_ACTION importSymbol;
    static TOOL_ACTION exportSymbol;
    static TOOL_ACTION updateSymbolFields;
    static TOOL_ACTION setUnitDisplayName;

    // Hierarchy navigation
    static TOOL_ACTION changeSheet;
    static TOOL_ACTION enterSheet;
    static TOOL_ACTION leaveSheet;
    static TOOL_ACTION navigateUp;
    static TOOL_ACTION navigateForward;
    static TOOL_ACTION navigateBack;
    static TOOL_ACTION navigatePrevious;
    static TOOL_ACTION navigateNext;
    static TOOL_ACTION showHierarchy;
    static TOOL_ACTION hypertextCommand;

    // Global edit tools
    static TOOL_ACTION cleanupSheetPins;
    static TOOL_ACTION editTextAndGraphics;

    // Miscellaneous
    static TOOL_ACTION toggleHiddenPins;
    static TOOL_ACTION toggleHiddenFields;
    static TOOL_ACTION showHiddenPins;
    static TOOL_ACTION showHiddenFields;
    static TOOL_ACTION toggleDirectiveLabels;
    static TOOL_ACTION toggleERCWarnings;
    static TOOL_ACTION toggleERCErrors;
    static TOOL_ACTION toggleERCExclusions;
    static TOOL_ACTION markSimExclusions;
    static TOOL_ACTION toggleOPVoltages;
    static TOOL_ACTION toggleOPCurrents;
    static TOOL_ACTION togglePinAltIcons;
    static TOOL_ACTION toggleSyncedPinsMode;
    static TOOL_ACTION restartMove;
    static TOOL_ACTION selectOnPCB;
    static TOOL_ACTION pushPinLength;
    static TOOL_ACTION pushPinNameSize;
    static TOOL_ACTION pushPinNumSize;
    static TOOL_ACTION showElectricalTypes;
    static TOOL_ACTION showPinNumbers;
    static TOOL_ACTION symbolTreeSearch;
    static TOOL_ACTION drawSheetOnClipboard;
    static TOOL_ACTION importGraphics;
    static TOOL_ACTION exportSymbolView;
    static TOOL_ACTION exportSymbolAsSVG;
    static TOOL_ACTION showPythonConsole;
    static TOOL_ACTION repairSchematic;
    static TOOL_ACTION previousUnit;
    static TOOL_ACTION nextUnit;

    // Line modes
    static TOOL_ACTION lineModeFree;
    static TOOL_ACTION lineMode90;
    static TOOL_ACTION lineMode45;
    static TOOL_ACTION lineModeNext;

    // Annotation
    static TOOL_ACTION toggleAnnotateAuto;

    // SPICE
    static TOOL_ACTION newAnalysisTab;
    static TOOL_ACTION openWorkbook;
    static TOOL_ACTION saveWorkbook;
    static TOOL_ACTION saveWorkbookAs;
    static TOOL_ACTION exportPlotAsPNG;
    static TOOL_ACTION exportPlotAsCSV;
    static TOOL_ACTION exportPlotToClipboard;
    static TOOL_ACTION exportPlotToSchematic;
    static TOOL_ACTION showSimulator;
    static TOOL_ACTION simProbe;
    static TOOL_ACTION simTune;
    static TOOL_ACTION toggleLegend;
    static TOOL_ACTION toggleDottedSecondary;
    static TOOL_ACTION toggleDarkModePlots;
    static TOOL_ACTION simAnalysisProperties;
    static TOOL_ACTION runSimulation;
    static TOOL_ACTION stopSimulation;
    static TOOL_ACTION editUserDefinedSignals;
    static TOOL_ACTION showNetlist;

    // Net highlighting
    static TOOL_ACTION highlightNet;
    static TOOL_ACTION clearHighlight;
    static TOOL_ACTION updateNetHighlighting;
    static TOOL_ACTION highlightNetTool;
    static TOOL_ACTION showNetNavigator;

    // Drag and drop
    static TOOL_ACTION ddAppendFile;

    struct PLACE_SYMBOL_PARAMS
    {
        ///< Provide a symbol to place
        SCH_SYMBOL* m_Symbol = nullptr;
        ///< If a symbol is provide, reannotate it?
        bool m_Reannotate = true;
    };
};
