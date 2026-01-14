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
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#include <advanced_config.h>
#include <trace_helpers.h>
#include <config_params.h>
#include <paths.h>

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
} // namespace AC_STACK

/**
 * List of known keys for advanced configuration options.
 *
 * Set these options in the file `kicad_advanced` in the KiCad configuration directory.
 */
namespace AC_KEYS
{

static const wxChar IncrementalConnectivity[] = wxT( "IncrementalConnectivity" );
static const wxChar Use3DConnexionDriver[] = wxT( "3DConnexionDriver" );
static const wxChar ExtraFillMargin[] = wxT( "ExtraFillMargin" );
static const wxChar EnableCreepageSlot[] = wxT( "EnableCreepageSlot" );
static const wxChar DRCEpsilon[] = wxT( "DRCEpsilon" );
static const wxChar DRCSliverWidthTolerance[] = wxT( "DRCSliverWidthTolerance" );
static const wxChar DRCSliverMinimumLength[] = wxT( "DRCSliverMinimumLength" );
static const wxChar DRCSliverAngleTolerance[] = wxT( "DRCSliverAngleTolerance" );
static const wxChar HoleWallThickness[] = wxT( "HoleWallPlatingThickness" );
static const wxChar CoroutineStackSize[] = wxT( "CoroutineStackSize" );
static const wxChar ShowRouterDebugGraphics[] = wxT( "ShowRouterDebugGraphics" );
static const wxChar EnableRouterDump[] = wxT( "EnableRouterDump" );
static const wxChar HyperZoom[] = wxT( "HyperZoom" );
static const wxChar CompactFileSave[] = wxT( "CompactSave" );
static const wxChar DrawArcAccuracy[] = wxT( "DrawArcAccuracy" );
static const wxChar DrawArcCenterStartEndMaxAngle[] = wxT( "DrawArcCenterStartEndMaxAngle" );
static const wxChar MaxTangentTrackAngleDeviation[] = wxT( "MaxTangentTrackAngleDeviation" );
static const wxChar MaxTrackLengthToKeep[] = wxT( "MaxTrackLengthToKeep" );
static const wxChar StrokeTriangulation[] = wxT( "StrokeTriangulation" );
static const wxChar ExtraZoneDisplayModes[] = wxT( "ExtraZoneDisplayModes" );
static const wxChar MinPlotPenWidth[] = wxT( "MinPlotPenWidth" );
static const wxChar DebugZoneFiller[] = wxT( "DebugZoneFiller" );
static const wxChar DebugPDFWriter[] = wxT( "DebugPDFWriter" );
static const wxChar PDFStrokeFontWidthFactor[] = wxT( "PDFStrokeFontWidthFactor" );
static const wxChar PDFStrokeFontXOffset[] = wxT( "PDFStrokeFontXOffset" );
static const wxChar PDFStrokeFontYOffset[] = wxT( "PDFStrokeFontYOffset" );
static const wxChar PDFStrokeFontBoldMultiplier[] = wxT( "PDFStrokeFontBoldMultiplier" );
static const wxChar PDFStrokeFontKerningFactor[] = wxT( "PDFStrokeFontKerningFactor" );
static const wxChar UsePdfPrint[] = wxT( "UsePdfPrint" );
static const wxChar SmallDrillMarkSize[] = wxT( "SmallDrillMarkSize" );
static const wxChar HotkeysDumper[] = wxT( "HotkeysDumper" );
static const wxChar DrawBoundingBoxes[] = wxT( "DrawBoundingBoxes" );
static const wxChar ShowPcbnewExportNetlist[] = wxT( "ShowPcbnewExportNetlist" );
static const wxChar Skip3DModelFileCache[] = wxT( "Skip3DModelFileCache" );
static const wxChar Skip3DModelMemoryCache[] = wxT( "Skip3DModelMemoryCache" );
static const wxChar HideVersionFromTitle[] = wxT( "HideVersionFromTitle" );
static const wxChar TraceMasks[] = wxT( "TraceMasks" );
static const wxChar ShowRepairSchematic[] = wxT( "ShowRepairSchematic" );
static const wxChar ShowEventCounters[] = wxT( "ShowEventCounters" );
static const wxChar AllowManualCanvasScale[] = wxT( "AllowManualCanvasScale" );
static const wxChar UpdateUIEventInterval[] = wxT( "UpdateUIEventInterval" );
static const wxChar V3DRT_BevelHeight_um[] = wxT( "V3DRT_BevelHeight_um" );
static const wxChar V3DRT_BevelExtentFactor[] = wxT( "V3DRT_BevelExtentFactor" );
static const wxChar EnablePcbDesignBlocks[] = wxT( "EnablePcbDesignBlocks" );
static const wxChar EnableGenerators[] = wxT( "EnableGenerators" );
static const wxChar EnableDrcRuleEditor[] = wxT( "EnableDrcRuleEditor" );
static const wxChar EnableLibWithText[] = wxT( "EnableLibWithText" );
static const wxChar EnableLibDir[] = wxT( "EnableLibDir" );
static const wxChar DisambiguationTime[] = wxT( "DisambiguationTime" );
static const wxChar PcbSelectionVisibilityRatio[] = wxT( "PcbSelectionVisibilityRatio" );
static const wxChar FontErrorSize[] = wxT( "FontErrorSize" );
static const wxChar OcePluginLinearDeflection[] = wxT( "OcePluginLinearDeflection" );
static const wxChar OcePluginAngularDeflection[] = wxT( "OcePluginAngularDeflection" );
static const wxChar TriangulateSimplificationLevel[] = wxT( "TriangulateSimplificationLevel" );
static const wxChar TriangulateMinimumArea[] = wxT( "TriangulateMinimumArea" );
static const wxChar EnableCacheFriendlyFracture[] = wxT( "EnableCacheFriendlyFracture" );
static const wxChar EnableAPILogging[] = wxT( "EnableAPILogging" );
static const wxChar MaxFileSystemWatchers[] = wxT( "MaxFileSystemWatchers" );
static const wxChar MinorSchematicGraphSize[] = wxT( "MinorSchematicGraphSize" );
static const wxChar ResolveTextRecursionDepth[] = wxT( "ResolveTextRecursionDepth" );
static const wxChar EnableExtensionSnaps[] = wxT( "EnableExtensionSnaps" );
static const wxChar ExtensionSnapTimeoutMs[] = wxT( "ExtensionSnapTimeoutMs" );
static const wxChar ExtensionSnapActivateOnHover[] = wxT( "ExtensionSnapActivateOnHover" );
static const wxChar EnableSnapAnchorsDebug[] = wxT( "EnableSnapAnchorsDebug" );
static const wxChar SnapHysteresis[] = wxT( "SnapHysteresis" );
static const wxChar SnapToAnchorMargin[] = wxT( "SnapToAnchorMargin" );
static const wxChar MinParallelAngle[] = wxT( "MinParallelAngle" );
static const wxChar HoleWallPaintingMultiplier[] = wxT( "HoleWallPaintingMultiplier" );
static const wxChar MsgPanelShowUuids[] = wxT( "MsgPanelShowUuids" );
static const wxChar MaximumThreads[] = wxT( "MaximumThreads" );
static const wxChar NetInspectorBulkUpdateOptimisationThreshold[] =
        wxT( "NetInspectorBulkUpdateOptimisationThreshold" );
static const wxChar ExcludeFromSimulationLineWidth[] = wxT( "ExcludeFromSimulationLineWidth" );
static const wxChar SimulatorMultiRunCombinationLimit[] = wxT( "SimulatorMultiRunCombinationLimit" );
static const wxChar GitIconRefreshInterval[] = wxT( "GitIconRefreshInterval" );
static const wxChar MaxPastedTextLength[] = wxT( "MaxPastedTextLength" );
static const wxChar PNSProcessClusterTimeout[] = wxT( "PNSProcessClusterTimeout" );
static const wxChar FollowBranchTimeout[] = wxT( "FollowBranchTimeoutMs" );
static const wxChar ImportSkipComponentBodies[] = wxT( "ImportSkipComponentBodies" );
static const wxChar ScreenDPI[] = wxT( "ScreenDPI" );
static const wxChar EnableUseAuiPerspective[] = wxT( "EnableUseAuiPerspective" );
static const wxChar HistoryLockStaleTimeout[] = wxT( "HistoryLockStaleTimeout" );
static const wxChar ZoneFillIterativeRefill[] = wxT( "ZoneFillIterativeRefill" );

} // namespace AC_KEYS


/**
 * List of known groups for advanced configuration options.
 *
 */
namespace AC_GROUPS
{
static const wxChar V3D_RayTracing[] = wxT( "G_3DV_RayTracing" );
}

/*
 * Get a simple string for common parameters.
 *
 * This isn't exhaustive, but it covers most common types that might be
 * used in the advance config
 */
wxString dumpParamCfg( const PARAM_CFG& aParam )
{
    wxString s = aParam.m_Ident + wxS( ": " );

    /*
     * This implementation is rather simplistic, but it is
     * effective enough for simple uses. A better implementation would be
     * some kind of visitor, but that's somewhat more work.
     */
    switch( aParam.m_Type )
    {
    case paramcfg_id::PARAM_INT:
    case paramcfg_id::PARAM_INT_WITH_SCALE: s << *static_cast<const PARAM_CFG_INT&>( aParam ).m_Pt_param; break;
    case paramcfg_id::PARAM_DOUBLE: s << *static_cast<const PARAM_CFG_DOUBLE&>( aParam ).m_Pt_param; break;
    case paramcfg_id::PARAM_WXSTRING: s << *static_cast<const PARAM_CFG_WXSTRING&>( aParam ).m_Pt_param; break;
    case paramcfg_id::PARAM_FILENAME: s << *static_cast<const PARAM_CFG_FILENAME&>( aParam ).m_Pt_param; break;
    case paramcfg_id::PARAM_BOOL:
        s << ( *static_cast<const PARAM_CFG_BOOL&>( aParam ).m_Pt_param ? wxS( "true" ) : wxS( "false" ) );
        break;
    default: s << wxS( "Unsupported PARAM_CFG variant: " ) << aParam.m_Type;
    }

    return s;
}


/**
 * Dump the configs in the given array to trace.
 */
static void dumpCfg( const std::vector<std::unique_ptr<PARAM_CFG>>& aArray )
{
    // only dump if we need to
    if( !wxLog::IsAllowedTraceMask( AdvancedConfigMask ) )
        return;

    for( const auto& param : aArray )
    {
        wxLogTrace( AdvancedConfigMask, dumpParamCfg( *param ) );
    }
}


/**
 * Get the filename for the advanced config file.
 *
 * The user must check the file exists if they care.
 */
static wxFileName getAdvancedCfgFilename()
{
    const static wxString cfg_filename{ wxS( "kicad_advanced" ) };
    return wxFileName( PATHS::GetUserSettingsPath(), cfg_filename );
}


ADVANCED_CFG::ADVANCED_CFG()
{
    wxLogTrace( AdvancedConfigMask, wxS( "Init advanced config" ) );

    // Init defaults - this is done in case the config doesn't exist,
    // then the values will remain as set here.
    m_CoroutineStackSize = AC_STACK::default_stack;
    m_ShowRouterDebugGraphics = false;
    m_EnableRouterDump = false;
    m_HyperZoom = false;
    m_DrawArcAccuracy = 10.0;
    m_DrawArcCenterMaxAngle = 50.0;
    m_MaxTangentAngleDeviation = 1.0;
    m_MaxTrackLengthToKeep = 0.0005;
    m_ExtraZoneDisplayModes = false;
    m_DrawTriangulationOutlines = false;

    m_ExtraClearance = 0.0005;
    m_EnableCreepageSlot = false;
    m_DRCEpsilon = 0.0005; // 0.5um is small enough not to materially violate
                           // any constraints.
    m_SliverWidthTolerance = 0.08;
    m_SliverMinimumLength = 0.0008;
    m_SliverAngleTolerance = 20.0;

    m_HoleWallThickness = 0.020; // IPC-6012 says 15-18um; Cadence says at least
                                 // 0.020 for a Class 2 board and at least 0.025
                                 // for Class 3.

    m_MinPlotPenWidth = 0.0212; // 1 pixel at 1200dpi.

    m_DebugZoneFiller = false;
    m_DebugPDFWriter = false;
    m_PDFStrokeFontWidthFactor = .12; // default 12% of EM
    m_PDFStrokeFontXOffset = 0.1;
    m_PDFStrokeFontYOffset = 0.35;
    m_PDFStrokeFontBoldMultiplier = 1.8;
    m_PDFStrokeFontKerningFactor = 1.0;
    m_UsePdfPrint = false;
    m_SmallDrillMarkSize = 0.35;
    m_HotkeysDumper = false;
    m_DrawBoundingBoxes = false;
    m_MsgPanelShowUuids = 0;
    m_ShowPcbnewExportNetlist = false;
    m_Skip3DModelFileCache = false;
    m_Skip3DModelMemoryCache = false;
    m_HideVersionFromTitle = false;
    m_ShowEventCounters = false;
    m_AllowManualCanvasScale = false;
    m_CompactSave = false;
    m_UpdateUIEventInterval = 0;
    m_ShowRepairSchematic = false;
    m_EnablePcbDesignBlocks = true;
    m_EnableGenerators = false;
    m_EnableDrcRuleEditor = false;
    m_EnableLibWithText = false;
    m_EnableLibDir = false;

    m_3DRT_BevelHeight_um = 30;
    m_3DRT_BevelExtentFactor = 1.0 / 16.0;

    m_EnableAPILogging = false;

    m_Use3DConnexionDriver = true;

    m_IncrementalConnectivity = true;

    m_DisambiguationMenuDelay = 500;

    m_PcbSelectionVisibilityRatio = 1.0;

    m_FontErrorSize = 2;

    m_OcePluginLinearDeflection = 0.14;
    m_OcePluginAngularDeflection = 30;

    m_TriangulateSimplificationLevel = 50;
    m_TriangulateMinimumArea = 1000;

    m_EnableCacheFriendlyFracture = true;

    m_MaxFilesystemWatchers = 16384;

    m_MinorSchematicGraphSize = 10000;

    m_ResolveTextRecursionDepth = 6;

    m_EnableExtensionSnaps = true;
    m_ExtensionSnapTimeoutMs = 500;
    m_ExtensionSnapActivateOnHover = true;
    m_EnableSnapAnchorsDebug = false;
    m_SnapHysteresis = 5;
    m_SnapToAnchorMargin = 1.1;

    m_MinParallelAngle = 0.001;
    m_HoleWallPaintingMultiplier = 1.5;

    m_MaximumThreads = 0;

    m_NetInspectorBulkUpdateOptimisationThreshold = 100;

    m_ExcludeFromSimulationLineWidth = 25;
    m_SimulatorMultiRunCombinationLimit = 12;

    m_GitIconRefreshInterval = 10000;

    m_MaxPastedTextLength = 100;

    m_PNSProcessClusterTimeout = 100; // Default: 100 ms
    m_FollowBranchTimeout = 500; // Default: 500 ms

    m_ImportSkipComponentBodies = false;

    m_ScreenDPI = 91;

    m_EnableUseAuiPerspective = false;
    m_HistoryLockStaleTimeout = 300; // 5 minutes default
    m_ZoneFillIterativeRefill = false;

    loadFromConfigFile();
}


const ADVANCED_CFG& ADVANCED_CFG::GetCfg()
{
    static ADVANCED_CFG instance;
    return instance;
}


void ADVANCED_CFG::Reload()
{
    loadFromConfigFile();
}


void ADVANCED_CFG::Save()
{
    wxFileName   k_advanced = getAdvancedCfgFilename();
    wxFileConfig file_cfg( wxS( "" ), wxS( "" ), k_advanced.GetFullPath() );

    wxConfigSaveSetups( &file_cfg, m_entries );
}


void ADVANCED_CFG::loadFromConfigFile()
{
    const wxFileName k_advanced = getAdvancedCfgFilename();

    // If we are running headless, use the class defaults because we cannot instantiate wxConfig
    if( !wxTheApp )
        return;

    if( !k_advanced.FileExists() )
    {
        wxLogTrace( AdvancedConfigMask, wxS( "File does not exist %s" ), k_advanced.GetFullPath() );

        // load the defaults
        wxConfig emptyConfig;
        loadSettings( emptyConfig );

        return;
    }

    wxLogTrace( AdvancedConfigMask, wxS( "Loading advanced config from: %s" ), k_advanced.GetFullPath() );

    wxFileConfig file_cfg( wxS( "" ), wxS( "" ), k_advanced.GetFullPath() );
    loadSettings( file_cfg );
}


void ADVANCED_CFG::loadSettings( wxConfigBase& aCfg )
{
    m_entries.clear();

    m_entries.push_back( std::make_unique<PARAM_CFG_DOUBLE>( true, AC_KEYS::ExtraFillMargin, &m_ExtraClearance,
                                                             m_ExtraClearance, 0.0, 1.0 ) );

    m_entries.push_back( std::make_unique<PARAM_CFG_BOOL>( true, AC_KEYS::EnableCreepageSlot, &m_EnableCreepageSlot,
                                                           m_EnableCreepageSlot ) );

    m_entries.push_back(
            std::make_unique<PARAM_CFG_DOUBLE>( true, AC_KEYS::DRCEpsilon, &m_DRCEpsilon, m_DRCEpsilon, 0.0, 1.0 ) );

    m_entries.push_back( std::make_unique<PARAM_CFG_DOUBLE>(
            true, AC_KEYS::DRCSliverWidthTolerance, &m_SliverWidthTolerance, m_SliverWidthTolerance, 0.01, 0.25 ) );

    m_entries.push_back( std::make_unique<PARAM_CFG_DOUBLE>(
            true, AC_KEYS::DRCSliverMinimumLength, &m_SliverMinimumLength, m_SliverMinimumLength, 1e-9, 10 ) );

    m_entries.push_back( std::make_unique<PARAM_CFG_DOUBLE>(
            true, AC_KEYS::DRCSliverAngleTolerance, &m_SliverAngleTolerance, m_SliverAngleTolerance, 1.0, 90.0 ) );

    m_entries.push_back( std::make_unique<PARAM_CFG_DOUBLE>( true, AC_KEYS::HoleWallThickness, &m_HoleWallThickness,
                                                             m_HoleWallThickness, 0.0, 1.0 ) );

    m_entries.push_back( std::make_unique<PARAM_CFG_INT>( true, AC_KEYS::CoroutineStackSize, &m_CoroutineStackSize,
                                                          AC_STACK::default_stack, AC_STACK::min_stack,
                                                          AC_STACK::max_stack ) );

    m_entries.push_back( std::make_unique<PARAM_CFG_INT>(
            true, AC_KEYS::UpdateUIEventInterval, &m_UpdateUIEventInterval, m_UpdateUIEventInterval, -1, 100000 ) );

    m_entries.push_back( std::make_unique<PARAM_CFG_BOOL>( true, AC_KEYS::ShowRouterDebugGraphics,
                                                           &m_ShowRouterDebugGraphics, m_ShowRouterDebugGraphics ) );

    m_entries.push_back( std::make_unique<PARAM_CFG_BOOL>( true, AC_KEYS::EnableRouterDump, &m_EnableRouterDump,
                                                           m_EnableRouterDump ) );

    m_entries.push_back( std::make_unique<PARAM_CFG_BOOL>( true, AC_KEYS::HyperZoom, &m_HyperZoom, m_HyperZoom ) );

    m_entries.push_back(
            std::make_unique<PARAM_CFG_BOOL>( true, AC_KEYS::CompactFileSave, &m_CompactSave, m_CompactSave ) );

    m_entries.push_back( std::make_unique<PARAM_CFG_DOUBLE>( true, AC_KEYS::DrawArcAccuracy, &m_DrawArcAccuracy,
                                                             m_DrawArcAccuracy, 0.0, 100000.0 ) );

    m_entries.push_back( std::make_unique<PARAM_CFG_DOUBLE>( true, AC_KEYS::DrawArcCenterStartEndMaxAngle,
                                                             &m_DrawArcCenterMaxAngle, m_DrawArcCenterMaxAngle, 0.0,
                                                             100000.0 ) );

    m_entries.push_back( std::make_unique<PARAM_CFG_DOUBLE>( true, AC_KEYS::MaxTangentTrackAngleDeviation,
                                                             &m_MaxTangentAngleDeviation, m_MaxTangentAngleDeviation,
                                                             0.0, 90.0 ) );

    m_entries.push_back( std::make_unique<PARAM_CFG_DOUBLE>(
            true, AC_KEYS::MaxTrackLengthToKeep, &m_MaxTrackLengthToKeep, m_MaxTrackLengthToKeep, 0.0, 1.0 ) );

    m_entries.push_back( std::make_unique<PARAM_CFG_BOOL>( true, AC_KEYS::ExtraZoneDisplayModes,
                                                           &m_ExtraZoneDisplayModes, m_ExtraZoneDisplayModes ) );

    m_entries.push_back( std::make_unique<PARAM_CFG_BOOL>(
            true, AC_KEYS::StrokeTriangulation, &m_DrawTriangulationOutlines, m_DrawTriangulationOutlines ) );

    m_entries.push_back( std::make_unique<PARAM_CFG_DOUBLE>( true, AC_KEYS::MinPlotPenWidth, &m_MinPlotPenWidth,
                                                             m_MinPlotPenWidth, 0.0, 1.0 ) );

    m_entries.push_back(
            std::make_unique<PARAM_CFG_BOOL>( true, AC_KEYS::DebugZoneFiller, &m_DebugZoneFiller, m_DebugZoneFiller ) );

    m_entries.push_back(
            std::make_unique<PARAM_CFG_BOOL>( true, AC_KEYS::DebugPDFWriter, &m_DebugPDFWriter, m_DebugPDFWriter ) );

    m_entries.push_back( std::make_unique<PARAM_CFG_DOUBLE>(
            true, AC_KEYS::PDFStrokeFontWidthFactor, &m_PDFStrokeFontWidthFactor, m_PDFStrokeFontWidthFactor ) );

    m_entries.push_back( std::make_unique<PARAM_CFG_DOUBLE>( true, AC_KEYS::PDFStrokeFontXOffset,
                                                             &m_PDFStrokeFontXOffset, m_PDFStrokeFontXOffset ) );

    m_entries.push_back( std::make_unique<PARAM_CFG_DOUBLE>( true, AC_KEYS::PDFStrokeFontYOffset,
                                                             &m_PDFStrokeFontYOffset, m_PDFStrokeFontYOffset ) );

    m_entries.push_back( std::make_unique<PARAM_CFG_DOUBLE>( true, AC_KEYS::PDFStrokeFontBoldMultiplier,
                                                             &m_PDFStrokeFontBoldMultiplier,
                                                             m_PDFStrokeFontBoldMultiplier ) );

    m_entries.push_back( std::make_unique<PARAM_CFG_DOUBLE>(
            true, AC_KEYS::PDFStrokeFontKerningFactor, &m_PDFStrokeFontKerningFactor, m_PDFStrokeFontKerningFactor ) );

    m_entries.push_back(
            std::make_unique<PARAM_CFG_BOOL>( true, AC_KEYS::UsePdfPrint, &m_UsePdfPrint, m_UsePdfPrint ) );

    m_entries.push_back( std::make_unique<PARAM_CFG_DOUBLE>( true, AC_KEYS::SmallDrillMarkSize, &m_SmallDrillMarkSize,
                                                             m_SmallDrillMarkSize, 0.0, 3.0 ) );

    m_entries.push_back(
            std::make_unique<PARAM_CFG_BOOL>( true, AC_KEYS::HotkeysDumper, &m_HotkeysDumper, m_HotkeysDumper ) );

    m_entries.push_back( std::make_unique<PARAM_CFG_BOOL>( true, AC_KEYS::DrawBoundingBoxes, &m_DrawBoundingBoxes,
                                                           m_DrawBoundingBoxes ) );

    m_entries.push_back( std::make_unique<PARAM_CFG_BOOL>( true, AC_KEYS::ShowPcbnewExportNetlist,
                                                           &m_ShowPcbnewExportNetlist, m_ShowPcbnewExportNetlist ) );

    m_entries.push_back( std::make_unique<PARAM_CFG_BOOL>( true, AC_KEYS::Skip3DModelFileCache, &m_Skip3DModelFileCache,
                                                           m_Skip3DModelFileCache ) );

    m_entries.push_back( std::make_unique<PARAM_CFG_BOOL>( true, AC_KEYS::Skip3DModelMemoryCache,
                                                           &m_Skip3DModelMemoryCache, m_Skip3DModelMemoryCache ) );

    m_entries.push_back( std::make_unique<PARAM_CFG_BOOL>( true, AC_KEYS::HideVersionFromTitle, &m_HideVersionFromTitle,
                                                           m_HideVersionFromTitle ) );

    m_entries.push_back( std::make_unique<PARAM_CFG_BOOL>( true, AC_KEYS::ShowRepairSchematic, &m_ShowRepairSchematic,
                                                           m_ShowRepairSchematic ) );

    m_entries.push_back( std::make_unique<PARAM_CFG_BOOL>( true, AC_KEYS::ShowEventCounters, &m_ShowEventCounters,
                                                           m_ShowEventCounters ) );

    m_entries.push_back( std::make_unique<PARAM_CFG_BOOL>( true, AC_KEYS::AllowManualCanvasScale,
                                                           &m_AllowManualCanvasScale, m_AllowManualCanvasScale ) );

    m_entries.push_back( std::make_unique<PARAM_CFG_INT>( true, AC_KEYS::V3DRT_BevelHeight_um, &m_3DRT_BevelHeight_um,
                                                          m_3DRT_BevelHeight_um, 0, std::numeric_limits<int>::max(),
                                                          AC_GROUPS::V3D_RayTracing ) );

    m_entries.push_back( std::make_unique<PARAM_CFG_DOUBLE>( true, AC_KEYS::V3DRT_BevelExtentFactor,
                                                             &m_3DRT_BevelExtentFactor, m_3DRT_BevelExtentFactor, 0.0,
                                                             100.0, AC_GROUPS::V3D_RayTracing ) );

    m_entries.push_back( std::make_unique<PARAM_CFG_BOOL>( true, AC_KEYS::Use3DConnexionDriver, &m_Use3DConnexionDriver,
                                                           m_Use3DConnexionDriver ) );

    m_entries.push_back( std::make_unique<PARAM_CFG_BOOL>( true, AC_KEYS::IncrementalConnectivity,
                                                           &m_IncrementalConnectivity, m_IncrementalConnectivity ) );

    m_entries.push_back( std::make_unique<PARAM_CFG_INT>( true, AC_KEYS::DisambiguationTime, &m_DisambiguationMenuDelay,
                                                          m_DisambiguationMenuDelay, 50, 10000 ) );

    m_entries.push_back( std::make_unique<PARAM_CFG_BOOL>( true, AC_KEYS::EnablePcbDesignBlocks,
                                                           &m_EnablePcbDesignBlocks, m_EnablePcbDesignBlocks ) );

    m_entries.push_back( std::make_unique<PARAM_CFG_BOOL>( true, AC_KEYS::EnableGenerators, &m_EnableGenerators,
                                                           m_EnableGenerators ) );

    m_entries.push_back( std::make_unique<PARAM_CFG_BOOL>( true, AC_KEYS::EnableDrcRuleEditor, &m_EnableDrcRuleEditor,
                                                           m_EnableDrcRuleEditor ) );

    m_entries.push_back( std::make_unique<PARAM_CFG_BOOL>( true, AC_KEYS::EnableAPILogging, &m_EnableAPILogging,
                                                           m_EnableAPILogging ) );

    m_entries.push_back( std::make_unique<PARAM_CFG_BOOL>( true, AC_KEYS::EnableLibWithText, &m_EnableLibWithText,
                                                           m_EnableLibWithText ) );

    m_entries.push_back(
            std::make_unique<PARAM_CFG_BOOL>( true, AC_KEYS::EnableLibDir, &m_EnableLibDir, m_EnableLibDir ) );

    m_entries.push_back( std::make_unique<PARAM_CFG_DOUBLE>( true, AC_KEYS::PcbSelectionVisibilityRatio,
                                                             &m_PcbSelectionVisibilityRatio,
                                                             m_PcbSelectionVisibilityRatio, 0.0, 1.0 ) );

    m_entries.push_back( std::make_unique<PARAM_CFG_DOUBLE>( true, AC_KEYS::FontErrorSize, &m_FontErrorSize,
                                                             m_FontErrorSize, 0.01, 100 ) );

    m_entries.push_back( std::make_unique<PARAM_CFG_DOUBLE>( true, AC_KEYS::OcePluginLinearDeflection,
                                                             &m_OcePluginLinearDeflection, m_OcePluginLinearDeflection,
                                                             0.01, 1.0 ) );

    m_entries.push_back( std::make_unique<PARAM_CFG_DOUBLE>( true, AC_KEYS::OcePluginAngularDeflection,
                                                             &m_OcePluginAngularDeflection,
                                                             m_OcePluginAngularDeflection, 0.01, 360.0 ) );

    m_entries.push_back( std::make_unique<PARAM_CFG_INT>( true, AC_KEYS::TriangulateSimplificationLevel,
                                                          &m_TriangulateSimplificationLevel,
                                                          m_TriangulateSimplificationLevel, 5, 1000 ) );

    m_entries.push_back( std::make_unique<PARAM_CFG_INT>(
            true, AC_KEYS::TriangulateMinimumArea, &m_TriangulateMinimumArea, m_TriangulateMinimumArea, 25, 100000 ) );

    m_entries.push_back( std::make_unique<PARAM_CFG_BOOL>( true, AC_KEYS::EnableCacheFriendlyFracture,
                                                           &m_EnableCacheFriendlyFracture,
                                                           m_EnableCacheFriendlyFracture ) );

    m_entries.push_back( std::make_unique<PARAM_CFG_INT>(
            true, AC_KEYS::MaxFileSystemWatchers, &m_MaxFilesystemWatchers, m_MaxFilesystemWatchers, 0, 2147483647 ) );

    m_entries.push_back( std::make_unique<PARAM_CFG_INT>( true, AC_KEYS::MinorSchematicGraphSize,
                                                          &m_MinorSchematicGraphSize, m_MinorSchematicGraphSize, 0,
                                                          2147483647 ) );

    m_entries.push_back( std::make_unique<PARAM_CFG_INT>( true, AC_KEYS::ResolveTextRecursionDepth,
                                                          &m_ResolveTextRecursionDepth, m_ResolveTextRecursionDepth, 0,
                                                          10 ) );

    m_entries.push_back( std::make_unique<PARAM_CFG_BOOL>( true, AC_KEYS::EnableExtensionSnaps, &m_EnableExtensionSnaps,
                                                           m_EnableExtensionSnaps ) );

    m_entries.push_back( std::make_unique<PARAM_CFG_INT>( true, AC_KEYS::ExtensionSnapTimeoutMs,
                                                          &m_ExtensionSnapTimeoutMs, m_ExtensionSnapTimeoutMs, 0 ) );

    m_entries.push_back( std::make_unique<PARAM_CFG_BOOL>( true, AC_KEYS::ExtensionSnapActivateOnHover,
                                                           &m_ExtensionSnapActivateOnHover,
                                                           m_ExtensionSnapActivateOnHover ) );

    m_entries.push_back( std::make_unique<PARAM_CFG_BOOL>( true, AC_KEYS::EnableSnapAnchorsDebug,
                                                           &m_EnableSnapAnchorsDebug, m_EnableSnapAnchorsDebug ) );

    m_entries.push_back( std::make_unique<PARAM_CFG_INT>( true, AC_KEYS::SnapHysteresis, &m_SnapHysteresis,
                                                          m_SnapHysteresis, 0, 100 ) );

    m_entries.push_back( std::make_unique<PARAM_CFG_DOUBLE>( true, AC_KEYS::SnapToAnchorMargin, &m_SnapToAnchorMargin,
                                                             m_SnapToAnchorMargin, 1.0, 2.0 ) );

    m_entries.push_back( std::make_unique<PARAM_CFG_DOUBLE>( true, AC_KEYS::MinParallelAngle, &m_MinParallelAngle,
                                                             m_MinParallelAngle, 0.0, 45.0 ) );

    m_entries.push_back( std::make_unique<PARAM_CFG_DOUBLE>( true, AC_KEYS::HoleWallPaintingMultiplier,
                                                             &m_HoleWallPaintingMultiplier,
                                                             m_HoleWallPaintingMultiplier, 0.1, 100.0 ) );

    m_entries.push_back( std::make_unique<PARAM_CFG_INT>( true, AC_KEYS::MsgPanelShowUuids, &m_MsgPanelShowUuids,
                                                          m_MsgPanelShowUuids ) );

    m_entries.push_back( std::make_unique<PARAM_CFG_INT>( true, AC_KEYS::MaximumThreads, &m_MaximumThreads,
                                                          m_MaximumThreads, 0, 500 ) );

    m_entries.push_back( std::make_unique<PARAM_CFG_INT>( true, AC_KEYS::NetInspectorBulkUpdateOptimisationThreshold,
                                                          &m_NetInspectorBulkUpdateOptimisationThreshold,
                                                          m_NetInspectorBulkUpdateOptimisationThreshold, 0, 1000 ) );

    m_entries.push_back( std::make_unique<PARAM_CFG_INT>( true, AC_KEYS::ExcludeFromSimulationLineWidth,
                                                          &m_ExcludeFromSimulationLineWidth,
                                                          m_ExcludeFromSimulationLineWidth, 1, 100 ) );

    m_entries.push_back( std::make_unique<PARAM_CFG_INT>( true, AC_KEYS::SimulatorMultiRunCombinationLimit,
                                                          &m_SimulatorMultiRunCombinationLimit,
                                                          m_SimulatorMultiRunCombinationLimit, 1, 100 ) );

    m_entries.push_back( std::make_unique<PARAM_CFG_INT>( true, AC_KEYS::GitIconRefreshInterval,
                                                          &m_GitIconRefreshInterval, m_GitIconRefreshInterval, 0, 100000 ) );

    m_entries.push_back( std::make_unique<PARAM_CFG_INT>( true, AC_KEYS::MaxPastedTextLength, &m_MaxPastedTextLength,
                                                          m_MaxPastedTextLength, 0, 100000 ) );

    m_entries.push_back( std::make_unique<PARAM_CFG_INT>( true, AC_KEYS::PNSProcessClusterTimeout,
                                                          &m_PNSProcessClusterTimeout, 100, 10, 10000 ) );

    m_entries.push_back( std::make_unique<PARAM_CFG_INT>( true, AC_KEYS::FollowBranchTimeout,
                                                          &m_FollowBranchTimeout, 500, 50, 5000 ) );

    m_entries.push_back( std::make_unique<PARAM_CFG_BOOL>( true, AC_KEYS::ImportSkipComponentBodies,
                                                           &m_ImportSkipComponentBodies, m_ImportSkipComponentBodies ) );

    m_entries.push_back( std::make_unique<PARAM_CFG_INT>( true, AC_KEYS::ScreenDPI, &m_ScreenDPI, m_ScreenDPI, 50, 500 ) );

    m_entries.push_back( std::make_unique<PARAM_CFG_BOOL>( true, AC_KEYS::EnableUseAuiPerspective,
                                                           &m_EnableUseAuiPerspective, m_EnableUseAuiPerspective ) );

    m_entries.push_back( std::make_unique<PARAM_CFG_INT>( true, AC_KEYS::HistoryLockStaleTimeout,
                                                          &m_HistoryLockStaleTimeout, m_HistoryLockStaleTimeout, 10,
                                                          86400 ) ); // 10 seconds to 24 hours

    m_entries.push_back( std::make_unique<PARAM_CFG_BOOL>( true, AC_KEYS::ZoneFillIterativeRefill,
                                                           &m_ZoneFillIterativeRefill, m_ZoneFillIterativeRefill ) );

    // Special case for trace mask setting...we just grab them and set them immediately
    // Because we even use wxLogTrace inside of advanced config
    m_entries.push_back( std::make_unique<PARAM_CFG_WXSTRING>( true, AC_KEYS::TraceMasks, &m_traceMasks, wxS( "" ) ) );

    // Load the config from file
    wxConfigLoadSetups( &aCfg, m_entries );

    // Now actually set the trace masks
    wxStringTokenizer traceMaskTokenizer( m_traceMasks, ", ", wxTOKEN_STRTOK );

    while( traceMaskTokenizer.HasMoreTokens() )
    {
        wxString mask = traceMaskTokenizer.GetNextToken();
        wxLog::AddTraceMask( mask );
    }

    dumpCfg( m_entries );

    wxLogTrace( kicadTraceCoroutineStack, wxT( "Using coroutine stack size %d" ), m_CoroutineStackSize );
}
