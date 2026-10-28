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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

/**
 * @file test_ref_suffix.cpp
 * Reference suffixes on reused sheets.
 */

#include <qa_utils/wx_utils/unit_test_utils.h>
#include <qa_utils/file_utils.h>
#include "eeschema_test_utils.h"

#include <api/api_sch_utils.h>
#include <api/schematic/schematic_types.pb.h>
#include <connectivity/conn_navigation.h>
#include <downgrade/sch_downgrade.h>
#include <downgrade_target.h>
#include <eda_search_data.h>
#include <erc/erc.h>
#include <erc/erc_item.h>
#include <netlist_exporters/netlist_exporter_allegro.h>
#include <netlist_exporters/netlist_exporter_kicad.h>
#include <netlist_exporters/netlist_exporter_xml.h>
#include <reporter.h>
#include <richio.h>
#include <sch_io/kicad_sexpr/sch_io_kicad_sexpr.h>
#include <sch_commit.h>
#include <sch_marker.h>
#include <sch_reference_list.h>
#include <symbol_fields_data_model.h>
#include <tools/backannotate.h>
#include <sch_screen.h>
#include <sch_symbol.h>
#include <sch_sheet.h>
#include <sch_sheet_path.h>
#include <schematic.h>
#include <tool/tool_manager.h>
#include <validators.h>
#include <wildcards_and_files_ext.h>

#include <set>

#include <wx/ffile.h>
#include <wx/regex.h>
#include <wx/tokenzr.h>


class REF_SUFFIX_FIXTURE : public KI_TEST::SCHEMATIC_TEST_FIXTURE
{
public:
    ~REF_SUFFIX_FIXTURE() override
    {
        if( m_dir.IsEmpty() )
            return;

        // Release the project lock file before its directory goes away
        m_schematic.reset();
        m_manager.UnloadProject( &m_manager.Prj(), false );
        wxFileName::Rmdir( m_dir, wxPATH_RMDIR_RECURSIVE );
    }

    void Load( const wxString& aPath )
    {
        LoadSchematic( wxFileName( KI_TEST::GetEeschemaTestDataDir() + aPath + wxS( "." )
                                   + FILEEXT::KiCadSchematicFileExtension ) );
    }

    // Work on a copy of issue13212 so we can save it
    void LoadCopy()
    {
        m_dir = wxFileName::CreateTempFileName( wxFileName::GetTempDir() + wxFileName::GetPathSeparator()
                                                + wxS( "ref_suffix" ) );
        wxRemoveFile( m_dir );
        wxFileName::Mkdir( m_dir );

        for( const wxString& name : { wxS( "issue13212" ), wxS( "issue13212_subsheet_1" ),
                                      wxS( "issue13212_subsheet_2" ) } )
        {
            wxFileName src( KI_TEST::GetEeschemaTestDataDir(), name, FILEEXT::KiCadSchematicFileExtension );
            wxFileName dst( m_dir, name, FILEEXT::KiCadSchematicFileExtension );
            BOOST_REQUIRE( wxCopyFile( src.GetFullPath(), dst.GetFullPath() ) );
        }

        LoadSchematic( RootFile() );
    }

    wxFileName RootFile() const
    {
        return wxFileName( m_dir, wxS( "issue13212" ), FILEEXT::KiCadSchematicFileExtension );
    }

    std::vector<SCH_SHEET_PATH> PathsOf( const wxString& aFile ) const
    {
        std::vector<SCH_SHEET_PATH> paths;

        for( const SCH_SHEET_PATH& path : m_schematic->Hierarchy() )
        {
            if( path.Last()->GetFileName() == aFile )
                paths.push_back( path );
        }

        return paths;
    }

    // Suffixes the two sheets of a reused file in page order; issue13212 reuses its subsheet_2
    std::vector<SCH_SHEET_PATH> SuffixChannels( const wxString& aFirst = wxS( "_M1" ),
                                                const wxString& aSecond = wxS( "_M2" ),
                                                const wxString& aFile = wxS( "issue13212_subsheet_2.kicad_sch" ) )
    {
        std::vector<SCH_SHEET_PATH> channels = PathsOf( aFile );
        BOOST_REQUIRE_EQUAL( channels.size(), 2 );

        AddSuffixField( channels[0].Last(), aFirst );
        AddSuffixField( channels[1].Last(), aSecond );
        m_schematic->Settings().m_AnnotateReusedSheetSuffixes = true;
        m_schematic->RefreshHierarchy();
        return channels;
    }

    // legacy_hierarchy reuses ampli_ht.kicad_sch twice, with separate numbering (U201 and U301)
    std::vector<SCH_SHEET_PATH> LoadAmpliChannels()
    {
        Load( wxS( "legacy_hierarchy/legacy_hierarchy" ) );
        return SuffixChannels( wxS( "_V" ), wxS( "_H" ), wxS( "ampli_ht.kicad_sch" ) );
    }

    static std::vector<SCH_SHEET*> SubSheets( SCH_SCREEN* aScreen )
    {
        std::vector<SCH_SHEET*> sheets;

        for( SCH_ITEM* item : aScreen->Items().OfType( SCH_SHEET_T ) )
            sheets.push_back( static_cast<SCH_SHEET*>( item ) );

        std::sort( sheets.begin(), sheets.end(),
                   []( const SCH_SHEET* a, const SCH_SHEET* b )
                   {
                       return a->GetName() < b->GetName();
                   } );

        return sheets;
    }

    static SCH_SYMBOL* FirstSymbol( const SCH_SHEET_PATH& aPath )
    {
        for( SCH_ITEM* item : aPath.LastScreen()->Items().OfType( SCH_SYMBOL_T ) )
            return static_cast<SCH_SYMBOL*>( item );

        return nullptr;
    }

    static SCH_SYMBOL* FindSymbol( const SCH_SHEET_PATH& aPath, const wxString& aRef, int aUnit = 1 )
    {
        for( SCH_ITEM* item : aPath.LastScreen()->Items().OfType( SCH_SYMBOL_T ) )
        {
            SCH_SYMBOL* symbol = static_cast<SCH_SYMBOL*>( item );

            if( symbol->GetRef( &aPath ) == aRef && symbol->GetUnitSelection( &aPath ) == aUnit )
                return symbol;
        }

        return nullptr;
    }

    static void AddSuffixField( SCH_SHEET* aSheet, const wxString& aText )
    {
        aSheet->AddField( SCH_FIELD( aSheet, FIELD_T::SHEET_USER, wxS( "Reference Suffix" ) ) )->SetText( aText );
    }

    // SCH_EDIT_FRAME::AnnotateSymbols in suffix mode once it has collected its scope
    void Annotate( SCH_REFERENCE_LIST& aRefs, const SCH_MULTI_UNIT_REFERENCE_MAP& aLocked, ANNOTATE_SCOPE_T aScope,
                   bool aResetAnnotation )
    {
        m_schematic->Settings().m_AnnotateReusedSheetSuffixes = true;
        BOOST_REQUIRE( !FindRefSuffixCollision( aRefs, *m_schematic ) );
        AnnotateReferences( aRefs, aLocked, *m_schematic, aScope, UNSORTED, INCREMENTAL_BY_REF, 0, aResetAnnotation );
    }

    void AnnotateAll( bool aResetAnnotation = false )
    {
        SCH_SHEET_LIST               sheets = m_schematic->Hierarchy();
        SCH_REFERENCE_LIST           refs;
        SCH_MULTI_UNIT_REFERENCE_MAP locked;

        sheets.GetSymbols( refs, SYMBOL_FILTER_NON_POWER );
        sheets.GetMultiUnitSymbols( locked, SYMBOL_FILTER_NON_POWER );
        Annotate( refs, locked, ANNOTATE_ALL, aResetAnnotation );
    }

    void AnnotateSelection( const std::vector<SCH_SYMBOL*>& aSymbols, const SCH_SHEET_PATH& aPath )
    {
        SCH_REFERENCE_LIST           refs;
        SCH_MULTI_UNIT_REFERENCE_MAP locked;

        for( SCH_SYMBOL* symbol : aSymbols )
        {
            aPath.AppendSymbol( refs, symbol, SYMBOL_FILTER_NON_POWER );
            aPath.AppendMultiUnitSymbol( locked, symbol, SYMBOL_FILTER_NON_POWER );
        }

        Annotate( refs, locked, ANNOTATE_SELECTION, false );
    }

    void AnnotatePlaced( SCH_SYMBOL* aSymbol, const std::vector<SCH_SHEET_PATH>& aChannels )
    {
        SCH_SHEET_LIST     hierarchy = m_schematic->Hierarchy();
        SCH_REFERENCE_LIST existing;

        hierarchy.GetSymbols( existing, SYMBOL_FILTER_ALL );
        existing.SortByReferenceOnly();
        AnnotateNewSymbol( aSymbol, aChannels, existing, m_schematic->Settings(), &hierarchy );
    }

    int CountDuplicates()
    {
        SCH_REFERENCE_LIST refs;
        int                count = 0;

        m_schematic->Hierarchy().GetSymbols( refs, SYMBOL_FILTER_NON_POWER );
        refs.CheckAnnotation(
                [&]( ERCE_T aType, const wxString&, SCH_REFERENCE*, SCH_REFERENCE* )
                {
                    count += aType == ERCE_DUPLICATE_REFERENCE;
                } );

        return count;
    }

    int CountErcMarkers( int aCode )
    {
        ERC_TESTER( m_schematic.get() ).RunTests( nullptr, nullptr, nullptr, &m_schematic->Project(), nullptr );

        int         count = 0;
        SCH_SCREENS screens( m_schematic->Root() );

        for( SCH_SCREEN* screen = screens.GetFirst(); screen; screen = screens.GetNext() )
        {
            for( SCH_ITEM* item : screen->Items().OfType( SCH_MARKER_T ) )
                count += static_cast<SCH_MARKER*>( item )->GetRCItem()->GetErrorCode() == aCode;
        }

        return count;
    }

    wxString m_dir;
};

BOOST_FIXTURE_TEST_SUITE( RefSuffix, REF_SUFFIX_FIXTURE )


BOOST_AUTO_TEST_CASE( SuffixTextRules )
{
    SCH_SHEET sheet;
    SCH_FIELD field( &sheet, FIELD_T::SHEET_USER, wxS( "Reference Suffix" ) );

    // Sheet Properties renumbers every non-mandatory field as SHEET_USER on OK
    field.SetOrdinal( 42, FIELD_T::SHEET_USER );
    BOOST_CHECK( field.GetId() == FIELD_T::SHEET_REF_SUFFIX );

    field.SetName( wxS( "reference suffix" ) );
    BOOST_CHECK( field.GetId() == FIELD_T::SHEET_USER );

    BOOST_CHECK( GetFieldValidationErrorMessage( FIELD_T::SHEET_REF_SUFFIX, wxS( "_${CH}" ) ).IsEmpty() );

    for( const wxString& bad : { wxS( "_M 1" ), wxS( "_M?" ), wxS( "_M1,_M2" ), wxS( "2" ) } )
        BOOST_CHECK_MESSAGE( !GetFieldValidationErrorMessage( FIELD_T::SHEET_REF_SUFFIX, bad ).IsEmpty(), bad );

    const std::set<wxString> inUse = { wxS( "_M1" ), wxS( "_M3" ), wxS( "_M09" ), wxS( "_K01" ), wxS( "_A" ),
                                       wxS( "_AY" ), wxS( "_b" ), wxS( "_z" ), wxS( "_LEFT" ), wxS( "_N2X" ),
                                       wxS( "_1ZZ" ), wxS( "_1AAA" ), wxS( "_W0" ),
                                       wxS( "_W18446744073709551615" ) };

    BOOST_CHECK_EQUAL( SCH_SHEET::NextRefSuffix( wxS( "_M1" ), inUse ), wxS( "_M10" ) );
    BOOST_CHECK_EQUAL( SCH_SHEET::NextRefSuffix( wxS( "_A" ), inUse ), wxS( "_AZ" ) );
    BOOST_CHECK_EQUAL( SCH_SHEET::NextRefSuffix( wxS( "_LEFT" ), inUse ), wxS( "_LEFT" ) );
}


BOOST_AUTO_TEST_CASE( SaveAndLoad )
{
    LoadCopy();

    std::vector<SCH_SHEET_PATH> channels = SuffixChannels();
    SCH_SYMBOL*                 symbol = FirstSymbol( channels[0] );
    const KIID                  uuid = symbol->m_Uuid;
    const KIID_PATH             paths[2] = { channels[0].Path(), channels[1].Path() };

    symbol->SetRef( &channels[0], wxS( "R91" ) );
    symbol->SetRef( &channels[1], wxS( "R92" ) );

    // Older readers show the stored reference, so it must already carry the suffix
    auto storesComposed =
            []( const wxString& aFile )
            {
                wxString text;
                BOOST_REQUIRE( wxFFile( aFile, wxS( "rb" ) ).ReadAll( &text ) );
                return text.Contains( wxS( "(reference \"R91_M1\")" ) )
                       && text.Contains( wxS( "(reference \"R92_M2\")" ) );
            };

    SCH_IO_KICAD_SEXPR io;
    io.SaveSchematicFile( RootFile().GetFullPath(), m_schematic->GetTopLevelSheets()[0], m_schematic.get() );
    io.SaveSchematicFile( channels[0].LastScreen()->GetFileName(), channels[0].Last(), m_schematic.get() );
    BOOST_CHECK( storesComposed( channels[0].LastScreen()->GetFileName() ) );

    wxFileName downgraded( m_dir, wxS( "downgraded" ), FILEEXT::KiCadSchematicFileExtension );

    for( const DOWNGRADE_TARGET& target : GetDowngradeTargets() )
    {
        BOOST_TEST_CONTEXT( target.m_name )
        {
            SaveSchematicForTarget( channels[0].Last(), m_schematic.get(), downgraded.GetFullPath(), target );
            BOOST_CHECK( storesComposed( downgraded.GetFullPath() ) );
        }
    }

    LoadSchematic( RootFile() );

    std::optional<SCH_SHEET_PATH> first = m_schematic->Hierarchy().GetSheetPathByKIIDPath( paths[0] );
    std::optional<SCH_SHEET_PATH> second = m_schematic->Hierarchy().GetSheetPathByKIIDPath( paths[1] );
    SCH_SYMBOL*                   loaded = static_cast<SCH_SYMBOL*>( m_schematic->ResolveItem( uuid ) );
    BOOST_REQUIRE( first && second && loaded );

    BOOST_CHECK( first->Last()->GetField( FIELD_T::SHEET_REF_SUFFIX ) );
    BOOST_CHECK_EQUAL( first->Last()->GetRefSuffix( m_schematic->Hierarchy()[0] ), wxS( "_M1" ) );
    BOOST_CHECK( m_schematic->GetRefSuffix( *first ).IsEmpty() );
    BOOST_CHECK_EQUAL( loaded->GetRef( &*first ), wxS( "R91" ) );

    io.SaveSchematicFile( first->LastScreen()->GetFileName(), first->Last(), m_schematic.get() );
    wxString plainFile;
    BOOST_REQUIRE( wxFFile( first->LastScreen()->GetFileName(), wxS( "rb" ) ).ReadAll( &plainFile ) );
    BOOST_CHECK( plainFile.Contains( wxS( "(reference \"R91\")" ) ) );
    BOOST_CHECK( plainFile.Contains( wxS( "(reference \"R92\")" ) ) );
    BOOST_CHECK( !plainFile.Contains( wxS( "(reference \"R91_M1\")" ) ) );

    m_schematic->Settings().m_AnnotateReusedSheetSuffixes = true;
    m_schematic->RefreshRefSuffixes();
    BOOST_CHECK_EQUAL( loaded->GetRef( &*first ), wxS( "R91_M1" ) );
    BOOST_CHECK_EQUAL( loaded->GetBaseRef( &*first ), wxS( "R91" ) );
    BOOST_CHECK_EQUAL( loaded->GetBaseRef( &*second ), wxS( "R92" ) );

    // Importing the saved file as a sheet strips the source project's suffixes
    SCHEMATIC host( &m_manager.Prj() );
    host.Reset();
    host.RefreshHierarchy();

    auto                        sheet = std::make_unique<SCH_SHEET>( &host );
    std::map<std::string, UTF8> props{ { "hierarchical_sheet_load", "1" } };

    sheet->SetFileName( RootFile().GetFullPath() );
    io.LoadSchematicFile( RootFile().GetFullPath(), &host, sheet.get(), &props );

    SCH_SHEET_LIST     imported( sheet.get() );
    SCH_SHEET_PATH     prefix = host.Hierarchy()[0];
    std::set<wxString> refs;

    imported.AddNewSymbolInstances( prefix, wxS( "host" ), true );

    // The loaded root takes the new sheet's UUID, so each channel must find its own source instance below it
    for( SCH_SHEET_PATH& path : imported )
    {
        for( SCH_ITEM* item : path.LastScreen()->Items().OfType( SCH_SYMBOL_T ) )
        {
            SCH_SYMBOL_INSTANCE instance;

            if( item->m_Uuid == uuid
                && static_cast<SCH_SYMBOL*>( item )->GetInstance( instance, ( prefix + path ).Path() ) )
            {
                refs.insert( instance.m_Reference );
            }
        }
    }

    BOOST_CHECK( refs == std::set<wxString>( { wxS( "R91" ), wxS( "R92" ) } ) );
}


BOOST_AUTO_TEST_CASE( ComposesPerPath )
{
    Load( wxS( "issue25112/issue25112" ) );

    // issue25112 reuses level1 twice and level1 reuses level2 twice, so four paths share one leaf
    SCH_SHEET_PATH          root = m_schematic->Hierarchy()[0];
    std::vector<SCH_SHEET*> outer = SubSheets( root.LastScreen() );
    BOOST_REQUIRE_EQUAL( outer.size(), 2 );

    std::vector<SCH_SHEET*> inner = SubSheets( outer[0]->GetScreen() );
    BOOST_REQUIRE_EQUAL( inner.size(), 2 );

    AddSuffixField( outer[0], wxS( "_M1" ) );
    AddSuffixField( outer[1], wxS( "_M2" ) );
    outer[0]->AddField( SCH_FIELD( outer[0], FIELD_T::SHEET_USER, wxS( "CH" ) ) )->SetText( wxS( "a" ) );
    outer[1]->AddField( SCH_FIELD( outer[1], FIELD_T::SHEET_USER, wxS( "CH" ) ) )->SetText( wxS( "b" ) );
    AddSuffixField( inner[0], wxS( "_${CH}" ) );
    m_schematic->Settings().m_AnnotateReusedSheetSuffixes = true;
    m_schematic->RefreshHierarchy();

    std::set<wxString> composed;
    SCH_SHEET_PATH     leaf;

    for( const SCH_SHEET_PATH& path : m_schematic->Hierarchy() )
    {
        if( path.Last() == inner[0] || path.Last() == inner[1] )
            composed.insert( m_schematic->GetRefSuffix( path ) );

        if( path.Last() == inner[0] && path.at( path.size() - 2 ) == outer[0] )
            leaf = path;
    }

    BOOST_CHECK( composed == std::set<wxString>( { wxS( "_M1_a" ), wxS( "_M1" ), wxS( "_M2_b" ), wxS( "_M2" ) } ) );

    // A path that differs from the hierarchy's only in the virtual root hashes apart but has the same suffix
    SCH_SHEET_PATH rooted;
    rooted.push_back( &m_schematic->Root() );

    for( size_t i = 0; i < leaf.size(); i++ )
        rooted.push_back( leaf.at( i ) );

    BOOST_REQUIRE_NE( rooted.GetCurrentHash(), leaf.GetCurrentHash() );
    BOOST_CHECK_EQUAL( m_schematic->GetRefSuffix( rooted ), wxS( "_M1_a" ) );

    // Neither the suffix nor a field it reads takes a variant
    m_schematic->AddVariant( wxS( "Alt" ) );
    outer[0]->SetFieldText( wxS( "CH" ), wxS( "x" ), &root, wxS( "Alt" ) );
    m_schematic->SetCurrentVariant( wxS( "Alt" ) );
    m_schematic->RefreshHierarchy();
    BOOST_CHECK_EQUAL( m_schematic->GetRefSuffix( leaf ), wxS( "_M1_a" ) );

    outer[1]->SetFieldText( wxS( "Reference Suffix" ), wxS( "_X" ), &root, wxS( "Alt" ) );
    BOOST_CHECK_EQUAL( outer[1]->GetRefSuffix( root ), wxS( "_X" ) );

    // An override saved while the suffix was an ordinary field must not show through
    SCH_SHEET_VARIANT stale( wxS( "Alt" ) );
    stale.m_Fields[wxS( "Reference Suffix" )] = wxS( "_STALE" );
    outer[1]->AddVariant( root, stale );

    const SCH_FIELD* suffix = std::as_const( *outer[1] ).GetField( FIELD_T::SHEET_REF_SUFFIX );
    BOOST_CHECK_EQUAL( suffix->GetShownText( &root, INTERNAL, wxS( "Alt" ) ), wxS( "_X" ) );

    // Project, title block and file name values change outside commits and would leave the cache stale
    m_schematic->Project().GetTextVars()[wxS( "LOT" )] = wxS( "7" );
    outer[1]->AddField( SCH_FIELD( outer[1], FIELD_T::SHEET_USER, wxS( "ERC_WARNING_CH" ) ) )->SetText( wxS( "a" ) );
    outer[1]->GetField( wxS( "Reference Suffix" ) )
            ->SetText( wxS( "_${ERC_WARNING_CH}${LOT}${REVISION}${Sheetfile}" ) );

    BOOST_CHECK_EQUAL( outer[1]->GetRefSuffix( root ), wxS( "_a${LOT}${REVISION}${Sheetfile}" ) );

    // Autosave skips clean files, but the child file stores references composed with the edited suffix
    SCH_SHEET    before( *outer[0] );
    SCH_SCREEN*  child = outer[0]->GetScreen();
    TOOL_MANAGER manager;

    manager.SetEnvironment( m_schematic.get(), nullptr, nullptr, nullptr, nullptr );
    child->SetContentModified( false );

    SCH_COMMIT commit( &manager );
    commit.Modify( outer[0], root.LastScreen() );
    outer[0]->GetField( wxS( "CH" ) )->SetText( wxS( "c" ) );
    BOOST_CHECK( before.HasHierarchyChanges( *outer[0] ) );

    commit.Push( wxS( "Edit channel" ), SKIP_UNDO );
    BOOST_CHECK( child->IsContentModified() );
}


BOOST_AUTO_TEST_CASE( ChannelRefsCompose )
{
    Load( wxS( "issue13212" ) );

    std::vector<SCH_SHEET_PATH> channels = SuffixChannels();
    SCH_SYMBOL*                 symbol = FirstSymbol( channels[0] );
    wxString                    base = symbol->GetBaseRef( &channels[0] );

    BOOST_CHECK_EQUAL( symbol->GetRef( &channels[0] ), base + wxS( "_M1" ) );
    BOOST_CHECK_EQUAL( symbol->GetRef( &channels[1] ), symbol->GetBaseRef( &channels[1] ) + wxS( "_M2" ) );

    // Commit and undo write the field text back through SetRef, so it must hold the base
    channels[0].UpdateAllScreenReferences();
    BOOST_CHECK_EQUAL( symbol->GetField( FIELD_T::REFERENCE )->GetText(), base );

    // Identical default net names would merge the channels' nets on the board
    m_schematic->RebuildConnectivity();
    std::vector<wxString> names = SCH_CONNECTIVITY::NAVIGATION_QUERY( *m_schematic ).NetNames();

    for( const SCH_SHEET_PATH& channel : channels )
    {
        wxString prefix = wxS( "(" ) + symbol->GetRef( &channel ) + wxS( "-" );
        BOOST_CHECK( std::ranges::any_of( names, [&]( const wxString& n ) { return n.Contains( prefix ); } ) );
    }
}


BOOST_AUTO_TEST_CASE( UserTextMapsToBase )
{
    Load( wxS( "issue13212" ) );

    std::vector<SCH_SHEET_PATH> channels = SuffixChannels();
    SCH_SYMBOL*                 symbol = FirstSymbol( channels[1] );
    SCH_REFERENCE_LIST          refs;

    channels[0].AppendSymbol( refs, symbol, SYMBOL_FILTER_ALL );
    channels[1].AppendSymbol( refs, symbol, SYMBOL_FILTER_ALL );

    // The board carries composed references, and channels of one file must not split onto two bases
    std::vector<BACKANNOTATE_REF> same = ResolveBackannotatedRefs( { { &refs[0], wxS( "R7_M1" ) },
                                                                     { &refs[1], wxS( "R7_M2" ) } }, true );
    std::vector<BACKANNOTATE_REF> split = ResolveBackannotatedRefs( { { &refs[0], wxS( "R7_M1" ) },
                                                                      { &refs[1], wxS( "R1_M2" ) } }, true );

    BOOST_CHECK_EQUAL( same[0].m_base, wxS( "R7" ) );
    BOOST_CHECK( !split[0].m_error.IsEmpty() );

    SCH_SEARCH_DATA data;
    data.replaceReferences = true;
    data.matchMode = EDA_SEARCH_MATCH_MODE::PLAIN;
    data.findString = symbol->GetBaseRef( &channels[0] );
    data.replaceString = wxS( "R99" );

    symbol->GetField( FIELD_T::REFERENCE )->Replace( data, &channels[0] );
    BOOST_CHECK_EQUAL( symbol->GetRef( &channels[0] ), wxS( "R99_M1" ) );

    // Replacing into the suffix would store part of it in the base
    data.findString = wxS( "_M1" );
    data.replaceString = wxS( "_X" );
    BOOST_CHECK( !symbol->GetField( FIELD_T::REFERENCE )->Replace( data, &channels[0] ) );

    // Typing the shown reference must not double its suffix, and in suffix mode the edit reaches every channel
    m_schematic->Settings().m_AnnotateReusedSheetSuffixes = true;
    m_schematic->SetCurrentSheet( channels[1] );
    symbol->SetRefProp( wxS( "R77_M2" ) );
    BOOST_CHECK_EQUAL( symbol->GetRef( &channels[0] ), wxS( "R77_M1" ) );

    kiapi::schematic::types::SchematicSymbolInstance proto;
    BOOST_REQUIRE( PackSymbol( &proto, symbol, channels[1] ) );
    BOOST_CHECK_EQUAL( proto.reference_field().text().text(), "R77_M2" );

    // A dialog writes back the base it loaded, and a base that happens to end in its suffix must survive that
    m_schematic->Settings().m_AnnotateReusedSheetSuffixes = false;
    symbol->SetRef( &channels[1], wxS( "R5_M2" ) );
    symbol->SetRefProp( symbol->GetBaseRef( &channels[1] ) );
    BOOST_CHECK_EQUAL( symbol->GetBaseRef( &channels[1] ), wxS( "R5_M2" ) );

    // A sibling suffix that ends in this channel's suffix still names the sibling
    channels[0].Last()->GetField( FIELD_T::SHEET_REF_SUFFIX )->SetText( wxS( "_A" ) );
    channels[1].Last()->GetField( FIELD_T::SHEET_REF_SUFFIX )->SetText( wxS( "_B_A" ) );
    m_schematic->Settings().m_AnnotateReusedSheetSuffixes = true;
    m_schematic->RefreshHierarchy();

    wxString base;
    BOOST_CHECK( !symbol->BaseRefFromUser( channels[0], wxS( "R8_B_A" ), base ).IsEmpty() );

    // The shown reference echoed back through the API keeps its base
    kiapi::common::commands::ItemStatus status;
    symbol->SetRef( &channels[0], wxS( "R8_B" ) );
    BOOST_REQUIRE( PackSymbol( &proto, symbol, channels[0] ) );
    BOOST_CHECK( ApplySymbolInstance( &status, symbol, proto, channels[0], m_schematic.get() ) );
    BOOST_CHECK_EQUAL( symbol->GetBaseRef( &channels[0] ), wxS( "R8_B" ) );
}


BOOST_AUTO_TEST_CASE( ErcChecks )
{
    LoadCopy();

    // Without suffixes the two channels of one file cannot share a base
    m_schematic->Settings().m_AnnotateReusedSheetSuffixes = true;
    BOOST_CHECK_EQUAL( CountErcMarkers( ERCE_DUPLICATE_SHEET_SUFFIX ), 1 );

    // R7 and R8 are a valid separate numbering but disagree in suffix mode
    std::vector<SCH_SHEET_PATH> channels = SuffixChannels();
    const KIID_PATH             firstPath = channels[0].Path();
    BOOST_CHECK_EQUAL( CountErcMarkers( ERCE_REF_SUFFIX_MISMATCH ), 1 );

    // Configured suffix collisions do not matter when separate numbering ignores them
    m_schematic->Settings().m_AnnotateReusedSheetSuffixes = false;
    channels[1].Last()->GetField( FIELD_T::SHEET_REF_SUFFIX )->SetText( wxS( "_M1" ) );
    m_schematic->RefreshHierarchy();
    BOOST_CHECK_EQUAL( CountErcMarkers( ERCE_DUPLICATE_SHEET_SUFFIX ), 0 );

    channels[1].Last()->GetField( FIELD_T::SHEET_REF_SUFFIX )->SetText( wxS( "_M2" ) );
    m_schematic->RefreshHierarchy();

    // A channel file saved before its parent gained a suffix keeps plain refs
    SCH_IO_KICAD_SEXPR().SaveSchematicFile( RootFile().GetFullPath(), m_schematic->GetTopLevelSheets()[0],
                                            m_schematic.get() );
    LoadSchematic( RootFile() );
    m_schematic->Settings().m_AnnotateReusedSheetSuffixes = false;
    m_schematic->RefreshRefSuffixes();
    BOOST_CHECK_EQUAL( CountErcMarkers( ERCE_REF_SUFFIX_MISMATCH ), 0 );
    m_schematic->Settings().m_AnnotateReusedSheetSuffixes = true;
    m_schematic->RefreshRefSuffixes();
    BOOST_CHECK_EQUAL( CountErcMarkers( ERCE_REF_SUFFIX_MISMATCH ), 2 );

    std::optional<SCH_SHEET_PATH> path = m_schematic->Hierarchy().GetSheetPathByKIIDPath( firstPath );
    BOOST_REQUIRE( path );

    FirstSymbol( *path )->SetRef( &*path, wxS( "R99" ) );
    BOOST_CHECK_EQUAL( CountErcMarkers( ERCE_REF_SUFFIX_MISMATCH ), 1 );
}


BOOST_AUTO_TEST_CASE( ErcChecksComposedPackageValues )
{
    std::vector<SCH_SHEET_PATH> channels = LoadAmpliChannels();
    SCH_SHEET_PATH              root = m_schematic->Hierarchy()[0];
    SCH_SYMBOL*                 source = FindSymbol( channels[0], wxS( "U201_V" ), 1 );
    BOOST_REQUIRE( source );
    m_schematic->Settings().m_AnnotateReusedSheetSuffixes = false;

    SCH_SYMBOL* unitA = new SCH_SYMBOL( *source->GetLibSymbolRef(), source->GetLibId(), &channels[0], 1 );
    SCH_SYMBOL* unitB = new SCH_SYMBOL( *source->GetLibSymbolRef(), source->GetLibId(), &root, 2 );
    channels[0].LastScreen()->Append( unitA );
    root.LastScreen()->Append( unitB );
    unitA->SetRef( &channels[0], wxS( "U777" ) );
    unitA->SetRef( &channels[1], wxS( "U778" ) );
    unitB->SetRef( &root, wxS( "U777_V2" ) );
    unitA->GetField( FIELD_T::VALUE )->SetText( wxS( "first-value" ) );
    unitB->GetField( FIELD_T::VALUE )->SetText( wxS( "second-value" ) );

    channels[0].Last()->GetField( FIELD_T::SHEET_REF_SUFFIX )->SetText( wxS( "_V2" ) );
    m_schematic->Settings().m_AnnotateReusedSheetSuffixes = true;
    m_schematic->RefreshHierarchy();

    BOOST_REQUIRE_EQUAL( unitA->GetRef( &channels[0] ), unitB->GetRef( &root ) );
    BOOST_CHECK_EQUAL( CountErcMarkers( ERCE_DIFFERENT_UNIT_VALUE ), 1 );

    unitB->GetField( FIELD_T::VALUE )->SetText( wxS( "first-value" ) );
    BOOST_CHECK_EQUAL( CountErcMarkers( ERCE_DIFFERENT_UNIT_VALUE ), 0 );
}


// The channel file starts with R7 and R8, which suffix mode must unify into one base
BOOST_AUTO_TEST_CASE( SuffixModeSharesOneBase )
{
    Load( wxS( "issue13212" ) );

    std::vector<SCH_SHEET_PATH> channels = SuffixChannels();
    SCH_SYMBOL*                 symbol = FirstSymbol( channels[0] );

    AnnotateAll( true );

    BOOST_CHECK_EQUAL( symbol->GetRef( &channels[1] ), symbol->GetBaseRef( &channels[0] ) + wxS( "_M2" ) );

    // Seven symbols take seven numbers, the channels' shared base among them
    std::set<wxString> bases;

    for( const SCH_SHEET_PATH& path : m_schematic->Hierarchy() )
    {
        for( SCH_ITEM* item : path.LastScreen()->Items().OfType( SCH_SYMBOL_T ) )
            bases.insert( static_cast<SCH_SYMBOL*>( item )->GetBaseRef( &path ) );
    }

    BOOST_CHECK( bases == std::set<wxString>( { wxS( "R1" ), wxS( "R2" ), wxS( "R3" ), wxS( "R4" ), wxS( "R5" ),
                                                wxS( "R6" ), wxS( "R7" ) } ) );

    // A symbol placed on a reused sheet takes one base for every channel
    for( const SCH_SHEET_PATH& channel : channels )
        symbol->SetRef( &channel, symbol->GetPrefix() + wxS( "?" ) );

    AnnotatePlaced( symbol, channels );

    BOOST_CHECK( !symbol->GetBaseRef( &channels[0] ).EndsWith( wxS( "?" ) ) );
    BOOST_CHECK_EQUAL( symbol->GetBaseRef( &channels[1] ), symbol->GetBaseRef( &channels[0] ) );
}


BOOST_AUTO_TEST_CASE( SeparateNumberingIgnoresConfiguredSuffixes )
{
    Load( wxS( "issue13212" ) );
    std::vector<SCH_SHEET_PATH> channels = SuffixChannels();
    SCH_SYMBOL*                 symbol = FirstSymbol( channels[0] );
    BOOST_REQUIRE( symbol );

    m_schematic->Settings().m_AnnotateReusedSheetSuffixes = false;
    m_schematic->RefreshRefSuffixes();

    BOOST_CHECK_EQUAL( channels[0].Last()->GetRefSuffix( m_schematic->Hierarchy()[0] ), wxS( "_M1" ) );
    BOOST_CHECK( !m_schematic->HasRefSuffixes() );
    BOOST_CHECK( m_schematic->GetRefSuffix( channels[0] ).IsEmpty() );
    BOOST_CHECK_EQUAL( symbol->GetRef( &channels[0] ), symbol->GetBaseRef( &channels[0] ) );

    symbol->SetRef( &channels[0], wxS( "R?" ) );
    SCH_REFERENCE_LIST           refs;
    SCH_MULTI_UNIT_REFERENCE_MAP locked;
    channels[0].AppendSymbol( refs, symbol, SYMBOL_FILTER_NON_POWER );
    channels[0].AppendMultiUnitSymbol( locked, symbol, SYMBOL_FILTER_NON_POWER );
    AnnotateReferences( refs, locked, *m_schematic, ANNOTATE_SELECTION, SORT_BY_X_POSITION, INCREMENTAL_BY_REF, 0,
                        true );

    BOOST_CHECK_EQUAL( symbol->GetRef( &channels[0] ), symbol->GetBaseRef( &channels[0] ) );
    BOOST_CHECK( !symbol->GetRef( &channels[0] ).EndsWith( wxS( "_M1" ) ) );

    m_schematic->Settings().m_AnnotateReusedSheetSuffixes = true;
    m_schematic->RefreshRefSuffixes();
    BOOST_CHECK_EQUAL( symbol->GetRef( &channels[0] ), symbol->GetBaseRef( &channels[0] ) + wxS( "_M1" ) );
}


BOOST_AUTO_TEST_CASE( SeparateNumberingResetUsesDistinctPlainReferences )
{
    Load( wxS( "issue13212" ) );
    std::vector<SCH_SHEET_PATH> channels = SuffixChannels();
    SCH_SHEET_PATH              root = m_schematic->Hierarchy()[0];
    SCH_SYMBOL*                 channel = FirstSymbol( channels[0] );
    SCH_SYMBOL*                 other = FindSymbol( root, wxS( "R1" ) );
    BOOST_REQUIRE( channel && other );

    other->SetRef( &root, wxS( "R1_M2" ) );
    m_schematic->Settings().m_AnnotateReusedSheetSuffixes = false;
    m_schematic->RefreshRefSuffixes();

    SCH_SHEET_LIST               sheets = m_schematic->Hierarchy();
    SCH_REFERENCE_LIST           refs;
    SCH_MULTI_UNIT_REFERENCE_MAP locked;
    sheets.GetSymbols( refs, SYMBOL_FILTER_NON_POWER );
    sheets.GetMultiUnitSymbols( locked, SYMBOL_FILTER_NON_POWER );
    AnnotateReferences( refs, locked, *m_schematic, ANNOTATE_ALL, SORT_BY_X_POSITION, INCREMENTAL_BY_REF, 0, true );

    BOOST_CHECK_EQUAL( CountDuplicates(), 0 );
    BOOST_CHECK_NE( channel->GetRef( &channels[0] ), channel->GetRef( &channels[1] ) );
    BOOST_CHECK_EQUAL( channel->GetRef( &channels[0] ), channel->GetBaseRef( &channels[0] ) );
    BOOST_CHECK_EQUAL( channel->GetRef( &channels[1] ), channel->GetBaseRef( &channels[1] ) );
    BOOST_CHECK( !channel->GetRef( &channels[0] ).EndsWith( wxS( "_M1" ) ) );
    BOOST_CHECK( !channel->GetRef( &channels[1] ).EndsWith( wxS( "_M2" ) ) );
}


BOOST_AUTO_TEST_CASE( SuffixModeNewReferenceAvoidsComposedCollision )
{
    Load( wxS( "issue13212" ) );

    std::vector<SCH_SHEET_PATH> channels = SuffixChannels();
    SCH_SHEET_PATH              root = m_schematic->Hierarchy()[0];
    SCH_SYMBOL*                 channel = FirstSymbol( channels[0] );
    SCH_SYMBOL*                 other = FindSymbol( root, wxS( "R1" ) );
    BOOST_REQUIRE( channel && other );

    other->SetRef( &root, wxS( "R1_M2" ) );

    for( const SCH_SHEET_PATH& path : channels )
        channel->SetRef( &path, wxS( "R?" ) );

    AnnotateSelection( { channel }, channels[0] );

    BOOST_CHECK_EQUAL( other->GetRef( &root ), wxS( "R1_M2" ) );
    BOOST_CHECK_EQUAL( channel->GetRef( &channels[0] ), wxS( "R7_M1" ) );
    BOOST_CHECK_EQUAL( channel->GetRef( &channels[1] ), wxS( "R7_M2" ) );
}


// A variable can give a suffix that runs into the base number, so R91 with suffix 2 reads R912
BOOST_AUTO_TEST_CASE( SuffixModeRenumbersComposedCollision )
{
    Load( wxS( "issue13212" ) );

    for( const SCH_SHEET_PATH& path : PathsOf( wxS( "issue13212_subsheet_2.kicad_sch" ) ) )
        path.Last()->AddField( SCH_FIELD( path.Last(), FIELD_T::SHEET_USER, wxS( "CH" ) ) )->SetText( wxS( "2" ) );

    std::vector<SCH_SHEET_PATH> channels = SuffixChannels( wxS( "_A" ), wxS( "${CH}" ) );
    std::vector<SCH_SHEET_PATH> singles = PathsOf( wxS( "issue13212_subsheet_1.kicad_sch" ) );
    BOOST_REQUIRE( !singles.empty() && m_schematic->GetRefSuffix( channels[1] ) == wxS( "2" ) );

    SCH_SHEET_PATH single = singles[0];
    SCH_SYMBOL*    channel = FirstSymbol( channels[0] );
    SCH_SYMBOL*    other = FirstSymbol( single );

    channel->SetRef( &channels[0], wxS( "R91" ) );
    channel->SetRef( &channels[1], wxS( "R92" ) );
    other->SetRef( &single, wxS( "R912" ) );
    AnnotateSelection( { channel }, channels[0] );

    BOOST_CHECK_NE( channel->GetRef( &channels[1] ), other->GetRef( &single ) );

    // R91 with suffix A1 and a bare R91A1 split into different prefixes yet show the same reference
    channels[1].Last()->GetField( FIELD_T::SHEET_REF_SUFFIX )->SetText( wxS( "A1" ) );
    m_schematic->RefreshHierarchy();

    channel->SetRef( &channels[0], wxS( "R91" ) );
    channel->SetRef( &channels[1], wxS( "R91" ) );
    other->SetRef( &single, wxS( "R91A1" ) );
    BOOST_CHECK_EQUAL( CountDuplicates(), 1 );

    // Two kept bases in one run differ yet compose alike
    channel->SetRef( &channels[1], wxS( "R92" ) );
    AnnotateAll();

    BOOST_CHECK_NE( channel->GetRef( &channels[1] ), other->GetRef( &single ) );
}


BOOST_AUTO_TEST_CASE( SuffixModeAnnotatesPackages )
{
    std::vector<SCH_SHEET_PATH> channels = LoadAmpliChannels();
    SCH_SYMBOL*                 x = FindSymbol( channels[0], wxS( "R203_V" ) );
    SCH_SYMBOL*                 y = FindSymbol( channels[0], wxS( "R213_V" ) );
    SCH_SYMBOL*                 unitA = FindSymbol( channels[0], wxS( "U201_V" ), 1 );
    SCH_SYMBOL*                 unitB = FindSymbol( channels[0], wxS( "U201_V" ), 2 );
    BOOST_REQUIRE( x && y && unitA && unitB );

    // Separate numbering may swap bases between channels, and sharing X's kept base must not land on Y
    x->SetRef( &channels[1], wxS( "R213" ) );
    y->SetRef( &channels[1], wxS( "R203" ) );
    AnnotateSelection( { x }, channels[1] );

    BOOST_CHECK_EQUAL( x->GetBaseRef( &channels[1] ), x->GetBaseRef( &channels[0] ) );
    BOOST_CHECK_NE( x->GetRef( &channels[1] ), y->GetRef( &channels[1] ) );

    // Only unit A's base and unit are taken on the other channel, so renumbering it must take unit B along
    x->SetRef( &channels[1], wxS( "U201" ) );
    AnnotateSelection( { unitA, unitB }, channels[0] );

    BOOST_CHECK_EQUAL( unitA->GetBaseRef( &channels[0] ), unitB->GetBaseRef( &channels[0] ) );
    BOOST_CHECK_NE( unitA->GetBaseRef( &channels[0] ), wxS( "U201" ) );

    // A placed unit B joins the U1 package unit A holds on every channel
    for( const SCH_SHEET_PATH& channel : channels )
    {
        unitA->SetRef( &channel, wxS( "U1" ) );
        unitB->SetRef( &channel, wxS( "U?" ) );
    }

    m_schematic->Settings().m_AnnotateMethod = ANNOTATE_ALGO_T::INCREMENTAL_BY_REF;
    m_schematic->Settings().m_AnnotateStartNum = 0;
    AnnotatePlaced( unitB, channels );

    BOOST_CHECK_EQUAL( unitB->GetBaseRef( &channels[0] ), wxS( "U1" ) );
    BOOST_CHECK_EQUAL( unitB->GetBaseRef( &channels[1] ), wxS( "U1" ) );

    // Copies that select different units pair into packages instead of sharing a base
    unitA->SetUnitSelection( &channels[1], 2 );
    unitB->SetUnitSelection( &channels[1], 1 );
    unitA->ClearAnnotation( nullptr, false );
    unitB->ClearAnnotation( nullptr, false );
    AnnotateAll();

    std::map<wxString, std::set<int>> packages;

    for( SCH_SYMBOL* symbol : { unitA, unitB } )
    {
        for( const SCH_SHEET_PATH& channel : channels )
            packages[symbol->GetRef( &channel )].insert( symbol->GetUnitSelection( &channel ) );
    }

    BOOST_CHECK_EQUAL( packages.size(), 2 );

    for( const auto& [package, units] : packages )
        BOOST_CHECK_MESSAGE( !package.Contains( wxS( "?" ) ) && units == std::set<int>( { 1, 2 } ), package );
}


// Older versions store every unit of a package with its lowest suffix, which loading must strip
BOOST_AUTO_TEST_CASE( LoadStripsDowngradedPackageSuffix )
{
    std::vector<SCH_SHEET_PATH> channels = LoadAmpliChannels();
    SCH_SYMBOL*                 unitA = FindSymbol( channels[0], wxS( "U201_V" ), 1 );
    SCH_SYMBOL*                 unitB = FindSymbol( channels[0], wxS( "U201_V" ), 2 );
    BOOST_REQUIRE( unitA && unitB );

    // As a v9 file stores U9 (unit A on _H, unit B on _V) and U8 (unit A on _V, unit B on _H)
    unitA->SetRef( &channels[1], wxS( "U9_H" ) );
    unitB->SetRef( &channels[0], wxS( "U9_H" ) );
    unitA->SetRef( &channels[0], wxS( "U8_H" ) );
    unitB->SetRef( &channels[1], wxS( "U8_H" ) );

    m_schematic->Hierarchy().StripRefSuffixes();

    BOOST_CHECK_EQUAL( unitB->GetBaseRef( &channels[0] ), wxS( "U9" ) );
    BOOST_CHECK_EQUAL( unitA->GetBaseRef( &channels[0] ), wxS( "U8" ) );

    // Each top level sheet strips its references alone, so a package spanning two recovers once both exist.
    // U9 (A on _H2_V, B on _V) keeps U9_H2_V whole and U7 (B on _H2_V, A on _V) strips U7_H2_V to U7_H2.
    channels[1].Last()->GetField( wxS( "Reference Suffix" ) )->SetText( wxS( "_H2_V" ) );
    m_schematic->RefreshHierarchy();
    unitA->SetRef( &channels[1], wxS( "U9" ) );
    unitB->SetRef( &channels[0], wxS( "U9_H2_V" ) );
    unitB->SetRefLacksSuffix( channels[0].Path() );
    unitB->SetRef( &channels[1], wxS( "U7" ) );
    unitA->SetRef( &channels[0], wxS( "U7_H2" ) );

    SCH_SHEET*  other = new SCH_SHEET( m_schematic.get() );
    SCH_SCREEN* screen = new SCH_SCREEN( m_schematic.get() );
    other->SetScreen( screen );
    screen->SetFileName( wxS( "other.kicad_sch" ) );

    m_schematic->SetTopLevelSheets( { m_schematic->GetTopLevelSheets()[0], other } );

    BOOST_CHECK_EQUAL( unitB->GetBaseRef( &channels[0] ), wxS( "U9" ) );
    BOOST_CHECK_EQUAL( unitA->GetBaseRef( &channels[0] ), wxS( "U7" ) );
}


// Every output names a part by the reference the schematic composes for it
BOOST_AUTO_TEST_CASE( OutputsAgreeOnComposedRefs )
{
    std::vector<SCH_SHEET_PATH> channels = LoadAmpliChannels();
    SCH_SYMBOL*                 unitA = FindSymbol( channels[0], wxS( "U201_V" ), 1 );
    SCH_SYMBOL*                 unitB = FindSymbol( channels[0], wxS( "U201_V" ), 2 );
    SCH_SYMBOL*                 low = FindSymbol( channels[0], wxS( "R212_V" ) );
    BOOST_REQUIRE( unitA && unitB && low );

    KI_TEST::SCOPED_TEMP_DIR temp( "ref-suffix-outputs" );
    NULL_REPORTER            reporter;
    const wxString           netFile = temp.PathStr() + wxS( "/netlist.net" );
    const wxString           xmlFile = temp.PathStr() + wxS( "/netlist.xml" );

    auto read =
            []( const wxString& aFile )
            {
                wxString text;
                BOOST_REQUIRE( wxFFile( aFile, wxS( "r" ) ).ReadAll( &text ) );
                return text;
            };

    // Allegro groups like parts by prefix and number, and R30_V sorts after R301_H as text
    low->SetRef( &channels[0], wxS( "R30" ) );
    BOOST_REQUIRE( NETLIST_EXPORTER_ALLEGRO( m_schematic.get(), nullptr ).WriteNetlist( netFile, 0, reporter ) );
    BOOST_CHECK( read( netFile ).Contains( wxS( "; R30_V,\n\tR201_V,\n\tR208_V,\n\tR301_H," ) ) );

    // Suffix mode gives both channels one base, so U201 is a dual op amp in each channel
    m_schematic->Settings().m_AnnotateReusedSheetSuffixes = true;

    for( SCH_ITEM* item : channels[0].LastScreen()->Items().OfType( SCH_SYMBOL_T ) )
    {
        SCH_SYMBOL* symbol = static_cast<SCH_SYMBOL*>( item );

        if( !symbol->GetRef( &channels[0] ).StartsWith( wxS( "#" ) ) )
            symbol->SetRef( &channels[1], symbol->GetBaseRef( &channels[0] ) );
    }

    BOOST_CHECK_EQUAL( CountDuplicates(), 0 );

    // Annotation never shares power symbol refs, so their per-channel numbers are not a mismatch
    BOOST_CHECK_EQUAL( CountErcMarkers( ERCE_REF_SUFFIX_MISMATCH ), 0 );

    SCH_MULTI_UNIT_REFERENCE_MAP packages;
    m_schematic->Hierarchy().GetMultiUnitSymbols( packages, SYMBOL_FILTER_NON_POWER );
    BOOST_CHECK_EQUAL( packages[wxS( "U201_V" )].GetCount(), 2 );
    BOOST_CHECK_EQUAL( packages[wxS( "U201_H" )].GetCount(), 2 );

    // U9 spans the channels, so one footprint U9_H holds unit A on _H and unit B on _V
    unitA->SetRef( &channels[0], wxS( "U8" ) );
    unitB->SetRef( &channels[1], wxS( "U8" ) );
    unitA->SetRef( &channels[1], wxS( "U9" ) );
    unitB->SetRef( &channels[0], wxS( "U9" ) );

    BOOST_CHECK_EQUAL( unitB->GetRef( &channels[0] ), wxS( "U9_H" ) );
    BOOST_CHECK_EQUAL( unitB->GetRef( &channels[0], true ), wxS( "U9B_V" ) );

    std::set<wxString> expected;

    for( const SCH_SHEET_PATH& path : m_schematic->Hierarchy() )
    {
        for( SCH_ITEM* item : path.LastScreen()->Items().OfType( SCH_SYMBOL_T ) )
        {
            const wxString ref = static_cast<SCH_SYMBOL*>( item )->GetRef( &path );

            if( !ref.StartsWith( wxS( "#" ) ) )
                expected.insert( ref );
        }
    }

    auto refsIn =
            []( const wxString& aText, const wxString& aPattern )
            {
                std::set<wxString> refs;
                wxRegEx            re( aPattern, wxRE_ADVANCED );
                wxString           rest = aText;
                size_t             start = 0;
                size_t             len = 0;

                while( re.Matches( rest ) && re.GetMatch( &start, &len, 0 ) )
                {
                    refs.insert( re.GetMatch( rest, 1 ) );
                    rest = rest.Mid( start + len );
                }

                return refs;
            };

    // Update PCB reads this netlist
    STRING_FORMATTER formatter;
    NETLIST_EXPORTER_KICAD( m_schematic.get(), nullptr ).Format( &formatter, GNL_ALL | GNL_OPT_KICAD );
    BOOST_CHECK( refsIn( formatter.GetString(), wxS( "\\(comp \\(ref \"([^\"]+)\"\\)" ) ) == expected );

    BOOST_REQUIRE( NETLIST_EXPORTER_XML( m_schematic.get(), nullptr ).WriteNetlist( xmlFile, 0, reporter ) );
    BOOST_CHECK( refsIn( read( xmlFile ), wxS( "<comp ref=\"([^\"]+)\"" ) ) == expected );

    // Allegro starts each package's references after "; " and puts the rest on tab-indented lines
    BOOST_REQUIRE( NETLIST_EXPORTER_ALLEGRO( m_schematic.get(), nullptr ).WriteNetlist( netFile, 0, reporter ) );

    wxString allegro = read( netFile );
    allegro = allegro.Mid( allegro.Find( wxS( "$PACKAGES\n" ) ) + 10 );
    BOOST_CHECK( refsIn( allegro.Left( allegro.Find( wxS( "$" ) ) ), wxS( "(?:; |\\n\\t)([^,\\s]+)" ) ) == expected );

    SCH_REFERENCE_LIST refs;
    m_schematic->Hierarchy().GetSymbols( refs, SYMBOL_FILTER_NON_POWER );

    SYMBOL_FIELDS_EDITOR_GRID_DATA_MODEL bom( refs );
    bom.AddColumn( GetDefaultFieldName( FIELD_T::REFERENCE, UNTRANSLATED ), wxS( "Reference" ), false );
    bom.SetGroupingEnabled( false );
    bom.RebuildRows();

    std::set<wxString> listed;

    for( int row = 0; row < bom.GetNumberRows(); ++row )
    {
        wxStringTokenizer tokens( bom.GetValue( row, 0 ), wxS( ", " ), wxTOKEN_STRTOK );

        while( tokens.HasMoreTokens() )
            listed.insert( tokens.GetNextToken() );
    }

    BOOST_CHECK( listed == expected );

    // With the unit that names the package left off the board, the other unit still places it
    channels[1].Last()->GetField( wxS( "Reference Suffix" ) )->SetText( wxS( "_Z" ) );
    m_schematic->RefreshHierarchy();
    unitB->SetExcludedFromBoard( true, &channels[0] );

    formatter.Clear();
    NETLIST_EXPORTER_KICAD( m_schematic.get(), nullptr ).Format( &formatter, GNL_ALL | GNL_OPT_KICAD );

    wxString netlist = formatter.GetString();
    int      comp = netlist.Find( wxS( "(comp (ref \"U9_V\")" ) );
    BOOST_REQUIRE( comp != wxNOT_FOUND );

    wxString block = netlist.Mid( comp + 1 );
    block = block.Left( block.Find( wxS( "(comp " ) ) );
    BOOST_CHECK( block.Contains( wxS( "(tstamps \"" ) + channels[1].PathAsString() + wxS( "\")" ) ) );
}


BOOST_AUTO_TEST_SUITE_END()
