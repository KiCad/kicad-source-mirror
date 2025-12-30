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
 * Test suite for SCH_SYMBOL object.
 */

#include <qa_utils/wx_utils/unit_test_utils.h>
#include "eeschema_test_utils.h"

// Code under test
#include <sch_symbol.h>
#include <sch_edit_frame.h>
#include <wildcards_and_files_ext.h>


class TEST_SCH_SYMBOL_FIXTURE : public KI_TEST::SCHEMATIC_TEST_FIXTURE
{
public:
    SCH_SYMBOL* GetFirstSymbol()
    {
        if( !m_schematic )
            return nullptr;

        SCH_SCREEN* screen = m_schematic->RootScreen();

        if( !screen )
            return nullptr;

        for( SCH_ITEM* item : screen->Items().OfType( SCH_SYMBOL_T ) )
        {
            SCH_SYMBOL* symbol = static_cast<SCH_SYMBOL*>( item );

            if( symbol )
                return symbol;
        }

        return nullptr;
    }

    ///< #SCH_SYMBOL object with no extra data set.
    SCH_SYMBOL m_symbol;
};


/**
 * Declare the test suite
 */
BOOST_FIXTURE_TEST_SUITE( SchSymbol, TEST_SCH_SYMBOL_FIXTURE )


/**
 * Check that we can get the default properties as expected.
 */
BOOST_AUTO_TEST_CASE( DefaultProperties )
{
}


/**
 * Test the orientation transform changes.
 */
BOOST_AUTO_TEST_CASE( Orientation )
{
    TRANSFORM t = m_symbol.GetTransform();

    m_symbol.SetOrientation( SYM_ORIENT_90 );
    t = m_symbol.GetTransform();
    m_symbol.SetTransform( TRANSFORM() );
    m_symbol.SetOrientation( SYM_ORIENT_180 );
    t = m_symbol.GetTransform();
    m_symbol.SetTransform( TRANSFORM() );
    m_symbol.SetOrientation( SYM_ORIENT_270 );
    t = m_symbol.GetTransform();
}


/**
 * Test symbol variant handling.
 */
BOOST_AUTO_TEST_CASE( SchSymbolVariantTest )
{
    wxFileName fn;
    fn.SetPath( KI_TEST::GetEeschemaTestDataDir() );
    fn.AppendDir( wxS( "variant_test" ) );
    fn.SetName( wxS( "variant_test" ) );
    fn.SetExt( FILEEXT::KiCadSchematicFileExtension );

    LoadSchematic( fn.GetFullPath() );

    SCH_SYMBOL* symbol = GetFirstSymbol();
    BOOST_CHECK( symbol );

    // Test for an empty (non-existant) variant.
    wxString                          variantName = wxS( "Variant1" );
    std::optional<SCH_SYMBOL_VARIANT> variant = symbol->GetVariant( m_schematic->Hierarchy()[0], variantName );
    BOOST_CHECK( !variant );

    // Test DNP property variant.
    BOOST_CHECK( !symbol->GetDNP() );
    symbol->SetDNP( true, &m_schematic->Hierarchy()[0], variantName );
    BOOST_CHECK( symbol->GetDNP( &m_schematic->Hierarchy()[0], variantName ) );

    // Test exclude from BOM property variant.
    BOOST_CHECK( !symbol->GetExcludedFromBOM() );
    symbol->SetExcludedFromBOM( true, &m_schematic->Hierarchy()[0], variantName );
    BOOST_CHECK( symbol->GetExcludedFromBOM( &m_schematic->Hierarchy()[0], variantName ) );

    // Test exclude from simulation property variant.
    BOOST_CHECK( !symbol->GetExcludedFromSim() );
    symbol->SetExcludedFromSim( true, &m_schematic->Hierarchy()[0], variantName );
    BOOST_CHECK( symbol->GetExcludedFromSim( &m_schematic->Hierarchy()[0], variantName ) );

    // Test a value field variant change.
    BOOST_CHECK( symbol->GetField( FIELD_T::VALUE )->GetShownText( &m_schematic->Hierarchy()[0],
                                                                   false, 0 ) == wxS( "1K" ) );
    symbol->GetField( FIELD_T::VALUE )->SetText( wxS( "10K" ), &m_schematic->Hierarchy()[0], variantName );
    BOOST_CHECK( symbol->GetField( FIELD_T::VALUE )->GetShownText( &m_schematic->Hierarchy()[0],
                                                                   false, 0, variantName ) == wxS( "10K" ) );
    // BOOST_CHECK( symbol->GetFieldText( FIELD_T::VALUE, &m_schematic->Hierarchy()[0], variantName ) == wxS( "10K" ) );
}


BOOST_AUTO_TEST_SUITE_END()
