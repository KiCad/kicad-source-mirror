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

#include <wx/regex.h>
#include <wx/log.h>

#include <algorithm>
#include <cmath>
#include <mutex>

#include <3d_cache/3d_cache.h>
#include <3d_math.h>
#include <base_units.h>
#include <board.h>
#include <footprint_library_adapter.h>
#include <footprint.h>
#include <libraries/library_manager.h>
#include <pad.h>
#include <pcb_field.h>
#include <plugins/3dapi/c3dmodel.h>
#include <project_pcb.h>
#include <exporters/fab_model/fab_pin.h>
#include <exporters/fab_model/fab_component_row.h>

#include "odb_component.h"
#include "odb_util.h"
#include "hash_eda.h"
#include "pcb_io_odbpp.h"
#include <trace_helpers.h>

double ODB::ModelHeightAboveBoard( const S3DMODEL& aModel, const FP_3DMODEL& aPlacement )
{
    glm::mat4 transform = CalcModelMatrix( SFVEC3F( aPlacement.m_Offset.x, aPlacement.m_Offset.y,
                                                     aPlacement.m_Offset.z ),
                                           SFVEC3F( aPlacement.m_Rotation.x, aPlacement.m_Rotation.y,
                                                     aPlacement.m_Rotation.z ),
                                           SFVEC3F( aPlacement.m_Scale.x, aPlacement.m_Scale.y,
                                                     aPlacement.m_Scale.z ) );
    double height = 0.0;

    for( unsigned int meshIndex = 0; meshIndex < aModel.m_MeshesSize; ++meshIndex )
    {
        const SMESH& mesh = aModel.m_Meshes[meshIndex];

        for( unsigned int vertexIndex = 0; vertexIndex < mesh.m_VertexSize; ++vertexIndex )
        {
            const SFVEC3F& vertex = mesh.m_Positions[vertexIndex];
            double z = ( transform * glm::vec4( vertex.x, vertex.y, vertex.z, 1.0f ) ).z;

            if( std::isfinite( z ) )
                height = std::max( height, z );
        }
    }

    return height;
}


bool ODB::HasShownModel( const FOOTPRINT* aFp )
{
    return std::any_of( aFp->Models().begin(), aFp->Models().end(),
                        []( const FP_3DMODEL& model )
                        {
                            return model.m_Show && !model.m_Filename.IsEmpty();
                        } );
}


namespace
{
double componentModelHeight( const FOOTPRINT* aFp )
{
    if( !ODB::HasShownModel( aFp ) )
        return 0.0;

    const BOARD* board = aFp->GetBoard();

    if( !board || !board->GetProject() )
        return 0.0;

    static std::mutex cacheMutex;
    std::lock_guard<std::mutex> lock( cacheMutex );
    PROJECT* project = board->GetProject();
    S3D_CACHE* cache = PROJECT_PCB::Get3DCacheManager( project );
    wxString basePath;

    if( FOOTPRINT_LIBRARY_ADAPTER* adapter = PROJECT_PCB::FootprintLibAdapter( project ) )
    {
        std::optional<LIBRARY_TABLE_ROW*> row = adapter->GetRow( aFp->GetFPID().GetLibNickname() );

        if( row )
            basePath = LIBRARY_MANAGER::GetFullURI( *row, true );
    }

    double height = 0.0;

    for( const FP_3DMODEL& placement : aFp->Models() )
    {
        if( !placement.m_Show || placement.m_Filename.IsEmpty() )
            continue;

        std::vector<const EMBEDDED_FILES*> embeddedFiles = { aFp->GetEmbeddedFiles(),
                                                               board->GetEmbeddedFiles() };
        S3DMODEL* model = cache->GetModel( placement.m_Filename, basePath, std::move( embeddedFiles ) );

        if( model )
            height = std::max( height, ODB::ModelHeightAboveBoard( *model, placement ) );
    }

    return height;
}
}


ODB_COMPONENT& COMPONENTS_MANAGER::AddComponent( const FOOTPRINT*         aFp,
                                                 const EDA_DATA::PACKAGE& aPkg )
{
    auto& comp = m_compList.emplace_back( m_compList.size(), aPkg.m_index );

    comp.m_center = ODB::AddXY( m_plugin->GetFormat(), aFp->GetPosition() );
    EDA_ANGLE angle = aFp->GetOrientation();

    if( angle != ANGLE_0 )
    {
        // odb Rotation is expressed in degrees and is always clockwise.
        // while kicad EDA_ANGLE is anticlockwise.
        angle = ANGLE_360 - angle;
        comp.m_rot = ODB::Double2String( m_plugin->GetFormat(), angle.Normalize().AsDegrees() );
    }

    if( aFp->IsFlipped() )
    {
        comp.m_mirror = wxT( "M" );
    }

    wxString originalRef = aFp->GetReference();
    comp.m_comp_name = ODB::GenLegalComponentName( originalRef );

    const ODB_FORMAT& format = m_plugin->GetFormat();
    FAB_COMPONENT_ROW row = MakeFabComponentRow( *aFp, format.m_variantName );
    comp.m_part_name = format.m_mpnField.IsEmpty() ? wxString() : row.Field( format.m_mpnField );

    if( comp.m_part_name.IsEmpty() )
        comp.m_part_name = wxString::Format( "%s_%s", aFp->GetFPID().GetFullLibraryName(),
                                             aFp->GetFPID().GetLibItemName().wx_str() );

    // ODB++ cannot handle spaces in these fields
    ODB::RemoveWhitespace( comp.m_part_name );
    comp.m_part_name = ODB::GenLegalComponentName( comp.m_part_name );

    double heightMm = componentModelHeight( aFp );

    if( heightMm <= 0.0 )
    {
        wxString value = row.Field( wxS( "Height" ) );

        if( !value.IsEmpty() )
        {
            double heightIU = EDA_UNIT_UTILS::UI::DoubleValueFromString( pcbIUScale, EDA_UNITS::MM, value );
            heightMm = pcbIUScale.IUTomm( heightIU );
        }
    }

    if( std::isfinite( heightMm ) && heightMm > 0.0 )
    {
        double outputHeight = format.m_unitsStr == "INCH" ? heightMm / 25.4 : heightMm;
        AddSystemAttribute( comp, ODB_ATTR::COMP_HEIGHT{ outputHeight } );
    }

    if( comp.m_comp_name.IsEmpty() )
    {
        // The spec requires a component name; some ODB++ parsers can't handle it being empty
        comp.m_comp_name = wxString::Format( "UNNAMED%zu", m_compList.size() );
    }

    // Warn if non-ASCII characters were converted
    if( comp.m_comp_name != originalRef )
    {
        if( m_plugin )
        {
            m_plugin->Report( wxString::Format( _( "Component '%s' has non-ASCII characters in its "
                                                   "designator; converted to '%s' for ODB++ export." ),
                                                originalRef,
                                                comp.m_comp_name ),
                              RPT_SEVERITY_WARNING );
        }
    }

    wxString base_comp_name = comp.m_comp_name;

    if( !m_usedCompNames.insert( comp.m_comp_name ).second )
    {
        size_t suffix = 1;
        wxString candidate;

        do
        {
            candidate = wxString::Format( "%s_%zu", base_comp_name, suffix++ );
        } while( !m_usedCompNames.insert( candidate ).second );

        if( m_plugin )
        {
            m_plugin->Report( wxString::Format( _( "Component '%s' has an ambiguous designator after "
                                                   "conversion; renamed to '%s' for ODB++ export." ),
                                                originalRef,
                                                candidate ),
                              RPT_SEVERITY_WARNING );
        }

        comp.m_comp_name = candidate;
    }

    for( PCB_FIELD* field : aFp->GetFields() )
    {
        if( field->GetId() == FIELD_T::REFERENCE )
            continue;

        wxString key = field->GetName();
        ODB::RemoveWhitespace( key );

        // A PRP record is one line
        wxString value = row.Field( field->GetName() );
        value.Replace( wxS( "\r" ), wxEmptyString );
        value.Replace( wxS( "\n" ), wxS( " " ) );

        comp.m_prp[key] = wxString::Format( "'%s'", value );
    }

    if( row.m_dnp )
    {
        AddSystemAttribute( comp, ODB_ATTR::NO_POP{ true } );
    }

    if( const BOARD* board = aFp->GetBoard(); board && !board->GetVariantNames().empty()
        && m_plugin->GetFormat().m_variantNames.m_listsFit )
    {
        std::vector<wxString> included;

        for( const wxString& variant : board->GetVariantNames() )
        {
            if( !aFp->GetDNPForVariant( variant ) )
                included.push_back( variant );
        }

        AddSystemAttribute( comp, ODB_ATTR::COMP_VARIANT_LIST{
                m_plugin->GetFormat().m_variantNames.Join( included ).ToStdString() } );
    }

    if( row.m_pressFit )
    {
        AddSystemAttribute( comp, ODB_ATTR::COMP_MOUNT_TYPE::PRESSFIT );
    }
    else if( row.m_mount == FAB_MOUNT::SMT )
    {
        AddSystemAttribute( comp, ODB_ATTR::COMP_MOUNT_TYPE::MT_SMD );
    }
    else if( row.m_mount == FAB_MOUNT::THT )
    {
        AddSystemAttribute( comp, ODB_ATTR::COMP_MOUNT_TYPE::THT );
    }
    else
    {
        AddSystemAttribute( comp, ODB_ATTR::COMP_MOUNT_TYPE::OTHER );
    }

    return comp;
}


void COMPONENTS_MANAGER::Write( std::ostream& ost ) const
{
    ost << "UNITS=" << m_plugin->GetFormat().m_unitsStr << std::endl;

    WriteAttributes( ost );

    for( const auto& comp : m_compList )
    {
        comp.Write( ost );
    }
}


void ODB_COMPONENT::Write( std::ostream& ost ) const
{
    ost << "# CMP " << m_index << std::endl;
    ost << "CMP " << m_pkg_ref << " " << m_center.first << " " << m_center.second << " " << m_rot << " " << m_mirror
        << " " << m_comp_name.utf8_string() << " " << m_part_name.utf8_string();

    WriteAttributes( ost );

    ost << std::endl;

    for( const auto& [key, value] : m_prp )
    {
        ost << "PRP " << key.utf8_string() << " " << value.utf8_string() << std::endl;
    }

    for( const auto& toep : m_toeprints )
    {
        toep.Write( ost );
    }

    ost << "#" << std::endl;
}


void ODB_COMPONENT::TOEPRINT::Write( std::ostream& ost ) const
{
    ost << "TOP " << m_pin_num << " " << m_center.first << " " << m_center.second << " " << m_rot << " " << m_mirror
        << " " << m_net_num << " " << m_subnet_num << " " << m_toeprint_name.utf8_string() << std::endl;
}
