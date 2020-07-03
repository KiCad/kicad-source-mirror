/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
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

#include <advanced_config.h>

#include <common.h>
#include <config_params.h>
#include <settings/settings_manager.h>

#include <wx/config.h>
#include <wx/filename.h>
#include <wx/log.h>

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
 * In Pcbnew, pads can have a fabrication property
 * Because this feature adds a new keyword in *.kicad_pcb and *.kicad_modfiles,
 * this is an advanced feature until it is fully finalized
 */
static const wxChar UsePadProperty[] = wxT( "UsePadProperty" );

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
    m_EnableUsePadProperty    = false;
    m_realTimeConnectivity    = true;
    m_coroutineStackSize      = AC_STACK::default_stack;
    m_ShowRouterDebugGraphics = false;

    loadFromConfigFile();
}


const ADVANCED_CFG& ADVANCED_CFG::GetCfg()
{
    static ADVANCED_CFG instance;
    return instance;
}


void ADVANCED_CFG::loadFromConfigFile()
{
    const auto k_advanced = getAdvancedCfgFilename();

    if( !k_advanced.FileExists() )
    {
        wxLogTrace( AdvancedConfigMask, "File does not exist %s", k_advanced.GetFullPath() );
        return;
    }

    wxLogTrace( AdvancedConfigMask, "Loading advanced config from: %s", k_advanced.GetFullPath() );

    wxFileConfig file_cfg( "", "", k_advanced.GetFullPath() );
    loadSettings( file_cfg );
}


void ADVANCED_CFG::loadSettings( wxConfigBase& aCfg )
{
    std::vector<PARAM_CFG*> configParams;

    configParams.push_back( new PARAM_CFG_BOOL( true, AC_KEYS::UsePadProperty,
                                                &m_EnableUsePadProperty, false ) );

    configParams.push_back( new PARAM_CFG_BOOL( true, AC_KEYS::RealtimeConnectivity,
                                                &m_realTimeConnectivity, false ) );

    configParams.push_back( new PARAM_CFG_INT( true, AC_KEYS::CoroutineStackSize,
                                               &m_coroutineStackSize, AC_STACK::default_stack,
                                               AC_STACK::min_stack, AC_STACK::max_stack ) );

    configParams.push_back( new PARAM_CFG_BOOL( true, AC_KEYS::ShowRouterDebugGraphics,
                                                &m_ShowRouterDebugGraphics, false ) );

    wxConfigLoadSetups( &aCfg, configParams );

    for( auto param : configParams )
        delete param;

    dumpCfg( configParams );
}


