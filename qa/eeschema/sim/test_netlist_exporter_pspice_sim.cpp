/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2020 S.Kocjan <s.kocjan@o2.pl>
 * Copyright (C) 2020 KiCad Developers, see CHANGELOG.TXT for contributors.
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
 * Test suite for NETLIST_EXPORTER_PSPICE_SIM
 */

#include <string.h>
#include <unit_test_utils/unit_test_utils.h>
#include <vector>
#include <wx/string.h>

// Code under test
#include <sim/netlist_exporter_pspice_sim.h>

class TEST_NETLIST_EXPORTER_PSPICE_SIM
{
public:
    TEST_NETLIST_EXPORTER_PSPICE_SIM()
            : m_netlist( new NETLIST_OBJECT_LIST ), m_exporter( m_netlist )
    {
    }

    NETLIST_OBJECT_LIST*        m_netlist;
    NETLIST_EXPORTER_PSPICE_SIM m_exporter;
};


/**
 * Declare the test suite
 */
BOOST_FIXTURE_TEST_SUITE( NetlistExporterPspiceSim, TEST_NETLIST_EXPORTER_PSPICE_SIM )


/**
 * Check if simulation command is recognised properly
 */
BOOST_AUTO_TEST_CASE( CommandToSimType )
{
    struct TEST_DATA
    {
        wxString command;
        SIM_TYPE type;
    };

    std::vector<struct TEST_DATA> testData = {
        { ".op", ST_OP },
        { ".option TEMP=27", ST_UNKNOWN },
        { ".tran 0 1 0.1", ST_TRANSIENT },
        { ".tran 0 1 0.1 UIC", ST_TRANSIENT },
        { ".ac dec 10 1 10K", ST_AC },
        { ".ac dec 10 1K 100MEG", ST_AC },
        { ".ac lin 100 1 100HZ", ST_AC },
        { ".dc VIN 0.25 5.0 0.25", ST_DC },
        { ".dc VDS 0 10 .5 VGS 0 5 1", ST_DC },
        { ".dc VCE 0 10 .25 IB 0 10u 1u", ST_DC },
        { ".dc RLoad 1k 2k 100", ST_DC },
        { ".dc TEMP -15 75 5", ST_DC },
        { ".disto dec 10 1kHz 100MEG", ST_DISTORTION },
        { ".disto dec 10 1kHz 100MEG 0.9", ST_DISTORTION },
        { ".noise v(5) VIN dec 10 1kHz 100MEG", ST_NOISE },
        { ".noise v(5,3) V1 oct 8 1.0 1.0e6 1", ST_NOISE },
        { ".pz 1 0 3 0 cur pol", ST_POLE_ZERO },
        { ".pz 2 3 5 0 vol zer", ST_POLE_ZERO },
        { ".pz 4 1 4 1 cur pz", ST_POLE_ZERO },
        { ".SENS V(1,OUT)", ST_SENSITIVITY },
        { ".SENS V(OUT) AC DEC 10 100 100k", ST_SENSITIVITY },
        { ".SENS I(VTEST)", ST_SENSITIVITY },
        { ".tf v(5, 3) VIN", ST_TRANS_FUNC },
        { ".tf i(VLOAD) VIN", ST_TRANS_FUNC },
    };

    for( auto& step : testData )
    {
        BOOST_CHECK_EQUAL( m_exporter.CommandToSimType( wxString( step.command ) ), step.type );
    }

    for( auto& step : testData )
    {
        step.command.Append( "\n" );
        BOOST_CHECK_EQUAL( m_exporter.CommandToSimType( wxString( step.command ) ), step.type );
    }
}


/**
 * Check conversion from internal spice vector name to eeschema format
 */
BOOST_AUTO_TEST_CASE( VectorToSignal )
{
    struct TEST_DATA
    {
        std::string   vector;
        wxString      signal;
        SIM_PLOT_TYPE type;
    };

    std::vector<struct TEST_DATA> testData = { { "@c3[i]", "I(C3)", SPT_CURRENT },
        { "@r12[i]", "I(R12)", SPT_CURRENT }, { "@r7[i]", "I(R7)", SPT_CURRENT },
        { "@l2[i]", "I(L2)", SPT_CURRENT }, { "@c2[i]", "I(C2)", SPT_CURRENT },
        { "@r6[i]", "I(R6)", SPT_CURRENT }, { "@r5[i]", "I(R5)", SPT_CURRENT },
        { "@r10[i]", "I(R10)", SPT_CURRENT }, { "@q3[ie]", "Ie(Q3)", SPT_CURRENT },
        { "@q3[ic]", "Ic(Q3)", SPT_CURRENT }, { "@q3[ib]", "Ib(Q3)", SPT_CURRENT },
        { "@r11[i]", "I(R11)", SPT_CURRENT }, { "@r8[i]", "I(R8)", SPT_CURRENT },
        { "@q1[ie]", "Ie(Q1)", SPT_CURRENT }, { "@q1[ic]", "Ic(Q1)", SPT_CURRENT },
        { "@q1[ib]", "Ib(Q1)", SPT_CURRENT }, { "@r1[i]", "I(R1)", SPT_CURRENT },
        { "@l1[i]", "I(L1)", SPT_CURRENT }, { "@c4[i]", "I(C4)", SPT_CURRENT },
        { "@r2[i]", "I(R2)", SPT_CURRENT }, { "@q2[ig]", "Ig(Q2)", SPT_CURRENT },
        { "@q2[id]", "Id(Q2)", SPT_CURRENT }, { "@q2[is]", "Is(Q2)", SPT_CURRENT },
        { "@v2[i]", "I(V2)", SPT_CURRENT }, { "@r9[i]", "I(R9)", SPT_CURRENT },
        { "@c1[i]", "I(C1)", SPT_CURRENT }, { "@v1[i]", "I(V1)", SPT_CURRENT },
        { "@r3[i]", "I(R3)", SPT_CURRENT }, { "@r4[i]", "I(R4)", SPT_CURRENT },
        { "vout", "V(vout)", SPT_VOLTAGE }, { "net-_q3-pad2_", "V(net-_q3-pad2_)", SPT_VOLTAGE },
        { "net-_q2-pad3_", "V(net-_q2-pad3_)", SPT_VOLTAGE },
        { "net-_q2-pad1_", "V(net-_q2-pad1_)", SPT_VOLTAGE },
        { "net-_q1-pad3_", "V(net-_q1-pad3_)", SPT_VOLTAGE },
        { "net-_l2-pad1_", "V(net-_l2-pad1_)", SPT_VOLTAGE },
        { "net-_c4-pad2_", "V(net-_c4-pad2_)", SPT_VOLTAGE },
        { "net-_c3-pad1_", "V(net-_c3-pad1_)", SPT_VOLTAGE },
        { "net-_c1-pad2_", "V(net-_c1-pad2_)", SPT_VOLTAGE }, { "/vin", "V(/vin)", SPT_VOLTAGE },
        { "/vbase", "V(/vbase)", SPT_VOLTAGE }, { "+12v", "V(+12v)", SPT_VOLTAGE },
        { "@m1[cgs]", "", SPT_UNKNOWN }, { "@d1[g11]", "", SPT_UNKNOWN },
        { "@d1[c12]", "", SPT_UNKNOWN }, { "@d1[y21]", "", SPT_UNKNOWN },
        { "@n1[vth0]", "", SPT_UNKNOWN }, { "@mn1[gm]", "", SPT_UNKNOWN },
        { "@m.xmos1.xmos2.m1[vdsat]", "", SPT_UNKNOWN } };

    for( auto& step : testData )
    {
        wxString      outputSignalName;
        SIM_PLOT_TYPE retVal;

        retVal = m_exporter.VectorToSignal( step.vector, outputSignalName );

        BOOST_CHECK_EQUAL( retVal, step.type );
        BOOST_CHECK_EQUAL( outputSignalName.Cmp( step.signal ), 0 );
    }
}


BOOST_AUTO_TEST_SUITE_END()
