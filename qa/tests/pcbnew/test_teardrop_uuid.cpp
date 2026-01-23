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
 * @file test_teardrop_uuid.cpp
 * Test that deterministic UUID generation works correctly for teardrop scenarios.
 * The core KIID::Combine function is tested in test_kiid.cpp. These tests verify
 * the expected behavior when combining track and pad UUIDs as used by teardrops.
 */

#include <qa_utils/wx_utils/unit_test_utils.h>

#include <kiid.h>


BOOST_AUTO_TEST_SUITE( TeardropUUID )


BOOST_AUTO_TEST_CASE( CombineTrackAndPadUuids )
{
    // Simulate the teardrop UUID generation: combine track and pad UUIDs
    KIID trackUuid;
    KIID padUuid;

    KIID teardropUuid = KIID::Combine( trackUuid, padUuid );

    // The combined UUID should be different from both inputs
    BOOST_CHECK( teardropUuid != trackUuid );
    BOOST_CHECK( teardropUuid != padUuid );

    // The same inputs should always produce the same output
    KIID teardropUuid2 = KIID::Combine( trackUuid, padUuid );
    BOOST_CHECK_EQUAL( teardropUuid.AsString(), teardropUuid2.AsString() );
}


BOOST_AUTO_TEST_CASE( MaskUuidIsIncrementedFromTeardropUuid )
{
    // Teardrop masks use the combined UUID, then increment to differentiate
    KIID trackUuid;
    KIID padUuid;

    KIID teardropUuid = KIID::Combine( trackUuid, padUuid );

    KIID maskUuid = KIID::Combine( trackUuid, padUuid );
    maskUuid.Increment();

    // The mask UUID should be different from the teardrop UUID
    BOOST_CHECK( maskUuid != teardropUuid );

    // Both should be deterministic
    KIID teardropUuid2 = KIID::Combine( trackUuid, padUuid );
    KIID maskUuid2 = KIID::Combine( trackUuid, padUuid );
    maskUuid2.Increment();

    BOOST_CHECK_EQUAL( teardropUuid.AsString(), teardropUuid2.AsString() );
    BOOST_CHECK_EQUAL( maskUuid.AsString(), maskUuid2.AsString() );
}


BOOST_AUTO_TEST_CASE( DifferentTracksProduceDifferentTeardropUuids )
{
    // Two tracks connecting to the same pad should produce different teardrop UUIDs
    KIID track1Uuid;
    KIID track2Uuid;
    KIID padUuid;

    KIID teardrop1Uuid = KIID::Combine( track1Uuid, padUuid );
    KIID teardrop2Uuid = KIID::Combine( track2Uuid, padUuid );

    // Different tracks should produce different teardrops
    BOOST_CHECK( teardrop1Uuid != teardrop2Uuid );

    // Each should still be deterministic
    KIID teardrop1Uuid2 = KIID::Combine( track1Uuid, padUuid );
    KIID teardrop2Uuid2 = KIID::Combine( track2Uuid, padUuid );

    BOOST_CHECK_EQUAL( teardrop1Uuid.AsString(), teardrop1Uuid2.AsString() );
    BOOST_CHECK_EQUAL( teardrop2Uuid.AsString(), teardrop2Uuid2.AsString() );
}


BOOST_AUTO_TEST_CASE( StableAcrossRegenerations )
{
    // Simulates regenerating teardrops: same track+pad should always give same UUID
    KIID trackUuid( "12345678-1234-1234-1234-123456789012" );
    KIID padUuid( "abcdef01-abcd-abcd-abcd-abcdef012345" );

    // First generation
    KIID teardropUuid1 = KIID::Combine( trackUuid, padUuid );

    // Simulate regeneration by computing again
    KIID teardropUuid2 = KIID::Combine( trackUuid, padUuid );

    // Should be identical
    BOOST_CHECK_EQUAL( teardropUuid1.AsString(), teardropUuid2.AsString() );
}


BOOST_AUTO_TEST_SUITE_END()
