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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <qa_utils/wx_utils/unit_test_utils.h>

#include <sim/sim_signal_utils.h>

using namespace SIM_SIGNAL;


BOOST_AUTO_TEST_SUITE( SimSignalUtils )


BOOST_AUTO_TEST_CASE( SplitMeasurementKeepsComparisons )
{
    wxString func, signal, params;

    SplitMeasurement( wxS( "MAX (V(out) >= 0.5)" ), func, signal, params );
    BOOST_CHECK_EQUAL( func, wxS( "MAX" ) );
    BOOST_CHECK_EQUAL( signal, wxS( "(V(out) >= 0.5)" ) );
    BOOST_CHECK( params.IsEmpty() );

    SplitMeasurement( wxS( "MAX (V(out) >= 0.5) from=1m to=2m" ), func, signal, params );
    BOOST_CHECK_EQUAL( signal, wxS( "(V(out) >= 0.5)" ) );
    BOOST_CHECK_EQUAL( params, wxS( "from=1m to=2m" ) );

    SplitMeasurement( wxS( "AVG V(out) == 1" ), func, signal, params );
    BOOST_CHECK_EQUAL( signal, wxS( "V(out) == 1" ) );
    BOOST_CHECK( params.IsEmpty() );

    SplitMeasurement( wxS( "AVG V(out) + k == 2" ), func, signal, params );
    BOOST_CHECK_EQUAL( signal, wxS( "V(out) + k == 2" ) );
    BOOST_CHECK( params.IsEmpty() );

    SplitMeasurement( wxS( "MIN V(\"a b=c\") from = 1m" ), func, signal, params );
    BOOST_CHECK_EQUAL( signal, wxS( "V(\"a b=c\")" ) );
    BOOST_CHECK_EQUAL( params, wxS( "from = 1m" ) );
}


BOOST_AUTO_TEST_CASE( MeasurableSignalMatchesPlot )
{
    wxString result;

    // AC gain is plotted in dB
    BOOST_CHECK( MeasurableSignal( wxS( "V(out)" ), PART::GAIN, result ) );
    BOOST_CHECK_EQUAL( result, wxS( "vdb(out)" ) );

    BOOST_CHECK( MeasurableSignal( wxS( "V(a)-V(b)" ), PART::GAIN, result ) );
    BOOST_CHECK_EQUAL( result, wxS( "db(V(a)-V(b))" ) );

    // S-parameter amplitude is plotted linear, and a bare complex vector measures its real part
    BOOST_CHECK( MeasurableSignal( wxS( "S_1_1" ), PART::AMPLITUDE, result ) );
    BOOST_CHECK_EQUAL( result, wxS( "mag(S_1_1)" ) );

    BOOST_CHECK( MeasurableSignal( wxS( "V(out)" ), PART::AMPLITUDE, result ) );
    BOOST_CHECK_EQUAL( result, wxS( "mag(V(out))" ) );

    BOOST_CHECK( !MeasurableSignal( wxS( "V(out)" ), PART::PHASE, result ) );
}


BOOST_AUTO_TEST_CASE( ExpandDeviceVoltagesSinglePass )
{
    std::map<wxString, wxString> devices = { { wxS( "C1" ), wxS( "V(r2) - V(n1)" ) },
                                             { wxS( "R2" ), wxS( "V(x)" ) } };

    // A net spelled like another part inside an expansion is not expanded again
    BOOST_CHECK_EQUAL( ExpandDeviceVoltages( wxS( "2*V(C1)" ), devices ), wxS( "2*(V(r2) - V(n1))" ) );

    BOOST_CHECK_EQUAL( ExpandDeviceVoltages( wxS( "vdb(c1)" ), devices ), wxS( "db(V(r2) - V(n1))" ) );
    BOOST_CHECK_EQUAL( ExpandDeviceVoltages( wxS( "IV(R2)+V(R2)" ), devices ), wxS( "IV(R2)+(V(x))" ) );
    BOOST_CHECK_EQUAL( ExpandDeviceVoltages( wxS( "V(\"R2\")" ), devices ), wxS( "V(\"R2\")" ) );
    BOOST_CHECK_EQUAL( ExpandDeviceVoltages( wxS( "V(out)" ), devices ), wxS( "V(out)" ) );
}


BOOST_AUTO_TEST_CASE( QuoteNetNamesIgnoresCase )
{
    std::vector<wxString> nets = { wxS( "OUT-1" ), wxS( "IN" ) };

    BOOST_CHECK_EQUAL( QuoteNetNames( wxS( "V(out-1)" ), nets ), wxS( "V(\"out-1\")" ) );
    BOOST_CHECK_EQUAL( QuoteNetNames( wxS( "sin(V(in))" ), nets ), wxS( "sin(V(\"in\"))" ) );
    BOOST_CHECK_EQUAL( QuoteNetNames( wxS( "V(\"OUT-1\")" ), nets ), wxS( "V(\"OUT-1\")" ) );
}


BOOST_AUTO_TEST_CASE( ExtractSignalRefsOnlyNamesSignals )
{
    std::vector<wxString> refs = ExtractSignalRefs( wxS( "im(V(out))" ) );

    BOOST_REQUIRE_EQUAL( refs.size(), 1 );
    BOOST_CHECK_EQUAL( refs[0], wxS( "V(out)" ) );

    refs = ExtractSignalRefs( wxS( "vdb(a, 0) * I(V1) / P(\"R,1\")" ) );

    BOOST_REQUIRE_EQUAL( refs.size(), 3 );
    BOOST_CHECK_EQUAL( refs[0], wxS( "V(a)" ) );
    BOOST_CHECK_EQUAL( refs[1], wxS( "I(V1)" ) );
    BOOST_CHECK_EQUAL( refs[2], wxS( "P(R,1)" ) );

    BOOST_CHECK( ExtractSignalRefs( wxS( "sin(2*pi*1k*time)" ) ).empty() );
    BOOST_CHECK( ParensBalanced( wxS( "V(\"a)\")" ) ) );
    BOOST_CHECK( !ParensBalanced( wxS( "V(a))(" ) ) );
}


BOOST_AUTO_TEST_CASE( DeviceVoltageFollowsModelPinOrder )
{
    // Device:D numbers the cathode 1, while the diode model declares the anode first
    std::vector<std::string> modelPins = { "2", "1" };
    std::vector<std::string> pinNumbers = { "1", "2" };

    BOOST_CHECK_EQUAL( DeviceVoltageExpression( modelPins, pinNumbers, { "k", "a" } ), wxS( "V(a) - V(k)" ) );
    BOOST_CHECK_EQUAL( DeviceVoltageExpression( modelPins, pinNumbers, { "0", "a" } ), wxS( "V(a)" ) );
    BOOST_CHECK_EQUAL( DeviceVoltageExpression( modelPins, pinNumbers, { "k", "GND" } ), wxS( "-V(k)" ) );
    BOOST_CHECK( DeviceVoltageExpression( modelPins, pinNumbers, { "a", "a" } ).IsEmpty() );
    BOOST_CHECK( DeviceVoltageExpression( modelPins, { "1" }, { "k" } ).IsEmpty() );
}


BOOST_AUTO_TEST_CASE( DerivedVectorsAreExactNames )
{
    BOOST_CHECK( IsDerivedSignalVector( wxS( "user3" ) ) );
    BOOST_CHECK( IsDerivedSignalVector( wxS( "KICAD_VDEV12" ) ) );
    BOOST_CHECK( !IsDerivedSignalVector( wxS( "user_clk" ) ) );
    BOOST_CHECK( !IsDerivedSignalVector( wxS( "kicad_vdev" ) ) );
}


BOOST_AUTO_TEST_SUITE_END()
