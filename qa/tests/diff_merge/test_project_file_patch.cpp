/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
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
 * http://www.gnu.org/licenses/gpl-3.0.html
 * or you may search the http://www.gnu.org website for the version 3 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#include <boost/test/unit_test.hpp>

#include <diff_merge/project_file_patch.h>
#include <diff_merge/diff_doc_kind.h>
#include <diff_merge/kicad_diff_types.h>

#include <json_common.h>

#include <wx/file.h>
#include <wx/filename.h>
#include <wx/stdpaths.h>
#include <wx/datetime.h>


using namespace KICAD_DIFF;
using nlohmann::json;


BOOST_AUTO_TEST_SUITE( ProjectFilePatch )


// Each DOC_PROP that lives in the project file must map to a json pointer.
BOOST_AUTO_TEST_CASE( DocPropPointerMappingDrcSeverities )
{
    auto p = DocPropJsonPointer( DOC_PROP_DRC_SEVERITIES );
    BOOST_REQUIRE( p.has_value() );
    BOOST_CHECK_EQUAL( *p, "/board/design_settings/rule_severities" );
}


BOOST_AUTO_TEST_CASE( DocPropPointerMappingErcSeverities )
{
    auto p = DocPropJsonPointer( DOC_PROP_ERC_SEVERITIES );
    BOOST_REQUIRE( p.has_value() );
    BOOST_CHECK_EQUAL( *p, "/erc" );
}


BOOST_AUTO_TEST_CASE( DocPropPointerMappingDrawingSheet )
{
    auto p = DocPropJsonPointer( DOC_PROP_DRAWING_SHEET );
    BOOST_REQUIRE( p.has_value() );
    BOOST_CHECK_EQUAL( *p, "/pcbnew/page_layout_descr_file" );
}


BOOST_AUTO_TEST_CASE( DocPropPointerMappingDrawingSheetSchematic )
{
    auto p = DocPropJsonPointer( DOC_PROP_DRAWING_SHEET, DOC_KIND::SCH );
    BOOST_REQUIRE( p.has_value() );
    BOOST_CHECK_EQUAL( *p, "/schematic/page_layout_descr_file" );
}


BOOST_AUTO_TEST_CASE( DocPropPointerMappingNetClasses )
{
    auto p = DocPropJsonPointer( DOC_PROP_NET_CLASSES );
    BOOST_REQUIRE( p.has_value() );
    BOOST_CHECK_EQUAL( *p, "/net_settings" );
}


// DOC_PROPs whose content lives in a sibling file (not .kicad_pro) must NOT
// have a pointer mapping -- the patch code would silently misroute them
// otherwise.
BOOST_AUTO_TEST_CASE( DocPropPointerMappingCustomRulesIsAbsent )
{
    BOOST_CHECK( !DocPropJsonPointer( DOC_PROP_CUSTOM_RULES ).has_value() );
}


BOOST_AUTO_TEST_CASE( DocPropPointerMappingFpLibTableIsAbsent )
{
    BOOST_CHECK( !DocPropJsonPointer( DOC_PROP_FP_LIB_TABLE ).has_value() );
}


BOOST_AUTO_TEST_CASE( DocPropPointerMappingSymLibTableIsAbsent )
{
    BOOST_CHECK( !DocPropJsonPointer( DOC_PROP_SYM_LIB_TABLE ).has_value() );
}


// Patching DRC severities copies just that sub-tree; other fields in target
// are preserved.
BOOST_AUTO_TEST_CASE( ApplyPatchPreservesNonTargetFields )
{
    json target = {
        { "board", { { "design_settings", { { "rule_severities", { { "old_key", "warning" } } } } } } },
        { "text_variables", { { "USER_VAR", "user value" } } },
        { "schematic", { { "legacy_lib_dir", "/old/path" } } }
    };

    json source = {
        { "board", { { "design_settings", { { "rule_severities", { { "new_key", "error" } } } } } } }
    };

    BOOST_REQUIRE( ApplyProjectFilePatch( target, source, DOC_PROP_DRC_SEVERITIES ) );

    // Patched field: replaced.
    BOOST_CHECK_EQUAL( target["board"]["design_settings"]["rule_severities"]["new_key"], "error" );
    BOOST_CHECK( !target["board"]["design_settings"]["rule_severities"].contains( "old_key" ) );

    // Non-patched fields: preserved.
    BOOST_CHECK_EQUAL( target["text_variables"]["USER_VAR"], "user value" );
    BOOST_CHECK_EQUAL( target["schematic"]["legacy_lib_dir"], "/old/path" );
}


// Patching a DOC_PROP whose pointer is missing in the source returns false
// and leaves target untouched.
BOOST_AUTO_TEST_CASE( ApplyPatchNoOpWhenSourceMissingField )
{
    json target = { { "board", { { "design_settings", { { "rule_severities", { { "k", "warning" } } } } } } } };
    json source = { { "unrelated", "value" } };

    BOOST_CHECK( !ApplyProjectFilePatch( target, source, DOC_PROP_DRC_SEVERITIES ) );
    BOOST_CHECK_EQUAL( target["board"]["design_settings"]["rule_severities"]["k"], "warning" );
}


// A DOC_PROP not in the mapping table returns false even when the JSON
// contains a similarly-named field.
BOOST_AUTO_TEST_CASE( ApplyPatchUnmappedPropertyReturnsFalse )
{
    json target = json::object();
    json source = { { "anything", 1 } };

    BOOST_CHECK( !ApplyProjectFilePatch( target, source, DOC_PROP_CUSTOM_RULES ) );
}


// Patching a nested object (net_settings) copies the whole sub-tree.
BOOST_AUTO_TEST_CASE( ApplyPatchNetClassesCopiesSubTree )
{
    json target = {
        { "net_settings", { { "classes", json::array() } } },
        { "schematic", { { "preserved", true } } }
    };

    json source = {
        { "net_settings", {
            { "classes", { { { "name", "HighSpeed" }, { "priority", 1 } } } },
            { "netclass_patterns", { { { "pattern", "DDR_*" }, { "netclass", "HighSpeed" } } } }
        } }
    };

    BOOST_REQUIRE( ApplyProjectFilePatch( target, source, DOC_PROP_NET_CLASSES ) );

    BOOST_CHECK_EQUAL( target["net_settings"]["classes"].size(), 1u );
    BOOST_CHECK_EQUAL( target["net_settings"]["netclass_patterns"].size(), 1u );
    BOOST_CHECK_EQUAL( target["schematic"]["preserved"], true );
}


BOOST_AUTO_TEST_CASE( ApplyPatchDrawingSheetSchematicDoesNotTouchPcbnew )
{
    json target = {
        { "pcbnew", { { "page_layout_descr_file", "target-pcb.kicad_wks" } } },
        { "schematic", { { "page_layout_descr_file", "target-sch.kicad_wks" } } }
    };

    json source = {
        { "pcbnew", { { "page_layout_descr_file", "source-pcb.kicad_wks" } } },
        { "schematic", { { "page_layout_descr_file", "source-sch.kicad_wks" } } }
    };

    BOOST_REQUIRE( ApplyProjectFilePatch( target, source, DOC_PROP_DRAWING_SHEET, DOC_KIND::SCH ) );

    BOOST_CHECK_EQUAL( target["schematic"]["page_layout_descr_file"], "source-sch.kicad_wks" );
    BOOST_CHECK_EQUAL( target["pcbnew"]["page_layout_descr_file"], "target-pcb.kicad_wks" );
}


// File orchestrator: missing output file -> writes source verbatim (with the
// requested fields treated as identity copies).
BOOST_AUTO_TEST_CASE( ApplyPatchesNoExistingFileWritesSource )
{
    wxString tmpDir = wxStandardPaths::Get().GetTempDir() + wxFILE_SEP_PATH
                      + wxS( "kicad_patch_" )
                      + wxString::Format( wxS( "%d" ),
                                          static_cast<int>( wxDateTime::Now().GetValue().GetValue() & 0xffffff ) );
    BOOST_REQUIRE( wxFileName::Mkdir( tmpDir, wxS_DIR_DEFAULT, wxPATH_MKDIR_FULL ) );

    const wxString outputPath = tmpDir + wxFILE_SEP_PATH + wxS( "test.kicad_pro" );
    BOOST_REQUIRE( !wxFileExists( outputPath ) );

    json source = { { "board", { { "design_settings", { { "rule_severities", { { "k", "error" } } } } } } } };

    std::set<wxString> docProps = { DOC_PROP_DRC_SEVERITIES };
    BOOST_REQUIRE( ApplyProjectFilePatches( outputPath, source, docProps ) );

    BOOST_REQUIRE( wxFileExists( outputPath ) );

    wxFile in( outputPath );
    wxString content;
    in.ReadAll( &content );
    json parsed = json::parse( content.ToStdString() );
    BOOST_CHECK_EQUAL( parsed["board"]["design_settings"]["rule_severities"]["k"], "error" );

    wxRemoveFile( outputPath );
    wxFileName::Rmdir( tmpDir );
}


BOOST_AUTO_TEST_CASE( ApplyPatchesEmptyPropertySetDoesNotCreateOutput )
{
    wxString tmpDir = wxStandardPaths::Get().GetTempDir() + wxFILE_SEP_PATH
                      + wxS( "kicad_patch_" )
                      + wxString::Format( wxS( "%d" ),
                                          static_cast<int>( wxDateTime::Now().GetValue().GetValue() & 0xffffff ) );
    BOOST_REQUIRE( wxFileName::Mkdir( tmpDir, wxS_DIR_DEFAULT, wxPATH_MKDIR_FULL ) );

    const wxString outputPath = tmpDir + wxFILE_SEP_PATH + wxS( "test.kicad_pro" );
    BOOST_REQUIRE( !wxFileExists( outputPath ) );

    json source = { { "text_variables", { { "USER_VAR", "source value" } } } };
    std::set<wxString> docProps;

    BOOST_REQUIRE( ApplyProjectFilePatches( outputPath, source, docProps ) );
    BOOST_CHECK( !wxFileExists( outputPath ) );

    if( wxFileExists( outputPath ) )
        wxRemoveFile( outputPath );

    wxFileName::Rmdir( tmpDir );
}


// File orchestrator: existing output file is patched -- only the touched
// fields are updated, all others preserved verbatim.
BOOST_AUTO_TEST_CASE( ApplyPatchesPreservesExistingNonTargetFields )
{
    wxString tmpDir = wxStandardPaths::Get().GetTempDir() + wxFILE_SEP_PATH
                      + wxS( "kicad_patch_" )
                      + wxString::Format( wxS( "%d" ),
                                          static_cast<int>( wxDateTime::Now().GetValue().GetValue() & 0xffffff ) );
    BOOST_REQUIRE( wxFileName::Mkdir( tmpDir, wxS_DIR_DEFAULT, wxPATH_MKDIR_FULL ) );

    const wxString outputPath = tmpDir + wxFILE_SEP_PATH + wxS( "test.kicad_pro" );

    json existing = {
        { "text_variables", { { "USER_VAR", "user value" } } },
        { "board", { { "design_settings", { { "rule_severities", { { "old", "warning" } } } } } } }
    };

    {
        wxFile out;
        BOOST_REQUIRE( out.Open( outputPath, wxFile::write ) );
        BOOST_REQUIRE( out.Write( wxString::FromUTF8( existing.dump( 2 ) ) ) );
    }

    json source = { { "board", { { "design_settings", { { "rule_severities", { { "new", "error" } } } } } } } };

    std::set<wxString> docProps = { DOC_PROP_DRC_SEVERITIES };
    BOOST_REQUIRE( ApplyProjectFilePatches( outputPath, source, docProps ) );

    wxFile in( outputPath );
    wxString content;
    in.ReadAll( &content );
    json parsed = json::parse( content.ToStdString() );

    BOOST_CHECK_EQUAL( parsed["board"]["design_settings"]["rule_severities"]["new"], "error" );
    BOOST_CHECK( !parsed["board"]["design_settings"]["rule_severities"].contains( "old" ) );
    BOOST_CHECK_EQUAL( parsed["text_variables"]["USER_VAR"], "user value" );

    wxRemoveFile( outputPath );
    wxFileName::Rmdir( tmpDir );
}


BOOST_AUTO_TEST_CASE( ApplyPatchesMissingRequestedSourceFieldFails )
{
    wxString tmpDir = wxStandardPaths::Get().GetTempDir() + wxFILE_SEP_PATH
                      + wxS( "kicad_patch_" )
                      + wxString::Format( wxS( "%d" ),
                                          static_cast<int>( wxDateTime::Now().GetValue().GetValue() & 0xffffff ) );
    BOOST_REQUIRE( wxFileName::Mkdir( tmpDir, wxS_DIR_DEFAULT, wxPATH_MKDIR_FULL ) );

    const wxString outputPath = tmpDir + wxFILE_SEP_PATH + wxS( "test.kicad_pro" );

    json existing = { { "text_variables", { { "USER_VAR", "user value" } } } };

    {
        wxFile out;
        BOOST_REQUIRE( out.Open( outputPath, wxFile::write ) );
        BOOST_REQUIRE( out.Write( wxString::FromUTF8( existing.dump( 2 ) ) ) );
    }

    json source = { { "board", json::object() } };
    std::set<wxString> docProps = { DOC_PROP_DRC_SEVERITIES };

    BOOST_CHECK( !ApplyProjectFilePatches( outputPath, source, docProps ) );

    wxFile in( outputPath );
    wxString content;
    in.ReadAll( &content );
    json parsed = json::parse( content.ToStdString() );
    BOOST_CHECK_EQUAL( parsed["text_variables"]["USER_VAR"], "user value" );

    wxRemoveFile( outputPath );
    wxFileName::Rmdir( tmpDir );
}


// Realistically-shaped .kicad_pro: rule_severities is a sibling of other
// design_settings params under board.design_settings.  The surgical patch must
// touch only rule_severities and leave its siblings byte-identical -- a full
// SaveProjectCopy fallback (the symptom of a wrong pointer) would clobber them.
BOOST_AUTO_TEST_CASE( ApplyPatchRealisticProjectShapeIsSurgical )
{
    json existing = {
        { "board", {
            { "design_settings", {
                { "rule_severities", { { "copper_edge_clearance", "error" } } },
                { "rules", { { "min_copper_edge_clearance", 0.5 } } },
                { "track_widths", json::array( { 0.2, 0.25 } ) }
            } }
        } },
        { "meta", { { "filename", "board.kicad_pro" } } }
    };

    json source = {
        { "board", {
            { "design_settings", {
                { "rule_severities", { { "copper_edge_clearance", "warning" } } },
                { "rules", { { "min_copper_edge_clearance", 99.0 } } },
                { "track_widths", json::array( { 9.0 } ) }
            } }
        } }
    };

    BOOST_REQUIRE( ApplyProjectFilePatch( existing, source, DOC_PROP_DRC_SEVERITIES ) );

    // Targeted severity took the source value.
    BOOST_CHECK_EQUAL( existing["board"]["design_settings"]["rule_severities"]["copper_edge_clearance"],
                       "warning" );

    // Unrelated design_settings siblings are byte-identical to the original,
    // proving the patch did not fall back to a full copy of the source subtree.
    BOOST_CHECK_EQUAL( existing["board"]["design_settings"]["rules"]["min_copper_edge_clearance"], 0.5 );
    BOOST_CHECK_EQUAL( existing["board"]["design_settings"]["track_widths"].dump(),
                       json::array( { 0.2, 0.25 } ).dump() );
    BOOST_CHECK_EQUAL( existing["meta"]["filename"], "board.kicad_pro" );
}


BOOST_AUTO_TEST_SUITE_END()
