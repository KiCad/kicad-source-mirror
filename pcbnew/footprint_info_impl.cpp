/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2011 Jean-Pierre Charras, <jp.charras@wanadoo.fr>
 * Copyright (C) 2013-2016 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright (C) 1992-2022 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <dialogs/html_message_box.h>
#include <footprint.h>
#include <footprint_info.h>
#include <fp_lib_table.h>
#include <kiway.h>
#include <locale_io.h>
#include <lib_id.h>
#include <progress_reporter.h>
#include <string_utils.h>
#include <thread_pool.h>
#include <wildcards_and_files_ext.h>

#include <wx/textfile.h>
#include <wx/txtstrm.h>
#include <wx/wfstream.h>


void FOOTPRINT_INFO_IMPL::load()
{
    FP_LIB_TABLE* fptable = m_owner->GetTable();

    wxASSERT( fptable );

    const FOOTPRINT* footprint = fptable->GetEnumeratedFootprint( m_nickname, m_fpname );

    if( footprint == nullptr ) // Should happen only with malformed/broken libraries
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


bool FOOTPRINT_LIST_IMPL::ReadFootprintFiles( FP_LIB_TABLE* aTable, const wxString* aNickname,
                                              PROGRESS_REPORTER* aProgressReporter )
{
    long long int generatedTimestamp = 0;

    if( !CatchErrors( [&]()
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
        m_progress_reporter->Report( _( "Fetching footprint libraries..." ) );
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
            m_progress_reporter->Report( _( "Loading footprints..." ) );
        }

        loadFootprints();

        if( m_progress_reporter )
            m_progress_reporter->AdvancePhase();
    }

    if( m_cancelled )
        m_list_timestamp = 0;       // God knows what we got before we were canceled
    else
        m_list_timestamp = generatedTimestamp;

    return m_errors.empty();
}


void FOOTPRINT_LIST_IMPL::loadLibs()
{
    thread_pool& tp = GetKiCadThreadPool();
    size_t num_returns = m_queue_in.size();
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


void FOOTPRINT_LIST_IMPL::loadFootprints()
{
    LOCALE_IO toggle_locale;

    // Parse the footprints in parallel. WARNING! This requires changing the locale, which is
    // GLOBAL. It is only thread safe to construct the LOCALE_IO before the threads are created,
    // destroy it after they finish, and block the main (GUI) thread while they work. Any deviation
    // from this will cause nasal demons.
    //
    // TODO: blast LOCALE_IO into the sun

    SYNC_QUEUE<std::unique_ptr<FOOTPRINT_INFO>> queue_parsed;
    thread_pool&                                tp = GetKiCadThreadPool();
    size_t                                      num_elements = m_queue_out.size();
    std::vector<std::future<size_t>>            returns( num_elements );

    auto fp_thread =
            [ this, &queue_parsed ]() -> size_t
            {
                wxString nickname;

                if( m_cancelled || !m_queue_out.pop( nickname ) )
                    return 0;

                wxArrayString fpnames;

                CatchErrors(
                        [&]()
                        {
                            m_lib_table->FootprintEnumerate( fpnames, nickname, false );
                        } );

                for( wxString fpname : fpnames )
                {
                    CatchErrors(
                            [&]()
                            {
                                auto* fpinfo = new FOOTPRINT_INFO_IMPL( this, nickname, fpname );
                                queue_parsed.move_push( std::unique_ptr<FOOTPRINT_INFO>( fpinfo ) );
                            } );

                    if( m_cancelled )
                        return 0;
                }

                if( m_progress_reporter )
                    m_progress_reporter->AdvanceProgress();

                return 1;
            };

    for( size_t ii = 0; ii < num_elements; ++ii )
        returns[ii] = tp.submit( fp_thread );

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

    std::unique_ptr<FOOTPRINT_INFO> fpi;

    while( queue_parsed.pop( fpi ) )
        m_list.push_back( std::move( fpi ) );

    std::sort( m_list.begin(), m_list.end(),
               []( std::unique_ptr<FOOTPRINT_INFO> const& lhs,
                   std::unique_ptr<FOOTPRINT_INFO> const& rhs ) -> bool
               {
                   return *lhs < *rhs;
               } );
}


FOOTPRINT_LIST_IMPL::FOOTPRINT_LIST_IMPL() :
    m_list_timestamp( 0 ),
    m_progress_reporter( nullptr ),
    m_cancelled( false )
{
}


void FOOTPRINT_LIST_IMPL::WriteCacheToFile( const wxString& aFilePath )
{
    wxFileName          tmpFileName = wxFileName::CreateTempFileName( aFilePath );
    wxFFileOutputStream outStream( tmpFileName.GetFullPath() );
    wxTextOutputStream  txtStream( outStream );

    if( !outStream.IsOk() )
    {
        return;
    }

    txtStream << wxString::Format( wxT( "%lld" ), m_list_timestamp ) << endl;

    for( std::unique_ptr<FOOTPRINT_INFO>& fpinfo : m_list )
    {
        txtStream << fpinfo->GetLibNickname() << endl;
        txtStream << fpinfo->GetName() << endl;
        txtStream << EscapeString( fpinfo->GetDescription(), CTX_LINE ) << endl;
        txtStream << EscapeString( fpinfo->GetKeywords(), CTX_LINE ) << endl;
        txtStream << wxString::Format( wxT( "%d" ), fpinfo->GetOrderNum() ) << endl;
        txtStream << wxString::Format( wxT( "%u" ), fpinfo->GetPadCount() ) << endl;
        txtStream << wxString::Format( wxT( "%u" ), fpinfo->GetUniquePadCount() ) << endl;
    }

    txtStream.Flush();
    outStream.Close();

    if( !wxRenameFile( tmpFileName.GetFullPath(), aFilePath, true ) )
    {
        // cleanup in case rename failed
        // its also not the end of the world since this is just a cache file
        wxRemoveFile( tmpFileName.GetFullPath() );
    }
}


void FOOTPRINT_LIST_IMPL::ReadCacheFromFile( const wxString& aFilePath )
{
    wxTextFile cacheFile( aFilePath );

    m_list_timestamp = 0;
    m_list.clear();

    try
    {
        if( cacheFile.Exists() && cacheFile.Open() )
        {
            cacheFile.GetFirstLine().ToLongLong( &m_list_timestamp );

            while( cacheFile.GetCurrentLine() + 6 < cacheFile.GetLineCount() )
            {
                wxString             libNickname    = cacheFile.GetNextLine();
                wxString             name           = cacheFile.GetNextLine();
                wxString             desc           = UnescapeString( cacheFile.GetNextLine() );
                wxString             keywords       = UnescapeString( cacheFile.GetNextLine() );
                int                  orderNum       = wxAtoi( cacheFile.GetNextLine() );
                unsigned int         padCount       = (unsigned) wxAtoi( cacheFile.GetNextLine() );
                unsigned int         uniquePadCount = (unsigned) wxAtoi( cacheFile.GetNextLine() );

                FOOTPRINT_INFO_IMPL* fpinfo = new FOOTPRINT_INFO_IMPL( libNickname, name, desc,
                                                                       keywords, orderNum,
                                                                       padCount,  uniquePadCount );

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

    if( cacheFile.IsOpened() )
        cacheFile.Close();
}
