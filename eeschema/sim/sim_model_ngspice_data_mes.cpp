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


void NGSPICE_MODEL_INFO_MAP::addMES()
{
    modelInfos[MODEL_TYPE::MES] = { "MES", "NMF", "PMF", { "D", "G", "S" }, "GaAs MESFET model", {}, {} };
    // Model parameters
    modelInfos[MODEL_TYPE::MES].modelParams.emplace_back( "type",  305, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_BOOL, "", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "-693161728", "116101380", "N-type or P-type MESfet model" );
    modelInfos[MODEL_TYPE::MES].modelParams.emplace_back( "nmf",  113, SIM_MODEL::PARAM::DIR_IN, SIM_VALUE::TYPE_BOOL, "", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "N type MESfet model" );
    modelInfos[MODEL_TYPE::MES].modelParams.emplace_back( "pmf",  114, SIM_MODEL::PARAM::DIR_IN, SIM_VALUE::TYPE_BOOL, "", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "P type MESfet model" );
    modelInfos[MODEL_TYPE::MES].modelParams.emplace_back( "vt0",  101, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "V", SIM_MODEL::PARAM::CATEGORY::DC, "-2", "-2", "Pinch-off voltage" );
    modelInfos[MODEL_TYPE::MES].modelParams.emplace_back( "vto",  101, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "-2", "-2", "n.a." );
    modelInfos[MODEL_TYPE::MES].modelParams.emplace_back( "alpha",  102, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "V", SIM_MODEL::PARAM::CATEGORY::DC, "2", "2", "Saturation voltage parameter" );
    modelInfos[MODEL_TYPE::MES].modelParams.emplace_back( "beta",  103, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "A/V²", SIM_MODEL::PARAM::CATEGORY::DC, "0.0025", "0.0025", "Transconductance parameter" );
    modelInfos[MODEL_TYPE::MES].modelParams.emplace_back( "lambda",  104, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "1/V", SIM_MODEL::PARAM::CATEGORY::DC, "0", "0", "Channel length modulation parm." );
    modelInfos[MODEL_TYPE::MES].modelParams.emplace_back( "b",  105, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::DC, "0.3", "0.3", "Doping tail extending parameter" );
    modelInfos[MODEL_TYPE::MES].modelParams.emplace_back( "rd",  106, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "Ω", SIM_MODEL::PARAM::CATEGORY::DC, "0", "0", "Drain ohmic resistance" );
    modelInfos[MODEL_TYPE::MES].modelParams.emplace_back( "gd",  301, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::DC, "0", "0", "Drain conductance" );
    modelInfos[MODEL_TYPE::MES].modelParams.emplace_back( "rs",  107, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "Ω", SIM_MODEL::PARAM::CATEGORY::DC, "0", "0", "Source ohmic resistance" );
    modelInfos[MODEL_TYPE::MES].modelParams.emplace_back( "gs",  302, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::DC, "0", "0", "Source conductance" );
    modelInfos[MODEL_TYPE::MES].modelParams.emplace_back( "cgs",  108, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "F", SIM_MODEL::PARAM::CATEGORY::CAPACITANCE, "0", "0", "G-S junction capacitance" );
    modelInfos[MODEL_TYPE::MES].modelParams.emplace_back( "cgd_",  109, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "F", SIM_MODEL::PARAM::CATEGORY::CAPACITANCE, "0", "0", "G-D junction capacitance" );
    modelInfos[MODEL_TYPE::MES].modelParams.emplace_back( "pb",  110, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "V", SIM_MODEL::PARAM::CATEGORY::DC, "1", "1", "Gate junction potential" );
    modelInfos[MODEL_TYPE::MES].modelParams.emplace_back( "is_",  111, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "A", SIM_MODEL::PARAM::CATEGORY::DC, "1e-14", "1e-14", "Junction saturation current" );
    modelInfos[MODEL_TYPE::MES].modelParams.emplace_back( "fc",  112, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::DC, "0.5", "0.5", "Forward bias junction fit parm." );
    modelInfos[MODEL_TYPE::MES].modelParams.emplace_back( "depl_cap",  303, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_FLOAT, "F", SIM_MODEL::PARAM::CATEGORY::CAPACITANCE, "0.5", "0.5", "Depletion capacitance" );
    modelInfos[MODEL_TYPE::MES].modelParams.emplace_back( "vcrit",  304, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_FLOAT, "V", SIM_MODEL::PARAM::CATEGORY::DC, "0.730289", "0.730289", "Critical voltage" );
    modelInfos[MODEL_TYPE::MES].modelParams.emplace_back( "kf",  115, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::NOISE, "NaN", "NaN", "Flicker noise coefficient" );
    modelInfos[MODEL_TYPE::MES].modelParams.emplace_back( "af",  116, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::NOISE, "NaN", "NaN", "Flicker noise exponent" );
    // Instance parameters
    modelInfos[MODEL_TYPE::MES].instanceParams.emplace_back( "off",  5, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_BOOL, "", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "Device initially off", true );
    modelInfos[MODEL_TYPE::MES].instanceParams.emplace_back( "m",  1, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::GEOMETRY, "", "", "Parallel Multiplier", true );
    modelInfos[MODEL_TYPE::MES].instanceParams.emplace_back( "area",  1, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::GEOMETRY, "", "", "Area factor", true );
    modelInfos[MODEL_TYPE::MES].instanceParams.emplace_back( "icvds",  2, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "V", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "Initial D-S voltage", true );
    modelInfos[MODEL_TYPE::MES].instanceParams.emplace_back( "icvgs",  3, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "V", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "Initial G-S voltage", true );
    modelInfos[MODEL_TYPE::MES].instanceParams.emplace_back( "dnode",  201, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_INT, "", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "Number of drain node", true );
    modelInfos[MODEL_TYPE::MES].instanceParams.emplace_back( "gnode",  202, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_INT, "", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "Number of gate node", true );
    modelInfos[MODEL_TYPE::MES].instanceParams.emplace_back( "snode",  203, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_INT, "", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "Number of source node", true );
    modelInfos[MODEL_TYPE::MES].instanceParams.emplace_back( "dprimenode",  204, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_INT, "", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "Number of internal drain node", true );
    modelInfos[MODEL_TYPE::MES].instanceParams.emplace_back( "sprimenode",  205, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_INT, "", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "Number of internal source node", true );
    modelInfos[MODEL_TYPE::MES].instanceParams.emplace_back( "vgs",  206, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_FLOAT, "V", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "Gate-Source voltage", true );
    modelInfos[MODEL_TYPE::MES].instanceParams.emplace_back( "vgd",  207, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_FLOAT, "V", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "Gate-Drain voltage", true );
    modelInfos[MODEL_TYPE::MES].instanceParams.emplace_back( "cg",  208, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_FLOAT, "F", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "Gate capacitance", true );
    modelInfos[MODEL_TYPE::MES].instanceParams.emplace_back( "cd",  209, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_FLOAT, "F", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "Drain capacitance", true );
    modelInfos[MODEL_TYPE::MES].instanceParams.emplace_back( "cgd",  210, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_FLOAT, "F", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "0", "0", "Gate-Drain capacitance", true );
    modelInfos[MODEL_TYPE::MES].instanceParams.emplace_back( "gm",  211, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "Transconductance", true );
    modelInfos[MODEL_TYPE::MES].instanceParams.emplace_back( "gds",  212, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "Drain-Source conductance", true );
    modelInfos[MODEL_TYPE::MES].instanceParams.emplace_back( "ggs",  213, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "Gate-Source conductance", true );
    modelInfos[MODEL_TYPE::MES].instanceParams.emplace_back( "ggd",  214, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "Gate-Drain conductance", true );
    modelInfos[MODEL_TYPE::MES].instanceParams.emplace_back( "cqgs",  216, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_FLOAT, "F", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "Capacitance due to gate-source charge storage", true );
    modelInfos[MODEL_TYPE::MES].instanceParams.emplace_back( "cqgd",  218, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_FLOAT, "F", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "Capacitance due to gate-drain charge storage", true );
    modelInfos[MODEL_TYPE::MES].instanceParams.emplace_back( "qgs",  215, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "Gate-Source charge storage", true );
    modelInfos[MODEL_TYPE::MES].instanceParams.emplace_back( "qgd",  217, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "Gate-Drain charge storage", true );
    modelInfos[MODEL_TYPE::MES].instanceParams.emplace_back( "is",  6, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_FLOAT, "A", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "1e-14", "1e-14", "Source current", true );
    modelInfos[MODEL_TYPE::MES].instanceParams.emplace_back( "p",  7, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "Power dissipated by the mesfet", true );


    modelInfos[MODEL_TYPE::MESA] = { "MESA", "NMF", "PMF", { "D", "G", "S" }, "GaAs MESFET model", {}, {} };
    // Model parameters
    modelInfos[MODEL_TYPE::MESA].modelParams.emplace_back( "type", 165, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_STRING, "", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "nmf", "nmf", "N-type or P-type MESfet model" );
    modelInfos[MODEL_TYPE::MESA].modelParams.emplace_back( "vto", 101, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "V", SIM_MODEL::PARAM::CATEGORY::DC, "-1.26", "-1.26", "Pinch-off voltage" );
    modelInfos[MODEL_TYPE::MESA].modelParams.emplace_back( "vt0", 101, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "-1.26", "-1.26", "n.a." );
    modelInfos[MODEL_TYPE::MESA].modelParams.emplace_back( "lambda", 103, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::DC, "0.045", "0.045", "Output conductance parameter" );
    modelInfos[MODEL_TYPE::MESA].modelParams.emplace_back( "lambdahf", 143, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::DC, "0.045", "0.045", "Output conductance parameter at high frequencies" );
    modelInfos[MODEL_TYPE::MESA].modelParams.emplace_back( "beta", 153, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "A/V^2", SIM_MODEL::PARAM::CATEGORY::DC, "0.0085", "0.0085", "Transconductance parameter" );
    modelInfos[MODEL_TYPE::MESA].modelParams.emplace_back( "vs", 102, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "m/s", SIM_MODEL::PARAM::CATEGORY::DC, "150000", "150000", "Saturation velocity" );
    modelInfos[MODEL_TYPE::MESA].modelParams.emplace_back( "rd", 104, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "ohm", SIM_MODEL::PARAM::CATEGORY::DC, "0", "0", "Drain ohmic resistance" );
    modelInfos[MODEL_TYPE::MESA].modelParams.emplace_back( "rs", 105, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "ohm", SIM_MODEL::PARAM::CATEGORY::DC, "0", "0", "Source ohmic resistance" );
    modelInfos[MODEL_TYPE::MESA].modelParams.emplace_back( "rg", 106, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "ohm", SIM_MODEL::PARAM::CATEGORY::DC, "0", "0", "Gate ohmic resistance" );
    modelInfos[MODEL_TYPE::MESA].modelParams.emplace_back( "ri", 107, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "ohm", SIM_MODEL::PARAM::CATEGORY::DC, "0", "0", "Gate-source ohmic resistance" );
    modelInfos[MODEL_TYPE::MESA].modelParams.emplace_back( "rf", 108, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "ohm", SIM_MODEL::PARAM::CATEGORY::DC, "0", "0", "Gate-drain ohmic resistance" );
    modelInfos[MODEL_TYPE::MESA].modelParams.emplace_back( "rdi", 109, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "ohm", SIM_MODEL::PARAM::CATEGORY::DC, "0", "0", "Intrinsic source ohmic resistance" );
    modelInfos[MODEL_TYPE::MESA].modelParams.emplace_back( "rsi", 110, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "ohm", SIM_MODEL::PARAM::CATEGORY::DC, "0", "0", "Intrinsic drain ohmic resistance" );
    modelInfos[MODEL_TYPE::MESA].modelParams.emplace_back( "phib", 111, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "°C", SIM_MODEL::PARAM::CATEGORY::TEMPERATURE, "8.01088e-20", "8.01088e-20", "Effective Schottky barrier height at room temperature" );
    modelInfos[MODEL_TYPE::MESA].modelParams.emplace_back( "phib1", 112, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::DC, "0", "0", "" );
    modelInfos[MODEL_TYPE::MESA].modelParams.emplace_back( "tphib", 112, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "0", "0", "n.a." );
    modelInfos[MODEL_TYPE::MESA].modelParams.emplace_back( "astar", 113, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::DC, "40000", "40000", "Effective Richardson constant" );
    modelInfos[MODEL_TYPE::MESA].modelParams.emplace_back( "ggr", 114, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::DC, "40", "40", "Reverse diode conductance" );
    modelInfos[MODEL_TYPE::MESA].modelParams.emplace_back( "del", 115, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::DC, "0.04", "0.04", "" );
    modelInfos[MODEL_TYPE::MESA].modelParams.emplace_back( "xchi", 116, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::DC, "0.033", "0.033", "" );
    modelInfos[MODEL_TYPE::MESA].modelParams.emplace_back( "tggr", 116, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "0.033", "0.033", "n.a." );
    modelInfos[MODEL_TYPE::MESA].modelParams.emplace_back( "n", 117, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::DC, "1", "1", "Emission coefficient" );
    modelInfos[MODEL_TYPE::MESA].modelParams.emplace_back( "eta", 118, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::DC, "1.73", "1.73", "Subthreshold ideality factor" );
    modelInfos[MODEL_TYPE::MESA].modelParams.emplace_back( "m_", 119, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::DC, "2.5", "2.5", "Knee shape parameter" );
    modelInfos[MODEL_TYPE::MESA].modelParams.emplace_back( "mc", 120, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::DC, "3", "3", "Knee shape parameter" );
    modelInfos[MODEL_TYPE::MESA].modelParams.emplace_back( "alpha", 149, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::DC, "0", "0", "" );
    modelInfos[MODEL_TYPE::MESA].modelParams.emplace_back( "sigma0", 121, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "V", SIM_MODEL::PARAM::CATEGORY::DC, "0.081", "0.081", "Threshold voltage coefficient" );
    modelInfos[MODEL_TYPE::MESA].modelParams.emplace_back( "vsigmat", 122, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::DC, "1.01", "1.01", "" );
    modelInfos[MODEL_TYPE::MESA].modelParams.emplace_back( "vsigma", 123, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::DC, "0.1", "0.1", "" );
    modelInfos[MODEL_TYPE::MESA].modelParams.emplace_back( "mu", 124, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::DC, "0.23", "0.23", "Mobility" );
    modelInfos[MODEL_TYPE::MESA].modelParams.emplace_back( "theta", 148, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::DC, "0", "0", "" );
    modelInfos[MODEL_TYPE::MESA].modelParams.emplace_back( "mu1", 125, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::DC, "0", "0", "Second moblity parameter" );
    modelInfos[MODEL_TYPE::MESA].modelParams.emplace_back( "mu2", 126, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::DC, "0", "0", "Third moblity parameter" );
    modelInfos[MODEL_TYPE::MESA].modelParams.emplace_back( "d", 127, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::DC, "1.2e-07", "1.2e-07", "Depth of device" );
    modelInfos[MODEL_TYPE::MESA].modelParams.emplace_back( "nd", 128, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::DC, "2e+23", "2e+23", "Doping density" );
    modelInfos[MODEL_TYPE::MESA].modelParams.emplace_back( "du", 154, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::DC, "3.5e-08", "3.5e-08", "Depth of device" );
    modelInfos[MODEL_TYPE::MESA].modelParams.emplace_back( "ndu", 155, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::DC, "1e+22", "1e+22", "Doping density" );
    modelInfos[MODEL_TYPE::MESA].modelParams.emplace_back( "th", 156, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "m", SIM_MODEL::PARAM::CATEGORY::DC, "1e-08", "1e-08", "Thickness of delta doped layer" );
    modelInfos[MODEL_TYPE::MESA].modelParams.emplace_back( "ndelta", 157, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::DC, "6e+24", "6e+24", "Delta doped layer doping density" );
    modelInfos[MODEL_TYPE::MESA].modelParams.emplace_back( "delta", 129, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::DC, "5", "5", "" );
    modelInfos[MODEL_TYPE::MESA].modelParams.emplace_back( "tc", 130, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::DC, "0", "0", "Transconductance compression factor" );
    modelInfos[MODEL_TYPE::MESA].modelParams.emplace_back( "tvto", 132, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "°C", SIM_MODEL::PARAM::CATEGORY::TEMPERATURE, "0", "0", "Temperature coefficient for vto" );
    modelInfos[MODEL_TYPE::MESA].modelParams.emplace_back( "alphat", 132, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "0", "0", "n.a." );
    modelInfos[MODEL_TYPE::MESA].modelParams.emplace_back( "tlambda", 134, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "°C", SIM_MODEL::PARAM::CATEGORY::TEMPERATURE, "1.79769e+308", "1.79769e+308", "Temperature coefficient for lambda" );
    modelInfos[MODEL_TYPE::MESA].modelParams.emplace_back( "teta0", 135, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "°C", SIM_MODEL::PARAM::CATEGORY::TEMPERATURE, "1.79769e+308", "1.79769e+308", "First temperature coefficient for eta" );
    modelInfos[MODEL_TYPE::MESA].modelParams.emplace_back( "teta1", 136, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "°C", SIM_MODEL::PARAM::CATEGORY::TEMPERATURE, "0", "0", "Second temperature coefficient for eta" );
    modelInfos[MODEL_TYPE::MESA].modelParams.emplace_back( "tmu", 137, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "°C", SIM_MODEL::PARAM::CATEGORY::TEMPERATURE, "300.15", "300.15", "Temperature coefficient for mobility" );
    modelInfos[MODEL_TYPE::MESA].modelParams.emplace_back( "xtm0", 138, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::DC, "0", "0", "First exponent for temp dependence of mobility" );
    modelInfos[MODEL_TYPE::MESA].modelParams.emplace_back( "xtm1", 139, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::DC, "0", "0", "Second exponent for temp dependence of mobility" );
    modelInfos[MODEL_TYPE::MESA].modelParams.emplace_back( "xtm2", 140, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::DC, "0", "0", "Third exponent for temp dependence of mobility" );
    modelInfos[MODEL_TYPE::MESA].modelParams.emplace_back( "ks", 141, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::DC, "0", "0", "Sidegating coefficient" );
    modelInfos[MODEL_TYPE::MESA].modelParams.emplace_back( "vsg", 142, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "V", SIM_MODEL::PARAM::CATEGORY::DC, "0", "0", "Sidegating voltage" );
    modelInfos[MODEL_TYPE::MESA].modelParams.emplace_back( "tf", 144, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "°C", SIM_MODEL::PARAM::CATEGORY::TEMPERATURE, "300.15", "300.15", "Characteristic temperature determined by traps" );
    modelInfos[MODEL_TYPE::MESA].modelParams.emplace_back( "flo", 145, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::DC, "0", "0", "" );
    modelInfos[MODEL_TYPE::MESA].modelParams.emplace_back( "delfo", 146, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::DC, "0", "0", "" );
    modelInfos[MODEL_TYPE::MESA].modelParams.emplace_back( "ag", 147, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::DC, "0", "0", "" );
    modelInfos[MODEL_TYPE::MESA].modelParams.emplace_back( "rtc1", 150, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::DC, "0", "0", "" );
    modelInfos[MODEL_TYPE::MESA].modelParams.emplace_back( "rtc2", 151, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::DC, "0", "0", "" );
    modelInfos[MODEL_TYPE::MESA].modelParams.emplace_back( "zeta", 152, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::DC, "1", "1", "" );
    modelInfos[MODEL_TYPE::MESA].modelParams.emplace_back( "level", 158, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "2", "2", "" );
    modelInfos[MODEL_TYPE::MESA].modelParams.emplace_back( "nmax", 159, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::DC, "2e+16", "2e+16", "" );
    modelInfos[MODEL_TYPE::MESA].modelParams.emplace_back( "gamma", 160, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::DC, "3", "3", "" );
    modelInfos[MODEL_TYPE::MESA].modelParams.emplace_back( "epsi", 161, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::DC, "1.08411e-10", "1.08411e-10", "" );
    modelInfos[MODEL_TYPE::MESA].modelParams.emplace_back( "cas", 163, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::DC, "1", "1", "" );
    modelInfos[MODEL_TYPE::MESA].modelParams.emplace_back( "cbs", 162, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::DC, "1", "1", "" );
    modelInfos[MODEL_TYPE::MESA].modelParams.emplace_back( "pmf", 164, SIM_MODEL::PARAM::DIR_IN, SIM_VALUE::TYPE_BOOL, "", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "P type MESfet model" );
    modelInfos[MODEL_TYPE::MESA].modelParams.emplace_back( "nmf", 131, SIM_MODEL::PARAM::DIR_IN, SIM_VALUE::TYPE_BOOL, "", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "N type MESfet model" );
    modelInfos[MODEL_TYPE::MESA].modelParams.emplace_back( "gd", 301, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::DC, "1.79769e+308", "1.79769e+308", "Drain conductance" );
    modelInfos[MODEL_TYPE::MESA].modelParams.emplace_back( "gs", 302, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::DC, "1.79769e+308", "1.79769e+308", "Source conductance" );
    modelInfos[MODEL_TYPE::MESA].modelParams.emplace_back( "vcrit", 305, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_FLOAT, "V", SIM_MODEL::PARAM::CATEGORY::DC, "0", "0", "Critical voltage" );
    // Instance parameters
    modelInfos[MODEL_TYPE::MESA].instanceParams.emplace_back( "off", 8, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_BOOL, "", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "Device initially off", true );
    modelInfos[MODEL_TYPE::MESA].instanceParams.emplace_back( "m", 12, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::GEOMETRY, "2.5", "2.5", "Parallel Multiplier", true );
    modelInfos[MODEL_TYPE::MESA].instanceParams.emplace_back( "l", 1, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "m", SIM_MODEL::PARAM::CATEGORY::GEOMETRY, "", "", "Length of device", true );
    modelInfos[MODEL_TYPE::MESA].instanceParams.emplace_back( "w", 2, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "m", SIM_MODEL::PARAM::CATEGORY::GEOMETRY, "", "", "Width of device", true );
    modelInfos[MODEL_TYPE::MESA].instanceParams.emplace_back( "icvds", 3, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "V", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "Initial D-S voltage", true );
    modelInfos[MODEL_TYPE::MESA].instanceParams.emplace_back( "icvgs", 4, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "V", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "Initial G-S voltage", true );
    modelInfos[MODEL_TYPE::MESA].instanceParams.emplace_back( "td", 5, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "°C", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "Instance drain temperature", true );
    modelInfos[MODEL_TYPE::MESA].instanceParams.emplace_back( "ts", 6, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "°C", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "Instance source temperature", true );
    modelInfos[MODEL_TYPE::MESA].instanceParams.emplace_back( "dtemp", 11, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "°C", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "Instance temperature difference", true );
    modelInfos[MODEL_TYPE::MESA].instanceParams.emplace_back( "dnode", 201, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_INT, "", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "Number of drain node", true );
    modelInfos[MODEL_TYPE::MESA].instanceParams.emplace_back( "gnode", 202, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_INT, "", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "Number of gate node", true );
    modelInfos[MODEL_TYPE::MESA].instanceParams.emplace_back( "snode", 203, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_INT, "", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "Number of source node", true );
    modelInfos[MODEL_TYPE::MESA].instanceParams.emplace_back( "dprimenode", 204, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_INT, "", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "Number of internal drain node", true );
    modelInfos[MODEL_TYPE::MESA].instanceParams.emplace_back( "sprimenode", 205, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_INT, "", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "Number of internal source node", true );
    modelInfos[MODEL_TYPE::MESA].instanceParams.emplace_back( "gprimenode", 206, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_INT, "", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "Number of internal gate node", true );
    modelInfos[MODEL_TYPE::MESA].instanceParams.emplace_back( "vgs", 207, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_FLOAT, "V", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "Gate-Source voltage", true );
    modelInfos[MODEL_TYPE::MESA].instanceParams.emplace_back( "vgd", 208, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_FLOAT, "V", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "Gate-Drain voltage", true );
    modelInfos[MODEL_TYPE::MESA].instanceParams.emplace_back( "cg", 209, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_FLOAT, "F", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "Gate capacitance", true );
    modelInfos[MODEL_TYPE::MESA].instanceParams.emplace_back( "cd", 210, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_FLOAT, "F", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "Drain capacitance", true );
    modelInfos[MODEL_TYPE::MESA].instanceParams.emplace_back( "cgd", 211, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_FLOAT, "F", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "Gate_Drain capacitance", true );
    modelInfos[MODEL_TYPE::MESA].instanceParams.emplace_back( "gm", 212, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "Transconductance", true );
    modelInfos[MODEL_TYPE::MESA].instanceParams.emplace_back( "gds", 213, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "Drain-Source conductance", true );
    modelInfos[MODEL_TYPE::MESA].instanceParams.emplace_back( "ggs", 214, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "Gate-Source conductance", true );
    modelInfos[MODEL_TYPE::MESA].instanceParams.emplace_back( "ggd", 215, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "Gate-Drain conductance", true );
    modelInfos[MODEL_TYPE::MESA].instanceParams.emplace_back( "qgs", 216, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "Gate-Source charge storage", true );
    modelInfos[MODEL_TYPE::MESA].instanceParams.emplace_back( "cqgs", 217, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_FLOAT, "F", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "Capacitance due to gate-source charge storage", true );
    modelInfos[MODEL_TYPE::MESA].instanceParams.emplace_back( "qgd", 218, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "Gate-Drain charge storage", true );
    modelInfos[MODEL_TYPE::MESA].instanceParams.emplace_back( "cqgd", 219, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_FLOAT, "F", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "Capacitance due to gate-drain charge storage", true );
    modelInfos[MODEL_TYPE::MESA].instanceParams.emplace_back( "cs", 9, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_FLOAT, "A", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "Source current", true );
    modelInfos[MODEL_TYPE::MESA].instanceParams.emplace_back( "p", 10, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "Power dissipated by the mesfet", true );
}