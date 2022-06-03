/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2020-2022 KiCad Developers.
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


// WARNING - this Tom's crappy PNS hack tool code. Please don't complain about its quality
// (unless you want to improve it).

#include "pns_log_file.h"

#include <board_design_settings.h>

#include <pcbnew/plugins/kicad/pcb_plugin.h>
#include <pcbnew/drc/drc_engine.h>

#include <../../unittests/common/console_log.h>

BOARD_CONNECTED_ITEM* PNS_LOG_FILE::ItemById( const PNS_LOG_FILE::EVENT_ENTRY& evt )
{
    BOARD_CONNECTED_ITEM* parent = nullptr;

    for( auto item : m_board->AllConnectedItems() )
    {
        if( item->m_Uuid == evt.uuid )
        {
            parent = item;
            break;
        };
    }

    return parent;
}

static const wxString readLine( FILE* f )
{
    char str[16384];
    fgets( str, sizeof( str ) - 1, f );
    return wxString( str );
}


PNS_LOG_FILE::PNS_LOG_FILE()
{
    m_routerSettings.reset( new PNS::ROUTING_SETTINGS( nullptr, "" ) );
}


bool PNS_LOG_FILE::Load( const wxFileName& logFileName )
{
    wxFileName fname_log( logFileName );
    fname_log.SetExt( wxT( "log" ) );

    wxFileName fname_dump( logFileName );
    fname_dump.SetExt( wxT( "dump" ) );

    wxFileName fname_project( logFileName );
    fname_project.SetExt( wxT( "kicad_pro" ) );
    fname_project.MakeAbsolute();

    FILE* f = fopen( fname_log.GetFullPath().c_str(), "rb" );

    printf("Loading dump from '%s'\n", (const char*) fname_log.GetFullPath().c_str() );

    if( !f )
        return false;

    while( !feof( f ) )
    {
        wxStringTokenizer tokens( readLine( f ) );

        if( !tokens.CountTokens() )
            continue;

        wxString cmd = tokens.GetNextToken();

        if( cmd == "event" )
        {
            EVENT_ENTRY evt;
            evt.p.x = wxAtoi( tokens.GetNextToken() );
            evt.p.y = wxAtoi( tokens.GetNextToken() );
            evt.type = (PNS::LOGGER::EVENT_TYPE) wxAtoi( tokens.GetNextToken() );
            evt.uuid = KIID( tokens.GetNextToken() );
            m_events.push_back( evt );
        }
        else if( cmd == "config" )
        {
            m_routerSettings->SetMode( (PNS::PNS_MODE) wxAtoi( tokens.GetNextToken() ) );
            m_routerSettings->SetRemoveLoops( wxAtoi( tokens.GetNextToken() ) );
            m_routerSettings->SetFixAllSegments( wxAtoi( tokens.GetNextToken() ) );
            m_routerSettings->SetCornerMode(
                    (DIRECTION_45::CORNER_MODE) wxAtoi( tokens.GetNextToken() ) );
        }
    }

    fclose( f );

    
    m_settingsMgr.reset( new SETTINGS_MANAGER ( true ) );
    m_settingsMgr->LoadProject( fname_project.GetFullPath() );

    try
    {
        PCB_PLUGIN io;
        m_board.reset( io.Load( fname_dump.GetFullPath(), nullptr, nullptr ) );
        m_board->SetProject( m_settingsMgr->GetProject( fname_project.GetFullPath() ) );

        std::shared_ptr<DRC_ENGINE> drcEngine( new DRC_ENGINE );

        CONSOLE_LOG            consoleLog;
        BOARD_DESIGN_SETTINGS& bds = m_board->GetDesignSettings();

        bds.m_DRCEngine = drcEngine;

        m_board->SynchronizeNetsAndNetClasses();

        drcEngine->SetBoard( m_board.get() );
        drcEngine->SetDesignSettings( &bds );
        drcEngine->SetLogReporter( new CONSOLE_MSG_REPORTER( &consoleLog ) );
        drcEngine->InitEngine( wxFileName() );
    }
    catch( const PARSE_ERROR& parse_error )
    {
        printf( "parse error : %s (%s)\n", (const char*) parse_error.Problem().c_str(),
                (const char*) parse_error.What().c_str() );

        return false;
    }

    return true;
}
