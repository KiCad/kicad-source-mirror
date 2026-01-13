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

#pragma once

#include <kicommon.h>
#include <string>
#include <set>
#include <wx/string.h>
#include <wx/filename.h>

namespace APP_MONITOR
{
	enum class BREADCRUMB_TYPE
    {
        DEFAULT,
        DBG,
        ERR,
        NAVIGATION,
        INFO,
        QUERY,
        TRANSACTION,
        UI,
        USER,
    };

	enum class BREADCRUMB_LEVEL
	{
        FATAL,
        ERR,
        WARNING,
        INFO,
        DBG
	};

    class TRANSACTION_IMPL;

    /**
     * This represents a sentry transaction which is used for time-performance metrics
     * You start a transaction and can denote "spans" inside the transaction for specific
     * portions of the transaction.
     */
    class KICOMMON_API TRANSACTION
    {
    public:
        TRANSACTION( const std::string& aName, const std::string& aOperation );
        ~TRANSACTION();

        void Start();
        void StartSpan( const std::string& aOperation, const std::string& aDescription );
        void FinishSpan();
        void Finish();

#ifdef KICAD_USE_SENTRY
    private:
        // We use a IMPL to avoid seeding sentry everywhere
        TRANSACTION_IMPL* m_impl = nullptr;
#endif
    };

    /**
     * This struct represents a key being used for the std::set that deduplicates asserts
     * during this running session.
     *
     * The elements are meant to make the assert sufficiently unique but at the same time
     * avoid unnecessary noise. One notable issue was we used to use msg as a key but
     * asserts with args in loops would defeat it and cause noise
     */
    struct KICOMMON_API ASSERT_CACHE_KEY
    {
        wxString file;
        int      line;
        wxString func;
        wxString cond;
    };

    KICOMMON_API
    bool operator<( const ASSERT_CACHE_KEY& aKey1, const ASSERT_CACHE_KEY& aKey2 );

    /**
     * This is a singleton class intended to manage sentry
     *
     * The inards of the api in this class are meant to be compiled out when KICAD_USE_SENTRY
     * is not defined and become "inert" in order to reduce the need to sprinkle #ifdef checks
     * everywhere.
     */
    class KICOMMON_API SENTRY
    {
    public:
        SENTRY( const SENTRY& obj ) = delete;

        static SENTRY* Instance()
        {
            if( m_instance == nullptr )
                m_instance = new SENTRY();

            return m_instance;
        }

        void Init();
        void Cleanup();

        bool            IsOptedIn();
        void            AddTag( const wxString& aKey, const wxString& aValue );
        void            SetSentryOptIn( bool aOptIn );
        const wxString& GetSentryId();
        void            ResetSentryId();

        void LogAssert( const ASSERT_CACHE_KEY& aKey, const wxString& aMsg );
        void LogException( const wxString& aMsg, bool aUnhandled );

    private:
        SENTRY();

        bool     isConfiguredOptedIn();
        void     sentryInit();
        wxString sentryCreateUid();
        void     readOrCreateUid();

        static SENTRY* m_instance;

        bool       m_isOptedIn;
        wxFileName m_sentry_optin_fn;
        wxFileName m_sentry_uid_fn;
        wxString   m_sentryUid;

        std::set<ASSERT_CACHE_KEY> m_assertCache;
    };

    /**
     * Add a sentry breadcrumb
     */
	KICOMMON_API void AddBreadcrumb( BREADCRUMB_TYPE aType, const wxString& aMsg, const wxString& aCategory,
                                     BREADCRUMB_LEVEL aLevel = BREADCRUMB_LEVEL::INFO );


    /**
     * Add a navigation breadcrumb
     */
    KICOMMON_API void AddNavigationBreadcrumb( const wxString& aMsg, const wxString& aCategory );


    /**
     * Add a transaction breadcrumb
     */
    KICOMMON_API void AddTransactionBreadcrumb( const wxString& aMsg, const wxString& aCategory );
}