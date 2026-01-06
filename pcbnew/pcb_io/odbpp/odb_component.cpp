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
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <wx/regex.h>
#include <wx/log.h>

#include <board.h>

#include "odb_component.h"
#include "odb_util.h"
#include "hash_eda.h"
#include "pcb_io_odbpp.h"


ODB_COMPONENT& COMPONENTS_MANAGER::AddComponent( const FOOTPRINT*         aFp,
                                                 const EDA_DATA::PACKAGE& aPkg )
{
    auto& comp = m_compList.emplace_back( m_compList.size(), aPkg.m_index );

    comp.m_center = ODB::AddXY( aFp->GetPosition() );
    EDA_ANGLE angle = aFp->GetOrientation();

    if( angle != ANGLE_0 )
    {
        // odb Rotation is expressed in degrees and is always clockwise.
        // while kicad EDA_ANGLE is anticlockwise.
        angle = ANGLE_360 - angle;
        comp.m_rot = ODB::Double2String( angle.Normalize().AsDegrees() );
    }

    if( aFp->IsFlipped() )
    {
        comp.m_mirror = wxT( "M" );
    }

    wxString originalRef = aFp->GetReference();
    comp.m_comp_name = ODB::GenLegalComponentName( originalRef );

    comp.m_part_name = wxString::Format( "%s_%s", aFp->GetFPID().GetFullLibraryName(),
                                         aFp->GetFPID().GetLibItemName().wx_str() );

    // ODB++ cannot handle spaces in these fields
    ODB::RemoveWhitespace( comp.m_part_name );

    if( comp.m_comp_name.IsEmpty() )
    {
        // The spec requires a component name; some ODB++ parsers can't handle it being empty
        comp.m_comp_name = wxString::Format( "UNNAMED%zu", m_compList.size() );
    }

    // Warn if non-ASCII characters were converted
    if( comp.m_comp_name != originalRef )
    {
        wxLogWarning( _( "Component '%s' has non-ASCII characters in its designator; "
                         "converted to '%s' for ODB++ export." ),
                      originalRef, comp.m_comp_name );
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

        wxLogWarning( _( "Component '%s' has an ambiguous designator after conversion; "
                         "renamed to '%s' for ODB++ export." ),
                      originalRef, candidate );

        comp.m_comp_name = candidate;
    }

    for( PCB_FIELD* field : aFp->GetFields() )
    {
        if( field->GetId() == FIELD_T::REFERENCE )
            continue;

        wxString key = field->GetName();
        ODB::RemoveWhitespace( key );
        comp.m_prp[key] = wxString::Format( "'%s'", field->GetShownText( false ) );
    }

    if( aFp->GetDNPForVariant( aFp->GetBoard() ? aFp->GetBoard()->GetCurrentVariant() : wxString() ) )
    {
        AddSystemAttribute( comp, ODB_ATTR::NO_POP{ true } );
    }

    if( aFp->GetAttributes() & FP_SMD )
    {
        AddSystemAttribute( comp, ODB_ATTR::COMP_MOUNT_TYPE::MT_SMD );
    }
    else if( aFp->GetAttributes() & FP_THROUGH_HOLE )
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
    ost << "UNITS=" << PCB_IO_ODBPP::m_unitsStr << std::endl;

    WriteAttributes( ost );

    for( const auto& comp : m_compList )
    {
        comp.Write( ost );
    }
}


void ODB_COMPONENT::Write( std::ostream& ost ) const
{
    ost << "# CMP " << m_index << std::endl;
    ost << "CMP " << m_pkg_ref << " " << m_center.first << " " << m_center.second << " " << m_rot
        << " " << m_mirror << " " << m_comp_name << " " << m_part_name;

    WriteAttributes( ost );

    ost << std::endl;

    for( const auto& [key, value] : m_prp )
    {
        ost << "PRP " << key << " " << value << std::endl;
    }

    for( const auto& toep : m_toeprints )
    {
        toep.Write( ost );
    }

    ost << "#" << std::endl;
}


void ODB_COMPONENT::TOEPRINT::Write( std::ostream& ost ) const
{
    ost << "TOP " << m_pin_num << " " << m_center.first << " " << m_center.second << " " << m_rot
        << " " << m_mirror << " " << m_net_num << " " << m_subnet_num << " " << m_toeprint_name
        << std::endl;
}
