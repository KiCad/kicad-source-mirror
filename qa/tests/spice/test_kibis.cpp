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

#include <qa_utils/wx_utils/unit_test_utils.h>

#include <sim/kibis/kibis.h>

namespace
{
std::string GetLibraryPath( const std::string& aBaseName )
{
    wxFileName fn( KI_TEST::GetEeschemaTestDataDir() );
    fn.SetName( aBaseName );
    fn.SetExt( "ibs" );
    return std::string( fn.GetFullPath().ToUTF8() );
}
} // namespace


BOOST_AUTO_TEST_SUITE( Kibis )


BOOST_AUTO_TEST_CASE( Null )
{
    KIBIS kibis;

    BOOST_TEST( !kibis.m_valid );

    // IBIS_BASE interface
    // If this isn't null, it's uninited and access will crash
    BOOST_REQUIRE( !kibis.m_Reporter );

    // Doesn't crash (also doesn't do anything)
    kibis.Report( "Dummy", RPT_SEVERITY_INFO );
}


BOOST_AUTO_TEST_CASE( Load_v1_1 )
{
    WX_STRING_REPORTER reporter;

    std::string path = GetLibraryPath( "ibis_v1_1" );
    KIBIS       top( path, &reporter );

    BOOST_TEST_INFO( "Parsed: " << path );
    BOOST_TEST_INFO( "Reported: " << reporter.GetMessages() );

    BOOST_TEST( top.m_valid );

    KIBIS_MODEL* model = top.GetModel( "Input" );

    BOOST_REQUIRE( model != nullptr );
    BOOST_TEST_INFO( "Model: " << model->m_name );

    BOOST_TEST( model->m_name == "Input" );
    BOOST_TEST( (int) model->m_type == (int) IBIS_MODEL_TYPE::INPUT_STD );
    BOOST_TEST( (int) model->m_polarity == (int) IBIS_MODEL_POLARITY::NON_INVERTING );
    BOOST_TEST( (int) model->m_enable = (int) IBIS_MODEL_ENABLE::ACTIVE_HIGH );

    BOOST_TEST( model->HasGNDClamp() );

    KIBIS_COMPONENT* comp = top.GetComponent( "Virtual" );

    BOOST_REQUIRE( comp != nullptr );

    BOOST_TEST_INFO( "Component: " << comp->m_name );

    BOOST_TEST( comp->m_name == "Virtual" );
    BOOST_TEST( comp->m_pins.size() == 4 );
}

BOOST_AUTO_TEST_CASE( Load_v5_1, * boost::unit_test::tolerance( 1e-15 ) )
{
    WX_STRING_REPORTER reporter;

    std::string path = GetLibraryPath( "ibis_v5_1" );
    KIBIS       top( path, &reporter );

    BOOST_TEST_INFO( "Parsed: " << path );
    BOOST_TEST_INFO( "Reported: " << reporter.GetMessages() );

    BOOST_TEST( top.m_valid );

    KIBIS_COMPONENT* comp = top.GetComponent( "Virtual" );

    BOOST_REQUIRE( comp != nullptr );
    BOOST_TEST_INFO( "Component: " << comp->m_name );

    BOOST_TEST( comp->m_name == "Virtual" );
    BOOST_TEST( comp->m_manufacturer == "KiCad" );
    BOOST_TEST( comp->m_pins.size() == 4 );

    KIBIS_PIN* pin;

    pin = comp->GetPin( "A1" );

    BOOST_REQUIRE( pin != nullptr );

    BOOST_TEST( pin->m_signalName == "VSS" );
    BOOST_TEST( pin->m_pinNumber == "A1" );
    BOOST_TEST( pin->m_Rpin.value[0] == 0.246 );
    BOOST_TEST( pin->m_Rpin.value[1] == 0.165 );
    BOOST_TEST( pin->m_Rpin.value[2] == 0.377 );
    BOOST_TEST( pin->m_Lpin.value[0] == 1.49e-9 );
    BOOST_TEST( pin->m_Lpin.value[1] == 0.98e-9 );
    BOOST_TEST( pin->m_Lpin.value[2] == 2.23e-9 );
    BOOST_TEST( pin->m_Cpin.value[0] == 0.40e-12 );
    BOOST_TEST( pin->m_Cpin.value[1] == 0.29e-12 );
    BOOST_TEST( pin->m_Cpin.value[2] == 0.56e-12 );
    BOOST_TEST( pin->m_models.size() == 0 );

    pin = comp->GetPin( "A2" );

    BOOST_REQUIRE( pin != nullptr );

    BOOST_TEST( pin->m_signalName == "VDD" );
    BOOST_TEST( pin->m_pinNumber == "A2" );
    BOOST_TEST( pin->m_Rpin.value[0] == 0.246 );
    BOOST_TEST( pin->m_Rpin.value[1] == 0.165 );
    BOOST_TEST( pin->m_Rpin.value[2] == 0.377 );
    BOOST_TEST( pin->m_Lpin.value[0] == 1.49e-9 );
    BOOST_TEST( pin->m_Lpin.value[1] == 0.98e-9 );
    BOOST_TEST( pin->m_Lpin.value[2] == 2.23e-9 );
    BOOST_TEST( pin->m_Cpin.value[0] == 0.40e-12 );
    BOOST_TEST( pin->m_Cpin.value[1] == 0.29e-12 );
    BOOST_TEST( pin->m_Cpin.value[2] == 0.56e-12 );
    BOOST_TEST( pin->m_models.size() == 0 );

    pin = comp->GetPin( "B1" );

    BOOST_REQUIRE( pin != nullptr );

    BOOST_TEST( pin->m_signalName == "A0" );
    BOOST_TEST( pin->m_pinNumber == "B1" );
    BOOST_TEST( pin->m_Rpin.value[0] == 0.225 );
    BOOST_TEST( pin->m_Rpin.value[1] == 0.225 );
    BOOST_TEST( pin->m_Rpin.value[2] == 0.225 );
    BOOST_TEST( pin->m_Lpin.value[0] == 1.18e-9 );
    BOOST_TEST( pin->m_Lpin.value[1] == 1.18e-9 );
    BOOST_TEST( pin->m_Lpin.value[2] == 1.18e-9 );
    BOOST_TEST( pin->m_Cpin.value[0] == 0.39e-12 );
    BOOST_TEST( pin->m_Cpin.value[1] == 0.39e-12 );
    BOOST_TEST( pin->m_Cpin.value[2] == 0.39e-12 );
    BOOST_TEST( pin->m_models.size() == 1 );
    BOOST_TEST( pin->m_models[0]->m_name == "AC40" );

    pin = comp->GetPin( "B2" );

    BOOST_REQUIRE( pin != nullptr );

    BOOST_TEST( pin->m_signalName == "DQ0" );
    BOOST_TEST( pin->m_pinNumber == "B2" );
    BOOST_TEST( pin->m_Rpin.value[0] == 0.214 );
    BOOST_TEST( pin->m_Rpin.value[1] == 0.214 );
    BOOST_TEST( pin->m_Rpin.value[2] == 0.214 );
    BOOST_TEST( pin->m_Lpin.value[0] == 1.05e-9 );
    BOOST_TEST( pin->m_Lpin.value[1] == 1.05e-9 );
    BOOST_TEST( pin->m_Lpin.value[2] == 1.05e-9 );
    BOOST_TEST( pin->m_Cpin.value[0] == 0.39e-12 );
    BOOST_TEST( pin->m_Cpin.value[1] == 0.39e-12 );
    BOOST_TEST( pin->m_Cpin.value[2] == 0.39e-12 );
    BOOST_TEST( pin->m_models.size() == 2 );
    BOOST_TEST( pin->m_models[0]->m_name == "DQ40" );
    BOOST_TEST( pin->m_models[1]->m_name == "DQ40_ODT40" );

    KIBIS_MODEL* model;

    model = top.GetModel( "AC40" );

    BOOST_REQUIRE( model != nullptr );
    BOOST_TEST_INFO( "Model: " << model->m_name );

    BOOST_TEST( model->m_name == "AC40" );
    BOOST_TEST( (int) model->m_type == (int) IBIS_MODEL_TYPE::OUTPUT );
    BOOST_TEST( (int) model->m_polarity == (int) IBIS_MODEL_POLARITY::NON_INVERTING );
    BOOST_TEST( (int) model->m_enable = (int) IBIS_MODEL_ENABLE::ACTIVE_HIGH );
    BOOST_TEST( model->m_vmeas == 0.675 );
    BOOST_TEST( model->m_cref == 5e-12 );
    BOOST_TEST( model->m_rref == 50.0 );
    BOOST_TEST( model->m_vref == 0.675 );
    BOOST_TEST( model->m_C_comp.value[0] == 2.68e-12 );
    BOOST_TEST( model->m_C_comp.value[1] == 2.64e-12 );
    BOOST_TEST( model->m_C_comp.value[2] == 2.75e-12 );
    BOOST_TEST( model->m_voltageRange.value[0] == 1.35 );
    BOOST_TEST( model->m_voltageRange.value[1] == 1.28 );
    BOOST_TEST( model->m_voltageRange.value[2] == 1.42 );
    BOOST_TEST( model->m_temperatureRange.value[0] == 50.0 );
    BOOST_TEST( model->m_temperatureRange.value[1] == 100.0 );
    BOOST_TEST( model->m_temperatureRange.value[2] == 0.0 );

    BOOST_TEST( model->HasGNDClamp() );
    BOOST_TEST( model->m_GNDClamp.m_entries.size() == 4 );
    BOOST_TEST( model->m_GNDClamp.m_entries[0].V == -1.35 );
    BOOST_TEST( model->m_GNDClamp.m_entries[0].I.value[0] == -26.89e-3 );
    BOOST_TEST( model->m_GNDClamp.m_entries[0].I.value[1] == -20.44e-3 );
    BOOST_TEST( model->m_GNDClamp.m_entries[0].I.value[2] == -35.94e-3 );
    BOOST_TEST( model->m_GNDClamp.m_entries[1].V == 0.0 );
    BOOST_TEST( model->m_GNDClamp.m_entries[1].I.value[0] == -29.09e-9 );
    BOOST_TEST( model->m_GNDClamp.m_entries[1].I.value[1] == -44.53e-9 );
    BOOST_TEST( model->m_GNDClamp.m_entries[1].I.value[2] == -51.87e-9 );
    BOOST_TEST( model->m_GNDClamp.m_entries[2].V == 1.35 );
    BOOST_TEST( model->m_GNDClamp.m_entries[2].I.value[0] == 0.518e-6 );
    BOOST_TEST( model->m_GNDClamp.m_entries[2].I.value[1] == 0.0 );
    BOOST_TEST( model->m_GNDClamp.m_entries[2].I.value[2] == 0.444e-6 );
    BOOST_TEST( model->m_GNDClamp.m_entries[3].V == 2.7 );
    BOOST_TEST( model->m_GNDClamp.m_entries[3].I.value[0] == 0.0 );
    BOOST_TEST( model->m_GNDClamp.m_entries[3].I.value[1] == 0.0 );
    BOOST_TEST( model->m_GNDClamp.m_entries[3].I.value[2] == 0.0 );

    BOOST_TEST( model->HasPOWERClamp() );
    BOOST_TEST( model->m_POWERClamp.m_entries.size() == 4 );
    BOOST_TEST( model->m_POWERClamp.m_entries[0].V == -1.35 );
    BOOST_TEST( model->m_POWERClamp.m_entries[0].I.value[0] == 55.06e-3 );
    BOOST_TEST( model->m_POWERClamp.m_entries[0].I.value[1] == 46.76e-3 );
    BOOST_TEST( model->m_POWERClamp.m_entries[0].I.value[2] == 74.09e-3 );
    BOOST_TEST( model->m_POWERClamp.m_entries[1].V == 0.0 );
    BOOST_TEST( model->m_POWERClamp.m_entries[1].I.value[0] == 0.0 );
    BOOST_TEST( model->m_POWERClamp.m_entries[1].I.value[1] == 0.515e-6 );
    BOOST_TEST( model->m_POWERClamp.m_entries[1].I.value[2] == 74.94e-9 );
    BOOST_TEST( model->m_POWERClamp.m_entries[2].V == 1.35 );
    BOOST_TEST( model->m_POWERClamp.m_entries[2].I.value[0] == 0.0 );
    BOOST_TEST( model->m_POWERClamp.m_entries[2].I.value[1] == 0.0 );
    BOOST_TEST( model->m_POWERClamp.m_entries[2].I.value[2] == 0.0 );
    BOOST_TEST( model->m_POWERClamp.m_entries[3].V == 2.7 );
    BOOST_TEST( model->m_POWERClamp.m_entries[3].I.value[0] == 0.0 );
    BOOST_TEST( model->m_POWERClamp.m_entries[3].I.value[1] == 0.0 );
    BOOST_TEST( model->m_POWERClamp.m_entries[3].I.value[2] == 0.0 );

    BOOST_TEST( model->HasPullup() );
    BOOST_TEST( model->m_pullup.m_entries.size() == 4 );
    BOOST_TEST( model->m_pullup.m_entries[0].V == -1.35 );
    BOOST_TEST( model->m_pullup.m_entries[0].I.value[0] == 18.04e-3 );
    BOOST_TEST( model->m_pullup.m_entries[0].I.value[1] == 16.15e-3 );
    BOOST_TEST( model->m_pullup.m_entries[0].I.value[2] == 17.46e-3 );
    BOOST_TEST( model->m_pullup.m_entries[1].V == 0.0 );
    BOOST_TEST( model->m_pullup.m_entries[1].I.value[0] == 3.412e-9 );
    BOOST_TEST( model->m_pullup.m_entries[1].I.value[1] == 0.527e-6 );
    BOOST_TEST( model->m_pullup.m_entries[1].I.value[2] == 0.327e-6 );
    BOOST_TEST( model->m_pullup.m_entries[2].V == 1.35 );
    BOOST_TEST( model->m_pullup.m_entries[2].I.value[0] == -26.46e-3 );
    BOOST_TEST( model->m_pullup.m_entries[2].I.value[1] == -24.75e-3 );
    BOOST_TEST( model->m_pullup.m_entries[2].I.value[2] == -22.82e-3 );
    BOOST_TEST( model->m_pullup.m_entries[3].V == 2.7 );
    BOOST_TEST( model->m_pullup.m_entries[3].I.value[0] == -31.03e-3 );
    BOOST_TEST( model->m_pullup.m_entries[3].I.value[1] == -28.82e-3 );
    BOOST_TEST( model->m_pullup.m_entries[3].I.value[2] == -27.31e-3 );

    BOOST_TEST( model->HasPulldown() );
    BOOST_TEST( model->m_pulldown.m_entries.size() == 4 );
    BOOST_TEST( model->m_pulldown.m_entries[0].V == -1.35 );
    BOOST_TEST( model->m_pulldown.m_entries[0].I.value[0] == -23.01e-3 );
    BOOST_TEST( model->m_pulldown.m_entries[0].I.value[1] == -20.02e-3 );
    BOOST_TEST( model->m_pulldown.m_entries[0].I.value[2] == -20.46e-3 );
    BOOST_TEST( model->m_pulldown.m_entries[1].V == 0.0 );
    BOOST_TEST( model->m_pulldown.m_entries[1].I.value[0] == -38.81e-9 );
    BOOST_TEST( model->m_pulldown.m_entries[1].I.value[1] == -32.33e-9 );
    BOOST_TEST( model->m_pulldown.m_entries[1].I.value[2] == -49.77e-9 );
    BOOST_TEST( model->m_pulldown.m_entries[2].V == 1.35 );
    BOOST_TEST( model->m_pulldown.m_entries[2].I.value[0] == 26.98e-3 );
    BOOST_TEST( model->m_pulldown.m_entries[2].I.value[1] == 23.63e-3 );
    BOOST_TEST( model->m_pulldown.m_entries[2].I.value[2] == 26.60e-3 );
    BOOST_TEST( model->m_pulldown.m_entries[3].V == 2.7 );
    BOOST_TEST( model->m_pulldown.m_entries[3].I.value[0] == 29.20e-3 );
    BOOST_TEST( model->m_pulldown.m_entries[3].I.value[1] == 25.40e-3 );
    BOOST_TEST( model->m_pulldown.m_entries[3].I.value[2] == 29.03e-3 );

    BOOST_TEST( model->m_ramp.m_Rload == 50.0 );
    BOOST_TEST( model->m_ramp.m_rising.value[0].m_dv == 0.462 );
    BOOST_TEST( model->m_ramp.m_rising.value[0].m_dt == 0.131e-9 );
    BOOST_TEST( model->m_ramp.m_rising.value[1].m_dv == 0.439 );
    BOOST_TEST( model->m_ramp.m_rising.value[1].m_dt == 0.138e-9 );
    BOOST_TEST( model->m_ramp.m_rising.value[2].m_dv == 0.462 );
    BOOST_TEST( model->m_ramp.m_rising.value[2].m_dt == 0.158e-9 );
    BOOST_TEST( model->m_ramp.m_falling.value[0].m_dv == 0.465 );
    BOOST_TEST( model->m_ramp.m_falling.value[0].m_dt == 0.116e-9 );
    BOOST_TEST( model->m_ramp.m_falling.value[1].m_dv == 0.438 );
    BOOST_TEST( model->m_ramp.m_falling.value[1].m_dt == 0.126e-9 );
    BOOST_TEST( model->m_ramp.m_falling.value[2].m_dv == 0.468 );
    BOOST_TEST( model->m_ramp.m_falling.value[2].m_dt == 0.117e-9 );

    BOOST_TEST( model->m_risingWaveforms.size() == 2 );
    BOOST_TEST( (int) model->m_risingWaveforms[1]->m_type == (int) IBIS_WAVEFORM_TYPE::RISING );
    BOOST_TEST( model->m_risingWaveforms[1]->m_R_fixture == 50.0 );
    BOOST_TEST( model->m_risingWaveforms[1]->m_V_fixture == 1.35 );
    BOOST_TEST( model->m_risingWaveforms[1]->m_V_fixture_min == 1.28 );
    BOOST_TEST( model->m_risingWaveforms[1]->m_V_fixture_max == 1.42 );
    BOOST_TEST( model->m_risingWaveforms[1]->m_table.m_entries.size() == 5 );
    BOOST_TEST( model->m_risingWaveforms[1]->m_table.m_entries[0].t == 0 );
    BOOST_TEST( model->m_risingWaveforms[1]->m_table.m_entries[0].V.value[0] == 0.573 );
    BOOST_TEST( model->m_risingWaveforms[1]->m_table.m_entries[0].V.value[1] == 0.550 );
    BOOST_TEST( model->m_risingWaveforms[1]->m_table.m_entries[0].V.value[2] == 0.636 );
    BOOST_TEST( model->m_risingWaveforms[1]->m_table.m_entries[1].t == 1.001e-9 );
    BOOST_TEST( model->m_risingWaveforms[1]->m_table.m_entries[1].V.value[0] == 0.574 );
    BOOST_TEST( model->m_risingWaveforms[1]->m_table.m_entries[1].V.value[1] == 0.551 );
    BOOST_TEST( model->m_risingWaveforms[1]->m_table.m_entries[1].V.value[2] == 0.906 );
    BOOST_TEST( model->m_risingWaveforms[1]->m_table.m_entries[2].t == 2e-9 );
    BOOST_TEST( model->m_risingWaveforms[1]->m_table.m_entries[2].V.value[0] == 1.348 );
    BOOST_TEST( model->m_risingWaveforms[1]->m_table.m_entries[2].V.value[1] == 1.269 );
    BOOST_TEST( model->m_risingWaveforms[1]->m_table.m_entries[2].V.value[2] == 1.416 );
    BOOST_TEST( model->m_risingWaveforms[1]->m_table.m_entries[3].t == 2.491e-9 );
    BOOST_TEST( model->m_risingWaveforms[1]->m_table.m_entries[3].V.value[0] == 1.349 );
    BOOST_TEST( model->m_risingWaveforms[1]->m_table.m_entries[3].V.value[1] == 1.280 );
    BOOST_TEST( model->m_risingWaveforms[1]->m_table.m_entries[3].V.value[2] == 1.416 );
    BOOST_TEST( model->m_risingWaveforms[1]->m_table.m_entries[4].t == 10e-9 );
    BOOST_TEST( model->m_risingWaveforms[1]->m_table.m_entries[4].V.value[0] == 1.349 );
    BOOST_TEST( model->m_risingWaveforms[1]->m_table.m_entries[4].V.value[1] == 1.282 );
    BOOST_TEST( model->m_risingWaveforms[1]->m_table.m_entries[4].V.value[2] == 1.417 );

    BOOST_TEST( model->m_fallingWaveforms.size() == 2 );
    BOOST_TEST( (int) model->m_fallingWaveforms[1]->m_type == (int) IBIS_WAVEFORM_TYPE::FALLING );
    BOOST_TEST( model->m_fallingWaveforms[1]->m_R_fixture == 50.0 );
    BOOST_TEST( model->m_fallingWaveforms[1]->m_V_fixture == 1.35 );
    BOOST_TEST( model->m_fallingWaveforms[1]->m_V_fixture_min == 1.28 );
    BOOST_TEST( model->m_fallingWaveforms[1]->m_V_fixture_max == 1.42 );
    BOOST_TEST( model->m_fallingWaveforms[1]->m_table.m_entries.size() == 5 );
    BOOST_TEST( model->m_fallingWaveforms[1]->m_table.m_entries[0].t == 0 );
    BOOST_TEST( model->m_fallingWaveforms[1]->m_table.m_entries[0].V.value[0] == 1.349 );
    BOOST_TEST( model->m_fallingWaveforms[1]->m_table.m_entries[0].V.value[1] == 1.282 );
    BOOST_TEST( model->m_fallingWaveforms[1]->m_table.m_entries[0].V.value[2] == 1.417 );
    BOOST_TEST( model->m_fallingWaveforms[1]->m_table.m_entries[1].t == 1.02e-9 );
    BOOST_TEST( model->m_fallingWaveforms[1]->m_table.m_entries[1].V.value[0] == 1.350 );
    BOOST_TEST( model->m_fallingWaveforms[1]->m_table.m_entries[1].V.value[1] == 1.283 );
    BOOST_TEST( model->m_fallingWaveforms[1]->m_table.m_entries[1].V.value[2] == 1.382 );
    BOOST_TEST( model->m_fallingWaveforms[1]->m_table.m_entries[2].t == 1.988e-9 );
    BOOST_TEST( model->m_fallingWaveforms[1]->m_table.m_entries[2].V.value[0] == 0.576 );
    BOOST_TEST( model->m_fallingWaveforms[1]->m_table.m_entries[2].V.value[1] == 0.606 );
    BOOST_TEST( model->m_fallingWaveforms[1]->m_table.m_entries[2].V.value[2] == 0.638 );
    BOOST_TEST( model->m_fallingWaveforms[1]->m_table.m_entries[3].t == 2.51e-9 );
    BOOST_TEST( model->m_fallingWaveforms[1]->m_table.m_entries[3].V.value[0] == 0.574 );
    BOOST_TEST( model->m_fallingWaveforms[1]->m_table.m_entries[3].V.value[1] == 0.552 );
    BOOST_TEST( model->m_fallingWaveforms[1]->m_table.m_entries[3].V.value[2] == 0.637 );
    BOOST_TEST( model->m_fallingWaveforms[1]->m_table.m_entries[4].t == 10e-9 );
    BOOST_TEST( model->m_fallingWaveforms[1]->m_table.m_entries[4].V.value[0] == 0.573 );
    BOOST_TEST( model->m_fallingWaveforms[1]->m_table.m_entries[4].V.value[1] == 0.550 );
    BOOST_TEST( model->m_fallingWaveforms[1]->m_table.m_entries[4].V.value[2] == 0.636 );

    BOOST_TEST( model->m_submodels.size() == 0 );

    model = top.GetModel( "DQ40_ODT40" );

    BOOST_REQUIRE( model != nullptr );
    BOOST_TEST_INFO( "Model: " << model->m_name );

    BOOST_TEST( model->m_name == "DQ40_ODT40" );
    BOOST_TEST( (int) model->m_type == (int) IBIS_MODEL_TYPE::IO );
    BOOST_TEST( model->m_vinl == 0.515 );
    BOOST_TEST( model->m_vinh == 0.835 );
    BOOST_TEST( model->m_vmeas == 0.675 );
    BOOST_TEST( model->m_cref == 0.0 );
    BOOST_TEST( model->m_rref == 25.0 );
    BOOST_TEST( model->m_vref == 0.675 );
    BOOST_TEST( model->m_C_comp.value[0] == 1.36e-12 );
    BOOST_TEST( model->m_C_comp.value[1] == 1.26e-12 );
    BOOST_TEST( model->m_C_comp.value[2] == 1.46e-12 );
    BOOST_TEST( model->m_voltageRange.value[0] == 1.35 );
    BOOST_TEST( model->m_voltageRange.value[1] == 1.28 );
    BOOST_TEST( model->m_voltageRange.value[2] == 1.42 );
    BOOST_TEST( model->m_temperatureRange.value[0] == 50.0 );
    BOOST_TEST( model->m_temperatureRange.value[1] == 100.0 );
    BOOST_TEST( model->m_temperatureRange.value[2] == 0.0 );

    BOOST_TEST( model->HasGNDClamp() );
    BOOST_TEST( model->m_GNDClamp.m_entries.size() == 4 );
    BOOST_TEST( model->m_GNDClamp.m_entries[0].V == -1.35 );
    BOOST_TEST( model->m_GNDClamp.m_entries[0].I.value[0] == -39.74e-3 );
    BOOST_TEST( model->m_GNDClamp.m_entries[0].I.value[1] == -37.76e-3 );
    BOOST_TEST( model->m_GNDClamp.m_entries[0].I.value[2] == -41.37e-3 );
    BOOST_TEST( model->m_GNDClamp.m_entries[1].V == -0.175 );
    BOOST_TEST( std::isnan( model->m_GNDClamp.m_entries[1].I.value[0] ) );
    BOOST_TEST( std::isnan( model->m_GNDClamp.m_entries[1].I.value[1] ) );
    BOOST_TEST( model->m_GNDClamp.m_entries[1].I.value[2] == 0.0 );
    BOOST_TEST( model->m_GNDClamp.m_entries[2].V == 1.35 );
    BOOST_TEST( model->m_GNDClamp.m_entries[2].I.value[0] == 0.0 );
    BOOST_TEST( model->m_GNDClamp.m_entries[2].I.value[1] == 0.0 );
    BOOST_TEST( model->m_GNDClamp.m_entries[2].I.value[2] == 0.0 );
    BOOST_TEST( model->m_GNDClamp.m_entries[3].V == 2.7 );
    BOOST_TEST( model->m_GNDClamp.m_entries[3].I.value[0] == 0.0 );
    BOOST_TEST( model->m_GNDClamp.m_entries[3].I.value[1] == 0.0 );
    BOOST_TEST( model->m_GNDClamp.m_entries[3].I.value[2] == 0.0 );

    BOOST_TEST( model->HasPOWERClamp() );
    BOOST_TEST( model->m_POWERClamp.m_entries.size() == 4 );
    BOOST_TEST( model->m_POWERClamp.m_entries[0].V == -1.35 );
    BOOST_TEST( model->m_POWERClamp.m_entries[0].I.value[0] == 88.45e-3 );
    BOOST_TEST( model->m_POWERClamp.m_entries[0].I.value[1] == 79.46e-3 );
    BOOST_TEST( model->m_POWERClamp.m_entries[0].I.value[2] == 95.25e-3 );
    BOOST_TEST( model->m_POWERClamp.m_entries[1].V == -0.28 );
    BOOST_TEST( std::isnan( model->m_POWERClamp.m_entries[1].I.value[0] ) );
    BOOST_TEST( model->m_POWERClamp.m_entries[1].I.value[1] == 0.0 );
    BOOST_TEST( std::isnan( model->m_POWERClamp.m_entries[1].I.value[2] ) );
    BOOST_TEST( model->m_POWERClamp.m_entries[2].V == 1.35 );
    BOOST_TEST( model->m_POWERClamp.m_entries[2].I.value[0] == 0.0 );
    BOOST_TEST( model->m_POWERClamp.m_entries[2].I.value[1] == 0.0 );
    BOOST_TEST( model->m_POWERClamp.m_entries[2].I.value[2] == 0.0 );
    BOOST_TEST( model->m_POWERClamp.m_entries[3].V == 2.7 );
    BOOST_TEST( model->m_POWERClamp.m_entries[3].I.value[0] == 0.0 );
    BOOST_TEST( model->m_POWERClamp.m_entries[3].I.value[1] == 0.0 );
    BOOST_TEST( model->m_POWERClamp.m_entries[3].I.value[2] == 0.0 );

    BOOST_TEST( model->HasPullup() );
    BOOST_TEST( model->m_pullup.m_entries.size() == 4 );
    BOOST_TEST( model->m_pullup.m_entries[0].V == -1.35 );
    BOOST_TEST( model->m_pullup.m_entries[0].I.value[0] == 7.027e-3 );
    BOOST_TEST( model->m_pullup.m_entries[0].I.value[1] == 7.411e-3 );
    BOOST_TEST( model->m_pullup.m_entries[0].I.value[2] == 8.620e-3 );
    BOOST_TEST( model->m_pullup.m_entries[1].V == 0.0 );
    BOOST_TEST( model->m_pullup.m_entries[1].I.value[0] == 31.17e-6 );
    BOOST_TEST( model->m_pullup.m_entries[1].I.value[1] == 35.34e-6 );
    BOOST_TEST( model->m_pullup.m_entries[1].I.value[2] == 10.66e-6 );
    BOOST_TEST( model->m_pullup.m_entries[2].V == 1.35 );
    BOOST_TEST( model->m_pullup.m_entries[2].I.value[0] == -21.21e-3 );
    BOOST_TEST( model->m_pullup.m_entries[2].I.value[1] == -19.19e-3 );
    BOOST_TEST( model->m_pullup.m_entries[2].I.value[2] == -25.02e-3 );
    BOOST_TEST( model->m_pullup.m_entries[3].V == 2.7 );
    BOOST_TEST( model->m_pullup.m_entries[3].I.value[0] == -25.77e-3 );
    BOOST_TEST( model->m_pullup.m_entries[3].I.value[1] == -23.32e-3 );
    BOOST_TEST( model->m_pullup.m_entries[3].I.value[2] == -30.53e-3 );

    BOOST_TEST( model->HasPulldown() );
    BOOST_TEST( model->m_pulldown.m_entries.size() == 4 );
    BOOST_TEST( model->m_pulldown.m_entries[0].V == -1.35 );
    BOOST_TEST( model->m_pulldown.m_entries[0].I.value[0] == -13.96e-3 );
    BOOST_TEST( model->m_pulldown.m_entries[0].I.value[1] == -12.59e-3 );
    BOOST_TEST( model->m_pulldown.m_entries[0].I.value[2] == -15.56e-3 );
    BOOST_TEST( model->m_pulldown.m_entries[1].V == 0.0 );
    BOOST_TEST( model->m_pulldown.m_entries[1].I.value[0] == -3.893e-6 );
    BOOST_TEST( model->m_pulldown.m_entries[1].I.value[1] == -4.109e-6 );
    BOOST_TEST( model->m_pulldown.m_entries[1].I.value[2] == -1.483e-6 );
    BOOST_TEST( model->m_pulldown.m_entries[2].V == 1.35 );
    BOOST_TEST( model->m_pulldown.m_entries[2].I.value[0] == 26.08e-3 );
    BOOST_TEST( model->m_pulldown.m_entries[2].I.value[1] == 22.37e-3 );
    BOOST_TEST( model->m_pulldown.m_entries[2].I.value[2] == 30.44e-3 );
    BOOST_TEST( model->m_pulldown.m_entries[3].V == 2.7 );
    BOOST_TEST( model->m_pulldown.m_entries[3].I.value[0] == 30.20e-3 );
    BOOST_TEST( model->m_pulldown.m_entries[3].I.value[1] == 25.46e-3 );
    BOOST_TEST( model->m_pulldown.m_entries[3].I.value[2] == 36.99e-3 );

    BOOST_TEST( model->m_ramp.m_Rload == 50.0 );
    BOOST_TEST( model->m_ramp.m_rising.value[0].m_dv == 0.451 );
    BOOST_TEST( model->m_ramp.m_rising.value[0].m_dt == 134e-12 );
    BOOST_TEST( model->m_ramp.m_rising.value[1].m_dv == 0.424 );
    BOOST_TEST( model->m_ramp.m_rising.value[1].m_dt == 184e-12 );
    BOOST_TEST( model->m_ramp.m_rising.value[2].m_dv == 0.495 );
    BOOST_TEST( model->m_ramp.m_rising.value[2].m_dt == 103e-12 );
    BOOST_TEST( model->m_ramp.m_falling.value[0].m_dv == 0.438 );
    BOOST_TEST( model->m_ramp.m_falling.value[0].m_dt == 120e-12 );
    BOOST_TEST( model->m_ramp.m_falling.value[1].m_dv == 0.411 );
    BOOST_TEST( model->m_ramp.m_falling.value[1].m_dt == 159e-12 );
    BOOST_TEST( model->m_ramp.m_falling.value[2].m_dv == 0.475 );
    BOOST_TEST( model->m_ramp.m_falling.value[2].m_dt == 90e-12 );

    BOOST_TEST( model->m_risingWaveforms.size() == 2 );
    BOOST_TEST( (int) model->m_risingWaveforms[0]->m_type == (int) IBIS_WAVEFORM_TYPE::RISING );
    BOOST_TEST( model->m_risingWaveforms[0]->m_R_fixture == 50.0 );
    BOOST_TEST( model->m_risingWaveforms[0]->m_V_fixture == 1.35 );
    BOOST_TEST( model->m_risingWaveforms[0]->m_V_fixture_min == 1.28 );
    BOOST_TEST( model->m_risingWaveforms[0]->m_V_fixture_max == 1.42 );
    BOOST_TEST( model->m_risingWaveforms[0]->m_table.m_entries.size() == 4 );
    BOOST_TEST( model->m_risingWaveforms[0]->m_table.m_entries[0].t == 0 );
    BOOST_TEST( model->m_risingWaveforms[0]->m_table.m_entries[0].V.value[0] == 0.6179 );
    BOOST_TEST( model->m_risingWaveforms[0]->m_table.m_entries[0].V.value[1] == 0.5969 );
    BOOST_TEST( model->m_risingWaveforms[0]->m_table.m_entries[0].V.value[2] == 0.6325 );
    BOOST_TEST( model->m_risingWaveforms[0]->m_table.m_entries[1].t == 199e-12 );
    BOOST_TEST( model->m_risingWaveforms[0]->m_table.m_entries[1].V.value[0] == 0.9465 );
    BOOST_TEST( model->m_risingWaveforms[0]->m_table.m_entries[1].V.value[1] == 0.8243 );
    BOOST_TEST( model->m_risingWaveforms[0]->m_table.m_entries[1].V.value[2] == 1.1430 );
    BOOST_TEST( model->m_risingWaveforms[0]->m_table.m_entries[2].t == 399e-12 );
    BOOST_TEST( model->m_risingWaveforms[0]->m_table.m_entries[2].V.value[0] == 1.340 );
    BOOST_TEST( model->m_risingWaveforms[0]->m_table.m_entries[2].V.value[1] == 1.261 );
    BOOST_TEST( model->m_risingWaveforms[0]->m_table.m_entries[2].V.value[2] == 1.420 );
    BOOST_TEST( model->m_risingWaveforms[0]->m_table.m_entries[3].t == 937e-12 );
    BOOST_TEST( model->m_risingWaveforms[0]->m_table.m_entries[3].V.value[0] == 1.349 );
    BOOST_TEST( model->m_risingWaveforms[0]->m_table.m_entries[3].V.value[1] == 1.282 );
    BOOST_TEST( model->m_risingWaveforms[0]->m_table.m_entries[3].V.value[2] == 1.424 );

    BOOST_TEST( model->m_fallingWaveforms.size() == 2 );
    BOOST_TEST( (int) model->m_fallingWaveforms[0]->m_type == (int) IBIS_WAVEFORM_TYPE::FALLING );
    BOOST_TEST( model->m_fallingWaveforms[0]->m_R_fixture == 50.0 );
    BOOST_TEST( model->m_fallingWaveforms[0]->m_V_fixture == 1.35 );
    BOOST_TEST( model->m_fallingWaveforms[0]->m_V_fixture_min == 1.28 );
    BOOST_TEST( model->m_fallingWaveforms[0]->m_V_fixture_max == 1.42 );
    BOOST_TEST( model->m_fallingWaveforms[0]->m_table.m_entries.size() == 4 );
    BOOST_TEST( model->m_fallingWaveforms[0]->m_table.m_entries[0].t == 0 );
    BOOST_TEST( model->m_fallingWaveforms[0]->m_table.m_entries[0].V.value[0] == 1.349 );
    BOOST_TEST( model->m_fallingWaveforms[0]->m_table.m_entries[0].V.value[1] == 1.282 );
    BOOST_TEST( model->m_fallingWaveforms[0]->m_table.m_entries[0].V.value[2] == 1.424 );
    BOOST_TEST( model->m_fallingWaveforms[0]->m_table.m_entries[1].t == 198e-12 );
    BOOST_TEST( model->m_fallingWaveforms[0]->m_table.m_entries[1].V.value[0] == 1.270 );
    BOOST_TEST( model->m_fallingWaveforms[0]->m_table.m_entries[1].V.value[1] == 1.248 );
    BOOST_TEST( model->m_fallingWaveforms[0]->m_table.m_entries[1].V.value[2] == 1.203 );
    BOOST_TEST( model->m_fallingWaveforms[0]->m_table.m_entries[2].t == 398e-12 );
    BOOST_TEST( model->m_fallingWaveforms[0]->m_table.m_entries[2].V.value[0] == 0.6815 );
    BOOST_TEST( model->m_fallingWaveforms[0]->m_table.m_entries[2].V.value[1] == 0.7726 );
    BOOST_TEST( model->m_fallingWaveforms[0]->m_table.m_entries[2].V.value[2] == 0.6551 );
    BOOST_TEST( model->m_fallingWaveforms[0]->m_table.m_entries[3].t == 937e-12 );
    BOOST_TEST( model->m_fallingWaveforms[0]->m_table.m_entries[3].V.value[0] == 0.6179 );
    BOOST_TEST( model->m_fallingWaveforms[0]->m_table.m_entries[3].V.value[1] == 0.5969);
    BOOST_TEST( model->m_fallingWaveforms[0]->m_table.m_entries[3].V.value[2] == 0.6325 );

    BOOST_TEST( model->m_submodels.size() == 1 );
    BOOST_TEST( model->m_submodels[0].m_name == "40ohm_ODT" );
    BOOST_TEST( (int) model->m_submodels[0].m_type == (int) IBIS_SUBMODEL_TYPE::DYNAMIC_CLAMP );
    BOOST_TEST( (int) model->m_submodels[0].m_mode == (int) IBIS_SUBMODEL_MODE::NON_DRIVING );

    BOOST_TEST( model->m_submodels[0].HasGNDClamp() );
    BOOST_TEST( model->m_submodels[0].m_GNDClamp.m_entries.size() == 4 );
    BOOST_TEST( model->m_submodels[0].m_GNDClamp.m_entries[0].V == -1.35 );
    BOOST_TEST( model->m_submodels[0].m_GNDClamp.m_entries[0].I.value[0] == -6.975e-3 );
    BOOST_TEST( model->m_submodels[0].m_GNDClamp.m_entries[0].I.value[1] == -6.287e-3 );
    BOOST_TEST( model->m_submodels[0].m_GNDClamp.m_entries[0].I.value[2] == -7.776e-3 );
    BOOST_TEST( model->m_submodels[0].m_GNDClamp.m_entries[1].V == 0.0 );
    BOOST_TEST( model->m_submodels[0].m_GNDClamp.m_entries[1].I.value[0] == -1.585e-6 );
    BOOST_TEST( model->m_submodels[0].m_GNDClamp.m_entries[1].I.value[1] == -2.224e-6 );
    BOOST_TEST( model->m_submodels[0].m_GNDClamp.m_entries[1].I.value[2] == -819.6e-9 );
    BOOST_TEST( model->m_submodels[0].m_GNDClamp.m_entries[2].V == 1.35 );
    BOOST_TEST( model->m_submodels[0].m_GNDClamp.m_entries[2].I.value[0] == 12.86e-3 );
    BOOST_TEST( model->m_submodels[0].m_GNDClamp.m_entries[2].I.value[1] == 11.10e-3 );
    BOOST_TEST( model->m_submodels[0].m_GNDClamp.m_entries[2].I.value[2] == 14.96e-3 );
    BOOST_TEST( model->m_submodels[0].m_GNDClamp.m_entries[3].V == 2.7 );
    BOOST_TEST( model->m_submodels[0].m_GNDClamp.m_entries[3].I.value[0] == 15.10e-3 );
    BOOST_TEST( model->m_submodels[0].m_GNDClamp.m_entries[3].I.value[1] == 12.73e-3 );
    BOOST_TEST( model->m_submodels[0].m_GNDClamp.m_entries[3].I.value[2] == 18.50e-3 );

    BOOST_TEST( model->m_submodels[0].HasPOWERClamp() );
    BOOST_TEST( model->m_submodels[0].m_POWERClamp.m_entries.size() == 4 );
    BOOST_TEST( model->m_submodels[0].m_POWERClamp.m_entries[0].V == -1.35 );
    BOOST_TEST( model->m_submodels[0].m_POWERClamp.m_entries[0].I.value[0] == 3.515e-3 );
    BOOST_TEST( model->m_submodels[0].m_POWERClamp.m_entries[0].I.value[1] == 3.706e-3 );
    BOOST_TEST( model->m_submodels[0].m_POWERClamp.m_entries[0].I.value[2] == 4.312e-3 );
    BOOST_TEST( model->m_submodels[0].m_POWERClamp.m_entries[1].V == 0.0 );
    BOOST_TEST( model->m_submodels[0].m_POWERClamp.m_entries[1].I.value[0] == 13.01e-6 );
    BOOST_TEST( model->m_submodels[0].m_POWERClamp.m_entries[1].I.value[1] == 18.85e-6 );
    BOOST_TEST( model->m_submodels[0].m_POWERClamp.m_entries[1].I.value[2] == 5.723e-6 );
    BOOST_TEST( model->m_submodels[0].m_POWERClamp.m_entries[2].V == 1.35 );
    BOOST_TEST( model->m_submodels[0].m_POWERClamp.m_entries[2].I.value[0] == -10.50e-3 );
    BOOST_TEST( model->m_submodels[0].m_POWERClamp.m_entries[2].I.value[1] == -9.514e-3 );
    BOOST_TEST( model->m_submodels[0].m_POWERClamp.m_entries[2].I.value[2] == -12.39e-3 );
    BOOST_TEST( model->m_submodels[0].m_POWERClamp.m_entries[3].V == 2.7 );
    BOOST_TEST( model->m_submodels[0].m_POWERClamp.m_entries[3].I.value[0] == -12.88e-3 );
    BOOST_TEST( model->m_submodels[0].m_POWERClamp.m_entries[3].I.value[1] == -11.66e-3 );
    BOOST_TEST( model->m_submodels[0].m_POWERClamp.m_entries[3].I.value[2] == -15.26e-3 );
}

BOOST_AUTO_TEST_SUITE_END()
