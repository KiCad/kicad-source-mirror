/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2013-2016 CERN
 * Copyright (C) 2016 KiCad Developers, see AUTHORS.txt for contributors.
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

#ifndef __PCB_ACTIONS_H
#define __PCB_ACTIONS_H

#include <tool/tool_action.h>
#include <tool/actions.h>
#include <core/optional.h>

class TOOL_EVENT;
class TOOL_MANAGER;

// Type of microwave items that are "simple" - just get placed on the board directly,
// without a graphical interactive setup stage
enum MWAVE_TOOL_SIMPLE_ID
{
    GAP,
    STUB,
    STUB_ARC,
    FUNCTION_SHAPE,
};


enum class ZONE_MODE
{
    ADD,             ///< Add a new zone/keepout with fresh settings
    CUTOUT,          ///< Make a cutout to an existing zone
    SIMILAR,         ///< Add a new zone with the same settings as an existing one
    GRAPHIC_POLYGON
};


/**
 * Class PCB_ACTIONS
 *
 * Gathers all the actions that are shared by tools. The instance of PCB_ACTIONS is created
 * inside of ACTION_MANAGER object that registers the actions.
 */
class PCB_ACTIONS : public ACTIONS
{
public:
    // Selection Tool
    /// Activation of the selection tool
    static TOOL_ACTION selectionActivate;

    /// Select a single item under the cursor position
    static TOOL_ACTION selectionCursor;

    /// Clears the current selection
    static TOOL_ACTION selectionClear;

    /// Selects an item (specified as the event parameter).
    static TOOL_ACTION selectItem;
    static TOOL_ACTION unselectItem;

    /// Selects a list of items (specified as the event parameter)
    static TOOL_ACTION selectItems;
    static TOOL_ACTION unselectItems;

    /// Runs a selection menu to select from a list of items
    static TOOL_ACTION selectionMenu;

    /// Selects a connection between junctions.
    static TOOL_ACTION selectConnection;

    /// Expands the current selection to select a connection between two junctions
    static TOOL_ACTION expandSelectedConnection;

    /// Selects whole copper connection.
    static TOOL_ACTION selectCopper;

    /// Selects all connections belonging to a single net.
    static TOOL_ACTION selectNet;

    /// Selects all components on sheet from Eeschema crossprobing.
    static TOOL_ACTION selectOnSheetFromEeschema;

    /// Selects all components on the same sheet as the selected footprint.
    static TOOL_ACTION selectSameSheet;

    /// Filters the items in the current selection (invokes dialog)
    static TOOL_ACTION filterSelection;

    /// move an item
    static TOOL_ACTION move;

    /// Rotation of selected objects
    static TOOL_ACTION rotateCw;
    static TOOL_ACTION rotateCcw;

    /// Flipping of selected objects
    static TOOL_ACTION flip;

    /// Mirroring of selected items
    static TOOL_ACTION mirror;

    /// Activation of the edit tool
    static TOOL_ACTION properties;

    /// Activation of the exact move tool
    static TOOL_ACTION moveExact;

    /// Activation of the duplication tool
    static TOOL_ACTION duplicate;

    /// Activation of the duplication tool with incrementing (e.g. pad number)
    static TOOL_ACTION duplicateIncrement;

    /// Deleting a BOARD_ITEM
    static TOOL_ACTION remove;
    static TOOL_ACTION deleteFull;

    static TOOL_ACTION selectLayerPair;

    /// Break a single track into two segments at the cursor
    static TOOL_ACTION breakTrack;

    /// Breaks track when router is not activated
    static TOOL_ACTION inlineBreakTrack;

    static TOOL_ACTION drag45Degree;
    static TOOL_ACTION dragFreeAngle;


    // Drawing Tool Activations
    static TOOL_ACTION drawLine;
    static TOOL_ACTION drawPolygon;
    static TOOL_ACTION drawCircle;
    static TOOL_ACTION drawArc;
    static TOOL_ACTION placeText;
    static TOOL_ACTION drawDimension;
    static TOOL_ACTION drawZone;
    static TOOL_ACTION drawVia;
    static TOOL_ACTION drawZoneKeepout;
    static TOOL_ACTION drawZoneCutout;
    static TOOL_ACTION drawSimilarZone;
    static TOOL_ACTION placeTarget;
    static TOOL_ACTION placeModule;
    static TOOL_ACTION placeImportedGraphics;
    static TOOL_ACTION setAnchor;
    static TOOL_ACTION deleteLastPoint;
    static TOOL_ACTION closeZoneOutline;

    /// Increase width of currently drawn line
    static TOOL_ACTION incWidth;

    /// Decrease width of currently drawn line
    static TOOL_ACTION decWidth;

    /// Switch posture when drawing arc
    static TOOL_ACTION arcPosture;

    // Push and Shove Router Tool

    /// Activation of the Push and Shove router
    static TOOL_ACTION routeSingleTrack;

    /// Activation of the Push and Shove router (differential pair mode)
    static TOOL_ACTION routeDiffPair;

    /// Activation of the Push and Shove router (tune single line mode)
    static TOOL_ACTION routerTuneSingleTrace;

    /// Activation of the Push and Shove router (diff pair tuning mode)
    static TOOL_ACTION routerTuneDiffPair;

    /// Activation of the Push and Shove router (skew tuning mode)
    static TOOL_ACTION routerTuneDiffPairSkew;

    /// Activation of the Push and Shove settings dialogs
    static TOOL_ACTION routerSettingsDialog;
    static TOOL_ACTION routerDiffPairDialog;


    /// Activation of the Push and Shove router (inline dragging mode)
    static TOOL_ACTION routerInlineDrag;

    // Point Editor
    /// Break outline (insert additional points to an edge)
    static TOOL_ACTION pointEditorAddCorner;

    /// Removes a corner
    static TOOL_ACTION pointEditorRemoveCorner;

    // Placement tool
    static TOOL_ACTION alignTop;
    static TOOL_ACTION alignBottom;
    static TOOL_ACTION alignLeft;
    static TOOL_ACTION alignRight;
    static TOOL_ACTION alignCenterX;
    static TOOL_ACTION alignCenterY;
    static TOOL_ACTION distributeHorizontally;
    static TOOL_ACTION distributeVertically;

    // Position Relative Tool
    /// Activation of the position relative tool
    static TOOL_ACTION positionRelative;

    /// Selection of anchor item for position relative tool
    static TOOL_ACTION selectpositionRelativeItem;

    // Display modes
    static TOOL_ACTION showRatsnest;
    static TOOL_ACTION ratsnestLineMode;
    static TOOL_ACTION trackDisplayMode;
    static TOOL_ACTION padDisplayMode;
    static TOOL_ACTION viaDisplayMode;
    static TOOL_ACTION graphicDisplayMode;
    static TOOL_ACTION zoneDisplayEnable;
    static TOOL_ACTION zoneDisplayDisable;
    static TOOL_ACTION zoneDisplayOutlines;
    static TOOL_ACTION zoneDisplayToggle;

    // Layer control
    static TOOL_ACTION layerTop;
    static TOOL_ACTION layerInner1;
    static TOOL_ACTION layerInner2;
    static TOOL_ACTION layerInner3;
    static TOOL_ACTION layerInner4;
    static TOOL_ACTION layerInner5;
    static TOOL_ACTION layerInner6;
    static TOOL_ACTION layerInner7;
    static TOOL_ACTION layerInner8;
    static TOOL_ACTION layerInner9;
    static TOOL_ACTION layerInner10;
    static TOOL_ACTION layerInner11;
    static TOOL_ACTION layerInner12;
    static TOOL_ACTION layerInner13;
    static TOOL_ACTION layerInner14;
    static TOOL_ACTION layerInner15;
    static TOOL_ACTION layerInner16;
    static TOOL_ACTION layerInner17;
    static TOOL_ACTION layerInner18;
    static TOOL_ACTION layerInner19;
    static TOOL_ACTION layerInner20;
    static TOOL_ACTION layerInner21;
    static TOOL_ACTION layerInner22;
    static TOOL_ACTION layerInner23;
    static TOOL_ACTION layerInner24;
    static TOOL_ACTION layerInner25;
    static TOOL_ACTION layerInner26;
    static TOOL_ACTION layerInner27;
    static TOOL_ACTION layerInner28;
    static TOOL_ACTION layerInner29;
    static TOOL_ACTION layerInner30;
    static TOOL_ACTION layerBottom;
    static TOOL_ACTION layerNext;
    static TOOL_ACTION layerPrev;
    static TOOL_ACTION layerAlphaInc;
    static TOOL_ACTION layerAlphaDec;
    static TOOL_ACTION layerToggle;

    static TOOL_ACTION layerChanged;        // notification

    static TOOL_ACTION flipBoard;

    // Track & via size control
    static TOOL_ACTION trackWidthInc;
    static TOOL_ACTION trackWidthDec;
    static TOOL_ACTION viaSizeInc;
    static TOOL_ACTION viaSizeDec;

    static TOOL_ACTION trackViaSizeChanged;   // notification

    // Zone actions
    static TOOL_ACTION zoneFill;
    static TOOL_ACTION zoneFillAll;
    static TOOL_ACTION zoneUnfill;
    static TOOL_ACTION zoneUnfillAll;
    static TOOL_ACTION zoneMerge;

    /// Duplicate zone onto another layer
    static TOOL_ACTION zoneDuplicate;

    // Global edit tool
    static TOOL_ACTION boardSetup;
    static TOOL_ACTION editTracksAndVias;
    static TOOL_ACTION editTextAndGraphics;
    static TOOL_ACTION globalDeletions;
    static TOOL_ACTION cleanupTracksAndVias;
    static TOOL_ACTION updateFootprint;
    static TOOL_ACTION updateFootprints;
    static TOOL_ACTION changeFootprint;
    static TOOL_ACTION changeFootprints;
    static TOOL_ACTION swapLayers;

    static TOOL_ACTION importNetlist;

    static TOOL_ACTION importSpecctraSession;
    static TOOL_ACTION exportSpecctraDSN;
    
    static TOOL_ACTION generateGerbers;
    static TOOL_ACTION generateDrillFiles;
    static TOOL_ACTION generatePosFile;
    static TOOL_ACTION generateReportFile;
    static TOOL_ACTION generateD356File;
    static TOOL_ACTION generateBOM;
    
    static TOOL_ACTION listNets;
    static TOOL_ACTION runDRC;

    static TOOL_ACTION editFootprintInFpEditor;
    static TOOL_ACTION showLayersManager;
    static TOOL_ACTION showMicrowaveToolbar;
    static TOOL_ACTION showPythonConsole;

    // Module editor tools

    static TOOL_ACTION toggleFootprintTree;
    
    // We don't use ACTION::new here because we need to distinguish between New Library
    // and New Footprint.
    static TOOL_ACTION newFootprint;
    
    // Create a new footprint using the Footprint Wizard
    static TOOL_ACTION createFootprint;

    // We don't use ACTION::save here because we need to distinguish between saving to
    // the library and saving to the board (which have different tooltips and icons).
    static TOOL_ACTION saveToBoard;
    static TOOL_ACTION saveToLibrary;

    static TOOL_ACTION editFootprint;
    static TOOL_ACTION deleteFootprint;
    static TOOL_ACTION cutFootprint;
    static TOOL_ACTION copyFootprint;
    static TOOL_ACTION pasteFootprint;
    static TOOL_ACTION importFootprint;
    static TOOL_ACTION exportFootprint;
    
    static TOOL_ACTION footprintProperties;
    static TOOL_ACTION defaultPadProperties;

    /// Activation of the drawing tool (placing a PAD)
    static TOOL_ACTION placePad;

    static TOOL_ACTION createPadFromShapes;
    static TOOL_ACTION explodePadToShapes;

    /// Tool for quick pad enumeration
    static TOOL_ACTION enumeratePads;

    /// Tool for creating an array of objects
    static TOOL_ACTION createArray;

    /// Display module edges as outlines
    static TOOL_ACTION moduleEdgeOutlines;

    // Pad tools

    /// Copy the selected pad's settings to the board design settings
    static TOOL_ACTION copyPadSettings;

    /// Copy the default pad settings to the selected pad
    static TOOL_ACTION applyPadSettings;

    /// Copy the current pad's settings to other pads in the module or on the board
    static TOOL_ACTION pushPadSettings;

    // Microwave tools
    static TOOL_ACTION microwaveCreateGap;

    static TOOL_ACTION microwaveCreateStub;

    static TOOL_ACTION microwaveCreateStubArc;

    static TOOL_ACTION microwaveCreateFunctionShape;

    static TOOL_ACTION microwaveCreateLine;

    // Locking
    static TOOL_ACTION toggleLock;
    static TOOL_ACTION lock;
    static TOOL_ACTION unlock;

    // Miscellaneous
    static TOOL_ACTION selectionTool;
    static TOOL_ACTION pickerTool;
    static TOOL_ACTION measureTool;
    static TOOL_ACTION updateUnits;
    static TOOL_ACTION clearHighlight;
    static TOOL_ACTION highlightNet;
    static TOOL_ACTION toggleLastNetHighlight;
    static TOOL_ACTION highlightNetTool;
    static TOOL_ACTION highlightNetSelection;
    static TOOL_ACTION highlightItem;
    static TOOL_ACTION drillOrigin;
    static TOOL_ACTION appendBoard;
    static TOOL_ACTION showEeschema;
    static TOOL_ACTION boardStatistics;

    // Ratsnest
    static TOOL_ACTION localRatsnestTool;
    static TOOL_ACTION hideDynamicRatsnest;
    static TOOL_ACTION updateLocalRatsnest;

    /// Find an item
    static TOOL_ACTION find;

    /// Find an item and start moving
    static TOOL_ACTION getAndPlace;

    static TOOL_ACTION autoplaceOffboardComponents;
    static TOOL_ACTION autoplaceSelectedComponents;

    ///> @copydoc COMMON_ACTIONS::TranslateLegacyId()
    virtual OPT<TOOL_EVENT> TranslateLegacyId( int aId ) override;
};

#endif
