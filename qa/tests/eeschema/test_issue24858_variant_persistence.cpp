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
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/**
 * Regression tests for issue 24858, deleted design variants reappear after reload.
 *
 * The failure chain is:
 *  - attribute setters create an instance variant entry even when the value
 *    being set equals the base symbol value
 *  - the writer serializes such diff-less entries as bare (variant (name "X"))
 *    blocks
 *  - on load the variant list is rebuilt from any instance entry found, so the
 *    blocks resurrect the variant
 */

#include <qa_utils/wx_utils/unit_test_utils.h>

#include <memory>

#include <lib_symbol.h>
#include <sch_pin.h>
#include <sch_symbol.h>
#include <sch_screen.h>
#include <sch_sheet.h>
#include <sch_sheet_path.h>
#include <schematic.h>
#include <settings/settings_manager.h>
#include <sch_io/kicad_sexpr/sch_io_kicad_sexpr.h>
#include <locale_io.h>

#include <wx/file.h>
#include <wx/filename.h>
#include <wx/stdpaths.h>


struct ISSUE24858_FIXTURE
{
    ISSUE24858_FIXTURE()
    {
        wxString tempDir = wxStandardPaths::Get().GetTempDir();
        wxString projectPath = tempDir + wxFileName::GetPathSeparator() + wxT( "test_issue24858.kicad_pro" );
        m_tempFiles.push_back( projectPath );

        m_settingsManager.LoadProject( projectPath.ToStdString() );
        m_schematic = std::make_unique<SCHEMATIC>( nullptr );
        m_schematic->SetProject( &m_settingsManager.Prj() );
    }

    ~ISSUE24858_FIXTURE()
    {
        for( const wxString& file : m_tempFiles )
        {
            if( wxFileExists( file ) )
                wxRemoveFile( file );
        }

        m_schematic.reset();
    }

    wxString tempFile()
    {
        wxString dir = wxStandardPaths::Get().GetTempDir();
        wxString name = wxFileName::CreateTempFileName( dir + wxFileName::GetPathSeparator() + "issue24858" );
        name += ".kicad_sch";
        m_tempFiles.push_back( name );
        return name;
    }

    /// Place a one-pin symbol on a fresh top sheet and set m_sheet, m_path and m_symbol.
    void placeSymbol()
    {
        m_schematic->CreateDefaultScreens();

        m_sheet = m_schematic->GetTopLevelSheets()[0];
        SCH_SCREEN* screen = m_sheet->GetScreen();
        screen->SetFileName( "issue24858.kicad_sch" );

        m_path.clear();
        m_path.push_back( m_sheet );

        m_libSym = std::make_unique<LIB_SYMBOL>( "R", nullptr );

        SCH_PIN* pin = new SCH_PIN( m_libSym.get() );
        pin->SetNumber( "1" );
        pin->SetType( ELECTRICAL_PINTYPE::PT_PASSIVE );
        pin->SetPosition( VECTOR2I( -508000, 0 ) );
        m_libSym->AddDrawItem( pin );

        m_symbol = new SCH_SYMBOL( *m_libSym, m_libSym->GetLibId(), &m_path, 0, 0, VECTOR2I( 15621000, 6223000 ) );
        m_symbol->UpdatePins();

        screen->Append( m_symbol );
    }

    wxString saveAndRead()
    {
        wxString fileName = tempFile();

        SCH_IO_KICAD_SEXPR io;
        io.SaveSchematicFile( fileName, m_sheet, m_schematic.get() );

        wxFile   f( fileName );
        wxString contents;
        f.ReadAll( &contents );

        m_savedFileName = fileName;

        return contents;
    }

    SCH_SYMBOL* reload()
    {
        SCH_IO_KICAD_SEXPR io;

        m_schematic->Reset();
        SCH_SHEET* defaultSheet = m_schematic->GetTopLevelSheet( 0 );
        SCH_SHEET* loaded = io.LoadSchematicFile( m_savedFileName, m_schematic.get() );
        m_schematic->AddTopLevelSheet( loaded );
        m_schematic->RemoveTopLevelSheet( defaultSheet );
        delete defaultSheet;

        m_sheet = loaded;

        SCH_SYMBOL* found = nullptr;

        for( SCH_ITEM* item : loaded->GetScreen()->Items().OfType( SCH_SYMBOL_T ) )
            found = static_cast<SCH_SYMBOL*>( item );

        return found;
    }

    SETTINGS_MANAGER            m_settingsManager;
    std::unique_ptr<SCHEMATIC>  m_schematic;
    std::unique_ptr<LIB_SYMBOL> m_libSym;
    SCH_SHEET*                  m_sheet = nullptr;
    SCH_SHEET_PATH              m_path;
    SCH_SYMBOL*                 m_symbol = nullptr;
    wxString                    m_savedFileName;
    std::vector<wxString>       m_tempFiles;
};


BOOST_FIXTURE_TEST_SUITE( Issue24858VariantPersistence, ISSUE24858_FIXTURE )


/**
 * Setting a variant attribute to the value it already resolves to must not create an
 * instance variant entry.  This is what the symbol fields table apply pass does for
 * every symbol, and it is the source of the phantom (variant (name "X")) blocks.
 */
BOOST_AUTO_TEST_CASE( BaseValueSetCreatesNoEntry )
{
    placeSymbol();

    wxString variantName = wxS( "Ghost" );

    m_symbol->SetDNP( m_symbol->GetDNP(), &m_path, variantName );
    m_symbol->SetExcludedFromSim( m_symbol->GetExcludedFromSim(), &m_path, variantName );
    m_symbol->SetExcludedFromBOM( m_symbol->GetExcludedFromBOM(), &m_path, variantName );
    m_symbol->SetExcludedFromPosFiles( m_symbol->GetExcludedFromPosFiles(), &m_path, variantName );

    BOOST_CHECK( !m_symbol->GetVariant( m_path, variantName ).has_value() );
}


/**
 * An instance variant entry with no differences from the base symbol must not be
 * serialized.  Diff-less blocks in the file are what resurrect deleted variants
 * on the next load.
 */
BOOST_AUTO_TEST_CASE( DiffLessEntryNotSerialized )
{
    placeSymbol();

    SCH_SYMBOL_VARIANT ghost( wxS( "Ghost" ) );
    ghost.InitializeAttributes( *m_symbol );
    m_symbol->AddVariant( m_path, ghost );

    wxString contents = saveAndRead();

    BOOST_CHECK( !contents.Contains( wxS( "(variant" ) ) );
}


/**
 * Positive control, an entry with a real differential must still be serialized.
 */
BOOST_AUTO_TEST_CASE( RealEntryStillSerialized )
{
    placeSymbol();

    SCH_SYMBOL_VARIANT loaded( wxS( "Loaded" ) );
    loaded.InitializeAttributes( *m_symbol );
    loaded.m_DNP = !m_symbol->GetDNP();
    m_symbol->AddVariant( m_path, loaded );

    wxString contents = saveAndRead();

    BOOST_CHECK( contents.Contains( wxS( "(variant" ) ) );
    BOOST_CHECK( contents.Contains( wxS( "(name \"Loaded\")" ) ) );
    BOOST_CHECK( contents.Contains( wxS( "dnp" ) ) );
}


/**
 * The issue 24858 flow.  A variant is deleted, then an apply pass writes the resolved
 * (now base) attribute values back with the deleted variant name.  After save and
 * reload no trace of the variant may remain.
 */
BOOST_AUTO_TEST_CASE( DeletedVariantStaysDeleted )
{
    placeSymbol();

    wxString variantName = wxS( "Ghost" );

    // Real override, then the user deletes the variant.
    m_symbol->SetDNP( !m_symbol->GetDNP(), &m_path, variantName );
    BOOST_REQUIRE( m_symbol->GetVariant( m_path, variantName ).has_value() );

    m_symbol->DeleteVariant( m_path, variantName );
    BOOST_REQUIRE( !m_symbol->GetVariant( m_path, variantName ).has_value() );

    // A later apply pass writes back the resolved value, which is now the base value.
    m_symbol->SetDNP( m_symbol->GetDNP( &m_path, variantName ), &m_path, variantName );

    saveAndRead();

    SCH_SYMBOL* reloaded = reload();
    BOOST_REQUIRE( reloaded );

    for( const SCH_SYMBOL_INSTANCE& instance : reloaded->GetInstances() )
        BOOST_CHECK( !instance.m_Variants.contains( variantName ) );

    std::set<wxString> variantNames = m_sheet->GetScreen()->GetVariantNames();
    BOOST_CHECK( !variantNames.contains( variantName ) );
}


BOOST_AUTO_TEST_SUITE_END()
