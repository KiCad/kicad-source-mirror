/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 * Author: SYSUEric <jzzhuang666@gmail.com>.
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
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "odb_netlist.h"

#include <string_view>

#include <board.h>
#include <footprint.h>
#include <pad.h>
#include <pcb_track.h>

#include <exporters/fab_model/fab_pin.h>
#include <exporters/fab_model/fab_test_points.h>

#include <odb_defines.h>
#include <odb_util.h>
#include <pcb_io_odbpp.h>


// Compute the side code for a via.
static ODB_NET_RECORD::SIDE ComputeViaAccessSide( int top_layer, int bottom_layer )
{
    // Easy case for through vias: top_layer is component, bottom_layer is
    // solder, side code is Both
    if( ( top_layer == F_Cu ) && ( bottom_layer == B_Cu ) )
        return ODB_NET_RECORD::SIDE::BOTH;

    // Blind via, reachable from front, Top
    if( top_layer == F_Cu )
        return ODB_NET_RECORD::SIDE::TOP;

    // Blind via, reachable from bottom, Down
    if( bottom_layer == B_Cu )
        return ODB_NET_RECORD::SIDE::DOWN;

    // It's a buried via, accessible from some inner layer, Inner
    return ODB_NET_RECORD::SIDE::INNER;
}


void ODB_NET_LIST::InitNetPoints( std::map<size_t, std::vector<ODB_NET_RECORD>>& aRecords )
{
    for( const FAB_TEST_POINT& point : FabTestPoints( *m_board ) )
    {
        ODB_NET_RECORD record;
        record.testpoint = false;
        record.is_via = point.m_via != nullptr;
        record.x_location = point.m_position.x;
        record.y_location = -point.m_position.y;
        record.soldermask = 3;
        record.epoint = point.m_netEnd ? "e" : "m";

        if( point.m_frontMask )
            record.soldermask &= ~1;

        if( point.m_backMask )
            record.soldermask &= ~2;

        if( const PCB_VIA* via = point.m_via )
        {
            PCB_LAYER_ID top, bottom;
            via->LayerPair( &top, &bottom );
            record.side = ComputeViaAccessSide( top, bottom );

            if( record.side == ODB_NET_RECORD::SIDE::INNER )
                continue;

            record.refdes = "VIA";
            record.smd = false;
            record.hole = true;
            record.drill_radius = point.m_drill / 2;
            record.mechanical = false;
            record.x_size = 0;
            record.y_size = 0;
        }
        else
        {
            const PAD* pad = point.m_pad;

            if( !point.m_front && !point.m_back )
                continue;

            if( point.m_front && point.m_back )
                record.side = ODB_NET_RECORD::SIDE::BOTH;
            else if( point.m_front )
                record.side = ODB_NET_RECORD::SIDE::TOP;
            else
                record.side = ODB_NET_RECORD::SIDE::DOWN;

            record.refdes = pad->GetParentFootprint()->GetReference();
            record.hole = pad->HasHole();
            record.drill_radius = record.hole ? point.m_drill / 2 : 0;
            record.smd = pad->GetAttribute() == PAD_ATTRIB::SMD || pad->GetAttribute() == PAD_ATTRIB::CONN;
            record.mechanical = pad->GetAttribute() == PAD_ATTRIB::NPTH;
            record.testpoint = GetFabPadRole( *pad ) == FAB_PAD_ROLE::TESTPOINT;

            // The land size is written only for undrilled pads, which are single-sided, so
            // the access side gives the copper to report
            PCB_LAYER_ID sideLayer =
                    pad->Padstack().EffectiveLayerFor( record.side == ODB_NET_RECORD::SIDE::DOWN ? B_Cu : F_Cu );
            record.x_size = pad->GetSize( sideLayer ).x;

            // A circle's stored second size can differ from its diameter
            if( pad->GetShape( sideLayer ) == PAD_SHAPE::CIRCLE )
                record.y_size = record.x_size;
            else
                record.y_size = pad->GetSize( sideLayer ).y;
        }

        aRecords[point.m_netCode].push_back( record );
    }
}


static std::string_view SideToString( ODB_NET_RECORD::SIDE aSide )
{
    switch( aSide )
    {
        case ODB_NET_RECORD::SIDE::TOP:     return "T";
        case ODB_NET_RECORD::SIDE::DOWN:    return "D";
        case ODB_NET_RECORD::SIDE::BOTH:    return "B";
        case ODB_NET_RECORD::SIDE::INNER:   return "I";
        // No default
    }

    return {};
}


void ODB_NET_LIST::WriteNetPointRecords( std::map<size_t, std::vector<ODB_NET_RECORD>>& aRecords,
                                         std::ostream&                                  aStream,
                                         const ODB_FORMAT&                              aFormat )
{
    aStream << "H optimize n staggered n" << std::endl;
    aStream << ODB_UNITS << "=" << aFormat.m_unitsStr << std::endl;

    for( const auto& [key, vec] : aRecords )
    {
        aStream << "$" << key << " " << m_plugin->GetLegalNetName( static_cast<int>( key ) ) << std::endl;
    }

    aStream << "#" << std::endl << "#Netlist points" << std::endl << "#" << std::endl;

    for( const auto& [key, vec] : aRecords )
    {
        for( const auto& net_point : vec )
        {
            aStream << key << " ";

            if( net_point.hole )
                aStream << ODB::Data2String( aFormat, net_point.drill_radius );
            else
                aStream << "0.002";

            aStream << " " << ODB::Data2String( aFormat, net_point.x_location - aFormat.m_originOffset.x ) << " "
                    << ODB::Data2String( aFormat, net_point.y_location + aFormat.m_originOffset.y ) << " "
                    << SideToString( net_point.side ) << " ";

            if( !net_point.hole )
                aStream << ODB::Data2String( aFormat, net_point.x_size ) << " "
                        << ODB::Data2String( aFormat, net_point.y_size ) << " ";

            std::string exp;

            if( net_point.soldermask == 3 )
                exp = "c";
            else if( net_point.soldermask == 2 )
                exp = "s";
            else if( net_point.soldermask == 1 )
                exp = "p";
            else if( net_point.soldermask == 0 )
                exp = "e";

            aStream << net_point.epoint << " " << exp;

            if( net_point.hole )
                aStream << " staggered 0 0 0";

            if( net_point.is_via )
                aStream << " v";

            if( net_point.testpoint )
                aStream << " t";

            aStream << std::endl;
        }
    }
}


void ODB_NET_LIST::Write( std::ostream& aStream, const ODB_FORMAT& aFormat )
{
    std::map<size_t, std::vector<ODB_NET_RECORD>> net_point_records;

    InitNetPoints( net_point_records );

    WriteNetPointRecords( net_point_records, aStream, aFormat );
}
