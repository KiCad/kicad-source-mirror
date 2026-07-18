/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.TXT for contributors.
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
 * @file test_nested_settings.cpp
 * Tests for NESTED_SETTINGS and ReleaseNestedSettings behavior.
 */

#include <qa_utils/wx_utils/unit_test_utils.h>
#include <settings/nested_settings.h>
#include <settings/parameters.h>
#include <settings/settings_manager.h>
#include <settings/json_settings_internals.h>

#include <wx/filename.h>
#include <wx/stdpaths.h>
#include <wx/utils.h>

#include <fstream>


/**
 * A test-accessible JSON_SETTINGS subclass that exposes the protected m_modified flag.
 */
class TEST_JSON_SETTINGS : public JSON_SETTINGS
{
public:
    TEST_JSON_SETTINGS() :
            JSON_SETTINGS( "test_parent", SETTINGS_LOC::NONE, 0, false, false, false )
    {
    }

    bool IsModified() const { return m_modified; }
    void ClearModified() { m_modified = false; }
};


/**
 * A minimal nested settings subclass for testing.  Simulates the pattern used by
 * ERC_SETTINGS, SCHEMATIC_SETTINGS, etc. where the constructor defines params that
 * may not exist in older project files.
 */
class TEST_NESTED_SETTINGS : public NESTED_SETTINGS
{
public:
    TEST_NESTED_SETTINGS( JSON_SETTINGS* aParent, const std::string& aPath,
                          bool aResetParamsIfMissing = true ) :
            NESTED_SETTINGS( "test_nested", 0, aParent, aPath, false ),
            m_testValue( 0 ),
            m_extraValue( 42 )
    {
        m_resetParamsIfMissing = aResetParamsIfMissing;

        m_params.emplace_back( new PARAM<int>( "test_value", &m_testValue, 0 ) );

        // Simulates a newly-added setting not present in old project files
        m_params.emplace_back( new PARAM<int>( "extra_value", &m_extraValue, 42 ) );

        // Load after params are registered (the real KiCad classes register params in
        // the JSON_SETTINGS base ctor, which runs before NESTED_SETTINGS calls LoadFromFile)
        LoadFromFile();
    }

    ~TEST_NESTED_SETTINGS() override
    {
        // Prevent the base destructor from calling ReleaseNestedSettings so we can test
        // that path explicitly
        m_parent = nullptr;
    }

    int m_testValue;
    int m_extraValue;
};


class NESTED_SETTINGS_TEST_FIXTURE
{
public:
    NESTED_SETTINGS_TEST_FIXTURE()
    {
        // ReleaseNestedSettings requires m_manager to be non-null
        m_parent.SetManager( reinterpret_cast<SETTINGS_MANAGER*>( 1 ) );
    }

    TEST_JSON_SETTINGS m_parent;
};


BOOST_FIXTURE_TEST_SUITE( NestedSettings, NESTED_SETTINGS_TEST_FIXTURE )


/**
 * Verify that releasing nested settings does not mark the parent as modified.
 *
 * This is the core regression test for https://gitlab.com/kicad/code/kicad/-/issues/22972
 * and https://gitlab.com/kicad/code/kicad/-/issues/24402.
 * When a nested settings object is destroyed during editor close, round-trip serialization
 * differences (such as new default params not present in the project file) would falsely mark
 * the parent project file as modified, causing the .kicad_pro timestamp to update even when
 * the user discarded changes.
 */
BOOST_AUTO_TEST_CASE( ReleaseDoesNotMarkParentModified )
{
    // Seed the parent with partial nested data that only contains "test_value".
    // This simulates an older project file that doesn't have the "extra_value" key.
    nlohmann::json nestedJson = {
        { "meta", { { "version", 0 } } },
        { "test_value", 5 }
    };

    ( *m_parent.Internals() )["/test_nested"_json_pointer] = nestedJson;

    BOOST_CHECK( !m_parent.IsModified() );

    // Create nested settings that load from parent JSON
    auto* nested = new TEST_NESTED_SETTINGS( &m_parent, "test_nested" );

    BOOST_CHECK_EQUAL( nested->m_testValue, 5 );
    BOOST_CHECK_EQUAL( nested->m_extraValue, 42 );

    // Ensure the parent starts from a clean state
    m_parent.ClearModified();

    // Release the nested settings without any user-initiated modifications
    m_parent.ReleaseNestedSettings( nested );

    // The parent must NOT be marked modified. Before the fix for #22972, the extra_value
    // param (absent from the original JSON) created a diff that falsely marked the parent.
    BOOST_CHECK_MESSAGE( !m_parent.IsModified(),
                         "Parent should not be marked modified after releasing nested settings "
                         "when no intentional changes were made" );

    delete nested;
}


/**
 * Verify that explicitly saving while the nested settings are alive correctly detects
 * modifications. This ensures the fix doesn't break the normal save path.
 */
BOOST_AUTO_TEST_CASE( ExplicitSaveDetectsModifications )
{
    nlohmann::json nestedJson = {
        { "meta", { { "version", 0 } } },
        { "test_value", 5 },
        { "extra_value", 42 }
    };

    ( *m_parent.Internals() )["/test_nested"_json_pointer] = nestedJson;

    auto* nested = new TEST_NESTED_SETTINGS( &m_parent, "test_nested" );

    BOOST_CHECK_EQUAL( nested->m_testValue, 5 );

    // Simulate a user-initiated change to the nested settings
    nested->m_testValue = 99;

    // SaveToFile on the nested settings should report the modification
    bool modified = nested->SaveToFile();
    BOOST_CHECK( modified );

    // The parent JSON should now contain the updated value
    auto parentJson = m_parent.GetJson( "test_nested.test_value" );

    BOOST_CHECK( parentJson.has_value() );
    BOOST_CHECK_EQUAL( parentJson->get<int>(), 99 );

    m_parent.ReleaseNestedSettings( nested );
    delete nested;
}


/**
 * Verify that releasing nested settings still flushes the data to the parent JSON.
 * Even though the parent is not marked modified, the in-memory state should be updated.
 */
BOOST_AUTO_TEST_CASE( ReleaseFlushesDataToParent )
{
    nlohmann::json nestedJson = {
        { "meta", { { "version", 0 } } },
        { "test_value", 5 }
    };

    ( *m_parent.Internals() )["/test_nested"_json_pointer] = nestedJson;

    auto* nested = new TEST_NESTED_SETTINGS( &m_parent, "test_nested" );

    // Change a value in the nested settings
    nested->m_testValue = 77;

    m_parent.ClearModified();
    m_parent.ReleaseNestedSettings( nested );

    // Verify the changed value was flushed to the parent's in-memory JSON
    auto parentJson = m_parent.GetJson( "test_nested.test_value" );

    BOOST_CHECK( parentJson.has_value() );
    BOOST_CHECK_EQUAL( parentJson->get<int>(), 77 );

    delete nested;
}


/**
 * When missing keys are not reset to their defaults on load (as for board design settings), an
 * absent param holding the default value is NOT a faithful round-trip of the file, so it must
 * count as modified and be flushed. Otherwise a user reset-to-default could be silently dropped.
 */
BOOST_AUTO_TEST_CASE( AbsentDefaultIsModifiedWhenNotResetIfMissing )
{
    nlohmann::json nestedJson = {
        { "meta", { { "version", 0 } } },
        { "test_value", 5 }
    };

    ( *m_parent.Internals() )["/test_nested"_json_pointer] = nestedJson;

    // extra_value is absent from the file; with reset-if-missing disabled it keeps its default.
    auto* nested = new TEST_NESTED_SETTINGS( &m_parent, "test_nested", /* aResetParamsIfMissing */ false );

    BOOST_CHECK_EQUAL( nested->m_extraValue, 42 );

    // The absent default must be reported as a change so it is persisted.
    BOOST_CHECK( nested->SaveToFile() );

    auto parentJson = m_parent.GetJson( "test_nested.extra_value" );

    BOOST_CHECK( parentJson.has_value() );
    BOOST_CHECK_EQUAL( parentJson->get<int>(), 42 );

    m_parent.ReleaseNestedSettings( nested );
    delete nested;
}


/**
 * A nested settings subclass that migrates a v0 block to v1 by writing a key directly into the
 * JSON, without any registered param reflecting the change. Models real migrations such as
 * NET_SETTINGS / BOARD_DESIGN_SETTINGS.
 */
class MIGRATING_NESTED_SETTINGS : public NESTED_SETTINGS
{
public:
    MIGRATING_NESTED_SETTINGS( JSON_SETTINGS* aParent, const std::string& aPath ) :
            NESTED_SETTINGS( "test_nested", 1, aParent, aPath, false )
    {
        registerMigration( 0, 1,
                           [this]()
                           {
                               Set<int>( "migrated_field", 7 );
                               return true;
                           } );

        LoadFromFile();
    }

    ~MIGRATING_NESTED_SETTINGS() override
    {
        m_parent = nullptr;
    }
};


/**
 * A migration that mutates only the JSON (not a registered param) must still flush to the parent,
 * otherwise the upgraded content is silently lost. Guards against detecting nested changes solely
 * through param Store() (see #24402 review).
 */
BOOST_AUTO_TEST_CASE( MigrationFlushesToParent )
{
    nlohmann::json nestedJson = {
        { "meta", { { "version", 0 } } }
    };

    ( *m_parent.Internals() )["/test_nested"_json_pointer] = nestedJson;

    auto* nested = new MIGRATING_NESTED_SETTINGS( &m_parent, "test_nested" );

    // The migrator bumped the version and wrote migrated_field directly into the JSON.
    BOOST_CHECK( nested->SaveToFile() );

    auto version = m_parent.GetJson( "test_nested.meta.version" );
    auto migrated = m_parent.GetJson( "test_nested.migrated_field" );

    BOOST_CHECK( version.has_value() );
    BOOST_CHECK_EQUAL( version->get<int>(), 1 );
    BOOST_CHECK( migrated.has_value() );
    BOOST_CHECK_EQUAL( migrated->get<int>(), 7 );

    m_parent.ReleaseNestedSettings( nested );
    delete nested;
}


/**
 * Verify that SaveToFile does not rewrite the on-disk file when its serialized contents
 * match what already exists. This is the regression test for the .kicad_pro/.kicad_prl
 * timestamp churn described in #24402: closing an editor flushes nested settings whose
 * round-trip serialization happens to equal the existing file, but the previous code
 * still rewrote the file (updating the mtime).
 */
class DISK_SETTINGS : public JSON_SETTINGS
{
public:
    DISK_SETTINGS( const wxString& aFilename ) :
            JSON_SETTINGS( aFilename, SETTINGS_LOC::NONE, 0, true, true, true ),
            m_value( 0 )
    {
        m_params.emplace_back( new PARAM<int>( "value", &m_value, 0 ) );
    }

    // Forces the next SaveToFile past the "modified" early-exit gate so that the new
    // on-disk content comparison is exercised even when no params actually changed.
    void ForceModified() { m_modified = true; }

    int m_value;
};


BOOST_AUTO_TEST_CASE( SaveToFileSkipsWriteWhenContentsMatch )
{
    wxFileName tempDir( wxStandardPaths::Get().GetTempDir(), wxEmptyString );
    wxString   tempName = wxString::Format( wxT( "kicad_qa_24402_%ld" ), wxGetProcessId() );
    tempDir.AppendDir( tempName );

    BOOST_REQUIRE( wxFileName::Mkdir( tempDir.GetPath(), wxS_DIR_DEFAULT, wxPATH_MKDIR_FULL ) );

    wxFileName tempFile( tempDir.GetPath(), wxT( "test.json" ) );

    DISK_SETTINGS settings( tempFile.GetFullPath() );
    settings.SetManager( reinterpret_cast<SETTINGS_MANAGER*>( 1 ) );
    settings.m_value = 7;

    // First save creates the file
    BOOST_REQUIRE( settings.SaveToFile( wxEmptyString, true ) );
    BOOST_REQUIRE( tempFile.FileExists() );

    // Capture the original mtime
    wxDateTime originalModTime = tempFile.GetModificationTime();

    // Make sure any subsequent write would produce a different mtime
    wxMilliSleep( 1500 );

    // Force the modified flag so the early "not modified, skipping save" exit is bypassed.
    // The serialized payload will still match what is already on disk, so the new on-disk
    // content comparison must be what prevents the write. This mimics the false-positive
    // modified flag pattern observed in #24402.
    settings.ForceModified();

    bool result = settings.SaveToFile();

    // SaveToFile returns false when it skips the write
    BOOST_CHECK( !result );

    wxFileName refreshed( tempFile.GetFullPath() );
    wxDateTime newModTime = refreshed.GetModificationTime();

    BOOST_CHECK_MESSAGE( originalModTime == newModTime,
                         "File mtime should not change when serialized contents match on-disk "
                         "contents" );

    // Also verify that an actual content change still writes the file.
    settings.m_value = 99;
    settings.ForceModified();

    BOOST_CHECK( settings.SaveToFile() );

    wxFileName refreshedAfterChange( tempFile.GetFullPath() );
    BOOST_CHECK( refreshedAfterChange.GetModificationTime() != originalModTime );

    // Cleanup
    wxRemoveFile( tempFile.GetFullPath() );
    wxFileName::Rmdir( tempDir.GetPath() );
}


/**
 * A disk-backed parent that carries a nested setting, mirroring the project file / nested
 * board or ERC settings relationship.
 */
class DISK_PARENT_SETTINGS : public JSON_SETTINGS
{
public:
    DISK_PARENT_SETTINGS( const wxString& aFilename ) :
            JSON_SETTINGS( aFilename, SETTINGS_LOC::NONE, 0, true, true, true ),
            m_ownValue( 0 )
    {
        m_params.emplace_back( new PARAM<int>( "own_value", &m_ownValue, 0 ) );
    }

    int m_ownValue;
};


/**
 * Verify that a genuine change to a nested setting survives the release-then-save sequence that
 * happens on editor close. The nested object flushes its change into the parent's in-memory JSON
 * and is then released; a subsequent parent save must still write that change to disk even though
 * the parent's modified heuristic is not set by the release. Regression guard for the data-loss
 * risk that accompanies suppressing the project rewrite in #24402.
 */
BOOST_AUTO_TEST_CASE( ReleasedNestedChangePersistsToDisk )
{
    wxFileName tempDir( wxStandardPaths::Get().GetTempDir(), wxEmptyString );
    wxString   tempName = wxString::Format( wxT( "kicad_qa_24402_nested_%ld" ), wxGetProcessId() );
    tempDir.AppendDir( tempName );

    BOOST_REQUIRE( wxFileName::Mkdir( tempDir.GetPath(), wxS_DIR_DEFAULT, wxPATH_MKDIR_FULL ) );

    wxFileName tempFile( tempDir.GetPath(), wxT( "parent.json" ) );

    DISK_PARENT_SETTINGS parent( tempFile.GetFullPath() );
    parent.SetManager( reinterpret_cast<SETTINGS_MANAGER*>( 1 ) );

    auto* nested = new TEST_NESTED_SETTINGS( &parent, "test_nested" );
    nested->m_testValue = 5;

    // Persist the baseline so the on-disk content comparison has something to compare against.
    BOOST_REQUIRE( parent.SaveToFile( wxEmptyString, true ) );
    BOOST_REQUIRE( tempFile.FileExists() );

    // Make a genuine change to the nested setting, then release it (editor close path) before the
    // parent is saved. The parent's modified flag is intentionally cleared to model the case where
    // only the nested data changed.
    nested->m_testValue = 123;
    parent.ReleaseNestedSettings( nested );
    delete nested;

    BOOST_REQUIRE( parent.SaveToFile() );

    // Re-read the file from disk and confirm the changed nested value was written.
    std::ifstream  in( tempFile.GetFullPath().fn_str(), std::ios::in | std::ios::binary );
    std::string    contents( ( std::istreambuf_iterator<char>( in ) ),
                             std::istreambuf_iterator<char>() );
    nlohmann::json onDisk = nlohmann::json::parse( contents );

    BOOST_CHECK_EQUAL( onDisk["test_nested"]["test_value"].get<int>(), 123 );

    wxRemoveFile( tempFile.GetFullPath() );
    wxFileName::Rmdir( tempDir.GetPath() );
}


/**
 * Reproduce issue #24402 class 1 directly: an older project file is missing a newly-introduced
 * nested default param. Opening and closing the editor (load + release the nested settings with no
 * user change) must not materialize that default into the parent and must not rewrite the file.
 */
BOOST_AUTO_TEST_CASE( DefaultFillDoesNotChurnExistingFile )
{
    wxFileName tempDir( wxStandardPaths::Get().GetTempDir(), wxEmptyString );
    wxString   tempName = wxString::Format( wxT( "kicad_qa_24402_churn_%ld" ), wxGetProcessId() );
    tempDir.AppendDir( tempName );

    BOOST_REQUIRE( wxFileName::Mkdir( tempDir.GetPath(), wxS_DIR_DEFAULT, wxPATH_MKDIR_FULL ) );

    wxFileName tempFile( tempDir.GetPath(), wxT( "parent.json" ) );

    DISK_PARENT_SETTINGS parent( tempFile.GetFullPath() );
    parent.SetManager( reinterpret_cast<SETTINGS_MANAGER*>( 1 ) );

    // Older file: the nested block has test_value but not the newer extra_value param.
    nlohmann::json nestedJson = { { "meta", { { "version", 0 } } }, { "test_value", 5 } };
    ( *parent.Internals() )["/test_nested"_json_pointer] = nestedJson;
    BOOST_REQUIRE( parent.SaveToFile( wxEmptyString, true ) );

    std::ifstream in0( tempFile.GetFullPath().fn_str(), std::ios::binary );
    std::string   before( ( std::istreambuf_iterator<char>( in0 ) ),
                          std::istreambuf_iterator<char>() );

    BOOST_REQUIRE( before.find( "extra_value" ) == std::string::npos );

    // Open and close the nested settings with no user change (the extra_value default loads).
    auto* nested = new TEST_NESTED_SETTINGS( &parent, "test_nested" );
    parent.ReleaseNestedSettings( nested );
    delete nested;

    // No genuine change, so the parent save must be a no-op.
    BOOST_CHECK( !parent.SaveToFile() );

    std::ifstream in1( tempFile.GetFullPath().fn_str(), std::ios::binary );
    std::string   after( ( std::istreambuf_iterator<char>( in1 ) ),
                         std::istreambuf_iterator<char>() );

    BOOST_CHECK_MESSAGE( before == after,
                         "Releasing an unchanged nested setting must not add default keys or "
                         "rewrite the existing file" );

    wxRemoveFile( tempFile.GetFullPath() );
    wxFileName::Rmdir( tempDir.GetPath() );
}


BOOST_AUTO_TEST_SUITE_END()
