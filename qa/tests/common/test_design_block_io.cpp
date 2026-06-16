/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <cstdint>
#include <cstdlib>
#include <cstring>

#include <wx/arrstr.h>
#include <wx/buffer.h>
#include <wx/translation.h>

#include <qa_utils/wx_utils/unit_test_utils.h>

#include <design_block_io.h>


namespace
{

// The Finnish/Arabic/Tamil/Thai catalogs that triggered issue 24659 translate the literal "KiCad".
const char* const ORIG_STRING = "KiCad";
const char* const TRANSLATED_STRING = "KiCad (ohjelma)";


/**
 * Build a minimal GNU gettext .mo image in memory that maps a single original string to a single
 * translation. The layout follows the documented MO format: a header, two string-descriptor
 * tables (original and translation), then the NUL-terminated string data.
 */
wxScopedCharBuffer MakeSingleEntryMo( const char* aOrig, const char* aTranslated )
{
    const uint32_t origLen = static_cast<uint32_t>( std::strlen( aOrig ) );
    const uint32_t transLen = static_cast<uint32_t>( std::strlen( aTranslated ) );

    const uint32_t headerSize = 28;
    const uint32_t numStrings = 1;
    const uint32_t origTableOffset = headerSize;
    const uint32_t transTableOffset = origTableOffset + numStrings * 8;
    const uint32_t origDataOffset = transTableOffset + numStrings * 8;
    const uint32_t transDataOffset = origDataOffset + origLen + 1;
    const uint32_t totalSize = transDataOffset + transLen + 1;

    // wxScopedCharBuffer::CreateOwned releases with free(), so the block must come from malloc().
    char* buf = static_cast<char*>( std::malloc( totalSize ) );
    std::memset( buf, 0, totalSize );

    auto put32 = [&]( uint32_t aOffset, uint32_t aValue )
    {
        std::memcpy( buf + aOffset, &aValue, sizeof( aValue ) );
    };

    put32( 0, 0x950412de ); // magic, native byte order
    put32( 4, 0 );          // revision
    put32( 8, numStrings );
    put32( 12, origTableOffset );
    put32( 16, transTableOffset );

    put32( origTableOffset, origLen );
    put32( origTableOffset + 4, origDataOffset );
    put32( transTableOffset, transLen );
    put32( transTableOffset + 4, transDataOffset );

    std::memcpy( buf + origDataOffset, aOrig, origLen );
    std::memcpy( buf + transDataOffset, aTranslated, transLen );

    return wxScopedCharBuffer::CreateOwned( buf, totalSize );
}


/**
 * A translations loader that always serves the in-memory single-entry catalog, regardless of which
 * domain or language wxTranslations asks for. Installing it lets the test drive _() to actually
 * translate "KiCad", which is what makes this a real regression test for issue 24659.
 */
class IN_MEMORY_LOADER : public wxTranslationsLoader
{
public:
    wxMsgCatalog* LoadCatalog( const wxString& aDomain, const wxString& aLang ) override
    {
        return wxMsgCatalog::CreateFromData( MakeSingleEntryMo( ORIG_STRING, TRANSLATED_STRING ),
                                             aDomain );
    }

    wxArrayString GetAvailableTranslations( const wxString& aDomain ) const override
    {
        wxArrayString langs;
        langs.Add( wxT( "fi" ) );
        return langs;
    }
};

} // namespace


/**
 * Installs a wxTranslations instance that actually translates "KiCad". wxTranslations::Set both
 * takes ownership of the new object and deletes the previous one. Teardown restores a fresh
 * catalog-less wxTranslations rather than a saved pointer, which both reproduces qa_common's
 * default no-translation state for subsequent tests and avoids double-freeing a saved pointer.
 */
struct TRANSLATED_KICAD_FIXTURE
{
    TRANSLATED_KICAD_FIXTURE()
    {
        wxTranslations* trans = new wxTranslations();
        trans->SetLoader( new IN_MEMORY_LOADER() );
        wxTranslations::Set( trans );
        trans->AddCatalog( wxT( "kicad" ) );

        m_active = trans->GetTranslatedString( ORIG_STRING ) != nullptr;
    }

    ~TRANSLATED_KICAD_FIXTURE() { wxTranslations::Set( new wxTranslations() ); }

    bool m_active = false;
};


BOOST_FIXTURE_TEST_SUITE( DesignBlockIo, TRANSLATED_KICAD_FIXTURE )


/**
 * The type token serialized into the design-block-lib-table must be the ASCII literal "KiCad",
 * never a translated string. Locales that translate the product name (Finnish, Arabic, Tamil,
 * Thai) otherwise write a table that no other locale can parse (issue 24659). With a catalog that
 * translates "KiCad" active, ShowType must still emit the bare token; if the production code wraps
 * it in _() this assertion fails.
 */
BOOST_AUTO_TEST_CASE( ShowTypeIsLocaleIndependent )
{
    BOOST_REQUIRE( m_active );
    BOOST_REQUIRE_EQUAL( wxGetTranslation( wxString::FromUTF8( ORIG_STRING ) ),
                         wxString::FromUTF8( TRANSLATED_STRING ) );

    BOOST_CHECK_EQUAL( DESIGN_BLOCK_IO_MGR::ShowType( DESIGN_BLOCK_IO_MGR::KICAD_SEXP ),
                       wxString( wxT( "KiCad" ) ) );
}


/**
 * EnumFromStr must map the ShowType output, and the bare ASCII token, back to KICAD_SEXP even with
 * a translating catalog active. This is the load half of the table portability contract.
 */
BOOST_AUTO_TEST_CASE( EnumFromStrRoundTrips )
{
    wxString token = DESIGN_BLOCK_IO_MGR::ShowType( DESIGN_BLOCK_IO_MGR::KICAD_SEXP );

    BOOST_CHECK_EQUAL( static_cast<int>( DESIGN_BLOCK_IO_MGR::EnumFromStr( token ) ),
                       static_cast<int>( DESIGN_BLOCK_IO_MGR::KICAD_SEXP ) );

    BOOST_CHECK_EQUAL( static_cast<int>( DESIGN_BLOCK_IO_MGR::EnumFromStr( wxT( "KiCad" ) ) ),
                       static_cast<int>( DESIGN_BLOCK_IO_MGR::KICAD_SEXP ) );
}


/**
 * Backward compatibility: a table already corrupted by an older release that wrote the translated
 * token must still load rather than report an unknown library type.
 */
BOOST_AUTO_TEST_CASE( EnumFromStrAcceptsLegacyTranslatedToken )
{
    BOOST_REQUIRE( m_active );

    BOOST_CHECK_EQUAL( static_cast<int>(
                               DESIGN_BLOCK_IO_MGR::EnumFromStr( wxString::FromUTF8( TRANSLATED_STRING ) ) ),
                       static_cast<int>( DESIGN_BLOCK_IO_MGR::KICAD_SEXP ) );
}


BOOST_AUTO_TEST_SUITE_END()
