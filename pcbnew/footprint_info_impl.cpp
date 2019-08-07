/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2011 Jean-Pierre Charras, <jp.charras@wanadoo.fr>
 * Copyright (C) 2013-2016 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright (C) 1992-2019 KiCad Developers, see AUTHORS.txt for contributors.
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
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */


#include <footprint_info_impl.h>

#include <class_module.h>
#include <common.h>
#include <fctsys.h>
#include <footprint_info.h>
#include <fp_lib_table.h>
#include <html_messagebox.h>
#include <io_mgr.h>
#include <kiface_ids.h>
#include <kiway.h>
#include <lib_id.h>
#include <macros.h>
#include <pgm_base.h>
#include <wildcards_and_files_ext.h>
#include <widgets/progress_reporter.h>

#include <thread>
#include <mutex>


void FOOTPRINT_INFO_IMPL::load()
{
    FP_LIB_TABLE* fptable = m_owner->GetTable();

    wxASSERT( fptable );

    const MODULE* footprint = fptable->GetEnumeratedFootprint( m_nickname, m_fpname );

    if( footprint == NULL ) // Should happen only with malformed/broken libraries
    {
        m_pad_count = 0;
        m_unique_pad_count = 0;
    }
    else
    {
        m_pad_count = footprint->GetPadCount( DO_NOT_INCLUDE_NPTH );
        m_unique_pad_count = footprint->GetUniquePadCount( DO_NOT_INCLUDE_NPTH );
        m_keywords = footprint->GetKeywords();
        m_doc = footprint->GetDescription();
    }

    m_loaded = true;
}


bool FOOTPRINT_LIST_IMPL::CatchErrors( const std::function<void()>& aFunc )
{
    try
    {
        aFunc();
    }
    catch( const IO_ERROR& ioe )
    {
        m_errors.move_push( std::make_unique<IO_ERROR>( ioe ) );
        return false;
    }
    catch( const std::exception& se )
    {
        // This is a round about way to do this, but who knows what THROW_IO_ERROR()
        // may be tricked out to do someday, keep it in the game.
        try
        {
            THROW_IO_ERROR( se.what() );
        }
        catch( const IO_ERROR& ioe )
        {
            m_errors.move_push( std::make_unique<IO_ERROR>( ioe ) );
        }
        return false;
    }

    return true;
}


void FOOTPRINT_LIST_IMPL::loader_job()
{
    wxString nickname;

    while( m_queue_in.pop( nickname ) && !m_cancelled )
    {
        CatchErrors( [this, &nickname]() {
            m_lib_table->PrefetchLib( nickname );
            m_queue_out.push( nickname );
        } );

        m_count_finished.fetch_add( 1 );

        if( m_progress_reporter )
            m_progress_reporter->AdvanceProgress();
    }
}


bool FOOTPRINT_LIST_IMPL::ReadFootprintFiles( FP_LIB_TABLE* aTable, const wxString* aNickname,
                                              PROGRESS_REPORTER* aProgressReporter )
{
    long long int generatedTimestamp = aTable->GenerateTimestamp( aNickname );

    if( generatedTimestamp == m_list_timestamp )
        return true;

    m_progress_reporter = aProgressReporter;
    m_cancelled = false;

    FOOTPRINT_ASYNC_LOADER loader;

    loader.SetList( this );
    loader.Start( aTable, aNickname );

    if( m_progress_reporter )
    {
        m_progress_reporter->SetMaxProgress( m_queue_in.size() );
        m_progress_reporter->Report( _( "Fetching Footprint Libraries" ) );
    }

    while( !m_cancelled && (int)m_count_finished.load() < m_loader->m_total_libs )
    {
        if( m_progress_reporter && !m_progress_reporter->KeepRefreshing() )
            m_cancelled = true;

        wxMilliSleep( 20 );
    }

    if( m_cancelled )
    {
        loader.Abort();
    }
    else
    {
        if( m_progress_reporter )
        {
            m_progress_reporter->AdvancePhase();
            m_progress_reporter->SetMaxProgress( m_queue_out.size() );
            m_progress_reporter->Report( _( "Loading Footprints" ) );
        }

        loader.Join();

        if( m_progress_reporter )
            m_progress_reporter->AdvancePhase();
    }

    if( m_cancelled )
        m_list_timestamp = 0;       // God knows what we got before we were cancelled
    else
        m_list_timestamp = generatedTimestamp;

    return m_errors.empty();
}


void FOOTPRINT_LIST_IMPL::StartWorkers( FP_LIB_TABLE* aTable, wxString const* aNickname,
        FOOTPRINT_ASYNC_LOADER* aLoader, unsigned aNThreads )
{
    m_loader = aLoader;
    m_lib_table = aTable;

    // Clear data before reading files
    m_count_finished.store( 0 );
    m_errors.clear();
    m_list.clear();
    m_threads.clear();
    m_queue_in.clear();
    m_queue_out.clear();

    if( aNickname )
        m_queue_in.push( *aNickname );
    else
    {
        for( auto const& nickname : aTable->GetLogicalLibs() )
            m_queue_in.push( nickname );
    }

    m_loader->m_total_libs = m_queue_in.size();

    for( unsigned i = 0; i < aNThreads; ++i )
    {
        m_threads.push_back( std::thread( &FOOTPRINT_LIST_IMPL::loader_job, this ) );
    }
}

void FOOTPRINT_LIST_IMPL::StopWorkers()
{
    std::lock_guard<std::mutex> lock1( m_join );

    // To safely stop our workers, we set the cancellation flag (they will each
    // exit on their next safe loop location when this is set).  Then we need to wait
    // for all threads to finish as closing the implementation will free the queues
    // that the threads write to.
    for( auto& i : m_threads )
        i.join();

    m_threads.clear();
    m_queue_in.clear();
    m_count_finished.store( 0 );

    // If we have cancelled in the middle of a load, clear our timestamp to re-load next time
    if( m_cancelled )
        m_list_timestamp = 0;
}

bool FOOTPRINT_LIST_IMPL::JoinWorkers()
{
    {
        std::lock_guard<std::mutex> lock1( m_join );

        for( auto& i : m_threads )
            i.join();

        m_threads.clear();
        m_queue_in.clear();
        m_count_finished.store( 0 );
    }

    size_t total_count = m_queue_out.size();

    LOCALE_IO toggle_locale;

    // Parse the footprints in parallel. WARNING! This requires changing the locale, which is
    // GLOBAL. It is only threadsafe to construct the LOCALE_IO before the threads are created,
    // destroy it after they finish, and block the main (GUI) thread while they work. Any deviation
    // from this will cause nasal demons.
    //
    // TODO: blast LOCALE_IO into the sun

    SYNC_QUEUE<std::unique_ptr<FOOTPRINT_INFO>> queue_parsed;
    std::vector<std::thread>                    threads;

    for( size_t ii = 0; ii < std::thread::hardware_concurrency() + 1; ++ii )
    {
        threads.push_back( std::thread( [this, &queue_parsed]() {
            wxString nickname;

            while( this->m_queue_out.pop( nickname ) && !m_cancelled )
            {
                wxArrayString fpnames;

                try
                {
                    m_lib_table->FootprintEnumerate( fpnames, nickname );
                }
                catch( const IO_ERROR& ioe )
                {
                    m_errors.move_push( std::make_unique<IO_ERROR>( ioe ) );
                }
                catch( const std::exception& se )
                {
                    // This is a round about way to do this, but who knows what THROW_IO_ERROR()
                    // may be tricked out to do someday, keep it in the game.
                    try
                    {
                        THROW_IO_ERROR( se.what() );
                    }
                    catch( const IO_ERROR& ioe )
                    {
                        m_errors.move_push( std::make_unique<IO_ERROR>( ioe ) );
                    }
                }

                for( unsigned jj = 0; jj < fpnames.size() && !m_cancelled; ++jj )
                {
                    wxString fpname = fpnames[jj];
                    FOOTPRINT_INFO* fpinfo = new FOOTPRINT_INFO_IMPL( this, nickname, fpname );
                    queue_parsed.move_push( std::unique_ptr<FOOTPRINT_INFO>( fpinfo ) );
                }

                if( m_progress_reporter )
                    m_progress_reporter->AdvanceProgress();

                m_count_finished.fetch_add( 1 );
            }
        } ) );
    }

    while( !m_cancelled && (size_t)m_count_finished.load() < total_count )
    {
        if( m_progress_reporter && !m_progress_reporter->KeepRefreshing() )
            m_cancelled = true;

        wxMilliSleep( 30 );
    }

    for( auto& thr : threads )
        thr.join();

    std::unique_ptr<FOOTPRINT_INFO> fpi;

    while( queue_parsed.pop( fpi ) )
        m_list.push_back( std::move( fpi ) );

    std::sort( m_list.begin(), m_list.end(), []( std::unique_ptr<FOOTPRINT_INFO> const& lhs,
                                                 std::unique_ptr<FOOTPRINT_INFO> const& rhs ) -> bool
                                             {
                                                 return *lhs < *rhs;
                                             } );

    return m_errors.empty();
}


FOOTPRINT_LIST_IMPL::FOOTPRINT_LIST_IMPL() :
    m_loader( nullptr ),
    m_count_finished( 0 ),
    m_list_timestamp( 0 ),
    m_progress_reporter( nullptr ),
    m_cancelled( false )
{
}


FOOTPRINT_LIST_IMPL::~FOOTPRINT_LIST_IMPL()
{
    StopWorkers();
}


void FOOTPRINT_LIST_IMPL::WriteCacheToFile( wxTextFile* aCacheFile )
{
    if( aCacheFile->Exists() )
    {
        aCacheFile->Open();
        aCacheFile->Clear();
    }
    else
    {
        aCacheFile->Create();
    }

    aCacheFile->AddLine( wxString::Format( "%lld", m_list_timestamp ) );

    for( auto& fpinfo : m_list )
    {
        aCacheFile->AddLine( fpinfo->GetLibNickname() );
        aCacheFile->AddLine( fpinfo->GetName() );
        aCacheFile->AddLine( EscapeString( fpinfo->GetDescription(), CTX_DELIMITED_STR ) );
        aCacheFile->AddLine( EscapeString( fpinfo->GetKeywords(), CTX_DELIMITED_STR ) );
        aCacheFile->AddLine( wxString::Format( "%d", fpinfo->GetOrderNum() ) );
        aCacheFile->AddLine( wxString::Format( "%u", fpinfo->GetPadCount() ) );
        aCacheFile->AddLine( wxString::Format( "%u", fpinfo->GetUniquePadCount() ) );
    }

    aCacheFile->Write();
    aCacheFile->Close();
}


void FOOTPRINT_LIST_IMPL::ReadCacheFromFile( wxTextFile* aCacheFile )
{
    m_list_timestamp = 0;
    m_list.clear();

    try
    {
        if( aCacheFile->Exists() )
        {
            aCacheFile->Open();

            aCacheFile->GetFirstLine().ToLongLong( &m_list_timestamp );

            while( aCacheFile->GetCurrentLine() + 6 < aCacheFile->GetLineCount() )
            {
                wxString libNickname = aCacheFile->GetNextLine();
                wxString name = aCacheFile->GetNextLine();
                wxString description = UnescapeString( aCacheFile->GetNextLine() );
                wxString keywords = UnescapeString( aCacheFile->GetNextLine() );
                int orderNum = wxAtoi( aCacheFile->GetNextLine() );
                unsigned int padCount = (unsigned) wxAtoi( aCacheFile->GetNextLine() );
                unsigned int uniquePadCount = (unsigned) wxAtoi( aCacheFile->GetNextLine() );

                auto* fpinfo = new FOOTPRINT_INFO_IMPL( libNickname, name, description, keywords,
                                                        orderNum, padCount, uniquePadCount );
                m_list.emplace_back( std::unique_ptr<FOOTPRINT_INFO>( fpinfo ) );
            }
        }
    }
    catch( ... )
    {
        // whatever went wrong, invalidate the cache
        m_list_timestamp = 0;
    }

    // Sanity check: an empty list is very unlikely to be correct.
    if( m_list.size() == 0 )
        m_list_timestamp = 0;

    if( aCacheFile->IsOpened() )
        aCacheFile->Close();
}
