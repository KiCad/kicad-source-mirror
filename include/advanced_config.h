/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
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

#include <kicommon.h>

class wxConfigBase;

/**
 * @defgroup advanced_config Advanced Configuration Variables
 *
 * Class containing "advanced" configuration options.
 *
 * Options set here are for developer or advanced users only. If a general user
 * needs to set one of these for normal KiCad use, either:
 * * They are working around some bug that should be fixed, or
 * * The parameter they are setting is of general interest and should be in the
 *   main application config, with UI provided.
 *
 * Options in this class are, in general, preferable to #defines, as they
 * allow more flexible configuration by developers, and don't hide code from
 * the compiler on other configurations, which can result in broken builds.
 *
 * Never use advanced configs in an untestable way. If a function depends on
 * advanced config such that you cannot test it without changing the config,
 * "lift" the config to a higher level and make pass it as parameter of the code
 * under test. The tests can pass their own values as needed.
 *
 * This also applies to code that does not depend on "common" - it cannot
 * use this class, so you must pass configuration in as proper parameters.
 *
 * Sometimes you can just use values directly, and sometimes helper functions
 * might be provided to allow extra logic (for example when a advanced config
 * applies only on certain platforms).
 *
 * For more information on what config keys set these parameters in the
 * config files, and why you might want to set them, see #AC_KEYS
 *
 */

class KICOMMON_API ADVANCED_CFG
{
public:
    /**
     * Get the singleton instance's config, which is shared by all consumers.
     *
     * This configuration is read-only - to set options, users should add the parameters to
     * their config files at ~/.config/kicad/advanced, or the platform equivalent.
     */
    static const ADVANCED_CFG& GetCfg();

    ///@{
    /// \ingroup advanced_config

    /**
     * Distance from an arc end point and the estimated end point, when rotating from the
     * start point to the end point.
     *
     * Setting name: "DrawArcAccuracy"
     * Valid values: 0 to 100000
     * Default value: 10
     */
    double m_DrawArcAccuracy;

    /**
     * When drawing an arc, the angle ( center - start ) - ( start - end ) can be limited to
     * avoid extremely high radii.
     *
     * Setting name: "DrawArcCenterStartEndMaxAngle"
     * Valid values: 0 to 100000
     * Default value: 50
     */
    double m_DrawArcCenterMaxAngle;

    /**
     * Maximum angle between the tangent line of an arc track and a connected straight track
     * in order to commence arc dragging. Units are degrees.
     *
     * Setting name: "MaxTangentTrackAngleDeviation"
     * Valid values: 0 to 90
     * Default value: 1
     */
    double m_MaxTangentAngleDeviation;

    /**
     * Maximum track length to keep after doing an arc track resizing operation. Units are mm.
     *
     * Setting name: "MaxTrackLengthToKeep"
     * Valid values: 0 to 1
     * Default value: 0.0005
     */
    double m_MaxTrackLengthToKeep;

    /**
     * When filling zones, we add an extra amount of clearance to each zone to ensure that
     * rounding errors do not overrun minimum clearance distances.
     *
     * This is the extra clearance in mm.
     *
     * Setting name: "ExtraFillMargin"
     * Valid values: 0 to 1
     * Default value: 0.0005
     */
    double m_ExtraClearance;

    /**
     * Enable the minimum slot width check for creepage
     *
     * Setting name: "EnableCreepageSlot"
     * Default value: false
     */
    bool m_EnableCreepageSlot;

    /**
     * Epsilon for DRC tests.
     *
     * @note Fo zone tests this is essentially additive with #m_ExtraClearance.  Units are mm.
     *
     * Setting name: "DRCEpsilon"
     * Valid values: 0 to 1
     * Default value: 0.0005
     */
    double m_DRCEpsilon;

    /**
     * Sliver width tolerance for DRC.
     *
     * Units are mm.
     *
     * Setting name: "DRCSliverWidthTolerance"
     * Valid values: 0.01 to 0.25
     * Default value: 0.08
     */
    double m_SliverWidthTolerance;

    /**
     * Sliver length tolerance for DRC.
     *
     * Units are mm.
     *
     * Setting name: "DRCSliverMinimumLength"
     * Valid values: 1e-9 to 10
     * Default value: 0.0008
     */
    double m_SliverMinimumLength;

    /**
     * Sliver angle to tolerance for DRC.
     *
     * Units are mm.
     *
     * Setting name: "DRCSliverAngleTolerance"
     * Valid values: 1 to 90
     * Default value: 20
     */
    double m_SliverAngleTolerance;


    /**
     * Dimension used to calculate the actual hole size from the finish hole size.
     *
     * @note IPC-6012 says 0.015-0.018mm; Cadence says at least 0.020mm for a Class 2 board and
     *       at least 0.025mm for Class 3.  Units are mm.
     *
     * Setting name: "HoleWallPlatingThickness"
     * Valid values: 1 to 90
     * Default value: 0.02
     */
    double m_HoleWallThickness;


    /**
     * Configure the coroutine stack size in bytes.
     *
     * @note This should be allocated in multiples of the system page size (n*4096 is generally
     *       safe)
     *
     * Setting name: "CoroutineStackSize"
     * Valid values: 32 * 4096 to 4096 * 4096
     * Default value: 256 * 4096
     */
    int m_CoroutineStackSize;

    /**
     * The update interval the wxWidgets sends wxUpdateUIEvents to windows.
     *
     * Setting this to -1 will disable all automatic UI events.  Any other
     * value is the number of milliseconds between events.
     *
     * @see https://docs.wxwidgets.org/3.0/classwx_update_u_i_event.html#a24daac56f682b866baac592e761ccede.
     *
     * Setting name: "UpdateUIEventInterval"
     * Valid values: -1 to 100000
     * Default value: 0
     */
    int m_UpdateUIEventInterval;

    /**
     * Show PNS router debug graphics while routing
     *
     * Setting name: "ShowRouterDebugGraphics"
     * Valid values: 0 or 1
     * Default value: 0
     */
    bool m_ShowRouterDebugGraphics;

    /**
     * Enable PNS router to dump state information for debug purpose (press `0` while routing)
     *
     * Setting name: "EnableRouterDump"
     * Valid values: 0 or 1
     * Default value: 0
     */
    bool m_EnableRouterDump;

    /**
     * Slide the zoom steps over for debugging things "up close".
     *
     * Setting name: "EnableRouterDump"
     * Valid values: 0 or 1
     * Default value: 0
     */
    bool m_HyperZoom;

    /**
     * Save files in compact display mode
     *
     * When set to true, this will wrap polygon point sets at 4 points per line rather
     * than a single point per line.  Single point per line helps with version control systems.
     *
     * Setting name: "CompactSave"
     * Valid values: 0 or 1
     * Default value: 0
     */
    bool m_CompactSave;

    /**
     * Enable drawing the triangulation outlines with a visible color.
     *
     * @note This only affects the OpenGL GAL.
     *
     * Setting name: "StrokeTriangulation"
     * Valid values: 0 or 1
     * Default value: 0
     */
    bool m_DrawTriangulationOutlines;

    /**
     * When true, adds zone-display-modes for stroking the zone fracture boundaries and the zone
     * triangulation.
     *
     * Setting name: "ExtraZoneDisplayModes"
     * Valid values: 0 or 1
     * Default value: 0
     */
    bool m_ExtraZoneDisplayModes;

    /**
     * Absolute minimum pen width for plotting.
     *
     * @note Some formats (PDF, for example) don't like ultra-thin lines.  PDF seems happy
     *       enough with 0.0212mm which equates to 1px @ 1200dpi.  Units are mm.
     *
     * Setting name: "MinPlotPenWidth"
     * Valid values: 0 to 1
     * Default value: 0.0212
     */
    double m_MinPlotPenWidth;

    /**
     * A mode that dumps the various stages of a F_Cu fill into In1_Cu through In9_Cu.
     *
     * Setting name: "DebugZoneFiller"
     * Valid values: 0 or 1
     * Default value: 0
     */
    bool m_DebugZoneFiller;

    /**
     * A mode that writes PDFs without compression.
     *
     * Setting name: "DebugPDFWriter"
     * Valid values: 0 or 1
     * Default value: 0
     */
    bool m_DebugPDFWriter;

    /**
     * The diameter of the drill marks on print and plot outputs (in mm) when the "Drill marks"
     * option is set to "Small mark".
     *
     * Setting name: "SmallDrillMarkSize"
     * Valid values: 0 to 3
     * Default value: 0.35
     */
    double m_SmallDrillMarkSize;

    /**
     * Enable the hotkeys dumper feature for generating documentation.
     *
     * Setting name: "HotkeysDumper"
     * Valid values: 0 or 1
     * Default value: 0
     */
    bool m_HotkeysDumper;

    /**
     * Draw GAL bounding boxes in painters.
     *
     * Setting name: "DrawBoundingBoxes"
     * Valid values: 0 or 1
     * Default value: 0
     */
    bool m_DrawBoundingBoxes;

    /**
     * Enable exporting board editor netlist to a file for troubleshooting purposes.
     *
     * Setting name: "ShowPcbnewExportNetlist"
     * Valid values: 0 or 1
     * Default value: 0
     */
    bool m_ShowPcbnewExportNetlist;

    /**
     * Skip reading/writing 3D model file caches.
     *
     * This does not prevent the models from being cached in memory meaning reopening the 3D
     * viewer in the same project session will not reload model data from disk again.
     *
     * Setting name: "Skip3DModelFileCache"
     * Valid values: 0 or 1
     * Default value: 0
     */
    bool m_Skip3DModelFileCache;

    /**
     * Skip reading/writing 3D model memory caches.
     &
     * This ensures 3D models are always reloaded from disk even if we previously opened the 3D
     * viewer.
     *
     * Setting name: "Skip3DModelMemoryCache"
     * Valid values: 0 or 1
     * Default value: 0
     */
    bool m_Skip3DModelMemoryCache;

    /**
     * Hide the build version from the KiCad manager frame title.
     *
     * Useful for making screenshots/videos of KiCad without pinning to a specific version.
     *
     * Setting name: "HideVersionFromTitle"
     * Valid values: 0 or 1
     * Default value: 0
     */
    bool m_HideVersionFromTitle;

    /**
     * Enable showing schematic repair output.
     *
     * Setting name: "ShowRepairSchematic"
     * Valid values: 0 or 1
     * Default value: 0
     */
    bool m_ShowRepairSchematic;

    /**
     * Shows debugging event counters in various places.
     *
     * Setting name: "ShowEventCounters"
     * Valid values: 0 or 1
     * Default value: 0
     */
    bool m_ShowEventCounters;

    /**
     * Allow manual scaling of canvas.
     *
     * Setting name: "AllowManualCanvasScale"
     * Valid values: 0 or 1
     * Default value: 0
     */
    bool m_AllowManualCanvasScale;

    /**
     * Set the bevel height of layer items in 3D viewer when ray tracing.
     *
     * Controls the start of curvature normal on the edge.  The value is in micrometer. Good
     * values should be around or less than the copper thickness.
     *
     * Setting name: "V3DRT_BevelHeight_um"
     * Valid values: 0 to std::numeric_limits<int>::max()
     * Default value: 30
     */
    int m_3DRT_BevelHeight_um;

    /**
     * 3D-Viewer raytracing factor applied to Extent.z of the item layer.
     *
     * This is used for calculating the bevel's height.
     *
     * Setting name: "V3DRT_BevelHeight_um"
     * Valid values: 0 to 100
     * Default value: 1/16
     */
    double m_3DRT_BevelExtentFactor;

    /**
     * Use Clipper2 instead of Clipper1.
     *
     * Setting name: "UseClipper2"
     * Valid values: 0 or 1
     * Default value: 1
     */
    bool m_UseClipper2;

    /**
     * Use the 3DConnexion Driver.
     *
     * Setting name: "3DConnexionDriver"
     * Valid values: 0 or 1
     * Default value: 1
     */
    bool m_Use3DConnexionDriver;

    /**
     * Use the new incremental netlister for realtime jobs.
     *
     * Setting name: "IncrementalConnectivity"
     * Valid values: 0 or 1
     * Default value: 1
     */
    bool m_IncrementalConnectivity;

    /**
     * The number of milliseconds to wait in a click before showing a disambiguation menu.
     *
     * Setting name: "DisambiguationTime"
     * Valid values: 50 or 10000
     * Default value: 300
     */
    int m_DisambiguationMenuDelay;

    /**
     * Enable the new Design Blocks feature
     *
     * Setting name: "EnableDesignBlocks"
     * Valid values: true or false
     * Default value: false
     */
    bool m_EnableDesignBlocks;

    /**
     * Enable support for generators.
     *
     * Setting name: "EnableGenerators"
     * Valid values: 0 or 1
     * Default value: 0
     */
    bool m_EnableGenerators;

    /**
     * Enable git integration.
     *
     * Setting name: "EnableGit"
     * Valid values: 0 or 1
     * Default value: 0
     */
    bool m_EnableGit;

    /**
     * Enable jobsets
     *
     * Setting name: "EnableJobset"
     * Valid values: 0 or 1
     * Default value: 0
     */
    bool m_EnableJobset;

    /**
     * Enable option to load lib files with text editor.
     *
     * Setting name: "EnableLibWithText"
     * Valid values: 0 or 1
     * Default value: 0
     */
    bool m_EnableLibWithText;

    /**
     * Enable option to open lib file directory.
     * Reveals one additional field under common preferences to set
     * system's file manager command in order for the context menu options to work.
     * On windows common settings preselect the default explorer with a hardcoded value.
     *
     * Examples,
     * Linux:  "nemo -n %F"
     *         "nautilus --browser %F"
     *         "dolphin --select %F" etc
     * Win11:  "explorer.exe /n,/select,%F"
     *
     * Setting name: "EnableLibDir"
     * Valid values: 0 or 1
     * Default value: 0
     */
    bool m_EnableLibDir;

    /**
     * Enable Eeschema printing using Cairo.
     *
     * Setting name: "EnableEeschemaPrintCairo"
     * Valid values: 0 or 1
     * Default value: 1
     */
    bool m_EnableEeschemaPrintCairo;

    /**
     * Board object selection visibility limit.
     *
     * This ratio is used to determine if an object in a selected object layer stack is
     * visible.  All alpha ratios less or equal to this value are considered invisible
     * to the user and will be pruned from the list of selections.  Valid values are
     * between 0 and less than 1. A value of 1 disables this feature.  Reasonable values
     * are between 0.01 and 0.03 depending on the layer colors.
     *
     * Setting name: "PcbSelectionVisibilityRatio"
     * Valid values: 0.0 to 1.0
     * Default value: 1
     */
    double m_PcbSelectionVisibilityRatio;

    /**
     * Deviation between font's bezier curve ideal and the poligonized curve.  This
     * is 1/16 of the font's internal units.
     *
     * Setting name: "FontErrorSize"
     * Valid values: 0.01 to 100
     * Default value: 2
     */
    double m_FontErrorSize;

    /**
     * OCE (STEP/IGES) 3D Plugin Tesselation Linear Deflection
     *
     * Linear deflection determines the maximum distance between the original geometry
     * and the tessellated representation, measured in millimeters (mm), influencing
     * the precision of flat surfaces.
     *
     * Setting name: "OcePluginLinearDeflection"
     * Valid values: 0.01 to 1.0
     * Default value: 0.14
     */
    double m_OcePluginLinearDeflection;

    /**
     * OCE (STEP/IGES) 3D Plugin Tesselation Angular Deflection
     *
     * Angular deflection specifies the maximum deviation angle (in degrees) between
     * the normals of adjacent facets in the tessellated model. Lower values result
     * in smoother curved surfaces by creating more facets to closely approximate
     * the curve.
     *
     * Setting name: "OcePluginAngularDeflection"
     * Valid values: 0.1 to 180
     * Default value: 30
     */
    double m_OcePluginAngularDeflection;

    /**
     * The number of internal units that will be allowed to deflect from the base
     * segment when creating a new segment.
     *
     * Setting name: "TriangulateSimplificationLevel"
     * Valid values: 0 to 1000
     * Default value: 50
    */
    int m_TriangulateSimplificationLevel;

    /**
     * The minimum area of a polygon that can be left over after triangulation and
     * still consider the triangulation successful.  This is internal units, so
     * it is square nm in pcbnew.
     *
     * Setting name: "TriangulateMinimumArea"
     * Valid values: 0 to 100000
     * Default value: 1000
     */
    int m_TriangulateMinimumArea;

    /**
     * Enable the use of a cache-friendlier and therefore faster version of the
     * polygon fracture algorithm.
     *
     * Setting name: "EnableCacheFriendlyFracture"
     * Valid values: 0 or 1
     * Default value: 1
     */
    bool m_EnableCacheFriendlyFracture;

    /**
     * Log IPC API requests and responses
     */
    bool m_EnableAPILogging;

    /**
     * Maximum number of filesystem watchers to use.
     *
     * Setting name: "MaxFilesystemWatchers"
     * Valid values: 0 to 2147483647
     * Default value: 16384
     */
    int m_MaxFilesystemWatchers;

    /**
     * Set the number of items in a schematic graph for it to be considered "minor"
     *
     * Setting name: "MinorSchematicGraphSize"
     * Valid values: 0 to 2147483647
     * Default value: 10000
     */
    int m_MinorSchematicGraphSize;

    /**
     * The number of recursions to resolve text variables.
     *
     * Setting name: "ResolveTextRecursionDepth"
     * Valid values: 0 to 10
     * Default value: 3
     */
    int m_ResolveTextRecursionDepth;

    /**
     * Enable snap anchors based on item line extensions.
     *
     * This should be removed when extension snaps are tuned up.
     *
     * Setting name: "EnableExtensionSnaps"
     * Default value: true
     */
    bool m_EnableExtensionSnaps;

    /**
     * If extension snaps are enabled, this is the timeout in milliseconds
     * before a hovered item gets extensions shown.
     *
     * This should be removed if a good value is agreed, or made configurable
     * if there's no universal good value.
     *
     * Setting name: "EnableExtensionSnapsMs"
     * Default value: 500
     * Valid values: >0
     */
    int m_ExtensionSnapTimeoutMs;

    /**
     * If extension snaps are enabled, 'activate' items on
     * hover, even if not near a snap point.
     *
     * This just to experiment with tuning.  It should either
     * be removed or made configurable when we know what feels best.
     *
     * Setting name: "ExtensionSnapActivateOnHover"
     * Default value: true
     */
    bool m_ExtensionSnapActivateOnHover;

    /**
     * Enable snap anchors debug visualization.
     *
     * Setting name: "EnableSnapAnchorsDebug"
     * Default value: false
     */
    bool m_EnableSnapAnchorsDebug;

    /**
     * When true, enable output to odb++
     */
    bool m_EnableODB;

    /**
     * Minimum overlapping angle for which an arc is considered to be parallel
     * to its paired arc.
     */
    double m_MinParallelAngle;

    /**
     * Enables the user-visible editing features for via stacks
     */
    bool m_EnableViaStacks;

///@}

private:
    ADVANCED_CFG();

    /**
     * Load the config from the normal config file
     */
    void loadFromConfigFile();

    /*
     * Load config from the given config base
     */
    void loadSettings( wxConfigBase& aCfg );
};
