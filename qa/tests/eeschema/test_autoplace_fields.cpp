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
 * Test suite for autoplace fields functionality.
 * Tests fix for issue #16538: autoplace fields overlaps symbol text.
 */

#include <qa_utils/wx_utils/unit_test_utils.h>
#include "eeschema_test_utils.h"

#include <sch_symbol.h>
#include <sch_field.h>
#include <sch_item.h>
#include <geometry/eda_angle.h>


class TEST_AUTOPLACE_FIELDS_FIXTURE : public KI_TEST::SCHEMATIC_TEST_FIXTURE
{
public:
    SCH_SYMBOL* GetSymbolByRef( const wxString& aRef )
    {
        if( !m_schematic )
            return nullptr;

        SCH_SCREEN* screen = m_schematic->RootScreen();

        if( !screen )
            return nullptr;

        for( SCH_ITEM* item : screen->Items().OfType( SCH_SYMBOL_T ) )
        {
            SCH_SYMBOL* symbol = static_cast<SCH_SYMBOL*>( item );

            if( symbol && symbol->GetRef( &m_schematic->Hierarchy()[0], false ) == aRef )
                return symbol;
        }

        return nullptr;
    }
};


BOOST_FIXTURE_TEST_SUITE( AutoplaceFields, TEST_AUTOPLACE_FIELDS_FIXTURE )


/**
 * Test that field bounding boxes for rotated symbols have correct dimensions.
 * This is a regression test for issue #16538.
 *
 * The bug was that when computing field bounding boxes for autoplace, the code
 * set field text angle to ANGLE_VERTICAL for 90/270 degree rotated symbols.
 * However, GetBoundingBox() applies BOTH the field angle AND the symbol transform,
 * resulting in 180-degree effective rotation (field angle + symbol transform).
 * This caused the bounding box dimensions to be incorrect.
 *
 * The fix sets field text angle to ANGLE_HORIZONTAL, so that GetBoundingBox()
 * applies only the symbol transform (90 degrees), giving correct dimensions
 * for vertical text display.
 */
BOOST_AUTO_TEST_CASE( RotatedSymbolFieldBoundingBox )
{
    wxFileName fn;
    fn.SetPath( KI_TEST::GetEeschemaTestDataDir() );
    fn.AppendDir( wxS( "issue16538" ) );
    fn.SetName( wxS( "issue16538" ) );
    fn.SetExt( FILEEXT::KiCadSchematicFileExtension );

    LoadSchematic( fn.GetFullPath() );

    SCH_SYMBOL* symbol = GetSymbolByRef( wxS( "C1" ) );
    BOOST_REQUIRE( symbol );

    // Verify the symbol is rotated 90 degrees (y1 != 0)
    TRANSFORM transform = symbol->GetTransform();
    BOOST_CHECK( transform.y1 != 0 );

    // Get the reference field
    SCH_FIELD* refField = symbol->GetField( FIELD_T::REFERENCE );
    BOOST_REQUIRE( refField );

    // Set the field to horizontal angle (as the fix does)
    refField->SetTextAngle( ANGLE_HORIZONTAL );

    // Get the bounding box after setting horizontal angle
    BOX2I bbox = refField->GetBoundingBox();

    // For a 90-degree rotated symbol with horizontal field angle:
    // The bounding box should be tall and narrow (text displays vertically)
    // Height should be greater than width for short text like "C1"
    BOOST_CHECK_MESSAGE( bbox.GetHeight() > bbox.GetWidth(),
                         wxString::Format( "Expected vertical bounding box (height > width) "
                                           "for rotated symbol. Got width=%lld, height=%lld",
                                           static_cast<long long>( bbox.GetWidth() ),
                                           static_cast<long long>( bbox.GetHeight() ) ) );
}


/**
 * Test that GetDrawRotation returns correct visual orientation.
 */
BOOST_AUTO_TEST_CASE( FieldDrawRotationForRotatedSymbol )
{
    wxFileName fn;
    fn.SetPath( KI_TEST::GetEeschemaTestDataDir() );
    fn.AppendDir( wxS( "issue16538" ) );
    fn.SetName( wxS( "issue16538" ) );
    fn.SetExt( FILEEXT::KiCadSchematicFileExtension );

    LoadSchematic( fn.GetFullPath() );

    SCH_SYMBOL* symbol = GetSymbolByRef( wxS( "C1" ) );
    BOOST_REQUIRE( symbol );

    SCH_FIELD* refField = symbol->GetField( FIELD_T::REFERENCE );
    BOOST_REQUIRE( refField );

    // Set field to horizontal angle (as the fix does for autoplace)
    refField->SetTextAngle( ANGLE_HORIZONTAL );

    // For a 90-degree rotated symbol with horizontal field angle,
    // GetDrawRotation should return vertical (the visual orientation)
    EDA_ANGLE drawRotation = refField->GetDrawRotation();
    BOOST_CHECK_MESSAGE( drawRotation.IsVertical(),
                         wxString::Format( "Expected vertical draw rotation for 90-degree "
                                           "rotated symbol with horizontal field angle. "
                                           "Got %f degrees", drawRotation.AsDegrees() ) );
}


/**
 * Regression test for issue #22927.
 *
 * The autoplacer must set field angles so that GetDrawRotation() returns HORIZONTAL
 * for all symbol orientations. For 90/270-degree rotated symbols, the stored angle
 * should be VERTICAL (counteracting the symbol's transform).
 */
BOOST_AUTO_TEST_CASE( RotatedSymbolFieldAngleForHorizontalDisplay )
{
    wxFileName fn;
    fn.SetPath( KI_TEST::GetEeschemaTestDataDir() );
    fn.AppendDir( wxS( "issue16538" ) );
    fn.SetName( wxS( "issue16538" ) );
    fn.SetExt( FILEEXT::KiCadSchematicFileExtension );

    LoadSchematic( fn.GetFullPath() );

    // C1 is a 90-degree rotated capacitor (y1 != 0)
    SCH_SYMBOL* rotatedSymbol = GetSymbolByRef( wxS( "C1" ) );
    BOOST_REQUIRE( rotatedSymbol );
    BOOST_REQUIRE( rotatedSymbol->GetTransform().y1 != 0 );

    SCH_FIELD* refField = rotatedSymbol->GetField( FIELD_T::REFERENCE );
    BOOST_REQUIRE( refField );

    // Simulate what the autoplacer should do for rotated symbols
    refField->SetTextAngle( ANGLE_VERTICAL );

    EDA_ANGLE drawRotation = refField->GetDrawRotation();
    BOOST_CHECK_MESSAGE( drawRotation.IsHorizontal(),
                         wxString::Format( wxS( "Rotated symbol with VERTICAL field angle "
                                                "should display horizontally. Got %f degrees" ),
                                           drawRotation.AsDegrees() ) );

    // R1 is a non-rotated resistor (y1 == 0)
    SCH_SYMBOL* normalSymbol = GetSymbolByRef( wxS( "R1" ) );
    BOOST_REQUIRE( normalSymbol );
    BOOST_REQUIRE( normalSymbol->GetTransform().y1 == 0 );

    refField = normalSymbol->GetField( FIELD_T::REFERENCE );
    BOOST_REQUIRE( refField );

    // For non-rotated symbols, HORIZONTAL field angle should stay horizontal
    refField->SetTextAngle( ANGLE_HORIZONTAL );

    drawRotation = refField->GetDrawRotation();
    BOOST_CHECK_MESSAGE( drawRotation.IsHorizontal(),
                         wxString::Format( wxS( "Non-rotated symbol with HORIZONTAL field angle "
                                                "should display horizontally. Got %f degrees" ),
                                           drawRotation.AsDegrees() ) );
}


/**
 * Verify that using HORIZONTAL field angle for bounding box computation on a rotated
 * symbol produces correct dimensions (not swapped).
 * This ensures the fix for #16538 is preserved while fixing #22927.
 */
BOOST_AUTO_TEST_CASE( RotatedSymbolBBoxWithHorizontalAngle )
{
    wxFileName fn;
    fn.SetPath( KI_TEST::GetEeschemaTestDataDir() );
    fn.AppendDir( wxS( "issue16538" ) );
    fn.SetName( wxS( "issue16538" ) );
    fn.SetExt( FILEEXT::KiCadSchematicFileExtension );

    LoadSchematic( fn.GetFullPath() );

    // C1 is a 90-degree rotated capacitor
    SCH_SYMBOL* symbol = GetSymbolByRef( wxS( "C1" ) );
    BOOST_REQUIRE( symbol );

    SCH_FIELD* refField = symbol->GetField( FIELD_T::REFERENCE );
    BOOST_REQUIRE( refField );

    // With HORIZONTAL angle, the bounding box for a rotated symbol
    // should be tall and narrow (symbol transform makes text vertical)
    refField->SetTextAngle( ANGLE_HORIZONTAL );
    BOX2I hBox = refField->GetBoundingBox();

    // With VERTICAL angle, the bounding box gets double-rotated (180°)
    // which produces incorrect (wider than tall) dimensions
    refField->SetTextAngle( ANGLE_VERTICAL );
    BOX2I vBox = refField->GetBoundingBox();

    // The HORIZONTAL version should have the correct vertical orientation
    BOOST_CHECK_MESSAGE( hBox.GetHeight() > hBox.GetWidth(),
                         wxString::Format( wxS( "HORIZONTAL angle bbox should be taller than "
                                                "wide. Got width=%lld, height=%lld" ),
                                           static_cast<long long>( hBox.GetWidth() ),
                                           static_cast<long long>( hBox.GetHeight() ) ) );

    // The VERTICAL version has incorrect dimensions (wider than tall)
    // due to the 180-degree effective rotation
    BOOST_CHECK_MESSAGE( vBox.GetWidth() > vBox.GetHeight(),
                         wxString::Format( wxS( "VERTICAL angle bbox should be wider than "
                                                "tall (known incorrect). Got width=%lld, "
                                                "height=%lld" ),
                                           static_cast<long long>( vBox.GetWidth() ),
                                           static_cast<long long>( vBox.GetHeight() ) ) );

    // Restore correct angle
    refField->SetTextAngle( ANGLE_VERTICAL );
}


BOOST_AUTO_TEST_SUITE_END()
