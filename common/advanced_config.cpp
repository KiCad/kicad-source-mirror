/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2018 KiCad Developers, see AUTHORS.txt for contributors.
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
 * List of known keys for advanced configuration options.
 *
 * Set these options in the file `kicad_advanced` in the
 * KiCad config directory.
 */
namespace AC_KEYS
{

/**
 * Currently (Version 5.1) SVG import is disabled by default, to avoid issues
 * SVG needs some enhancements.
 *
 * Especially, all SVG shapes are imported as curves and converted to a lot of segments.
 * A better approach is to convert to polylines (not yet existing in Pcbnew) and keep
 * arcs and circles as primitives (not yet possible with tinysvg library.
 * So, until these issues are solved, disable SVG import option.
 *
 * Warning: enable svg import is currently only for developers.
 */
static const wxChar EnableSvgImport[] = wxT( "EnableSvgImport" );

/**
 * Allow legacy canvas to be shown in GTK3. Legacy canvas is generally pretty
 * broken, but this avoids code in an ifdef where it could become broken
 * on other platforms
 */
static const wxChar AllowLegacyCanvasInGtk3[] = wxT( "AllowLegacyCanvasInGtk3" );

} // namespace KEYS


/*
 * Get a simple string for common parameters.
 *
 * This isn't exhaustive, but it covers most common types that might be
 * used in the advance config
 */
wxString dumpParamCfg( const PARAM_CFG_BASE& aParam )
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
static void dumpCfg( const PARAM_CFG_ARRAY& aArray )
{
    // only dump if we need to
    if( !wxLog::IsAllowedTraceMask( AdvancedConfigMask ) )
        return;

    for( const auto& param : aArray )
    {
        wxLogTrace( AdvancedConfigMask, dumpParamCfg( param ) );
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
    return wxFileName( GetKicadConfigPath(), cfg_filename );
}


ADVANCED_CFG::ADVANCED_CFG()
{
    wxLogTrace( AdvancedConfigMask, "Init advanced config" );

    // Init defaults - this is done in case the config doesn't exist,
    // then the values will remain as set here.
    m_enableSvgImport = false;
    m_allowLegacyCanvasInGtk3 = false;

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
    PARAM_CFG_ARRAY configParams;

    configParams.push_back(
            new PARAM_CFG_BOOL( true, AC_KEYS::EnableSvgImport, &m_enableSvgImport, false ) );

    configParams.push_back( new PARAM_CFG_BOOL(
            true, AC_KEYS::AllowLegacyCanvasInGtk3, &m_allowLegacyCanvasInGtk3, false ) );

    wxConfigLoadSetups( &aCfg, configParams );

    dumpCfg( configParams );
}


bool ADVANCED_CFG::AllowLegacyCanvas() const
{
    // default is to allow
    bool allow = true;

    // on GTK3, check the config
#ifdef __WXGTK3__
    allow = m_allowLegacyCanvasInGtk3;
#endif

    return allow;
}