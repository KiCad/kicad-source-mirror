/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2021 Jon Evans <jon@craftyjon.com>
 * Copyright (C) 2021 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <thread>

#include <core/wx_stl_compat.h>
#include <symbol_async_loader.h>
#include <symbol_lib_table.h>
#include <widgets/progress_reporter.h>


SYMBOL_ASYNC_LOADER::SYMBOL_ASYNC_LOADER( const std::vector<wxString>& aNicknames,
        SYMBOL_LIB_TABLE* aTable, bool aOnlyPowerSymbols,
        std::unordered_map<wxString, std::vector<LIB_SYMBOL*>>* aOutput,
        PROGRESS_REPORTER* aReporter ) :
        m_nicknames( aNicknames ),
        m_table( aTable ),
        m_onlyPowerSymbols( aOnlyPowerSymbols ),
        m_output( aOutput ),
        m_reporter( aReporter ),
        m_nextLibrary( 0 ),
        m_canceled( false )
{
    wxASSERT( m_table );
    m_threadCount = std::max<size_t>( 1, std::thread::hardware_concurrency() - 1 );

    m_returns.resize( m_threadCount );
}



SYMBOL_ASYNC_LOADER::~SYMBOL_ASYNC_LOADER()
{
    Abort();
}


void SYMBOL_ASYNC_LOADER::Start()
{
    for( size_t ii = 0; ii < m_threadCount; ++ii )
        m_returns[ii] = std::async( std::launch::async, &SYMBOL_ASYNC_LOADER::worker, this );
}


bool SYMBOL_ASYNC_LOADER::Join()
{
    for( size_t ii = 0; ii < m_threadCount; ++ii )
    {
        if( !m_returns[ii].valid() )
            continue;

        m_returns[ii].wait();

        const std::vector<LOADED_PAIR>& ret = m_returns[ii].get();

        if( m_output && !ret.empty() )
        {
            for( const LOADED_PAIR& pair : ret )
            {
                // Don't show libraries that had no power symbols
                if( m_onlyPowerSymbols && pair.second.empty() )
                    continue;

                // *Do* show empty libraries in the normal case
                m_output->insert( pair );
            }
        }
    }

    return true;
}


void SYMBOL_ASYNC_LOADER::Abort()
{
    m_canceled.store( true );
    Join();
}


bool SYMBOL_ASYNC_LOADER::Done()
{
    return m_nextLibrary.load() >= m_nicknames.size();
}


std::vector<SYMBOL_ASYNC_LOADER::LOADED_PAIR> SYMBOL_ASYNC_LOADER::worker()
{
    std::vector<LOADED_PAIR> ret;

    bool onlyPower = m_onlyPowerSymbols;

    for( size_t libraryIndex = m_nextLibrary++; libraryIndex < m_nicknames.size();
         libraryIndex = m_nextLibrary++ )
    {
        if( m_canceled.load() )
            break;

        const wxString& nickname = m_nicknames[libraryIndex];
        LOADED_PAIR     pair( nickname, {} );

        try
        {
            m_table->LoadSymbolLib( pair.second, nickname, onlyPower );
            ret.emplace_back( std::move( pair ) );
        }
        catch( const IO_ERROR& ioe )
        {
            wxString msg = wxString::Format( _( "Error loading symbol library %s.\n\n%s\n" ),
                                             nickname, ioe.What() );

            std::lock_guard<std::mutex> lock( m_errorMutex );
            m_errors += msg;
        }

        if( m_reporter )
            m_reporter->AdvancePhase( wxString::Format( _( "Loading library \"%s\"" ), nickname ) );
    }

    return ret;
}
