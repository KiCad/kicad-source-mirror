/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2024 KiCad Developers, see AUTHORS.txt for contributors.
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

#include "odb_component.h"
#include "odb_util.h"
#include "hash_eda.h"
#include "pcb_io_odbpp.h"


ODB_COMPONENT& COMPONENTS_MANAGER::AddComponent( const FOOTPRINT*         aFp,
                                                 const EDA_DATA::PACKAGE& aPkg )
{
    auto& comp = m_compList.emplace_back( m_compList.size(), aPkg.m_index );

    comp.m_center = ODB::AddXY( aFp->GetPosition() );

    if( aFp->GetOrientation() != ANGLE_0 )
    {
        // odb Rotation is expressed in degrees and is always clockwise.
        // while kicad EDA_ANGLE is anticlockwise.

        comp.m_rot =
                ODB::Double2String( ( ANGLE_360 - aFp->GetOrientation() ).Normalize().AsDegrees() );
    }

    if( aFp->GetLayer() != F_Cu )
    {
        comp.m_mirror = wxT( "M" );
    }

    comp.m_comp_name = aFp->GetReference().ToAscii();
    comp.m_part_name = wxString::Format( "%s_%s", aFp->GetFPID().GetFullLibraryName(),
                                         aFp->GetFPID().GetLibItemName().wx_str() );

    // ODB++ cannot handle spaces in these fields
    comp.m_comp_name.Trim().Trim( false );
    comp.m_part_name.Trim().Trim( false );

    if( comp.m_comp_name.IsEmpty() )
    {
        // The spec requires a component name; some ODB++ parsers can't handle it being empty
        comp.m_comp_name = wxString::Format( "UNNAMED%zu", m_compList.size() );
    }

    wxRegEx spaces( "\\s" );
    spaces.Replace( &comp.m_comp_name, "_" );
    spaces.Replace( &comp.m_part_name, "_" );

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
