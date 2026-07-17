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
 * @file test_issue24915_job_variant_fields.cpp
 * Test for issue #24915: the BOM job edit dialog must populate its preview data with
 * the job's saved variant, not the schematic's currently active variant.
 *
 * The dialog fix routes the job's variant into the data model. The dialog itself cannot
 * be constructed headlessly, so this test pins the contract the fix depends on: the model
 * resolves everything against its own current variant, regardless of which variant is
 * active on the schematic. Reuses the issue24217 fixture, variant H0 there marks R2 and
 * R3 DNP and overrides R1 to 10k and R3 to 2M.
 */

#include <boost/test/unit_test.hpp>

#include <qa_utils/wx_utils/unit_test_utils.h>

#include <eeschema_helpers.h>
#include <fields_data_model.h>
#include <locale_io.h>
#include <sch_reference_list.h>
#include <sch_sheet_path.h>
#include <sch_symbol.h>
#include <schematic.h>


namespace
{

struct ISSUE24915_FIXTURE
{
    void LoadFixture()
    {
        wxString schPath =
                wxString::FromUTF8( KI_TEST::GetEeschemaTestDataDir() ) + wxS( "issue24217/issue24217.kicad_sch" );

        m_schematic = EESCHEMA_HELPERS::LoadSchematic( schPath, true, true );
        BOOST_REQUIRE( m_schematic != nullptr );

        m_schematic->Hierarchy().GetSymbols( m_refs, SYMBOL_FILTER_NON_POWER, false );
        BOOST_REQUIRE_EQUAL( m_refs.GetCount(), 3u );
    }

    const SCH_REFERENCE& FindRef( const wxString& aRef ) const
    {
        for( size_t i = 0; i < m_refs.GetCount(); ++i )
        {
            if( m_refs[i].GetRef() == aRef )
                return m_refs[i];
        }

        BOOST_FAIL( "reference not found" );
        return m_refs[0];
    }

    wxString ModelValue( FIELDS_EDITOR_GRID_DATA_MODEL& aModel, const wxString& aRef, const wxString& aField ) const
    {
        int col = aModel.GetFieldNameCol( aField );
        BOOST_REQUIRE( col >= 0 );

        DATA_MODEL_ROW group( FindRef( aRef ), GROUP_SINGLETON );
        return aModel.GetValue( group, col );
    }

    // The export preview path, generated fields are re-resolved live instead of read
    // from the data store.
    wxString ExportValue( FIELDS_EDITOR_GRID_DATA_MODEL& aModel, const wxString& aRef, const wxString& aField ) const
    {
        int col = aModel.GetFieldNameCol( aField );
        BOOST_REQUIRE( col >= 0 );

        DATA_MODEL_ROW group( FindRef( aRef ), GROUP_SINGLETON );
        return aModel.GetValue( group, col, wxS( ", " ), wxS( "-" ), true, false );
    }

    LOCALE_IO          m_locale;
    SCHEMATIC*         m_schematic = nullptr;
    SCH_REFERENCE_LIST m_refs;
};

} // namespace


BOOST_FIXTURE_TEST_CASE( JobVariantDrivesFieldsDataStore, ISSUE24915_FIXTURE )
{
    LoadFixture();

    // The failing scenario from the issue: the schematic sits on the default variant
    // while the reopened job carries its own saved variant.
    m_schematic->SetCurrentVariant( wxEmptyString );

    FIELDS_EDITOR_GRID_DATA_MODEL jobModel( m_refs, nullptr );
    jobModel.SetCurrentVariant( wxS( "H0" ) );
    jobModel.AddColumn( wxS( "Value" ), wxS( "Value" ), false );
    jobModel.AddColumn( wxS( "${DNP}" ), wxS( "DNP" ), false );

    BOOST_CHECK_EQUAL( ModelValue( jobModel, wxS( "R1" ), wxS( "Value" ) ), wxS( "10k" ) );
    BOOST_CHECK_EQUAL( ModelValue( jobModel, wxS( "R3" ), wxS( "Value" ) ), wxS( "2M" ) );
    BOOST_CHECK_EQUAL( ModelValue( jobModel, wxS( "R2" ), wxS( "${DNP}" ) ), wxS( "1" ) );
    BOOST_CHECK_EQUAL( ModelValue( jobModel, wxS( "R1" ), wxS( "${DNP}" ) ), wxS( "0" ) );

    // The export path must follow the model's variant too, not the schematic's.
    BOOST_CHECK_EQUAL( ExportValue( jobModel, wxS( "R2" ), wxS( "${DNP}" ) ), wxS( "DNP" ) );
    BOOST_CHECK_EQUAL( ExportValue( jobModel, wxS( "R1" ), wxS( "${DNP}" ) ), wxEmptyString );
    BOOST_CHECK_EQUAL( ExportValue( jobModel, wxS( "R1" ), wxS( "Value" ) ), wxS( "10k" ) );
}


BOOST_FIXTURE_TEST_CASE( SchematicVariantDoesNotLeakIntoDataStore, ISSUE24915_FIXTURE )
{
    LoadFixture();

    // Control for the same contract from the other side: with H0 active on the
    // schematic, a store populated for the default variant must show base values.
    m_schematic->SetCurrentVariant( wxS( "H0" ) );

    FIELDS_EDITOR_GRID_DATA_MODEL baseModel( m_refs, nullptr );
    baseModel.AddColumn( wxS( "Value" ), wxS( "Value" ), false );
    baseModel.AddColumn( wxS( "${DNP}" ), wxS( "DNP" ), false );

    BOOST_CHECK_EQUAL( ModelValue( baseModel, wxS( "R1" ), wxS( "Value" ) ), wxS( "6k2" ) );
    BOOST_CHECK_EQUAL( ModelValue( baseModel, wxS( "R3" ), wxS( "Value" ) ), wxS( "1M" ) );
    BOOST_CHECK_EQUAL( ModelValue( baseModel, wxS( "R2" ), wxS( "${DNP}" ) ), wxS( "0" ) );
}
