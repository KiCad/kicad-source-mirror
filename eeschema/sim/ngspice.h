/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2016-2022 CERN
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * @author Tomasz Wlostowski <tomasz.wlostowski@cern.ch>
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

#ifndef NGSPICE_H
#define NGSPICE_H

#include <sim/spice_simulator.h>
#include <sim/sim_model.h>
#include <sim/sim_value.h>

#include <wx/dynlib.h>

#include <ngspice/sharedspice.h>

#include <enum_vector.h>

#include <atomic>

#if defined(__WINDOWS__)
struct _EXCEPTION_POINTERS;
#endif

// We have an issue here where NGSPICE incorrectly used bool for years
// and defined it to be int when in C-mode.  We cannot adjust the function
// signatures without re-writing sharedspice.h for KiCad.
// Instead, we maintain status-quo for older NGSPICE versions (<=34) and
// use the new signatures for newer versions
#ifndef NGSPICE_PACKAGE_VERSION
    typedef bool NG_BOOL;
#endif

class wxDynamicLibrary;


class NGSPICE : public SPICE_SIMULATOR
{
public:
    NGSPICE();
    virtual ~NGSPICE();

    ///< @copydoc SPICE_SIMULATOR::Init()
    void Init( const SPICE_SETTINGS* aSettings = nullptr ) override final;

    ///< @copydoc SPICE_SIMULATOR::Attach()
    bool Attach( const std::shared_ptr<SIMULATION_MODEL>& aModel, const wxString& aSimCommand,
                 unsigned aSimOptions, const wxString& aInputPath,
                 REPORTER& aReporter ) override final;

    ///< Load a netlist for the simulation
    bool LoadNetlist( const std::string& aNetlist ) override final;

    ///< @copydoc SPICE_SIMULATOR::Run()
    bool Run() override final;

    ///< @copydoc SPICE_SIMULATOR::Stop()
    bool Stop() override final;

    ///< @copydoc SPICE_SIMULATOR::IsRunning()
    bool IsRunning() override final;

    ///< @copydoc SPICE_SIMULATOR::Command()
    bool Command( const std::string& aCmd ) override final;

    ///< @copydoc SPICE_SIMULATOR::GetXAxis()
    wxString GetXAxis( SIM_TYPE aType ) const override final;

    ///< @copydoc SPICE_SIMULATOR::CurrentPlotName()
    wxString CurrentPlotName() const override final;

    ///< @copydoc SPICE_SIMULATOR::AllVectors()
    std::vector<std::string> AllVectors() const override final;

    ///< @copydoc SPICE_SIMULATOR::GetComplexVector()
    std::vector<COMPLEX> GetComplexVector( const std::string& aName, int aMaxLen = -1 ) override final;

    ///< @copydoc SPICE_SIMULATOR::GetRealVector()
    std::vector<double> GetRealVector( const std::string& aName, int aMaxLen = -1 ) override final;

    ///< @copydoc SPICE_SIMULATOR::GetImaginaryVector()
    std::vector<double> GetImaginaryVector( const std::string& aName, int aMaxLen = -1 ) override final;

    ///< @copydoc SPICE_SIMULATOR::GetGainVector()
    std::vector<double> GetGainVector( const std::string& aName, int aMaxLen = -1 ) override final;

    ///< @copydoc SPICE_SIMULATOR::GetPhaseVector()
    std::vector<double> GetPhaseVector( const std::string& aName, int aMaxLen = -1 ) override final;

    std::vector<std::string> GetSettingCommands() const override final;

    ///< @copydoc SPICE_SIMULATOR::GetNetlist()
    virtual const std::string GetNetlist() const override final;

    ///< @copydoc SIMULATOR::Clean()
    void Clean() override final;

private:
    // Performs DLL initialization, obtains function pointers
    void init_dll();

    // ngspice library functions
    typedef void ( *ngSpice_Init )( SendChar*, SendStat*, ControlledExit*, SendData*, SendInitData*,
                                    BGThreadRunning*, void* );

    typedef int          ( *ngSpice_Circ )( char** circarray );
    typedef int          ( *ngSpice_Command )( char* command );
    typedef pvector_info ( *ngGet_Vec_Info )( char* vecname );
    typedef char*        ( *ngCM_Input_Path )( const char* path );
    typedef char*        ( *ngSpice_CurPlot )( void );
    typedef char**       ( *ngSpice_AllPlots )( void );
    typedef char**       ( *ngSpice_AllVecs )( char* plotname );
    typedef bool         ( *ngSpice_Running )( void );
    typedef int          ( *ngSpice_LockRealloc )( void );
    typedef int          ( *ngSpice_UnlockRealloc )( void );

    ///< Handle to DLL functions
    ngSpice_Init          m_ngSpice_Init;
    ngSpice_Circ          m_ngSpice_Circ;
    ngSpice_Command       m_ngSpice_Command;
    ngGet_Vec_Info        m_ngGet_Vec_Info;
    ngCM_Input_Path       m_ngCM_Input_Path;
    ngSpice_CurPlot       m_ngSpice_CurPlot;
    ngSpice_AllPlots      m_ngSpice_AllPlots;
    ngSpice_AllVecs       m_ngSpice_AllVecs;
    ngSpice_Running       m_ngSpice_Running;
    ngSpice_LockRealloc   m_ngSpice_LockRealloc;
    ngSpice_UnlockRealloc m_ngSpice_UnlockRealloc;

    wxDynamicLibrary m_dll;

    class NGSPICE_LOCK_REALLOC
    {
    public:
        NGSPICE_LOCK_REALLOC( NGSPICE* ngspice ) :
                m_ngspice( ngspice )
        {
            if( m_ngspice->m_ngSpice_LockRealloc )
                m_ngspice->m_ngSpice_LockRealloc();
        };

        ~NGSPICE_LOCK_REALLOC()
        {
            if( m_ngspice->m_ngSpice_UnlockRealloc )
                m_ngspice->m_ngSpice_UnlockRealloc();
        };

    private:
        NGSPICE* m_ngspice;
    };

    ///< Execute commands from a file
    bool loadSpinit( const std::string& aFileName );

    void updateNgspiceSettings();

    ///< Check a few different locations for codemodel files and returns one if it exists.
    std::string findCmPath() const;

    ///< Send additional search path for codemodels to ngspice.
    bool setCodemodelsInputPath( const std::string& aPath );

    ///< Load codemodel files from a directory.
    bool loadCodemodels( const std::string& aPath );

    // Callback functions
    static int cbSendChar( char* what, int aId, void* aUser );
    static int cbSendStat( char* what, int aId, void* aUser );
    static int cbBGThreadRunning( NG_BOOL aFinished, int aId, void* aUser );
    static int cbControlledExit( int aStatus, NG_BOOL aImmediate, NG_BOOL aExitOnQuit, int aId,
                                 void* aUser );

    // Assure ngspice is in a valid state and reinitializes it if need be.
    void validate();

    // Install signal handlers to catch ngspice crashes
    void installSignalHandlers();

    // Restore original signal handlers
    void restoreSignalHandlers();

    // Signal handler for crashes
    static void signalHandler( int aSignal );

#if defined(__WINDOWS__)
    // Structured exception handler for Windows crashes
    static long __stdcall sehHandler( struct _EXCEPTION_POINTERS* aException );
#endif

private:
    bool        m_error;            ///< Error flag indicating that ngspice needs to be reloaded.

    static bool m_initialized;      ///< Ngspice should be initialized only once.

    std::string m_netlist;          ///< Current netlist

    static std::atomic<bool>    s_crashed;          ///< Set by signal handler when ngspice crashes.
    static std::atomic<int>     s_crashSignal;      ///< Signal that caused the crash.
    static NGSPICE*             s_currentInstance;  ///< Instance that is currently running ngspice.
};

#endif /* NGSPICE_H */
