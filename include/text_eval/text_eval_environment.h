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

#pragma once

#include <kicommon.h>
#include <wx/datetime.h>
#include <wx/string.h>
#include <cstdint>
#include <functional>
#include <map>
#include <optional>
#include <string>
#include <tuple>
#include <utility>

namespace TEXT_EVAL
{
class SOURCE_SCOPE;

/**
 * A text evaluation frame with one frozen clock and memoized external queries.
 *
 * Owned and used by a single thread.  Reads made while a SOURCE_SCOPE is open are recorded so a cache can tell
 * whether evaluated text depends on anything outside the document.
 */
class KICOMMON_API ENVIRONMENT
{
public:
    enum class VCS_QUERY
    {
        HEAD,
        HASH,
        DESCRIPTION,
        SIGNATURE,
        BRANCH,
        DIRTY,
        TIMESTAMP
    };

    /// Query, absolute path (Git directory for HEAD), selector or pattern, options, and whether the context is a file
    using VCS_KEY = std::tuple<VCS_QUERY, wxString, std::string, int, bool>;

    struct VCS_VALUE
    {
        std::string text;
        int64_t     number = 0;

        bool operator==( const VCS_VALUE& ) const = default;
    };

    using CROSS_REFERENCE_KEY = std::pair<wxString, int>;

    struct CROSS_REFERENCE_VALUE
    {
        wxString text;
        bool     resolved = false;

        bool operator==( const CROSS_REFERENCE_VALUE& ) const = default;
    };

    struct SOURCE_VALUES
    {
        std::map<wxString, std::optional<wxString>>          environmentVariables;
        bool                                                 randomUsed = false;
        std::optional<wxDateTime>                            time;
        std::map<wxString, wxString>                         gitHashes;
        std::map<VCS_KEY, VCS_VALUE>                         vcsValues;
        std::map<CROSS_REFERENCE_KEY, CROSS_REFERENCE_VALUE> crossReferences;

        bool operator==( const SOURCE_VALUES& ) const = default;
    };

    explicit ENVIRONMENT( const wxDateTime& aTime = wxDateTime::Now() ) :
            m_time( aTime )
    {
    }

    ENVIRONMENT( const ENVIRONMENT& ) = delete;
    ENVIRONMENT& operator=( const ENVIRONMENT& ) = delete;
    ~ENVIRONMENT();

    static ENVIRONMENT* Current();

    /**
     * @return the frozen time of the active environment, or the wall clock when none is active.
     */
    static wxDateTime CurrentTime();

    void RecordEnvironmentVariable( const wxString& aName, const std::optional<wxString>& aValue );
    void RecordRandomUse();
    void RecordCrossReference( const CROSS_REFERENCE_KEY& aKey, const CROSS_REFERENCE_VALUE& aValue );

    bool IsCollectingSources() const { return m_sources != nullptr; }

    /**
     * Return the value memoized for this frame.  \a aRead runs only on the first request for a key.
     */
    const wxString&  GitHash( const wxString& aPath, const std::function<wxString()>& aRead );
    const VCS_VALUE& VcsValue( const VCS_KEY& aKey, const std::function<VCS_VALUE()>& aRead );

private:
    friend class SOURCE_SCOPE;

    wxDateTime                   m_time;
    std::map<wxString, wxString> m_gitHashes;
    std::map<VCS_KEY, VCS_VALUE> m_vcsValues;
    SOURCE_SCOPE*                m_sources = nullptr;
};


/**
 * Record every source read through \a aEnvironment into \a aValues, including memo hits.
 *
 * Nested scopes each receive every read.  Clock reads are recorded only while the environment is active.
 * Scopes must be destroyed in reverse order and must not outlive the environment or the values.
 */
class KICOMMON_API SOURCE_SCOPE
{
public:
    SOURCE_SCOPE( ENVIRONMENT& aEnvironment, ENVIRONMENT::SOURCE_VALUES& aValues );
    ~SOURCE_SCOPE();

    SOURCE_SCOPE( const SOURCE_SCOPE& ) = delete;
    SOURCE_SCOPE& operator=( const SOURCE_SCOPE& ) = delete;

private:
    friend class ENVIRONMENT;

    ENVIRONMENT&                m_environment;
    ENVIRONMENT::SOURCE_VALUES& m_values;
    SOURCE_SCOPE*               m_previous;
};


/**
 * Make an environment current on this thread for the lifetime of the scope.
 *
 * Scopes nest.  Other threads, including thread pool workers, keep evaluating against the wall clock.
 */
class KICOMMON_API ENVIRONMENT_SCOPE
{
public:
    explicit ENVIRONMENT_SCOPE( ENVIRONMENT& aEnvironment );
    ~ENVIRONMENT_SCOPE();

    ENVIRONMENT_SCOPE( const ENVIRONMENT_SCOPE& ) = delete;
    ENVIRONMENT_SCOPE& operator=( const ENVIRONMENT_SCOPE& ) = delete;

private:
    ENVIRONMENT* m_previous;
};
} // namespace TEXT_EVAL
