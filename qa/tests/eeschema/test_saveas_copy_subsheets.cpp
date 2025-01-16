/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

/**
 * @file
 * Tests for Save As subsheet copy options
 */

#include <qa_utils/wx_utils/unit_test_utils.h>
#include "eeschema_test_utils.h"

#include <eeschema/save_project_utils.h>
#include <schematic.h>
#include <sch_sheet.h>
#include <sch_sheet_path.h>
#include <wildcards_and_files_ext.h>
#include <wx/filename.h>
#include <wx/ffile.h>
#include <wx/filefn.h>

class SAVEAS_SUBSHEET_FIXTURE : public KI_TEST::SCHEMATIC_TEST_FIXTURE
{
public:
    SAVEAS_SUBSHEET_FIXTURE();

    wxFileName GetSchematicPath( const wxString& aRelativePath );

    wxFileName m_baseDir;
    wxFileName m_srcDir;
    wxFileName m_externalDir;
};

SAVEAS_SUBSHEET_FIXTURE::SAVEAS_SUBSHEET_FIXTURE()
{
    wxFileName tmp( wxFileName::CreateTempFileName( wxS( "saveas" ) ) );
    wxString dirPath = tmp.GetPath();      // /tmp
    wxString dirName = tmp.GetName();      // saveasXXXX
    wxRemoveFile( tmp.GetFullPath() );

    wxFileName base( dirPath, wxEmptyString ); // represents /tmp
    base.AppendDir( dirName );                 // now /tmp/saveasXXXX
    base.Mkdir();                               // create the directory
    m_baseDir = base;

    m_srcDir = m_baseDir;
    m_srcDir.AppendDir( wxS( "src" ) );
    m_srcDir.Mkdir();

    m_externalDir = m_baseDir;
    m_externalDir.AppendDir( wxS( "external" ) );
    m_externalDir.Mkdir();

    wxFileName dataDir( KI_TEST::GetEeschemaTestDataDir() );

    wxCopyFile( dataDir.GetFullPath() + wxS( "/issue13212.kicad_sch" ),
                m_srcDir.GetFullPath() + wxS( "/issue13212.kicad_sch" ) );
    wxCopyFile( dataDir.GetFullPath() + wxS( "/issue13212_subsheet_1.kicad_sch" ),
                m_srcDir.GetFullPath() + wxS( "/issue13212_subsheet_1.kicad_sch" ) );
    wxCopyFile( dataDir.GetFullPath() + wxS( "/issue13212_subsheet_2.kicad_sch" ),
                m_externalDir.GetFullPath() + wxS( "/issue13212_subsheet_2.kicad_sch" ) );

    wxFileName rootFile( m_srcDir );
    rootFile.SetFullName( wxS( "issue13212.kicad_sch" ) );

    wxFFile file( rootFile.GetFullPath(), wxS( "rb" ) );
    wxString content;
    file.ReadAll( &content );
    file.Close();
    content.Replace( wxS( "issue13212_subsheet_2.kicad_sch" ),
                     wxS( "../external/issue13212_subsheet_2.kicad_sch" ) );
    wxFFile outFile( rootFile.GetFullPath(), wxS( "wb" ) );
    outFile.Write( content );
    outFile.Close();
}

wxFileName SAVEAS_SUBSHEET_FIXTURE::GetSchematicPath( const wxString& aRelativePath )
{
    wxFileName fn( m_srcDir );
    fn.SetName( aRelativePath );
    fn.SetExt( FILEEXT::KiCadSchematicFileExtension );
    return fn;
}

BOOST_FIXTURE_TEST_SUITE( SaveAsSubsheetCopy, SAVEAS_SUBSHEET_FIXTURE )

BOOST_AUTO_TEST_CASE( CopyInternalReferenceExternal )
{
    LoadSchematic( GetSchematicPath( wxS( "issue13212" ) ) );

    SCH_SCREENS screens( m_schematic->Root() );
    wxFileName srcRoot = GetSchematicPath( wxS( "issue13212" ) );

    wxFileName destRoot( m_baseDir );
    destRoot.AppendDir( wxS( "new" ) );
    destRoot.AppendDir( wxS( "location" ) );
    destRoot.SetName( wxS( "issue13212" ) );
    destRoot.SetExt( FILEEXT::KiCadSchematicFileExtension );
    { wxFileName destDir( destRoot ); destDir.SetFullName( wxEmptyString ); if( !destDir.DirExists() ) destDir.Mkdir( 0777, wxPATH_MKDIR_FULL ); }

    std::unordered_map<SCH_SCREEN*, wxString> filenameMap;
    wxString msg;

    m_schematic->Root().SetFileName( destRoot.GetFullName() );
    m_schematic->RootScreen()->SetFileName( destRoot.GetFullPath() );

    bool ok = PrepareSaveAsFiles( *m_schematic, screens, srcRoot, destRoot, false, true,
                                  false, filenameMap, msg );
    BOOST_CHECK( ok );

    SCH_SCREEN* internal = nullptr;
    SCH_SCREEN* external = nullptr;

    for( size_t i = 0; i < screens.GetCount(); i++ )
    {
        SCH_SCREEN* s = screens.GetScreen( i );

        if( wxString( s->GetFileName() ).EndsWith( wxS( "issue13212_subsheet_1.kicad_sch" ) ) )
            internal = s;
        else if( wxString( s->GetFileName() ).EndsWith( wxS( "issue13212_subsheet_2.kicad_sch" ) ) )
            external = s;
    }

    wxFileName internalExpected( destRoot.GetPath(), wxS( "issue13212_subsheet_1.kicad_sch" ) );
    BOOST_CHECK_EQUAL( internal->GetFileName(), internalExpected.GetFullPath() );

    wxFileName externalExpected( m_externalDir.GetFullPath(), wxS( "issue13212_subsheet_2.kicad_sch" ) );
    BOOST_CHECK_EQUAL( external->GetFileName(), externalExpected.GetFullPath() );

    SCH_SHEET_LIST sheetList = m_schematic->BuildSheetListSortedByPageNumbers();

    wxString externalSheetPath;

    for( const SCH_SHEET_PATH& path : sheetList )
    {
        if( path.Last()->GetFileName().Contains( wxS( "issue13212_subsheet_2" ) ) )
            externalSheetPath = path.Last()->GetFileName();
    }

    BOOST_CHECK_EQUAL( externalSheetPath, wxS( "../../external/issue13212_subsheet_2.kicad_sch" ) );
}

BOOST_AUTO_TEST_CASE( CopyIncludingExternal )
{
    LoadSchematic( GetSchematicPath( wxS( "issue13212" ) ) );

    SCH_SCREENS screens( m_schematic->Root() );
    wxFileName srcRoot = GetSchematicPath( wxS( "issue13212" ) );

    wxFileName destRoot( m_baseDir );
    destRoot.AppendDir( wxS( "destall" ) );
    destRoot.SetName( wxS( "issue13212" ) );
    destRoot.SetExt( FILEEXT::KiCadSchematicFileExtension );
    { wxFileName destDir( destRoot ); destDir.SetFullName( wxEmptyString ); if( !destDir.DirExists() ) destDir.Mkdir( 0777, wxPATH_MKDIR_FULL ); }

    std::unordered_map<SCH_SCREEN*, wxString> filenameMap;
    wxString msg;

    m_schematic->Root().SetFileName( destRoot.GetFullName() );
    m_schematic->RootScreen()->SetFileName( destRoot.GetFullPath() );

    bool ok = PrepareSaveAsFiles( *m_schematic, screens, srcRoot, destRoot, false, true,
                                  true, filenameMap, msg );
    BOOST_CHECK( ok );

    SCH_SCREEN* external = nullptr;

    for( size_t i = 0; i < screens.GetCount(); i++ )
    {
        SCH_SCREEN* s = screens.GetScreen( i );

        if( wxString( s->GetFileName() ).EndsWith( wxS( "issue13212_subsheet_2.kicad_sch" ) ) )
            external = s;
    }

    wxFileName externalExpected( destRoot.GetPath(), wxS( "issue13212_subsheet_2.kicad_sch" ) );
    BOOST_CHECK_EQUAL( external->GetFileName(), externalExpected.GetFullPath() );
}

BOOST_AUTO_TEST_CASE( NoCopyKeepsOriginalPaths )
{
    LoadSchematic( GetSchematicPath( wxS( "issue13212" ) ) );

    SCH_SCREENS screens( m_schematic->Root() );
    wxFileName srcRoot = GetSchematicPath( wxS( "issue13212" ) );

    wxFileName destRoot( m_baseDir );
    destRoot.AppendDir( wxS( "nocopy" ) );
    destRoot.SetName( wxS( "issue13212" ) );
    destRoot.SetExt( FILEEXT::KiCadSchematicFileExtension );
    { wxFileName destDir( destRoot ); destDir.SetFullName( wxEmptyString ); if( !destDir.DirExists() ) destDir.Mkdir( 0777, wxPATH_MKDIR_FULL ); }

    std::unordered_map<SCH_SCREEN*, wxString> filenameMap;
    wxString msg;

    m_schematic->Root().SetFileName( destRoot.GetFullName() );
    m_schematic->RootScreen()->SetFileName( destRoot.GetFullPath() );

    bool ok = PrepareSaveAsFiles( *m_schematic, screens, srcRoot, destRoot, false, false,
                                  false, filenameMap, msg );
    BOOST_CHECK( ok );

    SCH_SCREEN* internal = nullptr;

    for( size_t i = 0; i < screens.GetCount(); i++ )
    {
        SCH_SCREEN* s = screens.GetScreen( i );

        if( wxString( s->GetFileName() ).EndsWith( wxS( "issue13212_subsheet_1.kicad_sch" ) ) )
            internal = s;
    }

    wxFileName internalExpected( m_srcDir.GetFullPath(), wxS( "issue13212_subsheet_1.kicad_sch" ) );
    BOOST_CHECK_EQUAL( internal->GetFileName(), internalExpected.GetFullPath() );
}

BOOST_AUTO_TEST_SUITE_END()
