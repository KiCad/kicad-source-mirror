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

#include <sim/sim_library.h>
#include <sim/sim_library_spice.h>


std::unique_ptr<SIM_LIBRARY> SIM_LIBRARY::Create( wxString aFilePath )
{
    std::unique_ptr<SIM_LIBRARY> library = std::make_unique<SIM_LIBRARY_SPICE>();

    library->ReadFile( aFilePath );
    return library;
}


void SIM_LIBRARY::ReadFile( const wxString& aFilePath )
{
    m_filePath = aFilePath;
}


SIM_MODEL* SIM_LIBRARY::FindModel( const wxString& aModelName ) const
{
    for( unsigned i = 0; i < GetModelNames().size(); ++i )
    {
        wxString curModelName = GetModelNames().at( i );

        if( curModelName == aModelName )
            return m_models.at( i ).get();
    }

    return nullptr;
}


std::vector<std::reference_wrapper<SIM_MODEL>> SIM_LIBRARY::GetModels() const
{
    std::vector<std::reference_wrapper<SIM_MODEL>> ret;

    for( const std::unique_ptr<SIM_MODEL>& model : m_models )
        ret.emplace_back( *model );

    return ret;
}
