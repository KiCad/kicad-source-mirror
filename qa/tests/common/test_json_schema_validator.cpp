/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 3 of the License, or (at your
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

#include <boost/test/unit_test.hpp>

#include <json_schema_validator.h>
#include <remote_provider_utils.h>

#include <atomic>

#if defined( __linux__ ) && defined( __GLIBCXX__ )
#include <cxxabi.h>
#include <dlfcn.h>
#define KI_COUNT_THROWS 1
#endif


namespace
{

#ifdef KI_COUNT_THROWS
std::atomic<bool> g_countThrows{ false };
std::atomic<int>  g_throwCount{ 0 };
#endif


/**
 * Counts C++ throws raised on any thread while it is alive.
 */
class THROW_COUNTER
{
public:
    THROW_COUNTER()
    {
#ifdef KI_COUNT_THROWS
        g_throwCount.store( 0, std::memory_order_relaxed );
        g_countThrows.store( true, std::memory_order_relaxed );
#endif
    }

    ~THROW_COUNTER()
    {
#ifdef KI_COUNT_THROWS
        g_countThrows.store( false, std::memory_order_relaxed );
#endif
    }

    /// Number of throws seen so far, or -1 where the platform cannot be instrumented.
    int Count() const
    {
#ifdef KI_COUNT_THROWS
        return g_throwCount.load( std::memory_order_relaxed );
#else
        return -1;
#endif
    }

    static bool Supported()
    {
#ifdef KI_COUNT_THROWS
        return true;
#else
        return false;
#endif
    }
};


wxFileName schemaPath( const wxString& aName )
{
    wxFileName schemaFile = wxFileName::DirName( wxString::FromUTF8( QA_SRC_ROOT ) );
    schemaFile.AppendDir( wxS( "kicad" ) );
    schemaFile.AppendDir( wxS( "pcm" ) );
    schemaFile.AppendDir( wxS( "schemas" ) );
    schemaFile.SetFullName( aName );
    return schemaFile;
}


nlohmann::json validPackage()
{
    return nlohmann::json{
        { "name", "Test Package" },
        { "description", "short description" },
        { "description_full", "a longer description" },
        { "identifier", "com.example.test" },
        { "type", "plugin" },
        { "author", { { "name", "Someone" },
                      { "contact", { { "web", "https://example.test" } } } } },
        { "license", "MIT" },
        { "resources", { { "homepage", "https://example.test" } } },
        { "versions", nlohmann::json::array( { { { "version", "1.0" },
                                                 { "status", "stable" },
                                                 { "kicad_version", "6.0" } } } ) }
    };
}

} // namespace


#ifdef KI_COUNT_THROWS
// Interposes the ABI throw entry point, so keep counting scoped to the case that needs it.
// Defined in __cxxabiv1 to match the <cxxabi.h> declaration exactly.  Declaring it at global
// scope instead collides with the void*-typed forward declaration in <ext/concurrence.h>,
// which libstdc++ pulls in on one compiler but not the other.
namespace __cxxabiv1
{
extern "C" void __cxa_throw( void* aException, std::type_info* aTypeInfo,
                             void ( *aDestructor )( void* ) )
{
    if( g_countThrows.load( std::memory_order_relaxed ) )
        g_throwCount.fetch_add( 1, std::memory_order_relaxed );

    using THROW_FN = void ( * )( void*, std::type_info*, void ( * )( void* ) );
    static THROW_FN real = reinterpret_cast<THROW_FN>( dlsym( RTLD_NEXT, "__cxa_throw" ) );

    real( aException, aTypeInfo, aDestructor );
    __builtin_unreachable();
}
} // namespace __cxxabiv1
#endif


BOOST_AUTO_TEST_SUITE( JsonSchemaValidator )


/**
 * CreatePCM() builds these on every project open, where a throw is fatal with the CLR loaded.
 */
BOOST_AUTO_TEST_CASE( PcmSchemasLoadWithoutThrowing )
{
    for( const wxString& name : { wxS( "pcm.v1.schema.json" ), wxS( "pcm.v2.schema.json" ) } )
    {
        wxFileName schema = schemaPath( name );
        BOOST_REQUIRE_MESSAGE( schema.FileExists(), schema.GetFullPath() );

        THROW_COUNTER counter;
        JSON_SCHEMA_VALIDATOR validator( schema );

        if( THROW_COUNTER::Supported() )
        {
            BOOST_CHECK_MESSAGE( counter.Count() == 0,
                                 name << " raised " << counter.Count()
                                      << " exception(s) while building the schema" );
        }
    }
}


/**
 * Package reaches Contact through $ref, so the verdict only lands if that reference resolved.
 */
BOOST_AUTO_TEST_CASE( PcmPackageRefsResolve )
{
    JSON_SCHEMA_VALIDATOR         validator( schemaPath( wxS( "pcm.v1.schema.json" ) ) );
    nlohmann::json_uri            uri( "#/definitions/Package" );
    COLLECTING_JSON_ERROR_HANDLER good;

    validator.Validate( validPackage(), good, uri );
    BOOST_CHECK( !good.HasErrors() );

    // "contact" is required by the Contact definition, which Package only sees through a $ref
    nlohmann::json bad = validPackage();
    bad["author"].erase( "contact" );

    COLLECTING_JSON_ERROR_HANDLER missingContact;
    validator.Validate( bad, missingContact, uri );
    BOOST_CHECK( missingContact.HasErrors() );
}


/**
 * Draft 2019-09 renamed "definitions" to "$defs", so a schema part way through that migration
 * carries both and either name has to work as an initial URI.
 */
BOOST_AUTO_TEST_CASE( BothDefinitionContainersResolve )
{
    nlohmann::json schema = nlohmann::json::parse( R"({
        "$schema": "http://json-schema.org/draft-07/schema#",
        "$defs":       { "Modern": { "type": "integer" } },
        "definitions": { "Legacy": { "type": "integer" } }
    })" );

    JSON_SCHEMA_VALIDATOR validator( schema );

    for( const std::string& fragment : { "#/$defs/Modern", "#/definitions/Legacy" } )
    {
        nlohmann::json_uri uri( fragment );

        COLLECTING_JSON_ERROR_HANDLER resolved;
        validator.Validate( 42, resolved, uri );
        BOOST_CHECK_MESSAGE( !resolved.HasErrors(),
                             fragment << " did not resolve: " << resolved.FirstError() );

        COLLECTING_JSON_ERROR_HANDLER enforced;
        validator.Validate( "not an integer", enforced, uri );
        BOOST_CHECK_MESSAGE( enforced.HasErrors(), fragment << " did not enforce its type" );
    }
}


BOOST_AUTO_TEST_SUITE_END()
