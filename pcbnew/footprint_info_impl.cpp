/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2011 Jean-Pierre Charras, <jp.charras@wanadoo.fr>
 * Copyright (C) 2013-2016 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
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
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */


#include <footprint_info_impl.h>

#include <dialogs/html_message_box.h>
#include <footprint.h>
#include <footprint_info.h>
#include <footprint_library_adapter.h>
#include <kiway.h>
#include <lib_id.h>
#include <progress_reporter.h>
#include <string_utils.h>
#include <thread_pool.h>
#include <wildcards_and_files_ext.h>

#include <kiplatform/io.h>

#include <wx/textfile.h>
#include <wx/txtstrm.h>
#include <wx/wfstream.h>


void FOOTPRINT_INFO_IMPL::load()
{
    FOOTPRINT_LIBRARY_ADAPTER* adapter = m_owner->GetAdapter();
    wxCHECK( adapter, /* void */ );

    try
    {
        std::unique_ptr<FOOTPRINT> footprint( adapter->LoadFootprint( m_nickname, m_fpname, false ) );

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
            m_doc = footprint->GetLibDescription();
        }
    }
    catch( const IO_ERROR& ioe )
    {
        // Store error in the owner's error list for later display
        if( m_owner )
            m_owner->PushError( std::make_unique<IO_ERROR>( ioe ) );

        m_pad_count = 0;
        m_unique_pad_count = 0;
    }

    m_loaded = true;
}


void FOOTPRINT_LIST_IMPL::Clear()
{
    std::unique_lock<std::mutex> lock( m_loadInProgress );

    m_list.clear();
    m_list_timestamp = 0;
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


bool FOOTPRINT_LIST_IMPL::ReadFootprintFiles( FOOTPRINT_LIBRARY_ADAPTER* aAdapter, const wxString* aNickname,
                                              PROGRESS_REPORTER* aProgressReporter )
{
    std::unique_lock<std::mutex> lock( m_loadInProgress );

    m_adapter = aAdapter;
    m_adapter->BlockUntilLoaded();

    long long int generatedTimestamp = 0;

    if( !CatchErrors( [&]()
                 {
                     generatedTimestamp = aAdapter->GenerateTimestamp( aNickname );
                 } ) )
    {
        return false;
    }

    if( generatedTimestamp == m_list_timestamp )
        return true;

    // Disable KIID generation: not needed for library parts; sometimes very slow
    KIID_NIL_SET_RESET reset_kiid;

    m_progress_reporter = aProgressReporter;

    m_cancelled = false;

    // Clear data before reading files
    m_errors.clear();
    m_list.clear();
    m_queue.clear();

    if( aNickname )
    {
        m_queue.push( *aNickname );
    }
    else
    {
        for( const wxString& nickname : aAdapter->GetLibraryNames() )
            m_queue.push( nickname );
    }

    if( m_progress_reporter )
    {
        m_progress_reporter->SetMaxProgress( (int) m_queue.size() );
        m_progress_reporter->Report( _( "Loading footprints..." ) );
    }

    loadFootprints();

    if( m_progress_reporter )
        m_progress_reporter->AdvancePhase();

    if( m_cancelled )
        m_list_timestamp = 0;       // God knows what we got before we were canceled
    else
        m_list_timestamp = generatedTimestamp;

    return m_errors.empty();
}


void FOOTPRINT_LIST_IMPL::loadFootprints()
{
    // Parse the footprints in parallel.

    SYNC_QUEUE<std::unique_ptr<FOOTPRINT_INFO>> queue_parsed;
    thread_pool&                                tp = GetKiCadThreadPool();
    size_t                                      num_elements = m_queue.size();
    std::vector<std::future<size_t>>            returns( num_elements );

    auto fp_thread =
            [ this, &queue_parsed ]() -> size_t
            {
                // Each thread pool worker needs its own KIID_NIL_SET_RESET since g_createNilUuids
                // is thread_local. This prevents generating real UUIDs for temporary footprint data.
                KIID_NIL_SET_RESET reset_kiid;

                wxString nickname;

                if( m_cancelled || !m_queue.pop( nickname ) )
                    return 0;

                std::vector<wxString> fpnames;

                CatchErrors(
                        [&]()
                        {
                            fpnames = m_adapter->GetFootprintNames( nickname );
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
        returns[ii] = tp.submit_task( fp_thread );

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
                   if( !lhs || !rhs )
                       return false;

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
        txtStream << EscapeString( fpinfo->GetDesc(), CTX_LINE ) << endl;
        txtStream << EscapeString( fpinfo->GetKeywords(), CTX_LINE ) << endl;
        txtStream << wxString::Format( wxT( "%d" ), fpinfo->GetOrderNum() ) << endl;
        txtStream << wxString::Format( wxT( "%u" ), fpinfo->GetPadCount() ) << endl;
        txtStream << wxString::Format( wxT( "%u" ), fpinfo->GetUniquePadCount() ) << endl;
    }

    txtStream.Flush();
    outStream.Close();

    // Preserve the permissions of the current file
    KIPLATFORM::IO::DuplicatePermissions( aFilePath, tmpFileName.GetFullPath() );

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
