/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "ace_backend.h"

#include "db_backend_support.h"

#include <algorithm>
#include <atomic>
#include <charconv>
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <cwctype>
#include <future>
#include <map>
#include <memory>
#include <mutex>
#include <optional>
#include <random>
#include <system_error>
#include <thread>
#include <variant>

#include <fmt/format.h>

#include <wx/intl.h>
#include <wx/log.h>
#include <wx/string.h>

#include <database/access_fast_exit.h>
#include <database/database_value.h>
#include <database/file_in_use_backoff.h>

#include "access_engine_mutex.h"

#include <windows.h>
#include <objbase.h>
#include <oleauto.h>
#include <winnetwk.h>

using namespace std::chrono_literals;

namespace
{

// Values of the ADO enumerations this file uses
constexpr long adModeRead = 1;
constexpr long adModeShareDenyNone = 16;
constexpr long adCmdText = 1;
constexpr long adSchemaColumns = 4;
constexpr long adSchemaTables = 20;
constexpr long GET_ROWS_BATCH = static_cast<long>( DB_JOB_CONTEXT::BATCH_ROWS );

constexpr wchar_t PROG_ID_CURRENT[] = L"Microsoft.ACE.OLEDB.16.0";
constexpr wchar_t PROG_ID_FALLBACK[] = L"Microsoft.ACE.OLEDB.12.0";


std::wstring toWide( const std::string& aUtf8 )
{
    return std::wstring( wxString::FromUTF8( aUtf8 ).wc_str() );
}


std::string toUtf8( const wchar_t* aWide, size_t aLength )
{
    return std::string( wxString( aWide, aLength ).ToUTF8() );
}


std::string quoted( const std::string& aIdentifier )
{
    // Access names cannot hold a bracket, so no escaping is needed
    return "[" + aIdentifier + "]";
}


/// The select list naming aColumns, or every column when it is empty
std::string selectList( const std::vector<std::string>& aColumns )
{
    if( aColumns.empty() )
        return "*";

    std::string list;

    for( size_t i = 0; i < aColumns.size(); ++i )
        list += ( i ? ", " : "" ) + quoted( aColumns[i] );

    return list;
}


/// What went wrong in a COM or ADO call, with the provider's text when it gave one
struct ADO_ERROR
{
    HRESULT     hr = E_FAIL;
    std::string text;
    std::string sqlState = ""; ///< The engine's own error number as ADO reports it, such as 3031
};


std::string hresultText( HRESULT aHr )
{
    wchar_t* buffer = nullptr;
    DWORD    length = FormatMessageW( FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM
                                              | FORMAT_MESSAGE_IGNORE_INSERTS,
                                      nullptr, static_cast<DWORD>( aHr ), 0, reinterpret_cast<LPWSTR>( &buffer ), 0,
                                      nullptr );
    std::string text;

    if( length && buffer )
        text = toUtf8( buffer, length );
    else
        text = fmt::format( "HRESULT 0x{:08X}", static_cast<unsigned long>( aHr ) );

    if( buffer )
        LocalFree( buffer );

    while( !text.empty() && ( text.back() == '\r' || text.back() == '\n' || text.back() == ' ' ) )
        text.pop_back();

    return text;
}


/// Owns one VARIANT
class VAR
{
public:
    VAR() { VariantInit( &m_v ); }

    ~VAR() { VariantClear( &m_v ); }

    VAR( const VAR& ) = delete;
    VAR& operator=( const VAR& ) = delete;

    VAR( VAR&& aOther ) noexcept
    {
        m_v = aOther.m_v;
        VariantInit( &aOther.m_v );
    }

    VAR& operator=( VAR&& aOther ) noexcept
    {
        if( this != &aOther )
        {
            VariantClear( &m_v );
            m_v = aOther.m_v;
            VariantInit( &aOther.m_v );
        }

        return *this;
    }

    static VAR Str( const std::wstring& aText )
    {
        VAR v;

        V_VT( &v.m_v ) = VT_BSTR;
        V_BSTR( &v.m_v ) = SysAllocStringLen( aText.data(), static_cast<UINT>( aText.size() ) );
        return v;
    }

    static VAR Long( long aValue )
    {
        VAR v;

        V_VT( &v.m_v ) = VT_I4;
        V_I4( &v.m_v ) = aValue;
        return v;
    }

    /// An omitted optional argument
    static VAR Missing()
    {
        VAR v;

        V_VT( &v.m_v ) = VT_ERROR;
        V_ERROR( &v.m_v ) = DISP_E_PARAMNOTFOUND;
        return v;
    }

    /// A criteria array for OpenSchema holding only the third restriction, which is the table name
    static VAR TableCriteria( const std::wstring& aTable )
    {
        SAFEARRAYBOUND bound = { 3, 0 };
        SAFEARRAY*     array = SafeArrayCreate( VT_VARIANT, 1, &bound );

        if( !array )
            throw ADO_ERROR{ E_OUTOFMEMORY, std::string() };

        VAR   name = Str( aTable );
        LONG  index = 2;
        VAR   v;

        V_VT( &v.m_v ) = VT_ARRAY | VT_VARIANT;
        V_ARRAY( &v.m_v ) = array;

        const HRESULT hr = SafeArrayPutElement( array, &index, &name.m_v );

        if( FAILED( hr ) )
            throw ADO_ERROR{ hr, std::string() };

        return v;
    }

    const VARIANT& Raw() const { return m_v; }

    VARIANT* Out() { return &m_v; }

    bool AsBool() const
    {
        VAR copy;
    #if defined (__MINGW32__)
        if( FAILED( VariantChangeTypeEx( &copy.m_v, const_cast<VARIANT*>(&m_v), LOCALE_INVARIANT, 0, VT_BOOL ) ) )
    #else
        if( FAILED( VariantChangeTypeEx( &copy.m_v, &m_v, LOCALE_INVARIANT, 0, VT_BOOL ) ) )
    #endif
            return false;

        return V_BOOL( &copy.m_v ) != VARIANT_FALSE;
    }

    long AsLong() const
    {
        VAR copy;

    #if defined (__MINGW32__)
        if( FAILED( VariantChangeTypeEx( &copy.m_v, const_cast<VARIANT*>(&m_v), LOCALE_INVARIANT, 0, VT_I4 ) ) )
    #else
        if( FAILED( VariantChangeTypeEx( &copy.m_v, &m_v, LOCALE_INVARIANT, 0, VT_I4 ) ) )
    #endif
        return 0;

        return V_I4( &copy.m_v );
    }

    std::string AsString() const
    {
        VAR copy;

    #if defined (__MINGW32__)
        if( FAILED( VariantChangeTypeEx( &copy.m_v, const_cast<VARIANT*>(&m_v), LOCALE_INVARIANT, 0, VT_BSTR ) ) )
    #else
        if( FAILED( VariantChangeTypeEx( &copy.m_v, &m_v, LOCALE_INVARIANT, 0, VT_BSTR ) ) )
    #endif
            return std::string();

        const BSTR text = V_BSTR( &copy.m_v );

        return text ? toUtf8( text, SysStringLen( text ) ) : std::string();
    }

    IDispatch* Dispatch() const { return V_VT( &m_v ) == VT_DISPATCH ? V_DISPATCH( &m_v ) : nullptr; }

private:
    VARIANT m_v;
};


/// Owns one IDispatch and calls its members by name
class DISP
{
public:
    DISP() = default;

    explicit DISP( IDispatch* aOwned ) : m_p( aOwned ) {}

    ~DISP() { Reset(); }

    DISP( const DISP& ) = delete;
    DISP& operator=( const DISP& ) = delete;

    DISP( DISP&& aOther ) noexcept : m_p( aOther.m_p ), m_ids( std::move( aOther.m_ids ) ) { aOther.m_p = nullptr; }

    DISP& operator=( DISP&& aOther ) noexcept
    {
        if( this != &aOther )
        {
            Reset();
            m_p = aOther.m_p;
            m_ids = std::move( aOther.m_ids );
            aOther.m_p = nullptr;
        }

        return *this;
    }

    static DISP Create( const wchar_t* aProgId )
    {
        CLSID   clsid;
        HRESULT hr = CLSIDFromProgID( aProgId, &clsid );

        if( FAILED( hr ) )
            throw ADO_ERROR{ hr, std::string() };

        IDispatch* object = nullptr;

        hr = CoCreateInstance( clsid, nullptr, CLSCTX_INPROC_SERVER, IID_IDispatch,
                               reinterpret_cast<void**>( &object ) );

        if( FAILED( hr ) )
            throw ADO_ERROR{ hr, std::string() };

        return DISP( object );
    }

    /// Takes the object a VARIANT holds
    static DISP From( const VAR& aValue )
    {
        IDispatch* object = aValue.Dispatch();

        if( !object )
            throw ADO_ERROR{ E_POINTER, std::string() };

        object->AddRef();
        return DISP( object );
    }

    void Reset()
    {
        if( m_p )
        {
            m_p->Release();
            m_p = nullptr;
        }

        m_ids.clear();
    }

    /// The arguments must outlive the call, which a temporary does until the end of the statement
    template <typename... ARGS>
    VAR Property( const wchar_t* aName, const ARGS&... aArgs )
    {
        return invoke( aName, DISPATCH_PROPERTYGET, { aArgs.Raw()... } );
    }

    void SetProperty( const wchar_t* aName, const VAR& aValue )
    {
        invoke( aName, DISPATCH_PROPERTYPUT, { aValue.Raw() } );
    }

    template <typename... ARGS>
    VAR Call( const wchar_t* aName, const ARGS&... aArgs )
    {
        return invoke( aName, DISPATCH_METHOD, { aArgs.Raw()... } );
    }

private:
    VAR invoke( const wchar_t* aName, WORD aFlags, std::vector<VARIANT> aArgs )
    {
        auto found = m_ids.find( aName );

        if( found == m_ids.end() )
        {
            DISPID  id = 0;
            LPOLESTR name = const_cast<LPOLESTR>( aName );
            HRESULT hr = m_p->GetIDsOfNames( IID_NULL, &name, 1, LOCALE_USER_DEFAULT, &id );

            if( FAILED( hr ) )
                throw ADO_ERROR{ hr, std::string() };

            found = m_ids.emplace( aName, id ).first;
        }

        // IDispatch takes the arguments last to first
        std::vector<VARIANT> raw( aArgs.rbegin(), aArgs.rend() );

        DISPPARAMS params = {};
        DISPID     putId = DISPID_PROPERTYPUT;

        params.cArgs = static_cast<UINT>( raw.size() );
        params.rgvarg = raw.empty() ? nullptr : raw.data();

        if( aFlags == DISPATCH_PROPERTYPUT )
        {
            params.cNamedArgs = 1;
            params.rgdispidNamedArgs = &putId;
        }

        VAR       result;
        EXCEPINFO info = {};
        UINT      argError = 0;
        HRESULT   hr = m_p->Invoke( found->second, IID_NULL, LOCALE_USER_DEFAULT, aFlags, &params, result.Out(),
                                    &info, &argError );

        if( FAILED( hr ) )
        {
            if( hr == DISP_E_EXCEPTION && info.pfnDeferredFillIn )
                info.pfnDeferredFillIn( &info );

            ADO_ERROR error{ hr == DISP_E_EXCEPTION && FAILED( info.scode ) ? info.scode : hr, std::string() };

            if( info.bstrDescription )
                error.text = toUtf8( info.bstrDescription, SysStringLen( info.bstrDescription ) );

            SysFreeString( info.bstrSource );
            SysFreeString( info.bstrDescription );
            SysFreeString( info.bstrHelpFile );
            throw error;
        }

        return result;
    }

    IDispatch*                   m_p = nullptr;
    std::map<std::wstring, DISPID> m_ids;
};


/// Empties the connection's Errors collection, so that an older error is never taken for a newer failure
void clearProviderErrors( DISP& aConnection )
{
    try
    {
        DISP errors = DISP::From( aConnection.Property( L"Errors" ) );

        errors.Call( L"Clear" );
    }
    catch( const ADO_ERROR& )
    {
    }
}


/// Takes the text and error number from the connection's Errors collection, which names the real cause, and
/// empties the collection
void applyProviderErrors( DISP& aConnection, ADO_ERROR& aError )
{
    std::string text;
    std::string sqlState;

    try
    {
        DISP errors = DISP::From( aConnection.Property( L"Errors" ) );
        long count = errors.Property( L"Count" ).AsLong();

        for( long i = 0; i < count; ++i )
        {
            DISP        error = DISP::From( errors.Property( L"Item", VAR::Long( i ) ) );
            std::string description = error.Property( L"Description" ).AsString();

            if( sqlState.empty() )
                sqlState = error.Property( L"SQLState" ).AsString();

            if( description.empty() || text.find( description ) != std::string::npos )
                continue;

            text += ( text.empty() ? "" : "; " ) + description;
        }
    }
    catch( const ADO_ERROR& )
    {
    }

    clearProviderErrors( aConnection );

    if( !text.empty() )
        aError.text = text;

    aError.sqlState = sqlState;
}


/// The outcome of a call made on the worker; text for the user is composed later, on the caller's thread
struct CALL_RESULT
{
    bool        ok = true;
    DB_FAILURE  failure = DB_FAILURE::UNKNOWN;
    std::string detail;             ///< The provider's message, or what the job found
    bool        engineBusy = false; ///< The engine lock stayed out of reach, so nothing was tried
};


CALL_RESULT failed( DB_FAILURE aFailure, std::string aDetail )
{
    return { false, aFailure, std::move( aDetail ) };
}


/// What a catalog call found
struct CATALOG_RESULT
{
    CALL_RESULT              call;
    std::vector<std::string> names;
    std::string              table;
    bool                     found = false;
    std::vector<DB_COLUMN>   columns;
};


bool isNetworkError( DWORD aCode )
{
    switch( aCode )
    {
    case ERROR_BAD_NETPATH:
    case ERROR_BAD_NET_NAME:
    case ERROR_NETNAME_DELETED:
    case ERROR_UNEXP_NET_ERR:
    case ERROR_NETWORK_BUSY:
    case ERROR_NETWORK_UNREACHABLE:
    case ERROR_HOST_UNREACHABLE:
    case ERROR_CONNECTION_UNAVAIL:
    case ERROR_NO_NETWORK:
    case ERROR_REM_NOT_LIST:
    case ERROR_DEV_NOT_EXIST:
    case ERROR_SEM_TIMEOUT:
    case ERROR_NOT_CONNECTED:
        return true;

    default:
        return false;
    }
}


/// The HRESULTs of a provider that is gone, such as one uninstalled after the first load
bool isProviderMissing( HRESULT aHr )
{
    constexpr HRESULT ADO_PROVIDER_NOT_FOUND = static_cast<HRESULT>( 0x800A0E7A );

    return aHr == ADO_PROVIDER_NOT_FOUND || aHr == REGDB_E_CLASSNOTREG || aHr == CO_E_CLASSSTRING
           || aHr == HRESULT_FROM_WIN32( ERROR_MOD_NOT_FOUND );
}


/// Maps the engine's own error number to a failure
std::optional<DB_FAILURE> classifyByNumber( const std::string& aSqlState )
{
    int  number = 0;
    auto [end, code] = std::from_chars( aSqlState.data(), aSqlState.data() + aSqlState.size(), number );

    if( code != std::errc() || end != aSqlState.data() + aSqlState.size() )
        return std::nullopt;

    switch( number )
    {
    case 3031: return DB_FAILURE::LOGIN;           // not a valid password
    case 3024: return DB_FAILURE::FILE_NOT_FOUND;  // could not find file
    case 3043:                                     // disk or network error
    case 3044: return DB_FAILURE::UNREACHABLE;     // not a valid path
    case 3006:                                     // exclusively locked
    case 3045:                                     // file already in use
    case 3050:                                     // could not lock file
    case 3051:                                     // opened exclusively by another user, or no permission
    case 3356:                                     // opened by another user in a way that conflicts
    case 3704:                                     // already opened exclusively
    case 3734: return DB_FAILURE::FILE_IN_USE;     // placed in a state that prevents opening or locking
    default: return DB_FAILURE::UNKNOWN;
    }
}


/// Whole messages of the engine that name nothing, so an echoed identifier cannot imitate them
struct FIXED_MESSAGE
{
    const wchar_t* text;
    DB_FAILURE     failure;
};


const FIXED_MESSAGE FIXED_MESSAGES[] = {
    { L"not a valid password.", DB_FAILURE::LOGIN },
    { L"could not use ''; file already in use.", DB_FAILURE::FILE_IN_USE },
    { L"could not lock file.", DB_FAILURE::FILE_IN_USE },
    { L"disk or network error.", DB_FAILURE::UNREACHABLE },
};


/// Maps a provider error to a failure, by number, then by HRESULT, then by a whole fixed message
DB_FAILURE classifyFailure( const ADO_ERROR& aError, const std::string& aText )
{
    if( std::optional<DB_FAILURE> byNumber = classifyByNumber( aError.sqlState ) )
        return *byNumber;

    if( isProviderMissing( aError.hr ) )
        return DB_FAILURE::PROVIDER_MISSING;

    if( HRESULT_FACILITY( aError.hr ) == FACILITY_WIN32 && isNetworkError( HRESULT_CODE( aError.hr ) ) )
        return DB_FAILURE::UNREACHABLE;

    std::wstring lower = toWide( aText );

    std::transform( lower.begin(), lower.end(), lower.begin(),
                    []( wchar_t c )
                    {
                        return static_cast<wchar_t>( std::towlower( c ) );
                    } );

    for( const FIXED_MESSAGE& message : FIXED_MESSAGES )
    {
        if( lower == message.text )
            return message.failure;
    }

    return DB_FAILURE::UNKNOWN;
}


/// Everything a connection owns; lives and dies on the COM thread that made it
struct ADO_SESSION
{
    DISP connection;
};


// COM objects belong to the thread that made them, so the sessions of a backend stay in its worker's thread
// Built on first use, since a namespace-scope thread_local would be built on every thread of the process
std::map<uint64_t, ADO_SESSION>& sessions()
{
    thread_local std::map<uint64_t, ADO_SESSION> s_sessions;
    return s_sessions;
}

std::atomic<uint64_t> g_nextSession{ 1 };


ADO_SESSION& sessionFor( uint64_t aSession )
{
    auto it = sessions().find( aSession );

    if( it == sessions().end() )
        throw ADO_ERROR{ HRESULT_FROM_WIN32( ERROR_CONNECTION_UNAVAIL ), "the connection to the database was lost" };

    return it->second;
}


/// Runs aBody and turns what it throws into a failed result; aSession names the connection whose provider errors
/// explain a failure after the open
template <typename BODY>
CALL_RESULT guarded( BODY aBody, uint64_t aSession = 0 )
{
    try
    {
        auto opened = sessions().find( aSession );

        if( opened != sessions().end() )
            clearProviderErrors( opened->second.connection );

        return aBody();
    }
    catch( ADO_ERROR& error )
    {
        auto session = sessions().find( aSession );

        if( error.sqlState.empty() && session != sessions().end() )
            applyProviderErrors( session->second.connection, error );

        const std::string text = error.text.empty() ? hresultText( error.hr ) : error.text;
        const DB_FAILURE  failure = classifyFailure( error, text );

        wxLogTrace( traceDatabase, wxT( "ACE_BACKEND: call failed (0x%08lx, %s) as failure %d" ),
                    static_cast<unsigned long>( error.hr ), wxString::FromUTF8( error.sqlState ),
                    static_cast<int>( failure ) );

        return failed( failure, text );
    }
    catch( const std::exception& error )
    {
        return failed( DB_FAILURE::UNKNOWN, error.what() );
    }
}


bool comThreadInit()
{
    return SUCCEEDED( CoInitializeEx( nullptr, COINIT_APARTMENTTHREADED ) );
}


/// The operation whose job this worker thread ran last, which its exit hook waits out
thread_local std::shared_ptr<ACE_ENGINE_OP> s_threadOp;


/// Takes the engine lock until the operation's deadline, and records in the operation how the wait ended
std::unique_lock<std::timed_mutex> lockEngine( ACE_ENGINE_OP& aOp )
{
    // An expired operation stays expired, so a later close of it neither waits again nor loses the timeout
    if( aOp.state == ACE_ENGINE_OP::STATE::EXPIRED )
        return {};

    aOp.state = ACE_ENGINE_OP::STATE::WAITING;

    std::unique_lock<std::timed_mutex> lock = LockAccessEngine( aOp.deadline );

    aOp.state = lock.owns_lock() ? ACE_ENGINE_OP::STATE::ACQUIRED : ACE_ENGINE_OP::STATE::EXPIRED;
    return lock;
}


/// The time left until aDeadline, rounded up so a wait that ends at the deadline never ends before it
std::chrono::milliseconds remainingUntil( ACE_ENGINE_OP::CLOCK::time_point aDeadline )
{
    return std::max( std::chrono::milliseconds( 0 ),
                     std::chrono::ceil<std::chrono::milliseconds>( aDeadline - ACE_ENGINE_OP::CLOCK::now() ) );
}


// A session left open by a close that lost the lock is never released, so its thread never unloads the provider
thread_local bool s_leftSessionOpen = false;


void comThreadExit()
{
    // A thread that never ran a job has no operation to wait out, so it tries the lock once
    const ACE_ENGINE_OP::CLOCK::time_point deadline =
            s_threadOp ? s_threadOp->deadline : ACE_ENGINE_OP::CLOCK::time_point();

    // Dropping the last reference to the engine unloads Office libraries, which must not overlap a load elsewhere
    std::unique_lock lock = LockAccessEngine( deadline );

    if( !lock.owns_lock() || s_leftSessionOpen )
    {
        // Freeing the sessions unlocked would race the unload, so they stay for good and the thread simply ends
        MarkAccessWorkerAbandoned();
        new std::map<uint64_t, ADO_SESSION>( std::move( sessions() ) );
        return;
    }

    sessions().clear();
    CoUninitialize();
}


void closeSession( uint64_t aSession, ACE_ENGINE_OP& aOp )
{
    auto it = sessions().find( aSession );

    if( it == sessions().end() )
        return;

    std::unique_lock lock = lockEngine( aOp );

    if( !lock.owns_lock() )
    {
        // Closing unlocked would race the engine unload, so the session stays open for good
        MarkAccessWorkerAbandoned();
        new ADO_SESSION( std::move( it->second ) );
        sessions().erase( it );
        s_leftSessionOpen = true;
        return;
    }

    try
    {
        it->second.connection.Call( L"Close" );
    }
    catch( const ADO_ERROR& )
    {
    }

    sessions().erase( it );
}


/// What the provider probe found; an error means the probe itself failed, so the provider counts as missing
struct PROBE_ANSWER
{
    std::wstring progId;
    std::string  error;
    bool         busy = false; ///< The engine lock stayed out of reach, so nothing was probed
};


/// How long Connect waits for the probe, and how long the probe waits for the engine, which another Access load may
/// be holding up
constexpr std::chrono::seconds PROBE_WAIT{ 10 };


/// Looks for a provider that loads; runs on a COM thread of its own
PROBE_ANSWER findProvider()
{
    PROBE_ANSWER found;

    try
    {
        // The lock is declared first so that it outlives CoUninitialize, which can unload the engine
        std::unique_lock lock = LockAccessEngine( std::chrono::steady_clock::now() + PROBE_WAIT );

        if( !lock.owns_lock() )
        {
            found.busy = true;
            return found;
        }

        if( !comThreadInit() )
        {
            found.error = "COM could not be initialized";
            return found;
        }

        struct COM_SCOPE
        {
            ~COM_SCOPE() { CoUninitialize(); }
        } comScope;

        for( const wchar_t* candidate : { PROG_ID_CURRENT, PROG_ID_FALLBACK } )
        {
            CLSID clsid;

            if( FAILED( CLSIDFromProgID( candidate, &clsid ) ) )
                continue;

            IUnknown* provider = nullptr;

            if( SUCCEEDED( CoCreateInstance( clsid, nullptr, CLSCTX_INPROC_SERVER, IID_IUnknown,
                                             reinterpret_cast<void**>( &provider ) ) ) )
            {
                provider->Release();
                found.progId = candidate;
                break;
            }
        }
    }
    catch( const std::exception& error )
    {
        found.progId.clear();
        found.error = error.what();
        wxLogTrace( traceDatabase, wxT( "ACE_BACKEND: provider probe failed: %s" ), wxString::FromUTF8( found.error ) );
    }

    return found;
}


/// The probe's answer if it finished within aWait, which a finished answer does not need
std::optional<PROBE_ANSWER> probeAnswer( std::chrono::milliseconds aWait )
{
    static auto* s_mutex = new std::mutex;

    // Leaked so a probe thread still waiting on the engine never outlives its latch
    static DB_PROBE_LATCH<PROBE_ANSWER>* s_probe = new DB_PROBE_LATCH<PROBE_ANSWER>( findProvider );

    DB_PROBE_LATCH<PROBE_ANSWER>* probe;

    {
        std::lock_guard lock( *s_mutex );
        probe = s_probe;
    }

    std::optional<PROBE_ANSWER> answer = probe->Get( aWait );

    // A probe that never got the engine says nothing about the provider, so the next call probes afresh
    if( answer && answer->busy )
    {
        std::lock_guard lock( *s_mutex );

        if( s_probe == probe )
            s_probe = new DB_PROBE_LATCH<PROBE_ANSWER>( findProvider );
    }

    return answer;
}


/// Looks up the provider, or says the probe is still running
ACE_BACKEND::PROVIDER_STATE providerLookup( std::wstring& aProgId, std::chrono::milliseconds aWait )
{
    std::optional<PROBE_ANSWER> found = probeAnswer( aWait );

    if( !found )
        return ACE_BACKEND::PROVIDER_STATE::PENDING;

    if( found->busy )
        return ACE_BACKEND::PROVIDER_STATE::BUSY;

    aProgId = found->progId;
    return aProgId.empty() ? ACE_BACKEND::PROVIDER_STATE::MISSING : ACE_BACKEND::PROVIDER_STATE::AVAILABLE;
}


/// Turns a mapped drive path into the UNC path every process agrees on, so the lock file is shared correctly
std::wstring universalPath( const std::wstring& aPath )
{
    if( aPath.size() < 2 || aPath[1] != L':' )
        return aPath;

    DWORD             size = 1024;
    std::vector<BYTE> buffer( size );
    DWORD             code = WNetGetUniversalNameW( aPath.c_str(), UNIVERSAL_NAME_INFO_LEVEL, buffer.data(), &size );

    if( code == ERROR_MORE_DATA )
    {
        buffer.resize( size );
        code = WNetGetUniversalNameW( aPath.c_str(), UNIVERSAL_NAME_INFO_LEVEL, buffer.data(), &size );
    }

    if( code != NO_ERROR )
        return aPath;

    return reinterpret_cast<UNIVERSAL_NAME_INFOW*>( buffer.data() )->lpUniversalName;
}


/// An OLE DB initialization string value, quoted so semicolons and quotes in it stay part of the value
std::wstring quotedValue( const std::wstring& aValue )
{
    std::wstring result = L"\"";

    for( wchar_t c : aValue )
    {
        if( c == L'"' )
            result += L'"';

        result += c;
    }

    return result + L"\"";
}


CALL_RESULT connectJob( uint64_t aSession, const std::wstring& aProgId, const std::string& aPath,
                        const std::string& aPassword, int aTimeout, ACE_ENGINE_OP& aOp )
{
    return guarded(
            [&]() -> CALL_RESULT
            {
                const std::wstring path = universalPath( toWide( aPath ) );

                if( GetFileAttributesW( path.c_str() ) == INVALID_FILE_ATTRIBUTES )
                {
                    const DWORD code = GetLastError();

                    if( code == ERROR_FILE_NOT_FOUND || code == ERROR_PATH_NOT_FOUND )
                        return failed( DB_FAILURE::FILE_NOT_FOUND, std::string() );

                    if( isNetworkError( code ) )
                        return failed( DB_FAILURE::UNREACHABLE, hresultText( HRESULT_FROM_WIN32( code ) ) );
                }

                std::wstring connectionString = L"Provider=" + aProgId + L";Data Source=" + quotedValue( path ) + L";";

                if( !aPassword.empty() )
                {
                    connectionString += L"Jet OLEDB:Database Password=" + quotedValue( toWide( aPassword ) ) + L";";
                }

                // The engine loads and unloads Office libraries that race when the ODBC driver does the same
                std::unique_lock lock = lockEngine( aOp );

                if( !lock.owns_lock() )
                {
                    CALL_RESULT busy = failed( DB_FAILURE::TIMEOUT, std::string() );
                    busy.engineBusy = true;
                    return busy;
                }

                DISP connection = DISP::Create( L"ADODB.Connection" );

                connection.SetProperty( L"Mode", VAR::Long( adModeRead | adModeShareDenyNone ) );

                if( aTimeout > 0 )
                {
                    connection.SetProperty( L"ConnectionTimeout", VAR::Long( aTimeout ) );
                    connection.SetProperty( L"CommandTimeout", VAR::Long( aTimeout ) );
                }

                try
                {
                    connection.Call( L"Open", VAR::Str( connectionString ) );
                }
                catch( ADO_ERROR& error )
                {
                    applyProviderErrors( connection, error );
                    throw;
                }

                sessions()[aSession].connection = std::move( connection );
                return {};
            },
            aSession );
}


DB_VALUE convertValue( const VARIANT& aValue );


/// Fails a row batch the provider handed back in a shape that cannot be read
void checkBatch( HRESULT aHr )
{
    if( FAILED( aHr ) )
        throw ADO_ERROR{ aHr, "invalid row batch" };
}


/// Reads the rows still waiting in a recordset, batch by batch, into aOnRow; stops when aOnRow returns false
template <typename ON_ROW>
void forEachRow( DISP& aRecordset, ON_ROW aOnRow )
{
    for( ;; )
    {
        if( aRecordset.Property( L"EOF" ).AsBool() )
            return;

        VAR batch = aRecordset.Call( L"GetRows", VAR::Long( GET_ROWS_BATCH ) );

        // A batch that cannot be read would end the pass early, and two passes cut alike would agree
        if( V_VT( &batch.Raw() ) != ( VT_ARRAY | VT_VARIANT ) || !V_ARRAY( &batch.Raw() ) )
            checkBatch( E_FAIL );

        SAFEARRAY* array = V_ARRAY( &batch.Raw() );
        LONG       fieldLower = 0;
        LONG       fieldUpper = -1;
        LONG       rowLower = 0;
        LONG       rowUpper = -1;

        if( SafeArrayGetDim( array ) != 2 )
            checkBatch( E_FAIL );

        checkBatch( SafeArrayGetLBound( array, 1, &fieldLower ) );
        checkBatch( SafeArrayGetUBound( array, 1, &fieldUpper ) );
        checkBatch( SafeArrayGetLBound( array, 2, &rowLower ) );
        checkBatch( SafeArrayGetUBound( array, 2, &rowUpper ) );

        const size_t fields = static_cast<size_t>( fieldUpper - fieldLower + 1 );
        const size_t rows = static_cast<size_t>( rowUpper - rowLower + 1 );

        VARIANT* cells = nullptr;

        checkBatch( SafeArrayAccessData( array, reinterpret_cast<void**>( &cells ) ) );

        bool more = true;

        try
        {
            // The first subscript is the field, and it varies fastest in memory
            for( size_t row = 0; row < rows && more; ++row )
            {
                std::vector<DB_VALUE> values;
                values.reserve( fields );

                for( size_t field = 0; field < fields; ++field )
                    values.push_back( convertValue( cells[row * fields + field] ) );

                more = aOnRow( std::move( values ) );
            }
        }
        catch( ... )
        {
            SafeArrayUnaccessData( array );
            throw;
        }

        SafeArrayUnaccessData( array );

        if( !more || rows == 0 )
            return;
    }
}


std::string bstrText( BSTR aText )
{
    return aText ? toUtf8( aText, SysStringLen( aText ) ) : std::string();
}


DB_VALUE convertValue( const VARIANT& aValue )
{
    VARIANT value = aValue;

    // A by-reference variant points at the real value
    if( V_VT( &value ) & VT_BYREF )
    {
        VAR copy;

        if( FAILED( VariantCopyInd( copy.Out(), &value ) ) )
            return std::monostate();

        return convertValue( copy.Raw() );
    }

    switch( V_VT( &value ) )
    {
    case VT_EMPTY:
    case VT_NULL:
    case VT_ERROR:
        return std::monostate();

    case VT_BSTR:
        return bstrText( V_BSTR( &value ) );

    case VT_I1:
        return static_cast<int64_t>( V_I1( &value ) );

    case VT_UI1:
        return static_cast<int64_t>( V_UI1( &value ) );

    case VT_I2:
        return static_cast<int64_t>( V_I2( &value ) );

    case VT_UI2:
        return static_cast<int64_t>( V_UI2( &value ) );

    case VT_I4:
    case VT_INT:
        return static_cast<int64_t>( V_I4( &value ) );

    case VT_UI4:
    case VT_UINT:
        return static_cast<int64_t>( V_UI4( &value ) );

    case VT_I8:
        return static_cast<int64_t>( V_I8( &value ) );

    case VT_UI8:
        if( V_UI8( &value ) > static_cast<ULONGLONG>( INT64_MAX ) )
            return std::to_string( V_UI8( &value ) );

        return static_cast<int64_t>( V_UI8( &value ) );

    case VT_R4:
        // The shortest text that reads back as the single, so 0.1 does not become 0.10000000149
        // A negative zero would print as -0, which no other numeric column does
        return V_R4( &value ) == 0.0f ? std::string( "0" ) : fmt::format( "{}", V_R4( &value ) );

    case VT_R8:
        return V_R8( &value );

    case VT_BOOL:
        return V_BOOL( &value ) != VARIANT_FALSE;

    case VT_DATE:
    {
        SYSTEMTIME time;

        if( !VariantTimeToSystemTime( V_DATE( &value ), &time ) )
            return std::monostate();

        return fmt::format( "{:04}-{:02}-{:02} {:02}:{:02}:{:02}", time.wYear, time.wMonth, time.wDay, time.wHour,
                            time.wMinute, time.wSecond );
    }

    case VT_CY:
    case VT_DECIMAL:
    {
        BSTR    raw = nullptr;
        HRESULT hr = V_VT( &value ) == VT_CY ? VarBstrFromCy( V_CY( &value ), LOCALE_INVARIANT, 0, &raw )
                                             : VarBstrFromDec( &V_DECIMAL( &value ), LOCALE_INVARIANT, 0, &raw );

        if( FAILED( hr ) )
            return std::monostate();

        std::unique_ptr<OLECHAR, decltype( &SysFreeString )> text( raw, &SysFreeString );

        return NormalizeDecimalText( bstrText( text.get() ) );
    }

    case VT_ARRAY | VT_UI1:
        return std::monostate();

    default:
    {
        VAR text;

        if( FAILED( VariantChangeTypeEx( text.Out(), &value, LOCALE_INVARIANT, 0, VT_BSTR ) ) )
            return std::monostate();

        return bstrText( V_BSTR( &text.Raw() ) );
    }
    }
}


/// The position of a named field in a recordset
long fieldIndex( DISP& aRecordset, const wchar_t* aName )
{
    DISP fields = DISP::From( aRecordset.Property( L"Fields" ) );
    long count = fields.Property( L"Count" ).AsLong();

    for( long i = 0; i < count; ++i )
    {
        DISP field = DISP::From( fields.Property( L"Item", VAR::Long( i ) ) );

        if( _wcsicmp( toWide( field.Property( L"Name" ).AsString() ).c_str(), aName ) == 0 )
            return i;
    }

    throw ADO_ERROR{ E_FAIL, "the provider's catalog lacks the column " + toUtf8( aName, wcslen( aName ) ) };
}


/// A catalog number, which the provider may deliver as any numeric type
int64_t asInteger( const DB_VALUE& aValue )
{
    if( const int64_t* integer = std::get_if<int64_t>( &aValue ) )
        return *integer;

    if( const double* real = std::get_if<double>( &aValue ) )
        return static_cast<int64_t>( *real );

    if( const std::string* text = std::get_if<std::string>( &aValue ) )
        return std::strtoll( text->c_str(), nullptr, 10 );

    return 0;
}


/// The ODBC SQL data type nearest to an OLE DB type, and whether a column of the type can be read as text
void describeType( long aOleDbType, int& aOdbcType, bool& aSupported )
{
    // OLE DB DBTYPE values and the ODBC SQL types they map to
    enum OLE_DB_TYPE : long
    {
        DBTYPE_I2 = 2,
        DBTYPE_I4 = 3,
        DBTYPE_R4 = 4,
        DBTYPE_R8 = 5,
        DBTYPE_CY = 6,
        DBTYPE_DATE = 7,
        DBTYPE_BSTR = 8,
        DBTYPE_IDISPATCH = 9,
        DBTYPE_BOOL = 11,
        DBTYPE_UNKNOWN = 13,
        DBTYPE_DECIMAL = 14,
        DBTYPE_UI1 = 17,
        DBTYPE_I8 = 20,
        DBTYPE_GUID = 72,
        DBTYPE_BYTES = 128,
        DBTYPE_STR = 129,
        DBTYPE_WSTR = 130,
        DBTYPE_NUMERIC = 131,
        DBTYPE_UDT = 132,
        DBTYPE_DBDATE = 133,
        DBTYPE_DBTIME = 134,
        DBTYPE_DBTIMESTAMP = 135,
        DBTYPE_CHAPTER = 136,
        DBTYPE_VARIANT = 138
    };

    enum ODBC_SQL_TYPE : int
    {
        ODBC_SQL_NONE = 0,
        ODBC_SQL_NUMERIC = 2,
        ODBC_SQL_DECIMAL = 3,
        ODBC_SQL_INTEGER = 4,
        ODBC_SQL_SMALLINT = 5,
        ODBC_SQL_REAL = 7,
        ODBC_SQL_DOUBLE = 8,
        ODBC_SQL_VARCHAR = 12,
        ODBC_SQL_TYPE_TIMESTAMP = 93,
        ODBC_SQL_BIGINT = -5,
        ODBC_SQL_TINYINT = -6,
        ODBC_SQL_BIT = -7,
        ODBC_SQL_BINARY = -3,
        ODBC_SQL_GUID = -11
    };

    aSupported = true;
    aOdbcType = ODBC_SQL_VARCHAR;

    switch( aOleDbType )
    {
    case DBTYPE_I2: aOdbcType = ODBC_SQL_SMALLINT; break;
    case DBTYPE_I4: aOdbcType = ODBC_SQL_INTEGER; break;
    case DBTYPE_R4: aOdbcType = ODBC_SQL_REAL; break;
    case DBTYPE_R8: aOdbcType = ODBC_SQL_DOUBLE; break;
    case DBTYPE_CY: aOdbcType = ODBC_SQL_DECIMAL; break;
    case DBTYPE_BOOL: aOdbcType = ODBC_SQL_BIT; break;
    case DBTYPE_DECIMAL: aOdbcType = ODBC_SQL_DECIMAL; break;
    case DBTYPE_UI1: aOdbcType = ODBC_SQL_TINYINT; break;
    case DBTYPE_I8: aOdbcType = ODBC_SQL_BIGINT; break;
    case DBTYPE_GUID: aOdbcType = ODBC_SQL_GUID; break;
    case DBTYPE_NUMERIC: aOdbcType = ODBC_SQL_NUMERIC; break;

    case DBTYPE_DATE:
    case DBTYPE_DBDATE:
    case DBTYPE_DBTIME:
    case DBTYPE_DBTIMESTAMP: aOdbcType = ODBC_SQL_TYPE_TIMESTAMP; break;

    case DBTYPE_BSTR:
    case DBTYPE_STR:
    case DBTYPE_WSTR: aOdbcType = ODBC_SQL_VARCHAR; break;

    // Binary and OLE objects cannot be read as text
    case DBTYPE_BYTES:
        aOdbcType = ODBC_SQL_BINARY;
        aSupported = false;
        break;

    case DBTYPE_IDISPATCH:
    case DBTYPE_UNKNOWN:
    case DBTYPE_UDT:
    case DBTYPE_CHAPTER:
    case DBTYPE_VARIANT:
        aOdbcType = ODBC_SQL_NONE;
        aSupported = false;
        break;

    default: break;
    }
}


CATALOG_RESULT listTablesJob( uint64_t aSession )
{
    CATALOG_RESULT result;

    result.call = guarded(
            [&]() -> CALL_RESULT
            {
                ADO_SESSION& session = sessionFor( aSession );
                DISP         schema =
                        DISP::From( session.connection.Call( L"OpenSchema", VAR::Long( adSchemaTables ) ) );
                const long   nameIndex = fieldIndex( schema, L"TABLE_NAME" );
                const long   typeIndex = fieldIndex( schema, L"TABLE_TYPE" );

                forEachRow( schema,
                            [&]( std::vector<DB_VALUE>&& aRow )
                            {
                                const std::string type = DatabaseValueToString( aRow[typeIndex] );
                                const std::string name = DatabaseValueToString( aRow[nameIndex] );

                                // Saved select queries are views, and the engine's own bookkeeping tables are not data
                                if( ( type == "TABLE" || type == "VIEW" ) && name.rfind( "MSys", 0 ) != 0
                                    && name.rfind( "~", 0 ) != 0 )
                                {
                                    result.names.push_back( name );
                                }

                                return true;
                            } );

                schema.Call( L"Close" );
                return {};
            },
            aSession );

    return result;
}


CATALOG_RESULT describeTableJob( uint64_t aSession, const std::string& aTable )
{
    CATALOG_RESULT result;

    result.call = guarded(
            [&]() -> CALL_RESULT
            {
                ADO_SESSION& session = sessionFor( aSession );
                DISP         schema = DISP::From( session.connection.Call(
                        L"OpenSchema", VAR::Long( adSchemaColumns ), VAR::TableCriteria( toWide( aTable ) ) ) );
                const long   tableIndex = fieldIndex( schema, L"TABLE_NAME" );
                const long   nameIndex = fieldIndex( schema, L"COLUMN_NAME" );
                const long   positionIndex = fieldIndex( schema, L"ORDINAL_POSITION" );
                const long   typeIndex = fieldIndex( schema, L"DATA_TYPE" );

                std::vector<std::pair<int64_t, DB_COLUMN>> columns;

                forEachRow( schema,
                            [&]( std::vector<DB_VALUE>&& aRow )
                            {
                                DB_COLUMN column;

                                column.name = DatabaseValueToString( aRow[nameIndex] );
                                describeType( static_cast<long>( asInteger( aRow[typeIndex] ) ), column.type,
                                              column.supported );

                                if( !result.found )
                                {
                                    result.table = DatabaseValueToString( aRow[tableIndex] );
                                    result.found = true;
                                }

                                columns.emplace_back( asInteger( aRow[positionIndex] ), std::move( column ) );
                                return true;
                            } );

                schema.Call( L"Close" );

                std::stable_sort( columns.begin(), columns.end(),
                                  []( const auto& aLeft, const auto& aRight )
                                  {
                                      return aLeft.first < aRight.first;
                                  } );

                for( auto& column : columns )
                    result.columns.push_back( std::move( column.second ) );

                return {};
            },
            aSession );

    return result;
}


/// Opens a forward-only read-only recordset on aSql
DISP executeQuery( ADO_SESSION& aSession, const std::string& aSql )
{
    try
    {
        return DISP::From( aSession.connection.Call( L"Execute", VAR::Str( toWide( aSql ) ), VAR::Missing(),
                                                     VAR::Long( adCmdText ) ) );
    }
    catch( ADO_ERROR& error )
    {
        applyProviderErrors( aSession.connection, error );
        throw;
    }
}


/// FNV-1a over each value's type tag, null marker, length and bytes, so one value can never run into the next
class CONTENT_HASH
{
public:
    void Add( const DB_VALUE& aValue )
    {
        const uint8_t tag = static_cast<uint8_t>( aValue.index() );
        const uint8_t present = std::holds_alternative<std::monostate>( aValue ) ? 0 : 1;

        add( &tag, sizeof( tag ) );
        add( &present, sizeof( present ) );

        std::visit(
                [this]( const auto& aCell )
                {
                    payload( aCell );
                },
                aValue );
    }

    uint64_t Value() const { return m_hash; }

private:
    void payload( std::monostate ) { length( 0 ); }

    void payload( const std::string& aText )
    {
        length( aText.size() );
        add( aText.data(), aText.size() );
    }

    void payload( int64_t aValue )
    {
        length( sizeof( aValue ) );
        add( &aValue, sizeof( aValue ) );
    }

    void payload( double aValue )
    {
        length( sizeof( aValue ) );
        add( &aValue, sizeof( aValue ) );
    }

    void payload( bool aValue )
    {
        const uint8_t byte = aValue ? 1 : 0;

        length( sizeof( byte ) );
        add( &byte, sizeof( byte ) );
    }

    void length( uint64_t aLength ) { add( &aLength, sizeof( aLength ) ); }

    void add( const void* aData, size_t aSize )
    {
        const uint8_t* bytes = static_cast<const uint8_t*>( aData );

        for( size_t i = 0; i < aSize; ++i )
        {
            m_hash ^= bytes[i];
            m_hash *= 0x100000001b3ULL;
        }
    }

    uint64_t m_hash = 0xcbf29ce484222325ULL;
};


/// What one pass over a table found
struct PASS_RESULT
{
    CALL_RESULT call;
    size_t      rows = 0;
    uint64_t    hash = 0;
};


/// Reads the rows of aSql in order, hashes them and hands them to the caller
PASS_RESULT readTableJob( DB_JOB_CONTEXT& aContext, uint64_t aSession, const std::string& aSql,
                          const std::string& aUnorderedSql, std::shared_ptr<std::vector<std::string>> aLabels,
                          size_t aMaxRows )
{
    PASS_RESULT  pass;
    CONTENT_HASH hash;

    pass.call = guarded(
            [&]() -> CALL_RESULT
            {
                ADO_SESSION& session = sessionFor( aSession );
                DISP         recordset;

                try
                {
                    recordset = executeQuery( session, aSql );
                }
                catch( const ADO_ERROR& error )
                {
                    // Ordering by a memo or OLE column is refused, and an unordered sample is still useful
                    if( aUnorderedSql.empty() || classifyFailure( error, error.text ) != DB_FAILURE::UNKNOWN )
                        throw;

                    recordset = executeQuery( session, aUnorderedSql );
                }

                {
                    DISP       fields = DISP::From( recordset.Property( L"Fields" ) );
                    const long count = fields.Property( L"Count" ).AsLong();

                    for( long i = 0; i < count; ++i )
                    {
                        DISP field = DISP::From( fields.Property( L"Item", VAR::Long( i ) ) );

                        aLabels->push_back( field.Property( L"Name" ).AsString() );
                    }
                }

                bool   accepted = true;
                size_t delivered = 0;

                if( aMaxRows != 0 )
                {
                    forEachRow( recordset,
                                [&]( std::vector<DB_VALUE>&& aRow )
                                {
                                    // TOP keeps ties, so a bounded read can find more rows than asked for
                                    if( delivered >= aMaxRows )
                                        return false;

                                    ++delivered;

                                    for( const DB_VALUE& value : aRow )
                                        hash.Add( value );

                                    accepted = aContext.Emit( std::move( aRow ) );
                                    return accepted;
                                } );
                }

                if( !accepted || !aContext.Flush() )
                    return failed( DB_FAILURE::UNKNOWN, "the rows were not accepted" );

                try
                {
                    recordset.Call( L"Close" );
                }
                catch( ADO_ERROR& error )
                {
                    applyProviderErrors( session.connection, error );

                    // A dead connection fails the read, any other failure comes after a complete read
                    if( classifyFailure( error, error.text ) == DB_FAILURE::UNREACHABLE )
                        throw;

                    wxLogTrace( traceDatabase, wxT( "ACE_BACKEND: closing the recordset failed (0x%08lx, %s)" ),
                                static_cast<unsigned long>( error.hr ), wxString::FromUTF8( error.sqlState ) );
                }

                pass.rows = delivered;
                return {};
            },
            aSession );

    pass.hash = hash.Value();
    return pass;
}

} // namespace


ACE_BACKEND::ACE_BACKEND( std::string aPath, std::string aPassword, int aTimeoutSeconds ) :
        m_path( std::move( aPath ) ),
        m_password( std::move( aPassword ) ),
        m_timeout( aTimeoutSeconds ),
        m_worker( std::make_unique<DB_WORKER_THREAD>( DB_WORKER_THREAD::SourceKeyFor( m_path ), comThreadInit,
                                                      comThreadExit, true, MarkAccessWorkerAbandoned ) )
{
}


std::shared_ptr<ACE_ENGINE_OP> ACE_BACKEND::newOp() const
{
    return std::make_shared<ACE_ENGINE_OP>( ACE_ENGINE_OP::CLOCK::now() + DB_WORKER_THREAD::CallerBound( m_timeout ) );
}


ACE_BACKEND::~ACE_BACKEND()
{
    // Closing the session and the worker's exit hook share the one deadline
    auto op = newOp();

    dropConnection( op, true );
    m_worker->SetShutdownBound( remainingUntil( op->deadline ) );

    if( !m_worker->IsHealthy() )
    {
        wxLogTrace( traceDatabase, wxT( "ACE_BACKEND: %s is blocked in provider I/O; its thread is detached and "
                                        "finishes when the share answers" ),
                    wxString::FromUTF8( m_path ) );
    }
}


ACE_BACKEND::PROVIDER_STATE ACE_BACKEND::ProviderState()
{
    std::wstring progId;

    return providerLookup( progId, 0ms );
}


std::string ACE_BACKEND::ProviderPendingMessage()
{
    return utf8( _( "still checking for the Microsoft Access Database Engine; try again in a moment" ) );
}


std::string ACE_BACKEND::ProviderMissingMessage()
{
    std::string message = utf8( _( "the Microsoft Access Database Engine (64-bit) is not installed; download it from "
                                   "https://www.microsoft.com/en-us/download/details.aspx?id=54920" ) );

    std::optional<PROBE_ANSWER> answer = probeAnswer( 0ms );

    if( answer && !answer->error.empty() )
        message += " (" + answer->error + ")";

    return message;
}


void ACE_BACKEND::fail( DB_FAILURE aFailure, std::string aError )
{
    m_lastFailure = aFailure;
    m_lastError = ScrubSecret( std::move( aError ), m_password );

    wxLogTrace( traceDatabase, wxT( "ACE_BACKEND: %s" ), wxString::FromUTF8( m_lastError ) );
}


/// Composes the message for a failed call, naming the file
static std::string describeFailure( const std::string& aPath, const CALL_RESULT& aResult,
                                    const std::string& aPassword )
{
    if( aResult.engineBusy )
        return AccessEngineBusyMessage();

    const wxString path = wxString::FromUTF8( aPath );

    // A short password cannot be scrubbed out of provider text, so none of that text is shown
    const std::string redacted = RedactDetail( aResult.detail, aPassword );
    const wxString    suffix =
            redacted.empty() ? wxString() : wxString::Format( wxT( " (%s)" ), wxString::FromUTF8( redacted ) );
    wxString          message;

    switch( aResult.failure )
    {
    case DB_FAILURE::FILE_NOT_FOUND: return utf8( wxString::Format( _( "database file %s was not found" ), path ) );
    case DB_FAILURE::PROVIDER_MISSING: return ACE_BACKEND::ProviderMissingMessage();

    case DB_FAILURE::FILE_IN_USE:
        message = wxString::Format( _( "database file %s is locked by another user" ), path );
        break;

    case DB_FAILURE::LOGIN:
        message = wxString::Format( _( "the password for database file %s is missing or wrong" ), path );
        break;

    case DB_FAILURE::UNREACHABLE:
        message = wxString::Format( _( "database file %s could not be reached" ), path );
        break;

    default: message = wxString::Format( _( "database file %s could not be read" ), path ); break;
    }

    return utf8( message + suffix );
}


/// A failure after which the connection can no longer be trusted
static bool invalidates( DB_FAILURE aFailure )
{
    return aFailure == DB_FAILURE::UNREACHABLE || aFailure == DB_FAILURE::FILE_NOT_FOUND;
}


void ACE_BACKEND::failCall( DB_FAILURE aFailure, std::string aMessage )
{
    fail( aFailure, std::move( aMessage ) );

    if( invalidates( aFailure ) )
        dropConnection( m_lastOp );
}


bool ACE_BACKEND::requireConnection()
{
    if( m_connected )
        return true;

    fail( DB_FAILURE::UNKNOWN, utf8( _( "not connected to a database" ) ) );
    return false;
}


template <typename R>
std::optional<R> ACE_BACKEND::runOnWorker( std::function<R( DB_JOB_CONTEXT& )> aJob, const DB_ROW_SINK* aSink,
                                           std::shared_ptr<ACE_ENGINE_OP>           aOp,
                                           std::optional<std::chrono::milliseconds> aProgressBound )
{
    if( !aOp )
        aOp = newOp();

    m_lastOp = aOp;

    auto wrapped = [op = aOp, job = std::move( aJob )]( DB_JOB_CONTEXT& aContext )
    {
        s_threadOp = op;
        return job( aContext );
    };

    const std::chrono::milliseconds bound = aProgressBound ? *aProgressBound : remainingUntil( aOp->deadline );
    std::optional<R>                result = m_worker->Run<R>( std::move( wrapped ), bound, aSink );

    if( result )
        return result;

    const DB_WORKER_FAILURE failure = DescribeWorkerOutcome( m_worker->LastOutcome(), m_path );

    if( failure.connection == DB_WORKER_FAILURE::CONNECTION::DROP )
    {
        dropConnection( aOp );
    }
    else if( failure.connection == DB_WORKER_FAILURE::CONNECTION::FORGET )
    {
        // The stuck thread releases the connection itself when the blocked call returns
        m_connected = false;
        m_session = 0;
        m_cycleSessions.clear();
    }

    // The operation's own state says whether the abandoned job was stuck on the engine lock
    if( aOp->Busy() )
        fail( DB_FAILURE::TIMEOUT, AccessEngineBusyMessage() );
    else
        fail( failure.failure, failure.message );

    return std::nullopt;
}


void ACE_BACKEND::dropConnection( std::shared_ptr<ACE_ENGINE_OP> aOp, bool aForExit )
{
    std::vector<uint64_t> ids;

    if( m_session )
        ids.push_back( m_session );

    for( const CYCLE_SESSION& cycleSession : m_cycleSessions )
        ids.push_back( cycleSession.id );

    m_session = 0;
    m_cycleSessions.clear();
    m_connected = false;

    // An abandoned or dead worker holds the sessions itself, and a thread started to close nothing would wait
    // out the stuck one for no gain
    if( !m_worker->HasLiveThread() || ( ids.empty() && !aForExit ) )
        return;

    if( !aOp )
        aOp = newOp();

    closeOnWorker( std::move( ids ), aOp );
}


bool ACE_BACKEND::closeOnWorker( std::vector<uint64_t> aIds, std::shared_ptr<ACE_ENGINE_OP> aOp,
                                 std::optional<std::chrono::milliseconds> aBound )
{
    const bool closed = m_worker->RunCleanup(
            [ids = aIds, aOp]()
            {
                s_threadOp = aOp;

                for( uint64_t id : ids )
                    closeSession( id, *aOp );
            },
            aBound ? *aBound : remainingUntil( aOp->deadline ) );

    // A close that lost the lock leaves its session open, and the operation's state holds that even if the job
    // is still running
    if( !aIds.empty() && ( aOp->state == ACE_ENGINE_OP::STATE::EXPIRED || ( !closed && aOp->Busy() ) ) )
        fail( DB_FAILURE::TIMEOUT, AccessEngineBusyMessage() );

    return closed;
}


bool ACE_BACKEND::Connect()
{
    m_lastError.clear();
    m_lastFailure = DB_FAILURE::UNKNOWN;

    if( m_connected )
        return true;

    if( m_path.empty() )
    {
        fail( DB_FAILURE::FILE_NOT_FOUND, utf8( _( "no database file is set" ) ) );
        return false;
    }

    std::wstring progId;

    switch( providerLookup( progId, PROBE_WAIT ) )
    {
    case PROVIDER_STATE::PENDING:
        fail( DB_FAILURE::TIMEOUT, ProviderPendingMessage() );
        return false;

    case PROVIDER_STATE::BUSY: fail( DB_FAILURE::TIMEOUT, AccessEngineBusyMessage() ); return false;

    case PROVIDER_STATE::MISSING:
        fail( DB_FAILURE::PROVIDER_MISSING, ProviderMissingMessage() );
        return false;

    case PROVIDER_STATE::AVAILABLE:
        break;
    }

    const uint64_t session = g_nextSession++;

    // The same deadline bounds the caller's wait and the job's wait for the engine lock
    auto op = newOp();

    std::optional<CALL_RESULT> result = runOnWorker<CALL_RESULT>(
            [session, progId, path = m_path, password = m_password, timeout = m_timeout, op]( DB_JOB_CONTEXT& )
            {
                return connectJob( session, progId, path, password, timeout, *op );
            },
            nullptr, op );

    if( !result )
        return false;

    if( !result->ok )
    {
        fail( result->failure, describeFailure( m_path, *result, m_password ) );
        return false;
    }

    m_session = session;
    m_progId = progId;
    m_connected = true;
    return true;
}


void ACE_BACKEND::Disconnect()
{
    dropConnection();
}


bool ACE_BACKEND::ListTables( std::vector<std::string>& aTables )
{
    aTables.clear();
    m_lastError.clear();

    if( !requireConnection() )
        return false;

    const uint64_t session = m_session;

    std::optional<CATALOG_RESULT> result = runOnWorker<CATALOG_RESULT>(
            [session]( DB_JOB_CONTEXT& )
            {
                return listTablesJob( session );
            },
            nullptr );

    if( !result )
        return false;

    if( !result->call.ok )
    {
        failCall( result->call.failure, describeFailure( m_path, result->call, m_password ) );
        return false;
    }

    aTables = std::move( result->names );
    std::sort( aTables.begin(), aTables.end() );
    return true;
}


std::optional<DB_TABLE_INFO> ACE_BACKEND::DescribeTable( const std::string& aTable )
{
    // A table that is simply absent leaves no error, so nothing of an earlier failure may remain
    m_lastError.clear();
    m_lastFailure = DB_FAILURE::UNKNOWN;

    if( !requireConnection() )
        return std::nullopt;

    const uint64_t session = m_session;

    std::optional<CATALOG_RESULT> result = runOnWorker<CATALOG_RESULT>(
            [session, aTable]( DB_JOB_CONTEXT& )
            {
                return describeTableJob( session, aTable );
            },
            nullptr );

    if( !result )
        return std::nullopt;

    if( !result->call.ok )
    {
        failCall( result->call.failure, describeFailure( m_path, result->call, m_password ) );
        return std::nullopt;
    }

    if( !result->found )
        return std::nullopt;

    DB_TABLE_INFO info;

    info.name = std::move( result->table );
    info.columns = std::move( result->columns );

    return info;
}


bool ACE_BACKEND::ReadTable( const DB_TABLE_INFO& aTable, const std::vector<std::string>& aColumns, size_t aMaxRows,
                             std::vector<std::string>& aLabels, const DB_ROW_SINK& aSink )
{
    aLabels.clear();
    m_lastError.clear();

    if( !requireConnection() )
        return false;

    const std::string columns = selectList( aColumns );
    const bool        bounded = aMaxRows != ALL_ROWS;
    const std::string from = " FROM " + quoted( aTable.name );
    std::string       sql = "SELECT ";
    std::string       unordered;

    if( aMaxRows == 0 )
    {
        sql += columns + from + " WHERE 1=0";
    }
    else if( bounded )
    {
        const std::string top = "TOP " + std::to_string( aMaxRows ) + " ";

        unordered = sql + top + columns + from;
        sql += top + columns + from + " ORDER BY 1";
    }
    else
    {
        sql += columns + from;
    }

    return readPass( m_session, sql, unordered, aMaxRows, aLabels, &aSink ).has_value();
}


std::optional<ACE_BACKEND::PASS_DIGEST>
ACE_BACKEND::readPass( uint64_t aSession, const std::string& aSql, const std::string& aUnorderedSql, size_t aMaxRows,
                       std::vector<std::string>& aLabels, const DB_ROW_SINK* aSink,
                       std::optional<ACE_ENGINE_OP::CLOCK::time_point> aDeadline )
{
    static const DB_ROW_SINK discard = []( std::vector<DB_VALUE>&& ) {};

    DB_READ_DELIVERY delivery( aLabels, aSink ? *aSink : discard );
    auto             labels = delivery.Labels();

    std::shared_ptr<ACE_ENGINE_OP>           op;
    std::optional<std::chrono::milliseconds> progressBound;

    // A verified pass may wait for the engine only until the read's deadline, but for rows as long as they come
    if( aDeadline )
    {
        op = std::make_shared<ACE_ENGINE_OP>( *aDeadline );
        progressBound = DB_WORKER_THREAD::CallerBound( m_timeout );
    }

    std::optional<PASS_RESULT> result = runOnWorker<PASS_RESULT>(
            [aSession, aSql, aUnorderedSql, labels, aMaxRows]( DB_JOB_CONTEXT& aContext )
            {
                return readTableJob( aContext, aSession, aSql, aUnorderedSql, labels, aMaxRows );
            },
            delivery.Sink(), op, progressBound );

    if( !result )
        return std::nullopt;

    delivery.Finish();

    if( !result->call.ok )
    {
        aLabels.clear();

        if( delivery.SinkThrew() )
            fail( DB_FAILURE::UNKNOWN, DB_READ_DELIVERY::SinkFailureMessage() );
        else
            failCall( result->call.failure, describeFailure( m_path, result->call, m_password ) );

        return std::nullopt;
    }

    return PASS_DIGEST{ result->rows, result->hash, aLabels };
}


void ACE_BACKEND::BeginLoadCycle( const DB_STOP_CHECK& aStopped )
{
    // A cycle inside another keeps the stop of the outer one
    if( m_cycleDepth++ == 0 )
        m_cycleStop = aStopped;
}


void ACE_BACKEND::EndLoadCycle()
{
    if( m_cycleDepth == 0 || --m_cycleDepth > 0 )
        return;

    m_cycleStop = nullptr;

    std::vector<uint64_t> ids;

    for( const CYCLE_SESSION& cycleSession : m_cycleSessions )
        ids.push_back( cycleSession.id );

    m_cycleSessions.clear();

    if( ids.empty() )
        return;

    auto op = newOp();

    // A worker stuck in the close still holds the connection, and releases it when the blocked call returns
    if( !closeOnWorker( std::move( ids ), op ) )
    {
        m_connected = false;
        m_session = 0;
    }
}


bool ACE_BACKEND::cycleStopped() const
{
    return m_cycleStop && m_cycleStop();
}


void ACE_BACKEND::failStopped( const std::string& aTable )
{
    fail( DB_FAILURE::UNKNOWN,
          utf8( wxString::Format( _( "the read of table %s was stopped" ), wxString::FromUTF8( aTable ) ) ) );
}


bool ACE_BACKEND::pauseUnlessStopped( std::chrono::milliseconds aWait )
{
    const ACE_ENGINE_OP::CLOCK::time_point until = ACE_ENGINE_OP::CLOCK::now() + aWait;

    // In slices, so a close waits for one slice and not the whole pause
    while( !cycleStopped() )
    {
        const ACE_ENGINE_OP::CLOCK::duration left = until - ACE_ENGINE_OP::CLOCK::now();

        if( left <= ACE_ENGINE_OP::CLOCK::duration::zero() )
            return true;

        std::this_thread::sleep_for( std::min<ACE_ENGINE_OP::CLOCK::duration>( left, 50ms ) );
    }

    return false;
}


bool ACE_BACKEND::closeSpentSessions( ACE_ENGINE_OP::CLOCK::time_point aDeadline )
{
    if( m_cycleSessions.size() < MAX_CYCLE_SESSIONS )
        return true;

    // Every session has read the table by now, and the newest is kept for the tables after it
    const auto            spent = m_cycleSessions.end() - ( MAX_CYCLE_SESSIONS - 1 );
    std::vector<uint64_t> ids;

    for( auto it = m_cycleSessions.begin(); it != spent; ++it )
        ids.push_back( it->id );

    m_cycleSessions.erase( m_cycleSessions.begin(), spent );

    // The read's deadline may be close, and a worker given only what is left of it would be abandoned
    if( closeOnWorker( std::move( ids ), std::make_shared<ACE_ENGINE_OP>( aDeadline ),
                       DB_WORKER_THREAD::CallerBound( m_timeout ) ) )
    {
        return true;
    }

    // A worker stuck in the close still holds the connections, and releases them when the blocked call returns
    m_connected = false;
    m_session = 0;
    m_cycleSessions.clear();

    if( m_lastError.empty() )
    {
        const DB_WORKER_FAILURE failure = DescribeWorkerOutcome( m_worker->LastOutcome(), m_path );

        fail( failure.failure, failure.message );
    }

    return false;
}


std::optional<uint64_t> ACE_BACKEND::freshSession( const std::string&               aTable,
                                                   ACE_ENGINE_OP::CLOCK::time_point aDeadline,
                                                   const std::function<bool()>&     aMayStart )
{
    for( CYCLE_SESSION& cycleSession : m_cycleSessions )
    {
        if( cycleSession.tables.insert( aTable ).second )
            return cycleSession.id;
    }

    if( !closeSpentSessions( aDeadline ) )
        return std::nullopt;

    std::minstd_rand jitter( std::random_device{}() );

    for( int attempt = 0;; ++attempt )
    {
        // The close and the pause both take time, in which the read may have been stopped or run out of budget
        if( !aMayStart() )
            return std::nullopt;

        const uint64_t session = g_nextSession++;
        auto           op = std::make_shared<ACE_ENGINE_OP>( aDeadline );

        std::optional<CALL_RESULT> result = runOnWorker<CALL_RESULT>(
                [session, progId = m_progId, path = m_path, password = m_password, timeout = m_timeout,
                 op]( DB_JOB_CONTEXT& )
                {
                    return connectJob( session, progId, path, password, timeout, *op );
                },
                nullptr, op, DB_WORKER_THREAD::CallerBound( m_timeout ) );

        if( !result )
            return std::nullopt;

        if( result->ok )
        {
            m_cycleSessions.push_back( CYCLE_SESSION{ session, { aTable } } );

            wxLogTrace( traceDatabase, wxT( "ACE_BACKEND: session %llu opened for %s, %zu held by the cycle" ),
                        static_cast<unsigned long long>( session ), wxString::FromUTF8( aTable ),
                        m_cycleSessions.size() );
            return session;
        }

        const std::chrono::milliseconds wait = FileInUseBackoff( attempt, jitter );

        if( result->failure != DB_FAILURE::FILE_IN_USE || ACE_ENGINE_OP::CLOCK::now() + wait >= aDeadline )
        {
            failCall( result->failure, describeFailure( m_path, *result, m_password ) );
            return std::nullopt;
        }

        pauseUnlessStopped( wait );
    }
}


bool ACE_BACKEND::ReadAllRows( const DB_TABLE_INFO& aTable, const std::vector<std::string>& aColumns,
                               std::vector<std::string>& aLabels, const DB_ROW_SINK& aSink,
                               const DB_RESTART_SINK& aRestart )
{
    aLabels.clear();
    m_lastError.clear();

    if( !requireConnection() )
        return false;

    BeginLoadCycle( DB_STOP_CHECK() );

    const bool read = readVerified( aTable, aColumns, aLabels, aSink, aRestart );

    // The cycle's close may fail too, and the read's own failure is the one to report
    const std::string error = m_lastError;
    const DB_FAILURE  failure = m_lastFailure;

    EndLoadCycle();

    if( !read )
    {
        aLabels.clear();
        m_lastError = error;
        m_lastFailure = failure;
    }

    return read;
}


bool ACE_BACKEND::readVerified( const DB_TABLE_INFO& aTable, const std::vector<std::string>& aColumns,
                                std::vector<std::string>& aLabels, const DB_ROW_SINK& aSink,
                                const DB_RESTART_SINK& aRestart )
{
    using CLOCK = ACE_ENGINE_OP::CLOCK;

    const std::string sql = "SELECT " + selectList( aColumns ) + " FROM " + quoted( aTable.name );
    const auto        start = CLOCK::now();
    const auto        bound = DB_WORKER_THREAD::CallerBound( m_timeout );
    const wxString    table = wxString::FromUTF8( aTable.name );
    const wxString    path = wxString::FromUTF8( m_path );

    // Starts when pass 1 completes, so the first pair always runs and a large table still verifies
    std::optional<CLOCK::time_point> retryDeadline;

    std::minstd_rand                   random( std::random_device{}() );
    std::uniform_int_distribution<int> pauseMs( 50, 250 );

    // No retry starts after the budget, though a pair that finished after it and agreed is still verified
    auto mayStart =
            [&]()
            {
                if( cycleStopped() )
                {
                    failStopped( aTable.name );
                    return false;
                }

                if( !retryDeadline || CLOCK::now() < *retryDeadline )
                    return true;

                fail( DB_FAILURE::FILE_IN_USE,
                      utf8( wxString::Format( _( "table %s in database file %s is being modified by another user" ),
                                              table, path ) ) );
                return false;
            };

    auto timedPass =
            [&]( size_t aPass, const DB_ROW_SINK* aPassSink )
            {
                if( !mayStart() )
                    return std::optional<PASS_DIGEST>();

                const auto passStart = CLOCK::now();

                // The first pair waits for the engine a bound from its own start, a retry only to the budget's end
                const CLOCK::time_point lockDeadline = retryDeadline ? *retryDeadline : passStart + bound;

                std::optional<uint64_t> session = freshSession( aTable.name, lockDeadline, mayStart );

                if( !session || !mayStart() )
                    return std::optional<PASS_DIGEST>();

                const auto                 readStart = CLOCK::now();
                std::optional<PASS_DIGEST> digest =
                        readPass( *session, sql, std::string(), ALL_ROWS, aLabels, aPassSink, lockDeadline );

                if( digest )
                {
                    using MS = std::chrono::duration<double, std::milli>;

                    wxLogTrace( traceDatabase,
                                wxT( "ACE_BACKEND: pass %zu of %s on session %llu: %zu rows, hash %016llx, "
                                     "open %0.1f ms, read %0.1f ms" ),
                                aPass, table, static_cast<unsigned long long>( *session ), digest->rows,
                                static_cast<unsigned long long>( digest->hash ), MS( readStart - passStart ).count(),
                                MS( CLOCK::now() - readStart ).count() );
                }

                return digest;
            };

    std::optional<PASS_DIGEST> previous = timedPass( 0, nullptr );

    if( !previous )
        return false;

    for( size_t pass = 1;; ++pass )
    {
        if( pass > 1 )
        {
            if( !pauseUnlessStopped( std::chrono::milliseconds( pauseMs( random ) ) ) )
            {
                failStopped( aTable.name );
                return false;
            }

            aRestart();
        }

        std::optional<PASS_DIGEST> current = timedPass( pass, &aSink );

        if( !current )
            return false;

        if( !retryDeadline )
            retryDeadline = CLOCK::now() + bound;

        if( *current == *previous )
        {
            wxLogTrace( traceDatabase, wxT( "ACE_BACKEND: %s verified after %zu passes in %0.1f ms" ), table,
                        pass + 1, std::chrono::duration<double, std::milli>( CLOCK::now() - start ).count() );
            return true;
        }

        previous = std::move( current );
    }
}
