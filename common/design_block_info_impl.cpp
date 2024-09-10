/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 1992-2024 KiCad Developers, see AUTHORS.txt for contributors.
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


#include <design_block_info_impl.h>

#include <design_block.h>
#include <design_block_info.h>
#include <design_block_lib_table.h>
#include <kiway.h>
#include <locale_io.h>
#include <lib_id.h>
#include <progress_reporter.h>
#include <string_utils.h>
#include <core/thread_pool.h>
#include <wildcards_and_files_ext.h>

#include <kiplatform/io.h>

#include <wx/textfile.h>
#include <wx/txtstrm.h>
#include <wx/wfstream.h>


void DESIGN_BLOCK_INFO_IMPL::load()
{
    DESIGN_BLOCK_LIB_TABLE* dbtable = m_owner->GetTable();

    wxASSERT( dbtable );

    const DESIGN_BLOCK* design_block = dbtable->GetEnumeratedDesignBlock( m_nickname, m_dbname );

    if( design_block )
    {
        m_keywords = design_block->GetKeywords();
        m_doc = design_block->GetLibDescription();
    }

    m_loaded = true;
}


bool DESIGN_BLOCK_LIST_IMPL::CatchErrors( const std::function<void()>& aFunc )
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


bool DESIGN_BLOCK_LIST_IMPL::ReadDesignBlockFiles( DESIGN_BLOCK_LIB_TABLE* aTable,
                                                   const wxString*         aNickname,
                                                   PROGRESS_REPORTER*      aProgressReporter )
{
    long long int generatedTimestamp = 0;

    if( !CatchErrors(
                [&]()
                {
                    generatedTimestamp = aTable->GenerateTimestamp( aNickname );
                } ) )
    {
        return false;
    }

    if( generatedTimestamp == m_list_timestamp )
        return true;

    // Disable KIID generation: not needed for library parts; sometimes very slow
    KIID_NIL_SET_RESET reset_kiid;

    m_progress_reporter = aProgressReporter;

    if( m_progress_reporter )
    {
        m_progress_reporter->SetMaxProgress( m_queue_in.size() );
        m_progress_reporter->Report( _( "Fetching design_block libraries..." ) );
    }

    m_cancelled = false;
    m_lib_table = aTable;

    // Clear data before reading files
    m_errors.clear();
    m_list.clear();
    m_queue_in.clear();
    m_queue_out.clear();

    if( aNickname )
    {
        m_queue_in.push( *aNickname );
    }
    else
    {
        for( const wxString& nickname : aTable->GetLogicalLibs() )
            m_queue_in.push( nickname );
    }


    loadLibs();

    if( !m_cancelled )
    {
        if( m_progress_reporter )
        {
            m_progress_reporter->SetMaxProgress( m_queue_out.size() );
            m_progress_reporter->AdvancePhase();
            m_progress_reporter->Report( _( "Loading design_blocks..." ) );
        }

        loadDesignBlocks();

        if( m_progress_reporter )
            m_progress_reporter->AdvancePhase();
    }

    if( m_cancelled )
        m_list_timestamp = 0; // God knows what we got before we were canceled
    else
        m_list_timestamp = generatedTimestamp;

    return m_errors.empty();
}


void DESIGN_BLOCK_LIST_IMPL::loadLibs()
{
    thread_pool&                     tp = GetKiCadThreadPool();
    size_t                           num_returns = m_queue_in.size();
    std::vector<std::future<size_t>> returns( num_returns );

    auto loader_job =
            [this]() -> size_t
            {
                wxString nickname;
                size_t retval = 0;

                if( !m_cancelled && m_queue_in.pop( nickname ) )
                {
                    if( CatchErrors( [this, &nickname]()
                                     {
                                         m_lib_table->PrefetchLib( nickname );
                                         m_queue_out.push( nickname );
                                     } ) && m_progress_reporter )
                    {
                        m_progress_reporter->AdvanceProgress();
                    }

                    ++retval;
                }

                return retval;
            };

    for( size_t ii = 0; ii < num_returns; ++ii )
        returns[ii] = tp.submit( loader_job );

    for( const std::future<size_t>& ret : returns )
    {
        std::future_status status = ret.wait_for( std::chrono::milliseconds( 250 ) );

        while( status != std::future_status::ready )
        {
            if( m_progress_reporter && !m_progress_reporter->KeepRefreshing() )
                m_cancelled = true;

            status = ret.wait_for( std::chrono::milliseconds( 250 ) );
        }
    }
}


void DESIGN_BLOCK_LIST_IMPL::loadDesignBlocks()
{
    LOCALE_IO toggle_locale;

    // Parse the design_blocks in parallel. WARNING! This requires changing the locale, which is
    // GLOBAL. It is only thread safe to construct the LOCALE_IO before the threads are created,
    // destroy it after they finish, and block the main (GUI) thread while they work. Any deviation
    // from this will cause nasal demons.
    //
    // TODO: blast LOCALE_IO into the sun

    SYNC_QUEUE<std::unique_ptr<DESIGN_BLOCK_INFO>> queue_parsed;
    thread_pool&                                tp = GetKiCadThreadPool();
    size_t                                      num_elements = m_queue_out.size();
    std::vector<std::future<size_t>>            returns( num_elements );

    auto db_thread =
            [ this, &queue_parsed ]() -> size_t
            {
                wxString nickname;

                if( m_cancelled || !m_queue_out.pop( nickname ) )
                    return 0;

                wxArrayString dbnames;

                CatchErrors(
                        [&]()
                        {
                            m_lib_table->DesignBlockEnumerate( dbnames, nickname, false );
                        } );

                for( wxString dbname : dbnames )
                {
                    CatchErrors(
                            [&]()
                            {
                                auto* dbinfo = new DESIGN_BLOCK_INFO_IMPL( this, nickname, dbname );
                                queue_parsed.move_push( std::unique_ptr<DESIGN_BLOCK_INFO>( dbinfo ) );
                            } );

                    if( m_cancelled )
                        return 0;
                }

                if( m_progress_reporter )
                    m_progress_reporter->AdvanceProgress();

                return 1;
            };

    for( size_t ii = 0; ii < num_elements; ++ii )
        returns[ii] = tp.submit( db_thread );

    for( const std::future<size_t>& ret : returns )
    {
        std::future_status status = ret.wait_for( std::chrono::milliseconds( 250 ) );

        while( status != std::future_status::ready )
        {
            if( m_progress_reporter )
                m_progress_reporter->KeepRefreshing();

            status = ret.wait_for( std::chrono::milliseconds( 250 ) );
        }
    }

    std::unique_ptr<DESIGN_BLOCK_INFO> dbi;

    while( queue_parsed.pop( dbi ) )
        m_list.push_back( std::move( dbi ) );

    std::sort( m_list.begin(), m_list.end(),
               []( std::unique_ptr<DESIGN_BLOCK_INFO> const& lhs,
                   std::unique_ptr<DESIGN_BLOCK_INFO> const& rhs ) -> bool
               {
                   return *lhs < *rhs;
               } );
}


DESIGN_BLOCK_LIST_IMPL::DESIGN_BLOCK_LIST_IMPL() :
        m_list_timestamp( 0 ), m_progress_reporter( nullptr ), m_cancelled( false )
{
}
