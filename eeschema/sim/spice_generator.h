/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2022 Mikolaj Wielgus
 * Copyright (C) 2022 KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 3
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you may find one here:
 * https://www.gnu.org/licenses/gpl-3.0.html
 * or you may search the http://www.gnu.org website for the version 3 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#ifndef SPICE_GENERATOR_H
#define SPICE_GENERATOR_H

#include <sim/sim_model.h>


class SPICE_GENERATOR
{
public:
    SPICE_GENERATOR( const SIM_MODEL& aModel ) : m_model( aModel ) {}

    virtual wxString ModelLine( const wxString& aModelName ) const;

    wxString ItemLine( const wxString& aRefName,
                       const wxString& aModelName ) const;
    wxString ItemLine( const wxString& aRefName,
                       const wxString& aModelName,
                       const std::vector<wxString>& aSymbolPinNumbers ) const;
    virtual wxString ItemLine( const wxString& aRefName,
                               const wxString& aModelName,
                               const std::vector<wxString>& aSymbolPinNumbers,
                               const std::vector<wxString>& aPinNetNames ) const;
    virtual wxString ItemName( const wxString& aRefName ) const;
    virtual wxString ItemPins( const wxString& aRefName,
                               const wxString& aModelName,
                               const std::vector<wxString>& aSymbolPinNumbers,
                               const std::vector<wxString>& aPinNetNames ) const;
    virtual wxString ItemModelName( const wxString& aModelName ) const;
    virtual wxString ItemParams() const;

    virtual wxString TuningLine( const wxString& aSymbol ) const;
    
    virtual std::vector<wxString> CurrentNames( const wxString& aRefName ) const;

    virtual wxString Preview( const wxString& aModelName ) const;

protected:
    virtual std::vector<std::reference_wrapper<const SIM_MODEL::PIN>> GetPins() const
    {
        return m_model.GetPins();
    }

    std::vector<std::reference_wrapper<const SIM_MODEL::PARAM>> GetInstanceParams() const;

    const SIM_MODEL& m_model;
};

#endif // SPICE_GENERATOR_H
