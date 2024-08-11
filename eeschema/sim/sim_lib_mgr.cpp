/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2022 Mikolaj Wielgus
 * Copyright (C) 2022-2023 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <pgm_base.h>
#include <string>
#include <string_utils.h>
#include <common.h>
#include <functional>
#include <sch_symbol.h>

// Include simulator headers after wxWidgets headers to avoid conflicts with Windows headers
// (especially on msys2 + wxWidgets 3.0.x)
#include <sim/sim_lib_mgr.h>
#include <sim/sim_library.h>
#include <sim/sim_model.h>
#include <sim/sim_model_ideal.h>

using namespace std::placeholders;


SIM_LIB_MGR::SIM_LIB_MGR( const PROJECT* aPrj ) :
        m_project( aPrj ),
        m_forceFullParse( false )
{
}


void SIM_LIB_MGR::Clear()
{
    m_libraries.clear();
    m_models.clear();
}


wxString SIM_LIB_MGR::ResolveLibraryPath( const wxString& aLibraryPath, const PROJECT* aProject,
                                          REPORTER& aReporter )
{
    wxString expandedPath = ExpandEnvVarSubstitutions( aLibraryPath, aProject );

    // Convert it to UNIX format
    expandedPath.Replace( '\\', '/' );

    wxFileName fn( expandedPath );

    if( fn.IsAbsolute() )
        return fn.GetFullPath();

    wxFileName projectFn( aProject ? aProject->AbsolutePath( expandedPath ) : expandedPath );

    if( projectFn.Exists() )
        return projectFn.GetFullPath();

    wxFileName spiceLibFn( expandedPath );
    wxString   spiceLibDir;

    wxGetEnv( wxT( "SPICE_LIB_DIR" ), &spiceLibDir );

    if( !spiceLibDir.IsEmpty() && spiceLibFn.MakeAbsolute( spiceLibDir ) && spiceLibFn.Exists() )
        return spiceLibFn.GetFullPath();

    if( spiceLibDir.IsEmpty() || spiceLibFn.GetFullPath() == projectFn.GetFullPath() )
    {
        aReporter.Report( wxString::Format( _( "Simulation model library not found at '%s'" ),
                                            projectFn.GetFullPath() ) );
    }
    else
    {
        aReporter.Report( wxString::Format( _( "Simulation model library not found at '%s' or '%s'" ),
                                            projectFn.GetFullPath(),
                                            spiceLibFn.GetFullPath() ) );
    }

    return aLibraryPath;
}


wxString SIM_LIB_MGR::ResolveEmbeddedLibraryPath( const wxString& aLibPath,
                                                  const wxString& aRelativeLib,
                                                  REPORTER& aReporter )
{
    wxFileName testPath( aLibPath );
    wxString fullPath( aLibPath );

    if( !testPath.IsAbsolute() && !aRelativeLib.empty() )
    {
        wxString relLib( aRelativeLib );

        relLib = ResolveLibraryPath( relLib, m_project, aReporter );

        wxFileName fn( relLib );

        testPath.MakeAbsolute( fn.GetPath( true ) );
        fullPath = testPath.GetFullPath();
    }

    wxFileName fn( fullPath );

    if( !fn.Exists() )
        fullPath = aLibPath;

    fullPath = ResolveLibraryPath( fullPath, m_project, aReporter );

    return fullPath;
}


void SIM_LIB_MGR::SetLibrary( const wxString& aLibraryPath, REPORTER& aReporter )
{
    wxString path = ResolveLibraryPath( aLibraryPath, m_project, aReporter );

    if( aReporter.HasMessage() )
        return;

    if( !wxFileName::Exists( path ) )
    {
        aReporter.Report( wxString::Format( _( "Simulation model library not found at '%s'" ),
                                            path ) );
        return;
    }

    std::unique_ptr<SIM_LIBRARY> library = SIM_LIBRARY::Create( path, m_forceFullParse, aReporter,
            [&]( const wxString& libPath, const wxString& relativeLib ) -> wxString
            {
                return ResolveEmbeddedLibraryPath( libPath, relativeLib, aReporter );
            } );

    Clear();
    m_libraries[path] = std::move( library );
}


SIM_MODEL& SIM_LIB_MGR::CreateModel( SIM_MODEL::TYPE aType, const std::vector<LIB_PIN*>& aPins,
                                     REPORTER& aReporter )
{
    m_models.push_back( SIM_MODEL::Create( aType, aPins, aReporter ) );
    return *m_models.back();
}


SIM_MODEL& SIM_LIB_MGR::CreateModel( const SIM_MODEL* aBaseModel,
                                     const std::vector<LIB_PIN*>& aPins, REPORTER& aReporter )
{
    m_models.push_back( SIM_MODEL::Create( aBaseModel, aPins, aReporter ) );
    return *m_models.back();
}


template <typename T>
SIM_MODEL& SIM_LIB_MGR::CreateModel( const SIM_MODEL* aBaseModel,
                                     const std::vector<LIB_PIN*>& aPins,
                                     const std::vector<T>& aFields, REPORTER& aReporter )
{
    m_models.push_back( SIM_MODEL::Create( aBaseModel, aPins, aFields, aReporter ) );
    return *m_models.back();
}

template SIM_MODEL& SIM_LIB_MGR::CreateModel( const SIM_MODEL* aBaseModel,
                                              const std::vector<LIB_PIN*>& aPins,
                                              const std::vector<SCH_FIELD>& aFields,
                                              REPORTER& aReporter );
template SIM_MODEL& SIM_LIB_MGR::CreateModel( const SIM_MODEL* aBaseModel,
                                              const std::vector<LIB_PIN*>& aPins,
                                              const std::vector<LIB_FIELD>& aFields,
                                              REPORTER& aReporter );


SIM_LIBRARY::MODEL SIM_LIB_MGR::CreateModel( const SCH_SHEET_PATH* aSheetPath, SCH_SYMBOL& aSymbol,
                                             REPORTER& aReporter )
{
    // Note: currently this creates a resolved model (all Kicad variables references are resolved
    // before building the model).
    //
    // That's not what we want if this is ever called from the Simulation Model Editor (or other
    // editors, but it is what we want if called to generate a netlist or other exported items.


    std::vector<SCH_FIELD> fields;

    for( const SCH_FIELD& field : aSymbol.GetFields() )
    {
        if( field.GetId() == REFERENCE_FIELD )
        {
            fields.emplace_back( VECTOR2I(), -1, &aSymbol, field.GetName() );
            fields.back().SetText( aSymbol.GetRef( aSheetPath ) );
        }
        else if( field.GetId() == VALUE_FIELD
                 || field.GetName().StartsWith( wxS( "Sim." ) ) )
        {
            fields.emplace_back( VECTOR2I(), -1, &aSymbol, field.GetName() );
            fields.back().SetText( field.GetShownText( aSheetPath, false ) );
        }
    }

    auto getOrCreateField =
            [&aSymbol, &fields]( const wxString& name ) -> SCH_FIELD*
            {
                for( SCH_FIELD& field : fields )
                {
                    if( field.GetName().IsSameAs( name ) )
                        return &field;
                }

                fields.emplace_back( &aSymbol, -1, name );
                return &fields.back();
            };

    wxString deviceType;
    wxString modelType;
    wxString modelParams;
    wxString pinMap;
    bool     storeInValue = false;

    // Infer RLC and VI models if they aren't specified
    if( SIM_MODEL::InferSimModel( aSymbol, &fields, true, SIM_VALUE_GRAMMAR::NOTATION::SI,
                                  &deviceType, &modelType, &modelParams, &pinMap ) )
    {
        getOrCreateField( SIM_DEVICE_FIELD )->SetText( deviceType );

        if( !modelType.IsEmpty() )
            getOrCreateField( SIM_DEVICE_SUBTYPE_FIELD )->SetText( modelType );

        getOrCreateField( SIM_PARAMS_FIELD )->SetText( modelParams );
        getOrCreateField( SIM_PINS_FIELD )->SetText( pinMap );

        storeInValue = true;
    }

    std::vector<LIB_PIN*> sourcePins = aSymbol.GetAllLibPins();

    std::sort( sourcePins.begin(), sourcePins.end(),
               []( const LIB_PIN* lhs, const LIB_PIN* rhs )
               {
                   return StrNumCmp( lhs->GetNumber(), rhs->GetNumber(), true ) < 0;
               } );

    SIM_LIBRARY::MODEL model = CreateModel( fields, sourcePins, true, aReporter );

    model.model.SetIsStoredInValue( storeInValue );

    return model;
}


template <typename T>
SIM_LIBRARY::MODEL SIM_LIB_MGR::CreateModel( const std::vector<T>& aFields,
                                             const std::vector<LIB_PIN*>& aPins, bool aResolved,
                                             REPORTER& aReporter )
{
    std::string libraryPath = SIM_MODEL::GetFieldValue( &aFields, SIM_LIBRARY::LIBRARY_FIELD );
    std::string baseModelName = SIM_MODEL::GetFieldValue( &aFields, SIM_LIBRARY::NAME_FIELD );

    if( libraryPath != "" )
    {
        return CreateModel( libraryPath, baseModelName, aFields, aPins, aReporter );
    }
    else
    {
        m_models.push_back( SIM_MODEL::Create( aFields, aPins, aResolved, aReporter ) );
        return { baseModelName, *m_models.back() };
    }
}

template SIM_LIBRARY::MODEL SIM_LIB_MGR::CreateModel( const std::vector<SCH_FIELD>& aFields,
                                                      const std::vector<LIB_PIN*>& aPins,
                                                      bool aResolved, REPORTER& aReporter );
template SIM_LIBRARY::MODEL SIM_LIB_MGR::CreateModel( const std::vector<LIB_FIELD>& aFields,
                                                      const std::vector<LIB_PIN*>& aPins,
                                                      bool aResolved, REPORTER& aReporter );


template <typename T>
SIM_LIBRARY::MODEL SIM_LIB_MGR::CreateModel( const wxString& aLibraryPath,
                                             const std::string& aBaseModelName,
                                             const std::vector<T>& aFields,
                                             const std::vector<LIB_PIN*>& aPins,
                                             REPORTER& aReporter )
{
    wxString     msg;
    SIM_LIBRARY* library = nullptr;
    SIM_MODEL*   baseModel = nullptr;
    std::string  modelName;
    wxString     path = ResolveLibraryPath( aLibraryPath, m_project, aReporter );

    auto it = m_libraries.find( path );

    if( it == m_libraries.end() )
    {
        it = m_libraries.emplace( path, SIM_LIBRARY::Create( path, m_forceFullParse, aReporter,
                [&]( const wxString& libPath, const wxString& relativeLib ) -> wxString
                {
                    return ResolveEmbeddedLibraryPath( libPath, relativeLib, aReporter );
                } ) ).first;
    }

    library = &*it->second;

    if( aBaseModelName == "" )
    {
        msg.Printf( _( "Error loading simulation model: no '%s' field" ),
                    SIM_LIBRARY::NAME_FIELD );

        aReporter.Report( msg, RPT_SEVERITY_ERROR );

        modelName = _( "unknown" ).ToStdString();
    }
    else if( library )
    {
        baseModel = library->FindModel( aBaseModelName );
        modelName = aBaseModelName;

        if( !baseModel )
        {
            msg.Printf( _( "Error loading simulation model: could not find base model '%s' "
                           "in library '%s'" ),
                        aBaseModelName,
                        path );

            aReporter.Report( msg, RPT_SEVERITY_ERROR );
        }
    }

    m_models.push_back( SIM_MODEL::Create( baseModel, aPins, aFields, aReporter ) );

    return { modelName, *m_models.back() };
}


void SIM_LIB_MGR::SetModel( int aIndex, std::unique_ptr<SIM_MODEL> aModel )
{
    m_models.at( aIndex ) = std::move( aModel );
}


std::map<wxString, std::reference_wrapper<const SIM_LIBRARY>> SIM_LIB_MGR::GetLibraries() const
{
    std::map<wxString, std::reference_wrapper<const SIM_LIBRARY>> libraries;

    for( auto& [path, library] : m_libraries )
        libraries.try_emplace( path, *library );

    return libraries;
}


std::vector<std::reference_wrapper<SIM_MODEL>> SIM_LIB_MGR::GetModels() const
{
    std::vector<std::reference_wrapper<SIM_MODEL>> models;

    for( const std::unique_ptr<SIM_MODEL>& model : m_models )
        models.emplace_back( *model );

    return models;
}
