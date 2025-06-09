/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers.
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
#include <string_utils.h>

#include <pcbexpr_evaluator.h>

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
            ent.name = footprint->GetReference() + wxT( "-" ) + pad->GetNumber();
            ent.parent = pad;
            m_ftEndpoints.push_back( ent );
            ent.name = footprint->GetReference();
            ent.parent = pad;
            m_ftEndpoints.push_back( std::move( ent ) );
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
    for( CN_ITEM* u : path )
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
    Q.push_back( std::move( pInit ) );

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

        for( CN_ITEM* ci : last->ConnectedItems() )
        {
            bool vertexVisited = isVertexVisited( ci, path );

            for( std::vector<CN_ITEM*>& p : Q )
            {
                if( isVertexVisited( ci, p ) )
                {
                    vertexVisited = true;
                    break;
                }
            }

            if( !vertexVisited )
            {
                Path newpath( path );
                newpath.push_back( ci );
                Q.push_back( std::move( newpath ) );
            }
        }
    }

    return pathFound ? PS_OK : PS_NO_PATH;
};


int FROM_TO_CACHE::cacheFromToPaths( const wxString& aFrom, const wxString& aTo )
{
    std::vector<FT_PATH>                  paths;
    std::shared_ptr<CONNECTIVITY_DATA>    connectivity = m_board->GetConnectivity();
    std::shared_ptr<CN_CONNECTIVITY_ALGO> cnAlgo = connectivity->GetConnectivityAlgo();

    for( FT_ENDPOINT& endpoint : m_ftEndpoints )
    {
        if( WildCompareString( aFrom, endpoint.name, false ) )
        {
            FT_PATH p;
            p.net = endpoint.parent->GetNetCode();
            p.from = endpoint.parent;
            p.to = nullptr;
            paths.push_back( std::move( p ) );
        }
    }

    for( FT_PATH& path : paths )
    {
        int count = 0;

        wxString fromName = path.from->GetParentFootprint()->GetReference()
                                + wxT( "-" ) + path.from->GetNumber();

        auto padCandidates = connectivity->GetConnectedItems( path.from, EXCLUDE_ZONES );
        PAD* toPad = nullptr;

        for( BOARD_CONNECTED_ITEM* pitem : padCandidates )
        {
            if( pitem == path.from )
                continue;

            if( pitem->Type() != PCB_PAD_T )
                continue;

            const PAD *pad = static_cast<const PAD*>( pitem );

            wxString toName = pad->GetParentFootprint()->GetReference()
                                    + wxT( "-" ) + pad->GetNumber();

            for( const FT_ENDPOINT& endpoint : m_ftEndpoints )
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
                            path.to = nullptr;
                        }
                    }
                }
            }
        }
    }

    int newPaths = 0;

    for( FT_PATH& path : paths )
    {
        if( !path.from || !path.to )
            continue;

        CN_ITEM*              cnFrom = cnAlgo->ItemEntry( path.from ).GetItems().front();
        CN_ITEM*              cnTo = cnAlgo->ItemEntry( path.to ).GetItems().front();
        std::vector<CN_ITEM*> upath;

        auto result = uniquePathBetweenNodes( cnFrom, cnTo, upath );

        if( result == PS_OK )
            path.isUnique = true;
        else
            path.isUnique = false;

        if( result == PS_NO_PATH )
            continue;

        for( const CN_ITEM* item : upath )
            path.pathItems.insert( item->Parent() );

        m_ftPaths.push_back( path );
        newPaths++;
    }

    return newPaths;
}

bool  FROM_TO_CACHE::IsOnFromToPath( BOARD_CONNECTED_ITEM* aItem, const wxString& aFrom, const wxString& aTo )
{
    int nFromTosFound = 0;

    if( !m_board )
        return false;

    for( int attempt = 0; attempt < 2; attempt++ )
    {
        // item already belongs to path
        for( FT_PATH& ftPath : m_ftPaths )
        {
            if( aFrom == ftPath.fromWildcard && aTo == ftPath.toWildcard )
            {
                nFromTosFound++;

                if( ftPath.pathItems.count( aItem ) )
                    return true;
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
    for( FT_PATH& ftPath : m_ftPaths )
    {
        if ( ftPath.pathItems == aItems )
            return &ftPath;
    }

    return nullptr;
}
