/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2019-2023 KiCad Developers, see AUTHORS.txt for contributors.
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
}

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
static const wxChar UseClipper2[] = wxT( "UseClipper2" );
static const wxChar EnableGenerators[] = wxT( "EnableGenerators" );
static const wxChar EnableGit[] = wxT( "EnableGit" );
static const wxChar EnableLibWithText[] = wxT( "EnableLibWithText" );
static const wxChar EnableLibDir[] = wxT( "EnableLibDir" );
static const wxChar EnableEeschemaPrintCairo[] = wxT( "EnableEeschemaPrintCairo" );
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
static const wxChar ZoneConnectionFiller[] = wxT( "ZoneConnectionFiller" );

} // namespace KEYS


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
        s << ( *static_cast<const PARAM_CFG_BOOL&>( aParam ).m_Pt_param ? wxS( "true" ) : wxS( "false" ) );
        break;
    default: s << wxS( "Unsupported PARAM_CFG variant: " ) << aParam.m_Type;
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
    const static wxString cfg_filename{ wxS( "kicad_advanced" ) };
    return wxFileName( PATHS::GetUserSettingsPath(), cfg_filename );
}


ADVANCED_CFG::ADVANCED_CFG()
{
    wxLogTrace( AdvancedConfigMask, wxS( "Init advanced config" ) );

    // Init defaults - this is done in case the config doesn't exist,
    // then the values will remain as set here.
    m_CoroutineStackSize        = AC_STACK::default_stack;
    m_ShowRouterDebugGraphics   = false;
    m_EnableRouterDump          = false;
    m_HyperZoom                 = false;
    m_DrawArcAccuracy           = 10.0;
    m_DrawArcCenterMaxAngle     = 50.0;
    m_MaxTangentAngleDeviation  = 1.0;
    m_MaxTrackLengthToKeep      = 0.0005;
    m_ExtraZoneDisplayModes     = false;
    m_DrawTriangulationOutlines = false;

    m_ExtraClearance            = 0.0005;
    m_DRCEpsilon                = 0.0005;   // 0.5um is small enough not to materially violate
                                            // any constraints.
    m_SliverWidthTolerance      = 0.08;
    m_SliverMinimumLength       = 0.0008;
    m_SliverAngleTolerance      = 20.0;

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
    m_ShowEventCounters         = false;
    m_AllowManualCanvasScale    = false;
    m_CompactSave               = false;
    m_UpdateUIEventInterval     = 0;
    m_ShowRepairSchematic       = false;
    m_EnableGenerators          = false;
    m_EnableGit                 = false;
    m_EnableLibWithText         = false;
    m_EnableLibDir              = false;

    m_EnableEeschemaPrintCairo  = true;

    m_3DRT_BevelHeight_um       = 30;
    m_3DRT_BevelExtentFactor    = 1.0 / 16.0;

    m_UseClipper2               = true;
    m_EnableAPILogging          = false;

    m_Use3DConnexionDriver      = true;

    m_IncrementalConnectivity   = true;

    m_DisambiguationMenuDelay   = 500;

    m_PcbSelectionVisibilityRatio = 1.0;

    m_FontErrorSize             = 2;

    m_OcePluginLinearDeflection = 0.14;
    m_OcePluginAngularDeflection = 30;

    m_TriangulateSimplificationLevel = 50;
    m_TriangulateMinimumArea = 1000;

    m_EnableCacheFriendlyFracture = true;

    m_MaxFilesystemWatchers = 16384;

    m_MinorSchematicGraphSize = 10000;

    m_ResolveTextRecursionDepth = 3;

    m_ZoneConnectionFiller = false;

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
        wxLogTrace( AdvancedConfigMask, wxS( "File does not exist %s" ), k_advanced.GetFullPath() );

        // load the defaults
        wxConfig emptyConfig;
        loadSettings( emptyConfig );

        return;
    }

    wxLogTrace( AdvancedConfigMask, wxS( "Loading advanced config from: %s" ),
                k_advanced.GetFullPath() );

    wxFileConfig file_cfg( wxS( "" ), wxS( "" ), k_advanced.GetFullPath() );
    loadSettings( file_cfg );
}


void ADVANCED_CFG::loadSettings( wxConfigBase& aCfg )
{
    std::vector<PARAM_CFG*> configParams;

    configParams.push_back( new PARAM_CFG_DOUBLE( true, AC_KEYS::ExtraFillMargin,
                                                  &m_ExtraClearance,
                                                  m_ExtraClearance, 0.0, 1.0 ) );

    configParams.push_back( new PARAM_CFG_DOUBLE( true, AC_KEYS::DRCEpsilon,
                                                  &m_DRCEpsilon, m_DRCEpsilon, 0.0, 1.0 ) );

    configParams.push_back( new PARAM_CFG_DOUBLE( true, AC_KEYS::DRCSliverWidthTolerance,
                                                  &m_SliverWidthTolerance, m_SliverWidthTolerance,
                                                  0.01, 0.25 ) );

    configParams.push_back( new PARAM_CFG_DOUBLE( true, AC_KEYS::DRCSliverMinimumLength,
                                                  &m_SliverMinimumLength, m_SliverMinimumLength,
                                                  1e-9, 10 ) );

    configParams.push_back( new PARAM_CFG_DOUBLE( true, AC_KEYS::DRCSliverAngleTolerance,
                                                  &m_SliverAngleTolerance, m_SliverAngleTolerance,
                                                  1.0, 90.0 ) );

    configParams.push_back( new PARAM_CFG_DOUBLE( true, AC_KEYS::HoleWallThickness,
                                                  &m_HoleWallThickness, m_HoleWallThickness,
                                                  0.0, 1.0 ) );

    configParams.push_back( new PARAM_CFG_INT( true, AC_KEYS::CoroutineStackSize,
                                               &m_CoroutineStackSize, AC_STACK::default_stack,
                                               AC_STACK::min_stack, AC_STACK::max_stack ) );

    configParams.push_back( new PARAM_CFG_INT( true, AC_KEYS::UpdateUIEventInterval,
                                               &m_UpdateUIEventInterval, m_UpdateUIEventInterval,
                                               -1, 100000 ) );

    configParams.push_back( new PARAM_CFG_BOOL( true, AC_KEYS::ShowRouterDebugGraphics,
                                                &m_ShowRouterDebugGraphics,
                                                m_ShowRouterDebugGraphics ) );

    configParams.push_back( new PARAM_CFG_BOOL( true, AC_KEYS::EnableRouterDump,
                                                &m_EnableRouterDump, m_EnableRouterDump ) );

    configParams.push_back( new PARAM_CFG_BOOL( true, AC_KEYS::HyperZoom,
                                                &m_HyperZoom, m_HyperZoom ) );

    configParams.push_back( new PARAM_CFG_BOOL( true, AC_KEYS::CompactFileSave,
                                                &m_CompactSave, m_CompactSave ) );

    configParams.push_back( new PARAM_CFG_DOUBLE( true, AC_KEYS::DrawArcAccuracy,
                                                  &m_DrawArcAccuracy, m_DrawArcAccuracy,
                                                  0.0, 100000.0 ) );

    configParams.push_back( new PARAM_CFG_DOUBLE( true, AC_KEYS::DrawArcCenterStartEndMaxAngle,
                                                  &m_DrawArcCenterMaxAngle,
                                                  m_DrawArcCenterMaxAngle, 0.0, 100000.0 ) );

    configParams.push_back( new PARAM_CFG_DOUBLE( true, AC_KEYS::MaxTangentTrackAngleDeviation,
                                                  &m_MaxTangentAngleDeviation,
                                                  m_MaxTangentAngleDeviation, 0.0, 90.0 ) );

    configParams.push_back( new PARAM_CFG_DOUBLE( true, AC_KEYS::MaxTrackLengthToKeep,
                                                  &m_MaxTrackLengthToKeep, m_MaxTrackLengthToKeep,
                                                  0.0, 1.0 ) );

    configParams.push_back( new PARAM_CFG_BOOL( true, AC_KEYS::ExtraZoneDisplayModes,
                                                &m_ExtraZoneDisplayModes,
                                                m_ExtraZoneDisplayModes ) );

    configParams.push_back( new PARAM_CFG_BOOL( true, AC_KEYS::StrokeTriangulation,
                                                &m_DrawTriangulationOutlines,
                                                m_DrawTriangulationOutlines ) );

    configParams.push_back( new PARAM_CFG_DOUBLE( true, AC_KEYS::MinPlotPenWidth,
                                                  &m_MinPlotPenWidth, m_MinPlotPenWidth,
                                                  0.0, 1.0 ) );

    configParams.push_back( new PARAM_CFG_BOOL( true, AC_KEYS::DebugZoneFiller,
                                                &m_DebugZoneFiller, m_DebugZoneFiller ) );

    configParams.push_back( new PARAM_CFG_BOOL( true, AC_KEYS::DebugPDFWriter,
                                                &m_DebugPDFWriter, m_DebugPDFWriter ) );

    configParams.push_back( new PARAM_CFG_DOUBLE( true, AC_KEYS::SmallDrillMarkSize,
                                                  &m_SmallDrillMarkSize, m_SmallDrillMarkSize,
                                                  0.0, 3.0 ) );

    configParams.push_back( new PARAM_CFG_BOOL( true, AC_KEYS::HotkeysDumper,
                                                &m_HotkeysDumper, m_HotkeysDumper ) );

    configParams.push_back( new PARAM_CFG_BOOL( true, AC_KEYS::DrawBoundingBoxes,
                                                &m_DrawBoundingBoxes, m_DrawBoundingBoxes ) );

    configParams.push_back( new PARAM_CFG_BOOL( true, AC_KEYS::ShowPcbnewExportNetlist,
                                                &m_ShowPcbnewExportNetlist,
                                                m_ShowPcbnewExportNetlist ) );

    configParams.push_back( new PARAM_CFG_BOOL( true, AC_KEYS::Skip3DModelFileCache,
                                                &m_Skip3DModelFileCache, m_Skip3DModelFileCache ) );

    configParams.push_back( new PARAM_CFG_BOOL( true, AC_KEYS::Skip3DModelMemoryCache,
                                                &m_Skip3DModelMemoryCache, m_Skip3DModelMemoryCache ) );

    configParams.push_back( new PARAM_CFG_BOOL( true, AC_KEYS::HideVersionFromTitle,
                                                &m_HideVersionFromTitle, m_HideVersionFromTitle ) );

    configParams.push_back( new PARAM_CFG_BOOL( true, AC_KEYS::ShowRepairSchematic,
                                                &m_ShowRepairSchematic, m_ShowRepairSchematic ) );

    configParams.push_back( new PARAM_CFG_BOOL( true, AC_KEYS::ShowEventCounters,
                                                &m_ShowEventCounters, m_ShowEventCounters ) );

    configParams.push_back( new PARAM_CFG_BOOL( true, AC_KEYS::AllowManualCanvasScale,
                                                &m_AllowManualCanvasScale,
                                                m_AllowManualCanvasScale ) );

    configParams.push_back( new PARAM_CFG_INT( true, AC_KEYS::V3DRT_BevelHeight_um,
                                               &m_3DRT_BevelHeight_um, m_3DRT_BevelHeight_um,
                                               0, std::numeric_limits<int>::max(),
                                               AC_GROUPS::V3D_RayTracing ) );

    configParams.push_back( new PARAM_CFG_DOUBLE( true, AC_KEYS::V3DRT_BevelExtentFactor,
                                                  &m_3DRT_BevelExtentFactor,
                                                  m_3DRT_BevelExtentFactor, 0.0, 100.0,
                                                  AC_GROUPS::V3D_RayTracing ) );

    configParams.push_back( new PARAM_CFG_BOOL( true, AC_KEYS::UseClipper2,
                                                &m_UseClipper2, m_UseClipper2 ) );

    configParams.push_back( new PARAM_CFG_BOOL( true, AC_KEYS::Use3DConnexionDriver,
                                                &m_Use3DConnexionDriver, m_Use3DConnexionDriver ) );

    configParams.push_back( new PARAM_CFG_BOOL( true, AC_KEYS::IncrementalConnectivity,
                                                &m_IncrementalConnectivity,
                                                m_IncrementalConnectivity ) );

    configParams.push_back( new PARAM_CFG_INT( true, AC_KEYS::DisambiguationTime,
                                               &m_DisambiguationMenuDelay,
                                               m_DisambiguationMenuDelay,
                                               50, 10000 ) );

    configParams.push_back( new PARAM_CFG_BOOL( true, AC_KEYS::EnableGenerators,
                                                &m_EnableGenerators, m_EnableGenerators ) );

    configParams.push_back( new PARAM_CFG_BOOL( true, AC_KEYS::EnableAPILogging,
                                                &m_EnableAPILogging, m_EnableAPILogging ) );

    configParams.push_back( new PARAM_CFG_BOOL( true, AC_KEYS::EnableGit,
                                                &m_EnableGit, m_EnableGit ) );

    configParams.push_back( new PARAM_CFG_BOOL( true, AC_KEYS::EnableLibWithText,
                                                &m_EnableLibWithText, m_EnableLibWithText ) );

    configParams.push_back( new PARAM_CFG_BOOL( true, AC_KEYS::EnableLibDir,
                                                &m_EnableLibDir, m_EnableLibDir ) );

    configParams.push_back( new PARAM_CFG_BOOL( true, AC_KEYS::EnableEeschemaPrintCairo,
                                                &m_EnableEeschemaPrintCairo,
                                                m_EnableEeschemaPrintCairo ) );

    configParams.push_back( new PARAM_CFG_DOUBLE( true, AC_KEYS::PcbSelectionVisibilityRatio,
                                                  &m_PcbSelectionVisibilityRatio,
                                                  m_PcbSelectionVisibilityRatio, 0.0, 1.0 ) );

    configParams.push_back( new PARAM_CFG_DOUBLE( true, AC_KEYS::FontErrorSize,
                                                  &m_FontErrorSize,
                                                  m_FontErrorSize, 0.01, 100 ) );

    configParams.push_back( new PARAM_CFG_DOUBLE( true, AC_KEYS::OcePluginLinearDeflection,
                                                    &m_OcePluginLinearDeflection,
                                                    m_OcePluginLinearDeflection, 0.01, 1.0 ) );

    configParams.push_back( new PARAM_CFG_DOUBLE( true, AC_KEYS::OcePluginAngularDeflection,
                                                    &m_OcePluginAngularDeflection,
                                                    m_OcePluginAngularDeflection, 0.01, 360.0 ) );

    configParams.push_back( new PARAM_CFG_INT( true, AC_KEYS::TriangulateSimplificationLevel,
                                                    &m_TriangulateSimplificationLevel,
                                                    m_TriangulateSimplificationLevel, 0, 1000 ) );

    configParams.push_back( new PARAM_CFG_INT( true, AC_KEYS::TriangulateMinimumArea,
                                                    &m_TriangulateMinimumArea,
                                                    m_TriangulateMinimumArea, 0, 100000 ) );

    configParams.push_back( new PARAM_CFG_BOOL( true, AC_KEYS::EnableCacheFriendlyFracture,
                                                &m_EnableCacheFriendlyFracture,
                                                m_EnableCacheFriendlyFracture ) );

    configParams.push_back( new PARAM_CFG_INT( true, AC_KEYS::MaxFileSystemWatchers,
                                                  &m_MaxFilesystemWatchers, m_MaxFilesystemWatchers,
                                                  0, 2147483647 ) );

    configParams.push_back( new PARAM_CFG_INT( true, AC_KEYS::MinorSchematicGraphSize,
                                                  &m_MinorSchematicGraphSize, m_MinorSchematicGraphSize,
                                                  0, 2147483647 ) );

    configParams.push_back( new PARAM_CFG_INT( true, AC_KEYS::ResolveTextRecursionDepth,
                                                  &m_ResolveTextRecursionDepth,
                                                  m_ResolveTextRecursionDepth, 0, 10 ) );

    configParams.push_back( new PARAM_CFG_BOOL( true, AC_KEYS::ZoneConnectionFiller,
                                                &m_ZoneConnectionFiller, m_ZoneConnectionFiller ) );

    // Special case for trace mask setting...we just grab them and set them immediately
    // Because we even use wxLogTrace inside of advanced config
    wxString traceMasks;
    configParams.push_back( new PARAM_CFG_WXSTRING( true, AC_KEYS::TraceMasks, &traceMasks,
                                                    wxS( "" ) ) );

    // Load the config from file
    wxConfigLoadSetups( &aCfg, configParams );

    // Now actually set the trace masks
    wxStringTokenizer traceMaskTokenizer( traceMasks, wxS( "," ) );

    while( traceMaskTokenizer.HasMoreTokens() )
    {
        wxString mask = traceMaskTokenizer.GetNextToken();
        wxLog::AddTraceMask( mask );
    }

    dumpCfg( configParams );

    for( PARAM_CFG* param : configParams )
        delete param;

    wxLogTrace( kicadTraceCoroutineStack, wxT( "Using coroutine stack size %d" ), m_CoroutineStackSize );
}


