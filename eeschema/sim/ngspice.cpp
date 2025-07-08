/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2016-2022 CERN
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * @author Tomasz Wlostowski <tomasz.wlostowski@cern.ch>
 * @author Maciej Suminski <maciej.suminski@cern.ch>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 3
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you may find one here:
 * https://www.gnu.org/licenses/gpl-3.0.html
 * or you may search the http://www.gnu.org website for the version 3 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#include <config.h>     // Needed for MSW compilation
#include <common.h>
#include <locale_io.h>
#include <fmt/core.h>
#include <paths.h>
#include <richio.h>

#include "spice_circuit_model.h"
#include "ngspice.h"
#include "simulator_reporter.h"
#include "spice_settings.h"

#include <wx/stdpaths.h>
#include <wx/dir.h>
#include <wx/log.h>

#include <stdexcept>
#include <algorithm>


/**
 * Flag to enable debug output of Ngspice simulator.
 *
 * Use "KICAD_NGSPICE" to enable Ngspice simulator tracing.
 *
 * @ingroup trace_env_vars
 */
static const wxChar* const traceNgspice = wxT( "KICAD_NGSPICE" );


NGSPICE::NGSPICE() :
        m_ngSpice_Init( nullptr ),
        m_ngSpice_Circ( nullptr ),
        m_ngSpice_Command( nullptr ),
        m_ngGet_Vec_Info( nullptr ),
        m_ngCM_Input_Path( nullptr ),
        m_ngSpice_CurPlot( nullptr ),
        m_ngSpice_AllPlots( nullptr ),
        m_ngSpice_AllVecs( nullptr ),
        m_ngSpice_Running( nullptr ),
        m_ngSpice_LockRealloc( nullptr ),
        m_ngSpice_UnlockRealloc( nullptr ),
        m_error( false )
{
    init_dll();
}


NGSPICE::~NGSPICE() = default;


void NGSPICE::updateNgspiceSettings()
{
    for( const std::string& command : GetSettingCommands() )
    {
        wxLogTrace( traceNgspice, "Sending Ngspice configuration command '%s'.", command );
        Command( command );
    }
}


void NGSPICE::Init( const SPICE_SETTINGS* aSettings )
{
    Command( "reset" );
    updateNgspiceSettings();
}


wxString NGSPICE::CurrentPlotName() const
{
    return wxString( m_ngSpice_CurPlot() );
}


std::vector<std::string> NGSPICE::AllVectors() const
{
    LOCALE_IO c_locale; // ngspice works correctly only with C locale
    char*     currentPlot = m_ngSpice_CurPlot();
    char**    allVectors  = m_ngSpice_AllVecs( currentPlot );
    int       noOfVectors = 0;

    std::vector<std::string> retVal;

    if( allVectors != nullptr )
    {
        for( char** plot = allVectors; *plot != nullptr; plot++ )
            noOfVectors++;

        retVal.reserve( noOfVectors );

        for( int i = 0; i < noOfVectors; i++, allVectors++ )
        {
            std::string vec = *allVectors;
            retVal.push_back( vec );
        }
    }


    return retVal;
}


std::vector<COMPLEX> NGSPICE::GetComplexVector( const std::string& aName, int aMaxLen )
{
    LOCALE_IO            c_locale;       // ngspice works correctly only with C locale
    std::vector<COMPLEX> data;
    NGSPICE_LOCK_REALLOC lock( this );

    if( aMaxLen == 0 )
        return data;

    if( vector_info* vi = m_ngGet_Vec_Info( (char*) aName.c_str() ) )
    {
        int length = aMaxLen < 0 ? vi->v_length : std::min( aMaxLen, vi->v_length );
        data.reserve( length );

        if( vi->v_realdata )
        {
            for( int i = 0; i < length; i++ )
                data.emplace_back( vi->v_realdata[i], 0.0 );
        }
        else if( vi->v_compdata )
        {
            for( int i = 0; i < length; i++ )
                data.emplace_back( vi->v_compdata[i].cx_real, vi->v_compdata[i].cx_imag );
        }
    }

    return data;
}


std::vector<double> NGSPICE::GetRealVector( const std::string& aName, int aMaxLen )
{
    LOCALE_IO            c_locale;       // ngspice works correctly only with C locale
    std::vector<double>  data;
    NGSPICE_LOCK_REALLOC lock( this );

    if( aMaxLen == 0 )
        return data;

    if( vector_info* vi = m_ngGet_Vec_Info( (char*) aName.c_str() ) )
    {
        int length = aMaxLen < 0 ? vi->v_length : std::min( aMaxLen, vi->v_length );
        data.reserve( length );

        if( vi->v_realdata )
        {
            for( int i = 0; i < length; i++ )
                data.push_back( vi->v_realdata[i] );
        }
        else if( vi->v_compdata )
        {
            for( int i = 0; i < length; i++ )
                data.push_back( vi->v_compdata[i].cx_real );
        }
    }

    return data;
}


std::vector<double> NGSPICE::GetImaginaryVector( const std::string& aName, int aMaxLen )
{
    LOCALE_IO            c_locale;       // ngspice works correctly only with C locale
    std::vector<double>  data;
    NGSPICE_LOCK_REALLOC lock( this );

    if( aMaxLen == 0 )
        return data;

    if( vector_info* vi = m_ngGet_Vec_Info( (char*) aName.c_str() ) )
    {
        int length = aMaxLen < 0 ? vi->v_length : std::min( aMaxLen, vi->v_length );
        data.reserve( length );

        if( vi->v_compdata )
        {
            for( int i = 0; i < length; i++ )
                data.push_back( vi->v_compdata[i].cx_imag );
        }
    }

    return data;
}


std::vector<double> NGSPICE::GetGainVector( const std::string& aName, int aMaxLen )
{
    LOCALE_IO            c_locale;       // ngspice works correctly only with C locale
    std::vector<double>  data;
    NGSPICE_LOCK_REALLOC lock( this );

    if( aMaxLen == 0 )
        return data;

    if( vector_info* vi = m_ngGet_Vec_Info( (char*) aName.c_str() ) )
    {
        int length = aMaxLen < 0 ? vi->v_length : std::min( aMaxLen, vi->v_length );
        data.reserve( length );

        if( vi->v_realdata )
        {
            for( int i = 0; i < length; i++ )
                data.push_back( vi->v_realdata[i] );
        }
        else if( vi->v_compdata )
        {
            for( int i = 0; i < length; i++ )
                data.push_back( hypot( vi->v_compdata[i].cx_real, vi->v_compdata[i].cx_imag ) );
        }
    }

    return data;
}


std::vector<double> NGSPICE::GetPhaseVector( const std::string& aName, int aMaxLen )
{
    LOCALE_IO            c_locale;       // ngspice works correctly only with C locale
    std::vector<double>  data;
    NGSPICE_LOCK_REALLOC lock( this );

    if( aMaxLen == 0 )
        return data;

    if( vector_info* vi = m_ngGet_Vec_Info( (char*) aName.c_str() ) )
    {
        int length = aMaxLen < 0 ? vi->v_length : std::min( aMaxLen, vi->v_length );
        data.reserve( length );

        if( vi->v_realdata )
        {
            for( int i = 0; i < length; i++ )
                data.push_back( 0.0 );      // well, that's life
        }
        else if( vi->v_compdata )
        {
            for( int i = 0; i < length; i++ )
                data.push_back( atan2( vi->v_compdata[i].cx_imag, vi->v_compdata[i].cx_real ) );
        }
    }

    return data;
}


bool NGSPICE::Attach( const std::shared_ptr<SIMULATION_MODEL>& aModel, const wxString& aSimCommand,
                      unsigned aSimOptions, const wxString& aInputPath, REPORTER& aReporter )
{
    SPICE_CIRCUIT_MODEL* model = dynamic_cast<SPICE_CIRCUIT_MODEL*>( aModel.get() );
    STRING_FORMATTER     formatter;

    setCodemodelsInputPath( aInputPath.ToStdString() );

    if( model && model->GetNetlist( aSimCommand, aSimOptions, &formatter, aReporter ) )
    {
        SIMULATOR::Attach( aModel, aSimCommand, aSimOptions, aInputPath, aReporter );
        updateNgspiceSettings();
        LoadNetlist( formatter.GetString() );

        if( !( aSimOptions & NETLIST_EXPORTER_SPICE::OPTION_SAVE_ALL_EVENTS ) )
        {
            Command( "echo Command: esave none" );
            Command( "esave none" );
        }

        return true;
    }
    else
    {
        SIMULATOR::Attach( nullptr, wxEmptyString, 0, wxEmptyString, aReporter );
        return false;
    }
}


bool NGSPICE::LoadNetlist( const std::string& aNetlist )
{
    LOCALE_IO          c_locale;       // ngspice works correctly only with C locale
    std::vector<char*> lines;
    std::stringstream  ss( aNetlist );

    m_netlist.erase();

    for( std::string line; std::getline( ss, line ); )
    {
        lines.push_back( strdup( line.data() ) );
        m_netlist += line;
        m_netlist += '\n';
    }

    lines.push_back( nullptr ); // sentinel, as requested in ngSpice_Circ description

    Command( "remcirc" );
    bool success = !m_ngSpice_Circ( lines.data() );

    for( char* line : lines )
        free( line );

    return success;
}


bool NGSPICE::Run()
{
    LOCALE_IO toggle;               // ngspice works correctly only with C locale
    return Command( "bg_run" );     // bg_* commands execute in a separate thread
}


bool NGSPICE::Stop()
{
    LOCALE_IO c_locale;             // ngspice works correctly only with C locale
    return Command( "bg_halt" );    // bg_* commands execute in a separate thread
}


bool NGSPICE::IsRunning()
{
    // No need to use C locale here
    return m_ngSpice_Running();
}


bool NGSPICE::Command( const std::string& aCmd )
{
    LOCALE_IO c_locale;               // ngspice works correctly only with C locale
    validate();
    return !m_ngSpice_Command( (char*) aCmd.c_str() );
}


wxString NGSPICE::GetXAxis( SIM_TYPE aType ) const
{
    switch( aType )
    {
    case ST_AC:
    case ST_SP:
    case ST_NOISE:
    case ST_FFT:
        return wxS( "frequency" );

    case ST_DC:
        // find plot, which ends with "-sweep"
        for( wxString vector : AllVectors() )
        {
            if( vector.Lower().EndsWith( wxS( "-sweep" ) ) )
                return vector;
        }

        return wxS( "sweep" );

    case ST_TRAN:
        return wxS( "time" );

    default:
        return wxEmptyString;
    }
}


std::vector<std::string> NGSPICE::GetSettingCommands() const
{
    const NGSPICE_SETTINGS* settings = dynamic_cast<const NGSPICE_SETTINGS*>( Settings().get() );

    std::vector<std::string> commands;

    wxCHECK( settings, commands );

    switch( settings->GetCompatibilityMode() )
    {
    case NGSPICE_COMPATIBILITY_MODE::USER_CONFIG:                                                break;
    case NGSPICE_COMPATIBILITY_MODE::NGSPICE:   commands.emplace_back( "unset ngbehavior" );     break;
    case NGSPICE_COMPATIBILITY_MODE::PSPICE:    commands.emplace_back( "set ngbehavior=psa" );   break;
    case NGSPICE_COMPATIBILITY_MODE::LTSPICE:   commands.emplace_back( "set ngbehavior=lta" );   break;
    case NGSPICE_COMPATIBILITY_MODE::LT_PSPICE: commands.emplace_back( "set ngbehavior=ltpsa" ); break;
    case NGSPICE_COMPATIBILITY_MODE::HSPICE:    commands.emplace_back( "set ngbehavior=hsa" );   break;
    default:    wxFAIL_MSG( wxString::Format( "Undefined NGSPICE_COMPATIBILITY_MODE %d.",
                                              settings->GetCompatibilityMode() ) );              break;
    }

    return commands;
}


const std::string NGSPICE::GetNetlist() const
{
    return m_netlist;
}


void NGSPICE::init_dll()
{
    if( m_initialized )
        return;

    LOCALE_IO c_locale;               // ngspice works correctly only with C locale
    const wxStandardPaths& stdPaths = wxStandardPaths::Get();

    if( m_dll.IsLoaded() )      // enable force reload
        m_dll.Unload();

    // Extra effort to find libngspice
    // @todo Shouldn't we be using the normal KiCad path searching mechanism here?
    wxFileName dllFile( "", NGSPICE_DLL_FILE );
#if defined(__WINDOWS__)
  #if defined( _MSC_VER )
    std::vector<std::string> dllPaths = { "" };
  #else
    std::vector<std::string> dllPaths = { "", "/mingw64/bin", "/mingw32/bin" };
  #endif
#elif defined(__WXMAC__)
    std::vector<std::string> dllPaths = {
        PATHS::GetOSXKicadUserDataDir().ToStdString() + "/PlugIns/ngspice",
        PATHS::GetOSXKicadMachineDataDir().ToStdString() + "/PlugIns/ngspice",

        // when running kicad.app
        stdPaths.GetPluginsDir().ToStdString() + "/sim",

        // when running eeschema.app
        wxFileName( stdPaths.GetExecutablePath() ).GetPath().ToStdString() +
                "/../../../../../Contents/PlugIns/sim"
    };
#else   // Unix systems
    std::vector<std::string> dllPaths = { "/usr/local/lib" };
#endif

    if( wxGetEnv( wxT( "KICAD_RUN_FROM_BUILD_DIR" ), nullptr ) )
        dllPaths.emplace_back( NGSPICE_DLL_DIR );

#if defined(__WINDOWS__) || (__WXMAC__)
    for( const auto& path : dllPaths )
    {
        dllFile.SetPath( path );
        wxLogTrace( traceNgspice, "libngspice search path: %s", dllFile.GetFullPath() );
        m_dll.Load( dllFile.GetFullPath(), wxDL_VERBATIM | wxDL_QUIET | wxDL_NOW );

        if( m_dll.IsLoaded() )
        {
            wxLogTrace( traceNgspice, "libngspice path found in: %s", dllFile.GetFullPath() );
            break;
        }
    }

    if( !m_dll.IsLoaded() ) // try also the system libraries
        m_dll.Load( wxDynamicLibrary::CanonicalizeName( "ngspice" ) );
#else
    // First, try the system libraries
    m_dll.Load( NGSPICE_DLL_FILE, wxDL_VERBATIM | wxDL_QUIET | wxDL_NOW );

    // If failed, try some other paths:
    if( !m_dll.IsLoaded() )
    {
        for( const auto& path : dllPaths )
        {
            dllFile.SetPath( path );
            wxLogTrace( traceNgspice, "libngspice search path: %s", dllFile.GetFullPath() );
            m_dll.Load( dllFile.GetFullPath(), wxDL_VERBATIM | wxDL_QUIET | wxDL_NOW );

            if( m_dll.IsLoaded() )
            {
                wxLogTrace( traceNgspice, "libngspice path found in: %s", dllFile.GetFullPath() );
                break;
            }
        }
    }
#endif

    if( !m_dll.IsLoaded() )
        throw std::runtime_error( _( "Unable to load ngspice shared library. Please check your install." ).ToStdString() );

    m_error = false;

    // Obtain function pointers
    m_ngSpice_Init = (ngSpice_Init) m_dll.GetSymbol( "ngSpice_Init" );
    m_ngSpice_Circ = (ngSpice_Circ) m_dll.GetSymbol( "ngSpice_Circ" );
    m_ngSpice_Command = (ngSpice_Command) m_dll.GetSymbol( "ngSpice_Command" );
    m_ngGet_Vec_Info = (ngGet_Vec_Info) m_dll.GetSymbol( "ngGet_Vec_Info" );
    m_ngCM_Input_Path = (ngCM_Input_Path) m_dll.GetSymbol( "ngCM_Input_Path" );
    m_ngSpice_CurPlot  = (ngSpice_CurPlot) m_dll.GetSymbol( "ngSpice_CurPlot" );
    m_ngSpice_AllPlots = (ngSpice_AllPlots) m_dll.GetSymbol( "ngSpice_AllPlots" );
    m_ngSpice_AllVecs = (ngSpice_AllVecs) m_dll.GetSymbol( "ngSpice_AllVecs" );
    m_ngSpice_Running = (ngSpice_Running) m_dll.GetSymbol( "ngSpice_running" ); // it is not a typo

    if( m_dll.HasSymbol( "ngSpice_LockRealloc" ) )
    {
        m_ngSpice_LockRealloc = (ngSpice_LockRealloc) m_dll.GetSymbol( "ngSpice_LockRealloc" );
        m_ngSpice_UnlockRealloc = (ngSpice_UnlockRealloc) m_dll.GetSymbol( "ngSpice_UnlockRealloc" );
    }

    m_ngSpice_Init( &cbSendChar, &cbSendStat, &cbControlledExit, nullptr, nullptr,
                    &cbBGThreadRunning, this );

    // Load a custom spinit file, to fix the problem with loading .cm files
    // Switch to the executable directory, so the relative paths are correct
    wxString cwd( wxGetCwd() );
    wxFileName exeDir( stdPaths.GetExecutablePath() );
    wxSetWorkingDirectory( exeDir.GetPath() );

    // Find *.cm files
    std::string cmPath = findCmPath();

    // __CMPATH is used in custom spinit file to point to the codemodels directory
    if( !cmPath.empty() )
        Command( "set __CMPATH=\"" + cmPath + "\"" );

    // Possible relative locations for spinit file
    const std::vector<std::string> spiceinitPaths =
    {
        ".",
#ifdef __WXMAC__
        stdPaths.GetPluginsDir().ToStdString() + "/sim/ngspice/scripts",
        wxFileName( stdPaths.GetExecutablePath() ).GetPath().ToStdString() +
                "/../../../../../Contents/PlugIns/sim/ngspice/scripts"
#endif
        "../share/kicad",
        "../share",
        "../../share/kicad",
        "../../share"
    };

    bool foundSpiceinit = false;

    for( const auto& path : spiceinitPaths )
    {
        wxLogTrace( traceNgspice, "ngspice init script search path: %s", path );

        if( loadSpinit( path + "/spiceinit" ) )
        {
            wxLogTrace( traceNgspice, "ngspice path found in: %s", path );
            foundSpiceinit = true;
            break;
        }
    }

    // Last chance to load codemodel files, we have not found
    // spiceinit file, but we know the path to *.cm files
    if( !foundSpiceinit && !cmPath.empty() )
        loadCodemodels( cmPath );

    // Restore the working directory
    wxSetWorkingDirectory( cwd );

    // Workarounds to avoid hang ups on certain errors
    // These commands have to be called, no matter what is in the spinit file
    // We have to allow interactive for user-defined signals.  Hopefully whatever bug this was
    // meant to address has gone away in the last 5 years...
    //Command( "unset interactive" );
    Command( "set noaskquit" );
    Command( "set nomoremode" );

    // reset and remcirc give an error if no circuit is loaded, so load an empty circuit at the
    // start.

    std::vector<char*> lines;
    lines.push_back( strdup( "*" ) );
    lines.push_back( strdup( ".end" ) );
    lines.push_back( nullptr ); // Sentinel.

    m_ngSpice_Circ( lines.data() );

    for( auto line : lines )
        free( line );

    m_initialized = true;
}


bool NGSPICE::loadSpinit( const std::string& aFileName )
{
    if( !wxFileName::FileExists( aFileName ) )
        return false;

    wxTextFile file;

    if( !file.Open( aFileName ) )
        return false;

    for( wxString& cmd = file.GetFirstLine(); !file.Eof(); cmd = file.GetNextLine() )
        Command( cmd.ToStdString() );

    return true;
}


std::string NGSPICE::findCmPath() const
{
    const std::vector<std::string> cmPaths =
    {
#ifdef __WXMAC__
        "/Applications/ngspice/lib/ngspice",
        "Contents/Frameworks",
        wxStandardPaths::Get().GetPluginsDir().ToStdString() + "/sim/ngspice",
        wxFileName( wxStandardPaths::Get().GetExecutablePath() ).GetPath().ToStdString() +
                "/../../../../../Contents/PlugIns/sim/ngspice",
        "../Plugins/sim/ngspice",
#endif
        "../eeschema/ngspice",
        "../lib/ngspice",
        "../../lib/ngspice",
        "lib/ngspice",
        "ngspice"
    };

    for( const auto& path : cmPaths )
    {
        wxLogTrace( traceNgspice, "ngspice code models search path: %s", path );

        if( wxFileName::FileExists( path + "/spice2poly.cm" ) )
        {
            wxLogTrace( traceNgspice, "ngspice code models found in: %s", path );
            return path;
        }
    }

    return std::string();
}


bool NGSPICE::setCodemodelsInputPath( const std::string& aPath )
{
    if( !m_ngCM_Input_Path )
        return false;

    LOCALE_IO c_locale; // ngspice works correctly only with C locale

    m_ngCM_Input_Path( aPath.c_str() );

    return true;
}


bool NGSPICE::loadCodemodels( const std::string& aPath )
{
    wxArrayString cmFiles;
    size_t count = wxDir::GetAllFiles( aPath, &cmFiles );

    for( const auto& cm : cmFiles )
        Command( fmt::format( "codemodel '{}'", cm.ToStdString() ) );

    return count != 0;
}


int NGSPICE::cbSendChar( char* aWhat, int aId, void* aUser )
{
    NGSPICE* sim = reinterpret_cast<NGSPICE*>( aUser );

    if( sim->m_reporter )
    {
        // strip stdout/stderr from the line
        if( ( strncasecmp( aWhat, "stdout ", 7 ) == 0 )
                || ( strncasecmp( aWhat, "stderr ", 7 ) == 0 ) )
        {
            aWhat += 7;
        }

        sim->m_reporter->Report( aWhat );
    }

    return 0;
}


int NGSPICE::cbSendStat( char *aWhat, int aId, void* aUser )
{
    return 0;
}


int NGSPICE::cbBGThreadRunning( NG_BOOL aFinished, int aId, void* aUser )
{
    NGSPICE* sim = reinterpret_cast<NGSPICE*>( aUser );

    if( sim->m_reporter )
        sim->m_reporter->OnSimStateChange( sim, aFinished ? SIM_IDLE : SIM_RUNNING );

    return 0;
}


int NGSPICE::cbControlledExit( int aStatus, NG_BOOL aImmediate, NG_BOOL aExitOnQuit, int aId,
                               void* aUser )
{
    // Something went wrong, reload the dll
    NGSPICE* sim = reinterpret_cast<NGSPICE*>( aUser );
    sim->m_error = true;

    return 0;
}


void NGSPICE::validate()
{
    if( m_error )
    {
        m_initialized = false;
        init_dll();
    }
}


void NGSPICE::Clean()
{
    Command( "destroy all" );
}


bool NGSPICE::m_initialized = false;
