/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2022 Mikolaj Wielgus
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

#include <sim/sim_model_ngspice.h>


void NGSPICE_MODEL_INFO_MAP::addJFET()
{
    modelInfos[MODEL_TYPE::JFET] = { "JFET", "NJF", "PJF", { "D", "G", "S" }, "Junction Field effect transistor", {}, {} };
    // Model parameters
    modelInfos[MODEL_TYPE::JFET].modelParams.emplace_back( "type",  305, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_STRING, "", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "njf", "pjf", "N-type or P-type JFET model" );
    modelInfos[MODEL_TYPE::JFET].modelParams.emplace_back( "njf",  111, SIM_MODEL::PARAM::DIR_IN, SIM_VALUE::TYPE_BOOL, "", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "N type JFET model" );
    modelInfos[MODEL_TYPE::JFET].modelParams.emplace_back( "pjf",  112, SIM_MODEL::PARAM::DIR_IN, SIM_VALUE::TYPE_BOOL, "", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "P type JFET model" );
    modelInfos[MODEL_TYPE::JFET].modelParams.emplace_back( "vt0",  101, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "V", SIM_MODEL::PARAM::CATEGORY::DC, "-2", "-2", "Threshold voltage" );
    modelInfos[MODEL_TYPE::JFET].modelParams.emplace_back( "vto",  101, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "-2", "-2", "n.a." );
    modelInfos[MODEL_TYPE::JFET].modelParams.emplace_back( "beta",  102, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "A/V²", SIM_MODEL::PARAM::CATEGORY::DC, "0.0001", "0.0001", "Transconductance" );
    modelInfos[MODEL_TYPE::JFET].modelParams.emplace_back( "lambda",  103, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "1/V", SIM_MODEL::PARAM::CATEGORY::DC, "0", "0", "Channel length modulation coefficient" );
    modelInfos[MODEL_TYPE::JFET].modelParams.emplace_back( "rd",  104, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "Ω", SIM_MODEL::PARAM::CATEGORY::DC, "0", "0", "Drain ohmic resistance" );
    modelInfos[MODEL_TYPE::JFET].modelParams.emplace_back( "gd",  301, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::DC, "0", "0", "Drain conductance" );
    modelInfos[MODEL_TYPE::JFET].modelParams.emplace_back( "rs",  105, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "Ω", SIM_MODEL::PARAM::CATEGORY::DC, "0", "0", "Source ohmic resistance" );
    modelInfos[MODEL_TYPE::JFET].modelParams.emplace_back( "gs",  302, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::DC, "0", "0", "Source conductance" );
    modelInfos[MODEL_TYPE::JFET].modelParams.emplace_back( "cgs",  106, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::DC, "0", "0", "G-S junction capactance" );
    modelInfos[MODEL_TYPE::JFET].modelParams.emplace_back( "cgd",  107, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::DC, "0", "0", "G-D junction cap" );
    modelInfos[MODEL_TYPE::JFET].modelParams.emplace_back( "pb",  108, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "V", SIM_MODEL::PARAM::CATEGORY::DC, "1", "1", "Gate junction potential" );
    modelInfos[MODEL_TYPE::JFET].modelParams.emplace_back( "is_",  109, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "A", SIM_MODEL::PARAM::CATEGORY::DC, "1e-14", "1e-14", "Gate junction saturation current" );
    modelInfos[MODEL_TYPE::JFET].modelParams.emplace_back( "fc",  110, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::DC, "0.5", "0.5", "Forward bias junction fit parameter" );
    modelInfos[MODEL_TYPE::JFET].modelParams.emplace_back( "b",  114, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::DC, "1", "1", "Doping tail parameter" );
    modelInfos[MODEL_TYPE::JFET].modelParams.emplace_back( "tnom",  113, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "°C", SIM_MODEL::PARAM::CATEGORY::TEMPERATURE, "27", "27", "Measurement temperature" );
    modelInfos[MODEL_TYPE::JFET].modelParams.emplace_back( "tcv",  115, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "V°C", SIM_MODEL::PARAM::CATEGORY::TEMPERATURE, "0", "0", "Threshold voltage temperature coefficient" );
    modelInfos[MODEL_TYPE::JFET].modelParams.emplace_back( "vtotc",  116, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "V/°C", SIM_MODEL::PARAM::CATEGORY::TEMPERATURE, "0", "0", "Threshold voltage temperature coefficient alternate" );
    modelInfos[MODEL_TYPE::JFET].modelParams.emplace_back( "bex",  117, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::TEMPERATURE, "0", "0", "Mobility temperature exponent" );
    modelInfos[MODEL_TYPE::JFET].modelParams.emplace_back( "betatce",  118, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "%/°C", SIM_MODEL::PARAM::CATEGORY::TEMPERATURE, "0", "0", "Mobility temperature exponent alternate" );
    modelInfos[MODEL_TYPE::JFET].modelParams.emplace_back( "xti",  119, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::TEMPERATURE, "3", "3", "Gate junction saturation current temperature exponent" );
    modelInfos[MODEL_TYPE::JFET].modelParams.emplace_back( "eg",  120, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "V", SIM_MODEL::PARAM::CATEGORY::DC, "1.11", "1.11", "Bandgap voltage" );
    modelInfos[MODEL_TYPE::JFET].modelParams.emplace_back( "kf",  121, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::NOISE, "0", "0", "Flicker Noise Coefficient" );
    modelInfos[MODEL_TYPE::JFET].modelParams.emplace_back( "af",  122, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::NOISE, "1", "1", "Flicker Noise Exponent" );
    modelInfos[MODEL_TYPE::JFET].modelParams.emplace_back( "nlev",  123, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_INT, "", SIM_MODEL::PARAM::CATEGORY::NOISE, "2", "2", "Noise equation selector" );
    modelInfos[MODEL_TYPE::JFET].modelParams.emplace_back( "gdsnoi",  124, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::NOISE, "1", "1", "Channel noise coefficient" );
    modelInfos[MODEL_TYPE::JFET].modelParams.emplace_back( "alpha", 401, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "1/V", SIM_MODEL::PARAM::CATEGORY::DC, "0", "0", "Ionization coefficient" );
    modelInfos[MODEL_TYPE::JFET].modelParams.emplace_back( "m_", 402, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::DC, "0.5", "0.5", "Gate p-n grading coefficient" );
    modelInfos[MODEL_TYPE::JFET].modelParams.emplace_back( "n", 403, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::DC, "1", "1", "Gate p-n emission coefficient" );
    modelInfos[MODEL_TYPE::JFET].modelParams.emplace_back( "isr", 404, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "A", SIM_MODEL::PARAM::CATEGORY::DC, "0", "0", "Gate p-n recombination current" );
    modelInfos[MODEL_TYPE::JFET].modelParams.emplace_back( "nr", 405, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::DC, "2", "2", "Gate p-n recombination current emission coefficient" );
    modelInfos[MODEL_TYPE::JFET].modelParams.emplace_back( "vk", 406, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "V", SIM_MODEL::PARAM::CATEGORY::DC, "0", "0", "Ionization knee voltage" );

    // Instance parameters
    modelInfos[MODEL_TYPE::JFET].instanceParams.emplace_back( "off",  5, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_BOOL, "", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "Device initially off", true );
    modelInfos[MODEL_TYPE::JFET].instanceParams.emplace_back( "ic",  4, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT_VECTOR, "", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "Initial VDS,VGS vector", true );
    modelInfos[MODEL_TYPE::JFET].instanceParams.emplace_back( "m",  8, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::GEOMETRY, "", "", "Parallel multiplier", true );
    modelInfos[MODEL_TYPE::JFET].instanceParams.emplace_back( "area",  1, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::GEOMETRY, "", "", "Area factor", true );
    modelInfos[MODEL_TYPE::JFET].instanceParams.emplace_back( "ic-vds",  2, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "V", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "Initial D-S voltage", true );
    modelInfos[MODEL_TYPE::JFET].instanceParams.emplace_back( "ic-vgs",  3, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "Initial G-S volrage", true );
    modelInfos[MODEL_TYPE::JFET].instanceParams.emplace_back( "temp",  6, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "°C", SIM_MODEL::PARAM::CATEGORY::PRINCIPAL, "", "", "Instance temperature", true );
    modelInfos[MODEL_TYPE::JFET].instanceParams.emplace_back( "dtemp",  7, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "°C", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "Instance temperature difference", true );
    modelInfos[MODEL_TYPE::JFET].instanceParams.emplace_back( "drain-node",  301, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_INT, "", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "Number of drain node", true );
    modelInfos[MODEL_TYPE::JFET].instanceParams.emplace_back( "gate-node",  302, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_INT, "", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "Number of gate node", true );
    modelInfos[MODEL_TYPE::JFET].instanceParams.emplace_back( "source-node",  303, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_INT, "", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "Number of source node", true );
    modelInfos[MODEL_TYPE::JFET].instanceParams.emplace_back( "drain-prime-node",  304, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_INT, "", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "Internal drain node", true );
    modelInfos[MODEL_TYPE::JFET].instanceParams.emplace_back( "source-prime-node",  305, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_INT, "", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "Internal source node", true );
    modelInfos[MODEL_TYPE::JFET].instanceParams.emplace_back( "vgs",  306, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_FLOAT, "V", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "Voltage G-S", true );
    modelInfos[MODEL_TYPE::JFET].instanceParams.emplace_back( "vgd",  307, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_FLOAT, "V", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "Voltage G-D", true );
    modelInfos[MODEL_TYPE::JFET].instanceParams.emplace_back( "ig",  308, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_FLOAT, "A", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "Current at gate node", true );
    modelInfos[MODEL_TYPE::JFET].instanceParams.emplace_back( "id",  309, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_FLOAT, "A", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "Current at drain node", true );
    modelInfos[MODEL_TYPE::JFET].instanceParams.emplace_back( "is",  319, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_FLOAT, "A", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "1e-14", "1e-14", "Source current", true );
    modelInfos[MODEL_TYPE::JFET].instanceParams.emplace_back( "igd",  310, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_FLOAT, "A", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "Current G-D", true );
    modelInfos[MODEL_TYPE::JFET].instanceParams.emplace_back( "gm",  311, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "Transconductance", true );
    modelInfos[MODEL_TYPE::JFET].instanceParams.emplace_back( "gds",  312, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "Conductance D-S", true );
    modelInfos[MODEL_TYPE::JFET].instanceParams.emplace_back( "ggs",  313, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "Conductance G-S", true );
    modelInfos[MODEL_TYPE::JFET].instanceParams.emplace_back( "ggd",  314, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "Conductance G-D", true );
    modelInfos[MODEL_TYPE::JFET].instanceParams.emplace_back( "qgs",  315, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "Charge storage G-S junction", true );
    modelInfos[MODEL_TYPE::JFET].instanceParams.emplace_back( "qgd",  317, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "Charge storage G-D junction", true );
    modelInfos[MODEL_TYPE::JFET].instanceParams.emplace_back( "cqgs",  316, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_FLOAT, "F", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "Capacitance due to charge storage G-S junction", true );
    modelInfos[MODEL_TYPE::JFET].instanceParams.emplace_back( "cqgd",  318, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_FLOAT, "F", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "Capacitance due to charge storage G-D junction", true );
    modelInfos[MODEL_TYPE::JFET].instanceParams.emplace_back( "p",  320, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "Power dissipated by the JFET", true );


    modelInfos[MODEL_TYPE::JFET2] = { "JFET2", "NJF", "PJF", { "D", "G", "S" }, "Short channel field effect transistor", {}, {} };
    // Model parameters
    modelInfos[MODEL_TYPE::JFET2].modelParams.emplace_back( "type", 305, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_STRING, "", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "njf", "pjf", "N-type or P-type JFET2 model" );
    modelInfos[MODEL_TYPE::JFET2].modelParams.emplace_back( "njf", 102, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_BOOL, "", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "NaN", "NaN", "N type JFET2 model" );
    modelInfos[MODEL_TYPE::JFET2].modelParams.emplace_back( "pjf", 103, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_BOOL, "", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "NaN", "NaN", "P type JFET2 model" );
    modelInfos[MODEL_TYPE::JFET2].modelParams.emplace_back( "acgam", 107, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::DC, "0", "0", "" );
    modelInfos[MODEL_TYPE::JFET2].modelParams.emplace_back( "af", 108, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::NOISE, "1", "1", "Flicker Noise Exponent" );
    modelInfos[MODEL_TYPE::JFET2].modelParams.emplace_back( "beta", 109, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "A/V^2", SIM_MODEL::PARAM::CATEGORY::DC, "0.0001", "0.0001", "Transconductance parameter" );
    modelInfos[MODEL_TYPE::JFET2].modelParams.emplace_back( "cds", 146, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "F", SIM_MODEL::PARAM::CATEGORY::CAPACITANCE, "0", "0", "D-S junction capacitance" );
    modelInfos[MODEL_TYPE::JFET2].modelParams.emplace_back( "cgd", 110, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "F", SIM_MODEL::PARAM::CATEGORY::CAPACITANCE, "0", "0", "G-D junction capacitance" );
    modelInfos[MODEL_TYPE::JFET2].modelParams.emplace_back( "cgs", 111, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "F", SIM_MODEL::PARAM::CATEGORY::CAPACITANCE, "0", "0", "G-S junction capacitance" );
    modelInfos[MODEL_TYPE::JFET2].modelParams.emplace_back( "delta", 113, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "1/W", SIM_MODEL::PARAM::CATEGORY::DC, "0", "0", "coef of thermal current reduction" );
    modelInfos[MODEL_TYPE::JFET2].modelParams.emplace_back( "hfeta", 114, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::DC, "0", "0", "drain feedback modulation" );
    modelInfos[MODEL_TYPE::JFET2].modelParams.emplace_back( "hfe1", 115, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::DC, "0", "0", "" );
    modelInfos[MODEL_TYPE::JFET2].modelParams.emplace_back( "hfe2", 116, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::DC, "0", "0", "" );
    modelInfos[MODEL_TYPE::JFET2].modelParams.emplace_back( "hfg1", 117, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::DC, "0", "0", "" );
    modelInfos[MODEL_TYPE::JFET2].modelParams.emplace_back( "hfg2", 118, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::DC, "0", "0", "" );
    modelInfos[MODEL_TYPE::JFET2].modelParams.emplace_back( "mvst", 119, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "A", SIM_MODEL::PARAM::CATEGORY::DC, "0", "0", "modulation index for subtreshold current" );
    modelInfos[MODEL_TYPE::JFET2].modelParams.emplace_back( "mxi", 120, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "V", SIM_MODEL::PARAM::CATEGORY::DC, "0", "0", "Saturation potential modulation" );
    modelInfos[MODEL_TYPE::JFET2].modelParams.emplace_back( "fc", 121, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::DC, "0.5", "0.5", "Forward bias junction fit" );
    modelInfos[MODEL_TYPE::JFET2].modelParams.emplace_back( "ibd", 122, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "A", SIM_MODEL::PARAM::CATEGORY::DC, "0", "0", "Breakdown current of diode junction" );
    modelInfos[MODEL_TYPE::JFET2].modelParams.emplace_back( "is_", 123, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "A", SIM_MODEL::PARAM::CATEGORY::DC, "1e-14", "1e-14", "Gate junction saturation current" );
    modelInfos[MODEL_TYPE::JFET2].modelParams.emplace_back( "kf", 124, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::NOISE, "0", "0", "Flicker noise coefficient" );
    modelInfos[MODEL_TYPE::JFET2].modelParams.emplace_back( "lambda", 125, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "1/V", SIM_MODEL::PARAM::CATEGORY::DC, "0", "0", "Channel length modulation" );
    modelInfos[MODEL_TYPE::JFET2].modelParams.emplace_back( "lfgam", 126, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::DC, "0", "0", "Drain feedback coefficient" );
    modelInfos[MODEL_TYPE::JFET2].modelParams.emplace_back( "lfg1", 127, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::DC, "0", "0", "" );
    modelInfos[MODEL_TYPE::JFET2].modelParams.emplace_back( "lfg2", 128, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::DC, "0", "0", "" );
    modelInfos[MODEL_TYPE::JFET2].modelParams.emplace_back( "n", 129, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::DC, "1", "1", "Gate junction ideality factor" );
    modelInfos[MODEL_TYPE::JFET2].modelParams.emplace_back( "p_", 130, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::DC, "2", "2", "Power law (triode region)" );
    modelInfos[MODEL_TYPE::JFET2].modelParams.emplace_back( "vbi", 131, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "V", SIM_MODEL::PARAM::CATEGORY::DC, "1", "1", "Gate junction potential" );
    modelInfos[MODEL_TYPE::JFET2].modelParams.emplace_back( "pb", 131, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "1", "1", "n.a." );
    modelInfos[MODEL_TYPE::JFET2].modelParams.emplace_back( "q", 132, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::DC, "2", "2", "Power Law (saturated region)" );
    modelInfos[MODEL_TYPE::JFET2].modelParams.emplace_back( "rd", 133, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "ohm", SIM_MODEL::PARAM::CATEGORY::DC, "0", "0", "Drain ohmic resistance" );
    modelInfos[MODEL_TYPE::JFET2].modelParams.emplace_back( "rs", 134, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "ohm", SIM_MODEL::PARAM::CATEGORY::DC, "0", "0", "Source ohmic resistance" );
    modelInfos[MODEL_TYPE::JFET2].modelParams.emplace_back( "taud", 135, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "s", SIM_MODEL::PARAM::CATEGORY::DC, "0", "0", "Thermal relaxation time" );
    modelInfos[MODEL_TYPE::JFET2].modelParams.emplace_back( "taug", 136, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "s", SIM_MODEL::PARAM::CATEGORY::DC, "0", "0", "Drain feedback relaxation time" );
    modelInfos[MODEL_TYPE::JFET2].modelParams.emplace_back( "vbd", 137, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "V", SIM_MODEL::PARAM::CATEGORY::DC, "1", "1", "Breakdown potential of diode jnc" );
    modelInfos[MODEL_TYPE::JFET2].modelParams.emplace_back( "ver", 139, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::DC, "0", "0", "version number of PS model" );
    modelInfos[MODEL_TYPE::JFET2].modelParams.emplace_back( "vst", 140, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::DC, "0", "0", "Crit Poten subthreshold conduction" );
    modelInfos[MODEL_TYPE::JFET2].modelParams.emplace_back( "vt0", 141, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "V", SIM_MODEL::PARAM::CATEGORY::DC, "-2", "-2", "Threshold voltage" );
    modelInfos[MODEL_TYPE::JFET2].modelParams.emplace_back( "vto", 141, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "-2", "-2", "n.a." );
    modelInfos[MODEL_TYPE::JFET2].modelParams.emplace_back( "xc", 142, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "F", SIM_MODEL::PARAM::CATEGORY::DC, "0", "0", "amount of cap. red at pinch-off" );
    modelInfos[MODEL_TYPE::JFET2].modelParams.emplace_back( "xi", 143, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "m/s", SIM_MODEL::PARAM::CATEGORY::DC, "1000", "1000", "Velocity saturation index" );
    modelInfos[MODEL_TYPE::JFET2].modelParams.emplace_back( "z", 144, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "m/s", SIM_MODEL::PARAM::CATEGORY::DC, "1", "1", "Rate of velocity saturation" );
    modelInfos[MODEL_TYPE::JFET2].modelParams.emplace_back( "hfgam", 145, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::DC, "0", "0", "High freq drain feedback" );
    modelInfos[MODEL_TYPE::JFET2].modelParams.emplace_back( "gd", 301, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::DC, "0", "0", "Drain conductance" );
    modelInfos[MODEL_TYPE::JFET2].modelParams.emplace_back( "gs", 302, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::DC, "0", "0", "Source conductance" );
    modelInfos[MODEL_TYPE::JFET2].modelParams.emplace_back( "tnom", 104, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "°C", SIM_MODEL::PARAM::CATEGORY::TEMPERATURE, "27", "27", "Measurement temperature" );
    // Instance parameters
    modelInfos[MODEL_TYPE::JFET2].instanceParams.emplace_back( "off", 5, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_BOOL, "", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "Device initially off", true );
    modelInfos[MODEL_TYPE::JFET2].instanceParams.emplace_back( "ic", 4, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT_VECTOR, "", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "Initial VDS,VGS vector", true );
    modelInfos[MODEL_TYPE::JFET2].instanceParams.emplace_back( "m", 8, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::GEOMETRY, "", "", "Parallel Multiplier", true );
    modelInfos[MODEL_TYPE::JFET2].instanceParams.emplace_back( "area", 1, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::GEOMETRY, "", "", "Area factor", true );
    modelInfos[MODEL_TYPE::JFET2].instanceParams.emplace_back( "ic-vds", 2, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "V", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "Initial D-S voltage", true );
    modelInfos[MODEL_TYPE::JFET2].instanceParams.emplace_back( "ic-vgs", 3, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "Initial G-S volrage", true );
    modelInfos[MODEL_TYPE::JFET2].instanceParams.emplace_back( "temp", 6, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "°C", SIM_MODEL::PARAM::CATEGORY::PRINCIPAL, "", "", "Instance temperature", true );
    modelInfos[MODEL_TYPE::JFET2].instanceParams.emplace_back( "dtemp", 7, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "°C", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "Instance temperature difference", true );
    modelInfos[MODEL_TYPE::JFET2].instanceParams.emplace_back( "drain-node", 301, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_INT, "", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "Number of drain node", true );
    modelInfos[MODEL_TYPE::JFET2].instanceParams.emplace_back( "gate-node", 302, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_INT, "", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "Number of gate node", true );
    modelInfos[MODEL_TYPE::JFET2].instanceParams.emplace_back( "source-node", 303, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_INT, "", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "Number of source node", true );
    modelInfos[MODEL_TYPE::JFET2].instanceParams.emplace_back( "drain-prime-node", 304, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_INT, "", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "Internal drain node", true );
    modelInfos[MODEL_TYPE::JFET2].instanceParams.emplace_back( "source-prime-node", 305, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_INT, "", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "Internal source node", true );
    modelInfos[MODEL_TYPE::JFET2].instanceParams.emplace_back( "vgs", 306, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_FLOAT, "V", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "Voltage G-S", true );
    modelInfos[MODEL_TYPE::JFET2].instanceParams.emplace_back( "vgd", 307, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_FLOAT, "V", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "Voltage G-D", true );
    modelInfos[MODEL_TYPE::JFET2].instanceParams.emplace_back( "ig", 308, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_FLOAT, "A", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "Current at gate node", true );
    modelInfos[MODEL_TYPE::JFET2].instanceParams.emplace_back( "id", 309, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_FLOAT, "A", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "Current at drain node", true );
    modelInfos[MODEL_TYPE::JFET2].instanceParams.emplace_back( "is", 319, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_FLOAT, "A", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "1e-14", "1e-14", "Source current", true );
    modelInfos[MODEL_TYPE::JFET2].instanceParams.emplace_back( "igd", 310, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_FLOAT, "A", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "Current G-D", true );
    modelInfos[MODEL_TYPE::JFET2].instanceParams.emplace_back( "gm", 311, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "Transconductance", true );
    modelInfos[MODEL_TYPE::JFET2].instanceParams.emplace_back( "gds", 312, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "Conductance D-S", true );
    modelInfos[MODEL_TYPE::JFET2].instanceParams.emplace_back( "ggs", 313, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "Conductance G-S", true );
    modelInfos[MODEL_TYPE::JFET2].instanceParams.emplace_back( "ggd", 314, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "Conductance G-D", true );
    modelInfos[MODEL_TYPE::JFET2].instanceParams.emplace_back( "qgs", 315, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "Charge storage G-S junction", true );
    modelInfos[MODEL_TYPE::JFET2].instanceParams.emplace_back( "qgd", 317, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "Charge storage G-D junction", true );
    modelInfos[MODEL_TYPE::JFET2].instanceParams.emplace_back( "cqgs", 316, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_FLOAT, "F", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "Capacitance due to charge storage G-S junction", true );
    modelInfos[MODEL_TYPE::JFET2].instanceParams.emplace_back( "cqgd", 318, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_FLOAT, "F", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "Capacitance due to charge storage G-D junction", true );
    modelInfos[MODEL_TYPE::JFET2].instanceParams.emplace_back( "p", 320, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "2", "2", "Power dissipated by the JFET2", true );
    modelInfos[MODEL_TYPE::JFET2].instanceParams.emplace_back( "vtrap", 321, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_FLOAT, "V", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "Quiescent drain feedback potential", true );
    modelInfos[MODEL_TYPE::JFET2].instanceParams.emplace_back( "vpave", 322, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "Quiescent power dissipation", true );
}