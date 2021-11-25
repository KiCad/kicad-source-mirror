/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2019-2021 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <advanced_config.h>

#include <config_params.h>
#include <settings/settings_manager.h>

#include <wx/app.h>
#include <wx/config.h>
#include <wx/filename.h>
#include <wx/log.h>
#include <wx/tokenzr.h>

/*
 * Flag to enable advanced config debugging
 *
 * Use "KICAD_ADVANCED_CONFIG" to enable.
 *
 * @ingroup trace_env_vars
 */
static const wxChar AdvancedConfigMask[] = wxT( "KICAD_ADVANCED_CONFIG" );

/**
 * Limits and default settings for the coroutine stack size allowed.
 * Warning! Setting the stack size below the default may lead to unexplained crashes
 * This configuration setting is intended for developers only.
 */
namespace AC_STACK
{
    static constexpr int min_stack = 32 * 4096;
    static constexpr int default_stack = 256 * 4096;
    static constexpr int max_stack = 4096 * 4096;
}

/**
 * List of known keys for advanced configuration options.
 *
 * Set these options in the file `kicad_advanced` in the
 * KiCad config directory.
 */
namespace AC_KEYS
{

/**
 * When filling zones, we add an extra amount of clearance to each zone to ensure that rounding
 * errors do not overrun minimum clearance distances.  This is the extra in mm.
 */
static const wxChar ExtraFillMargin[] = wxT( "ExtraFillMargin" );

/**
 * A fudge factor for DRC.  Required to prevent false positives due to rounding errors, errors
 * in polygonalization, etc.
 * Previous versions hard-coded various values from 0.000005mm to 0.002mm.
 */
static const wxChar DRCEpsilon[] = wxT( "DRCEpsilon" );

/**
 * Used to calculate the actual hole size from the finish hole size.
 * IPC-6012 says 0.015-0.018mm; Cadence says at least 0.020mm for a Class 2 board and at least
 * 0.025mm for Class 3.
 */
static const wxChar HoleWallThickness[] = wxT( "HoleWallPlatingThickness" );

/**
 * Testing mode for new connectivity algorithm.  Setting this to on will cause all modifications
 * to the netlist to be recalculated on the fly.  This may be slower than the standard process
 * at the moment
 */
static const wxChar RealtimeConnectivity[] = wxT( "RealtimeConnectivity" );

/**
 * Configure the coroutine stack size in bytes.  This should be allocated in multiples of
 * the system page size (n*4096 is generally safe)
 */
static const wxChar CoroutineStackSize[] = wxT( "CoroutineStackSize" );

/**
 * Show PNS router debug graphics while routing
 */
static const wxChar ShowRouterDebugGraphics[] = wxT( "ShowRouterDebugGraphics" );

/**
 * When set to true, this will wrap polygon point sets at 4 points per line rather
 * than a single point per line.  Single point per line helps with version control systems
 */
static const wxChar CompactFileSave[] = wxT( "CompactSave" );

/**
 * For drawsegments - arcs.
 * Distance from an arc end point and the estimated end point,
 * when rotating from the start point to the end point.
 * 0 will not allow any approximate result, and the arc will not show.
 * Squared value for performances, in system unit.
 */
static const wxChar DrawArcAccuracy[] = wxT( "DrawArcAccuracy" );

/**
 * For drawsegments - arcs.
 * When drawing an arc, the angle ( center - start ) - ( start - end ),
 * can be limited to avoid extremely high radii.
 * The value is the tan( angle )
 */
static const wxChar DrawArcCenterStartEndMaxAngle[] = wxT( "DrawArcCenterStartEndMaxAngle" );

/**
 * For arc track interactive drag-resizing
 * Maximum angle between the tangent line of an arc track and a connected straight track
 * in order to commence arc dragging. Units are degrees.
 */
static const wxChar MaxTangentTrackAngleDeviation[] = wxT( "MaxTangentTrackAngleDeviation" );

/**
 * For arc track interactive drag-resizing
 * Maximum track length to keep after doing an arc track resizing operation. Units are mm.
 */
static const wxChar MaxTrackLengthToKeep[] = wxT( "MaxTrackLengthToKeep" );

/**
 * When true, GAL will stroke the triangulations (only used in OpenGL) with a visible color
 */
static const wxChar StrokeTriangulation[] = wxT( "StrokeTriangulation" );

/**
 * When true, a third zone-display-mode is included which strokes the filled areas and fracture
 * boundaries.
 */
static const wxChar ExtraZoneDisplayModes[] = wxT( "ExtraZoneDisplayModes" );

/**
 * Absolute minimum pen width to send to the plotter.  PDF seems happy enough with 0.0212mm
 * (which equates to 1px @ 1200dpi).
 */
static const wxChar MinPlotPenWidth[] = wxT( "MinPlotPenWidth" );

static const wxChar DebugZoneFiller[] = wxT( "DebugZoneFiller" );

static const wxChar DebugPDFWriter[] = wxT( "DebugPDFWriter" );

/**
 * The diameter of the drill marks on print and plot outputs (in mm),
 * when the "Drill marks" option is set to "Small mark"
 */
static const wxChar SmallDrillMarkSize[] = wxT( "SmallDrillMarkSize" );

static const wxChar HotkeysDumper[] = wxT( "HotkeysDumper" );

static const wxChar DrawBoundingBoxes[] = wxT( "DrawBoundingBoxes" );

static const wxChar ShowPcbnewExportNetlist[] = wxT( "ShowPcbnewExportNetlist" );

static const wxChar Skip3DModelFileCache[] = wxT( "Skip3DModelFileCache" );

static const wxChar Skip3DModelMemoryCache[] = wxT( "Skip3DModelMemoryCache" );

static const wxChar HideVersionFromTitle[] = wxT( "HideVersionFromTitle" );

static const wxChar TraceMasks[] = wxT( "TraceMasks" );

static const wxChar ShowRepairSchematic[] = wxT( "ShowRepairSchematic" );

} // namespace KEYS


/*
 * Get a simple string for common parameters.
 *
 * This isn't exhaustive, but it covers most common types that might be
 * used in the advance config
 */
wxString dumpParamCfg( const PARAM_CFG& aParam )
{
    wxString s = aParam.m_Ident + ": ";

    /*
     * This implementation is rather simplistic, but it is
     * effective enough for simple uses. A better implementation would be
     * some kind of visitor, but that's somewhat more work.
     */
    switch( aParam.m_Type )
    {
    case paramcfg_id::PARAM_INT:
    case paramcfg_id::PARAM_INT_WITH_SCALE:
        s << *static_cast<const PARAM_CFG_INT&>( aParam ).m_Pt_param;
        break;
    case paramcfg_id::PARAM_DOUBLE:
        s << *static_cast<const PARAM_CFG_DOUBLE&>( aParam ).m_Pt_param;
        break;
    case paramcfg_id::PARAM_WXSTRING:
        s << *static_cast<const PARAM_CFG_WXSTRING&>( aParam ).m_Pt_param;
        break;
    case paramcfg_id::PARAM_FILENAME:
        s << *static_cast<const PARAM_CFG_FILENAME&>( aParam ).m_Pt_param;
        break;
    case paramcfg_id::PARAM_BOOL:
        s << ( *static_cast<const PARAM_CFG_BOOL&>( aParam ).m_Pt_param ? "true" : "false" );
        break;
    default: s << "Unsupported PARAM_CFG variant: " << aParam.m_Type;
    }

    return s;
}


/**
 * Dump the configs in the given array to trace.
 */
static void dumpCfg( const std::vector<PARAM_CFG*>& aArray )
{
    // only dump if we need to
    if( !wxLog::IsAllowedTraceMask( AdvancedConfigMask ) )
        return;

    for( const PARAM_CFG* param : aArray )
    {
        wxLogTrace( AdvancedConfigMask, dumpParamCfg( *param ) );
    }
}


/**
 * Get the filename for the advanced config file
 *
 * The user must check the file exists if they care.
 */
static wxFileName getAdvancedCfgFilename()
{
    const static wxString cfg_filename{ "kicad_advanced" };
    return wxFileName( SETTINGS_MANAGER::GetUserSettingsPath(), cfg_filename );
}


ADVANCED_CFG::ADVANCED_CFG()
{
    wxLogTrace( AdvancedConfigMask, "Init advanced config" );

    // Init defaults - this is done in case the config doesn't exist,
    // then the values will remain as set here.
    m_RealTimeConnectivity      = true;
    m_CoroutineStackSize        = AC_STACK::default_stack;
    m_ShowRouterDebugGraphics   = false;
    m_DrawArcAccuracy           = 10.0;
    m_DrawArcCenterMaxAngle     = 50.0;
    m_MaxTangentAngleDeviation  = 1.0;
    m_MaxTrackLengthToKeep      = 0.0001;
    m_ExtraZoneDisplayModes     = false;
    m_DrawTriangulationOutlines = false;

    m_ExtraClearance            = 0.0001;
    m_DRCEpsilon                = 0.0001;   // 0.1um is small enough not to materially violate
                                            // any constraints.

    m_HoleWallThickness         = 0.020;    // IPC-6012 says 15-18um; Cadence says at least
                                            // 0.020 for a Class 2 board and at least 0.025
                                            // for Class 3.

    m_MinPlotPenWidth           = 0.0212;   // 1 pixel at 1200dpi.

    m_DebugZoneFiller           = false;
    m_DebugPDFWriter            = false;
    m_SmallDrillMarkSize        = 0.35;
    m_HotkeysDumper             = false;
    m_DrawBoundingBoxes         = false;
    m_ShowPcbnewExportNetlist   = false;
    m_Skip3DModelFileCache      = false;
    m_Skip3DModelMemoryCache    = false;
    m_HideVersionFromTitle      = false;

    loadFromConfigFile();
}


const ADVANCED_CFG& ADVANCED_CFG::GetCfg()
{
    static ADVANCED_CFG instance;
    return instance;
}


void ADVANCED_CFG::loadFromConfigFile()
{
    const wxFileName k_advanced = getAdvancedCfgFilename();

    // If we are running headless, use the class defaults because we cannot instantiate wxConfig
    if( !wxTheApp )
        return;

    if( !k_advanced.FileExists() )
    {
        wxLogTrace( AdvancedConfigMask, "File does not exist %s", k_advanced.GetFullPath() );

        // load the defaults
        wxConfig emptyConfig;
        loadSettings( emptyConfig );

        return;
    }

    wxLogTrace( AdvancedConfigMask, "Loading advanced config from: %s", k_advanced.GetFullPath() );

    wxFileConfig file_cfg( "", "", k_advanced.GetFullPath() );
    loadSettings( file_cfg );
}


void ADVANCED_CFG::loadSettings( wxConfigBase& aCfg )
{
    std::vector<PARAM_CFG*> configParams;

    configParams.push_back( new PARAM_CFG_BOOL( true, AC_KEYS::RealtimeConnectivity,
                                                &m_RealTimeConnectivity, true ) );

    configParams.push_back( new PARAM_CFG_DOUBLE( true, AC_KEYS::ExtraFillMargin,
                                                  &m_ExtraClearance, 0.0005, 0.0, 1.0 ) );

    configParams.push_back( new PARAM_CFG_DOUBLE( true, AC_KEYS::DRCEpsilon,
                                                  &m_DRCEpsilon, 0.0005, 0.0, 1.0 ) );

    configParams.push_back( new PARAM_CFG_DOUBLE( true, AC_KEYS::HoleWallThickness,
                                                  &m_HoleWallThickness, 0.020, 0.0, 1.0 ) );

    configParams.push_back( new PARAM_CFG_INT( true, AC_KEYS::CoroutineStackSize,
                                               &m_CoroutineStackSize, AC_STACK::default_stack,
                                               AC_STACK::min_stack, AC_STACK::max_stack ) );

    configParams.push_back( new PARAM_CFG_BOOL( true, AC_KEYS::ShowRouterDebugGraphics,
                                                &m_ShowRouterDebugGraphics, false ) );

    configParams.push_back( new PARAM_CFG_BOOL( true, AC_KEYS::CompactFileSave,
                                                &m_CompactSave, false ) );

    configParams.push_back( new PARAM_CFG_DOUBLE( true, AC_KEYS::DrawArcAccuracy,
                                                  &m_DrawArcAccuracy, 10.0, 0.0, 100000.0 ) );

    configParams.push_back( new PARAM_CFG_DOUBLE( true, AC_KEYS::DrawArcCenterStartEndMaxAngle,
                                                  &m_DrawArcCenterMaxAngle, 50.0, 0.0, 100000.0 ) );

    configParams.push_back( new PARAM_CFG_DOUBLE( true, AC_KEYS::MaxTangentTrackAngleDeviation,
                                                  &m_MaxTangentAngleDeviation, 1.0, 0.0, 90.0 ) );

    configParams.push_back( new PARAM_CFG_DOUBLE( true, AC_KEYS::MaxTrackLengthToKeep,
                                                  &m_MaxTrackLengthToKeep, 0.0005, 0.0, 1.0 ) );

    configParams.push_back( new PARAM_CFG_BOOL( true, AC_KEYS::ExtraZoneDisplayModes,
                                                &m_ExtraZoneDisplayModes, false ) );

    configParams.push_back( new PARAM_CFG_BOOL( true, AC_KEYS::StrokeTriangulation,
                                                &m_DrawTriangulationOutlines, false ) );

    configParams.push_back( new PARAM_CFG_DOUBLE( true, AC_KEYS::MinPlotPenWidth,
                                                  &m_MinPlotPenWidth, 0.0212, 0.0, 1.0 ) );

    configParams.push_back( new PARAM_CFG_BOOL( true, AC_KEYS::DebugZoneFiller,
                                                &m_DebugZoneFiller, false ) );

    configParams.push_back( new PARAM_CFG_BOOL( true, AC_KEYS::DebugPDFWriter,
                                                &m_DebugPDFWriter, false ) );

    configParams.push_back( new PARAM_CFG_DOUBLE( true, AC_KEYS::SmallDrillMarkSize,
                                                  &m_SmallDrillMarkSize, 0.35, 0.0, 3.0 ) );

    configParams.push_back( new PARAM_CFG_BOOL( true, AC_KEYS::HotkeysDumper,
                                                &m_HotkeysDumper, false ) );

    configParams.push_back( new PARAM_CFG_BOOL( true, AC_KEYS::DrawBoundingBoxes,
                                                &m_DrawBoundingBoxes, false ) );

    configParams.push_back( new PARAM_CFG_BOOL( true, AC_KEYS::ShowPcbnewExportNetlist,
                                                &m_ShowPcbnewExportNetlist, false ) );

    configParams.push_back( new PARAM_CFG_BOOL( true, AC_KEYS::Skip3DModelFileCache,
                                                &m_Skip3DModelFileCache, false ) );

    configParams.push_back( new PARAM_CFG_BOOL( true, AC_KEYS::Skip3DModelMemoryCache,
                                                &m_Skip3DModelMemoryCache, false ) );

    configParams.push_back( new PARAM_CFG_BOOL( true, AC_KEYS::HideVersionFromTitle,
                                                &m_HideVersionFromTitle, false ) );

    configParams.push_back( new PARAM_CFG_BOOL( true, AC_KEYS::ShowRepairSchematic,
                                                &m_ShowRepairSchematic, false ) );

    // Special case for trace mask setting...we just grab them and set them immediately
    // Because we even use wxLogTrace inside of advanced config
    wxString traceMasks = "";
    configParams.push_back( new PARAM_CFG_WXSTRING( true, AC_KEYS::TraceMasks, &traceMasks, "" ) );

    // Load the config from file
    wxConfigLoadSetups( &aCfg, configParams );

    // Now actually set the trace masks
    wxStringTokenizer traceMaskTokenizer( traceMasks, "," );
    while( traceMaskTokenizer.HasMoreTokens() )
    {
        wxString mask = traceMaskTokenizer.GetNextToken();
        wxLog::AddTraceMask( mask );
    }

    dumpCfg( configParams );

    for( PARAM_CFG* param : configParams )
        delete param;
}


