/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2025 Mark Roszko <mark.roszko@gmail.com>
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
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#include "exporters/step/kicad3d_info.h"

const Standard_GUID& KICAD3D_INFO::GetID()
{
    static Standard_GUID ID( "D683031E-AD81-4084-B0F3-C0A474CAA93D" );
    return ID;
}


Handle( KICAD3D_INFO ) KICAD3D_INFO::Set( const TDF_Label&   aLabel,
                                          KICAD3D_MODEL_TYPE aModelType,
                                          std::string aDisplayName )
{
    Handle( KICAD3D_INFO ) T;
    if( !aLabel.FindAttribute( KICAD3D_INFO::GetID(), T ) )
    {
        T = new KICAD3D_INFO();
        T->SetModelType( aModelType );
        T->SetDisplayName( aDisplayName );
        aLabel.AddAttribute( T );
    }
    return T;
}


const Standard_GUID& KICAD3D_INFO::ID() const
{
    return GetID();
}


void KICAD3D_INFO::Restore( const Handle( TDF_Attribute ) & aAttribute )
{
    Handle( KICAD3D_INFO ) old = Handle( KICAD3D_INFO )::DownCast( aAttribute );

    m_modelType = old->m_modelType;
    m_displayName = old->m_displayName;
}


Handle( TDF_Attribute ) KICAD3D_INFO::NewEmpty() const
{
    return new KICAD3D_INFO();
}


void KICAD3D_INFO::Paste( const Handle( TDF_Attribute ) & aAttribute,
                          const Handle( TDF_RelocationTable ) & aRelocationTable ) const
{
    Handle( KICAD3D_INFO ) that = Handle( KICAD3D_INFO )::DownCast( aAttribute );

    that->m_modelType = m_modelType;
    that->m_displayName = m_displayName;
}


Standard_OStream& KICAD3D_INFO::Dump( Standard_OStream& aOS ) const
{
    aOS << "KICAD3D_INFO" << "\n";
    aOS << "Model Type: " << ( m_modelType == KICAD3D_MODEL_TYPE::BOARD ? "Board" : "Component" ) << "\n";
    return aOS;
}


KICAD3D_INFO::KICAD3D_INFO() : m_modelType( KICAD3D_MODEL_TYPE::BOARD )
{
}