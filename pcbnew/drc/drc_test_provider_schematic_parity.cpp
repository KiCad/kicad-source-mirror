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

#include <board.h>
#include <pad.h>
#include <drc/drc_engine.h>
#include <drc/drc_item.h>
#include <drc/drc_rule.h>
#include <drc/drc_test_provider.h>

#include <kiway.h>
#include <netlist_reader/pcb_netlist.h>

/*
    Schematic parity test.

    Errors generated:
    - DRCE_MISSING_FOOTPRINT
    - DRCE_DUPLICATE_FOOTPRINT
    - DRCE_EXTRA_FOOTPRINT
    - DRCE_SCHEMATIC_PARITY
    - DRCE_FOOTPRINT_FILTERS

    TODO:
    - cross-check PCB netlist against SCH netlist
    - cross-check PCB fields against SCH fields
*/

class DRC_TEST_PROVIDER_SCHEMATIC_PARITY : public DRC_TEST_PROVIDER
{
public:
    DRC_TEST_PROVIDER_SCHEMATIC_PARITY()
    {
        m_isRuleDriven = false;
    }

    virtual ~DRC_TEST_PROVIDER_SCHEMATIC_PARITY() = default;

    virtual bool Run() override;

    virtual const wxString GetName() const override { return wxT( "schematic_parity" ); };

private:
    void testNetlist( NETLIST& aNetlist );
};


void DRC_TEST_PROVIDER_SCHEMATIC_PARITY::testNetlist( NETLIST& aNetlist )
{
    BOARD* board = m_drcEngine->GetBoard();

    auto compare = []( const FOOTPRINT* x, const FOOTPRINT* y )
                   {
                       return x->GetReference().CmpNoCase( y->GetReference() ) < 0;
                   };

    auto footprints = std::set<FOOTPRINT*, decltype( compare )>( compare );

    // Search for duplicate footprints on the board
    for( FOOTPRINT* footprint : board->Footprints() )
    {
        if( m_drcEngine->IsErrorLimitExceeded( DRCE_DUPLICATE_FOOTPRINT ) )
            break;

        auto ins = footprints.insert( footprint );

        if( !ins.second && !( footprint->GetAttributes() & FP_BOARD_ONLY ) )
        {
            std::shared_ptr<DRC_ITEM> drcItem = DRC_ITEM::Create( DRCE_DUPLICATE_FOOTPRINT );
            drcItem->SetItems( footprint, *ins.first );

            reportViolation( drcItem, footprint->GetPosition(), UNDEFINED_LAYER );
        }
    }

    // Search for component footprints in the netlist but not on the board.
    for( unsigned ii = 0; ii < aNetlist.GetCount(); ii++ )
    {
        COMPONENT* component = aNetlist.GetComponent( ii );
        FOOTPRINT* footprint = board->FindFootprintByReference( component->GetReference() );

        if( footprint == nullptr )
        {
            if( !m_drcEngine->IsErrorLimitExceeded( DRCE_MISSING_FOOTPRINT ) )
            {
                wxString msg;
                msg.Printf( _( "Missing footprint %s (%s)" ),
                            component->GetReference(),
                            component->GetValue() );

                std::shared_ptr<DRC_ITEM> drcItem = DRC_ITEM::Create( DRCE_MISSING_FOOTPRINT );

                drcItem->SetErrorMessage( msg );
                reportViolation( drcItem, VECTOR2I(), UNDEFINED_LAYER );
            }
        }
        else
        {
            if( component->GetValue() != footprint->GetValue()
                && !m_drcEngine->IsErrorLimitExceeded( DRCE_SCHEMATIC_PARITY ) )
            {
                wxString msg;
                msg.Printf( _( "Value (%s) doesn't match symbol value (%s)" ),
                            footprint->GetReference(), footprint->GetValue(),
                            component->GetValue() );

                std::shared_ptr<DRC_ITEM> drcItem = DRC_ITEM::Create( DRCE_SCHEMATIC_PARITY );
                drcItem->SetErrorMessage( msg );
                drcItem->SetItems( footprint );
                reportViolation( drcItem, footprint->GetPosition(), UNDEFINED_LAYER );
            }

            if( component->GetFPID().GetUniStringLibId() != footprint->GetFPID().GetUniStringLibId()
                && !m_drcEngine->IsErrorLimitExceeded( DRCE_SCHEMATIC_PARITY ) )
            {
                wxString msg;
                msg.Printf( _( "%s doesn't match footprint given by symbol (%s)" ),
                            footprint->GetFPID().GetUniStringLibId(),
                            component->GetFPID().GetUniStringLibId() );

                std::shared_ptr<DRC_ITEM> drcItem = DRC_ITEM::Create( DRCE_SCHEMATIC_PARITY );
                drcItem->SetErrorMessage( msg );
                drcItem->SetItems( footprint );
                reportViolation( drcItem, footprint->GetPosition(), UNDEFINED_LAYER );
            }

            if( !m_drcEngine->IsErrorLimitExceeded( DRCE_FOOTPRINT_FILTERS ) )
            {
                wxString libIdLower = footprint->GetFPID().GetUniStringLibId().Lower();
                wxString fpNameLower = footprint->GetFPID().GetUniStringLibItemName().Lower();
                size_t   filtercount = component->GetFootprintFilters().GetCount();
                bool     found = ( 0 == filtercount ); // if no entries, do not filter

                for( size_t jj = 0; jj < filtercount && !found; jj++ )
                {
                    wxString filterLower = component->GetFootprintFilters()[jj].Lower();

                    if( filterLower.Find( ':' ) == wxNOT_FOUND )
                        found = fpNameLower.Matches( filterLower );
                    else
                        found = libIdLower.Matches( filterLower );
                }

                if( !found )
                {
                    wxString msg;
                    msg.Printf( _( "%s doesn't match symbol's footprint filters (%s)" ),
                                footprint->GetFPID().GetUniStringLibId(),
                                wxJoin( component->GetFootprintFilters(), ' ' ) );

                    std::shared_ptr<DRC_ITEM> drcItem = DRC_ITEM::Create( DRCE_FOOTPRINT_FILTERS );
                    drcItem->SetErrorMessage( msg );
                    drcItem->SetItems( footprint );
                    reportViolation( drcItem, footprint->GetPosition(), UNDEFINED_LAYER );
                }
            }

            if( ( component->GetProperties().count( "dnp" ) > 0 )
                != ( ( footprint->GetAttributes() & FP_DNP ) > 0 )
                && !m_drcEngine->IsErrorLimitExceeded( DRCE_SCHEMATIC_PARITY ) )
            {
                wxString msg;
                msg.Printf( _( "'%s' settings differ" ), _( "Do not populate" ) );

                std::shared_ptr<DRC_ITEM> drcItem = DRC_ITEM::Create( DRCE_SCHEMATIC_PARITY );
                drcItem->SetErrorMessage( drcItem->GetErrorMessage( true ) + wxS( ": " ) + msg );
                drcItem->SetItems( footprint );
                reportViolation( drcItem, footprint->GetPosition(), UNDEFINED_LAYER );
            }

            if( ( component->GetProperties().count( "exclude_from_bom" ) > 0 )
                != ( (footprint->GetAttributes() & FP_EXCLUDE_FROM_BOM ) > 0 )
                && !m_drcEngine->IsErrorLimitExceeded( DRCE_SCHEMATIC_PARITY ) )
            {
                wxString msg;
                msg.Printf( _( "'%s' settings differ" ), _( "Exclude from bill of materials" ) );

                std::shared_ptr<DRC_ITEM> drcItem = DRC_ITEM::Create( DRCE_SCHEMATIC_PARITY );
                drcItem->SetErrorMessage( drcItem->GetErrorMessage( true ) + wxS( ": " ) + msg );
                drcItem->SetItems( footprint );
                reportViolation( drcItem, footprint->GetPosition(), UNDEFINED_LAYER );
            }

            // Compare custom fields between schematic component and PCB footprint
            if( !m_drcEngine->IsErrorLimitExceeded( DRCE_SCHEMATIC_FIELDS_PARITY ) )
            {
                std::unordered_map<wxString, wxString> fpFieldsAsMap;

                for( PCB_FIELD* field : footprint->GetFields() )
                {
                    wxCHECK2( field, continue );

                    if( field->IsReference() || field->IsValue() || field->IsComponentClass() )
                        continue;

                    fpFieldsAsMap[field->GetName()] = field->GetText();
                }

                // Remove the extra component fields we don't want to evaluate here
                nlohmann::ordered_map<wxString, wxString> compFields = component->GetFields();
                compFields.erase( GetCanonicalFieldName( FIELD_T::REFERENCE ) );
                compFields.erase( GetCanonicalFieldName( FIELD_T::VALUE ) );
                compFields.erase( GetCanonicalFieldName( FIELD_T::FOOTPRINT ) );
                compFields.erase( wxT( "Component Class" ) );

                bool fieldsMatch = true;
                wxString mismatchDetail;

                for( const auto& [name, value] : compFields )
                {
                    auto it = fpFieldsAsMap.find( name );

                    if( it == fpFieldsAsMap.end() )
                    {
                        fieldsMatch = false;
                        mismatchDetail = wxString::Format( _( "Missing symbol field '%s' in footprint" ), name );
                        break;
                    }

                    if( it->second != value )
                    {
                        fieldsMatch = false;
                        mismatchDetail = wxString::Format( _( "Field '%s' differs (PCB: '%s', Schematic: '%s')" ),
                                                           name, it->second, value );
                        break;
                    }
                }

                if( !fieldsMatch && !mismatchDetail.IsEmpty() )
                {
                    std::shared_ptr<DRC_ITEM> drcItem = DRC_ITEM::Create( DRCE_SCHEMATIC_FIELDS_PARITY );

                    drcItem->SetErrorMessage( mismatchDetail );
                    drcItem->SetItems( footprint );
                    reportViolation( drcItem, footprint->GetPosition(), UNDEFINED_LAYER );
                }
            }

            for( PAD* pad : footprint->Pads() )
            {
                if( m_drcEngine->IsErrorLimitExceeded( DRCE_NET_CONFLICT ) )
                    break;

                if( !pad->CanHaveNumber() )
                    continue;

                const COMPONENT_NET& sch_net = component->GetNet( pad->GetNumber() );
                const wxString&      pcb_netname = pad->GetNetname();

                if( !pcb_netname.IsEmpty() && sch_net.GetPinName().IsEmpty() )
                {
                    wxString msg;
                    msg.Printf( _( "No corresponding pin found in schematic" ) );

                    std::shared_ptr<DRC_ITEM> drcItem = DRC_ITEM::Create( DRCE_NET_CONFLICT );
                    drcItem->SetErrorMessage( msg );
                    drcItem->SetItems( pad );
                    reportViolation( drcItem, footprint->GetPosition(), UNDEFINED_LAYER );
                }
                else if( pcb_netname.IsEmpty() && !sch_net.GetNetName().IsEmpty() )
                {
                    wxString msg;
                    msg.Printf( _( "Pad missing net given by schematic (%s)" ),
                                sch_net.GetNetName() );

                    std::shared_ptr<DRC_ITEM> drcItem = DRC_ITEM::Create( DRCE_NET_CONFLICT );
                    drcItem->SetErrorMessage( msg );
                    drcItem->SetItems( pad );
                    reportViolation( drcItem, footprint->GetPosition(), UNDEFINED_LAYER );
                }
                else if( pcb_netname != sch_net.GetNetName()
                         && !( pcb_netname.starts_with(
                                 wxT( "unconnected-" ) )
                                 && pcb_netname.starts_with( sch_net.GetNetName() ) ) )
                {
                    wxString msg;
                    msg.Printf( _( "Pad net (%s) doesn't match net given by schematic (%s)" ),
                                pcb_netname,
                                sch_net.GetNetName() );

                    std::shared_ptr<DRC_ITEM> drcItem = DRC_ITEM::Create( DRCE_NET_CONFLICT );
                    drcItem->SetErrorMessage( msg );
                    drcItem->SetItems( pad );
                    reportViolation( drcItem, footprint->GetPosition(), UNDEFINED_LAYER );
                }
            }

            for( unsigned jj = 0; jj < component->GetNetCount(); ++jj )
            {
                if( m_drcEngine->IsErrorLimitExceeded( DRCE_NET_CONFLICT ) )
                    break;

                const COMPONENT_NET& sch_net = component->GetNet( jj );

                if( !footprint->FindPadByNumber( sch_net.GetPinName() ) )
                {
                    wxString msg;

                    if( sch_net.GetNetName().StartsWith( wxT( "unconnected-" ) ) )
                    {
                        msg = sch_net.GetPinName();
                    }
                    else
                    {
                        msg = wxString::Format( wxT( "%s (%s)" ),
                                                sch_net.GetPinName(),
                                                sch_net.GetNetName() );
                    }

                    msg.Printf( _( "No pad found for pin %s in schematic" ), msg );

                    std::shared_ptr<DRC_ITEM> drcItem = DRC_ITEM::Create( DRCE_NET_CONFLICT );
                    drcItem->SetErrorMessage( msg );
                    drcItem->SetItems( footprint );
                    reportViolation( drcItem, footprint->GetPosition(), UNDEFINED_LAYER );
                }
            }
        }
    }

    // Search for component footprints found on board but not in netlist.
    for( FOOTPRINT* footprint : board->Footprints() )
    {
        if( m_drcEngine->IsErrorLimitExceeded( DRCE_EXTRA_FOOTPRINT ) )
            break;

        if( footprint->GetAttributes() & FP_BOARD_ONLY )
            continue;

        if( !aNetlist.GetComponentByReference( footprint->GetReference() ) )
        {
            std::shared_ptr<DRC_ITEM> drcItem = DRC_ITEM::Create( DRCE_EXTRA_FOOTPRINT );

            drcItem->SetItems( footprint );
            reportViolation( drcItem, footprint->GetPosition(), UNDEFINED_LAYER );
        }
    }
}


bool DRC_TEST_PROVIDER_SCHEMATIC_PARITY::Run()
{
    if( m_drcEngine->GetTestFootprints() )
    {
        if( !reportPhase( _( "Checking PCB to schematic parity..." ) ) )
            return false;

        auto netlist = m_drcEngine->GetSchematicNetlist();

        if( !netlist )
        {
            REPORT_AUX( wxT( "No netlist provided, skipping schematic parity tests." ) );
            return true;
        }

        testNetlist( *netlist );
    }

    return !m_drcEngine->IsCancelled();
}


namespace detail
{
static DRC_REGISTER_TEST_PROVIDER<DRC_TEST_PROVIDER_SCHEMATIC_PARITY> dummy;
}
