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
 * @file test_backup_settings.cpp
 * Tests for the auto_backup.format and auto_backup.location settings introduced
 * in COMMON_SETTINGS schema 6.
 */

#include <qa_utils/wx_utils/unit_test_utils.h>

#include <pgm_base.h>
#include <settings/common_settings.h>
#include <settings/json_settings_internals.h>
#include <settings/settings_manager.h>

#include <nlohmann/json.hpp>
#include <wx/filename.h>


BOOST_AUTO_TEST_SUITE( BackupSettings )


// Verify the new auto_backup.format and auto_backup.location parameters round-trip
// through the JSON_SETTINGS persistence layer.  PARAM_ENUM serializes enums as
// integers; this test exercises both directions.
BOOST_AUTO_TEST_CASE( BackupFormatLocationRoundTrip )
{
    COMMON_SETTINGS cs;

    // Set non-default values.
    cs.m_Backup.format = BACKUP_FORMAT::ZIP;
    cs.m_Backup.location = BACKUP_LOCATION::USER_DIR;

    // Force the parameters to write through to the in-memory JSON.
    cs.Store();

    // Mutate the struct, then load back from JSON to confirm values were persisted.
    cs.m_Backup.format = BACKUP_FORMAT::INCREMENTAL;
    cs.m_Backup.location = BACKUP_LOCATION::PROJECT_DIR;

    cs.Load();

    BOOST_CHECK( cs.m_Backup.format == BACKUP_FORMAT::ZIP );
    BOOST_CHECK( cs.m_Backup.location == BACKUP_LOCATION::USER_DIR );
}


// Fresh-install defaults: new users get the new INCREMENTAL/PROJECT_DIR behavior.
// Existing users are handled by the schema-5 -> schema-6 migration test below.
BOOST_AUTO_TEST_CASE( BackupFormatLocationDefaults )
{
    COMMON_SETTINGS cs;

    cs.ResetToDefaults();

    BOOST_CHECK( cs.m_Backup.format == BACKUP_FORMAT::INCREMENTAL );
    BOOST_CHECK( cs.m_Backup.location == BACKUP_LOCATION::PROJECT_DIR );
}


// Pre-schema-6 configs unconditionally produced timestamped ZIP archives whenever a
// save was eligible for backup.  The migration must therefore write ZIP into upgraded
// configs that lack auto_backup.format -- otherwise existing users with backups
// enabled would silently lose archive creation after the upgrade because the new
// PARAM_ENUM default (INCREMENTAL) causes TriggerBackupIfNeeded() to short-circuit.
BOOST_AUTO_TEST_CASE( SchemaFiveToSixPreservesArchiveBehavior )
{
    COMMON_SETTINGS cs;

    // Persist the in-memory params to the JSON tree so meta.version exists, then
    // simulate a schema-5 config: drop the auto_backup subtree entirely (matching what
    // a real pre-schema-6 file would contain) and rewind meta.version.
    cs.Store();

    JSON_SETTINGS_INTERNALS* internals = cs.Internals();

    if( internals->contains( nlohmann::json::json_pointer( "/auto_backup" ) ) )
        internals->erase( "auto_backup" );

    cs.Set<int>( "meta.version", 5 );

    BOOST_REQUIRE( cs.Migrate() );

    // Re-load params from the migrated JSON so m_Backup reflects what the migration wrote.
    cs.Load();

    BOOST_CHECK( cs.m_Backup.format == BACKUP_FORMAT::ZIP );
    BOOST_CHECK( cs.m_Backup.location == BACKUP_LOCATION::PROJECT_DIR );
}


// If a schema-5 config somehow already has an auto_backup.format key, the migration
// must respect it rather than overwriting the user's choice.
BOOST_AUTO_TEST_CASE( SchemaFiveToSixRespectsExistingFormat )
{
    COMMON_SETTINGS cs;

    cs.Store();

    cs.Set<int>( "auto_backup.format", static_cast<int>( BACKUP_FORMAT::INCREMENTAL ) );
    cs.Set<int>( "meta.version", 5 );

    BOOST_REQUIRE( cs.Migrate() );

    cs.Load();

    BOOST_CHECK( cs.m_Backup.format == BACKUP_FORMAT::INCREMENTAL );
}


BOOST_AUTO_TEST_SUITE_END()
