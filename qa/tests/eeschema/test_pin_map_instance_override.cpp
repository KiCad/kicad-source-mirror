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

#include <qa_utils/wx_utils/unit_test_utils.h>

#include <memory>

#include <pin_map.h>
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


struct PIN_MAP_OVERRIDE_FIXTURE
{
    PIN_MAP_OVERRIDE_FIXTURE()
    {
        wxString tempDir = wxStandardPaths::Get().GetTempDir();
        wxString projectPath = tempDir + wxFileName::GetPathSeparator()
                               + wxT( "test_pinmap_ovr.kicad_pro" );
        m_tempFiles.push_back( projectPath );

        m_settingsManager.LoadProject( projectPath.ToStdString() );
        m_schematic = std::make_unique<SCHEMATIC>( nullptr );
        m_schematic->SetProject( &m_settingsManager.Prj() );
    }

    ~PIN_MAP_OVERRIDE_FIXTURE()
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
        wxString name = wxFileName::CreateTempFileName( dir + wxFileName::GetPathSeparator()
                                                        + "pin_map_override" );
        name += ".kicad_sch";
        m_tempFiles.push_back( name );
        return name;
    }

    /// Place a one-pin symbol carrying @a aOverride on a fresh top sheet and return the sheet.
    SCH_SHEET* placeSymbol( const PIN_MAP_INSTANCE_OVERRIDE& aOverride )
    {
        m_schematic->CreateDefaultScreens();

        SCH_SHEET*  sheet = m_schematic->GetTopLevelSheets()[0];
        SCH_SCREEN* screen = sheet->GetScreen();
        screen->SetFileName( "pin_map_override.kicad_sch" );

        SCH_SHEET_PATH path;
        path.push_back( sheet );

        m_libSym = std::make_unique<LIB_SYMBOL>( "LM358", nullptr );

        SCH_PIN* pin1 = new SCH_PIN( m_libSym.get() );
        pin1->SetNumber( "1" );
        pin1->SetType( ELECTRICAL_PINTYPE::PT_PASSIVE );
        pin1->SetPosition( VECTOR2I( -508000, 0 ) );
        m_libSym->AddDrawItem( pin1 );

        SCH_SYMBOL* sym = new SCH_SYMBOL( *m_libSym, m_libSym->GetLibId(), &path, 0, 0,
                                          VECTOR2I( 15621000, 6223000 ) );
        sym->UpdatePins();

        if( !aOverride.IsDefault() )
            sym->SetPinMapOverride( aOverride );

        screen->Append( sym );

        return sheet;
    }

    SCH_SYMBOL* reload( const wxString& aFileName )
    {
        SCH_IO_KICAD_SEXPR io;

        m_schematic->Reset();
        SCH_SHEET* defaultSheet = m_schematic->GetTopLevelSheet( 0 );
        SCH_SHEET* loaded = io.LoadSchematicFile( aFileName, m_schematic.get() );
        m_schematic->AddTopLevelSheet( loaded );
        m_schematic->RemoveTopLevelSheet( defaultSheet );
        delete defaultSheet;

        SCH_SYMBOL* found = nullptr;

        for( SCH_ITEM* item : loaded->GetScreen()->Items().OfType( SCH_SYMBOL_T ) )
            found = static_cast<SCH_SYMBOL*>( item );

        return found;
    }

    SETTINGS_MANAGER            m_settingsManager;
    std::unique_ptr<SCHEMATIC>  m_schematic;
    std::unique_ptr<LIB_SYMBOL> m_libSym;
    std::vector<wxString>       m_tempFiles;
};


BOOST_FIXTURE_TEST_SUITE( PinMapInstanceOverride, PIN_MAP_OVERRIDE_FIXTURE )


BOOST_AUTO_TEST_CASE( BaseOverrideSetGet )
{
    SCH_SYMBOL sym;

    BOOST_CHECK( sym.GetPinMapOverride().IsDefault() );

    PIN_MAP_INSTANCE_OVERRIDE ov;
    ov.m_Mode = PIN_MAP_OVERRIDE_MODE::USE_NAMED_MAP;
    ov.m_ActiveMapName = wxS( "DFN-8-EP" );
    ov.m_Edits.push_back( { wxS( "1" ), wxS( "8" ) } );
    ov.m_Edits.push_back( { wxS( "8" ), wxS( "1" ) } );

    sym.SetPinMapOverride( ov );

    BOOST_CHECK( sym.GetPinMapOverride() == ov );
    BOOST_CHECK( !sym.GetPinMapOverride().IsDefault() );
}


BOOST_AUTO_TEST_CASE( VariantSnapshotsBaseOverride )
{
    // A variant created for an unrelated reason must snapshot the base override so it does not
    // mask it when read back through that variant.
    SCH_SYMBOL sym;

    PIN_MAP_INSTANCE_OVERRIDE ov;
    ov.m_Mode = PIN_MAP_OVERRIDE_MODE::FORCE_IDENTITY;
    sym.SetPinMapOverride( ov );

    SCH_SYMBOL_VARIANT variant( wxS( "Assembly" ) );
    variant.InitializeAttributes( sym );

    BOOST_CHECK( variant.m_PinMapOverride == ov );
}


BOOST_AUTO_TEST_CASE( BaseOverrideRoundTripThroughFile )
{
    LOCALE_IO dummy;

    PIN_MAP_INSTANCE_OVERRIDE ov;
    ov.m_Mode = PIN_MAP_OVERRIDE_MODE::USE_NAMED_MAP;
    ov.m_ActiveMapName = wxS( "DFN-8-EP" );
    ov.m_Edits.push_back( { wxS( "1" ), wxS( "8" ) } );

    SCH_SHEET* sheet = placeSymbol( ov );
    wxString   fileName = tempFile();

    SCH_IO_KICAD_SEXPR io;
    io.SaveSchematicFile( fileName, sheet, m_schematic.get() );

    {
        wxFile   f( fileName );
        wxString contents;
        f.ReadAll( &contents );
        BOOST_CHECK( contents.Contains( wxS( "(pin_map_override" ) ) );
        BOOST_CHECK( contents.Contains( wxS( "named_map" ) ) );
    }

    m_libSym.reset();

    SCH_SYMBOL* reloaded = reload( fileName );
    BOOST_REQUIRE( reloaded );

    PIN_MAP_INSTANCE_OVERRIDE result = reloaded->GetPinMapOverride();
    BOOST_CHECK( result.m_Mode == PIN_MAP_OVERRIDE_MODE::USE_NAMED_MAP );
    BOOST_CHECK_EQUAL( result.m_ActiveMapName, wxS( "DFN-8-EP" ) );
    BOOST_REQUIRE_EQUAL( result.m_Edits.size(), 1u );
    BOOST_CHECK_EQUAL( result.m_Edits[0].m_PinNumber, wxS( "1" ) );
    BOOST_CHECK_EQUAL( result.m_Edits[0].m_PadNumber, wxS( "8" ) );
}


BOOST_AUTO_TEST_CASE( DefaultOverrideEmitsNothing )
{
    LOCALE_IO dummy;

    SCH_SHEET* sheet = placeSymbol( PIN_MAP_INSTANCE_OVERRIDE() );
    wxString   fileName = tempFile();

    SCH_IO_KICAD_SEXPR io;
    io.SaveSchematicFile( fileName, sheet, m_schematic.get() );

    wxFile   f( fileName );
    wxString contents;
    f.ReadAll( &contents );
    BOOST_CHECK( !contents.Contains( wxS( "(pin_map_override" ) ) );
}


BOOST_AUTO_TEST_SUITE_END()
