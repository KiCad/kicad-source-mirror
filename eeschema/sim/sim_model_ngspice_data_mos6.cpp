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


void NGSPICE_MODEL_INFO_MAP::addMOS6()
{
    modelInfos[MODEL_TYPE::MOS6] = { "Mos6", "NMOS", "PMOS", { "D", "G", "S", "B" }, "Level 6 MOSfet model with Meyer capacitance model", {}, {} };
    // Model parameters
    modelInfos[MODEL_TYPE::MOS6].modelParams.emplace_back( "type",  140, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_STRING, "", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "nmos", "pmos", "N-channel or P-channel MOS" );
    modelInfos[MODEL_TYPE::MOS6].modelParams.emplace_back( "vto",  101, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "V", SIM_MODEL::PARAM::CATEGORY::DC, "0", "0", "Threshold voltage" );
    modelInfos[MODEL_TYPE::MOS6].modelParams.emplace_back( "vt0",  101, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "0", "0", "n.a." );
    modelInfos[MODEL_TYPE::MOS6].modelParams.emplace_back( "kv",  102, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "V", SIM_MODEL::PARAM::CATEGORY::DC, "2", "2", "Saturation voltage factor" );
    modelInfos[MODEL_TYPE::MOS6].modelParams.emplace_back( "nv",  103, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "V", SIM_MODEL::PARAM::CATEGORY::DC, "0.5", "0.5", "Saturation voltage coeff." );
    modelInfos[MODEL_TYPE::MOS6].modelParams.emplace_back( "kc",  104, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "A", SIM_MODEL::PARAM::CATEGORY::DC, "5e-05", "5e-05", "Saturation current factor" );
    modelInfos[MODEL_TYPE::MOS6].modelParams.emplace_back( "nc",  105, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "A", SIM_MODEL::PARAM::CATEGORY::DC, "1", "1", "Saturation current coeff." );
    modelInfos[MODEL_TYPE::MOS6].modelParams.emplace_back( "nvth",  106, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "V", SIM_MODEL::PARAM::CATEGORY::DC, "0.5", "0.5", "Threshold voltage coeff." );
    modelInfos[MODEL_TYPE::MOS6].modelParams.emplace_back( "ps_",  107, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "A", SIM_MODEL::PARAM::CATEGORY::DC, "0", "0", "Sat. current modification  par." );
    modelInfos[MODEL_TYPE::MOS6].modelParams.emplace_back( "gamma",  108, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "sqrt V", SIM_MODEL::PARAM::CATEGORY::DC, "0", "0", "Bulk threshold parameter" );
    modelInfos[MODEL_TYPE::MOS6].modelParams.emplace_back( "gamma1",  109, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "sqrt V", SIM_MODEL::PARAM::CATEGORY::DC, "0", "0", "Bulk threshold parameter 1" );
    modelInfos[MODEL_TYPE::MOS6].modelParams.emplace_back( "sigma",  110, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::DC, "0", "0", "static const feedback effect par." );
    modelInfos[MODEL_TYPE::MOS6].modelParams.emplace_back( "phi",  111, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "V", SIM_MODEL::PARAM::CATEGORY::DC, "0.6", "0.6", "Surface potential" );
    modelInfos[MODEL_TYPE::MOS6].modelParams.emplace_back( "lambda",  112, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "1/V", SIM_MODEL::PARAM::CATEGORY::DC, "0", "0", "Channel length modulation param." );
    modelInfos[MODEL_TYPE::MOS6].modelParams.emplace_back( "lambda0",  113, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "1/V", SIM_MODEL::PARAM::CATEGORY::DC, "0", "0", "Channel length modulation param. 0" );
    modelInfos[MODEL_TYPE::MOS6].modelParams.emplace_back( "lambda1",  114, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "1/V", SIM_MODEL::PARAM::CATEGORY::DC, "0", "0", "Channel length modulation param. 1" );
    modelInfos[MODEL_TYPE::MOS6].modelParams.emplace_back( "rd_",  115, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "Ω", SIM_MODEL::PARAM::CATEGORY::DC, "0", "0", "Drain ohmic resistance" );
    modelInfos[MODEL_TYPE::MOS6].modelParams.emplace_back( "rs_",  116, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "Ω", SIM_MODEL::PARAM::CATEGORY::DC, "0", "0", "Source ohmic resistance" );
    modelInfos[MODEL_TYPE::MOS6].modelParams.emplace_back( "cbd_",  117, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "F", SIM_MODEL::PARAM::CATEGORY::CAPACITANCE, "0", "0", "B-D junction capacitance" );
    modelInfos[MODEL_TYPE::MOS6].modelParams.emplace_back( "cbs_",  118, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "F", SIM_MODEL::PARAM::CATEGORY::CAPACITANCE, "0", "0", "B-S junction capacitance" );
    modelInfos[MODEL_TYPE::MOS6].modelParams.emplace_back( "is_",  119, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "A", SIM_MODEL::PARAM::CATEGORY::DC, "1e-14", "1e-14", "Bulk junction sat. current" );
    modelInfos[MODEL_TYPE::MOS6].modelParams.emplace_back( "pb",  120, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "V", SIM_MODEL::PARAM::CATEGORY::DC, "0.8", "0.8", "Bulk junction potential" );
    modelInfos[MODEL_TYPE::MOS6].modelParams.emplace_back( "cgso",  121, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "F/m", SIM_MODEL::PARAM::CATEGORY::DC, "0", "0", "Gate-source overlap cap." );
    modelInfos[MODEL_TYPE::MOS6].modelParams.emplace_back( "cgdo",  122, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "F/m", SIM_MODEL::PARAM::CATEGORY::DC, "0", "0", "Gate-drain overlap cap." );
    modelInfos[MODEL_TYPE::MOS6].modelParams.emplace_back( "cgbo",  123, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "F/m", SIM_MODEL::PARAM::CATEGORY::DC, "0", "0", "Gate-bulk overlap cap." );
    modelInfos[MODEL_TYPE::MOS6].modelParams.emplace_back( "rsh",  131, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "Ω/m", SIM_MODEL::PARAM::CATEGORY::DC, "0", "0", "Sheet resistance" );
    modelInfos[MODEL_TYPE::MOS6].modelParams.emplace_back( "cj",  124, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "F/m²", SIM_MODEL::PARAM::CATEGORY::DC, "0", "0", "Bottom junction cap per area" );
    modelInfos[MODEL_TYPE::MOS6].modelParams.emplace_back( "mj",  125, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::DC, "0.5", "0.5", "Bottom grading coefficient" );
    modelInfos[MODEL_TYPE::MOS6].modelParams.emplace_back( "cjsw",  126, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "F/m²", SIM_MODEL::PARAM::CATEGORY::DC, "0", "0", "Side junction cap per area" );
    modelInfos[MODEL_TYPE::MOS6].modelParams.emplace_back( "mjsw",  127, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::DC, "0.5", "0.5", "Side grading coefficient" );
    modelInfos[MODEL_TYPE::MOS6].modelParams.emplace_back( "js",  128, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "A/m²", SIM_MODEL::PARAM::CATEGORY::DC, "0", "0", "Bulk jct. sat. current density" );
    modelInfos[MODEL_TYPE::MOS6].modelParams.emplace_back( "ld",  130, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::DC, "0", "0", "Lateral diffusion" );
    modelInfos[MODEL_TYPE::MOS6].modelParams.emplace_back( "tox",  129, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "m", SIM_MODEL::PARAM::CATEGORY::DC, "0", "0", "Oxide thickness" );
    modelInfos[MODEL_TYPE::MOS6].modelParams.emplace_back( "u0",  132, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "cm²/V s", SIM_MODEL::PARAM::CATEGORY::DC, "0", "0", "Surface mobility" );
    modelInfos[MODEL_TYPE::MOS6].modelParams.emplace_back( "uo",  132, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "0", "0", "n.a." );
    modelInfos[MODEL_TYPE::MOS6].modelParams.emplace_back( "fc",  133, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::DC, "0.5", "0.5", "Forward bias jct. fit parm." );
    modelInfos[MODEL_TYPE::MOS6].modelParams.emplace_back( "nmos",  137, SIM_MODEL::PARAM::DIR_IN, SIM_VALUE::TYPE_BOOL, "", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "N type MOSfet model" );
    modelInfos[MODEL_TYPE::MOS6].modelParams.emplace_back( "pmos",  138, SIM_MODEL::PARAM::DIR_IN, SIM_VALUE::TYPE_BOOL, "", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "P type MOSfet model" );
    modelInfos[MODEL_TYPE::MOS6].modelParams.emplace_back( "tpg",  135, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_INT, "", SIM_MODEL::PARAM::CATEGORY::DC, "0", "0", "Gate type" );
    modelInfos[MODEL_TYPE::MOS6].modelParams.emplace_back( "nsub",  134, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "1/cm³", SIM_MODEL::PARAM::CATEGORY::DC, "0", "0", "Substrate doping" );
    modelInfos[MODEL_TYPE::MOS6].modelParams.emplace_back( "nss",  136, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "1/cm²", SIM_MODEL::PARAM::CATEGORY::DC, "0", "0", "Surface state density" );
    modelInfos[MODEL_TYPE::MOS6].modelParams.emplace_back( "tnom",  139, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "°C", SIM_MODEL::PARAM::CATEGORY::TEMPERATURE, "300.15", "300.15", "Parameter measurement temperature" );
    // Instance parameters
    modelInfos[MODEL_TYPE::MOS6].instanceParams.emplace_back( "l",  2, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "m", SIM_MODEL::PARAM::CATEGORY::GEOMETRY, "", "", "Length", true );
    modelInfos[MODEL_TYPE::MOS6].instanceParams.emplace_back( "w",  1, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "m", SIM_MODEL::PARAM::CATEGORY::GEOMETRY, "", "", "Width", true );
    modelInfos[MODEL_TYPE::MOS6].instanceParams.emplace_back( "m",  22, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::GEOMETRY, "", "", "Parallel Multiplier", true );
    modelInfos[MODEL_TYPE::MOS6].instanceParams.emplace_back( "ad",  4, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::GEOMETRY, "", "", "Drain area", true );
    modelInfos[MODEL_TYPE::MOS6].instanceParams.emplace_back( "as",  3, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::GEOMETRY, "", "", "Source area", true );
    modelInfos[MODEL_TYPE::MOS6].instanceParams.emplace_back( "pd",  6, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::GEOMETRY, "", "", "Drain perimeter", true );
    modelInfos[MODEL_TYPE::MOS6].instanceParams.emplace_back( "ps",  5, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::GEOMETRY, "0", "0", "Source perimeter", true );
    modelInfos[MODEL_TYPE::MOS6].instanceParams.emplace_back( "id",  215, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_FLOAT, "A", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "Drain current", true );
    modelInfos[MODEL_TYPE::MOS6].instanceParams.emplace_back( "cd",  215, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_FLOAT, "A", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "Drain current", true );
    modelInfos[MODEL_TYPE::MOS6].instanceParams.emplace_back( "is",  18, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_FLOAT, "A", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "1e-14", "1e-14", "Source current", true );
    modelInfos[MODEL_TYPE::MOS6].instanceParams.emplace_back( "ig",  17, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_FLOAT, "A", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "Gate current", true );
    modelInfos[MODEL_TYPE::MOS6].instanceParams.emplace_back( "ib",  16, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_FLOAT, "A", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "Bulk current", true );
    modelInfos[MODEL_TYPE::MOS6].instanceParams.emplace_back( "ibs",  216, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_FLOAT, "F", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "B-S junction capacitance", true );
    modelInfos[MODEL_TYPE::MOS6].instanceParams.emplace_back( "ibd",  217, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_FLOAT, "F", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "B-D junction capacitance", true );
    modelInfos[MODEL_TYPE::MOS6].instanceParams.emplace_back( "vgs",  231, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_FLOAT, "V", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "Gate-Source voltage", true );
    modelInfos[MODEL_TYPE::MOS6].instanceParams.emplace_back( "vds",  232, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_FLOAT, "V", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "Drain-Source voltage", true );
    modelInfos[MODEL_TYPE::MOS6].instanceParams.emplace_back( "vbs",  230, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_FLOAT, "V", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "Bulk-Source voltage", true );
    modelInfos[MODEL_TYPE::MOS6].instanceParams.emplace_back( "vbd",  229, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_FLOAT, "V", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "Bulk-Drain voltage", true );
    modelInfos[MODEL_TYPE::MOS6].instanceParams.emplace_back( "nrd",  8, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::GEOMETRY, "", "", "Drain squares", true );
    modelInfos[MODEL_TYPE::MOS6].instanceParams.emplace_back( "nrs",  7, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::GEOMETRY, "", "", "Source squares", true );
    modelInfos[MODEL_TYPE::MOS6].instanceParams.emplace_back( "off",  9, SIM_MODEL::PARAM::DIR_IN, SIM_VALUE::TYPE_BOOL, "", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "Device initially off", true );
    modelInfos[MODEL_TYPE::MOS6].instanceParams.emplace_back( "icvds",  12, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "V", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "Initial D-S voltage", true );
    modelInfos[MODEL_TYPE::MOS6].instanceParams.emplace_back( "icvgs",  13, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "V", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "Initial G-S voltage", true );
    modelInfos[MODEL_TYPE::MOS6].instanceParams.emplace_back( "icvbs",  11, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "V", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "Initial B-S voltage", true );
    modelInfos[MODEL_TYPE::MOS6].instanceParams.emplace_back( "temp",  20, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "°C", SIM_MODEL::PARAM::CATEGORY::PRINCIPAL, "", "", "Instance temperature", true );
    modelInfos[MODEL_TYPE::MOS6].instanceParams.emplace_back( "dtemp",  21, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "°C", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "Instance temperature difference", true );
    modelInfos[MODEL_TYPE::MOS6].instanceParams.emplace_back( "ic",  10, SIM_MODEL::PARAM::DIR_IN, SIM_VALUE::TYPE_FLOAT_VECTOR, "", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "Vector of D-S, G-S, B-S voltages", true );
    modelInfos[MODEL_TYPE::MOS6].instanceParams.emplace_back( "sens_l",  15, SIM_MODEL::PARAM::DIR_IN, SIM_VALUE::TYPE_BOOL, "", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "flag to request sensitivity WRT length", true );
    modelInfos[MODEL_TYPE::MOS6].instanceParams.emplace_back( "sens_w",  14, SIM_MODEL::PARAM::DIR_IN, SIM_VALUE::TYPE_BOOL, "", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "flag to request sensitivity WRT width", true );
    modelInfos[MODEL_TYPE::MOS6].instanceParams.emplace_back( "dnode",  203, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_INT, "", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "Number of the drain node", true );
    modelInfos[MODEL_TYPE::MOS6].instanceParams.emplace_back( "gnode",  204, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_INT, "", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "Number of the gate node", true );
    modelInfos[MODEL_TYPE::MOS6].instanceParams.emplace_back( "snode",  205, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_INT, "", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "Number of the source node", true );
    modelInfos[MODEL_TYPE::MOS6].instanceParams.emplace_back( "bnode",  206, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_INT, "", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "Number of the node", true );
    modelInfos[MODEL_TYPE::MOS6].instanceParams.emplace_back( "dnodeprime",  207, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_INT, "", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "Number of int. drain node", true );
    modelInfos[MODEL_TYPE::MOS6].instanceParams.emplace_back( "snodeprime",  208, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_INT, "", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "Number of int. source node", true );
    modelInfos[MODEL_TYPE::MOS6].instanceParams.emplace_back( "rs",  258, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_FLOAT, "Ω", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "0", "0", "Source resistance", true );
    modelInfos[MODEL_TYPE::MOS6].instanceParams.emplace_back( "sourceconductance",  209, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "Source conductance", true );
    modelInfos[MODEL_TYPE::MOS6].instanceParams.emplace_back( "rd",  259, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_FLOAT, "Ω", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "0", "0", "Drain resistance", true );
    modelInfos[MODEL_TYPE::MOS6].instanceParams.emplace_back( "drainconductance",  210, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "Drain conductance", true );
    modelInfos[MODEL_TYPE::MOS6].instanceParams.emplace_back( "von",  211, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_FLOAT, "V", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "Turn-on voltage", true );
    modelInfos[MODEL_TYPE::MOS6].instanceParams.emplace_back( "vdsat",  212, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_FLOAT, "V", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "Saturation drain voltage", true );
    modelInfos[MODEL_TYPE::MOS6].instanceParams.emplace_back( "sourcevcrit",  213, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_FLOAT, "V", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "Critical source voltage", true );
    modelInfos[MODEL_TYPE::MOS6].instanceParams.emplace_back( "drainvcrit",  214, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_FLOAT, "V", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "Critical drain voltage", true );
    modelInfos[MODEL_TYPE::MOS6].instanceParams.emplace_back( "gmbs",  218, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "Bulk-Source transconductance", true );
    modelInfos[MODEL_TYPE::MOS6].instanceParams.emplace_back( "gm",  219, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "Transconductance", true );
    modelInfos[MODEL_TYPE::MOS6].instanceParams.emplace_back( "gds",  220, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "Drain-Source conductance", true );
    modelInfos[MODEL_TYPE::MOS6].instanceParams.emplace_back( "gbd",  221, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "Bulk-Drain conductance", true );
    modelInfos[MODEL_TYPE::MOS6].instanceParams.emplace_back( "gbs",  222, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "Bulk-Source conductance", true );
    modelInfos[MODEL_TYPE::MOS6].instanceParams.emplace_back( "cgs",  233, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_FLOAT, "F", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "Gate-Source capacitance", true );
    modelInfos[MODEL_TYPE::MOS6].instanceParams.emplace_back( "cgd",  236, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_FLOAT, "F", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "Gate-Drain capacitance", true );
    modelInfos[MODEL_TYPE::MOS6].instanceParams.emplace_back( "cgb",  239, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_FLOAT, "F", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "Gate-Bulk capacitance", true );
    modelInfos[MODEL_TYPE::MOS6].instanceParams.emplace_back( "cbd",  223, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_FLOAT, "F", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "0", "0", "Bulk-Drain capacitance", true );
    modelInfos[MODEL_TYPE::MOS6].instanceParams.emplace_back( "cbs",  224, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_FLOAT, "F", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "0", "0", "Bulk-Source capacitance", true );
    modelInfos[MODEL_TYPE::MOS6].instanceParams.emplace_back( "cbd0",  225, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_FLOAT, "F", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "Zero-Bias B-D junction capacitance", true );
    modelInfos[MODEL_TYPE::MOS6].instanceParams.emplace_back( "cbs0",  227, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_FLOAT, "F", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "Zero-Bias B-S junction capacitance", true );
    modelInfos[MODEL_TYPE::MOS6].instanceParams.emplace_back( "cqgs",  235, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_FLOAT, "F", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "Capacitance due to gate-source charge storage", true );
    modelInfos[MODEL_TYPE::MOS6].instanceParams.emplace_back( "cqgd",  238, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_FLOAT, "F", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "Capacitance due to gate-drain charge storage", true );
    modelInfos[MODEL_TYPE::MOS6].instanceParams.emplace_back( "cqgb",  241, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_FLOAT, "F", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "Capacitance due to gate-bulk charge storage", true );
    modelInfos[MODEL_TYPE::MOS6].instanceParams.emplace_back( "cqbd",  243, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_FLOAT, "F", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "Capacitance due to bulk-drain charge storage", true );
    modelInfos[MODEL_TYPE::MOS6].instanceParams.emplace_back( "cqbs",  245, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_FLOAT, "F", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "Capacitance due to bulk-source charge storage", true );
    modelInfos[MODEL_TYPE::MOS6].instanceParams.emplace_back( "qgs",  234, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "Gate-Source charge storage", true );
    modelInfos[MODEL_TYPE::MOS6].instanceParams.emplace_back( "qgd",  237, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "Gate-Drain charge storage", true );
    modelInfos[MODEL_TYPE::MOS6].instanceParams.emplace_back( "qgb",  240, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "Gate-Bulk charge storage", true );
    modelInfos[MODEL_TYPE::MOS6].instanceParams.emplace_back( "qbd",  242, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "Bulk-Drain charge storage", true );
    modelInfos[MODEL_TYPE::MOS6].instanceParams.emplace_back( "qbs",  244, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "Bulk-Source charge storage", true );
    modelInfos[MODEL_TYPE::MOS6].instanceParams.emplace_back( "p",  19, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "Instaneous power", true );
    modelInfos[MODEL_TYPE::MOS6].instanceParams.emplace_back( "sens_l_dc",  256, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_FLOAT, "m", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "dc sensitivity wrt length", true );
    modelInfos[MODEL_TYPE::MOS6].instanceParams.emplace_back( "sens_l_real",  246, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_FLOAT, "m", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "real part of ac sensitivity wrt length", true );
    modelInfos[MODEL_TYPE::MOS6].instanceParams.emplace_back( "sens_l_imag",  247, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_FLOAT, "m", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "imag part of ac sensitivity wrt length", true );
    modelInfos[MODEL_TYPE::MOS6].instanceParams.emplace_back( "sens_l_mag",  248, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "sensitivity wrt l of ac magnitude", true );
    modelInfos[MODEL_TYPE::MOS6].instanceParams.emplace_back( "sens_l_ph",  249, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "sensitivity wrt l of ac phase", true );
    modelInfos[MODEL_TYPE::MOS6].instanceParams.emplace_back( "sens_l_cplx",  250, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_COMPLEX, "", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "ac sensitivity wrt length", true );
    modelInfos[MODEL_TYPE::MOS6].instanceParams.emplace_back( "sens_w_dc",  257, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_FLOAT, "m", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "dc sensitivity wrt width", true );
    modelInfos[MODEL_TYPE::MOS6].instanceParams.emplace_back( "sens_w_real",  251, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_FLOAT, "m", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "real part of ac sensitivity wrt width", true );
    modelInfos[MODEL_TYPE::MOS6].instanceParams.emplace_back( "sens_w_imag",  252, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_FLOAT, "m", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "imag part of ac sensitivity wrt width", true );
    modelInfos[MODEL_TYPE::MOS6].instanceParams.emplace_back( "sens_w_mag",  253, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "sensitivity wrt w of ac magnitude", true );
    modelInfos[MODEL_TYPE::MOS6].instanceParams.emplace_back( "sens_w_ph",  254, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "sensitivity wrt w of ac phase", true );
    modelInfos[MODEL_TYPE::MOS6].instanceParams.emplace_back( "sens_w_cplx",  255, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_COMPLEX, "", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "ac sensitivity wrt width", true );
}