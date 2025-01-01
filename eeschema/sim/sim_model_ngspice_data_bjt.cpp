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


void NGSPICE_MODEL_INFO_MAP::addBJT()
{
    modelInfos[MODEL_TYPE::BJT] = { "BJT", "NPN", "PNP", { "C", "B", "E", "<S>" }, "Bipolar Junction Transistor", {}, {} };
    // Model parameters
    modelInfos[MODEL_TYPE::BJT].modelParams.emplace_back( "type",  309, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_STRING, "", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "npn", "pnp", "NPN or PNP" );
    modelInfos[MODEL_TYPE::BJT].modelParams.emplace_back( "npn",  101, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_BOOL, "", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "NaN", "NaN", "NPN type device" );
    modelInfos[MODEL_TYPE::BJT].modelParams.emplace_back( "pnp",  102, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_BOOL, "", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "NaN", "NaN", "PNP type device" );
    modelInfos[MODEL_TYPE::BJT].modelParams.emplace_back( "subs",  204, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_INT, "", SIM_MODEL::PARAM::CATEGORY::DC, "-1721368256", "-514428616", "Vertical or Lateral device" );
    modelInfos[MODEL_TYPE::BJT].modelParams.emplace_back( "tnom",  151, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "°C", SIM_MODEL::PARAM::CATEGORY::TEMPERATURE, "27", "27", "Parameter measurement temperature" );
    modelInfos[MODEL_TYPE::BJT].modelParams.emplace_back( "tref",  151, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "27", "27", "n.a." );
    modelInfos[MODEL_TYPE::BJT].modelParams.emplace_back( "is_",  103, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "A", SIM_MODEL::PARAM::CATEGORY::DC, "1e-16", "1e-16", "Saturation Current" );
    modelInfos[MODEL_TYPE::BJT].modelParams.emplace_back( "ibe",  104, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "A", SIM_MODEL::PARAM::CATEGORY::DC, "0", "0", "Base-Emitter saturation Current" );
    modelInfos[MODEL_TYPE::BJT].modelParams.emplace_back( "ibc",  105, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "A", SIM_MODEL::PARAM::CATEGORY::DC, "0", "0", "Base-Collector saturation Current" );
    modelInfos[MODEL_TYPE::BJT].modelParams.emplace_back( "bf",  106, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::DC, "100", "100", "Ideal forward beta" );
    modelInfos[MODEL_TYPE::BJT].modelParams.emplace_back( "nf",  107, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::DC, "1", "1", "Forward emission coefficient" );
    modelInfos[MODEL_TYPE::BJT].modelParams.emplace_back( "vaf",  108, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "V", SIM_MODEL::PARAM::CATEGORY::DC, "0", "0", "Forward Early voltage" );
    modelInfos[MODEL_TYPE::BJT].modelParams.emplace_back( "va",  108, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "0", "0", "n.a." );
    modelInfos[MODEL_TYPE::BJT].modelParams.emplace_back( "ikf",  109, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "A", SIM_MODEL::PARAM::CATEGORY::DC, "0", "0", "Forward beta roll-off corner current" );
    modelInfos[MODEL_TYPE::BJT].modelParams.emplace_back( "ik",  109, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "0", "0", "n.a." );
    modelInfos[MODEL_TYPE::BJT].modelParams.emplace_back( "ise",  110, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "A", SIM_MODEL::PARAM::CATEGORY::DC, "0", "0", "B-E leakage saturation current" );
    modelInfos[MODEL_TYPE::BJT].modelParams.emplace_back( "c2",  110, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "0", "0", "n.a." );
    modelInfos[MODEL_TYPE::BJT].modelParams.emplace_back( "ne",  111, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::DC, "1.5", "1.5", "B-E leakage emission coefficient" );
    modelInfos[MODEL_TYPE::BJT].modelParams.emplace_back( "br",  112, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::DC, "1", "1", "Ideal reverse beta" );
    modelInfos[MODEL_TYPE::BJT].modelParams.emplace_back( "nr",  113, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::DC, "1", "1", "Reverse emission coefficient" );
    modelInfos[MODEL_TYPE::BJT].modelParams.emplace_back( "var",  114, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "V", SIM_MODEL::PARAM::CATEGORY::DC, "0", "0", "Reverse Early voltage" );
    modelInfos[MODEL_TYPE::BJT].modelParams.emplace_back( "vb",  114, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "0", "0", "n.a." );
    modelInfos[MODEL_TYPE::BJT].modelParams.emplace_back( "ikr",  115, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "A", SIM_MODEL::PARAM::CATEGORY::DC, "0", "0", "reverse beta roll-off corner current" );
    modelInfos[MODEL_TYPE::BJT].modelParams.emplace_back( "isc",  116, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "A", SIM_MODEL::PARAM::CATEGORY::DC, "0", "0", "B-C leakage saturation current" );
    modelInfos[MODEL_TYPE::BJT].modelParams.emplace_back( "c4",  116, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "0", "0", "n.a." );
    modelInfos[MODEL_TYPE::BJT].modelParams.emplace_back( "nc",  117, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::DC, "2", "2", "B-C leakage emission coefficient" );
    modelInfos[MODEL_TYPE::BJT].modelParams.emplace_back( "rb",  118, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "Ω", SIM_MODEL::PARAM::CATEGORY::DC, "0", "0", "Zero bias base resistance" );
    modelInfos[MODEL_TYPE::BJT].modelParams.emplace_back( "irb",  119, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "Ω", SIM_MODEL::PARAM::CATEGORY::DC, "0", "0", "Current for base resistance=(rb+rbm)/2" );
    modelInfos[MODEL_TYPE::BJT].modelParams.emplace_back( "rbm",  120, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "Ω", SIM_MODEL::PARAM::CATEGORY::DC, "0", "0", "Minimum base resistance" );
    modelInfos[MODEL_TYPE::BJT].modelParams.emplace_back( "re",  121, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "Ω", SIM_MODEL::PARAM::CATEGORY::DC, "0", "0", "Emitter resistance" );
    modelInfos[MODEL_TYPE::BJT].modelParams.emplace_back( "rc",  122, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "Ω", SIM_MODEL::PARAM::CATEGORY::DC, "0", "0", "Collector resistance" );
    modelInfos[MODEL_TYPE::BJT].modelParams.emplace_back( "cje",  123, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "F", SIM_MODEL::PARAM::CATEGORY::CAPACITANCE, "0", "0", "Zero bias B-E depletion capacitance" );
    modelInfos[MODEL_TYPE::BJT].modelParams.emplace_back( "vje",  124, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "V", SIM_MODEL::PARAM::CATEGORY::DC, "0.75", "0.75", "B-E built in potential" );
    modelInfos[MODEL_TYPE::BJT].modelParams.emplace_back( "pe",  124, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "V", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "0.75", "0.75", "B-E built in potential" );
    modelInfos[MODEL_TYPE::BJT].modelParams.emplace_back( "mje",  125, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::DC, "0.33", "0.33", "B-E junction grading coefficient" );
    modelInfos[MODEL_TYPE::BJT].modelParams.emplace_back( "me",  125, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "0.33", "0.33", "B-E junction grading coefficient" );
    modelInfos[MODEL_TYPE::BJT].modelParams.emplace_back( "tf",  126, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "s", SIM_MODEL::PARAM::CATEGORY::DC, "0", "0", "Ideal forward transit time" );
    modelInfos[MODEL_TYPE::BJT].modelParams.emplace_back( "xtf",  127, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::DC, "0", "0", "Coefficient for bias dependence of TF" );
    modelInfos[MODEL_TYPE::BJT].modelParams.emplace_back( "vtf",  128, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "V", SIM_MODEL::PARAM::CATEGORY::DC, "0", "0", "Voltage giving VBC dependence of TF" );
    modelInfos[MODEL_TYPE::BJT].modelParams.emplace_back( "itf",  129, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "A", SIM_MODEL::PARAM::CATEGORY::DC, "0", "0", "High current dependence of TF" );
    modelInfos[MODEL_TYPE::BJT].modelParams.emplace_back( "ptf",  130, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "deg", SIM_MODEL::PARAM::CATEGORY::DC, "0", "0", "Excess phase" );
    modelInfos[MODEL_TYPE::BJT].modelParams.emplace_back( "cjc",  131, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "F", SIM_MODEL::PARAM::CATEGORY::CAPACITANCE, "0", "0", "Zero bias B-C depletion capacitance" );
    modelInfos[MODEL_TYPE::BJT].modelParams.emplace_back( "vjc",  132, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "V", SIM_MODEL::PARAM::CATEGORY::DC, "0.75", "0.75", "B-C built in potential" );
    modelInfos[MODEL_TYPE::BJT].modelParams.emplace_back( "pc",  132, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "V", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "0.75", "0.75", "B-C built in potential" );
    modelInfos[MODEL_TYPE::BJT].modelParams.emplace_back( "mjc",  133, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::DC, "0.33", "0.33", "B-C junction grading coefficient" );
    modelInfos[MODEL_TYPE::BJT].modelParams.emplace_back( "mc",  133, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "0.33", "0.33", "B-C junction grading coefficient" );
    modelInfos[MODEL_TYPE::BJT].modelParams.emplace_back( "xcjc",  134, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::DC, "1", "1", "Fraction of B-C cap to internal base" );
    modelInfos[MODEL_TYPE::BJT].modelParams.emplace_back( "tr",  135, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "s", SIM_MODEL::PARAM::CATEGORY::DC, "0", "0", "Ideal reverse transit time" );
    modelInfos[MODEL_TYPE::BJT].modelParams.emplace_back( "cjs",  136, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "F", SIM_MODEL::PARAM::CATEGORY::CAPACITANCE, "0", "0", "Zero bias Substrate capacitance" );
    modelInfos[MODEL_TYPE::BJT].modelParams.emplace_back( "csub_",  136, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "F", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "0", "0", "Zero bias Substrate capacitance" );
    modelInfos[MODEL_TYPE::BJT].modelParams.emplace_back( "ccs",  136, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "F", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "0", "0", "Zero bias Substrate capacitance" );
    modelInfos[MODEL_TYPE::BJT].modelParams.emplace_back( "vjs",  137, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "V", SIM_MODEL::PARAM::CATEGORY::DC, "0.75", "0.75", "Substrate junction built in potential" );
    modelInfos[MODEL_TYPE::BJT].modelParams.emplace_back( "ps",  137, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "V", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "0.75", "0.75", "Substrate junction built in potential" );
    modelInfos[MODEL_TYPE::BJT].modelParams.emplace_back( "mjs",  138, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::DC, "0", "0", "Substrate junction grading coefficient" );
    modelInfos[MODEL_TYPE::BJT].modelParams.emplace_back( "ms",  138, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "0", "0", "Substrate junction grading coefficient" );
    modelInfos[MODEL_TYPE::BJT].modelParams.emplace_back( "xtb",  139, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::DC, "0", "0", "Forward and reverse beta temp. exp." );
    modelInfos[MODEL_TYPE::BJT].modelParams.emplace_back( "eg",  140, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "eV", SIM_MODEL::PARAM::CATEGORY::DC, "1.11", "1.11", "Energy gap for IS temp. dependency" );
    modelInfos[MODEL_TYPE::BJT].modelParams.emplace_back( "xti",  141, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::DC, "3", "3", "Temp. exponent for IS" );
    modelInfos[MODEL_TYPE::BJT].modelParams.emplace_back( "fc",  142, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::DC, "0.5", "0.5", "Forward bias junction fit parameter" );
    modelInfos[MODEL_TYPE::BJT].modelParams.emplace_back( "kf",  144, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::NOISE, "0", "0", "Flicker Noise Coefficient" );
    modelInfos[MODEL_TYPE::BJT].modelParams.emplace_back( "af",  143, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::NOISE, "0", "0", "Flicker Noise Exponent" );
    modelInfos[MODEL_TYPE::BJT].modelParams.emplace_back( "invearlyvoltf",  301, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_FLOAT, "V", SIM_MODEL::PARAM::CATEGORY::DC, "0", "0", "Inverse early voltage:forward" );
    modelInfos[MODEL_TYPE::BJT].modelParams.emplace_back( "invearlyvoltr",  302, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_FLOAT, "V", SIM_MODEL::PARAM::CATEGORY::DC, "0", "0", "Inverse early voltage:reverse" );
    modelInfos[MODEL_TYPE::BJT].modelParams.emplace_back( "invrollofff",  303, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::DC, "0", "0", "Inverse roll off - forward" );
    modelInfos[MODEL_TYPE::BJT].modelParams.emplace_back( "invrolloffr",  304, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::DC, "0", "0", "Inverse roll off - reverse" );
    modelInfos[MODEL_TYPE::BJT].modelParams.emplace_back( "collectorconduct",  305, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::DC, "0", "0", "Collector conductance" );
    modelInfos[MODEL_TYPE::BJT].modelParams.emplace_back( "emitterconduct",  306, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::DC, "0", "0", "Emitter conductance" );
    modelInfos[MODEL_TYPE::BJT].modelParams.emplace_back( "transtimevbcfact",  307, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_FLOAT, "s", SIM_MODEL::PARAM::CATEGORY::DC, "0", "0", "Transit time VBC factor" );
    modelInfos[MODEL_TYPE::BJT].modelParams.emplace_back( "excessphasefactor",  308, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_FLOAT, "deg", SIM_MODEL::PARAM::CATEGORY::DC, "0", "0", "Excess phase fact." );
    modelInfos[MODEL_TYPE::BJT].modelParams.emplace_back( "iss",  145, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "A", SIM_MODEL::PARAM::CATEGORY::DC, "0", "0", "Substrate Jct. Saturation Current" );
    modelInfos[MODEL_TYPE::BJT].modelParams.emplace_back( "ns",  146, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "A", SIM_MODEL::PARAM::CATEGORY::DC, "1", "1", "Substrate current emission coefficient" );
    modelInfos[MODEL_TYPE::BJT].modelParams.emplace_back( "rco",  147, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "Ω", SIM_MODEL::PARAM::CATEGORY::DC, "0.01", "0.01", "Intrinsic coll. resistance" );
    modelInfos[MODEL_TYPE::BJT].modelParams.emplace_back( "vo",  148, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "V", SIM_MODEL::PARAM::CATEGORY::DC, "10", "10", "Epi drift saturation voltage" );
    modelInfos[MODEL_TYPE::BJT].modelParams.emplace_back( "gamma",  149, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::DC, "1e-11", "1e-11", "Epi doping parameter" );
    modelInfos[MODEL_TYPE::BJT].modelParams.emplace_back( "qco",  150, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "C", SIM_MODEL::PARAM::CATEGORY::DC, "0", "0", "Epi Charge parameter" );
    modelInfos[MODEL_TYPE::BJT].modelParams.emplace_back( "tlev",  152, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_INT, "", SIM_MODEL::PARAM::CATEGORY::TEMPERATURE, "0", "0", "Temperature equation selector" );
    modelInfos[MODEL_TYPE::BJT].modelParams.emplace_back( "tlevc",  153, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_INT, "", SIM_MODEL::PARAM::CATEGORY::TEMPERATURE, "0", "0", "Temperature equation selector" );
    modelInfos[MODEL_TYPE::BJT].modelParams.emplace_back( "tbf1",  154, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "°C", SIM_MODEL::PARAM::CATEGORY::TEMPERATURE, "0", "0", "BF 1. temperature coefficient" );
    modelInfos[MODEL_TYPE::BJT].modelParams.emplace_back( "tbf2",  155, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "°C", SIM_MODEL::PARAM::CATEGORY::TEMPERATURE, "0", "0", "BF 2. temperature coefficient" );
    modelInfos[MODEL_TYPE::BJT].modelParams.emplace_back( "tbr1",  156, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "°C", SIM_MODEL::PARAM::CATEGORY::TEMPERATURE, "0", "0", "BR 1. temperature coefficient" );
    modelInfos[MODEL_TYPE::BJT].modelParams.emplace_back( "tbr2",  157, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "°C", SIM_MODEL::PARAM::CATEGORY::TEMPERATURE, "0", "0", "BR 2. temperature coefficient" );
    modelInfos[MODEL_TYPE::BJT].modelParams.emplace_back( "tikf1",  158, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "°C", SIM_MODEL::PARAM::CATEGORY::TEMPERATURE, "0", "0", "IKF 1. temperature coefficient" );
    modelInfos[MODEL_TYPE::BJT].modelParams.emplace_back( "tikf2",  159, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "°C", SIM_MODEL::PARAM::CATEGORY::TEMPERATURE, "0", "0", "IKF 2. temperature coefficient" );
    modelInfos[MODEL_TYPE::BJT].modelParams.emplace_back( "tikr1",  160, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "°C", SIM_MODEL::PARAM::CATEGORY::TEMPERATURE, "0", "0", "IKR 1. temperature coefficient" );
    modelInfos[MODEL_TYPE::BJT].modelParams.emplace_back( "tikr2",  161, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "°C", SIM_MODEL::PARAM::CATEGORY::TEMPERATURE, "0", "0", "IKR 2. temperature coefficient" );
    modelInfos[MODEL_TYPE::BJT].modelParams.emplace_back( "tirb1",  162, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "°C", SIM_MODEL::PARAM::CATEGORY::TEMPERATURE, "0", "0", "IRB 1. temperature coefficient" );
    modelInfos[MODEL_TYPE::BJT].modelParams.emplace_back( "tirb2",  163, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "°C", SIM_MODEL::PARAM::CATEGORY::TEMPERATURE, "0", "0", "IRB 2. temperature coefficient" );
    modelInfos[MODEL_TYPE::BJT].modelParams.emplace_back( "tnc1",  164, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "°C", SIM_MODEL::PARAM::CATEGORY::TEMPERATURE, "0", "0", "NC 1. temperature coefficient" );
    modelInfos[MODEL_TYPE::BJT].modelParams.emplace_back( "tnc2",  165, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "°C", SIM_MODEL::PARAM::CATEGORY::TEMPERATURE, "0", "0", "NC 2. temperature coefficient" );
    modelInfos[MODEL_TYPE::BJT].modelParams.emplace_back( "tne1",  166, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "°C", SIM_MODEL::PARAM::CATEGORY::TEMPERATURE, "0", "0", "NE 1. temperature coefficient" );
    modelInfos[MODEL_TYPE::BJT].modelParams.emplace_back( "tne2",  167, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "°C", SIM_MODEL::PARAM::CATEGORY::TEMPERATURE, "0", "0", "NE 2. temperature coefficient" );
    modelInfos[MODEL_TYPE::BJT].modelParams.emplace_back( "tnf1",  168, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "°C", SIM_MODEL::PARAM::CATEGORY::TEMPERATURE, "0", "0", "NF 1. temperature coefficient" );
    modelInfos[MODEL_TYPE::BJT].modelParams.emplace_back( "tnf2",  169, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "°C", SIM_MODEL::PARAM::CATEGORY::TEMPERATURE, "0", "0", "NF 2. temperature coefficient" );
    modelInfos[MODEL_TYPE::BJT].modelParams.emplace_back( "tnr1",  170, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "°C", SIM_MODEL::PARAM::CATEGORY::TEMPERATURE, "0", "0", "NR 1. temperature coefficient" );
    modelInfos[MODEL_TYPE::BJT].modelParams.emplace_back( "tnr2",  171, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "°C", SIM_MODEL::PARAM::CATEGORY::TEMPERATURE, "0", "0", "NR 2. temperature coefficient" );
    modelInfos[MODEL_TYPE::BJT].modelParams.emplace_back( "trb1",  172, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "°C", SIM_MODEL::PARAM::CATEGORY::TEMPERATURE, "0", "0", "RB 1. temperature coefficient" );
    modelInfos[MODEL_TYPE::BJT].modelParams.emplace_back( "trb",  172, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "0", "0", "n.a." );
    modelInfos[MODEL_TYPE::BJT].modelParams.emplace_back( "trb2",  173, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "°C", SIM_MODEL::PARAM::CATEGORY::TEMPERATURE, "0", "0", "RB 2. temperature coefficient" );
    modelInfos[MODEL_TYPE::BJT].modelParams.emplace_back( "trc1",  174, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "°C", SIM_MODEL::PARAM::CATEGORY::TEMPERATURE, "0", "0", "RC 1. temperature coefficient" );
    modelInfos[MODEL_TYPE::BJT].modelParams.emplace_back( "trc",  174, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "0", "0", "n.a." );
    modelInfos[MODEL_TYPE::BJT].modelParams.emplace_back( "trc2",  175, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "°C", SIM_MODEL::PARAM::CATEGORY::TEMPERATURE, "0", "0", "RC 2. temperature coefficient" );
    modelInfos[MODEL_TYPE::BJT].modelParams.emplace_back( "tre1",  176, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "°C", SIM_MODEL::PARAM::CATEGORY::TEMPERATURE, "0", "0", "RE 1. temperature coefficient" );
    modelInfos[MODEL_TYPE::BJT].modelParams.emplace_back( "tre",  176, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "0", "0", "n.a." );
    modelInfos[MODEL_TYPE::BJT].modelParams.emplace_back( "tre2",  177, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "°C", SIM_MODEL::PARAM::CATEGORY::TEMPERATURE, "0", "0", "RE 2. temperature coefficient" );
    modelInfos[MODEL_TYPE::BJT].modelParams.emplace_back( "trm1",  178, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "°C", SIM_MODEL::PARAM::CATEGORY::TEMPERATURE, "0", "0", "RBM 1. temperature coefficient" );
    modelInfos[MODEL_TYPE::BJT].modelParams.emplace_back( "trm2",  179, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "°C", SIM_MODEL::PARAM::CATEGORY::TEMPERATURE, "0", "0", "RBM 2. temperature coefficient" );
    modelInfos[MODEL_TYPE::BJT].modelParams.emplace_back( "tvaf1",  180, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "°C", SIM_MODEL::PARAM::CATEGORY::TEMPERATURE, "0", "0", "VAF 1. temperature coefficient" );
    modelInfos[MODEL_TYPE::BJT].modelParams.emplace_back( "tvaf2",  181, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "°C", SIM_MODEL::PARAM::CATEGORY::TEMPERATURE, "0", "0", "VAF 2. temperature coefficient" );
    modelInfos[MODEL_TYPE::BJT].modelParams.emplace_back( "tvar1",  182, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "°C", SIM_MODEL::PARAM::CATEGORY::TEMPERATURE, "0", "0", "VAR 1. temperature coefficient" );
    modelInfos[MODEL_TYPE::BJT].modelParams.emplace_back( "tvar2",  183, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "°C", SIM_MODEL::PARAM::CATEGORY::TEMPERATURE, "0", "0", "VAR 2. temperature coefficient" );
    modelInfos[MODEL_TYPE::BJT].modelParams.emplace_back( "ctc",  184, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "°C", SIM_MODEL::PARAM::CATEGORY::TEMPERATURE, "0", "0", "CJC temperature coefficient" );
    modelInfos[MODEL_TYPE::BJT].modelParams.emplace_back( "cte",  185, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "°C", SIM_MODEL::PARAM::CATEGORY::TEMPERATURE, "0", "0", "CJE temperature coefficient" );
    modelInfos[MODEL_TYPE::BJT].modelParams.emplace_back( "cts",  186, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "°C", SIM_MODEL::PARAM::CATEGORY::TEMPERATURE, "0", "0", "CJS temperature coefficient" );
    modelInfos[MODEL_TYPE::BJT].modelParams.emplace_back( "tvjc",  187, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "°C", SIM_MODEL::PARAM::CATEGORY::TEMPERATURE, "0", "0", "VJC temperature coefficient" );
    modelInfos[MODEL_TYPE::BJT].modelParams.emplace_back( "tvje",  188, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "°C", SIM_MODEL::PARAM::CATEGORY::TEMPERATURE, "0", "0", "VJE temperature coefficient" );
    modelInfos[MODEL_TYPE::BJT].modelParams.emplace_back( "tvjs",  189, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "°C", SIM_MODEL::PARAM::CATEGORY::TEMPERATURE, "0", "0", "VJS temperature coefficient" );
    modelInfos[MODEL_TYPE::BJT].modelParams.emplace_back( "titf1",  190, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "°C", SIM_MODEL::PARAM::CATEGORY::TEMPERATURE, "0", "0", "ITF 1. temperature coefficient" );
    modelInfos[MODEL_TYPE::BJT].modelParams.emplace_back( "titf2",  191, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "°C", SIM_MODEL::PARAM::CATEGORY::TEMPERATURE, "0", "0", "ITF 2. temperature coefficient" );
    modelInfos[MODEL_TYPE::BJT].modelParams.emplace_back( "ttf1",  192, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "°C", SIM_MODEL::PARAM::CATEGORY::TEMPERATURE, "0", "0", "TF 1. temperature coefficient" );
    modelInfos[MODEL_TYPE::BJT].modelParams.emplace_back( "ttf2",  193, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "°C", SIM_MODEL::PARAM::CATEGORY::TEMPERATURE, "0", "0", "TF 2. temperature coefficient" );
    modelInfos[MODEL_TYPE::BJT].modelParams.emplace_back( "ttr1",  194, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "°C", SIM_MODEL::PARAM::CATEGORY::TEMPERATURE, "0", "0", "TR 1. temperature coefficient" );
    modelInfos[MODEL_TYPE::BJT].modelParams.emplace_back( "ttr2",  195, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "°C", SIM_MODEL::PARAM::CATEGORY::TEMPERATURE, "0", "0", "TR 2. temperature coefficient" );
    modelInfos[MODEL_TYPE::BJT].modelParams.emplace_back( "tmje1",  196, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "°C", SIM_MODEL::PARAM::CATEGORY::TEMPERATURE, "0", "0", "MJE 1. temperature coefficient" );
    modelInfos[MODEL_TYPE::BJT].modelParams.emplace_back( "tmje2",  197, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "°C", SIM_MODEL::PARAM::CATEGORY::TEMPERATURE, "0", "0", "MJE 2. temperature coefficient" );
    modelInfos[MODEL_TYPE::BJT].modelParams.emplace_back( "tmjc1",  198, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "°C", SIM_MODEL::PARAM::CATEGORY::TEMPERATURE, "0", "0", "MJC 1. temperature coefficient" );
    modelInfos[MODEL_TYPE::BJT].modelParams.emplace_back( "tmjc2",  199, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "°C", SIM_MODEL::PARAM::CATEGORY::TEMPERATURE, "0", "0", "MJC 2. temperature coefficient" );
    modelInfos[MODEL_TYPE::BJT].modelParams.emplace_back( "tmjs1",  200, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "°C", SIM_MODEL::PARAM::CATEGORY::TEMPERATURE, "0", "0", "MJS 1. temperature coefficient" );
    modelInfos[MODEL_TYPE::BJT].modelParams.emplace_back( "tmjs2",  201, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "°C", SIM_MODEL::PARAM::CATEGORY::TEMPERATURE, "0", "0", "MJS 2. temperature coefficient" );
    modelInfos[MODEL_TYPE::BJT].modelParams.emplace_back( "tns1",  202, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "°C", SIM_MODEL::PARAM::CATEGORY::TEMPERATURE, "0", "0", "NS 1. temperature coefficient" );
    modelInfos[MODEL_TYPE::BJT].modelParams.emplace_back( "tns2",  203, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "°C", SIM_MODEL::PARAM::CATEGORY::TEMPERATURE, "0", "0", "NS 2. temperature coefficient" );
    modelInfos[MODEL_TYPE::BJT].modelParams.emplace_back( "nkf",  205, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::DC, "0.5", "0.5", "NKF High current beta rolloff exponent" );
    modelInfos[MODEL_TYPE::BJT].modelParams.emplace_back( "nk",  205, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "0.5", "0.5", "n.a." );
    modelInfos[MODEL_TYPE::BJT].modelParams.emplace_back( "tis1",  206, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "°C", SIM_MODEL::PARAM::CATEGORY::TEMPERATURE, "0", "0", "IS 1. temperature coefficient" );
    modelInfos[MODEL_TYPE::BJT].modelParams.emplace_back( "tis2",  207, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "°C", SIM_MODEL::PARAM::CATEGORY::TEMPERATURE, "0", "0", "IS 2. temperature coefficient" );
    modelInfos[MODEL_TYPE::BJT].modelParams.emplace_back( "tise1",  208, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "°C", SIM_MODEL::PARAM::CATEGORY::TEMPERATURE, "0", "0", "ISE 1. temperature coefficient" );
    modelInfos[MODEL_TYPE::BJT].modelParams.emplace_back( "tise2",  209, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "°C", SIM_MODEL::PARAM::CATEGORY::TEMPERATURE, "0", "0", "ISE 2. temperature coefficient" );
    modelInfos[MODEL_TYPE::BJT].modelParams.emplace_back( "tisc1",  210, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "°C", SIM_MODEL::PARAM::CATEGORY::TEMPERATURE, "0", "0", "ISC 1. temperature coefficient" );
    modelInfos[MODEL_TYPE::BJT].modelParams.emplace_back( "tisc2",  211, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "°C", SIM_MODEL::PARAM::CATEGORY::TEMPERATURE, "0", "0", "ISC 2. temperature coefficient" );
    modelInfos[MODEL_TYPE::BJT].modelParams.emplace_back( "tiss1",  212, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "°C", SIM_MODEL::PARAM::CATEGORY::TEMPERATURE, "0", "0", "ISS 1. temperature coefficient" );
    modelInfos[MODEL_TYPE::BJT].modelParams.emplace_back( "tiss2",  213, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "°C", SIM_MODEL::PARAM::CATEGORY::TEMPERATURE, "0", "0", "ISS 2. temperature coefficient" );
    modelInfos[MODEL_TYPE::BJT].modelParams.emplace_back( "quasimod",  214, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_INT, "", SIM_MODEL::PARAM::CATEGORY::TEMPERATURE, "0", "0", "Temperature equation selector" );
    modelInfos[MODEL_TYPE::BJT].modelParams.emplace_back( "vg",  215, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "eV", SIM_MODEL::PARAM::CATEGORY::DC, "1.206", "1.206", "Energy gap for QS temp. dependency" );
    modelInfos[MODEL_TYPE::BJT].modelParams.emplace_back( "cn",  216, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::TEMPERATURE, "2.42", "2.2", "Temperature exponent of RCI" );
    modelInfos[MODEL_TYPE::BJT].modelParams.emplace_back( "d",  217, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::TEMPERATURE, "0.87", "0.52", "Temperature exponent of VO" );
    modelInfos[MODEL_TYPE::BJT].modelParams.emplace_back( "vbe_max",  218, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "V", SIM_MODEL::PARAM::CATEGORY::LIMITING_VALUES, "1e+99", "1e+99", "maximum voltage B-E junction" );
    modelInfos[MODEL_TYPE::BJT].modelParams.emplace_back( "vbc_max",  219, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "V", SIM_MODEL::PARAM::CATEGORY::LIMITING_VALUES, "1e+99", "1e+99", "maximum voltage B-C junction" );
    modelInfos[MODEL_TYPE::BJT].modelParams.emplace_back( "vce_max",  220, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "V", SIM_MODEL::PARAM::CATEGORY::LIMITING_VALUES, "1e+99", "1e+99", "maximum voltage C-E branch" );
    modelInfos[MODEL_TYPE::BJT].modelParams.emplace_back( "pd_max",  221, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::LIMITING_VALUES, "1e+99", "1e+99", "maximum device power dissipation" );
    modelInfos[MODEL_TYPE::BJT].modelParams.emplace_back( "ic_max",  222, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "A", SIM_MODEL::PARAM::CATEGORY::LIMITING_VALUES, "1e+99", "1e+99", "maximum collector current" );
    modelInfos[MODEL_TYPE::BJT].modelParams.emplace_back( "ib_max",  223, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "A", SIM_MODEL::PARAM::CATEGORY::LIMITING_VALUES, "1e+99", "1e+99", "maximum base current" );
    modelInfos[MODEL_TYPE::BJT].modelParams.emplace_back( "te_max",  224, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "°C", SIM_MODEL::PARAM::CATEGORY::LIMITING_VALUES, "1e+99", "1e+99", "maximum temperature" );
    modelInfos[MODEL_TYPE::BJT].modelParams.emplace_back( "rth0",  225, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "Ω", SIM_MODEL::PARAM::CATEGORY::TEMPERATURE, "0", "0", "thermal resistance juntion to ambient" );
    // Instance parameters
    modelInfos[MODEL_TYPE::BJT].instanceParams.emplace_back( "off",  2, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_BOOL, "", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "Device initially off", true );
    modelInfos[MODEL_TYPE::BJT].instanceParams.emplace_back( "icvbe",  3, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "V", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "Initial B-E voltage", true );
    modelInfos[MODEL_TYPE::BJT].instanceParams.emplace_back( "icvce",  4, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "V", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "Initial C-E voltage", true );
    modelInfos[MODEL_TYPE::BJT].instanceParams.emplace_back( "m",  9, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::GEOMETRY, "", "", "Parallel Multiplier", true );
    modelInfos[MODEL_TYPE::BJT].instanceParams.emplace_back( "area",  1, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::GEOMETRY, "", "", "(Emitter) Area factor", true );
    modelInfos[MODEL_TYPE::BJT].instanceParams.emplace_back( "areab",  10, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::GEOMETRY, "", "", "Base area factor", true );
    modelInfos[MODEL_TYPE::BJT].instanceParams.emplace_back( "areac",  11, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::GEOMETRY, "", "", "Collector area factor", true );
    modelInfos[MODEL_TYPE::BJT].instanceParams.emplace_back( "ic",  5, SIM_MODEL::PARAM::DIR_IN, SIM_VALUE::TYPE_FLOAT_VECTOR, "", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "Initial condition vector", true );
    modelInfos[MODEL_TYPE::BJT].instanceParams.emplace_back( "sens_area",  6, SIM_MODEL::PARAM::DIR_IN, SIM_VALUE::TYPE_BOOL, "", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "flag to request sensitivity WRT area", true );
    modelInfos[MODEL_TYPE::BJT].instanceParams.emplace_back( "colnode",  212, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_INT, "", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "Number of collector node", true );
    modelInfos[MODEL_TYPE::BJT].instanceParams.emplace_back( "basenode",  213, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_INT, "", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "Number of base node", true );
    modelInfos[MODEL_TYPE::BJT].instanceParams.emplace_back( "emitnode",  214, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_INT, "", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "Number of emitter node", true );
    modelInfos[MODEL_TYPE::BJT].instanceParams.emplace_back( "substnode",  215, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_INT, "", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "Number of substrate node", true );
    modelInfos[MODEL_TYPE::BJT].instanceParams.emplace_back( "colprimenode",  217, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_INT, "", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "Internal collector node", true );
    modelInfos[MODEL_TYPE::BJT].instanceParams.emplace_back( "baseprimenode",  218, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_INT, "", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "Internal base node", true );
    modelInfos[MODEL_TYPE::BJT].instanceParams.emplace_back( "emitprimenode",  219, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_INT, "", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "Internal emitter node", true );
    modelInfos[MODEL_TYPE::BJT].instanceParams.emplace_back( "ic",  222, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_FLOAT, "A", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "Current at collector node", true );
    modelInfos[MODEL_TYPE::BJT].instanceParams.emplace_back( "ib",  223, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_FLOAT, "A", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "Current at base node", true );
    modelInfos[MODEL_TYPE::BJT].instanceParams.emplace_back( "ie",  247, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_FLOAT, "A", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "Emitter current", true );
    modelInfos[MODEL_TYPE::BJT].instanceParams.emplace_back( "is",  248, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_FLOAT, "A", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "1e-16", "1e-16", "Substrate current", true );
    modelInfos[MODEL_TYPE::BJT].instanceParams.emplace_back( "vbe",  220, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_FLOAT, "V", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "B-E voltage", true );
    modelInfos[MODEL_TYPE::BJT].instanceParams.emplace_back( "vbc",  221, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_FLOAT, "V", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "B-C voltage", true );
    modelInfos[MODEL_TYPE::BJT].instanceParams.emplace_back( "gm",  226, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "Small signal transconductance", true );
    modelInfos[MODEL_TYPE::BJT].instanceParams.emplace_back( "gpi",  224, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "Small signal input conductance - pi", true );
    modelInfos[MODEL_TYPE::BJT].instanceParams.emplace_back( "gmu",  225, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "Small signal conductance - mu", true );
    modelInfos[MODEL_TYPE::BJT].instanceParams.emplace_back( "gx",  236, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "Conductance from base to internal base", true );
    modelInfos[MODEL_TYPE::BJT].instanceParams.emplace_back( "go",  227, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "Small signal output conductance", true );
    modelInfos[MODEL_TYPE::BJT].instanceParams.emplace_back( "geqcb",  238, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "d(Ibe)/d(Vbc)", true );
    modelInfos[MODEL_TYPE::BJT].instanceParams.emplace_back( "gcsub",  239, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_FLOAT, "F", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "Internal Subs. cap. equiv. cond.", true );
    modelInfos[MODEL_TYPE::BJT].instanceParams.emplace_back( "gdsub",  254, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "Internal Subs. Diode equiv. cond.", true );
    modelInfos[MODEL_TYPE::BJT].instanceParams.emplace_back( "geqbx",  240, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_FLOAT, "F", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "Internal C-B-base cap. equiv. cond.", true );
    modelInfos[MODEL_TYPE::BJT].instanceParams.emplace_back( "cpi",  250, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_FLOAT, "F", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "Internal base to emitter capacitance", true );
    modelInfos[MODEL_TYPE::BJT].instanceParams.emplace_back( "cmu",  251, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_FLOAT, "F", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "Internal base to collector capacitance", true );
    modelInfos[MODEL_TYPE::BJT].instanceParams.emplace_back( "cbx",  252, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_FLOAT, "F", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "Base to collector capacitance", true );
    modelInfos[MODEL_TYPE::BJT].instanceParams.emplace_back( "csub",  253, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_FLOAT, "F", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "0", "0", "Substrate capacitance", true );
    modelInfos[MODEL_TYPE::BJT].instanceParams.emplace_back( "cqbe",  229, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_FLOAT, "F", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "Cap. due to charge storage in B-E jct.", true );
    modelInfos[MODEL_TYPE::BJT].instanceParams.emplace_back( "cqbc",  231, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_FLOAT, "F", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "Cap. due to charge storage in B-C jct.", true );
    modelInfos[MODEL_TYPE::BJT].instanceParams.emplace_back( "cqsub",  233, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_FLOAT, "F", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "Cap. due to charge storage in Subs. jct.", true );
    modelInfos[MODEL_TYPE::BJT].instanceParams.emplace_back( "cqbx",  235, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_FLOAT, "F", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "Cap. due to charge storage in B-X jct.", true );
    modelInfos[MODEL_TYPE::BJT].instanceParams.emplace_back( "cexbc",  237, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_FLOAT, "F", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "Total Capacitance in B-X junction", true );
    modelInfos[MODEL_TYPE::BJT].instanceParams.emplace_back( "qbe",  228, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "Charge storage B-E junction", true );
    modelInfos[MODEL_TYPE::BJT].instanceParams.emplace_back( "qbc",  230, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "Charge storage B-C junction", true );
    modelInfos[MODEL_TYPE::BJT].instanceParams.emplace_back( "qsub",  232, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "Charge storage Subs. junction", true );
    modelInfos[MODEL_TYPE::BJT].instanceParams.emplace_back( "qbx",  234, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "Charge storage B-X junction", true );
    modelInfos[MODEL_TYPE::BJT].instanceParams.emplace_back( "p",  249, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "Power dissipation", true );
    modelInfos[MODEL_TYPE::BJT].instanceParams.emplace_back( "sens_dc",  246, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "dc sensitivity", true );
    modelInfos[MODEL_TYPE::BJT].instanceParams.emplace_back( "sens_real",  241, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "real part of ac sensitivity", true );
    modelInfos[MODEL_TYPE::BJT].instanceParams.emplace_back( "sens_imag",  242, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "dc sens. & imag part of ac sens.", true );
    modelInfos[MODEL_TYPE::BJT].instanceParams.emplace_back( "sens_mag",  243, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "sensitivity of ac magnitude", true );
    modelInfos[MODEL_TYPE::BJT].instanceParams.emplace_back( "sens_ph",  244, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "sensitivity of ac phase", true );
    modelInfos[MODEL_TYPE::BJT].instanceParams.emplace_back( "sens_cplx",  245, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_COMPLEX, "", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "ac sensitivity", true );
    modelInfos[MODEL_TYPE::BJT].instanceParams.emplace_back( "temp",  7, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "°C", SIM_MODEL::PARAM::CATEGORY::PRINCIPAL, "", "", "instance temperature", true );
    modelInfos[MODEL_TYPE::BJT].instanceParams.emplace_back( "dtemp",  8, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "°C", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "instance temperature delta from circuit", true );
}