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

#pragma once

#include <TDF_Attribute.hxx>
#include <TDF_Label.hxx>
#include <Standard_GUID.hxx>
#include <TDF_Data.hxx>


enum class KICAD3D_MODEL_TYPE
{
    BOARD,
    COMPONENT
};


/**
 * This is an internal KiCad use attribute to add additional markup to a opencascade
 * document for internal processing
 */
class KICAD3D_INFO : public TDF_Attribute
{
public:
    /**
     * Get the GUID of this attribute
     */
    static const Standard_GUID& GetID();

    /**
     * Finds or creates the attribute attached to <theLabel>.
     * The found or created attribute is returned
     */
    static Handle( KICAD3D_INFO ) Set( const TDF_Label& aLabel,
                                       KICAD3D_MODEL_TYPE aModelType,
                                       std::string aDisplayName = "" );

public:
    KICAD3D_INFO();

    KICAD3D_MODEL_TYPE GetModelType() const { return m_modelType; }
    void               SetModelType( const KICAD3D_MODEL_TYPE& aModelType ) { m_modelType = aModelType; }

    const std::string& GetDisplayName() const { return m_displayName; }
    void               SetDisplayName( const std::string& aName ) { m_displayName = aName; }

    //Overridden methods from TDF_Attribute
    const Standard_GUID& ID() const override;
    void Restore( const Handle( TDF_Attribute ) & aAttribute ) override;
    Handle( TDF_Attribute ) NewEmpty() const override;
    void Paste( const Handle( TDF_Attribute ) & aAttribute,
                const Handle( TDF_RelocationTable ) & aRelocationTable ) const override;
    Standard_OStream& Dump( Standard_OStream& aOS ) const override;

private:
    KICAD3D_MODEL_TYPE m_modelType;
    std::string        m_displayName;
};