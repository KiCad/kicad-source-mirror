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


void NGSPICE_MODEL_INFO_MAP::addVBIC()
{
    modelInfos[MODEL_TYPE::VBIC] = { "VBIC", "NPN", "PNP", { "C", "B", "E", "<S>", "<TJ>" }, "Vertical Bipolar Inter-Company Model", {}, {} };
    // Model parameters
    modelInfos[MODEL_TYPE::VBIC].modelParams.emplace_back( "type",  305, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_STRING, "", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "npn", "pnp", "NPN or PNP" );
    modelInfos[MODEL_TYPE::VBIC].modelParams.emplace_back( "npn",  101, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_BOOL, "", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "NaN", "NaN", "NPN type device" );
    modelInfos[MODEL_TYPE::VBIC].modelParams.emplace_back( "pnp",  102, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_BOOL, "", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "NaN", "NaN", "PNP type device" );
    modelInfos[MODEL_TYPE::VBIC].modelParams.emplace_back( "tnom",  103, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "°C", SIM_MODEL::PARAM::CATEGORY::TEMPERATURE, "27", "27", "Parameter measurement temperature" );
    modelInfos[MODEL_TYPE::VBIC].modelParams.emplace_back( "tref",  103, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "27", "27", "n.a." );
    modelInfos[MODEL_TYPE::VBIC].modelParams.emplace_back( "rcx",  104, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "Ω", SIM_MODEL::PARAM::CATEGORY::DC, "0", "0", "Extrinsic coll resistance" );
    modelInfos[MODEL_TYPE::VBIC].modelParams.emplace_back( "rci",  105, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "Ω", SIM_MODEL::PARAM::CATEGORY::DC, "0.1", "0.1", "Intrinsic coll resistance" );
    modelInfos[MODEL_TYPE::VBIC].modelParams.emplace_back( "vo",  106, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "V", SIM_MODEL::PARAM::CATEGORY::DC, "0", "0", "Epi drift saturation voltage" );
    modelInfos[MODEL_TYPE::VBIC].modelParams.emplace_back( "gamm",  107, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::DC, "0", "0", "Epi doping parameter" );
    modelInfos[MODEL_TYPE::VBIC].modelParams.emplace_back( "hrcf",  108, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "A", SIM_MODEL::PARAM::CATEGORY::DC, "1", "1", "High current RC factor" );
    modelInfos[MODEL_TYPE::VBIC].modelParams.emplace_back( "rbx",  109, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "Ω", SIM_MODEL::PARAM::CATEGORY::DC, "0", "0", "Extrinsic base resistance" );
    modelInfos[MODEL_TYPE::VBIC].modelParams.emplace_back( "rbi",  110, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "Ω", SIM_MODEL::PARAM::CATEGORY::DC, "0.1", "0.1", "Intrinsic base resistance" );
    modelInfos[MODEL_TYPE::VBIC].modelParams.emplace_back( "re",  111, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "Ω", SIM_MODEL::PARAM::CATEGORY::DC, "0", "0", "Intrinsic emitter resistance" );
    modelInfos[MODEL_TYPE::VBIC].modelParams.emplace_back( "rs",  112, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "Ω", SIM_MODEL::PARAM::CATEGORY::DC, "0", "0", "Intrinsic substrate resistance" );
    modelInfos[MODEL_TYPE::VBIC].modelParams.emplace_back( "rbp",  113, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "Ω", SIM_MODEL::PARAM::CATEGORY::DC, "0.1", "0.1", "Parasitic base resistance" );
    modelInfos[MODEL_TYPE::VBIC].modelParams.emplace_back( "is_",  114, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "A", SIM_MODEL::PARAM::CATEGORY::DC, "1e-16", "1e-16", "Transport saturation current" );
    modelInfos[MODEL_TYPE::VBIC].modelParams.emplace_back( "nf",  115, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::DC, "1", "1", "Forward emission coefficient" );
    modelInfos[MODEL_TYPE::VBIC].modelParams.emplace_back( "nr",  116, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::DC, "1", "1", "Reverse emission coefficient" );
    modelInfos[MODEL_TYPE::VBIC].modelParams.emplace_back( "fc",  117, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "F", SIM_MODEL::PARAM::CATEGORY::CAPACITANCE, "0.9", "0.9", "Fwd bias depletion capacitance limit" );
    modelInfos[MODEL_TYPE::VBIC].modelParams.emplace_back( "cbeo",  118, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "F/m", SIM_MODEL::PARAM::CATEGORY::CAPACITANCE, "0", "0", "Extrinsic B-E overlap capacitance" );
    modelInfos[MODEL_TYPE::VBIC].modelParams.emplace_back( "cje",  119, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "F", SIM_MODEL::PARAM::CATEGORY::CAPACITANCE, "0", "0", "Zero bias B-E depletion capacitance" );
    modelInfos[MODEL_TYPE::VBIC].modelParams.emplace_back( "pe",  120, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "V", SIM_MODEL::PARAM::CATEGORY::DC, "0.75", "0.75", "B-E built in potential" );
    modelInfos[MODEL_TYPE::VBIC].modelParams.emplace_back( "me",  121, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::DC, "0.33", "0.33", "B-E junction grading coefficient" );
    modelInfos[MODEL_TYPE::VBIC].modelParams.emplace_back( "aje",  122, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "F", SIM_MODEL::PARAM::CATEGORY::CAPACITANCE, "-0.5", "-0.5", "B-E capacitance smoothing factor" );
    modelInfos[MODEL_TYPE::VBIC].modelParams.emplace_back( "cbco",  123, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "F/m", SIM_MODEL::PARAM::CATEGORY::CAPACITANCE, "0", "0", "Extrinsic B-C overlap capacitance" );
    modelInfos[MODEL_TYPE::VBIC].modelParams.emplace_back( "cjc",  124, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "F", SIM_MODEL::PARAM::CATEGORY::CAPACITANCE, "0", "0", "Zero bias B-C depletion capacitance" );
    modelInfos[MODEL_TYPE::VBIC].modelParams.emplace_back( "qco",  125, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "C", SIM_MODEL::PARAM::CATEGORY::DC, "0", "0", "Epi charge parameter" );
    modelInfos[MODEL_TYPE::VBIC].modelParams.emplace_back( "cjep",  126, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "F", SIM_MODEL::PARAM::CATEGORY::CAPACITANCE, "0", "0", "B-C extrinsic zero bias capacitance" );
    modelInfos[MODEL_TYPE::VBIC].modelParams.emplace_back( "pc",  127, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "V", SIM_MODEL::PARAM::CATEGORY::DC, "0.75", "0.75", "B-C built in potential" );
    modelInfos[MODEL_TYPE::VBIC].modelParams.emplace_back( "mc",  128, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::DC, "0.33", "0.33", "B-C junction grading coefficient" );
    modelInfos[MODEL_TYPE::VBIC].modelParams.emplace_back( "ajc",  129, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "F", SIM_MODEL::PARAM::CATEGORY::CAPACITANCE, "-0.5", "-0.5", "B-C capacitance smoothing factor" );
    modelInfos[MODEL_TYPE::VBIC].modelParams.emplace_back( "cjcp",  130, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "F", SIM_MODEL::PARAM::CATEGORY::CAPACITANCE, "0", "0", "Zero bias S-C capacitance" );
    modelInfos[MODEL_TYPE::VBIC].modelParams.emplace_back( "ps",  131, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "V", SIM_MODEL::PARAM::CATEGORY::DC, "0.75", "0.75", "S-C junction built in potential" );
    modelInfos[MODEL_TYPE::VBIC].modelParams.emplace_back( "ms",  132, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::DC, "0.33", "0.33", "S-C junction grading coefficient" );
    modelInfos[MODEL_TYPE::VBIC].modelParams.emplace_back( "ajs",  133, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "F", SIM_MODEL::PARAM::CATEGORY::CAPACITANCE, "-0.5", "-0.5", "S-C capacitance smoothing factor" );
    modelInfos[MODEL_TYPE::VBIC].modelParams.emplace_back( "ibei",  134, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "A", SIM_MODEL::PARAM::CATEGORY::DC, "1e-18", "1e-18", "Ideal B-E saturation current" );
    modelInfos[MODEL_TYPE::VBIC].modelParams.emplace_back( "wbe",  135, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::DC, "1", "1", "Portion of IBEI from Vbei, 1-WBE from Vbex" );
    modelInfos[MODEL_TYPE::VBIC].modelParams.emplace_back( "nei",  136, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::DC, "1", "1", "Ideal B-E emission coefficient" );
    modelInfos[MODEL_TYPE::VBIC].modelParams.emplace_back( "iben",  137, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "A", SIM_MODEL::PARAM::CATEGORY::DC, "0", "0", "Non-ideal B-E saturation current" );
    modelInfos[MODEL_TYPE::VBIC].modelParams.emplace_back( "nen",  138, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::DC, "2", "2", "Non-ideal B-E emission coefficient" );
    modelInfos[MODEL_TYPE::VBIC].modelParams.emplace_back( "ibci",  139, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "A", SIM_MODEL::PARAM::CATEGORY::DC, "1e-16", "1e-16", "Ideal B-C saturation current" );
    modelInfos[MODEL_TYPE::VBIC].modelParams.emplace_back( "nci",  140, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::DC, "1", "1", "Ideal B-C emission coefficient" );
    modelInfos[MODEL_TYPE::VBIC].modelParams.emplace_back( "ibcn",  141, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "A", SIM_MODEL::PARAM::CATEGORY::DC, "0", "0", "Non-ideal B-C saturation current" );
    modelInfos[MODEL_TYPE::VBIC].modelParams.emplace_back( "ncn",  142, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::DC, "2", "2", "Non-ideal B-C emission coefficient" );
    modelInfos[MODEL_TYPE::VBIC].modelParams.emplace_back( "avc1",  143, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::DC, "0", "0", "B-C weak avalanche parameter 1" );
    modelInfos[MODEL_TYPE::VBIC].modelParams.emplace_back( "avc2",  144, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::DC, "0", "0", "B-C weak avalanche parameter 2" );
    modelInfos[MODEL_TYPE::VBIC].modelParams.emplace_back( "isp",  145, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "A", SIM_MODEL::PARAM::CATEGORY::DC, "0", "0", "Parasitic transport saturation current" );
    modelInfos[MODEL_TYPE::VBIC].modelParams.emplace_back( "wsp",  146, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::DC, "1", "1", "Portion of ICCP" );
    modelInfos[MODEL_TYPE::VBIC].modelParams.emplace_back( "nfp",  147, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::DC, "1", "1", "Parasitic fwd emission coefficient" );
    modelInfos[MODEL_TYPE::VBIC].modelParams.emplace_back( "ibeip",  148, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "A", SIM_MODEL::PARAM::CATEGORY::DC, "0", "0", "Ideal parasitic B-E saturation current" );
    modelInfos[MODEL_TYPE::VBIC].modelParams.emplace_back( "ibenp",  149, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "A", SIM_MODEL::PARAM::CATEGORY::DC, "0", "0", "Non-ideal parasitic B-E saturation current" );
    modelInfos[MODEL_TYPE::VBIC].modelParams.emplace_back( "ibcip",  150, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "A", SIM_MODEL::PARAM::CATEGORY::DC, "0", "0", "Ideal parasitic B-C saturation current" );
    modelInfos[MODEL_TYPE::VBIC].modelParams.emplace_back( "ncip",  151, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::DC, "1", "1", "Ideal parasitic B-C emission coefficient" );
    modelInfos[MODEL_TYPE::VBIC].modelParams.emplace_back( "ibcnp",  152, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "A", SIM_MODEL::PARAM::CATEGORY::DC, "0", "0", "Nonideal parasitic B-C saturation current" );
    modelInfos[MODEL_TYPE::VBIC].modelParams.emplace_back( "ncnp",  153, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::DC, "2", "2", "Nonideal parasitic B-C emission coefficient" );
    modelInfos[MODEL_TYPE::VBIC].modelParams.emplace_back( "vef",  154, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "V", SIM_MODEL::PARAM::CATEGORY::DC, "0", "0", "Forward Early voltage" );
    modelInfos[MODEL_TYPE::VBIC].modelParams.emplace_back( "ver",  155, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "V", SIM_MODEL::PARAM::CATEGORY::DC, "0", "0", "Reverse Early voltage" );
    modelInfos[MODEL_TYPE::VBIC].modelParams.emplace_back( "ikf",  156, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "A", SIM_MODEL::PARAM::CATEGORY::DC, "0", "0", "Forward knee current" );
    modelInfos[MODEL_TYPE::VBIC].modelParams.emplace_back( "ikr",  157, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "A", SIM_MODEL::PARAM::CATEGORY::DC, "0", "0", "Reverse knee current" );
    modelInfos[MODEL_TYPE::VBIC].modelParams.emplace_back( "ikp",  158, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "A", SIM_MODEL::PARAM::CATEGORY::DC, "0", "0", "Parasitic knee current" );
    modelInfos[MODEL_TYPE::VBIC].modelParams.emplace_back( "tf",  159, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "s", SIM_MODEL::PARAM::CATEGORY::DC, "0", "0", "Ideal forward transit time" );
    modelInfos[MODEL_TYPE::VBIC].modelParams.emplace_back( "qtf",  160, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "m", SIM_MODEL::PARAM::CATEGORY::DC, "0", "0", "Variation of TF with base-width modulation" );
    modelInfos[MODEL_TYPE::VBIC].modelParams.emplace_back( "xtf",  161, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::DC, "0", "0", "Coefficient for bias dependence of TF" );
    modelInfos[MODEL_TYPE::VBIC].modelParams.emplace_back( "vtf",  162, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "V", SIM_MODEL::PARAM::CATEGORY::DC, "0", "0", "Voltage giving VBC dependence of TF" );
    modelInfos[MODEL_TYPE::VBIC].modelParams.emplace_back( "itf",  163, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "A", SIM_MODEL::PARAM::CATEGORY::DC, "0", "0", "High current dependence of TF" );
    modelInfos[MODEL_TYPE::VBIC].modelParams.emplace_back( "tr",  164, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "s", SIM_MODEL::PARAM::CATEGORY::DC, "0", "0", "Ideal reverse transit time" );
    modelInfos[MODEL_TYPE::VBIC].modelParams.emplace_back( "td",  165, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "s", SIM_MODEL::PARAM::CATEGORY::DC, "0", "0", "Forward excess-phase delay time" );
    modelInfos[MODEL_TYPE::VBIC].modelParams.emplace_back( "kfn",  166, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::NOISE, "0", "0", "B-E Flicker Noise Coefficient" );
    modelInfos[MODEL_TYPE::VBIC].modelParams.emplace_back( "afn",  167, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::NOISE, "1", "1", "B-E Flicker Noise Exponent" );
    modelInfos[MODEL_TYPE::VBIC].modelParams.emplace_back( "bfn",  168, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::NOISE, "1", "1", "B-E Flicker Noise 1/f dependence" );
    modelInfos[MODEL_TYPE::VBIC].modelParams.emplace_back( "xre",  169, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::TEMPERATURE, "0", "0", "Temperature exponent of RE" );
    modelInfos[MODEL_TYPE::VBIC].modelParams.emplace_back( "xrb",  170, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::TEMPERATURE, "0", "0", "Temperature exponent of RB" );
    modelInfos[MODEL_TYPE::VBIC].modelParams.emplace_back( "xrbi",  171, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::TEMPERATURE, "0", "0", "Temperature exponent of RBI" );
    modelInfos[MODEL_TYPE::VBIC].modelParams.emplace_back( "xrc",  172, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::TEMPERATURE, "0", "0", "Temperature exponent of RC" );
    modelInfos[MODEL_TYPE::VBIC].modelParams.emplace_back( "xrci",  173, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::TEMPERATURE, "0", "0", "Temperature exponent of RCI" );
    modelInfos[MODEL_TYPE::VBIC].modelParams.emplace_back( "xrs",  174, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::TEMPERATURE, "0", "0", "Temperature exponent of RS" );
    modelInfos[MODEL_TYPE::VBIC].modelParams.emplace_back( "xvo",  175, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::TEMPERATURE, "0", "0", "Temperature exponent of VO" );
    modelInfos[MODEL_TYPE::VBIC].modelParams.emplace_back( "ea",  176, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "eV", SIM_MODEL::PARAM::CATEGORY::DC, "1.12", "1.12", "Activation energy for IS" );
    modelInfos[MODEL_TYPE::VBIC].modelParams.emplace_back( "eaie",  177, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "eV", SIM_MODEL::PARAM::CATEGORY::DC, "1.12", "1.12", "Activation energy for IBEI" );
    modelInfos[MODEL_TYPE::VBIC].modelParams.emplace_back( "eaic",  178, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "eV", SIM_MODEL::PARAM::CATEGORY::DC, "1.12", "1.12", "Activation energy for IBCI/IBEIP" );
    modelInfos[MODEL_TYPE::VBIC].modelParams.emplace_back( "eais",  179, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "eV", SIM_MODEL::PARAM::CATEGORY::DC, "1.12", "1.12", "Activation energy for IBCIP" );
    modelInfos[MODEL_TYPE::VBIC].modelParams.emplace_back( "eane",  180, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "eV", SIM_MODEL::PARAM::CATEGORY::DC, "1.12", "1.12", "Activation energy for IBEN" );
    modelInfos[MODEL_TYPE::VBIC].modelParams.emplace_back( "eanc",  181, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "eV", SIM_MODEL::PARAM::CATEGORY::DC, "1.12", "1.12", "Activation energy for IBCN/IBENP" );
    modelInfos[MODEL_TYPE::VBIC].modelParams.emplace_back( "eans",  182, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "eV", SIM_MODEL::PARAM::CATEGORY::DC, "1.12", "1.12", "Activation energy for IBCNP" );
    modelInfos[MODEL_TYPE::VBIC].modelParams.emplace_back( "xis",  183, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::TEMPERATURE, "3", "3", "Temperature exponent of IS" );
    modelInfos[MODEL_TYPE::VBIC].modelParams.emplace_back( "xii",  184, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::TEMPERATURE, "3", "3", "Temperature exponent of IBEI,IBCI,IBEIP,IBCIP" );
    modelInfos[MODEL_TYPE::VBIC].modelParams.emplace_back( "xin",  185, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::TEMPERATURE, "3", "3", "Temperature exponent of IBEN,IBCN,IBENP,IBCNP" );
    modelInfos[MODEL_TYPE::VBIC].modelParams.emplace_back( "tnf",  186, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::TEMPERATURE, "0", "0", "Temperature exponent of NF" );
    modelInfos[MODEL_TYPE::VBIC].modelParams.emplace_back( "tavc",  187, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::TEMPERATURE, "0", "0", "Temperature exponent of AVC2" );
    modelInfos[MODEL_TYPE::VBIC].modelParams.emplace_back( "rth",  188, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "Ω", SIM_MODEL::PARAM::CATEGORY::TEMPERATURE, "0", "0", "Thermal resistance" );
    modelInfos[MODEL_TYPE::VBIC].modelParams.emplace_back( "cth",  189, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::DC, "0", "0", "Thermal capacitance" );
    modelInfos[MODEL_TYPE::VBIC].modelParams.emplace_back( "vrt",  190, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "V", SIM_MODEL::PARAM::CATEGORY::DC, "0", "0", "Punch-through voltage of internal B-C junction" );
    modelInfos[MODEL_TYPE::VBIC].modelParams.emplace_back( "art",  191, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::DC, "0.1", "0.1", "Smoothing parameter for reach-through" );
    modelInfos[MODEL_TYPE::VBIC].modelParams.emplace_back( "ccso",  192, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "F", SIM_MODEL::PARAM::CATEGORY::CAPACITANCE, "0", "0", "Fixed C-S capacitance" );
    modelInfos[MODEL_TYPE::VBIC].modelParams.emplace_back( "qbm",  193, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::DC, "0", "0", "Select SGP qb formulation" );
    modelInfos[MODEL_TYPE::VBIC].modelParams.emplace_back( "nkf",  194, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "A", SIM_MODEL::PARAM::CATEGORY::DC, "0.5", "0.5", "High current beta rolloff" );
    modelInfos[MODEL_TYPE::VBIC].modelParams.emplace_back( "xikf",  195, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::TEMPERATURE, "0", "0", "Temperature exponent of IKF" );
    modelInfos[MODEL_TYPE::VBIC].modelParams.emplace_back( "xrcx",  196, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::TEMPERATURE, "0", "0", "Temperature exponent of RCX" );
    modelInfos[MODEL_TYPE::VBIC].modelParams.emplace_back( "xrbx",  197, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::TEMPERATURE, "0", "0", "Temperature exponent of RBX" );
    modelInfos[MODEL_TYPE::VBIC].modelParams.emplace_back( "xrbp",  198, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::TEMPERATURE, "0", "0", "Temperature exponent of RBP" );
    modelInfos[MODEL_TYPE::VBIC].modelParams.emplace_back( "isrr",  199, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::DC, "1", "1", "Separate IS for fwd and rev" );
    modelInfos[MODEL_TYPE::VBIC].modelParams.emplace_back( "xisr",  200, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::TEMPERATURE, "0", "0", "Temperature exponent of ISR" );
    modelInfos[MODEL_TYPE::VBIC].modelParams.emplace_back( "dear",  201, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "eV", SIM_MODEL::PARAM::CATEGORY::DC, "0", "0", "Delta activation energy for ISRR" );
    modelInfos[MODEL_TYPE::VBIC].modelParams.emplace_back( "eap",  202, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "eV", SIM_MODEL::PARAM::CATEGORY::DC, "1.12", "1.12", "Exitivation energy for ISP" );
    modelInfos[MODEL_TYPE::VBIC].modelParams.emplace_back( "vbbe",  203, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "V", SIM_MODEL::PARAM::CATEGORY::DC, "0", "0", "B-E breakdown voltage" );
    modelInfos[MODEL_TYPE::VBIC].modelParams.emplace_back( "nbbe",  204, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::DC, "1", "1", "B-E breakdown emission coefficient" );
    modelInfos[MODEL_TYPE::VBIC].modelParams.emplace_back( "ibbe",  205, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "A", SIM_MODEL::PARAM::CATEGORY::DC, "1e-06", "1e-06", "B-E breakdown current" );
    modelInfos[MODEL_TYPE::VBIC].modelParams.emplace_back( "tvbbe1",  206, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "°C", SIM_MODEL::PARAM::CATEGORY::TEMPERATURE, "0", "0", "Linear temperature coefficient of VBBE" );
    modelInfos[MODEL_TYPE::VBIC].modelParams.emplace_back( "tvbbe2",  207, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "°C", SIM_MODEL::PARAM::CATEGORY::TEMPERATURE, "0", "0", "Quadratic temperature coefficient of VBBE" );
    modelInfos[MODEL_TYPE::VBIC].modelParams.emplace_back( "tnbbe",  208, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "°C", SIM_MODEL::PARAM::CATEGORY::TEMPERATURE, "0", "0", "Temperature coefficient of NBBE" );
    modelInfos[MODEL_TYPE::VBIC].modelParams.emplace_back( "ebbe",  209, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::DC, "0", "0", "exp(-VBBE/(NBBE*Vtv))" );
    modelInfos[MODEL_TYPE::VBIC].modelParams.emplace_back( "dtemp_",  210, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "°C", SIM_MODEL::PARAM::CATEGORY::TEMPERATURE, "0", "0", "Locale Temperature difference" );
    modelInfos[MODEL_TYPE::VBIC].modelParams.emplace_back( "vers",  211, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::DC, "1.2", "1.2", "Revision Version" );
    modelInfos[MODEL_TYPE::VBIC].modelParams.emplace_back( "vref",  212, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::DC, "0", "0", "Reference Version" );
    modelInfos[MODEL_TYPE::VBIC].modelParams.emplace_back( "vbe_max",  213, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "V", SIM_MODEL::PARAM::CATEGORY::LIMITING_VALUES, "1e+99", "1e+99", "maximum voltage B-E junction" );
    modelInfos[MODEL_TYPE::VBIC].modelParams.emplace_back( "vbc_max",  214, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "V", SIM_MODEL::PARAM::CATEGORY::LIMITING_VALUES, "1e+99", "1e+99", "maximum voltage B-C junction" );
    modelInfos[MODEL_TYPE::VBIC].modelParams.emplace_back( "vce_max",  215, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "V", SIM_MODEL::PARAM::CATEGORY::LIMITING_VALUES, "1e+99", "1e+99", "maximum voltage C-E branch" );
    // Instance parameters
    modelInfos[MODEL_TYPE::VBIC].instanceParams.emplace_back( "m",  8, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::GEOMETRY, "", "", "Multiplier", true );
    modelInfos[MODEL_TYPE::VBIC].instanceParams.emplace_back( "area",  1, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::GEOMETRY, "", "", "Area factor", true );
    modelInfos[MODEL_TYPE::VBIC].instanceParams.emplace_back( "off",  2, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_BOOL, "", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "Device initially off", true );
    modelInfos[MODEL_TYPE::VBIC].instanceParams.emplace_back( "ic",  3, SIM_MODEL::PARAM::DIR_IN, SIM_VALUE::TYPE_FLOAT_VECTOR, "", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "Initial condition vector", true );
    modelInfos[MODEL_TYPE::VBIC].instanceParams.emplace_back( "icvbe",  4, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "V", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "Initial B-E voltage", true );
    modelInfos[MODEL_TYPE::VBIC].instanceParams.emplace_back( "icvce",  5, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "V", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "Initial C-E voltage", true );
    modelInfos[MODEL_TYPE::VBIC].instanceParams.emplace_back( "temp",  6, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "°C", SIM_MODEL::PARAM::CATEGORY::PRINCIPAL, "", "", "Instance temperature", true );
    modelInfos[MODEL_TYPE::VBIC].instanceParams.emplace_back( "dtemp",  7, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "°C", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "0", "0", "Instance delta temperature", true );
    modelInfos[MODEL_TYPE::VBIC].instanceParams.emplace_back( "collnode",  222, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_INT, "", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "Number of collector node", true );
    modelInfos[MODEL_TYPE::VBIC].instanceParams.emplace_back( "basenode",  223, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_INT, "", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "Number of base node", true );
    modelInfos[MODEL_TYPE::VBIC].instanceParams.emplace_back( "emitnode",  224, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_INT, "", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "Number of emitter node", true );
    modelInfos[MODEL_TYPE::VBIC].instanceParams.emplace_back( "subsnode",  225, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_INT, "", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "Number of substrate node", true );
    modelInfos[MODEL_TYPE::VBIC].instanceParams.emplace_back( "collcxnode",  226, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_INT, "", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "Internal collector node", true );
    modelInfos[MODEL_TYPE::VBIC].instanceParams.emplace_back( "collcinode",  227, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_INT, "", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "Internal collector node", true );
    modelInfos[MODEL_TYPE::VBIC].instanceParams.emplace_back( "basebxnode",  228, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_INT, "", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "Internal base node", true );
    modelInfos[MODEL_TYPE::VBIC].instanceParams.emplace_back( "basebinode",  229, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_INT, "", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "Internal base node", true );
    modelInfos[MODEL_TYPE::VBIC].instanceParams.emplace_back( "basebpnode",  230, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_INT, "", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "Internal base node", true );
    modelInfos[MODEL_TYPE::VBIC].instanceParams.emplace_back( "emiteinode",  231, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_INT, "", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "Internal emitter node", true );
    modelInfos[MODEL_TYPE::VBIC].instanceParams.emplace_back( "subssinode",  232, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_INT, "", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "Internal substrate node", true );
    modelInfos[MODEL_TYPE::VBIC].instanceParams.emplace_back( "vbe",  233, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_FLOAT, "V", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "B-E voltage", true );
    modelInfos[MODEL_TYPE::VBIC].instanceParams.emplace_back( "vbc",  234, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_FLOAT, "V", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "B-C voltage", true );
    modelInfos[MODEL_TYPE::VBIC].instanceParams.emplace_back( "ic",  235, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_FLOAT, "A", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "Collector current", true );
    modelInfos[MODEL_TYPE::VBIC].instanceParams.emplace_back( "ib",  236, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_FLOAT, "A", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "Base current", true );
    modelInfos[MODEL_TYPE::VBIC].instanceParams.emplace_back( "ie",  237, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_FLOAT, "A", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "Emitter current", true );
    modelInfos[MODEL_TYPE::VBIC].instanceParams.emplace_back( "is",  238, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_FLOAT, "A", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "1e-16", "1e-16", "Substrate current", true );
    modelInfos[MODEL_TYPE::VBIC].instanceParams.emplace_back( "gm",  239, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "Small signal transconductance dIc/dVbe", true );
    modelInfos[MODEL_TYPE::VBIC].instanceParams.emplace_back( "go",  240, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "Small signal output conductance dIc/dVbc", true );
    modelInfos[MODEL_TYPE::VBIC].instanceParams.emplace_back( "gpi",  241, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "Small signal input conductance dIb/dVbe", true );
    modelInfos[MODEL_TYPE::VBIC].instanceParams.emplace_back( "gmu",  242, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "Small signal conductance dIb/dVbc", true );
    modelInfos[MODEL_TYPE::VBIC].instanceParams.emplace_back( "gx",  243, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "Conductance from base to internal base", true );
    modelInfos[MODEL_TYPE::VBIC].instanceParams.emplace_back( "cbe",  257, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_FLOAT, "F", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "Internal base to emitter capacitance", true );
    modelInfos[MODEL_TYPE::VBIC].instanceParams.emplace_back( "cbex",  258, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_FLOAT, "F", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "External base to emitter capacitance", true );
    modelInfos[MODEL_TYPE::VBIC].instanceParams.emplace_back( "cbc",  259, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_FLOAT, "F", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "Internal base to collector capacitance", true );
    modelInfos[MODEL_TYPE::VBIC].instanceParams.emplace_back( "cbcx",  260, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_FLOAT, "F", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "External Base to collector capacitance", true );
    modelInfos[MODEL_TYPE::VBIC].instanceParams.emplace_back( "cbep",  261, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_FLOAT, "F", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "Parasitic Base to emitter capacitance", true );
    modelInfos[MODEL_TYPE::VBIC].instanceParams.emplace_back( "cbcp",  262, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_FLOAT, "F", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "Parasitic Base to collector capacitance", true );
    modelInfos[MODEL_TYPE::VBIC].instanceParams.emplace_back( "p",  263, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "Power dissipation", true );
    modelInfos[MODEL_TYPE::VBIC].instanceParams.emplace_back( "geqcb",  253, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_FLOAT, "F", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "Internal C-B-base cap. equiv. cond.", true );
    modelInfos[MODEL_TYPE::VBIC].instanceParams.emplace_back( "geqbx",  256, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_FLOAT, "F", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "External C-B-base cap. equiv. cond.", true );
    modelInfos[MODEL_TYPE::VBIC].instanceParams.emplace_back( "qbe",  244, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "Charge storage B-E junction", true );
    modelInfos[MODEL_TYPE::VBIC].instanceParams.emplace_back( "cqbe",  245, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_FLOAT, "F", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "Cap. due to charge storage in B-E jct.", true );
    modelInfos[MODEL_TYPE::VBIC].instanceParams.emplace_back( "qbc",  246, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "Charge storage B-C junction", true );
    modelInfos[MODEL_TYPE::VBIC].instanceParams.emplace_back( "cqbc",  247, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_FLOAT, "F", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "Cap. due to charge storage in B-C jct.", true );
    modelInfos[MODEL_TYPE::VBIC].instanceParams.emplace_back( "qbx",  248, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "Charge storage B-X junction", true );
    modelInfos[MODEL_TYPE::VBIC].instanceParams.emplace_back( "cqbx",  249, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_FLOAT, "F", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "Cap. due to charge storage in B-X jct.", true );
}