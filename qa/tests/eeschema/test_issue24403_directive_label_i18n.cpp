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
 * @file test_issue24403_directive_label_i18n.cpp
 *
 * Regression coverage for https://gitlab.com/kicad/code/kicad/-/issues/24403.
 *
 * A KiCad UI in German (or any non-English language) used to write the directive-label net class
 * field name as the translated form (e.g. "Netzklasse" in German). When the same file was opened
 * with an English UI the canonical token "Netclass" was no longer present, so the directive label
 * lost its association with the project's net class settings. Cross-language collaboration over
 * Git therefore destroyed all directive-label net class bindings.
 *
 * These tests pin the contract on both the writer and the migration paths.
 */

#include <boost/test/unit_test.hpp>
#include <qa_utils/wx_utils/unit_test_utils.h>
#include <eeschema_test_utils.h>

#include <wx/ffile.h>
#include <wx/filename.h>
#include <wx/stdpaths.h>

#include <sch_field.h>
#include <sch_io/kicad_sexpr/sch_io_kicad_sexpr.h>
#include <sch_label.h>
#include <sch_screen.h>
#include <sch_sheet.h>
#include <schematic.h>
#include <settings/settings_manager.h>


struct DIRECTIVE_LABEL_I18N_FIXTURE
{
    DIRECTIVE_LABEL_I18N_FIXTURE() :
            m_settingsManager()
    {
        wxString tempDir = wxStandardPaths::Get().GetTempDir();
        wxString projectPath =
                tempDir + wxFileName::GetPathSeparator() + wxT( "issue24403_project.kicad_pro" );
        m_tempFiles.push_back( projectPath );

        m_settingsManager.LoadProject( projectPath.ToStdString() );
        m_schematic = std::make_unique<SCHEMATIC>( nullptr );
        m_schematic->SetProject( &m_settingsManager.Prj() );
    }

    ~DIRECTIVE_LABEL_I18N_FIXTURE()
    {
        for( const wxString& file : m_tempFiles )
        {
            if( wxFileExists( file ) )
                wxRemoveFile( file );
        }

        m_schematic.reset();
    }

    wxString MakeTempSchematicPath( const wxString& aPrefix )
    {
        // wxFileName::CreateTempFileName() creates (and tracks) the file it returns. Track both
        // the base file and the .kicad_sch path we hand out so cleanup removes everything.
        wxString tempDir = wxStandardPaths::Get().GetTempDir();
        wxString baseName = wxFileName::CreateTempFileName(
                tempDir + wxFileName::GetPathSeparator() + aPrefix );
        m_tempFiles.push_back( baseName );

        wxString fileName = baseName + wxT( ".kicad_sch" );
        m_tempFiles.push_back( fileName );
        return fileName;
    }

    SCH_DIRECTIVE_LABEL* AddDirectiveLabel( const wxString& aFieldName,
                                            const wxString& aNetClassName )
    {
        std::vector<SCH_SHEET*> topSheets = m_schematic->GetTopLevelSheets();
        BOOST_REQUIRE( !topSheets.empty() );

        SCH_SCREEN* screen = topSheets[0]->GetScreen();
        BOOST_REQUIRE( screen );

        SCH_DIRECTIVE_LABEL* label = new SCH_DIRECTIVE_LABEL( VECTOR2I( 0, 0 ) );
        label->GetFields().emplace_back( label, FIELD_T::USER, aFieldName );
        label->GetFields().back().SetText( aNetClassName );

        screen->Append( label );
        return label;
    }

    SETTINGS_MANAGER           m_settingsManager;
    std::unique_ptr<SCHEMATIC> m_schematic;
    std::vector<wxString>      m_tempFiles;
};


BOOST_FIXTURE_TEST_SUITE( Issue24403DirectiveLabelI18n, DIRECTIVE_LABEL_I18N_FIXTURE )


/**
 * The serializer must always emit the canonical "Netclass" token for the directive-label net
 * class field, even when the in-memory field name happens to be a translated string (which can
 * happen with files written by older KiCad versions).
 */
BOOST_AUTO_TEST_CASE( CanonicalNetclassEmittedForTranslatedFieldName )
{
    m_schematic->CreateDefaultScreens();

    // Simulate a directive label whose field name carries a translated form, mirroring the
    // state produced by a German UI under the buggy 10.0.x code path.
    AddDirectiveLabel( wxT( "Netzklasse" ), wxT( "HighSpeed" ) );

    std::vector<SCH_SHEET*> topSheets = m_schematic->GetTopLevelSheets();
    SCH_IO_KICAD_SEXPR      io;
    wxString                outFile = MakeTempSchematicPath( "issue24403_de" );

    BOOST_REQUIRE_NO_THROW( io.SaveSchematicFile( outFile, topSheets[0], m_schematic.get() ) );

    wxFFile  read( outFile, "rb" );
    wxString contents;
    BOOST_REQUIRE( read.IsOpened() && read.ReadAll( &contents ) );

    BOOST_CHECK_MESSAGE( contents.Contains( wxT( "\"Netclass\"" ) ),
                         "Saved schematic must contain canonical \"Netclass\" token" );
    BOOST_CHECK_MESSAGE( !contents.Contains( wxT( "\"Netzklasse\"" ) ),
                         "Saved schematic must not contain translated \"Netzklasse\" token" );
    BOOST_CHECK_MESSAGE( contents.Contains( wxT( "HighSpeed" ) ),
                         "Saved schematic must preserve the net class value" );
}


/**
 * Round-trip: a file written by a non-English UI (here we simulate German "Netzklasse") must be
 * recognised as a directive net class binding when reloaded, so that GetCanonicalName() returns
 * the canonical token used by the rest of the code base.
 */
BOOST_AUTO_TEST_CASE( TranslatedFieldNameRoundTripsAsCanonical )
{
    m_schematic->CreateDefaultScreens();

    // First write a synthetic file containing the legacy translated form so we exercise the
    // loader's migration path directly.
    wxString legacyFile = MakeTempSchematicPath( "issue24403_legacy_de" );

    {
        wxFFile out( legacyFile, "wb" );
        BOOST_REQUIRE( out.IsOpened() );

        const wxString body = wxT(
                "(kicad_sch (version 20260512) (generator \"qa_test\") (generator_version \"9.99\")\n"
                "  (paper \"A4\")\n"
                "  (lib_symbols)\n"
                "  (directive_label \"\" (length 2.54) (shape round)\n"
                "    (at 0 0 0)\n"
                "    (effects (font (size 1.27 1.27)) (justify left bottom))\n"
                "    (uuid \"00000000-0000-0000-0000-000000024403\")\n"
                "    (property  \"Netzklasse\" \"HighSpeed\" (at 0 -1 0)\n"
                "      (effects (font (size 1.27 1.27)) (justify left bottom))\n"
                "    )\n"
                "  )\n"
                "  (sheet_instances (path \"/\" (page \"1\")))\n"
                ")\n" );

        BOOST_REQUIRE( out.Write( body ) );
    }

    SCH_IO_KICAD_SEXPR io;
    SCH_SHEET*         loaded = nullptr;
    BOOST_REQUIRE_NO_THROW( loaded = io.LoadSchematicFile( legacyFile, m_schematic.get() ) );
    BOOST_REQUIRE( loaded );

    // Mount the loaded sheet so we can re-serialise through the standard SaveSchematicFile path.
    SCH_SHEET* defaultSheet = m_schematic->GetTopLevelSheet( 0 );
    m_schematic->AddTopLevelSheet( loaded );
    m_schematic->RemoveTopLevelSheet( defaultSheet );
    delete defaultSheet;

    SCH_SCREEN* screen = loaded->GetScreen();
    BOOST_REQUIRE( screen );

    int directiveCount = 0;
    int netclassFieldCount = 0;

    for( SCH_ITEM* item : screen->Items().OfType( SCH_DIRECTIVE_LABEL_T ) )
    {
        directiveCount++;

        SCH_DIRECTIVE_LABEL* directive = static_cast<SCH_DIRECTIVE_LABEL*>( item );

        for( const SCH_FIELD& field : directive->GetFields() )
        {
            if( field.GetCanonicalName() == wxT( "Netclass" ) )
            {
                netclassFieldCount++;
                BOOST_CHECK_EQUAL( field.GetText().ToStdString(), std::string( "HighSpeed" ) );
            }
        }
    }

    BOOST_CHECK_EQUAL( directiveCount, 1 );
    BOOST_CHECK_EQUAL( netclassFieldCount, 1 );

    // After re-saving, the file must converge on the canonical token regardless of the form it
    // was loaded with.
    wxString                resaved = MakeTempSchematicPath( "issue24403_legacy_de_resave" );
    std::vector<SCH_SHEET*> topSheets = m_schematic->GetTopLevelSheets();
    BOOST_REQUIRE( !topSheets.empty() );

    BOOST_REQUIRE_NO_THROW( io.SaveSchematicFile( resaved, topSheets[0], m_schematic.get() ) );

    wxFFile  resavedFile( resaved, "rb" );
    wxString resavedContents;
    BOOST_REQUIRE( resavedFile.IsOpened() && resavedFile.ReadAll( &resavedContents ) );

    BOOST_CHECK_MESSAGE( resavedContents.Contains( wxT( "\"Netclass\"" ) ),
                         "Re-saved schematic must use canonical \"Netclass\" token" );
    BOOST_CHECK_MESSAGE( !resavedContents.Contains( wxT( "\"Netzklasse\"" ) ),
                         "Re-saved schematic must drop the translated form" );
}


/**
 * Direct contract test for the helper that recognises translated forms. Bug fixes that
 * inadvertently shrink the recognised translations will fail this test instead of silently
 * regressing the migration path.
 */
BOOST_AUTO_TEST_CASE( IsNetclassLabelFieldNameRecognisesTranslations )
{
    BOOST_CHECK( SCH_FIELD::IsNetclassLabelFieldName( wxT( "Netclass" ) ) );
    BOOST_CHECK( SCH_FIELD::IsNetclassLabelFieldName( wxT( "Net Class" ) ) );
    BOOST_CHECK( SCH_FIELD::IsNetclassLabelFieldName( wxString::FromUTF8( "Netzklasse" ) ) );
    BOOST_CHECK( SCH_FIELD::IsNetclassLabelFieldName( wxString::FromUTF8( "Classe de xarxa" ) ) );
    BOOST_CHECK( SCH_FIELD::IsNetclassLabelFieldName( wxString::FromUTF8( "Classe d'Equipot" ) ) );
    BOOST_CHECK( SCH_FIELD::IsNetclassLabelFieldName( wxString::FromUTF8( "Clase de red" ) ) );
    BOOST_CHECK( SCH_FIELD::IsNetclassLabelFieldName( wxString::FromUTF8( "网络类" ) ) );

    BOOST_CHECK( !SCH_FIELD::IsNetclassLabelFieldName( wxT( "Component Class" ) ) );
    BOOST_CHECK( !SCH_FIELD::IsNetclassLabelFieldName( wxT( "Reference" ) ) );
    BOOST_CHECK( !SCH_FIELD::IsNetclassLabelFieldName( wxEmptyString ) );
}


BOOST_AUTO_TEST_SUITE_END()
