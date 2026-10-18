/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "import_net_reconciler.h"

#include <board.h>
#include <footprint.h>
#include <import_net_names.h>
#include <netinfo.h>
#include <netlist_reader/pcb_netlist.h>
#include <pad.h>
#include <pcb_field.h>
#include <board_design_settings.h>
#include <reporter.h>
#include <template_fieldnames.h>

#include <map>
#include <set>
#include <vector>


namespace
{
using PIN_SET = std::set<std::pair<wxString, wxString>>;

constexpr int MAX_REPORTED = 5;


// a reference used twice, or not yet assigned, names no single footprint
std::map<wxString, FOOTPRINT*> uniqueFootprints( BOARD& aBoard )
{
    std::map<wxString, FOOTPRINT*> footprints;
    std::set<wxString>             repeated;

    for( FOOTPRINT* footprint : aBoard.Footprints() )
    {
        if( footprint->GetReference().EndsWith( wxS( "?" ) ) )
            continue;

        if( !footprints.emplace( footprint->GetReference(), footprint ).second )
            repeated.insert( footprint->GetReference() );
    }

    for( const wxString& ref : repeated )
        footprints.erase( ref );

    return footprints;
}
} // namespace


int ReconcileImportedReferences( BOARD& aBoard, NETLIST& aNetlist )
{
    std::set<wxString> boardRefs;

    for( FOOTPRINT* footprint : aBoard.Footprints() )
        boardRefs.insert( footprint->GetReference() );

    std::set<wxString> schematicRefs;

    // the schematic references no footprint carries, by upper case, with a count to spot ambiguity
    std::map<wxString, std::pair<wxString, int>> orphans;

    for( unsigned i = 0; i < aNetlist.GetCount(); i++ )
    {
        const wxString ref = aNetlist.GetComponent( i )->GetReference();

        schematicRefs.insert( ref );

        if( boardRefs.count( ref ) )
            continue;

        auto& orphan = orphans[ref.Upper()];
        orphan.first = ref;
        orphan.second++;
    }

    // Allegro upper cases a reference and the importer prefixes one that starts with a digit
    std::map<wxString, std::vector<FOOTPRINT*>> claims;

    for( FOOTPRINT* footprint : aBoard.Footprints() )
    {
        const wxString ref = footprint->GetReference();

        if( ref.EndsWith( wxS( "?" ) ) || schematicRefs.count( ref ) )
            continue;

        wxString key = ref.Upper();

        // the importer prefixes only a reference that starts with a non-letter
        const bool prefixed = key.StartsWith( wxS( "UNK" ) ) && key.length() > 3 && !wxIsalpha( key[3] );

        if( prefixed && orphans.count( key ) && orphans.count( key.Mid( 3 ) ) )
            continue;

        if( prefixed && !orphans.count( key ) )
            key = key.Mid( 3 );

        if( orphans.count( key ) )
            claims[key].push_back( footprint );
    }

    int renamed = 0;

    for( const auto& [key, footprints] : claims )
    {
        const auto& orphan = orphans.at( key );

        if( orphan.second != 1 || footprints.size() != 1 )
            continue;

        footprints.front()->SetReference( orphan.first );
        renamed++;
    }

    return renamed;
}


NET_RECONCILE_RESULT ReconcileImportedNetNames( BOARD& aBoard, NETLIST& aNetlist, REPORTER& aReporter )
{
    NET_RECONCILE_RESULT result;

    std::map<wxString, FOOTPRINT*> footprints = uniqueFootprints( aBoard );

    // the pins of every schematic net, on the footprints the board has
    std::map<wxString, PIN_SET> schematicNets;
    std::set<wxString>          shared;

    for( unsigned i = 0; i < aNetlist.GetCount(); i++ )
    {
        COMPONENT* component = aNetlist.GetComponent( i );
        wxString   ref = component->GetReference();

        if( !footprints.count( ref ) )
            continue;

        shared.insert( ref );

        for( unsigned j = 0; j < component->GetNetCount(); j++ )
        {
            const COMPONENT_NET& pin = component->GetNet( j );

            if( !pin.GetNetName().IsEmpty() )
                schematicNets[pin.GetNetName()].emplace( ref, pin.GetPinName() );
        }
    }

    std::map<PIN_SET, wxString> schematicNameByPins;

    for( const auto& [name, pins] : schematicNets )
        schematicNameByPins.emplace( pins, name );

    // the pads of every board net, and the pads without one
    std::map<wxString, PIN_SET> boardNets;
    std::map<std::pair<wxString, wxString>, std::vector<PAD*>> bareJoins;

    for( const wxString& ref : shared )
    {
        for( PAD* pad : footprints.at( ref )->Pads() )
        {
            std::pair<wxString, wxString> key( ref, pad->GetNumber() );

            if( pad->GetNetCode() > 0 )
                boardNets[pad->GetNetname()].insert( key );
            else
                bareJoins[key].push_back( pad );
        }
    }

    std::map<wxString, wxString> renames;

    for( const auto& [boardName, pins] : boardNets )
    {
        auto match = schematicNameByPins.find( pins );

        if( match == schematicNameByPins.end() )
        {
            result.m_conflicting++;

            if( result.m_conflicting <= MAX_REPORTED )
            {
                aReporter.Report( wxString::Format( _( "Board net '%s' does not match a single schematic net.\n" ),
                                                    boardName ),
                                  RPT_SEVERITY_INFO );
            }

            continue;
        }

        if( match->second == boardName )
            result.m_alreadyNamed++;
        else
            renames.emplace( boardName, match->second );
    }

    // a rename onto a name a board net keeps would reject the whole batch, and dropping one such rename can
    // leave the name that another rename was moving onto occupied
    for( bool removed = true; removed; )
    {
        removed = false;

        for( auto it = renames.begin(); it != renames.end(); )
        {
            if( aBoard.FindNet( it->second ) && !renames.count( it->second ) )
            {
                result.m_conflicting++;
                it = renames.erase( it );
                removed = true;
            }
            else
            {
                ++it;
            }
        }
    }

    if( ApplyImportedNetNameMap( aBoard, renames, aReporter ) )
        result.m_renamed = static_cast<int>( renames.size() );

    // the schematic names a net that joins one pin, which the board leaves without a net
    for( const auto& [name, pins] : schematicNets )
    {
        if( pins.size() != 1 || !name.StartsWith( wxS( "unconnected-(" ) ) || aBoard.FindNet( name ) )
            continue;

        auto bare = bareJoins.find( *pins.begin() );

        if( bare == bareJoins.end() )
            continue;

        NETINFO_ITEM* net = new NETINFO_ITEM( &aBoard, name );
        aBoard.Add( net );

        for( PAD* pad : bare->second )
            pad->SetNet( net );

        result.m_unconnectedAssigned++;
    }

    aReporter.Report( wxString::Format( _( "Named %d board nets after the schematic, %d already matched, %d differ "
                                           "and %d unconnected pads named.\n" ),
                                        result.m_renamed, result.m_alreadyNamed, result.m_conflicting,
                                        result.m_unconnectedAssigned ),
                      RPT_SEVERITY_INFO );

    return result;
}


int FillImportedFootprintFields( BOARD& aBoard, NETLIST& aNetlist )
{
    std::map<wxString, FOOTPRINT*> footprints = uniqueFootprints( aBoard );
    int                            filled = 0;

    // Update PCB from Schematic treats these as handled separately or not editable on the board
    const std::set<wxString> skipped = { GetDefaultFieldName( FIELD_T::REFERENCE, UNTRANSLATED ),
                                         GetDefaultFieldName( FIELD_T::VALUE, UNTRANSLATED ),
                                         GetDefaultFieldName( FIELD_T::FOOTPRINT, UNTRANSLATED ),
                                         wxS( "Component Class" ) };

    for( unsigned i = 0; i < aNetlist.GetCount(); i++ )
    {
        COMPONENT* component = aNetlist.GetComponent( i );
        auto       found = footprints.find( component->GetReference() );

        if( found == footprints.end() )
            continue;

        FOOTPRINT* footprint = found->second;
        bool       changed = false;

        if( footprint->GetValue().IsEmpty() && !component->GetValue().IsEmpty() )
        {
            footprint->SetValue( component->GetValue() );
            changed = true;
        }

        for( const auto& [name, value] : component->GetFields() )
        {
            if( skipped.count( name ) )
                continue;

            // an empty field still has to exist, as the schematic parity check looks for it
            if( PCB_FIELD* existing = footprint->HasField( name ) ? footprint->GetField( name ) : nullptr )
            {
                if( existing->GetText().IsEmpty() && !value.IsEmpty() )
                {
                    existing->SetText( value );
                    changed = true;
                }

                continue;
            }

            PCB_FIELD* field = new PCB_FIELD( footprint, FIELD_T::USER );
            footprint->Add( field );

            field->SetName( name );
            field->SetText( value );
            field->SetVisible( false );
            field->SetLayer( footprint->GetLayer() == F_Cu ? F_Fab : B_Fab );
            field->SetPosition( footprint->GetPosition() );
            field->Rotate( footprint->GetPosition(), footprint->GetOrientation() );
            field->StyleFromSettings( aBoard.GetDesignSettings(), true );
            changed = true;
        }

        if( changed )
            filled++;
    }

    return filled;
}
