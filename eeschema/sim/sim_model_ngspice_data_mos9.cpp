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


void NGSPICE_MODEL_INFO_MAP::addMOS9()
{
    modelInfos[MODEL_TYPE::MOS9] = { "Mos9", "NMOS", "PMOS", { "D", "G", "S", "B" }, "Modified Level 3 MOSfet model", {}, {} };
    // Model parameters
    modelInfos[MODEL_TYPE::MOS9].modelParams.emplace_back( "type",  144, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_STRING, "", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "nmos", "pmos", "N-channel or P-channel MOS" );
    modelInfos[MODEL_TYPE::MOS9].modelParams.emplace_back( "nmos",  133, SIM_MODEL::PARAM::DIR_IN, SIM_VALUE::TYPE_BOOL, "", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "N type MOSfet model" );
    modelInfos[MODEL_TYPE::MOS9].modelParams.emplace_back( "pmos",  134, SIM_MODEL::PARAM::DIR_IN, SIM_VALUE::TYPE_BOOL, "", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "P type MOSfet model" );
    modelInfos[MODEL_TYPE::MOS9].modelParams.emplace_back( "vto",  101, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "V", SIM_MODEL::PARAM::CATEGORY::DC, "0", "0", "Threshold voltage" );
    modelInfos[MODEL_TYPE::MOS9].modelParams.emplace_back( "vt0",  101, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "0", "0", "n.a." );
    modelInfos[MODEL_TYPE::MOS9].modelParams.emplace_back( "kp",  102, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "A/V²", SIM_MODEL::PARAM::CATEGORY::DC, "2.07189e-05", "2.07189e-05", "Transconductance parameter" );
    modelInfos[MODEL_TYPE::MOS9].modelParams.emplace_back( "gamma",  103, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "sqrt V", SIM_MODEL::PARAM::CATEGORY::DC, "0", "0", "Bulk threshold parameter" );
    modelInfos[MODEL_TYPE::MOS9].modelParams.emplace_back( "phi",  104, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "V", SIM_MODEL::PARAM::CATEGORY::DC, "0.6", "0.6", "Surface potential" );
    modelInfos[MODEL_TYPE::MOS9].modelParams.emplace_back( "rd_",  105, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "Ω", SIM_MODEL::PARAM::CATEGORY::DC, "0", "0", "Drain ohmic resistance" );
    modelInfos[MODEL_TYPE::MOS9].modelParams.emplace_back( "rs_",  106, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "Ω", SIM_MODEL::PARAM::CATEGORY::DC, "0", "0", "Source ohmic resistance" );
    modelInfos[MODEL_TYPE::MOS9].modelParams.emplace_back( "cbd_",  107, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "F", SIM_MODEL::PARAM::CATEGORY::CAPACITANCE, "0", "0", "B-D junction capacitance" );
    modelInfos[MODEL_TYPE::MOS9].modelParams.emplace_back( "cbs_",  108, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "F", SIM_MODEL::PARAM::CATEGORY::CAPACITANCE, "0", "0", "B-S junction capacitance" );
    modelInfos[MODEL_TYPE::MOS9].modelParams.emplace_back( "is_",  109, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "A", SIM_MODEL::PARAM::CATEGORY::DC, "1e-14", "1e-14", "Bulk junction sat. current" );
    modelInfos[MODEL_TYPE::MOS9].modelParams.emplace_back( "pb",  110, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "V", SIM_MODEL::PARAM::CATEGORY::DC, "0.8", "0.8", "Bulk junction potential" );
    modelInfos[MODEL_TYPE::MOS9].modelParams.emplace_back( "cgso",  111, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "F/m", SIM_MODEL::PARAM::CATEGORY::DC, "0", "0", "Gate-source overlap cap." );
    modelInfos[MODEL_TYPE::MOS9].modelParams.emplace_back( "cgdo",  112, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "F/m", SIM_MODEL::PARAM::CATEGORY::DC, "0", "0", "Gate-drain overlap cap." );
    modelInfos[MODEL_TYPE::MOS9].modelParams.emplace_back( "cgbo",  113, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "F/m", SIM_MODEL::PARAM::CATEGORY::DC, "0", "0", "Gate-bulk overlap cap." );
    modelInfos[MODEL_TYPE::MOS9].modelParams.emplace_back( "rsh",  114, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "Ω/m", SIM_MODEL::PARAM::CATEGORY::DC, "0", "0", "Sheet resistance" );
    modelInfos[MODEL_TYPE::MOS9].modelParams.emplace_back( "cj",  115, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "F/m²", SIM_MODEL::PARAM::CATEGORY::DC, "0", "0", "Bottom junction cap per area" );
    modelInfos[MODEL_TYPE::MOS9].modelParams.emplace_back( "mj",  116, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::DC, "0.5", "0.5", "Bottom grading coefficient" );
    modelInfos[MODEL_TYPE::MOS9].modelParams.emplace_back( "cjsw",  117, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "F/m²", SIM_MODEL::PARAM::CATEGORY::DC, "0", "0", "Side junction cap per area" );
    modelInfos[MODEL_TYPE::MOS9].modelParams.emplace_back( "mjsw",  118, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::DC, "0.33", "0.33", "Side grading coefficient" );
    modelInfos[MODEL_TYPE::MOS9].modelParams.emplace_back( "js",  119, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "A/m²", SIM_MODEL::PARAM::CATEGORY::DC, "0", "0", "Bulk jct. sat. current density" );
    modelInfos[MODEL_TYPE::MOS9].modelParams.emplace_back( "tox",  120, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "m", SIM_MODEL::PARAM::CATEGORY::DC, "1e-07", "1e-07", "Oxide thickness" );
    modelInfos[MODEL_TYPE::MOS9].modelParams.emplace_back( "ld",  121, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::DC, "0", "0", "Lateral diffusion" );
    modelInfos[MODEL_TYPE::MOS9].modelParams.emplace_back( "xl",  145, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "m", SIM_MODEL::PARAM::CATEGORY::DC, "0", "0", "Length mask adjustment" );
    modelInfos[MODEL_TYPE::MOS9].modelParams.emplace_back( "wd",  146, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "m", SIM_MODEL::PARAM::CATEGORY::DC, "0", "0", "Width Narrowing (Diffusion)" );
    modelInfos[MODEL_TYPE::MOS9].modelParams.emplace_back( "xw",  147, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "m", SIM_MODEL::PARAM::CATEGORY::DC, "0", "0", "Width mask adjustment" );
    modelInfos[MODEL_TYPE::MOS9].modelParams.emplace_back( "delvto",  148, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "V", SIM_MODEL::PARAM::CATEGORY::DC, "0", "0", "Threshold voltage Adjust" );
    modelInfos[MODEL_TYPE::MOS9].modelParams.emplace_back( "delvt0",  148, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "V", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "0", "0", "Threshold voltage Adjust" );
    modelInfos[MODEL_TYPE::MOS9].modelParams.emplace_back( "u0",  122, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "cm²/V s", SIM_MODEL::PARAM::CATEGORY::DC, "600", "600", "Surface mobility" );
    modelInfos[MODEL_TYPE::MOS9].modelParams.emplace_back( "uo",  122, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "600", "600", "n.a." );
    modelInfos[MODEL_TYPE::MOS9].modelParams.emplace_back( "fc",  123, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::DC, "0.5", "0.5", "Forward bias jct. fit parm." );
    modelInfos[MODEL_TYPE::MOS9].modelParams.emplace_back( "nsub",  124, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "1/cm³", SIM_MODEL::PARAM::CATEGORY::DC, "0", "0", "Substrate doping" );
    modelInfos[MODEL_TYPE::MOS9].modelParams.emplace_back( "tpg",  125, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_INT, "", SIM_MODEL::PARAM::CATEGORY::DC, "0", "0", "Gate type" );
    modelInfos[MODEL_TYPE::MOS9].modelParams.emplace_back( "nss",  126, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "1/cm²", SIM_MODEL::PARAM::CATEGORY::DC, "0", "0", "Surface state density" );
    modelInfos[MODEL_TYPE::MOS9].modelParams.emplace_back( "vmax",  131, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "m/s", SIM_MODEL::PARAM::CATEGORY::DC, "0", "0", "Maximum carrier drift velocity" );
    modelInfos[MODEL_TYPE::MOS9].modelParams.emplace_back( "xj",  135, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::DC, "0", "0", "Junction depth" );
    modelInfos[MODEL_TYPE::MOS9].modelParams.emplace_back( "nfs",  129, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "1/cm²", SIM_MODEL::PARAM::CATEGORY::DC, "0", "0", "Fast surface state density" );
    modelInfos[MODEL_TYPE::MOS9].modelParams.emplace_back( "xd",  138, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "m", SIM_MODEL::PARAM::CATEGORY::DC, "0", "0", "Depletion layer width" );
    modelInfos[MODEL_TYPE::MOS9].modelParams.emplace_back( "alpha",  139, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::DC, "0", "0", "Alpha" );
    modelInfos[MODEL_TYPE::MOS9].modelParams.emplace_back( "eta",  127, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "V", SIM_MODEL::PARAM::CATEGORY::DC, "0", "0", "Vds dependence of threshold voltage" );
    modelInfos[MODEL_TYPE::MOS9].modelParams.emplace_back( "delta",  128, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "m", SIM_MODEL::PARAM::CATEGORY::DC, "0", "0", "Width effect on threshold" );
    modelInfos[MODEL_TYPE::MOS9].modelParams.emplace_back( "theta",  130, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "1/V", SIM_MODEL::PARAM::CATEGORY::DC, "0", "0", "Vgs dependence on mobility" );
    modelInfos[MODEL_TYPE::MOS9].modelParams.emplace_back( "kappa",  132, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::DC, "0.2", "0.2", "Kappa" );
    modelInfos[MODEL_TYPE::MOS9].modelParams.emplace_back( "tnom",  141, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "°C", SIM_MODEL::PARAM::CATEGORY::TEMPERATURE, "27", "27", "Parameter measurement temperature" );
    modelInfos[MODEL_TYPE::MOS9].modelParams.emplace_back( "kf",  142, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::NOISE, "0", "0", "Flicker noise coefficient" );
    modelInfos[MODEL_TYPE::MOS9].modelParams.emplace_back( "af",  143, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::NOISE, "1", "1", "Flicker noise exponent" );
    // Instance parameters
    modelInfos[MODEL_TYPE::MOS9].instanceParams.emplace_back( "m",  80, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::GEOMETRY, "", "", "Multiplier", true );
    modelInfos[MODEL_TYPE::MOS9].instanceParams.emplace_back( "l",  2, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "m", SIM_MODEL::PARAM::CATEGORY::GEOMETRY, "", "", "Length", true );
    modelInfos[MODEL_TYPE::MOS9].instanceParams.emplace_back( "w",  1, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "m", SIM_MODEL::PARAM::CATEGORY::GEOMETRY, "", "", "Width", true );
    modelInfos[MODEL_TYPE::MOS9].instanceParams.emplace_back( "ad",  4, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "m²", SIM_MODEL::PARAM::CATEGORY::GEOMETRY, "", "", "Drain area", true );
    modelInfos[MODEL_TYPE::MOS9].instanceParams.emplace_back( "as",  3, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "m²", SIM_MODEL::PARAM::CATEGORY::GEOMETRY, "", "", "Source area", true );
    modelInfos[MODEL_TYPE::MOS9].instanceParams.emplace_back( "pd",  6, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "m", SIM_MODEL::PARAM::CATEGORY::GEOMETRY, "", "", "Drain perimeter", true );
    modelInfos[MODEL_TYPE::MOS9].instanceParams.emplace_back( "ps",  5, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "m", SIM_MODEL::PARAM::CATEGORY::GEOMETRY, "", "", "Source perimeter", true );
    modelInfos[MODEL_TYPE::MOS9].instanceParams.emplace_back( "id",  34, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_FLOAT, "A", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "Drain current", true );
    modelInfos[MODEL_TYPE::MOS9].instanceParams.emplace_back( "cd",  34, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_FLOAT, "A", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "Drain current", true );
    modelInfos[MODEL_TYPE::MOS9].instanceParams.emplace_back( "ibd",  36, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_FLOAT, "A", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "B-D junction current", true );
    modelInfos[MODEL_TYPE::MOS9].instanceParams.emplace_back( "ibs",  35, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_FLOAT, "A", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "B-S junction current", true );
    modelInfos[MODEL_TYPE::MOS9].instanceParams.emplace_back( "is",  18, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_FLOAT, "A", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "1e-14", "1e-14", "Source current", true );
    modelInfos[MODEL_TYPE::MOS9].instanceParams.emplace_back( "ig",  17, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_FLOAT, "A", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "Gate current", true );
    modelInfos[MODEL_TYPE::MOS9].instanceParams.emplace_back( "ib",  16, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_FLOAT, "A", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "Bulk current", true );
    modelInfos[MODEL_TYPE::MOS9].instanceParams.emplace_back( "vgs",  50, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_FLOAT, "V", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "Gate-Source voltage", true );
    modelInfos[MODEL_TYPE::MOS9].instanceParams.emplace_back( "vds",  51, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_FLOAT, "V", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "Drain-Source voltage", true );
    modelInfos[MODEL_TYPE::MOS9].instanceParams.emplace_back( "vbs",  49, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_FLOAT, "V", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "Bulk-Source voltage", true );
    modelInfos[MODEL_TYPE::MOS9].instanceParams.emplace_back( "vbd",  48, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_FLOAT, "V", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "Bulk-Drain voltage", true );
    modelInfos[MODEL_TYPE::MOS9].instanceParams.emplace_back( "nrd",  8, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::GEOMETRY, "", "", "Drain squares", true );
    modelInfos[MODEL_TYPE::MOS9].instanceParams.emplace_back( "nrs",  7, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::GEOMETRY, "", "", "Source squares", true );
    modelInfos[MODEL_TYPE::MOS9].instanceParams.emplace_back( "off",  9, SIM_MODEL::PARAM::DIR_IN, SIM_VALUE::TYPE_BOOL, "", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "Device initially off", true );
    modelInfos[MODEL_TYPE::MOS9].instanceParams.emplace_back( "icvds",  12, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "V", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "Initial D-S voltage", true );
    modelInfos[MODEL_TYPE::MOS9].instanceParams.emplace_back( "icvgs",  13, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "V", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "Initial G-S voltage", true );
    modelInfos[MODEL_TYPE::MOS9].instanceParams.emplace_back( "icvbs",  11, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "V", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "Initial B-S voltage", true );
    modelInfos[MODEL_TYPE::MOS9].instanceParams.emplace_back( "ic",  10, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT_VECTOR, "", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "Vector of D-S, G-S, B-S voltages", true );
    modelInfos[MODEL_TYPE::MOS9].instanceParams.emplace_back( "temp",  77, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "°C", SIM_MODEL::PARAM::CATEGORY::PRINCIPAL, "", "", "Instance operating temperature", true );
    modelInfos[MODEL_TYPE::MOS9].instanceParams.emplace_back( "dtemp",  81, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "°C", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "Instance operating temperature difference", true );
    modelInfos[MODEL_TYPE::MOS9].instanceParams.emplace_back( "sens_l",  15, SIM_MODEL::PARAM::DIR_IN, SIM_VALUE::TYPE_BOOL, "", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "flag to request sensitivity WRT length", true );
    modelInfos[MODEL_TYPE::MOS9].instanceParams.emplace_back( "sens_w",  14, SIM_MODEL::PARAM::DIR_IN, SIM_VALUE::TYPE_BOOL, "", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "flag to request sensitivity WRT width", true );
    modelInfos[MODEL_TYPE::MOS9].instanceParams.emplace_back( "dnode",  22, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_INT, "", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "Number of drain node", true );
    modelInfos[MODEL_TYPE::MOS9].instanceParams.emplace_back( "gnode",  23, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_INT, "", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "Number of gate node", true );
    modelInfos[MODEL_TYPE::MOS9].instanceParams.emplace_back( "snode",  24, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_INT, "", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "Number of source node", true );
    modelInfos[MODEL_TYPE::MOS9].instanceParams.emplace_back( "bnode",  25, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_INT, "", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "Number of bulk node", true );
    modelInfos[MODEL_TYPE::MOS9].instanceParams.emplace_back( "dnodeprime",  26, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_INT, "", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "Number of internal drain node", true );
    modelInfos[MODEL_TYPE::MOS9].instanceParams.emplace_back( "snodeprime",  27, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_INT, "", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "Number of internal source node", true );
    modelInfos[MODEL_TYPE::MOS9].instanceParams.emplace_back( "von",  30, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_FLOAT, "V", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "Turn-on voltage", true );
    modelInfos[MODEL_TYPE::MOS9].instanceParams.emplace_back( "vdsat",  31, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_FLOAT, "V", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "Saturation drain voltage", true );
    modelInfos[MODEL_TYPE::MOS9].instanceParams.emplace_back( "sourcevcrit",  32, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_FLOAT, "V", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "Critical source voltage", true );
    modelInfos[MODEL_TYPE::MOS9].instanceParams.emplace_back( "drainvcrit",  33, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_FLOAT, "V", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "Critical drain voltage", true );
    modelInfos[MODEL_TYPE::MOS9].instanceParams.emplace_back( "rs",  78, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_FLOAT, "Ω", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "0", "0", "Source resistance", true );
    modelInfos[MODEL_TYPE::MOS9].instanceParams.emplace_back( "sourceconductance",  28, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "Source conductance", true );
    modelInfos[MODEL_TYPE::MOS9].instanceParams.emplace_back( "rd",  79, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_FLOAT, "Ω", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "0", "0", "Drain resistance", true );
    modelInfos[MODEL_TYPE::MOS9].instanceParams.emplace_back( "drainconductance",  29, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "Drain conductance", true );
    modelInfos[MODEL_TYPE::MOS9].instanceParams.emplace_back( "gm",  38, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "Transconductance", true );
    modelInfos[MODEL_TYPE::MOS9].instanceParams.emplace_back( "gds",  39, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "Drain-Source conductance", true );
    modelInfos[MODEL_TYPE::MOS9].instanceParams.emplace_back( "gmb",  37, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "Bulk-Source transconductance", true );
    modelInfos[MODEL_TYPE::MOS9].instanceParams.emplace_back( "gmbs",  37, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "Bulk-Source transconductance", true );
    modelInfos[MODEL_TYPE::MOS9].instanceParams.emplace_back( "gbd",  40, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "Bulk-Drain conductance", true );
    modelInfos[MODEL_TYPE::MOS9].instanceParams.emplace_back( "gbs",  41, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "Bulk-Source conductance", true );
    modelInfos[MODEL_TYPE::MOS9].instanceParams.emplace_back( "cbd",  42, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_FLOAT, "F", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "0", "0", "Bulk-Drain capacitance", true );
    modelInfos[MODEL_TYPE::MOS9].instanceParams.emplace_back( "cbs",  43, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_FLOAT, "F", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "0", "0", "Bulk-Source capacitance", true );
    modelInfos[MODEL_TYPE::MOS9].instanceParams.emplace_back( "cgs",  52, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_FLOAT, "F", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "Gate-Source capacitance", true );
    modelInfos[MODEL_TYPE::MOS9].instanceParams.emplace_back( "cgd",  55, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_FLOAT, "F", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "Gate-Drain capacitance", true );
    modelInfos[MODEL_TYPE::MOS9].instanceParams.emplace_back( "cgb",  58, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_FLOAT, "F", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "Gate-Bulk capacitance", true );
    modelInfos[MODEL_TYPE::MOS9].instanceParams.emplace_back( "cqgs",  54, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_FLOAT, "F", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "Capacitance due to gate-source charge storage", true );
    modelInfos[MODEL_TYPE::MOS9].instanceParams.emplace_back( "cqgd",  57, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_FLOAT, "F", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "Capacitance due to gate-drain charge storage", true );
    modelInfos[MODEL_TYPE::MOS9].instanceParams.emplace_back( "cqgb",  60, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_FLOAT, "F", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "Capacitance due to gate-bulk charge storage", true );
    modelInfos[MODEL_TYPE::MOS9].instanceParams.emplace_back( "cqbd",  62, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_FLOAT, "F", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "Capacitance due to bulk-drain charge storage", true );
    modelInfos[MODEL_TYPE::MOS9].instanceParams.emplace_back( "cqbs",  64, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_FLOAT, "F", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "Capacitance due to bulk-source charge storage", true );
    modelInfos[MODEL_TYPE::MOS9].instanceParams.emplace_back( "cbd0",  44, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_FLOAT, "F", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "Zero-Bias B-D junction capacitance", true );
    modelInfos[MODEL_TYPE::MOS9].instanceParams.emplace_back( "cbdsw0",  45, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_FLOAT, "F", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "Zero-Bias B-D sidewall capacitance", true );
    modelInfos[MODEL_TYPE::MOS9].instanceParams.emplace_back( "cbs0",  46, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_FLOAT, "F", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "Zero-Bias B-S junction capacitance", true );
    modelInfos[MODEL_TYPE::MOS9].instanceParams.emplace_back( "cbssw0",  47, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_FLOAT, "F", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "Zero-Bias B-S sidewall capacitance", true );
    modelInfos[MODEL_TYPE::MOS9].instanceParams.emplace_back( "qbs",  63, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "Bulk-Source charge storage", true );
    modelInfos[MODEL_TYPE::MOS9].instanceParams.emplace_back( "qgs",  53, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "Gate-Source charge storage", true );
    modelInfos[MODEL_TYPE::MOS9].instanceParams.emplace_back( "qgd",  56, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "Gate-Drain charge storage", true );
    modelInfos[MODEL_TYPE::MOS9].instanceParams.emplace_back( "qgb",  59, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "Gate-Bulk charge storage", true );
    modelInfos[MODEL_TYPE::MOS9].instanceParams.emplace_back( "qbd",  61, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "Bulk-Drain charge storage", true );
    modelInfos[MODEL_TYPE::MOS9].instanceParams.emplace_back( "p",  19, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "Instantaneous power", true );
    modelInfos[MODEL_TYPE::MOS9].instanceParams.emplace_back( "sens_l_dc",  76, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_FLOAT, "m", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "dc sensitivity wrt length", true );
    modelInfos[MODEL_TYPE::MOS9].instanceParams.emplace_back( "sens_l_real",  70, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_FLOAT, "m", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "real part of ac sensitivity wrt length", true );
    modelInfos[MODEL_TYPE::MOS9].instanceParams.emplace_back( "sens_l_imag",  71, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_FLOAT, "m", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "imag part of ac sensitivity wrt length", true );
    modelInfos[MODEL_TYPE::MOS9].instanceParams.emplace_back( "sens_l_cplx",  74, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_COMPLEX, "", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "ac sensitivity wrt length", true );
    modelInfos[MODEL_TYPE::MOS9].instanceParams.emplace_back( "sens_l_mag",  72, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "sensitivity wrt l of ac magnitude", true );
    modelInfos[MODEL_TYPE::MOS9].instanceParams.emplace_back( "sens_l_ph",  73, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "sensitivity wrt l of ac phase", true );
    modelInfos[MODEL_TYPE::MOS9].instanceParams.emplace_back( "sens_w_dc",  75, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_FLOAT, "m", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "dc sensitivity wrt width", true );
    modelInfos[MODEL_TYPE::MOS9].instanceParams.emplace_back( "sens_w_real",  65, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_FLOAT, "m", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "real part of ac sensitivity wrt width", true );
    modelInfos[MODEL_TYPE::MOS9].instanceParams.emplace_back( "sens_w_imag",  66, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_FLOAT, "m", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "imag part of ac sensitivity wrt width", true );
    modelInfos[MODEL_TYPE::MOS9].instanceParams.emplace_back( "sens_w_mag",  67, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "sensitivity wrt w of ac magnitude", true );
    modelInfos[MODEL_TYPE::MOS9].instanceParams.emplace_back( "sens_w_ph",  68, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "sensitivity wrt w of ac phase", true );
    modelInfos[MODEL_TYPE::MOS9].instanceParams.emplace_back( "sens_w_cplx",  69, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_COMPLEX, "", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "ac sensitivity wrt width", true );
}