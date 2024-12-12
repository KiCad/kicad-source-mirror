/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2013-2016 CERN
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
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

#include <layer_ids.h>
#include <tool/tool_action.h>
#include <tool/actions.h>

enum class ZONE_MODE
{
    ADD,             ///< Add a new zone/keepout with fresh settings
    CUTOUT,          ///< Make a cutout to an existing zone
    SIMILAR,         ///< Add a new zone with the same settings as an existing one
    GRAPHIC_POLYGON
};

class DESIGN_BLOCK;

/**
 * Gather all the actions that are shared by tools.
 *
 * The instance of PCB_ACTIONS is created inside of #ACTION_MANAGER object that registers
 * the actions.
 */
class PCB_ACTIONS : public ACTIONS
{
public:
    /**
     * Translate a layer ID into the action that switches to that layer.
     *
     * @param aLayerID is the layer to switch to
     * @return the action that will switch to the specified layer
     */
    static TOOL_ACTION* LayerIDToAction( PCB_LAYER_ID aLayerID );

    // Selection Tool
    /// Sets selection to specified items, zooms to fit, if enabled
    static TOOL_ACTION syncSelection;

    /// Sets selection to specified items with connected nets, zooms to fit, if enabled
    static TOOL_ACTION syncSelectionWithNets;

    /// Run a selection menu to select from a list of items
    static TOOL_ACTION selectionMenu;

    /// Select tracks between junctions or expands an existing selection to pads or the
    /// entire connection.
    static TOOL_ACTION selectConnection;

    /// Removes all tracks from the selected items to the first pad
    static TOOL_ACTION unrouteSelected;

    /// Removes track segment from the selected item to the next segment
    static TOOL_ACTION unrouteSegment;

    /// Select all connections belonging to a single net.
    static TOOL_ACTION selectNet;

    /// Remove all connections belonging to a single net from the active selection
    static TOOL_ACTION deselectNet;

    /// Select unconnected footprints from ratsnest of selection
    static TOOL_ACTION selectUnconnected;

    /// Select and move nearest unconnected footprint from ratsnest of selection
    static TOOL_ACTION grabUnconnected;

    /// Select all components on sheet from Eeschema crossprobing.
    static TOOL_ACTION selectOnSheetFromEeschema;

    /// Select all components on the same sheet as the selected footprint.
    static TOOL_ACTION selectSameSheet;

    /// Select symbols/pins on schematic corresponding to selected footprints/pads.
    static TOOL_ACTION selectOnSchematic;

    /// Filter the items in the current selection (invokes dialog)
    static TOOL_ACTION filterSelection;

    /// move or drag an item
    static TOOL_ACTION move;

    /// move items one-by-one
    static TOOL_ACTION moveIndividually;

    /// move with a reference point
    static TOOL_ACTION moveWithReference;

    /// copy command with manual reference point selection
    static TOOL_ACTION copyWithReference;

    /// Rotation of selected objects
    static TOOL_ACTION rotateCw;
    static TOOL_ACTION rotateCcw;

    /// Flipping of selected objects
    static TOOL_ACTION flip;

    /// Mirroring of selected items
    static TOOL_ACTION mirrorH;
    static TOOL_ACTION mirrorV;

    /// Swapping of selected items
    static TOOL_ACTION swap;

    /// Swap nets between selected pads/gates (and connected copper)
    static TOOL_ACTION swapPadNets;
    static TOOL_ACTION swapGateNets;

    /// Pack and start moving selected footprints
    static TOOL_ACTION packAndMoveFootprints;

    // Compound Action Tool actions, e.g. Move Individually
    static TOOL_ACTION skip;

    /// Update selected tracks & vias to the current track & via dimensions
    static TOOL_ACTION changeTrackWidth;
    static TOOL_ACTION changeTrackLayerNext;
    static TOOL_ACTION changeTrackLayerPrev;

    /// Fillet (i.e. adds an arc tangent to) all selected straight tracks by a user defined radius
    static TOOL_ACTION filletTracks;

    /// Fillet (i.e. adds an arc tangent to) all selected straight lines by a user defined radius
    static TOOL_ACTION filletLines;
    /// Chamfer (i.e. adds a straight line) all selected straight lines by a user defined setback
    static TOOL_ACTION chamferLines;
    /// Add "dogbone" corners to selected lines to allow routing with a cutter radius
    static TOOL_ACTION dogboneCorners;
    /// Connect selected shapes, possibly extending or cutting them, or adding extra geometry
    static TOOL_ACTION healShapes;
    /// Extend selected lines to meet at a point
    static TOOL_ACTION extendLines;
    /// Simplify polygon outlines
    static TOOL_ACTION simplifyPolygons;
    /// Edit polygon vertices in a table
    static TOOL_ACTION editVertices;
    /// Create outset items from selection
    static TOOL_ACTION outsetItems;

    /// Merge multiple polygons into a single polygon
    static TOOL_ACTION mergePolygons;
    /// Subtract polygons from other polygons
    static TOOL_ACTION subtractPolygons;
    /// Intersection of multiple polygons
    static TOOL_ACTION intersectPolygons;

    /// Activation of the edit tool
    static TOOL_ACTION properties;

    /// Activation of the exact move tool
    static TOOL_ACTION moveExact;

    /// Activation of the duplication tool with incrementing (e.g. pad number)
    static TOOL_ACTION duplicateIncrement;

    /// Deleting a BOARD_ITEM
    static TOOL_ACTION remove;
    static TOOL_ACTION deleteFull;

    static TOOL_ACTION selectLayerPair;

    /// Break a single track into two segments at the cursor
    static TOOL_ACTION breakTrack;

    static TOOL_ACTION drag45Degree;
    static TOOL_ACTION dragFreeAngle;

    // Drawing Tool Activations
    static TOOL_ACTION drawLine;
    static TOOL_ACTION drawPolygon;
    static TOOL_ACTION drawRectangle;
    static TOOL_ACTION drawCircle;
    static TOOL_ACTION drawArc;
    static TOOL_ACTION drawBezier;
    static TOOL_ACTION placePoint;
    static TOOL_ACTION placeReferenceImage;
    static TOOL_ACTION placeText;
    static TOOL_ACTION drawTextBox;
    static TOOL_ACTION drawTable;
    static TOOL_ACTION spacingIncrease;
    static TOOL_ACTION spacingDecrease;
    static TOOL_ACTION amplIncrease;
    static TOOL_ACTION amplDecrease;
    static TOOL_ACTION lengthTunerSettings;
    static TOOL_ACTION drawAlignedDimension;
    static TOOL_ACTION drawCenterDimension;
    static TOOL_ACTION drawRadialDimension;
    static TOOL_ACTION drawOrthogonalDimension;
    static TOOL_ACTION drawLeader;
    static TOOL_ACTION placeBarcode;
    static TOOL_ACTION drawZone;
    static TOOL_ACTION drawVia;
    static TOOL_ACTION drawRuleArea;
    static TOOL_ACTION drawZoneCutout;
    static TOOL_ACTION drawSimilarZone;
    static TOOL_ACTION placeCharacteristics;
    static TOOL_ACTION placeStackup;
    static TOOL_ACTION placeFootprint;
    static TOOL_ACTION placeImportedGraphics;
    static TOOL_ACTION setAnchor;
    static TOOL_ACTION deleteLastPoint;

    // Line mode grouping and events
    static TOOL_ACTION lineModeFree;         ///< Unconstrained angle mode (icon lines_any)
    static TOOL_ACTION lineMode90;           ///< 90-degree-only mode (icon lines90)
    static TOOL_ACTION lineMode45;           ///< 45-degree-or-orthogonal mode (icon hv45mode)
    static TOOL_ACTION lineModeNext;         ///< Cycle through angle modes
    static TOOL_ACTION angleSnapModeChanged; ///< Notification event when angle mode changes
    static TOOL_ACTION closeOutline;

    /// Increase width of currently drawn line
    static TOOL_ACTION incWidth;

    /// Decrease width of currently drawn line
    static TOOL_ACTION decWidth;

    /// Switch posture when drawing arc
    static TOOL_ACTION arcPosture;

    /// Switch between dimension arrow directions
    static TOOL_ACTION changeDimensionArrows;

    /// Snapping controls
    static TOOL_ACTION magneticSnapActiveLayer;
    static TOOL_ACTION magneticSnapAllLayers;
    static TOOL_ACTION magneticSnapToggle;

    // Push and Shove Router Tool

    /// Activation of the Push and Shove router
    static TOOL_ACTION routeSingleTrack;

    /// Activation of the Push and Shove router (differential pair mode)
    static TOOL_ACTION routeDiffPair;

    static TOOL_ACTION tuneSingleTrack;
    static TOOL_ACTION tuneDiffPair;
    static TOOL_ACTION tuneSkew;

    static TOOL_ACTION routerUndoLastSegment;

    static TOOL_ACTION routerContinueFromEnd;
    static TOOL_ACTION routerAttemptFinish;
    static TOOL_ACTION routerRouteSelected;
    static TOOL_ACTION routerRouteSelectedFromEnd;
    static TOOL_ACTION routerAutorouteSelected;

    /// Activation of the Push and Shove settings dialogs
    static TOOL_ACTION routerSettingsDialog;
    static TOOL_ACTION routerDiffPairDialog;

    /// Actions to enable switching modes via hotkey assignments
    static TOOL_ACTION routerHighlightMode;
    static TOOL_ACTION routerShoveMode;
    static TOOL_ACTION routerWalkaroundMode;
    static TOOL_ACTION cycleRouterMode;

    /// Activation of the Push and Shove router (inline dragging mode)
    static TOOL_ACTION routerInlineDrag;

    /// Generator tool
    static TOOL_ACTION regenerateAllTuning;
    static TOOL_ACTION regenerateAll;
    static TOOL_ACTION regenerateSelected;
    static TOOL_ACTION regenerateItem;
    static TOOL_ACTION genStartEdit;
    static TOOL_ACTION genUpdateEdit;
    static TOOL_ACTION genFinishEdit;
    static TOOL_ACTION genCancelEdit;
    static TOOL_ACTION genRemove;

    static TOOL_ACTION generatorsShowManager;

    // Point Editor
    static TOOL_ACTION pointEditorAddCorner;
    static TOOL_ACTION pointEditorRemoveCorner;
    static TOOL_ACTION pointEditorChamferCorner;

    static TOOL_ACTION pointEditorMoveCorner;
    static TOOL_ACTION pointEditorMoveMidpoint;

    // Placement Tool
    static TOOL_ACTION alignTop;
    static TOOL_ACTION alignBottom;
    static TOOL_ACTION alignLeft;
    static TOOL_ACTION alignRight;
    static TOOL_ACTION alignCenterX;
    static TOOL_ACTION alignCenterY;
    static TOOL_ACTION distributeHorizontallyCenters;
    static TOOL_ACTION distributeHorizontallyGaps;
    static TOOL_ACTION distributeVerticallyCenters;
    static TOOL_ACTION distributeVerticallyGaps;

    // Position Relative Tool
    static TOOL_ACTION positionRelative;
    static TOOL_ACTION interactiveOffsetTool;

    /// Selection of reference points/items
    static TOOL_ACTION selectItemInteractively;
    static TOOL_ACTION selectPointInteractively;

    // Display modes
    static TOOL_ACTION showRatsnest;
    static TOOL_ACTION ratsnestLineMode;
    static TOOL_ACTION netColorModeCycle;
    static TOOL_ACTION ratsnestModeCycle;
    static TOOL_ACTION trackDisplayMode;
    static TOOL_ACTION padDisplayMode;
    static TOOL_ACTION viaDisplayMode;
    static TOOL_ACTION zoneDisplayFilled;
    static TOOL_ACTION zoneDisplayOutline;
    static TOOL_ACTION zoneDisplayFractured;
    static TOOL_ACTION zoneDisplayTriangulated;
    static TOOL_ACTION zoneDisplayToggle;
    static TOOL_ACTION showPadNumbers;
    static TOOL_ACTION fpAutoZoom;

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
    static TOOL_ACTION layerPairPresetsCycle;

    // Group to link all actions that directly select layers
    static TOOL_ACTION_GROUP layerDirectSwitchActions();

    static TOOL_ACTION layerChanged;        // notification

    static TOOL_ACTION flipBoard;

    static TOOL_ACTION rehatchShapes;

    // Track & via size control
    static TOOL_ACTION trackWidthInc;
    static TOOL_ACTION trackWidthDec;
    static TOOL_ACTION viaSizeInc;
    static TOOL_ACTION viaSizeDec;

    static TOOL_ACTION autoTrackWidth;

    static TOOL_ACTION trackViaSizeChanged;   // notification

    static TOOL_ACTION assignNetClass;

    // Zone actions
    static TOOL_ACTION zoneFill;
    static TOOL_ACTION zoneFillAll;
    static TOOL_ACTION zoneFillDirty;
    static TOOL_ACTION zoneUnfill;
    static TOOL_ACTION zoneUnfillAll;
    static TOOL_ACTION zoneMerge;

    /// Duplicate zone onto another layer
    static TOOL_ACTION zoneDuplicate;

    /// Scripting Actions
    static TOOL_ACTION pluginsShowFolder;

    // Board editor control
    static TOOL_ACTION appendBoard;
    static TOOL_ACTION rescueAutosave;
    static TOOL_ACTION openNonKicadBoard;
    static TOOL_ACTION exportFootprints;
    static TOOL_ACTION boardSetup;

    static TOOL_ACTION importNetlist;

    static TOOL_ACTION importSpecctraSession;
    static TOOL_ACTION exportSpecctraDSN;

    static TOOL_ACTION generateGerbers;
    static TOOL_ACTION generateDrillFiles;
    static TOOL_ACTION generatePosFile;
    static TOOL_ACTION generateReportFile;
    static TOOL_ACTION generateIPC2581File;
    static TOOL_ACTION generateODBPPFile;
    static TOOL_ACTION generateD356File;
    static TOOL_ACTION generateBOM;

    static TOOL_ACTION exportGenCAD;
    static TOOL_ACTION exportVRML;
    static TOOL_ACTION exportIDF;
    static TOOL_ACTION exportSTEP;
    static TOOL_ACTION exportCmpFile;
    static TOOL_ACTION exportHyperlynx;

    // Global edit tool
    static TOOL_ACTION editTracksAndVias;
    static TOOL_ACTION editTextAndGraphics;
    static TOOL_ACTION editTeardrops;
    static TOOL_ACTION globalDeletions;
    static TOOL_ACTION cleanupTracksAndVias;
    static TOOL_ACTION cleanupGraphics;
    static TOOL_ACTION updateFootprint;
    static TOOL_ACTION updateFootprints;
    static TOOL_ACTION changeFootprint;
    static TOOL_ACTION changeFootprints;
    static TOOL_ACTION swapLayers;
    static TOOL_ACTION removeUnusedPads;

    static TOOL_ACTION runDRC;
    static TOOL_ACTION drcRuleEditor;

    static TOOL_ACTION editFpInFpEditor;
    static TOOL_ACTION editLibFpInFpEditor;

    static TOOL_ACTION showLayersManager;
    static TOOL_ACTION showNetInspector;
    static TOOL_ACTION showPythonConsole;
    static TOOL_ACTION zonesManager;

    // Design Block management
    static TOOL_ACTION placeDesignBlock;
    static TOOL_ACTION placeLinkedDesignBlock;
    static TOOL_ACTION applyDesignBlockLayout;
    static TOOL_ACTION saveToLinkedDesignBlock;
    static TOOL_ACTION showDesignBlockPanel;
    static TOOL_ACTION saveBoardAsDesignBlock;
    static TOOL_ACTION saveSelectionAsDesignBlock;
    static TOOL_ACTION updateDesignBlockFromBoard;
    static TOOL_ACTION updateDesignBlockFromSelection;
    static TOOL_ACTION deleteDesignBlock;
    static TOOL_ACTION editDesignBlockProperties;

    // Footprint editor tools

    // We don't use ACTION::new here because we need to distinguish between New Library
    // and New Footprint.
    static TOOL_ACTION newFootprint;

    // Create a new footprint using the Footprint Wizard
    static TOOL_ACTION createFootprint;

    static TOOL_ACTION editFootprint;
    static TOOL_ACTION duplicateFootprint;
    static TOOL_ACTION renameFootprint;
    static TOOL_ACTION deleteFootprint;
    static TOOL_ACTION cutFootprint;
    static TOOL_ACTION copyFootprint;
    static TOOL_ACTION pasteFootprint;
    static TOOL_ACTION importFootprint;
    static TOOL_ACTION exportFootprint;

    static TOOL_ACTION footprintProperties;
    static TOOL_ACTION defaultPadProperties;
    static TOOL_ACTION padTable;

    static TOOL_ACTION checkFootprint;

    static TOOL_ACTION loadFpFromBoard;
    static TOOL_ACTION saveFpToBoard;

    static TOOL_ACTION previousFootprint;
    static TOOL_ACTION nextFootprint;

    /// Activation of the drawing tool (placing a PAD)
    static TOOL_ACTION placePad;

    static TOOL_ACTION explodePad;
    static TOOL_ACTION recombinePad;

    /// Tool for quick pad enumeration
    static TOOL_ACTION enumeratePads;

    /// Tool for creating an array of objects
    static TOOL_ACTION createArray;

    /// Display footprint graphics as outlines
    static TOOL_ACTION graphicsOutlines;

    /// Display texts as lines
    static TOOL_ACTION textOutlines;

    // Pad tools

    /// Copy the selected pad's settings to the board design settings
    static TOOL_ACTION copyPadSettings;

    /// Copy the default pad settings to the selected pad
    static TOOL_ACTION applyPadSettings;

    /// Copy the current pad's settings to other pads in the footprint or on the board
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
    static TOOL_ACTION drillOrigin;
    static TOOL_ACTION drillResetOrigin;
    static TOOL_ACTION drillSetOrigin;
    static TOOL_ACTION showEeschema;
    static TOOL_ACTION boardStatistics;
    static TOOL_ACTION boardReannotate;
    static TOOL_ACTION repairBoard;
    static TOOL_ACTION repairFootprint;
    static TOOL_ACTION inspectClearance;
    static TOOL_ACTION inspectConstraints;
    static TOOL_ACTION diffFootprint;
    static TOOL_ACTION showFootprintAssociations;
    static TOOL_ACTION collect3DModels;

    // Appearance controls
    static TOOL_ACTION clearHighlight;          // Turns off highlight and resets previous highlight
    static TOOL_ACTION highlightNet;            // Highlights a net by code (cross-probe highlight)
    static TOOL_ACTION toggleLastNetHighlight;  // Toggles between current and previous highlight
    static TOOL_ACTION toggleNetHighlight;      // Toggles between highlight off and highlight on
    static TOOL_ACTION highlightNetSelection;   // Turns on highlight and takes net from selection
    static TOOL_ACTION highlightItem;           // Select component via cross-probe

    // Ratsnest
    static TOOL_ACTION hideNetInRatsnest;
    static TOOL_ACTION showNetInRatsnest;
    static TOOL_ACTION localRatsnestTool;
    static TOOL_ACTION hideLocalRatsnest;
    static TOOL_ACTION updateLocalRatsnest;

    /// Find an item
    static TOOL_ACTION find;

    /// Find an item and start moving
    static TOOL_ACTION getAndPlace;

    static TOOL_ACTION autoplaceOffboardComponents;
    static TOOL_ACTION autoplaceSelectedComponents;

    // convert tool
    static TOOL_ACTION convertToPoly;
    static TOOL_ACTION convertToZone;
    static TOOL_ACTION convertToKeepout;
    static TOOL_ACTION convertToLines;
    static TOOL_ACTION convertToArc;
    static TOOL_ACTION convertToTracks;

    /// Drag and drop
    static TOOL_ACTION ddAppendBoard;
    static TOOL_ACTION ddImportFootprint;
    static TOOL_ACTION ddImportGraphics;

    static TOOL_ACTION repeatLayout;
    static TOOL_ACTION generatePlacementRuleAreas;

    /// Footprint wizard frame actions:
    static TOOL_ACTION showWizards;
    static TOOL_ACTION resetWizardPrms;
    static TOOL_ACTION selectPreviousWizardPage;
    static TOOL_ACTION selectNextWizardPage;
    static TOOL_ACTION exportFpToEditor;
};

class PCB_EVENTS
{
public:
    // These are functions that access the underlying event because the event constructor
    // needs the ACTION::cancelInteractive action, so we must

    /// Hotkey feedback
    static const TOOL_EVENT& SnappingModeChangedByKeyEvent();
    static const TOOL_EVENT& LayerPairPresetChangedByKeyEvent();
};

#endif
