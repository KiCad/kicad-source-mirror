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
 * You should have received a copy of the GNU General Public License along
 * with this program. If not, see <http://www.gnu.org/licenses/>.
 */


#include <dialogs/git/git_transfer.h>

#include <exception>

#include <wx/app.h>
#include <wx/intl.h>
#include <wx/log.h>

#include <git/git_backend.h>


void KIGIT::CallOnUi( std::function<void()> aFn )
{
    if( !wxTheApp )
        return;

    wxTheApp->CallAfter( std::move( aFn ) );
}


void KIGIT::CallIfAlive( const std::weak_ptr<bool>& aWatch, std::function<void()> aFn )
{
    CallOnUi(
            [aWatch, fn = std::move( aFn )]()
            {
                if( aWatch.lock() )
                    fn();
            } );
}


bool KIGIT::StartTransfer( const std::string& aLabel, std::function<void()> aWork )
{
    GIT_BACKEND* backend = GetGitBackend();

    return backend && backend->OrphanRegistry().Register( aLabel, std::move( aWork ) );
}


KIGIT_STATUS_REPORTER::KIGIT_STATUS_REPORTER( const std::weak_ptr<bool>&             aAlive,
                                              std::function<void( const wxString& )> aShow ) :
        PROGRESS_REPORTER_BASE( 1 ),
        m_alive( aAlive ),
        m_show( std::move( aShow ) )
{
}


bool KIGIT_STATUS_REPORTER::updateUI()
{
    wxString text;

    {
        std::lock_guard<std::mutex> lock( m_mutex );
        text = m_rptMessage;
    }

    if( text == m_lastShown )
        return true;

    m_lastShown = text;

    KIGIT::CallIfAlive( m_alive,
                        [show = m_show, text]()
                        {
                            show( text );
                        } );

    return true;
}


KIGIT_TRANSFER::~KIGIT_TRANSFER()
{
    Cancel();
    m_alive.reset();

    if( m_completion )
        *m_completion = {};
}


void KIGIT_TRANSFER::Cancel()
{
    if( m_cancel )
        m_cancel->store( true );
}


bool KIGIT_TRANSFER::Start( const std::string&                                              aLabel,
                            const std::function<void( const KIGIT_COMMON::CANCEL_TOKEN& )>& aWork,
                            const std::function<void()>&                                    aDone )
{
    // Made before the thread starts, so a cancel pressed while the task is still queued reaches
    // this transfer rather than being discarded when it begins.
    m_cancel = KIGIT_COMMON::NewCancelToken();

    std::weak_ptr<bool>        alive = m_alive;
    KIGIT_COMMON::CANCEL_TOKEN token = m_cancel;

    if( m_completion )
        *m_completion = {};

    auto completion = std::make_shared<std::function<void()>>( aDone );
    m_completion = completion;
    m_busy = true;

    bool started = false;

    try
    {
        started = KIGIT::StartTransfer( aLabel,
                                        [this, alive, token, aWork, completion]()
                                        {
                                            aWork( token );

                                            KIGIT::CallOnUi(
                                                    [this, alive, completion]()
                                                    {
                                                        // Empty the holder on the UI thread even if its owner
                                                        // has gone. The worker may release its copy afterwards.
                                                        std::function<void()> done = std::move( *completion );
                                                        *completion = {};

                                                        if( alive.lock() && m_completion == completion )
                                                        {
                                                            m_completion.reset();
                                                            m_busy = false;

                                                            if( done )
                                                                done();
                                                        }
                                                    } );
                                        } );
    }
    catch( ... )
    {
        // A failed thread construction must not leave its owner waiting for a completion
        // which cannot arrive.
        *completion = {};
        m_completion.reset();
        m_busy = false;
        throw;
    }

    if( !started )
    {
        *completion = {};
        m_completion.reset();
        m_busy = false;
    }

    return started;
}


KIGIT_QUERY::~KIGIT_QUERY()
{
    Cancel();
}


void KIGIT_QUERY::Run( std::shared_ptr<KIGIT_COMMON> aSession, std::function<void( KIGIT_COMMON& )> aWork,
                       std::function<void()> aDone )
{
    REQUEST request{ std::move( aSession ), std::move( aWork ), std::move( aDone ), ++m_generation };

    if( m_transfer.IsBusy() )
    {
        m_pending = std::move( request );
        m_transfer.Cancel();
        return;
    }

    start( std::move( request ) );
}


void KIGIT_QUERY::Cancel()
{
    ++m_generation;
    m_pending.reset();
    m_transfer.Cancel();
}


void KIGIT_QUERY::start( REQUEST aRequest )
{
    auto error = std::make_shared<std::exception_ptr>();

    auto done = [this, error, generation = aRequest.generation, complete = std::move( aRequest.done )]()
    {
        if( m_pending )
        {
            REQUEST next = std::move( *m_pending );
            m_pending.reset();
            start( std::move( next ) );
            return;
        }

        if( generation != m_generation )
            return;

        // The completion may destroy this query along with its window. Do not touch members
        // after calling it. Logging is also left to the UI thread, where the target can show it.
        if( complete )
            complete();

        if( *error )
        {
            try
            {
                std::rethrow_exception( *error );
            }
            catch( const std::exception& e )
            {
                wxLogError( _( "Could not read the Git repository: %s" ), wxString::FromUTF8( e.what() ) );
            }
            catch( ... )
            {
                wxLogError( _( "Could not read the Git repository." ) );
            }
        }
    };

    bool started = false;

    try
    {
        started = m_transfer.Start(
                "read repository",
                [error, session = std::move( aRequest.session ),
                 work = std::move( aRequest.work )]( const KIGIT_COMMON::CANCEL_TOKEN& aCancel )
                {
                    try
                    {
                        if( !session || aCancel->load() || session->IsShuttingDown() )
                            return;

                        std::unique_lock<std::mutex> lock = session->LockAction();

                        if( aCancel->load() || session->IsShuttingDown() || !session->GetRepo() )
                            return;

                        KIGIT_COMMON::OPERATION_GUARD operation( session.get(), aCancel );

                        if( !aCancel->load() && work )
                            work( *session );
                    }
                    catch( ... )
                    {
                        // Let the transfer post its completion even when a read throws, or this
                        // query would stay busy and never start the latest pending request.
                        *error = std::current_exception();
                    }
                },
                done );
    }
    catch( ... )
    {
        *error = std::current_exception();
    }

    if( !started )
        done();
}
