/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2004-2020 KiCad Developers.
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

#include <cstdio>
#include <memory>
#include <reporter.h>
#include <board.h>
#include <kicad_string.h>

#include <pcb_expr_evaluator.h>

#include <connectivity/connectivity_data.h>
#include <connectivity/connectivity_algo.h>

#include <connectivity/from_to_cache.h>

void FROM_TO_CACHE::buildEndpointList( )
{
    m_ftEndpoints.clear();

    for( FOOTPRINT* footprint : m_board->Footprints() )
    {
        for( PAD* pad : footprint->Pads() )
        {
            FT_ENDPOINT ent;
            ent.name = footprint->GetReference() + "-" + pad->GetName();
            ent.parent = pad;
            m_ftEndpoints.push_back( ent );
            ent.name = footprint->GetReference();
            ent.parent = pad;
            m_ftEndpoints.push_back( ent );
        }
    }
}


enum PATH_STATUS {
    PS_OK = 0,
    PS_MULTIPLE_PATHS = -1,
    PS_NO_PATH = -2
};


static bool isVertexVisited( CN_ITEM* v, const std::vector<CN_ITEM*>& path )
{
    for( auto u : path )
    {
        if ( u == v )
            return true;
    }

    return false;
}


static PATH_STATUS uniquePathBetweenNodes( CN_ITEM* u, CN_ITEM* v, std::vector<CN_ITEM*>& outPath )
{
    using Path = std::vector<CN_ITEM*>;
    std::deque<Path> Q;

    Path pInit;
    bool pathFound = false;
    pInit.push_back( u );
    Q.push_back( pInit );

    while( Q.size() )
    {
        Path path = Q.front();
        Q.pop_front();
        CN_ITEM* last = path.back();

        if( last == v )
        {
            outPath = path;
            if( pathFound )
                return PS_MULTIPLE_PATHS;
            pathFound = true;
        }

        for( auto ci : last->ConnectedItems() )
        {
            bool vertexVisited = isVertexVisited( ci, path );

            for( auto& p : Q )
                if( isVertexVisited( ci, p ) )
                {
                    vertexVisited = true;
                    break;
                }

            if( !vertexVisited )
            {
                Path newpath( path );
                newpath.push_back( ci );
                Q.push_back( newpath );
            }
        }
    }

    return pathFound ? PS_OK : PS_NO_PATH;
};


int FROM_TO_CACHE::cacheFromToPaths( const wxString& aFrom, const wxString& aTo )
{
    std::vector<FT_PATH> paths;
    auto connectivity = m_board->GetConnectivity();
    auto cnAlgo = connectivity->GetConnectivityAlgo();

    for( auto& endpoint : m_ftEndpoints )
    {
        if( WildCompareString( aFrom, endpoint.name, false ) )
        {
            FT_PATH p;
            p.net = endpoint.parent->GetNetCode();
            p.from = endpoint.parent;
            p.to = nullptr;
            paths.push_back(p);
        }
    }

    for( auto &path : paths )
    {
        int count = 0;
        auto netName = path.from->GetNetname();

        wxString fromName = path.from->GetParent()->GetReference() + "-" + path.from->GetName();

        const KICAD_T onlyRouting[] = { PCB_PAD_T, PCB_ARC_T, PCB_VIA_T, PCB_TRACE_T, EOT };

        auto padCandidates = connectivity->GetConnectedItems( path.from, onlyRouting );
        PAD* toPad = nullptr;

        for( auto pitem : padCandidates )
        {
            if( pitem == path.from )
                continue;

            if( pitem->Type() != PCB_PAD_T )
                continue;

            const PAD *pad = static_cast<const PAD*>( pitem );

            wxString toName = pad->GetParent()->GetReference() + "-" + pad->GetName();


            for ( const auto& endpoint : m_ftEndpoints )
            {
                if( pad == endpoint.parent )
                {
                        if( WildCompareString( aTo, endpoint.name, false ) )
                        {
                            count++;
                            toPad = endpoint.parent;

                            path.to = toPad;
                            path.fromName = fromName;
                            path.toName = toName;
                            path.fromWildcard = aFrom;
                            path.toWildcard = aTo;

                            if( count >= 2 )
                            {
                                // fixme: report this somewhere?
                                //printf("Multiple targets found, aborting...\n");
                                path.to = nullptr;
                            }
                        }
                    }
                }
        }
    }

    int newPaths = 0;

    for( auto &path : paths )
    {
        if( !path.from || !path.to )
            continue;


        CN_ITEM *cnFrom = cnAlgo->ItemEntry( path.from ).GetItems().front();
        CN_ITEM *cnTo = cnAlgo->ItemEntry( path.to ).GetItems().front();
        CN_ITEM::CONNECTED_ITEMS upath;

        auto result = uniquePathBetweenNodes( cnFrom, cnTo, upath );

        if( result == PS_OK )
            path.isUnique = true;
        else
            path.isUnique = false;


        //printf( "%s\n", (const char *) wxString::Format( _("Check path: %s -> %s (net %s)"), path.fromName, path.toName, cnFrom->Parent()->GetNetname() ) );

        if( result == PS_NO_PATH )
            continue;

        for( const auto item : upath )
        {
            path.pathItems.insert( item->Parent() );
        }

        m_ftPaths.push_back(path);
        newPaths++;
    }

    // reportAux( _("Cached %d paths\n"), newPaths );

    return newPaths;
}

bool  FROM_TO_CACHE::IsOnFromToPath( BOARD_CONNECTED_ITEM* aItem, const wxString& aFrom, const wxString& aTo )
{
    int nFromTosFound = 0;

    if( !m_board )
        return false;

    //printf("Check %d cached paths [%p]\n", m_ftPaths.size(), aItem );
    for( int attempt = 0; attempt < 2; attempt++ )
    {
        // item already belongs to path
        for( auto& ftPath : m_ftPaths )
        {
            if( aFrom == ftPath.fromWildcard &&
                    aTo == ftPath.toWildcard )
                {
                    nFromTosFound++;

                    if( ftPath.pathItems.count( aItem ) )
                    {
            //            printf("Found cached path for %p [%s->%s]\n", aItem, (const char *)ftPath.fromName, (const char *) ftPath.toName );
                        return true;
                    }
                }
        }

        if( !nFromTosFound )
            cacheFromToPaths( aFrom, aTo );
        else
            return false;
    }

    return false;
}


void FROM_TO_CACHE::Rebuild( BOARD* aBoard )
{
    m_board = aBoard;
    buildEndpointList();
    m_ftPaths.clear();
}


FROM_TO_CACHE::FT_PATH* FROM_TO_CACHE::QueryFromToPath( const std::set<BOARD_CONNECTED_ITEM*>& aItems )
{
    for( auto& ftPath : m_ftPaths )
    {
        if ( ftPath.pathItems == aItems )
            return &ftPath;
    }

    return nullptr;
}