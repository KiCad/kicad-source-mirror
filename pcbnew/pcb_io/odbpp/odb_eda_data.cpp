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

#include "odb_eda_data.h"

#include <base_units.h>
#include <build_version.h>
#include <hash_eda.h>
#include <footprint.h>
#include <pad.h>
#include <exporters/fab_model/fab_pin.h>
#include <string_utils.h>

#include <netinfo.h>
#include <odb_feature.h>
#include <pcb_io_odbpp.h>


EDA_DATA::EDA_DATA()
{
    auto& x = nets_map.emplace( std::piecewise_construct, std::forward_as_tuple( 0 ),
                                std::forward_as_tuple( nets.size(), "$NONE$" ) )
                      .first->second;

    nets.push_back( &x );
}


void EDA_DATA::NET::Write( std::ostream& ost, bool aIncludeComponents ) const
{
    ost << "NET " << m_name;

    WriteAttributes( ost );

    // Empty attributes still need their own field before the NET ID
    if( m_uid != 0 )
        ost << ( m_ODBattributes.empty() ? ";;ID=" : ";ID=" ) << m_uid;

    ost << std::endl;

    for( const auto& subnet : subnets )
    {
        if( !aIncludeComponents && dynamic_cast<const SUB_NET_TOEPRINT*>( subnet.get() ) )
            continue;

        subnet->Write( ost );
    }
}


void EDA_DATA::AddNET( const NETINFO_ITEM* aNet, const wxString& aName )
{
    if( nets_map.end() == nets_map.find( aNet->GetNetCode() ) )
    {
        auto& net = nets_map.emplace( std::piecewise_construct,
                                      std::forward_as_tuple( aNet->GetNetCode() ),
                                      std::forward_as_tuple( nets.size(), aName ) )
                            .first->second;

        nets.push_back( &net );

        //TODO: netname check
    }
}


void EDA_DATA::AssignNetUids( PCB_IO_ODBPP& aPlugin )
{
    for( NET* net : nets )
        net->m_uid = aPlugin.NewUid();
}


bool EDA_DATA::AddShort( const std::set<int>& aNetCodes, const wxString& aLayer,
                         size_t aFeatureIndex, uint32_t aUid )
{
    std::set<uint32_t> uids;

    for( int code : aNetCodes )
    {
        if( code > 0 && nets_map.count( code ) )
            uids.insert( nets_map.at( code ).m_uid );
    }

    if( uids.size() < 2 )
        return false;

    m_shorts.push_back( { std::move( uids ), GetLyrIdx( aLayer ), aFeatureIndex, aUid } );
    return true;
}


void EDA_DATA::SUB_NET::Write( std::ostream& ost ) const
{
    ost << "SNT ";

    WriteSubnet( ost );

    ost << std::endl;

    for( const auto& fid : feature_ids )
    {
        fid.Write( ost );
    }
}


void EDA_DATA::FEATURE_ID::Write( std::ostream& ost ) const
{
    static const std::map<TYPE, std::string> type_map = {
        { TYPE::COPPER, "C" },
        { TYPE::HOLE, "H" },
    };

    ost << "FID " << type_map.at( type ) << " " << layer << " " << feature_id << std::endl;
}


void EDA_DATA::SUB_NET_VIA::WriteSubnet( std::ostream& ost ) const
{
    ost << "VIA";
}


void EDA_DATA::SUB_NET_TRACE::WriteSubnet( std::ostream& ost ) const
{
    ost << "TRC";
}


void EDA_DATA::SUB_NET_PLANE::WriteSubnet( std::ostream& ost ) const
{
    static const std::map<FILL_TYPE, std::string> fill_type_map = { { FILL_TYPE::SOLID, "S" },
                                                                    { FILL_TYPE::OUTLINE, "O" } };

    static const std::map<CUTOUT_TYPE, std::string> cutout_type_map = {
        { CUTOUT_TYPE::CIRCLE, "C" },
        { CUTOUT_TYPE::RECTANGLE, "R" },
        { CUTOUT_TYPE::OCTAGON, "O" },
        { CUTOUT_TYPE::EXACT, "E" }
    };

    ost << "PLN " << fill_type_map.at( fill_type ) << " " << cutout_type_map.at( cutout_type )
        << " " << fill_size;
}


void EDA_DATA::SUB_NET_TOEPRINT::WriteSubnet( std::ostream& ost ) const
{
    static const std::map<SIDE, std::string> side_map = {
        { SIDE::BOTTOM, "B" },
        { SIDE::TOP, "T" },
    };
    ost << "TOP " << side_map.at( side ) << " " << comp_num << " " << toep_num;
}


void EDA_DATA::SUB_NET::AddFeatureID( FEATURE_ID::TYPE type, const wxString& layer,
                                      size_t feature_id )
{
    feature_ids.emplace_back( type, m_edadata->GetLyrIdx( layer ), feature_id );
}


size_t EDA_DATA::GetLyrIdx( const wxString& aLayer )
{
    if( layers_map.count( aLayer ) )
    {
        return layers_map.at( aLayer );
    }
    else
    {
        auto idx = layers_map.size();
        layers_map.emplace( aLayer, idx );
        layers.push_back( aLayer );
        return idx;
    }
}


void OUTLINE_SQUARE::Write( std::ostream& ost, const ODB_FORMAT& aFormat ) const
{
    ost << "SQ " << ODB::Data2String( aFormat, m_center.x ) << " " << ODB::Data2String( aFormat, m_center.y ) << " "
        << ODB::Data2String( aFormat, m_halfSide ) << std::endl;
}


void OUTLINE_CIRCLE::Write( std::ostream& ost, const ODB_FORMAT& aFormat ) const
{
    ost << "CR " << ODB::Data2String( aFormat, m_center.x ) << " " << ODB::Data2String( aFormat, m_center.y ) << " "
        << ODB::Data2String( aFormat, m_radius ) << std::endl;
}


void OUTLINE_RECT::Write( std::ostream& ost, const ODB_FORMAT& aFormat ) const
{
    ost << "RC " << ODB::Data2String( aFormat, m_lower_left.x ) << " " << ODB::Data2String( aFormat, m_lower_left.y )
        << " " << ODB::Data2String( aFormat, m_width ) << " " << ODB::Data2String( aFormat, m_height ) << std::endl;
}


void OUTLINE_CONTOUR::Write( std::ostream& ost, const ODB_FORMAT& aFormat ) const
{
    if( !m_surfaces )
        return;

    ost << "CT" << std::endl;
    m_surfaces->WriteData( ost, aFormat );
    ost << "CE" << std::endl;
}


void EDA_DATA::AddPackage( const FOOTPRINT* aFp, const ODB_FORMAT& aFormat )
{
    // ODBPP only need unique PACKAGE in PKG record in eda/data file.
    // the PKG index can repeat to be ref in CMP record in component file.

    std::shared_ptr<FOOTPRINT> fp( static_cast<FOOTPRINT*>( aFp->Clone() ) );
    m_eda_footprints.emplace_back( fp );
    fp->SetParentGroup( nullptr );
    fp->SetPosition( { 0, 0 } );

    if( aFp->IsFlipped() )
    {
        // ODB++ needs both flips
        fp->Flip( fp->GetPosition(), FLIP_DIRECTION::LEFT_RIGHT );
        fp->Flip( fp->GetPosition(), FLIP_DIRECTION::TOP_BOTTOM );
    }

    fp->SetOrientation( ANGLE_0 );

    size_t   hash = hash_fp_item( fp.get(), HASH_POS | REL_COORD );
    size_t   pkg_index = packages_map.size();
    wxString fp_name = fp->GetFPID().GetLibItemName().wx_str();
    ODB::RemoveWhitespace( fp_name );

    if( fp_name.IsEmpty() )
        fp_name = wxS( "__" );

    auto [iter, success] = packages_map.emplace( hash, PACKAGE( pkg_index, fp_name ) );

    if( !success )
    {
        return;
    }

    PACKAGE* pkg = &( iter->second );

    packages.push_back( pkg );

    BOX2I bbox = fp->GetBoundingBox();
    pkg->m_xmin = bbox.GetPosition().x;
    pkg->m_ymin = -bbox.GetEnd().y;
    pkg->m_xmax = bbox.GetEnd().x;
    pkg->m_ymax = -bbox.GetPosition().y;
    pkg->m_pitch = UINT64_MAX;

    if( fp->Pads().size() < 2 )
        pkg->m_pitch = pcbIUScale.mmToIU( 1.0 ); // placeholder value

    for( size_t i = 0; i < fp->Pads().size(); ++i )
    {
        const PAD* pad1 = fp->Pads()[i];

        for( size_t j = i + 1; j < fp->Pads().size(); ++j )
        {
            const PAD*     pad2 = fp->Pads()[j];
            const uint64_t pin_dist = ( pad1->GetCenter() - pad2->GetCenter() ).EuclideanNorm();
            pkg->m_pitch = std::min( pkg->m_pitch, pin_dist );
        }
    }

    const SHAPE_POLY_SET& courtyard = fp->GetCourtyard( F_CrtYd );
    const SHAPE_POLY_SET& courtyard_back = fp->GetCourtyard( B_CrtYd );
    SHAPE_POLY_SET        pkg_outline;

    if( courtyard.OutlineCount() > 0 )
        pkg_outline = courtyard;

    if( courtyard_back.OutlineCount() > 0 )
    {
        pkg_outline = courtyard_back;
    }

    if( !courtyard.OutlineCount() && !courtyard_back.OutlineCount() )
    {
        pkg_outline = fp->GetBoundingHull();
    }

    // TODO: Here we put rect, square, and circle, all as polygon

    if( pkg_outline.OutlineCount() > 0 )
    {
        for( int ii = 0; ii < pkg_outline.OutlineCount(); ++ii )
        {
            pkg->m_pkgOutlines.push_back(
                    std::make_unique<OUTLINE_CONTOUR>( pkg_outline.Polygon( ii ) ) );
        }
    }

    for( size_t i = 0; i < fp->Pads().size(); ++i )
    {
        const PAD* pad = fp->Pads()[i];
        pkg->AddPin( pad, i, aFormat );
    }

    return;
}


void EDA_DATA::PACKAGE::AddPin( const PAD* aPad, size_t aPinNum, const ODB_FORMAT& aFormat )
{
    FAB_PIN fabPin = MakeFabPin( *aPad, aPinNum );
    wxString name = fabPin.m_name;

    // ODB is unhappy with whitespace in most places
    ODB::RemoveWhitespace( name );

    // Whitespace-only pad numbers can become empty after filtering
    if( name.IsEmpty() )
    {
        name = wxString::Format( "PAD%zu", aPinNum );
    }

    // // for SNT record, pad, net, pin
    std::shared_ptr<PIN> pin = std::make_shared<PIN>( m_pinsVec.size(), name );
    m_pinsVec.push_back( pin );

    VECTOR2D relpos = aPad->GetFPRelativePosition();

    pin->m_center = ODB::AddRelativeXY( aFormat, relpos );

    if( aPad->HasHole() )
    {
        pin->type = PIN::TYPE::THROUGH_HOLE;
    }
    else
    {
        pin->type = PIN::TYPE::SURFACE;
    }

    switch( fabPin.m_electrical )
    {
    case FAB_ELECTRICAL::MECHANICAL: pin->etype = PIN::ELECTRICAL_TYPE::MECHANICAL; break;
    case FAB_ELECTRICAL::ELECTRICAL: pin->etype = PIN::ELECTRICAL_TYPE::ELECTRICAL; break;
    case FAB_ELECTRICAL::UNDEFINED:  pin->etype = PIN::ELECTRICAL_TYPE::UNDEFINED; break;
    }

    switch( fabPin.m_mount )
    {
    case FAB_MOUNT::THT:   pin->mtype = PIN::MOUNT_TYPE::THROUGH_HOLE; break;
    case FAB_MOUNT::HOLE:  pin->mtype = PIN::MOUNT_TYPE::HOLE; break;
    case FAB_MOUNT::SMT:   pin->mtype = PIN::MOUNT_TYPE::SMT; break;
    case FAB_MOUNT::OTHER: pin->mtype = PIN::MOUNT_TYPE::UNDEFINED; break;
    }

    if( fabPin.m_role == FAB_PAD_ROLE::PRESSFIT )
        pin->mtype = PIN::MOUNT_TYPE::PRESSFIT;

    // AddPackage() unflips the footprint, so this pin record always describes the component-side
    // land
    PCB_LAYER_ID mountLayer = aPad->Padstack().EffectiveLayerFor( F_Cu );

    const std::shared_ptr<SHAPE_POLY_SET>& polygons = aPad->GetEffectivePolygon( mountLayer, ERROR_INSIDE );

    // TODO: Here we put all pad shapes as polygonl, we should switch by pad shape
    // Note:pad only use polygons->Polygon(0),
    if( polygons->OutlineCount() > 0 )
    {
        pin->m_pinOutlines.push_back( std::make_unique<OUTLINE_CONTOUR>( polygons->Polygon( 0 ) ) );
    }
}


void EDA_DATA::PIN::Write( std::ostream& ost, const ODB_FORMAT& aFormat ) const
{
    static const std::map<TYPE, std::string> type_map = { { TYPE::SURFACE, "S" },
                                                          { TYPE::THROUGH_HOLE, "T" },
                                                          { TYPE::BLIND, "B" } };

    static const std::map<ELECTRICAL_TYPE, std::string> etype_map = {
        { ELECTRICAL_TYPE::ELECTRICAL, "E" },
        { ELECTRICAL_TYPE::MECHANICAL, "M" },
        { ELECTRICAL_TYPE::UNDEFINED, "U" }
    };
    static const std::map<MOUNT_TYPE, std::string> mtype_map = { { MOUNT_TYPE::THROUGH_HOLE, "T" },
                                                                 { MOUNT_TYPE::HOLE, "H" },
                                                                 { MOUNT_TYPE::SMT, "S" },
                                                                 { MOUNT_TYPE::PRESSFIT, "P" },
                                                                 { MOUNT_TYPE::UNDEFINED, "U" } };

    ost << "PIN " << m_name.utf8_string() << " " << type_map.at( type ) << " " << m_center.first << " "
        << m_center.second << " 0 " << etype_map.at( etype ) << " " << mtype_map.at( mtype ) << std::endl;

    for( const auto& outline : m_pinOutlines )
    {
        outline->Write( ost, aFormat );
    }
}


void EDA_DATA::PACKAGE::Write( std::ostream& ost, const ODB_FORMAT& aFormat ) const
{
    ODB_FORMAT localFormat = aFormat;
    localFormat.m_originOffset = { 0, 0 };
    ost << "PKG " << m_name << " " << ODB::Data2String( localFormat, m_pitch ) << " "
        << ODB::Data2String( localFormat, m_xmin ) << " " << ODB::Data2String( localFormat, m_ymin ) << " "
        << ODB::Data2String( localFormat, m_xmax ) << " " << ODB::Data2String( localFormat, m_ymax );

    WriteAttributes( ost );

    ost << std::endl;

    for( const auto& outline : m_pkgOutlines )
    {
        outline->Write( ost, localFormat );
    }

    for( const auto& pin : m_pinsVec )
    {
        pin->Write( ost, localFormat );
    }
}


void EDA_DATA::Write( std::ostream& ost, const ODB_FORMAT& aFormat ) const
{
    ost << "# " << wxDateTime::Now().FormatISOCombined() << std::endl;
    ost << "HDR KiCad EDA " << TO_UTF8( GetBuildVersion() ) << std::endl;
    ost << "UNITS=" << aFormat.m_unitsStr << std::endl;
    ost << "LYR";

    for( const auto& layer : layers )
    {
        ost << " " << layer;
    }

    ost << std::endl;

    WriteAttributes( ost, "#" );

    if( aFormat.m_writeEdaNets )
    {
        for( const auto& net : nets )
        {
            ost << "#NET " << net->m_index << std::endl;
            net->Write( ost, aFormat.Includes( FAB::SECTION::COMPONENTS ) );
        }
    }

    size_t i = 0;
    for( const auto* pkg : packages )
    {
        if( !aFormat.Includes( FAB::SECTION::PACKAGES ) )
            break;

        ost << "# PKG " << i << std::endl;
        i++;
        pkg->Write( ost, aFormat );
        ost << "#" << std::endl;
    }
}


void EDA_DATA::WriteShortf( std::ostream& ost, const ODB_FORMAT& aFormat ) const
{
    ost << "UNITS=" << aFormat.m_unitsStr << std::endl;
    ost << "LYR";

    for( const wxString& layer : layers )
        ost << " " << layer;

    ost << std::endl;

    for( const SHORT_NET& shortNet : m_shorts )
    {
        ost << "SRTF";

        for( uint32_t uid : shortNet.m_netUids )
            ost << " " << uid;

        ost << ";ID=" << shortNet.m_uid << std::endl;
        ost << "FID C " << shortNet.m_layerIndex << " " << shortNet.m_featureIndex << std::endl;
    }
}
