/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2022 Mikolaj Wielgus
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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

#include <sim/sim_library_ibis.h>
#include <sim/sim_library.h>
#include <sim/sim_library_spice.h>
#include <boost/algorithm/string/case_conv.hpp>
#include <wildcards_and_files_ext.h>
#include <macros.h>


std::unique_ptr<SIM_LIBRARY>
SIM_LIBRARY::Create( const wxString& aFilePath, bool aForceFullParse, REPORTER& aReporter,
                     const std::function<wxString( const wxString&, const wxString& )>& aResolver )
{
    std::unique_ptr<SIM_LIBRARY> library;

    if( aFilePath.EndsWith( FILEEXT::IbisFileExtension ) )
        library = std::make_unique<SIM_LIBRARY_IBIS>();
    else
        library = std::make_unique<SIM_LIBRARY_SPICE>( aForceFullParse );

    library->m_pathResolver = aResolver;
    library->ReadFile( aFilePath, aReporter );

    return library;
}


void SIM_LIBRARY::ReadFile( const wxString& aFilePath, REPORTER& aReporter )
{
    m_filePath = aFilePath;
}


SIM_MODEL* SIM_LIBRARY::FindModel( const std::string& aModelName ) const
{
    std::string lowerName = boost::to_lower_copy( aModelName );

    for( int i = 0; i < static_cast<int>( m_modelNames.size() ); ++i )
    {
        if( boost::to_lower_copy( m_modelNames.at( i ) ) == lowerName )
            return m_models.at( i ).get();
    }

    return nullptr;
}


std::vector<SIM_LIBRARY::MODEL> SIM_LIBRARY::GetModels() const
{
    std::vector<MODEL> result;

    for( int i = 0; i < static_cast<int>( m_modelNames.size() ); ++i )
        result.push_back( { m_modelNames.at( i ), *m_models.at( i ) } );

    return result;
}
