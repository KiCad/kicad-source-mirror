/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2004-2020 KiCad Developers.
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

#include <common.h>
#include <class_board.h>
#include <class_drawsegment.h>
#include <class_pad.h>

#include <drc/drc_engine.h>
#include <drc/drc_item.h>
#include <drc/drc_rule.h>
#include <drc/drc_test_provider.h>

#include <connectivity/connectivity_data.h>
#include <connectivity/connectivity_algo.h>

#include <pcb_expr_evaluator.h>

/*
    Single-ended matched length test.
    Errors generated:
    - DRCE_SIGNAL_LENGTH

*/

namespace test {

class DRC_TEST_PROVIDER_MATCHED_LENGTH : public DRC_TEST_PROVIDER
{
public:
    DRC_TEST_PROVIDER_MATCHED_LENGTH ()
    {
    }

    virtual ~DRC_TEST_PROVIDER_MATCHED_LENGTH() 
    {
    }

    virtual bool Run() override;

    virtual const wxString GetName() const override 
    {
        return "length";
    };

    virtual const wxString GetDescription() const override
    {
        return "Tests matched track lengths.";
    }

    virtual int GetNumPhases() const override
    {
        return 1;
    }

    virtual std::set<DRC_CONSTRAINT_TYPE_T> GetConstraintTypes() const override;

    bool checkFromToPath( BOARD_CONNECTED_ITEM* aItem, const wxString& aFrom, const wxString& aTo );

private:

    struct FROM_TO_ENDPOINT
    {
        wxString name;
        D_PAD* parent;
    };

    struct FROM_TO_PATH
    {
        int net;
        D_PAD *from;
        D_PAD *to;
        wxString fromName, toName;
        wxString fromWildcard, toWildcard;
        bool isUnique;
        std::set<BOARD_CONNECTED_ITEM*> pathItems;
    };

    int cacheFromToPaths( const wxString& aFrom, const wxString& aTo );
    void buildEndpointList();

    std::vector<FROM_TO_ENDPOINT> m_ftEndpoints;
    std::vector<FROM_TO_PATH> m_ftPaths;

    BOARD* m_board;
    int m_largestClearance;
};

};


bool exprFromTo( LIBEVAL::CONTEXT* aCtx, void* self )
{
    PCB_EXPR_VAR_REF* vref = static_cast<PCB_EXPR_VAR_REF*>( self );
    BOARD_ITEM*       item = vref ? vref->GetObject( aCtx ) : nullptr;
    LIBEVAL::VALUE*   result = aCtx->AllocValue();

    LIBEVAL::VALUE*   argTo = aCtx->Pop();
    LIBEVAL::VALUE*   argFrom = aCtx->Pop();

    result->Set(0.0);
    aCtx->Push( result );

    if(!item)
        return false;

    auto drcEngine = item->GetBoard()->GetDesignSettings().m_DRCEngine;

    assert ( drcEngine );

    auto lengthTestProvider = static_cast<test::DRC_TEST_PROVIDER_MATCHED_LENGTH*>( drcEngine->GetTestProvider("length") );

    if( lengthTestProvider->checkFromToPath( static_cast<BOARD_CONNECTED_ITEM*>( item ),
    argFrom->AsString(),
    argTo->AsString() ) )
    {
        result->Set(1.0);
    }

    return true;
}

void test::DRC_TEST_PROVIDER_MATCHED_LENGTH::buildEndpointList( )
{
    m_ftEndpoints.clear();

    for( auto mod : m_board->Modules() )
    {
        for( auto pad : mod->Pads() )
        {
            FROM_TO_ENDPOINT ent;
            ent.name = mod->GetReference() + "-" + pad->GetName();
            ent.parent = pad;
            m_ftEndpoints.push_back( ent );
            ent.name = mod->GetReference();
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

    Path p;
    int pathFound = false;
    p.push_back( u );
    Q.push_back( p );

    while( Q.size() )
    {
        Path path = Q.front();
        Q.pop_front();
        CN_ITEM *last = path.back();

        if( last == v )
        {
            outPath = path;
            if( pathFound )
                return PS_MULTIPLE_PATHS;
            pathFound = true;
        }

        for( auto ci : last->ConnectedItems() )
        {
            bool vertexVisited = isVertexVisited(ci, path);

            for( auto &p : Q )
                if (isVertexVisited(ci, p))
                {
                    vertexVisited = true;
                    break;
                }

         if (!vertexVisited) {
            Path newpath(path);
            newpath.push_back(ci);
            Q.push_back(newpath);
         }
      }
    }

    return pathFound ? PS_OK : PS_NO_PATH;
};


int test::DRC_TEST_PROVIDER_MATCHED_LENGTH::cacheFromToPaths( const wxString& aFrom, const wxString& aTo )
{
    std::vector<FROM_TO_PATH> paths;
    auto connectivity = m_board->GetConnectivity();
    auto cnAlgo = connectivity->GetConnectivityAlgo();

    for( auto& endpoint : m_ftEndpoints )
    {
        if( WildCompareString( aFrom, endpoint.name, false ) )
        {
            FROM_TO_PATH p;
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
        D_PAD* toPad = nullptr;

        for( auto pitem : padCandidates )
        {
            if( pitem == path.from )
                continue;

            if( pitem->Type() != PCB_PAD_T )
                continue;

            D_PAD *pad = static_cast<D_PAD*>( pitem );

            wxString toName = pad->GetParent()->GetReference() + "-" + pad->GetName();


            for ( auto& endpoint : m_ftEndpoints )
            {
                if( pad == endpoint.parent )
                {
                        if( WildCompareString( aTo, endpoint.name, false ) )
                        {
                            //printf("match from-to: %s -> %s [net %s] p %p\n", (const char *)fromName, (const char *) toName, (const char *) netName, endpoint.parent );
                            count++;
                            toPad = endpoint.parent;

                            path.to = toPad;
                            path.fromName = fromName;
                            path.toName = toName;
                            path.fromWildcard = aFrom;
                            path.toWildcard = aTo;

                            if( count >= 2 )
                            {
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


        reportAux( wxString::Format( _("Check path: %s -> %s (net %s)"), path.fromName, path.toName, cnFrom->Parent()->GetNetname() ) );

        if( result == PS_NO_PATH )
            continue;

        for( auto item : upath )
        {
            path.pathItems.insert( item->Parent() );
        }

        m_ftPaths.push_back(path);
        newPaths++;
    }

    reportAux( _("Cached %d paths\n"), newPaths );

    return newPaths;
}

bool test::DRC_TEST_PROVIDER_MATCHED_LENGTH::checkFromToPath( BOARD_CONNECTED_ITEM* aItem, const wxString& aFrom, const wxString& aTo )
{
    int nFromTosFound = 0;
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


bool test::DRC_TEST_PROVIDER_MATCHED_LENGTH::Run()
{
    m_board = m_drcEngine->GetBoard();

    reportPhase(( "Gathering length-constrained connections..." ));

    typedef std::vector<BOARD_CONNECTED_ITEM*> CITEMS;
    std::map<DRC_RULE*, CITEMS> itemSets;

     auto evaluateLengthConstraints =
            [&]( BOARD_ITEM *item ) -> bool
            {
                auto constraint = m_drcEngine->EvalRulesForItems( DRC_CONSTRAINT_TYPE_LENGTH, item );

                if( constraint.IsNull() )
                    return true;

                auto citem = static_cast<BOARD_CONNECTED_ITEM*>( item );

                itemSets[ constraint.GetParentRule() ].push_back( citem );

                return true;
            };

    buildEndpointList( );

    forEachGeometryItem( { PCB_TRACE_T, PCB_VIA_T, PCB_ARC_T },
                    LSET::AllCuMask(), evaluateLengthConstraints );


    struct LENGTH_ENTRY {
        int netcode;
        wxString netname;
        CITEMS items;
        int viaCount;
        int totalRoute;
        int totalVia;
        int totalPadToDie;
        int total;
    };

    typedef std::vector<LENGTH_ENTRY> LENGTH_ENTRIES;
    std::map<DRC_RULE*, LENGTH_ENTRIES> matches;

    for( auto it : itemSets )
    {
        std::map<int, CITEMS> netMap;

        for( auto citem : it.second )
            netMap[ citem->GetNetCode() ].push_back( citem );


        for( auto nitem : netMap )
        {
            LENGTH_ENTRY ent;
            ent.items = nitem.second;
            ent.netcode = nitem.first;
            ent.netname = m_board->GetNetInfo().GetNetItem( ent.netcode )->GetNetname();

            ent.viaCount = 0;
            ent.totalRoute = 0;
            ent.totalVia = 0;
            ent.totalPadToDie = 0;
        
            for( auto citem : nitem.second )
            {
                if ( auto bi = dyn_cast<VIA*>( citem ) )
                {
                    ent.viaCount++;
                    ent.totalVia += 0; // fixme: via thru distance
                }
                else if ( auto bi = dyn_cast<TRACK*>(citem ))
                {
                    ent.totalRoute += bi->GetLength();
                }
            }

            ent.total = ent.totalRoute + ent.totalVia + ent.totalPadToDie;

            matches[ it.first ].push_back(ent);
        }
    }

    for( auto it : matches )
    {
        DRC_RULE *rule = it.first;
        auto& matchedConnections = it.second;

        std::sort( matchedConnections.begin(), matchedConnections.end(), 
            [] ( const LENGTH_ENTRY&a, const LENGTH_ENTRY&b ) -> int
            {
                return a.netname < b.netname;
            }
        );

        reportAux( wxString::Format( _("Length-constrained traces for rule '%s':"), it.first->m_Name ) );

        for( const auto& ent : matchedConnections )
        {
            reportAux(wxString::Format(
                " - net %s: %d matching items, total: %s (tracks: %s, vias: %s, pad-to-die: %s), vias: %d",
                 ent.netname,
                 (int) ent.items.size(),
                 MessageTextFromValue( userUnits(), ent.total, true ),
                 MessageTextFromValue( userUnits(), ent.totalRoute, true ),
                 MessageTextFromValue( userUnits(), ent.totalVia, true ),
                 MessageTextFromValue( userUnits(), ent.totalPadToDie, true ),
                 ent.viaCount
                ) );
        }


        OPT<DRC_CONSTRAINT> lengthConstraint = rule->FindConstraint( DRC_CONSTRAINT_TYPE_LENGTH );

        if( lengthConstraint )
        {
            for( const auto& ent : matchedConnections )
            {
                bool minViolation = false;
                bool maxViolation = false;
                int minLen, maxLen;
                
                if( lengthConstraint->GetValue().HasMin() && ent.total < lengthConstraint->GetValue().Min() )
                {
                    //printf("Min violation %d %d\n", ent.total, lengthConstraint->GetValue().Min() );
                    minViolation = true;
                    minLen = lengthConstraint->GetValue().Min();
                }
                else if( lengthConstraint->GetValue().HasMax() && ent.total > lengthConstraint->GetValue().Max() )
                {
                    //printf("Max violation %d %d\n", ent.total, lengthConstraint->GetValue().Min() );
                    maxViolation = true;
                    maxLen = lengthConstraint->GetValue().Max();
                }

                if( (minViolation || maxViolation) )
                {
                    std::shared_ptr<DRC_ITEM> drcItem = DRC_ITEM::Create( DRCE_LENGTH_OUT_OF_RANGE );
                    wxString msg = drcItem->GetErrorText() + " (" + lengthConstraint->GetParentRule()->m_Name + " ";

                    if( minViolation )
                        msg += wxString::Format( _("minimum length: %s; actual: %s)" ),
                        MessageTextFromValue( userUnits(), minLen, true ),
                        MessageTextFromValue( userUnits(), ent.total, true ) );
                    else if( maxViolation )
                        msg += wxString::Format( _("maximum length: %s; actual: %s)" ),
                        MessageTextFromValue( userUnits(), maxLen, true ),
                        MessageTextFromValue( userUnits(), ent.total, true ) );

                        drcItem->SetErrorMessage( msg );
                        // drcItem->SetItems( aRefItem->parent, aTestItem->parent );
                        drcItem->SetViolatingRule( lengthConstraint->GetParentRule() );

                    reportViolation( drcItem, wxPoint(0, 0) ); //aRefItem->parent->GetPosition() );
                }
            }
        }
    }

    reportRuleStatistics();

    return true;
}


std::set<DRC_CONSTRAINT_TYPE_T> test::DRC_TEST_PROVIDER_MATCHED_LENGTH::GetConstraintTypes() const
{
    return { DRC_CONSTRAINT_TYPE_LENGTH, DRC_CONSTRAINT_TYPE_SKEW };
}


namespace detail
{
    static DRC_REGISTER_TEST_PROVIDER<test::DRC_TEST_PROVIDER_MATCHED_LENGTH> dummy;
}