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

#ifndef KICAD_DB_BACKEND_SUPPORT_H
#define KICAD_DB_BACKEND_SUPPORT_H

#include <chrono>
#include <functional>
#include <future>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <thread>
#include <vector>

#include <wx/string.h>

#include <database/database_backend.h>
#include "db_worker_thread.h"


/// @return aText as UTF-8, the encoding the backends take paths, names and messages in
inline std::string utf8( const wxString& aText )
{
    return std::string( aText.ToUTF8() );
}


/// What a backend reports, and what it does with its connection, when a worker call did not complete
struct DB_WORKER_FAILURE
{
    enum class CONNECTION
    {
        KEEP,  ///< Nothing ran, so the connection is intact
        DROP,  ///< The connection is closed through the worker
        FORGET ///< The stuck job releases the connection itself when its blocked call returns
    };

    DB_FAILURE  failure;
    std::string message;
    CONNECTION  connection;
};


/// Maps the outcome of a call that returned no result to a failure and a connection decision
DB_WORKER_FAILURE DescribeWorkerOutcome( DB_WORKER_THREAD::OUTCOME aOutcome, const std::string& aPath );


/**
 * Hands rows from a worker job to a caller's sink and gives the caller the column labels.
 *
 * The job fills Labels() before its first row, so they are copied out when that row arrives or, for a read
 * that returned no rows, by Finish().
 */
class DB_READ_DELIVERY
{
public:
    DB_READ_DELIVERY( std::vector<std::string>& aLabels, const DB_ROW_SINK& aSink );

    DB_READ_DELIVERY( const DB_READ_DELIVERY& ) = delete;
    DB_READ_DELIVERY& operator=( const DB_READ_DELIVERY& ) = delete;

    /// The labels the job fills in
    const std::shared_ptr<std::vector<std::string>>& Labels() const { return m_jobLabels; }

    /// The sink to pass to the worker call
    const DB_ROW_SINK* Sink() const { return &m_deliver; }

    /// Copies the labels out if no row did, after the call returned a result
    void Finish();

    /// True if the caller's sink threw
    bool SinkThrew() const { return m_sinkThrew; }

    /// The message for a read the caller's sink gave up on
    static std::string SinkFailureMessage();

private:
    std::vector<std::string>&                 m_labels;
    const DB_ROW_SINK&                        m_sink;
    std::shared_ptr<std::vector<std::string>> m_jobLabels;
    DB_ROW_SINK                               m_deliver;
    bool                                      m_labelsCopied = false;
    bool                                      m_sinkThrew = false;
};


/**
 * Replaces a secret in a message, as ***.  The doubled-quote form that an initialization string holds for a secret
 * containing a quote goes too.
 *
 * A secret of four or more characters goes wherever it occurs.  A shorter one goes only where it stands apart from
 * letters, digits and non-ASCII bytes, since replacing a one-character secret inside words would leave the message
 * unreadable.  That leaves a short secret hiding inside a longer word, so provider text that may echo a short secret
 * must go through RedactDetail instead.
 */
std::string ScrubSecret( std::string aText, const std::string& aSecret );


/// The provider text to show for a call made with aSecret, which is nothing at all when the secret is short
std::string RedactDetail( const std::string& aDetail, const std::string& aSecret );


/**
 * Runs a slow probe once on a thread of its own and keeps its answer for good.
 *
 * Only a finished answer is kept.  A caller that gives up waiting gets nothing, and the next caller
 * waits on the same probe, so contention that delays the probe is never taken for a negative result.
 */
template <typename T>
class DB_PROBE_LATCH
{
public:
    explicit DB_PROBE_LATCH( std::function<T()> aProbe ) :
            m_probe( std::move( aProbe ) )
    {
    }

    /// The probe's answer, or nothing if it has not finished within aWait.  A probe that throws answers T().
    std::optional<T> Get( std::chrono::milliseconds aWait )
    {
        std::call_once( m_started,
                        [this]()
                        {
                            auto promise = std::make_shared<std::promise<T>>();

                            m_answer = promise->get_future().share();

                            std::thread(
                                    [promise, probe = m_probe]()
                                    {
                                        try
                                        {
                                            promise->set_value( probe() );
                                        }
                                        catch( ... )
                                        {
                                            promise->set_value( T() );
                                        }
                                    } )
                                    .detach();
                        } );

        const std::shared_future<T> answer = m_answer;

        if( answer.wait_for( aWait ) != std::future_status::ready )
            return std::nullopt;

        return answer.get();
    }

private:
    std::function<T()>    m_probe;
    std::once_flag        m_started;
    std::shared_future<T> m_answer;
};

#endif
