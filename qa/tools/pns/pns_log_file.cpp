/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers.
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

#include <wx/filename.h>
#include <wx/ffile.h>
#include <wx/stdstream.h>
#include <wx/wfstream.h>

#include "pns_log_file.h"
#include "pns_arc.h"

#include <router/pns_segment.h>

#include <board_design_settings.h>

#include <pcbnew/pcb_io/kicad_sexpr/pcb_io_kicad_sexpr.h>
#include <pcbnew/drc/drc_engine.h>

#include <project.h>
#include <project/project_local_settings.h>
#include <wildcards_and_files_ext.h>

#include <../../tests/common/console_log.h>

std::vector<BOARD_CONNECTED_ITEM*> PNS_LOG_FILE::ItemsById( const PNS::LOGGER::EVENT_ENTRY& evt )
{
    std::vector<BOARD_CONNECTED_ITEM*> parents;

    parents.resize( evt.uuids.size() );

    for( BOARD_CONNECTED_ITEM* item : m_board->AllConnectedItems() )
    {
        for( int i = 0; i < evt.uuids.size(); i++ )
        {
            if( item->m_Uuid == evt.uuids[i] )
            {
                parents[i] = item;
                break;
            };
        }
    }

    return parents;
}

BOARD_CONNECTED_ITEM* PNS_LOG_FILE::ItemById( const PNS::LOGGER::EVENT_ENTRY& evt )
{
    auto parents = ItemsById( evt );
    if ( parents.size() > 0 )
        return parents[0];

    return nullptr;
}


static const wxString readLine( FILE* f )
{
    char str[16384];
    fgets( str, sizeof( str ) - 1, f );
    return wxString( str );
}


PNS_LOG_FILE::PNS_LOG_FILE() :
    m_mode( PNS::ROUTER_MODE::PNS_MODE_ROUTE_SINGLE )
{
    m_routerSettings.reset( new PNS::ROUTING_SETTINGS( nullptr, "" ) );
}


std::shared_ptr<SHAPE> PNS_LOG_FILE::parseShape( const nlohmann::json& aJSON )
{
    const wxString type = static_cast<wxString>( aJSON.at( "type" ).get<wxString>() );

    if( type == wxT("segment") )
    {
        std::shared_ptr<SHAPE_SEGMENT> sh( new SHAPE_SEGMENT );
        sh->SetSeg( SEG( aJSON.at( "start" ).get<VECTOR2I>(), aJSON.at( "end" ).get<VECTOR2I>() ) );
        sh->SetWidth( aJSON.at( "width" ).get<int>() );
        return sh;
    }
    else if( type == wxT("circle") )
    {
        std::shared_ptr<SHAPE_CIRCLE> sh( new SHAPE_CIRCLE );
        sh->SetCenter( aJSON.at( "center" ).get<VECTOR2I>() );
        sh->SetRadius( aJSON.at( "radius" ).get<int>() );
        return sh;
    }
    else if( type == wxT("arc") )
    {
        VECTOR2I start = aJSON.at( "start" ).get<VECTOR2I>();
        VECTOR2I mid = aJSON.at( "mid" ).get<VECTOR2I>();
        VECTOR2I end = aJSON.at( "end" ).get<VECTOR2I>();
        int width = aJSON.at( "width" ).get<int>();

        std::shared_ptr<SHAPE_ARC> sh( new SHAPE_ARC( start, mid, end, width ) );
        return sh;
    }

    return nullptr;
}


std::shared_ptr<SHAPE> PNS_LOG_FILE::parseLegacyShape( SHAPE_TYPE expectedType, wxStringTokenizer& aTokens )
{
    SHAPE_TYPE type = static_cast<SHAPE_TYPE> ( wxAtoi( aTokens.GetNextToken() ) );

    if( type == SHAPE_TYPE::SH_SEGMENT )
    {
        std::shared_ptr<SHAPE_SEGMENT> sh( new SHAPE_SEGMENT );
        VECTOR2I a, b;
        a.x = wxAtoi( aTokens.GetNextToken() );
        a.y = wxAtoi( aTokens.GetNextToken() );
        b.x = wxAtoi( aTokens.GetNextToken() );
        b.y = wxAtoi( aTokens.GetNextToken() );
        int width = wxAtoi( aTokens.GetNextToken() );
        sh->SetSeg( SEG( a, b ));
        sh->SetWidth( width );
        return sh;
    }
    else if( type == SHAPE_TYPE::SH_CIRCLE )
    {
        std::shared_ptr<SHAPE_CIRCLE> sh(  new SHAPE_CIRCLE );
        VECTOR2I a;
        a.x = wxAtoi( aTokens.GetNextToken() );
        a.y = wxAtoi( aTokens.GetNextToken() );
        int radius = wxAtoi( aTokens.GetNextToken() );
        sh->SetCenter( a );
        sh->SetRadius( radius );
        return sh;
    }

    return nullptr;
}

bool PNS_LOG_FILE::parseLegacyCommonPnsProps( PNS::ITEM* aItem, const wxString& cmd,
                                        wxStringTokenizer& aTokens )
{
    if( cmd == wxS( "net" ) )
    {
        aItem->SetNet( m_board->FindNet( wxAtoi( aTokens.GetNextToken() ) ) );
        return true;
    }
    else if( cmd == wxS( "layers" ) )
    {
        int start = wxAtoi( aTokens.GetNextToken() );
        int end = wxAtoi( aTokens.GetNextToken() );
        aItem->SetLayers( PNS_LAYER_RANGE( start, end ) );
        return true;
    }
    return false;
}


bool PNS_LOG_FILE::parseCommonPnsProps( const nlohmann::json& aJSON, PNS::ITEM* aItem )
{
    aItem->SetNet( m_board->FindNet( aJSON.at( "net" ).get<wxString>() ) );
    aItem->SetLayers(
            PNS_LAYER_RANGE( aJSON.at( "layers" ).at( 0 ).get<int>(), aJSON.at( "layers" ).at( 1 ).get<int>() ) );

    return true;
}

std::unique_ptr<PNS::SEGMENT> PNS_LOG_FILE::parseLegacyPnsSegmentFromString( wxStringTokenizer& aTokens )
{
    std::unique_ptr<PNS::SEGMENT> seg( new PNS::SEGMENT() );

    while( aTokens.CountTokens() )
    {
        wxString cmd = aTokens.GetNextToken();

        if( !parseLegacyCommonPnsProps( seg.get(), cmd, aTokens ) )
        {
            if( cmd == wxS( "shape" ) )
            {
                std::shared_ptr<SHAPE> sh = parseLegacyShape( SH_SEGMENT, aTokens );

                if( !sh )
                    return nullptr;

                seg->SetShape( *static_cast<SHAPE_SEGMENT*>( sh.get() ) );

            }
        }
    }

    return seg;
}

std::unique_ptr<PNS::VIA> PNS_LOG_FILE::parseLegacyPnsViaFromString( wxStringTokenizer& aTokens )
{
    std::unique_ptr<PNS::VIA> via( new PNS::VIA() );

    while( aTokens.CountTokens() )
    {
        wxString cmd = aTokens.GetNextToken();

        if( !parseLegacyCommonPnsProps( via.get(), cmd, aTokens ) )
        {
            if( cmd == wxS( "shape" ) )
            {
                std::shared_ptr<SHAPE> sh = parseLegacyShape( SH_CIRCLE, aTokens );

                if( !sh )
                    return nullptr;

                SHAPE_CIRCLE* sc = static_cast<SHAPE_CIRCLE*>( sh.get() );

                via->SetPos( sc->GetCenter() );
                via->SetDiameter( PNS::VIA::ALL_LAYERS, 2 * sc->GetRadius() );
            }
            else if( cmd == wxS( "drill" ) )
            {
                via->SetDrill( wxAtoi( aTokens.GetNextToken() ) );
            }
        }
    }

    return via;
}

std::unique_ptr<PNS::ITEM> PNS_LOG_FILE::parseItem( const nlohmann::json& aJSON )
{
    wxString kind = aJSON.at("kind").get<wxString>();

    if( kind == wxT("segment") )
    {
        auto parsedShape = parseShape( aJSON.at("shape") );

        if( !parsedShape )
            return nullptr;

        auto shape = static_cast<const SHAPE_SEGMENT*>( parsedShape.get() );
        std::unique_ptr<PNS::SEGMENT> seg( new PNS::SEGMENT( *shape, nullptr ) );
        parseCommonPnsProps( aJSON, seg.get() );
        return std::move( seg );
    }
    else if ( kind == wxT( "arc" ) )
    {
        auto parsedShape = parseShape( aJSON.at("shape") );

        if( !parsedShape )
            return nullptr;

        auto shape = static_cast<const SHAPE_ARC*>( parsedShape.get() );
        std::unique_ptr<PNS::ARC> arc( new PNS::ARC( *shape, nullptr ) );
        parseCommonPnsProps( aJSON, arc.get() );
        return std::move( arc );
    }
    else if ( kind == wxT( "via" ) )
    {
        auto parsedShape = parseShape( aJSON.at("shape") );

        if( !parsedShape )
            return nullptr;

        auto shape = static_cast<const SHAPE_CIRCLE*>( parsedShape.get() );
        std::unique_ptr<PNS::VIA> via( new PNS::VIA() );
        parseCommonPnsProps( aJSON, via.get() );
        via->SetPos( shape->Centre() );
        via->SetDiameter( via->Layers().Start(), shape->GetRadius() * 2 );
        via->SetDrill( aJSON.at("drill").get<int>() );
        return std::move(via);
    }

    return nullptr;
}


std::unique_ptr<PNS::ITEM> PNS_LOG_FILE::parseLegacyItemFromString( wxStringTokenizer& aTokens )
{
    wxString type = aTokens.GetNextToken();

    if( type == wxS( "segment" ) )
        return parseLegacyPnsSegmentFromString( aTokens );
    else if( type == wxS( "via" ) )
        return parseLegacyPnsViaFromString( aTokens );

    return nullptr;
}

bool comparePnsItems( const PNS::ITEM* a , const PNS::ITEM* b )
{
    if( a->Kind() != b->Kind() )
        return false;

    if( a->Net() != b->Net() )
        return false;

    if( a->Layers() != b->Layers() )
        return false;

    if( a->Kind() == PNS::ITEM::VIA_T )
    {
        const PNS::VIA* va = static_cast<const PNS::VIA*>(a);
        const PNS::VIA* vb = static_cast<const PNS::VIA*>(b);

        // TODO(JE) padstacks
        if( va->Diameter( PNS::VIA::ALL_LAYERS ) != vb->Diameter( PNS::VIA::ALL_LAYERS ) )
            return false;

        if( va->Drill() != vb->Drill() )
            return false;

        if( va->Pos() != vb->Pos() )
            return false;

    }
    else if ( a->Kind() == PNS::ITEM::SEGMENT_T )
    {
        const PNS::SEGMENT* sa = static_cast<const PNS::SEGMENT*>(a);
        const PNS::SEGMENT* sb = static_cast<const PNS::SEGMENT*>(b);

        if( sa->Seg() != sb->Seg() )
            return false;

        if( sa->Width() != sb->Width() )
            return false;
    }

    return true;
}


const std::set<PNS::ITEM*> deduplicate( const std::vector<PNS::ITEM*>& items )
{
    std::set<PNS::ITEM*> rv;

    for( PNS::ITEM* item : items )
    {
        bool isDuplicate = false;

        for( PNS::ITEM* ritem : rv )
        {
            if( comparePnsItems( ritem, item) )
            {
                isDuplicate = true;
                break;
            }

            if( !isDuplicate )
                rv.insert( item );
        }
    }

    return rv;
}


bool PNS_LOG_FILE::COMMIT_STATE::Compare( const PNS_LOG_FILE::COMMIT_STATE& aOther )
{
    COMMIT_STATE check( aOther );

    //printf("pre-compare: %d/%d\n", check.m_addedItems.size(), check.m_removedIds.size() );
    //printf("pre-compare (log): %d/%d\n", m_addedItems.size(), m_removedIds.size() );

    for( const KIID& uuid : m_removedIds )
    {
        if( check.m_removedIds.find( uuid ) != check.m_removedIds.end() )
            check.m_removedIds.erase( uuid );
        else
            return false; // removed twice? wtf
    }

    std::set<PNS::ITEM*> addedItems = deduplicate( m_addedItems );
    std::set<PNS::ITEM*> chkAddedItems = deduplicate( check.m_addedItems );

    for( PNS::ITEM* item : addedItems )
    {
        for( PNS::ITEM* chk : chkAddedItems )
        {
            if( comparePnsItems( item, chk ) )
            {
                chkAddedItems.erase( chk );
                break;
            }
        }
    }

    //printf("post-compare: %d/%d\n", chkAddedItems.size(), check.m_removedIds.size() );

    if( chkAddedItems.empty() && check.m_removedIds.empty() )
        return true;
    else
        return false;   // Set breakpoint here to trap failing tests
}


bool PNS_LOG_FILE::SaveLog( const wxFileName& logFileName, REPORTER* aRpt )
{
    PNS::LOGGER::LOG_DATA logData;

    logData.m_AddedItems = m_commitState.m_addedItems;
    logData.m_RemovedItems = m_commitState.m_removedIds;
    logData.m_Heads = m_commitState.m_heads;
    logData.m_BoardHash = m_boardHash;
    logData.m_TestCaseType = m_testCaseType;
    logData.m_Events = m_events;
    logData.m_Mode = m_mode;
    
    wxString logString = PNS::LOGGER::FormatLogFileAsJSON( logData );

    wxFFileOutputStream fp( logFileName.GetFullPath(), wxT( "wt" ) );

    if( !fp.IsOk() )
    {
        if( aRpt )
        {
            aRpt->Report( wxString::Format( wxT("Failed to write log file: %s"), logFileName.GetFullPath() ), RPT_SEVERITY_ERROR );
        }
        return false;
    }

    wxScopedCharBuffer utf8 = logString.ToUTF8();
    fp.Write( utf8.data(), utf8.length() );
    fp.Close();

    return true;
}


bool PNS_LOG_FILE::Load( const wxFileName& logFileName, REPORTER* aRpt, const wxString boardFileName )
{
    wxFileName fname_log( logFileName );
    fname_log.SetExt( wxT( "log" ) );

    wxFileName fname_dump( logFileName );
    fname_dump.SetExt( wxT( "dump" ) );

    if( !boardFileName.IsEmpty() )
    {
        fname_dump = boardFileName;
    }

    wxFileName fname_project( logFileName );
    fname_project.SetExt( wxT( "kicad_pro" ) );
    fname_project.MakeAbsolute();

    wxFileName fname_settings( logFileName );
    fname_settings.SetExt( wxT( "settings" ) );

    aRpt->Report( wxString::Format( wxT( "Loading router settings from '%s'" ),
                                    fname_settings.GetFullPath() ) );

    bool ok = m_routerSettings->LoadFromRawFile( fname_settings.GetFullPath() );

    if( !ok )
    {
        aRpt->Report( wxT( "Failed to load routing settings. Using defaults." ),
                      RPT_SEVERITY_WARNING );
    }

    aRpt->Report( wxString::Format( wxT( "Loading project settings from '%s'" ),
                                    fname_settings.GetFullPath() ) );

    m_settingsMgr.reset( new SETTINGS_MANAGER );
    m_settingsMgr->LoadProject( fname_project.GetFullPath() );
    PROJECT* project = m_settingsMgr->GetProject( fname_project.GetFullPath() );
    project->SetReadOnly();

    try
    {
        PCB_IO_KICAD_SEXPR io;
        aRpt->Report( wxString::Format( wxT("Loading board snapshot from '%s'"),
                                        fname_dump.GetFullPath() ) );

        m_board.reset( io.LoadBoard( fname_dump.GetFullPath(), nullptr, nullptr ) );
        m_board->SetProject( project );

        std::shared_ptr<DRC_ENGINE> drcEngine( new DRC_ENGINE );

        BOARD_DESIGN_SETTINGS& bds = m_board->GetDesignSettings();

        bds.m_DRCEngine = drcEngine;
        bds.m_UseConnectedTrackWidth = project->GetLocalSettings().m_AutoTrackWidth;

        m_board->SynchronizeNetsAndNetClasses( true );

        drcEngine->SetBoard( m_board.get() );
        drcEngine->SetDesignSettings( &bds );
        drcEngine->SetLogReporter( aRpt );

        // Load the test case's custom DRC rules if it ships any.
        wxFileName fname_rules( logFileName );
        fname_rules.SetExt( FILEEXT::DesignRulesFileExtension );

        if( fname_rules.FileExists() )
            drcEngine->InitEngine( fname_rules );
        else
            drcEngine->InitEngine( wxFileName() );
    }
    catch( const PARSE_ERROR& parse_error )
    {
        aRpt->Report( wxString::Format( "parse error : %s (%s)\n",
                                        parse_error.Problem(),
                                        parse_error.What() ),
                      RPT_SEVERITY_ERROR );

        return false;
    }


    ok = loadJsonLog( logFileName.GetFullPath(), aRpt, false );
    if( !ok && logFileName.FileExists() )
    {
        aRpt->Report("Falling back to legacy log format...\n", RPT_SEVERITY_WARNING);
        ok = loadLegacyLog( logFileName.GetFullPath(), aRpt );
    }

    return ok;
}


bool PNS_LOG_FILE::loadJsonLog( const wxString& aFilename, REPORTER* aRpt, bool aHashOnly )
{
    wxFFileInputStream fp( aFilename, wxT( "rt" ) );
    wxStdInputStream   fstream( fp );


    if ( aRpt )
    {
        aRpt->Report( wxString::Format( "Loading log from: %s", aFilename ) );
    }

    if( !fp.IsOk() )
    {
        if( aRpt )
            aRpt->Report( wxT("Failed to load."), RPT_SEVERITY_ERROR );

        return false;
    }

    try
    {
        nlohmann::json logJson = nlohmann::json::parse( fstream, nullptr,
                                                        /* allow_exceptions = */ true,
                                                        /* ignore_comments  = */ true );

        if( logJson.contains("board_hash") )
        {
            m_boardHash = logJson.at("board_hash").get<wxString>();
        }

        if( logJson.contains("test_case_type") )
        {
            m_testCaseType = static_cast<PNS::LOGGER::TEST_CASE_TYPE>( logJson.at("test_case_type").get<int>() );
        }

        if ( aHashOnly )
            return true;

        m_mode = static_cast<PNS::ROUTER_MODE>( logJson.at( "mode" ).get<int>() );

        for( const nlohmann::json& event : logJson.at( "events" ) )
        {
            m_events.push_back( std::move( PNS::LOGGER::ParseEventFromJSON( event ) ) );
        }

        for( const nlohmann::json& addedItem : logJson.at( "addedItems" ) )
        {
            m_parsed_items.push_back( std::move( parseItem( addedItem ) ) );
            m_commitState.m_addedItems.push_back( m_parsed_items.back().get() );
        }

        for( const nlohmann::json& addedItem : logJson.at( "removedItems" ) )
        {
            m_commitState.m_removedIds.insert( addedItem.get<KIID>() );
        }

        if( aRpt )
        {
            aRpt->Report( wxString::Format( "JSON log load: %lu events, %lu added, %lu removed\n", m_events.size(),
                                            m_commitState.m_addedItems.size(), m_commitState.m_removedIds.size() ),
                          RPT_SEVERITY_INFO );
        }
            

    }
    catch( const std::exception& exc )
    {
        if( aRpt )
        {
            aRpt->Report( wxString::Format( "JSON log parse failure: %s\n", exc.what() ), RPT_SEVERITY_ERROR );
        }
        return false;
    }

    return true;
}


bool PNS_LOG_FILE::loadLegacyLog( const wxString& aFilename, REPORTER* aRpt )
{
    FILE* f = fopen( aFilename.c_str(), "rb" );

    aRpt->Report( wxString::Format( "Loading log from '%s'", aFilename ) );

    if( !f )
    {
        aRpt->Report( wxT( "Failed to load log file." ), RPT_SEVERITY_ERROR );
        return false;
    }

    try
    {
        while( !feof( f ) )
        {
            wxString          line = readLine( f );
            wxStringTokenizer tokens( line );

            if( !tokens.CountTokens() )
                continue;

            wxString cmd = tokens.GetNextToken();

            if( cmd == wxT( "mode" ) )
            {
                m_mode = static_cast<PNS::ROUTER_MODE>( wxAtoi( tokens.GetNextToken() ) );
            }
            else if( cmd == wxT( "event" ) )
            {
                m_events.push_back( std::move( PNS::LOGGER::ParseEvent( line ) ) );
            }
            else if( cmd == wxT( "added" ) )
            {
                m_parsed_items.push_back( std::move( parseLegacyItemFromString( tokens ) ) );
                m_commitState.m_addedItems.push_back( m_parsed_items.back().get() );
            }
            else if( cmd == wxT( "removed" ) )
            {
                m_commitState.m_removedIds.insert( KIID( tokens.GetNextToken() ) );
            }
        }
    }
    catch( ... )
    {
        return false;
    }

    fclose( f );
    return true;
}

const std::optional<wxString> PNS_LOG_FILE::GetLogBoardHash( const wxString& logFileName )
{
    loadJsonLog( logFileName, nullptr, true );
    return m_boardHash;
}

