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


void NGSPICE_MODEL_INFO_MAP::addDIODE()
{
    modelInfos[MODEL_TYPE::DIODE] = { "Diode", "D", "", { "A", "K" }, "Junction Diode model", {}, {} };
    // Model parameters
    modelInfos[MODEL_TYPE::DIODE].modelParams.emplace_back( "level",  100, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_INT, "", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "1", "", "Diode level selector" );
    modelInfos[MODEL_TYPE::DIODE].modelParams.emplace_back( "is",  101, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "A", SIM_MODEL::PARAM::CATEGORY::DC, "1e-14", "", "Saturation current" );
    modelInfos[MODEL_TYPE::DIODE].modelParams.emplace_back( "js",  101, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "1e-14", "", "n.a." );
    modelInfos[MODEL_TYPE::DIODE].modelParams.emplace_back( "jsw",  118, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "A", SIM_MODEL::PARAM::CATEGORY::DC, "0", "", "Sidewall Saturation current" );
    modelInfos[MODEL_TYPE::DIODE].modelParams.emplace_back( "tnom",  115, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "°C", SIM_MODEL::PARAM::CATEGORY::TEMPERATURE, "27", "", "Parameter measurement temperature" );
    modelInfos[MODEL_TYPE::DIODE].modelParams.emplace_back( "tref",  115, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "°C", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "27", "", "Parameter measurement temperature" );
    modelInfos[MODEL_TYPE::DIODE].modelParams.emplace_back( "rs",  102, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "Ω", SIM_MODEL::PARAM::CATEGORY::DC, "0", "", "Ohmic resistance" );
    modelInfos[MODEL_TYPE::DIODE].modelParams.emplace_back( "trs",  129, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "1/°C", SIM_MODEL::PARAM::CATEGORY::TEMPERATURE, "0", "", "Ohmic resistance 1st order temp. coeff." );
    modelInfos[MODEL_TYPE::DIODE].modelParams.emplace_back( "trs1",  129, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "0", "", "n.a." );
    modelInfos[MODEL_TYPE::DIODE].modelParams.emplace_back( "trs2",  130, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "1/°C²", SIM_MODEL::PARAM::CATEGORY::TEMPERATURE, "0", "", "Ohmic resistance 2nd order temp. coeff." );
    modelInfos[MODEL_TYPE::DIODE].modelParams.emplace_back( "n",  103, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::DC, "1", "", "Emission Coefficient" );
    modelInfos[MODEL_TYPE::DIODE].modelParams.emplace_back( "ns",  141, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::DC, "1", "", "Sidewall emission Coefficient" );
    modelInfos[MODEL_TYPE::DIODE].modelParams.emplace_back( "tt",  104, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "s", SIM_MODEL::PARAM::CATEGORY::DC, "0", "", "Transit Time" );
    modelInfos[MODEL_TYPE::DIODE].modelParams.emplace_back( "ttt1",  125, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "1/°C", SIM_MODEL::PARAM::CATEGORY::TEMPERATURE, "0", "", "Transit Time 1st order temp. coeff." );
    modelInfos[MODEL_TYPE::DIODE].modelParams.emplace_back( "ttt2",  126, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "1/°C²", SIM_MODEL::PARAM::CATEGORY::TEMPERATURE, "0", "", "Transit Time 2nd order temp. coeff." );
    modelInfos[MODEL_TYPE::DIODE].modelParams.emplace_back( "cjo",  105, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "F", SIM_MODEL::PARAM::CATEGORY::CAPACITANCE, "0", "", "Junction capacitance" );
    modelInfos[MODEL_TYPE::DIODE].modelParams.emplace_back( "cj0",  105, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "F", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "0", "", "Junction capacitance" );
    modelInfos[MODEL_TYPE::DIODE].modelParams.emplace_back( "cj",  105, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "F", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "0", "", "Junction capacitance" );
    modelInfos[MODEL_TYPE::DIODE].modelParams.emplace_back( "vj",  106, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "V", SIM_MODEL::PARAM::CATEGORY::DC, "1", "", "Junction potential" );
    modelInfos[MODEL_TYPE::DIODE].modelParams.emplace_back( "pb",  106, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "1", "", "n.a." );
    modelInfos[MODEL_TYPE::DIODE].modelParams.emplace_back( "m_",  107, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::DC, "0.5", "", "Grading coefficient" );
    modelInfos[MODEL_TYPE::DIODE].modelParams.emplace_back( "mj",  107, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "0.5", "", "n.a." );
    modelInfos[MODEL_TYPE::DIODE].modelParams.emplace_back( "tm1",  127, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "1/°C²", SIM_MODEL::PARAM::CATEGORY::TEMPERATURE, "0", "", "Grading coefficient 1st temp. coeff." );
    modelInfos[MODEL_TYPE::DIODE].modelParams.emplace_back( "tm2",  128, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "1/°C", SIM_MODEL::PARAM::CATEGORY::TEMPERATURE, "0", "", "Grading coefficient 2nd temp. coeff." );
    modelInfos[MODEL_TYPE::DIODE].modelParams.emplace_back( "cjp",  119, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "F", SIM_MODEL::PARAM::CATEGORY::CAPACITANCE, "0", "", "Sidewall junction capacitance" );
    modelInfos[MODEL_TYPE::DIODE].modelParams.emplace_back( "cjsw",  119, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "0", "", "n.a." );
    modelInfos[MODEL_TYPE::DIODE].modelParams.emplace_back( "php",  120, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "V", SIM_MODEL::PARAM::CATEGORY::DC, "1", "", "Sidewall junction potential" );
    modelInfos[MODEL_TYPE::DIODE].modelParams.emplace_back( "mjsw",  121, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::DC, "0.33", "", "Sidewall Grading coefficient" );
    modelInfos[MODEL_TYPE::DIODE].modelParams.emplace_back( "ikf",  122, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "A", SIM_MODEL::PARAM::CATEGORY::DC, "0", "", "Forward Knee current" );
    modelInfos[MODEL_TYPE::DIODE].modelParams.emplace_back( "ik",  122, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "0", "", "n.a." );
    modelInfos[MODEL_TYPE::DIODE].modelParams.emplace_back( "ikr",  123, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "A", SIM_MODEL::PARAM::CATEGORY::DC, "0", "", "Reverse Knee current" );
    modelInfos[MODEL_TYPE::DIODE].modelParams.emplace_back( "nbv",  138, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::DC, "1", "", "Breakdown Emission Coefficient" );
    modelInfos[MODEL_TYPE::DIODE].modelParams.emplace_back( "area_",  139, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::DC, "1", "", "Area factor" );
    modelInfos[MODEL_TYPE::DIODE].modelParams.emplace_back( "pj_",  140, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::DC, "0", "", "Perimeter factor" );
    modelInfos[MODEL_TYPE::DIODE].modelParams.emplace_back( "tlev",  131, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_INT, "", SIM_MODEL::PARAM::CATEGORY::TEMPERATURE, "0", "", "Diode temperature equation selector" );
    modelInfos[MODEL_TYPE::DIODE].modelParams.emplace_back( "tlevc",  132, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_INT, "", SIM_MODEL::PARAM::CATEGORY::TEMPERATURE, "0", "", "Diode temperature equation selector" );
    modelInfos[MODEL_TYPE::DIODE].modelParams.emplace_back( "eg",  108, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "eV", SIM_MODEL::PARAM::CATEGORY::DC, "1.11", "", "Activation energy" );
    modelInfos[MODEL_TYPE::DIODE].modelParams.emplace_back( "xti",  109, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "A", SIM_MODEL::PARAM::CATEGORY::TEMPERATURE, "3", "", "Saturation current temperature exp." );
    modelInfos[MODEL_TYPE::DIODE].modelParams.emplace_back( "cta",  133, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "1/°C", SIM_MODEL::PARAM::CATEGORY::TEMPERATURE, "0", "", "Area junction temperature coefficient" );
    modelInfos[MODEL_TYPE::DIODE].modelParams.emplace_back( "ctc",  133, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "0", "", "n.a." );
    modelInfos[MODEL_TYPE::DIODE].modelParams.emplace_back( "ctp",  134, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "1/°C", SIM_MODEL::PARAM::CATEGORY::TEMPERATURE, "0", "", "Perimeter junction capacitance temperature coefficient" );
    modelInfos[MODEL_TYPE::DIODE].modelParams.emplace_back( "tpb",  135, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "1/°C", SIM_MODEL::PARAM::CATEGORY::TEMPERATURE, "0", "", "Area junction potential temperature coefficient" );
    modelInfos[MODEL_TYPE::DIODE].modelParams.emplace_back( "tvj",  135, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "0", "", "n.a." );
    modelInfos[MODEL_TYPE::DIODE].modelParams.emplace_back( "tphp",  136, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "1/°C", SIM_MODEL::PARAM::CATEGORY::TEMPERATURE, "0", "", "Perimeter junction potential temperature coefficient" );
    modelInfos[MODEL_TYPE::DIODE].modelParams.emplace_back( "jtun",  142, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "A", SIM_MODEL::PARAM::CATEGORY::DC, "0", "", "Tunneling saturation current" );
    modelInfos[MODEL_TYPE::DIODE].modelParams.emplace_back( "jtunsw",  143, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "A", SIM_MODEL::PARAM::CATEGORY::DC, "0", "", "Tunneling sidewall saturation current" );
    modelInfos[MODEL_TYPE::DIODE].modelParams.emplace_back( "ntun",  144, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::DC, "30", "", "Tunneling emission coefficient" );
    modelInfos[MODEL_TYPE::DIODE].modelParams.emplace_back( "xtitun",  145, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::DC, "3", "", "Tunneling saturation current exponential" );
    modelInfos[MODEL_TYPE::DIODE].modelParams.emplace_back( "keg",  146, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::DC, "1", "", "EG correction factor for tunneling" );
    modelInfos[MODEL_TYPE::DIODE].modelParams.emplace_back( "kf",  116, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::NOISE, "0", "", "flicker noise coefficient" );
    modelInfos[MODEL_TYPE::DIODE].modelParams.emplace_back( "af",  117, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::NOISE, "1", "", "flicker noise exponent" );
    modelInfos[MODEL_TYPE::DIODE].modelParams.emplace_back( "fc",  110, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::DC, "0.5", "", "Forward bias junction fit parameter" );
    modelInfos[MODEL_TYPE::DIODE].modelParams.emplace_back( "fcs",  124, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::DC, "0.5", "", "Forward bias sidewall junction fit parameter" );
    modelInfos[MODEL_TYPE::DIODE].modelParams.emplace_back( "bv",  111, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "V", SIM_MODEL::PARAM::CATEGORY::DC, "1e+99", "", "Reverse breakdown voltage" );
    modelInfos[MODEL_TYPE::DIODE].modelParams.emplace_back( "ibv",  112, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "A", SIM_MODEL::PARAM::CATEGORY::DC, "0.001", "", "Current at reverse breakdown voltage" );
    modelInfos[MODEL_TYPE::DIODE].modelParams.emplace_back( "ib",  112, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "0.001", "", "n.a." );
    modelInfos[MODEL_TYPE::DIODE].modelParams.emplace_back( "tcv",  137, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "1/°C", SIM_MODEL::PARAM::CATEGORY::TEMPERATURE, "0", "", "Reverse breakdown voltage temperature coefficient" );
    modelInfos[MODEL_TYPE::DIODE].modelParams.emplace_back( "cond",  114, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::DC, "0", "", "Ohmic conductance" );
    modelInfos[MODEL_TYPE::DIODE].modelParams.emplace_back( "isr",  152, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "A", SIM_MODEL::PARAM::CATEGORY::DC, "1e-14", "", "Recombination saturation current" );
    modelInfos[MODEL_TYPE::DIODE].modelParams.emplace_back( "nr",  153, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::DC, "1", "", "Recombination current emission coefficient" );
    modelInfos[MODEL_TYPE::DIODE].modelParams.emplace_back( "fv_max",  147, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "V", SIM_MODEL::PARAM::CATEGORY::LIMITING_VALUES, "1e+99", "", "maximum voltage in forward direction" );
    modelInfos[MODEL_TYPE::DIODE].modelParams.emplace_back( "bv_max",  148, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "V", SIM_MODEL::PARAM::CATEGORY::LIMITING_VALUES, "1e+99", "", "maximum voltage in reverse direction" );
    modelInfos[MODEL_TYPE::DIODE].modelParams.emplace_back( "id_max",  149, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "A", SIM_MODEL::PARAM::CATEGORY::LIMITING_VALUES, "1e+99", "", "maximum current" );
    modelInfos[MODEL_TYPE::DIODE].modelParams.emplace_back( "te_max",  150, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "°C", SIM_MODEL::PARAM::CATEGORY::LIMITING_VALUES, "1e+99", "", "temperature" );
    modelInfos[MODEL_TYPE::DIODE].modelParams.emplace_back( "pd_max",  151, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::LIMITING_VALUES, "1e+99", "", "maximum power dissipation" );
    modelInfos[MODEL_TYPE::DIODE].modelParams.emplace_back( "rth0",  154, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "Ω", SIM_MODEL::PARAM::CATEGORY::TEMPERATURE, "0", "", "Self-heating thermal resistance" );
    modelInfos[MODEL_TYPE::DIODE].modelParams.emplace_back( "cth0",  155, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "F", SIM_MODEL::PARAM::CATEGORY::DC, "1e-05", "", "Self-heating thermal capacitance" );
    modelInfos[MODEL_TYPE::DIODE].modelParams.emplace_back( "lm_",  156, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "m", SIM_MODEL::PARAM::CATEGORY::DC, "0", "", "Length of metal capacitor (level=3)" );
    modelInfos[MODEL_TYPE::DIODE].modelParams.emplace_back( "lp_",  157, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "m", SIM_MODEL::PARAM::CATEGORY::DC, "0", "", "Length of polysilicon capacitor (level=3)" );
    modelInfos[MODEL_TYPE::DIODE].modelParams.emplace_back( "wm_",  158, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "m", SIM_MODEL::PARAM::CATEGORY::DC, "0", "", "Width of metal capacitor (level=3)" );
    modelInfos[MODEL_TYPE::DIODE].modelParams.emplace_back( "wp_",  159, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "m", SIM_MODEL::PARAM::CATEGORY::DC, "0", "", "Width of polysilicon capacitor (level=3)" );
    modelInfos[MODEL_TYPE::DIODE].modelParams.emplace_back( "xom",  160, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "m", SIM_MODEL::PARAM::CATEGORY::DC, "1e-06", "", "Thickness of the metal to bulk oxide (level=3)" );
    modelInfos[MODEL_TYPE::DIODE].modelParams.emplace_back( "xoi",  161, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "m", SIM_MODEL::PARAM::CATEGORY::DC, "1e-06", "", "Thickness of the polysilicon to bulk oxide (level=3)" );
    modelInfos[MODEL_TYPE::DIODE].modelParams.emplace_back( "xm",  162, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::DC, "0", "", "Masking and etching effects in metal (level=3)" );
    modelInfos[MODEL_TYPE::DIODE].modelParams.emplace_back( "xp",  163, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::DC, "0", "", "Masking and etching effects in polysilicon (level=3)" );
    modelInfos[MODEL_TYPE::DIODE].modelParams.emplace_back( "d",  113, SIM_MODEL::PARAM::DIR_IN, SIM_VALUE::TYPE_BOOL, "", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "Diode model" );
    // Instance parameters
    modelInfos[MODEL_TYPE::DIODE].instanceParams.emplace_back( "off",  3, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_BOOL, "", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "Initially off", true );
    modelInfos[MODEL_TYPE::DIODE].instanceParams.emplace_back( "temp",  11, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "°C", SIM_MODEL::PARAM::CATEGORY::PRINCIPAL, "", "", "Instance temperature", true );
    modelInfos[MODEL_TYPE::DIODE].instanceParams.emplace_back( "dtemp",  23, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "°C", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "Instance delta temperature", true );
    modelInfos[MODEL_TYPE::DIODE].instanceParams.emplace_back( "ic",  2, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "V", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "Initial device voltage", true );
    modelInfos[MODEL_TYPE::DIODE].instanceParams.emplace_back( "m",  22, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::GEOMETRY, "0.5", "", "Multiplier", true );
    modelInfos[MODEL_TYPE::DIODE].instanceParams.emplace_back( "area",  1, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::GEOMETRY, "1", "", "Area factor", true );
    modelInfos[MODEL_TYPE::DIODE].instanceParams.emplace_back( "pj",  19, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::GEOMETRY, "0", "", "Perimeter factor", true );
    modelInfos[MODEL_TYPE::DIODE].instanceParams.emplace_back( "w",  20, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "m", SIM_MODEL::PARAM::CATEGORY::GEOMETRY, "", "", "Diode width", true );
    modelInfos[MODEL_TYPE::DIODE].instanceParams.emplace_back( "l",  21, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "m", SIM_MODEL::PARAM::CATEGORY::GEOMETRY, "", "", "Diode length", true );
    modelInfos[MODEL_TYPE::DIODE].instanceParams.emplace_back( "lm",  25, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "m", SIM_MODEL::PARAM::CATEGORY::GEOMETRY, "0", "", "Length of metal capacitor (level=3)", true );
    modelInfos[MODEL_TYPE::DIODE].instanceParams.emplace_back( "lp",  26, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "m", SIM_MODEL::PARAM::CATEGORY::GEOMETRY, "0", "", "Length of polysilicon capacitor (level=3)", true );
    modelInfos[MODEL_TYPE::DIODE].instanceParams.emplace_back( "wm",  27, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "m", SIM_MODEL::PARAM::CATEGORY::GEOMETRY, "0", "", "Width of metal capacitor (level=3)", true );
    modelInfos[MODEL_TYPE::DIODE].instanceParams.emplace_back( "wp",  28, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "m", SIM_MODEL::PARAM::CATEGORY::GEOMETRY, "0", "", "Width of polysilicon capacitor (level=3)", true );
    modelInfos[MODEL_TYPE::DIODE].instanceParams.emplace_back( "thermal",  24, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_BOOL, "", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "Self heating mode selector", true );
    modelInfos[MODEL_TYPE::DIODE].instanceParams.emplace_back( "sens_area",  9, SIM_MODEL::PARAM::DIR_IN, SIM_VALUE::TYPE_BOOL, "", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "flag to request sensitivity WRT area", true );
    modelInfos[MODEL_TYPE::DIODE].instanceParams.emplace_back( "vd",  5, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_FLOAT, "V", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "Diode voltage", true );
    modelInfos[MODEL_TYPE::DIODE].instanceParams.emplace_back( "id",  4, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_FLOAT, "A", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "Diode current", true );
    modelInfos[MODEL_TYPE::DIODE].instanceParams.emplace_back( "c",  4, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_FLOAT, "A", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "Diode current", true );
    modelInfos[MODEL_TYPE::DIODE].instanceParams.emplace_back( "gd",  8, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "Diode conductance", true );
    modelInfos[MODEL_TYPE::DIODE].instanceParams.emplace_back( "cd",  18, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_FLOAT, "F", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "Diode capacitance", true );
    modelInfos[MODEL_TYPE::DIODE].instanceParams.emplace_back( "charge",  6, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "Diode capacitor charge", true );
    modelInfos[MODEL_TYPE::DIODE].instanceParams.emplace_back( "qd",  6, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "Diode capacitor charge", true );
    modelInfos[MODEL_TYPE::DIODE].instanceParams.emplace_back( "capcur",  7, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_FLOAT, "A", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "Diode capacitor current", true );
    modelInfos[MODEL_TYPE::DIODE].instanceParams.emplace_back( "p",  10, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "Diode power", true );
    modelInfos[MODEL_TYPE::DIODE].instanceParams.emplace_back( "sens_dc",  17, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "dc sensitivity", true );
    modelInfos[MODEL_TYPE::DIODE].instanceParams.emplace_back( "sens_real",  12, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "dc sens. and real part of ac sensitivity", true );
    modelInfos[MODEL_TYPE::DIODE].instanceParams.emplace_back( "sens_imag",  13, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "imag part of ac sensitivity", true );
    modelInfos[MODEL_TYPE::DIODE].instanceParams.emplace_back( "sens_mag",  14, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "sensitivity of ac magnitude", true );
    modelInfos[MODEL_TYPE::DIODE].instanceParams.emplace_back( "sens_ph",  15, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "sensitivity of ac phase", true );
    modelInfos[MODEL_TYPE::DIODE].instanceParams.emplace_back( "sens_cplx",  16, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_COMPLEX, "", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "ac sensitivity", true );
}