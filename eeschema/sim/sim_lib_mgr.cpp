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

#include <sim/sim_lib_mgr.h>
#include <sch_symbol.h>
#include <sim/sim_library.h>
#include <sim/sim_model.h>
#include <pgm_base.h>


SIM_LIB_MGR::SIM_LIB_MGR( const PROJECT& aPrj ) : m_project( aPrj )
{
}


SIM_MODEL& SIM_LIB_MGR::CreateModel( SCH_SYMBOL& aSymbol )
{
    std::vector<LIB_PIN*> pins = aSymbol.GetLibPins();
    SCH_FIELD* libraryField = aSymbol.FindField( SIM_LIBRARY::LIBRARY_FIELD );

    if( libraryField )
    {
        wxString path = libraryField->GetShownText();
        wxString absolutePath = m_project.AbsolutePath( path );
        SIM_LIBRARY* library = nullptr;

        try
        {
            auto it = m_libraries.try_emplace( std::string( path.ToUTF8() ),
                    SIM_LIBRARY::Create( std::string( absolutePath.ToUTF8() ) ) ).first;
            library = &*it->second;
        }
        catch( const IO_ERROR& e )
        {
            THROW_IO_ERROR(
                    wxString::Format( _( "Error loading simulation model library '%s'.\n%s" ),
                                      absolutePath,
                                      e.What() ) );
        }

        SCH_FIELD* nameField = aSymbol.FindField( SIM_LIBRARY::NAME_FIELD );

        if( !nameField )
        {
            THROW_IO_ERROR( wxString::Format( _( "Error loading simulation model: no '%s' field" ),
                                              SIM_LIBRARY::NAME_FIELD ) );
        }

        std::string baseModelName = std::string( nameField->GetShownText().ToUTF8() );
        SIM_MODEL* baseModel = library->FindModel( baseModelName );

        if( !baseModel )
        {
            THROW_IO_ERROR(
                    wxString::Format( _( "Error loading simulation model: could not find base model '%s' in library '%s'" ),
                                      baseModelName,
                                      absolutePath ) );
        }

        m_models.push_back( SIM_MODEL::Create( *baseModel,
                                               static_cast<int>( pins.size() ),
                                               aSymbol.GetFields() ) );
    }
    else
    {
        m_models.push_back( SIM_MODEL::Create( static_cast<int>( pins.size() ),
                                               aSymbol.GetFields() ) );
    }

    return *m_models.back();
}
