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

#include <class_board.h>
#include <drc/drc_engine.h>
#include <drc/drc.h>
#include <drc/drc_item.h>
#include <drc/drc_rule.h>
#include <drc/drc_test_provider.h>

#include <kiway.h>
#include <netlist_reader/pcb_netlist.h>

/*
    Layout-versus-schematic (LVS) test.

    Errors generated:
    - DRCE_MISSING_FOOTPRINT
    - DRCE_DUPLICATE_FOOTPRINT
    - DRCE_EXTRA_FOOTPRINT

    TODO:
    - cross-check PCB netlist against SCH netlist
    - cross-check PCB fields against SCH fields
*/

class DRC_TEST_PROVIDER_LVS : public DRC_TEST_PROVIDER
{
public:
    DRC_TEST_PROVIDER_LVS()
    {
        m_isRuleDriven = false;
    }

    virtual ~DRC_TEST_PROVIDER_LVS()
    {
    }

    virtual bool Run() override;

    virtual const wxString GetName() const override
    {
        return "LVS";
    };

    virtual const wxString GetDescription() const override
    {
        return "Performs layout-vs-schematics integity check";
    }

    virtual std::set<DRC_CONSTRAINT_TYPE_T> GetMatchingConstraintIds() const override;

private:

    bool fetchNetlistFromSchematic( NETLIST& aNetlist );
    void testFootprints( NETLIST& aNetlist );
};


void DRC_TEST_PROVIDER_LVS::testFootprints( NETLIST& aNetlist )
{
    BOARD* board = m_drcEngine->GetBoard();

    auto compare = []( const MODULE* x, const MODULE* y )
                   {
                       return x->GetReference().CmpNoCase( y->GetReference() ) < 0;
                   };

    auto mods = std::set<MODULE*, decltype( compare )>( compare );

    // Search for duplicate footprints on the board
    for( MODULE* mod : board->Modules() )
    {
        if( m_drcEngine->IsErrorLimitExceeded( DRCE_DUPLICATE_FOOTPRINT ) )
            break;

        auto ins = mods.insert( mod );

        if( !ins.second )
        {
            std::shared_ptr<DRC_ITEM> drcItem = DRC_ITEM::Create( DRCE_DUPLICATE_FOOTPRINT );
            drcItem->SetItems( mod, *ins.first );

            ReportWithMarker( drcItem, mod->GetPosition() );
        }
    }

    // Search for component footprints in the netlist but not on the board.
    for( unsigned ii = 0; ii < aNetlist.GetCount(); ii++ )
    {
        COMPONENT* component = aNetlist.GetComponent( ii );
        MODULE*    module    = board->FindModuleByReference( component->GetReference() );

        if( module == nullptr )
        {
            if( m_drcEngine->IsErrorLimitExceeded( DRCE_MISSING_FOOTPRINT ) )
                break;

            m_msg.Printf( _( "Missing footprint %s (%s)" ),
                          component->GetReference(),
                          component->GetValue() );

            std::shared_ptr<DRC_ITEM> drcItem = DRC_ITEM::Create( DRCE_MISSING_FOOTPRINT );

            drcItem->SetErrorMessage( m_msg );
            Report( drcItem );
        }
        else
        {
            for( D_PAD* pad : module->Pads() )
            {
                if( m_drcEngine->IsErrorLimitExceeded( DRCE_NET_CONFLICT ) )
                    break;

                const COMPONENT_NET& sch_net = component->GetNet( pad->GetName() );
                const wxString&      pcb_netname = pad->GetNetname();

                if( !pcb_netname.IsEmpty() && sch_net.GetPinName().IsEmpty() )
                {
                    m_msg.Printf( _( "No corresponding pin found in schematic." ) );

                    std::shared_ptr<DRC_ITEM> drcItem = DRC_ITEM::Create( DRCE_NET_CONFLICT );
                    drcItem->SetErrorMessage( m_msg );
                    drcItem->SetItems( pad );
                    ReportWithMarker( drcItem, module->GetPosition() );
                }
                else if( pcb_netname.IsEmpty() && !sch_net.GetNetName().IsEmpty() )
                {
                    m_msg.Printf( _( "Pad missing net given by schematic (%s)." ),
                                  sch_net.GetNetName() );

                    std::shared_ptr<DRC_ITEM> drcItem = DRC_ITEM::Create( DRCE_NET_CONFLICT );
                    drcItem->SetErrorMessage( m_msg );
                    drcItem->SetItems( pad );
                    ReportWithMarker( drcItem, module->GetPosition() );
                }
                else if( pcb_netname != sch_net.GetNetName() )
                {
                    m_msg.Printf( _( "Pad net (%s) doesn't match net given by schematic (%s)." ),
                                  pcb_netname,
                                  sch_net.GetNetName() );

                    std::shared_ptr<DRC_ITEM> drcItem = DRC_ITEM::Create( DRCE_NET_CONFLICT );
                    drcItem->SetErrorMessage( m_msg );
                    drcItem->SetItems( pad );
                    ReportWithMarker( drcItem, module->GetPosition() );
                }
            }

            for( unsigned jj = 0; jj < component->GetNetCount(); ++jj )
            {
                if( m_drcEngine->IsErrorLimitExceeded( DRCE_NET_CONFLICT ) )
                    break;

                const COMPONENT_NET& sch_net = component->GetNet( jj );

                if( !module->FindPadByName( sch_net.GetPinName() ) )
                {
                    m_msg.Printf( _( "No pad found for pin %s in schematic." ),
                                  sch_net.GetPinName() );

                    std::shared_ptr<DRC_ITEM> drcItem = DRC_ITEM::Create( DRCE_NET_CONFLICT );
                    drcItem->SetErrorMessage( m_msg );
                    drcItem->SetItems( module );
                    ReportWithMarker( drcItem, module->GetPosition() );
                }
            }
        }
    }

    // Search for component footprints found on board but not in netlist.
    for( MODULE* module : board->Modules() )
    {
        if( m_drcEngine->IsErrorLimitExceeded( DRCE_EXTRA_FOOTPRINT ) )
            break;

        if( !aNetlist.GetComponentByReference( module->GetReference() ) )
        {
            std::shared_ptr<DRC_ITEM> drcItem = DRC_ITEM::Create( DRCE_EXTRA_FOOTPRINT );

            drcItem->SetItems( module );
            ReportWithMarker( drcItem, module->GetPosition() );
        }
    }
}


bool DRC_TEST_PROVIDER_LVS::fetchNetlistFromSchematic( NETLIST& aNetlist )
{
    // fixme: make it work without dependency on EDIT_FRAME/kiway
#if 0
    std::string payload;

    Kiway().ExpressMail( FRAME_SCH, MAIL_SCH_GET_NETLIST, payload, nullptr );

    try
    {
        auto lineReader = new STRING_LINE_READER( payload, _( "Eeschema netlist" ) );
        KICAD_NETLIST_READER netlistReader( lineReader, &aNetlist );
        netlistReader.LoadNetlist();
    }
    catch( const IO_ERROR& )
    {
        assert( false ); // should never happen
        return false;
    }

#endif

    return false;
}

bool DRC_TEST_PROVIDER_LVS::Run()
{
    ReportStage( _( "Layout-vs-Schematic checks..." ), 0, 2 );

#if 0
       
    if ( !Kiface().IsSingle() )
    {
        NETLIST netlist; // fixme: fetch from schematic without referring directly to the FRAME

        if( ! fetchNetlistFromSchematic( netlist ) )
        {
            ReportAux( _( "Unable to fetch the schematic netlist. Skipping LVS checks. ") );
            return true;
        }
    
        testFootprints( netlist );
    }
#endif

    reportRuleStatistics();

    return true;
}


std::set<DRC_CONSTRAINT_TYPE_T> DRC_TEST_PROVIDER_LVS::GetMatchingConstraintIds() const
{
    return {};
}


namespace detail
{
static DRC_REGISTER_TEST_PROVIDER<DRC_TEST_PROVIDER_LVS> dummy;
}
