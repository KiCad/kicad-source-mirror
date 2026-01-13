/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.TXT for contributors.
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

#include <app_monitor.h>
#include <pgm_base.h>
#include <magic_enum.hpp>
#include <kiplatform/policy.h>
#include <policy_keys.h>
#include <paths.h>
#include <wx/filename.h>
#include <wx/ffile.h>

#include <boost/uuid/uuid_io.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <build_version.h>

#ifdef KICAD_USE_SENTRY
#include <sentry.h>
#endif

using namespace APP_MONITOR;

SENTRY::SENTRY() :
        m_isOptedIn( false )
{
}


void SENTRY::Init()
{
#ifdef KICAD_USE_SENTRY
    sentryInit();
#endif
}


void SENTRY::Cleanup()
{
#ifdef KICAD_USE_SENTRY
    sentry_close();
#endif
}


void SENTRY::AddTag( const wxString& aKey, const wxString& aValue )
{
#ifdef KICAD_USE_SENTRY
    sentry_set_tag( aKey.c_str(), aValue.c_str() );
#endif
}


void SENTRY::SetSentryOptIn( bool aOptIn )
{
#ifdef KICAD_USE_SENTRY
    if( aOptIn )
    {
        // Opting in (when not opted in before) may require reading the uid off disk
        // as it was skipped previously.
        readOrCreateUid();

        if( !m_sentry_optin_fn.Exists() )
        {
            wxFFile sentryInitFile( m_sentry_optin_fn.GetFullPath(), "w" );
            sentryInitFile.Write( "" );
            sentryInitFile.Close();
        }

        m_isOptedIn = true;
    }
    else
    {
        if( m_sentry_optin_fn.Exists() )
        {
            wxRemoveFile( m_sentry_optin_fn.GetFullPath() );
            sentry_close();
        }

        m_isOptedIn = false;
    }
#else
    m_isOptedIn = false;
#endif
}


wxString SENTRY::sentryCreateUid()
{
    boost::uuids::uuid uuid = boost::uuids::random_generator()();
    wxString           userGuid = boost::uuids::to_string( uuid );

    wxFFile sentryInitFile( m_sentry_uid_fn.GetFullPath(), "w" );
    sentryInitFile.Write( userGuid );
    sentryInitFile.Close();

    return userGuid;
}


void SENTRY::ResetSentryId()
{
    m_sentryUid = sentryCreateUid();
}


const wxString& SENTRY::GetSentryId()
{
    return m_sentryUid;
}


void SENTRY::readOrCreateUid()
{
    if( m_sentry_optin_fn.Exists() )
    {
        wxFFile sentryInitFile( m_sentry_uid_fn.GetFullPath() );
        sentryInitFile.ReadAll( &m_sentryUid );
        sentryInitFile.Close();
    }

    if( m_sentryUid.IsEmpty() || m_sentryUid.length() != 36 )
    {
        ResetSentryId();
    }
}


void SENTRY::sentryInit()
{
#ifdef KICAD_USE_SENTRY
    m_sentry_optin_fn = wxFileName( PATHS::GetUserCachePath(), "sentry-opt-in" );
    m_sentry_uid_fn = wxFileName( PATHS::GetUserCachePath(), "sentry-uid" );

    if( isConfiguredOptedIn() )
    {
        m_isOptedIn = true;

        readOrCreateUid();

        sentry_options_t* options = sentry_options_new();

#ifndef KICAD_SENTRY_DSN
#error "Project configuration error, missing KICAD_SENTRY_DSN"
#endif
        // only capture 5% of transactions
        sentry_options_set_traces_sample_rate( options, 0.05 );
        sentry_options_set_dsn( options, KICAD_SENTRY_DSN );

        wxFileName tmp;
        tmp.AssignDir( PATHS::GetUserCachePath() );
        tmp.AppendDir( "sentry" );

#ifdef __WINDOWS__
        sentry_options_set_database_pathw( options, tmp.GetPathWithSep().wc_str() );
#else
        sentry_options_set_database_path( options, tmp.GetPathWithSep().c_str() );
#endif
        sentry_options_set_symbolize_stacktraces( options, true );
        sentry_options_set_auto_session_tracking( options, false );

        sentry_options_set_release( options, GetCommitHash().ToStdString().c_str() );

        // This just gives us more filtering within sentry, issues still get grouped across
        // environments.
        sentry_options_set_environment( options, GetMajorMinorVersion().c_str() );

        sentry_init( options );

        sentry_value_t user = sentry_value_new_object();
        sentry_value_set_by_key( user, "id", sentry_value_new_string( m_sentryUid.c_str() ) );
        sentry_set_user( user );

        sentry_set_tag( "kicad.version", GetBuildVersion().ToStdString().c_str() );
    }
#endif
}


bool SENTRY::isConfiguredOptedIn()
{
    // policy takes precedence
    KIPLATFORM::POLICY::PBOOL policyState = KIPLATFORM::POLICY::GetPolicyBool( POLICY_KEY_DATACOLLECTION );
    if( policyState != KIPLATFORM::POLICY::PBOOL::NOT_CONFIGURED )
    {
        return policyState == KIPLATFORM::POLICY::PBOOL::ENABLED;
    }

    return m_sentry_optin_fn.Exists();
}


bool SENTRY::IsOptedIn()
{
#ifdef KICAD_USE_SENTRY
    // always override with policy, just in case
    KIPLATFORM::POLICY::PBOOL policyState = KIPLATFORM::POLICY::GetPolicyBool( POLICY_KEY_DATACOLLECTION );
    if( policyState != KIPLATFORM::POLICY::PBOOL::NOT_CONFIGURED )
    {
        return policyState == KIPLATFORM::POLICY::PBOOL::ENABLED;
    }

    return m_isOptedIn;
#else
    return false;
#endif
}


void SENTRY::LogAssert( const ASSERT_CACHE_KEY& aKey, const wxString& aAssertMsg )
{
#ifdef KICAD_USE_SENTRY
    if( !APP_MONITOR::SENTRY::Instance()->IsOptedIn() )
    {
        return;
    }

    // We use an assert cache to avoid collecting too many events
    // Because they can come from functions that are called hundreds of times in loops
    if( m_assertCache.find( aKey ) == m_assertCache.end() )
    {
        sentry_value_t exc = sentry_value_new_exception( "assert", aAssertMsg.c_str() );
        sentry_value_set_stacktrace( exc, NULL, 0 );

        sentry_value_t sentryEvent = sentry_value_new_event();
        sentry_event_add_exception( sentryEvent, exc );
        sentry_capture_event( sentryEvent );
        m_assertCache.insert( aKey );
    }
#endif
}


void SENTRY::LogException( const wxString& aMsg, bool aUnhandled )
{
#ifdef KICAD_USE_SENTRY
    if( !APP_MONITOR::SENTRY::Instance()->IsOptedIn() )
    {
        return;
    }

    sentry_scope_t* local_scope = sentry_local_scope_new();
    sentry_scope_set_tag( local_scope, "unhandled", aUnhandled ? "true" : "false" );

    sentry_value_t exc = sentry_value_new_exception( "exception", aMsg.c_str() );
    sentry_value_set_stacktrace( exc, NULL, 0 );

    sentry_value_t sentryEvent = sentry_value_new_event();
    sentry_event_add_exception( sentryEvent, exc );
    sentry_capture_event_with_scope( sentryEvent, local_scope );
#endif
}

#ifdef KICAD_USE_SENTRY
static std::string GetSentryBreadCrumbType( BREADCRUMB_TYPE aType )
{
    // the only special case due to collisions with defines
    if( aType ==  BREADCRUMB_TYPE::DBG )
        return "debug";

    if( aType == BREADCRUMB_TYPE::ERR )
        return "error";

    std::string ret( magic_enum::enum_name( aType ) );

    std::transform( ret.begin(), ret.end(), ret.begin(),
                    []( unsigned char c )
                    {
                        return std::tolower( c );
                    } );

    return ret;
}


static std::string GetSentryBreadCrumbLevel( BREADCRUMB_LEVEL aLevel )
{
    // the only special case due to collisions with defines
    if( aLevel == BREADCRUMB_LEVEL::DBG )
        return "debug";

    if( aLevel == BREADCRUMB_LEVEL::ERR )
        return "error";

    std::string ret( magic_enum::enum_name( aLevel ) );

    std::transform( ret.begin(), ret.end(), ret.begin(),
                    []( unsigned char c )
                    {
                        return std::tolower( c );
                    } );

    return ret;
}
#endif

namespace APP_MONITOR
{
SENTRY* SENTRY::m_instance = nullptr;


bool operator<( const ASSERT_CACHE_KEY& aKey1, const ASSERT_CACHE_KEY& aKey2 )
{
    return aKey1.file < aKey2.file || aKey1.line < aKey2.line || aKey1.func < aKey2.func || aKey1.cond < aKey2.cond;
}


void AddBreadcrumb( BREADCRUMB_TYPE aType, const wxString& aMsg, const wxString& aCategory, BREADCRUMB_LEVEL aLevel )
{
#ifdef KICAD_USE_SENTRY
    if( !SENTRY::Instance()->IsOptedIn() )
    {
        return;
    }

    std::string    type = GetSentryBreadCrumbType( aType );
    sentry_value_t crumb = sentry_value_new_breadcrumb( type.c_str(), aMsg.c_str() );

    sentry_value_set_by_key( crumb, "category", sentry_value_new_string( aCategory.c_str() ) );

    if( aType == BREADCRUMB_TYPE::ERR )
    {
        std::string level = GetSentryBreadCrumbLevel( aLevel );
        sentry_value_set_by_key( crumb, "level", sentry_value_new_string( level.c_str() ) );
    }

    sentry_add_breadcrumb( crumb );
#endif
}


void AddNavigationBreadcrumb( const wxString& aMsg, const wxString& aCategory )
{
#ifdef KICAD_USE_SENTRY
    AddBreadcrumb( BREADCRUMB_TYPE::NAVIGATION, aMsg, aCategory, BREADCRUMB_LEVEL::INFO );
#endif
}


void AddTransactionBreadcrumb( const wxString& aMsg, const wxString& aCategory )
{
#ifdef KICAD_USE_SENTRY
    AddBreadcrumb( BREADCRUMB_TYPE::TRANSACTION, aMsg, aCategory, BREADCRUMB_LEVEL::INFO );
#endif
}

#ifdef KICAD_USE_SENTRY
class TRANSACTION_IMPL
{
public:
    TRANSACTION_IMPL( const std::string& aName, const std::string& aOperation )
    {
        m_ctx = sentry_transaction_context_new( aName.c_str(), aOperation.c_str() );
    }

    ~TRANSACTION_IMPL() {
        Finish();

        // note m_ctx is handled by sentry
    }

    void Start()
    {
        m_tx = sentry_transaction_start( m_ctx, sentry_value_new_null() );
    }

    void Finish()
    {
        FinishSpan();

        if( m_tx )
        {
            sentry_transaction_finish( m_tx );
            m_tx = nullptr;
        }
    }

    void StartSpan( const std::string& aOperation, const std::string& aDescription )
    {
        if( m_span )
            return;

        if( !m_tx )
            return;

        m_span = sentry_transaction_start_child( m_tx, aOperation.c_str(), aDescription.c_str() );
    }

    void FinishSpan()
    {
        if( m_span )
        {
            sentry_span_finish( m_span );
            m_span = nullptr;
        }
    }

private:
    sentry_transaction_context_t* m_ctx = nullptr;
    sentry_transaction_t*         m_tx = nullptr;
    sentry_span_t*                m_span = nullptr;
};
#endif
}


TRANSACTION::TRANSACTION( const std::string& aName, const std::string& aOperation )
{
#ifdef KICAD_USE_SENTRY
    if( SENTRY::Instance()->IsOptedIn() )
    {
        m_impl = new TRANSACTION_IMPL( aName, aOperation );
    }
#endif
}


TRANSACTION::~TRANSACTION()
{
#ifdef KICAD_USE_SENTRY
    delete m_impl;
#endif
}


void TRANSACTION::Start()
{
#ifdef KICAD_USE_SENTRY
    if( m_impl )
    {
        m_impl->Start();
    }
#endif
}


void TRANSACTION::StartSpan( const std::string& aOperation, const std::string& aDescription )
{
#ifdef KICAD_USE_SENTRY
    if( m_impl )
    {
        m_impl->StartSpan( aOperation, aDescription );
    }
#endif
}


void TRANSACTION::Finish()
{
#ifdef KICAD_USE_SENTRY
    if( m_impl )
    {
        m_impl->Finish();
    }
#endif
}


void TRANSACTION::FinishSpan()
{
#ifdef KICAD_USE_SENTRY
    if( m_impl )
    {
        m_impl->FinishSpan();
    }
#endif
}