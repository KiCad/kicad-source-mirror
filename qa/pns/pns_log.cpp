/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2020-2021 KiCad Developers.
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

#include "pns_log.h"

#include <qa/drc_proto/drc_proto.h>

#include <board_design_settings.h>
#include <pcbnew/drc/drc_engine.h>
#include <pcbnew/drc/drc_test_provider.h>

using namespace PNS;

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


bool PNS_LOG_FILE::Load( const std::string& logName, const std::string boardName )
{
    FILE* f = fopen( logName.c_str(), "rb" );

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
        }
    }

    fclose( f );

    try
    {
        PCB_IO io;
        m_board.reset( io.Load( boardName.c_str(), nullptr, nullptr ) );

        std::shared_ptr<DRC_ENGINE> drcEngine( new DRC_ENGINE );

        CONSOLE_LOG            consoleLog;
        BOARD_DESIGN_SETTINGS& bds = m_board->GetDesignSettings();

        bds.m_DRCEngine = drcEngine;

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


PNS_TEST_ENVIRONMENT::PNS_TEST_ENVIRONMENT()
{
}


PNS_TEST_ENVIRONMENT::~PNS_TEST_ENVIRONMENT()
{
}


void PNS_TEST_ENVIRONMENT::SetMode( PNS::ROUTER_MODE mode )
{
    m_mode = mode;
}


void PNS_TEST_ENVIRONMENT::createRouter()
{
    m_iface.reset( new PNS_KICAD_IFACE_BASE );
    m_router.reset( new ROUTER );
    m_iface->SetBoard( m_board.get() );
    m_router->SetInterface( m_iface.get() );
    m_router->ClearWorld();
    m_router->SetMode( m_mode );
    m_router->SyncWorld();
    m_router->LoadSettings( new PNS::ROUTING_SETTINGS( nullptr, "" ) );
    m_router->Settings().SetMode( PNS::RM_Walkaround );
    m_router->Sizes().SetTrackWidth( 250000 );

    //m_router->Settings().SetOptimizeDraggedTrack( true );

    m_debugDecorator.Clear();
    m_iface->SetDebugDecorator( &m_debugDecorator );
}

void PNS_TEST_ENVIRONMENT::ReplayLog ( PNS_LOG_FILE* aLog, int aStartEventIndex, int aFrom,
                                       int aTo )
{

    m_board = aLog->GetBoard();

    createRouter();

    m_router->LoadSettings( aLog->GetRoutingSettings() );

    printf("Router mode: %d\n", m_router->Settings().Mode() );

    for( auto evt : aLog->Events() )
    {
        auto item = aLog->ItemById(evt);
        ITEM* ritem = item ? m_router->GetWorld()->FindItemByParent( item ) : nullptr;

        switch(evt.type)
        {
        case LOGGER::EVT_START_ROUTE:
        {
            m_debugDecorator.NewStage( "route-start", 0 );
            printf( "  rtr start-route (%d, %d) %p \n", evt.p.x, evt.p.y, ritem );
            m_router->StartRouting( evt.p, ritem, ritem ? ritem->Layers().Start() : F_Cu );
            break;
        }

        case LOGGER::EVT_START_DRAG:
        {
            m_debugDecorator.NewStage( "drag-start", 0 );
            bool rv = m_router->StartDragging( evt.p, ritem, 0 );
            printf( "  rtr start-drag (%d, %d) %p ret %d\n", evt.p.x, evt.p.y, ritem, rv ? 1 : 0 );
            break;
        }

        case LOGGER::EVT_FIX:
        {
            break;
        }

        case LOGGER::EVT_MOVE:
        {
            m_debugDecorator.NewStage( "move", 0 );
            printf( "  move -> (%d, %d)\n", evt.p.x, evt.p.y );
            m_router->Move( evt.p, ritem );
            break;
        }

        default:
            break;
        }

        PNS::NODE* node = nullptr;

        if( m_router->GetState() == PNS::ROUTER::ROUTE_TRACK )
        {
#if 0
            m_debugDecorator.BeginGroup( "head");

            auto traces = m_router->Placer()->Traces();

            for( const auto& t : traces.CItems() )
            {
                const LINE *l  = static_cast<LINE*>(t.item);
                const auto& sh = l->CLine();

                m_debugDecorator.AddLine( sh, 4, 10000 );
            }

            m_debugDecorator.EndGroup();
#endif

            node = m_router->Placer()->CurrentNode( true );
        }
        else if( m_router->GetState() == PNS::ROUTER::DRAG_SEGMENT )
        {
            node = m_router->GetDragger()->CurrentNode();
        }

        if( !node )
            return;

        NODE::ITEM_VECTOR removed, added;

        node->GetUpdatedItems( removed, added );

#if 0
        if( ! added.empty() )
        {
            bool first = true;
            m_debugDecorator.BeginGroup( "node-added-items");

            for( auto t : added )
            {
                if( t->OfKind( PNS::ITEM::SEGMENT_T ) )
                {
                    auto s = static_cast<PNS::SEGMENT*>( t );
                    m_debugDecorator.AddSegment( s->Seg(), 2 );
                    first = false;
                }
            }

            m_debugDecorator.EndGroup();
        }
#endif
    }
}


PNS_TEST_DEBUG_DECORATOR::STAGE* PNS_TEST_DEBUG_DECORATOR::currentStage()
{
    if( m_stages.empty() )
        m_stages.push_back( new STAGE() );

    return m_stages.back();
}


void PNS_TEST_DEBUG_DECORATOR::BeginGroup( const std::string& name,
                                           const SRC_LOCATION_INFO& aSrcLoc )
{
    STAGE* st = currentStage();
    DEBUG_ENT *ent = new DEBUG_ENT();

    ent->m_name = name;
    ent->m_iter = m_iter;

    if( m_activeEntry )
    {
        m_activeEntry->AddChild( ent );
    }

    printf( "LOG BeginGroup %s %p\n", name.c_str(), ent );

    m_activeEntry = ent;
    m_grouping = true;
}


void PNS_TEST_DEBUG_DECORATOR::EndGroup( const SRC_LOCATION_INFO& aSrcLoc )
{
    printf( "LOG EndGroup\n" );

    if( !m_activeEntry )
        return;

    m_activeEntry = m_activeEntry->m_parent;

    if( !m_activeEntry )
        m_grouping = false;
}

void PNS_TEST_DEBUG_DECORATOR::addEntry( DEBUG_ENT* ent )
{
    auto st = currentStage();
    m_activeEntry->AddChild( ent );
}


/*    virtual void AddLine( const SHAPE_LINE_CHAIN& aLine, const KIGFX::COLOR4D& aColor,
                          int aWidth, const std::string aName,
                          const SRC_LOCATION_INFO& aSrcLoc = SRC_LOCATION_INFO() ) override;
    virtual void AddSegment( SEG aS, const KIGFX::COLOR4D& aColor,
                             const std::string        aName,
                             const SRC_LOCATION_INFO& aSrcLoc = SRC_LOCATION_INFO() ) override;
    virtual void AddBox( BOX2I aB, const KIGFX::COLOR4D& aColor,
                         const std::string        aName,
                         const SRC_LOCATION_INFO& aSrcLoc = SRC_LOCATION_INFO() ) override;
*/


void PNS_TEST_DEBUG_DECORATOR::AddPoint( const VECTOR2I& aP, const KIGFX::COLOR4D& aColor,
                                         int aSize, const std::string& aName,
                                         const SRC_LOCATION_INFO& aSrcLoc )
{
    auto sh = new SHAPE_LINE_CHAIN;

    sh->Append( aP.x - aSize, aP.y - aSize );
    sh->Append( aP.x + aSize, aP.y + aSize );
    sh->Append( aP.x, aP.y );
    sh->Append( aP.x - aSize, aP.y + aSize );
    sh->Append( aP.x + aSize, aP.y - aSize );

    DEBUG_ENT* ent = new DEBUG_ENT();

    ent->m_shapes.push_back( sh );
    ent->m_color = aColor;
    ent->m_width = 30000;
    ent->m_iter = m_iter;
    ent->m_name = aName;
    ent->m_hasLabels = false;
    ent->m_srcLoc = aSrcLoc;

    addEntry( ent );
}


void PNS_TEST_DEBUG_DECORATOR::AddLine( const SHAPE_LINE_CHAIN& aLine, const KIGFX::COLOR4D& aColor,
                                        int aWidth, const std::string& aName,
                                        const SRC_LOCATION_INFO& aSrcLoc )
{
    auto       sh = new SHAPE_LINE_CHAIN( aLine );
    DEBUG_ENT* ent = new DEBUG_ENT();

    ent->m_shapes.push_back( sh );
    ent->m_color = aColor;
    ent->m_width = aWidth;
    ent->m_name = aName;
    ent->m_iter = m_iter;
    ent->m_srcLoc = aSrcLoc;

    addEntry( ent );
}


void PNS_TEST_DEBUG_DECORATOR::AddSegment( const SEG& aS, const KIGFX::COLOR4D& aColor,
                                           const std::string&       aName,
                                           const SRC_LOCATION_INFO& aSrcLoc )
{
    auto       sh = new SHAPE_LINE_CHAIN( { aS.A, aS.B } );
    DEBUG_ENT* ent = new DEBUG_ENT();

    ent->m_shapes.push_back( sh );
    ent->m_color = aColor;
    ent->m_width = 10000;
    ent->m_name = aName;
    ent->m_iter = m_iter;
    ent->m_srcLoc = aSrcLoc;

    addEntry( ent );
}


void PNS_TEST_DEBUG_DECORATOR::AddBox( const BOX2I& aB, const KIGFX::COLOR4D& aColor,
                                       const std::string& aName, const SRC_LOCATION_INFO& aSrcLoc )
{
    auto       sh = new SHAPE_RECT( aB.GetPosition(), aB.GetWidth(), aB.GetHeight() );
    DEBUG_ENT* ent = new DEBUG_ENT();

    ent->m_shapes.push_back( sh );
    ent->m_color = aColor;
    ent->m_width = 10000;
    ent->m_name = aName;
    ent->m_iter = m_iter;
    ent->m_srcLoc = aSrcLoc;
    addEntry( ent );
}


void PNS_TEST_DEBUG_DECORATOR::Message( const wxString& msg, const SRC_LOCATION_INFO& aSrcLoc )
{
    DEBUG_ENT* ent = new DEBUG_ENT();
    ent->m_msg = msg.c_str();
    ent->m_srcLoc = aSrcLoc;
    addEntry( ent );
}


void PNS_TEST_DEBUG_DECORATOR::NewStage( const std::string& name, int iter,
                                         const SRC_LOCATION_INFO& aSrcLoc )
{
    m_stages.push_back( new STAGE );
    m_activeEntry = m_stages.back()->m_entries;
}


void PNS_TEST_DEBUG_DECORATOR::DEBUG_ENT::IterateTree(
        std::function<bool( PNS_TEST_DEBUG_DECORATOR::DEBUG_ENT* )> visitor, int depth )
{
    if( !visitor( this ) )
        return;


    for( auto child : m_children )
    {
        child->IterateTree( visitor, depth + 1 );
    }
}


BOX2I PNS_TEST_DEBUG_DECORATOR::GetStageExtents( int stage ) const
{
    STAGE* st = m_stages[stage];
    BOX2I  bb;
    bool   first = true;

    auto visitor = [&]( DEBUG_ENT* ent ) -> bool {
        for( auto sh : ent->m_shapes )
        {
            if( first )
                bb = sh->BBox();
            else
                bb.Merge( sh->BBox() );

            first = false;
        }

        return true;
    };

    return bb;
}
