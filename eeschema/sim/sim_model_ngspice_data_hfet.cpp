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


void NGSPICE_MODEL_INFO_MAP::addHFET()
{
    modelInfos[MODEL_TYPE::HFET1] = { "HFET1", "NMF", "PMF", { "D", "G", "S" }, "HFET1 Model", {}, {} };
    // Model parameters
    modelInfos[MODEL_TYPE::HFET1].modelParams.emplace_back( "vt0", 101, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "V", SIM_MODEL::PARAM::CATEGORY::DC, "Pinch-off voltage" );
    modelInfos[MODEL_TYPE::HFET1].modelParams.emplace_back( "vto", 101, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "n.a." );
    modelInfos[MODEL_TYPE::HFET1].modelParams.emplace_back( "lambda", 102, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::DC, "Output conductance parameter" );
    modelInfos[MODEL_TYPE::HFET1].modelParams.emplace_back( "rd", 103, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "ohm", SIM_MODEL::PARAM::CATEGORY::DC, "Drain ohmic resistance" );
    modelInfos[MODEL_TYPE::HFET1].modelParams.emplace_back( "rs", 104, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "ohm", SIM_MODEL::PARAM::CATEGORY::DC, "Source ohmic resistance" );
    modelInfos[MODEL_TYPE::HFET1].modelParams.emplace_back( "rg", 105, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "ohm", SIM_MODEL::PARAM::CATEGORY::DC, "Gate ohmic resistance" );
    modelInfos[MODEL_TYPE::HFET1].modelParams.emplace_back( "rdi", 133, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "ohm", SIM_MODEL::PARAM::CATEGORY::DC, "Drain ohmic resistance" );
    modelInfos[MODEL_TYPE::HFET1].modelParams.emplace_back( "rsi", 134, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "ohm", SIM_MODEL::PARAM::CATEGORY::DC, "Source ohmic resistance" );
    modelInfos[MODEL_TYPE::HFET1].modelParams.emplace_back( "rgs", 106, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "ohm", SIM_MODEL::PARAM::CATEGORY::DC, "Gate-source ohmic resistance" );
    modelInfos[MODEL_TYPE::HFET1].modelParams.emplace_back( "rgd", 107, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "ohm", SIM_MODEL::PARAM::CATEGORY::DC, "Gate-drain ohmic resistance" );
    modelInfos[MODEL_TYPE::HFET1].modelParams.emplace_back( "ri", 108, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::DC, "" );
    modelInfos[MODEL_TYPE::HFET1].modelParams.emplace_back( "rf", 109, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::DC, "" );
    modelInfos[MODEL_TYPE::HFET1].modelParams.emplace_back( "eta", 110, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::DC, "Subthreshold ideality factor" );
    modelInfos[MODEL_TYPE::HFET1].modelParams.emplace_back( "m_", 111, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::DC, "Knee shape parameter" );
    modelInfos[MODEL_TYPE::HFET1].modelParams.emplace_back( "mc", 112, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::DC, "Knee shape parameter" );
    modelInfos[MODEL_TYPE::HFET1].modelParams.emplace_back( "gamma", 113, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::DC, "Knee shape parameter" );
    modelInfos[MODEL_TYPE::HFET1].modelParams.emplace_back( "sigma0", 114, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "V", SIM_MODEL::PARAM::CATEGORY::DC, "Threshold voltage coefficient" );
    modelInfos[MODEL_TYPE::HFET1].modelParams.emplace_back( "vsigmat", 115, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::DC, "" );
    modelInfos[MODEL_TYPE::HFET1].modelParams.emplace_back( "vsigma", 116, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::DC, "" );
    modelInfos[MODEL_TYPE::HFET1].modelParams.emplace_back( "mu", 117, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::DC, "Moblity" );
    modelInfos[MODEL_TYPE::HFET1].modelParams.emplace_back( "di", 118, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::DC, "Depth of device" );
    modelInfos[MODEL_TYPE::HFET1].modelParams.emplace_back( "delta", 119, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::DC, "" );
    modelInfos[MODEL_TYPE::HFET1].modelParams.emplace_back( "vs", 120, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "m/s", SIM_MODEL::PARAM::CATEGORY::DC, "Saturation velocity" );
    modelInfos[MODEL_TYPE::HFET1].modelParams.emplace_back( "nmax", 121, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::DC, "" );
    modelInfos[MODEL_TYPE::HFET1].modelParams.emplace_back( "deltad", 122, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "m", SIM_MODEL::PARAM::CATEGORY::DC, "Thickness correction" );
    modelInfos[MODEL_TYPE::HFET1].modelParams.emplace_back( "js1d", 123, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::DC, "" );
    modelInfos[MODEL_TYPE::HFET1].modelParams.emplace_back( "js2d", 124, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::DC, "" );
    modelInfos[MODEL_TYPE::HFET1].modelParams.emplace_back( "js1s", 125, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::DC, "" );
    modelInfos[MODEL_TYPE::HFET1].modelParams.emplace_back( "js2s", 126, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::DC, "" );
    modelInfos[MODEL_TYPE::HFET1].modelParams.emplace_back( "m1d", 127, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::DC, "" );
    modelInfos[MODEL_TYPE::HFET1].modelParams.emplace_back( "m2d", 128, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::DC, "" );
    modelInfos[MODEL_TYPE::HFET1].modelParams.emplace_back( "m1s", 129, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::DC, "" );
    modelInfos[MODEL_TYPE::HFET1].modelParams.emplace_back( "m2s", 130, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::DC, "" );
    modelInfos[MODEL_TYPE::HFET1].modelParams.emplace_back( "epsi", 132, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::DC, "" );
    modelInfos[MODEL_TYPE::HFET1].modelParams.emplace_back( "p_", 138, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::DC, "" );
    modelInfos[MODEL_TYPE::HFET1].modelParams.emplace_back( "cm3", 152, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::DC, "" );
    modelInfos[MODEL_TYPE::HFET1].modelParams.emplace_back( "a1", 135, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::DC, "" );
    modelInfos[MODEL_TYPE::HFET1].modelParams.emplace_back( "a2", 136, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::DC, "" );
    modelInfos[MODEL_TYPE::HFET1].modelParams.emplace_back( "mv1", 137, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::DC, "" );
    modelInfos[MODEL_TYPE::HFET1].modelParams.emplace_back( "kappa", 139, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::DC, "" );
    modelInfos[MODEL_TYPE::HFET1].modelParams.emplace_back( "delf", 140, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::DC, "" );
    modelInfos[MODEL_TYPE::HFET1].modelParams.emplace_back( "fgds", 141, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::DC, "" );
    modelInfos[MODEL_TYPE::HFET1].modelParams.emplace_back( "tf", 142, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::DC, "" );
    modelInfos[MODEL_TYPE::HFET1].modelParams.emplace_back( "cds", 143, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::DC, "" );
    modelInfos[MODEL_TYPE::HFET1].modelParams.emplace_back( "phib", 144, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::DC, "" );
    modelInfos[MODEL_TYPE::HFET1].modelParams.emplace_back( "talpha", 145, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::DC, "" );
    modelInfos[MODEL_TYPE::HFET1].modelParams.emplace_back( "mt1", 146, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::DC, "" );
    modelInfos[MODEL_TYPE::HFET1].modelParams.emplace_back( "mt2", 147, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::DC, "" );
    modelInfos[MODEL_TYPE::HFET1].modelParams.emplace_back( "ck1", 148, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::DC, "" );
    modelInfos[MODEL_TYPE::HFET1].modelParams.emplace_back( "ck2", 149, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::DC, "" );
    modelInfos[MODEL_TYPE::HFET1].modelParams.emplace_back( "cm1", 150, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::DC, "" );
    modelInfos[MODEL_TYPE::HFET1].modelParams.emplace_back( "cm2", 151, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::DC, "" );
    modelInfos[MODEL_TYPE::HFET1].modelParams.emplace_back( "astar", 153, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::DC, "" );
    modelInfos[MODEL_TYPE::HFET1].modelParams.emplace_back( "eta1", 154, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::DC, "" );
    modelInfos[MODEL_TYPE::HFET1].modelParams.emplace_back( "d1", 155, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::DC, "" );
    modelInfos[MODEL_TYPE::HFET1].modelParams.emplace_back( "vt1", 156, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::DC, "" );
    modelInfos[MODEL_TYPE::HFET1].modelParams.emplace_back( "eta2", 157, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::DC, "" );
    modelInfos[MODEL_TYPE::HFET1].modelParams.emplace_back( "d2", 158, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::DC, "" );
    modelInfos[MODEL_TYPE::HFET1].modelParams.emplace_back( "vt2", 159, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::DC, "" );
    modelInfos[MODEL_TYPE::HFET1].modelParams.emplace_back( "ggr", 160, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::DC, "" );
    modelInfos[MODEL_TYPE::HFET1].modelParams.emplace_back( "del", 161, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::DC, "" );
    modelInfos[MODEL_TYPE::HFET1].modelParams.emplace_back( "gatemod", 162, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_INT, "", SIM_MODEL::PARAM::CATEGORY::DC, "" );
    modelInfos[MODEL_TYPE::HFET1].modelParams.emplace_back( "klambda", 163, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::DC, "" );
    modelInfos[MODEL_TYPE::HFET1].modelParams.emplace_back( "kmu", 164, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::DC, "" );
    modelInfos[MODEL_TYPE::HFET1].modelParams.emplace_back( "kvto", 165, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::DC, "" );
    modelInfos[MODEL_TYPE::HFET1].modelParams.emplace_back( "type", 168, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_STRING, "", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "NHFET or PHFET" );
    modelInfos[MODEL_TYPE::HFET1].modelParams.emplace_back( "nhfet", 166, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_BOOL, "", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "N HFET device" );
    modelInfos[MODEL_TYPE::HFET1].modelParams.emplace_back( "phfet", 167, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_BOOL, "", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "P HFET device" );
    // Instance parameters
// FIXME: Default values were lost for some reason, filled them with "".
    modelInfos[MODEL_TYPE::HFET1].instanceParams.emplace_back( "off", 7, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_BOOL, "", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "Device initially off", true );
    modelInfos[MODEL_TYPE::HFET1].instanceParams.emplace_back( "m", 11, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::GEOMETRY, "", "", "Parallel Multiplier", true );
    modelInfos[MODEL_TYPE::HFET1].instanceParams.emplace_back( "l", 1, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "m", SIM_MODEL::PARAM::CATEGORY::GEOMETRY, "", "", "Length of device", true );
    modelInfos[MODEL_TYPE::HFET1].instanceParams.emplace_back( "w", 2, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "m", SIM_MODEL::PARAM::CATEGORY::GEOMETRY, "", "", "Width of device", true );
    modelInfos[MODEL_TYPE::HFET1].instanceParams.emplace_back( "icvds", 3, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "V", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "Initial D-S voltage", true );
    modelInfos[MODEL_TYPE::HFET1].instanceParams.emplace_back( "icvgs", 4, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "V", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "Initial G-S voltage", true );
    modelInfos[MODEL_TYPE::HFET1].instanceParams.emplace_back( "temp", 5, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "°C", SIM_MODEL::PARAM::CATEGORY::PRINCIPAL, "", "", "Instance temperature", true );
    modelInfos[MODEL_TYPE::HFET1].instanceParams.emplace_back( "dtemp", 10, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "°C", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "Instance temperature difference", true );
    modelInfos[MODEL_TYPE::HFET1].instanceParams.emplace_back( "dnode", 201, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_INT, "", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "Number of drain node", true );
    modelInfos[MODEL_TYPE::HFET1].instanceParams.emplace_back( "gnode", 202, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_INT, "", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "Number of gate node", true );
    modelInfos[MODEL_TYPE::HFET1].instanceParams.emplace_back( "snode", 203, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_INT, "", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "Number of source node", true );
    modelInfos[MODEL_TYPE::HFET1].instanceParams.emplace_back( "dprimenode", 204, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_INT, "", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "Number of internal drain node", true );
    modelInfos[MODEL_TYPE::HFET1].instanceParams.emplace_back( "sprimenode", 205, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_INT, "", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "Number of internal source node", true );
    modelInfos[MODEL_TYPE::HFET1].instanceParams.emplace_back( "vgs", 206, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_FLOAT, "V", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "Gate-Source voltage", true );
    modelInfos[MODEL_TYPE::HFET1].instanceParams.emplace_back( "vgd", 207, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_FLOAT, "V", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "Gate-Drain voltage", true );
    modelInfos[MODEL_TYPE::HFET1].instanceParams.emplace_back( "cg", 208, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_FLOAT, "F", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "Gate capacitance", true );
    modelInfos[MODEL_TYPE::HFET1].instanceParams.emplace_back( "cd", 209, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_FLOAT, "F", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "Drain capacitance", true );
    modelInfos[MODEL_TYPE::HFET1].instanceParams.emplace_back( "cgd", 210, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_FLOAT, "F", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "Gate_Drain capacitance", true );
    modelInfos[MODEL_TYPE::HFET1].instanceParams.emplace_back( "gm", 211, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "Transconductance", true );
    modelInfos[MODEL_TYPE::HFET1].instanceParams.emplace_back( "gds", 212, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "Drain-Source conductance", true );
    modelInfos[MODEL_TYPE::HFET1].instanceParams.emplace_back( "ggs", 213, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "Gate-Source conductance", true );
    modelInfos[MODEL_TYPE::HFET1].instanceParams.emplace_back( "ggd", 214, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "Gate-Drain conductance", true );
    modelInfos[MODEL_TYPE::HFET1].instanceParams.emplace_back( "qgs", 215, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "Gate-Source charge storage", true );
    modelInfos[MODEL_TYPE::HFET1].instanceParams.emplace_back( "cqgs", 216, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_FLOAT, "F", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "Capacitance due to gate-source charge storage", true );
    modelInfos[MODEL_TYPE::HFET1].instanceParams.emplace_back( "qgd", 217, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "Gate-Drain charge storage", true );
    modelInfos[MODEL_TYPE::HFET1].instanceParams.emplace_back( "cqgd", 218, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_FLOAT, "F", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "Capacitance due to gate-drain charge storage", true );
    modelInfos[MODEL_TYPE::HFET1].instanceParams.emplace_back( "cs", 8, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_FLOAT, "A", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "Source current", true );
    modelInfos[MODEL_TYPE::HFET1].instanceParams.emplace_back( "p", 9, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "Power dissipated by the mesfet", true );


    modelInfos[MODEL_TYPE::HFET2] = { "HFET2", "NMF", "PMF", { "D", "G", "S" }, "HFET2 Model", {}, {} };
    // Model parameters
    modelInfos[MODEL_TYPE::HFET2].modelParams.emplace_back( "type", 139, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_STRING, "", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "664639780", "-1511458520", "NHFET or PHFET" );
    modelInfos[MODEL_TYPE::HFET2].modelParams.emplace_back( "nhfet", 101, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_BOOL, "", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "N type HFET model" );
    modelInfos[MODEL_TYPE::HFET2].modelParams.emplace_back( "phfet", 102, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_BOOL, "", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "P type HFET model" );
    modelInfos[MODEL_TYPE::HFET2].modelParams.emplace_back( "cf", 103, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::DC, "", "", "" );
    modelInfos[MODEL_TYPE::HFET2].modelParams.emplace_back( "d1", 104, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::DC, "", "", "" );
    modelInfos[MODEL_TYPE::HFET2].modelParams.emplace_back( "d2", 105, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::DC, "", "", "" );
    modelInfos[MODEL_TYPE::HFET2].modelParams.emplace_back( "del", 106, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::DC, "", "", "" );
    modelInfos[MODEL_TYPE::HFET2].modelParams.emplace_back( "delta", 107, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::DC, "", "", "" );
    modelInfos[MODEL_TYPE::HFET2].modelParams.emplace_back( "deltad", 108, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "m", SIM_MODEL::PARAM::CATEGORY::DC, "", "", "Thickness correction" );
    modelInfos[MODEL_TYPE::HFET2].modelParams.emplace_back( "di", 109, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::DC, "", "", "Depth of device" );
    modelInfos[MODEL_TYPE::HFET2].modelParams.emplace_back( "epsi", 110, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::DC, "", "", "" );
    modelInfos[MODEL_TYPE::HFET2].modelParams.emplace_back( "eta", 111, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::DC, "", "", "Subthreshold ideality factor" );
    modelInfos[MODEL_TYPE::HFET2].modelParams.emplace_back( "eta1", 112, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::DC, "", "", "" );
    modelInfos[MODEL_TYPE::HFET2].modelParams.emplace_back( "eta2", 113, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::DC, "", "", "" );
    modelInfos[MODEL_TYPE::HFET2].modelParams.emplace_back( "gamma", 114, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::DC, "", "", "Knee shape parameter" );
    modelInfos[MODEL_TYPE::HFET2].modelParams.emplace_back( "ggr", 115, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::DC, "", "", "" );
    modelInfos[MODEL_TYPE::HFET2].modelParams.emplace_back( "js", 116, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::DC, "", "", "" );
    modelInfos[MODEL_TYPE::HFET2].modelParams.emplace_back( "klambda", 117, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::DC, "", "", "" );
    modelInfos[MODEL_TYPE::HFET2].modelParams.emplace_back( "kmu", 118, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::DC, "", "", "" );
    modelInfos[MODEL_TYPE::HFET2].modelParams.emplace_back( "knmax", 119, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::DC, "", "", "" );
    modelInfos[MODEL_TYPE::HFET2].modelParams.emplace_back( "kvto", 120, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::DC, "", "", "" );
    modelInfos[MODEL_TYPE::HFET2].modelParams.emplace_back( "lambda", 121, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::DC, "0", "0", "Output conductance parameter" );
    modelInfos[MODEL_TYPE::HFET2].modelParams.emplace_back( "m_", 122, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::DC, "", "", "Knee shape parameter" );
    modelInfos[MODEL_TYPE::HFET2].modelParams.emplace_back( "mc", 123, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::DC, "", "", "Knee shape parameter" );
    modelInfos[MODEL_TYPE::HFET2].modelParams.emplace_back( "mu", 124, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::DC, "", "", "Moblity" );
    modelInfos[MODEL_TYPE::HFET2].modelParams.emplace_back( "n", 125, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::DC, "", "", "" );
    modelInfos[MODEL_TYPE::HFET2].modelParams.emplace_back( "nmax", 126, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::DC, "", "", "" );
    modelInfos[MODEL_TYPE::HFET2].modelParams.emplace_back( "p_", 127, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::DC, "", "", "" );
    modelInfos[MODEL_TYPE::HFET2].modelParams.emplace_back( "rd", 128, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "ohm", SIM_MODEL::PARAM::CATEGORY::DC, "0", "0", "Drain ohmic resistance" );
    modelInfos[MODEL_TYPE::HFET2].modelParams.emplace_back( "rdi", 129, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "ohm", SIM_MODEL::PARAM::CATEGORY::DC, "", "", "Drain ohmic resistance" );
    modelInfos[MODEL_TYPE::HFET2].modelParams.emplace_back( "rs", 130, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "ohm", SIM_MODEL::PARAM::CATEGORY::DC, "0", "0", "Source ohmic resistance" );
    modelInfos[MODEL_TYPE::HFET2].modelParams.emplace_back( "rsi", 131, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "ohm", SIM_MODEL::PARAM::CATEGORY::DC, "", "", "Source ohmic resistance" );
    modelInfos[MODEL_TYPE::HFET2].modelParams.emplace_back( "sigma0", 132, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::DC, "", "", "DIBL parameter" );
    modelInfos[MODEL_TYPE::HFET2].modelParams.emplace_back( "vs", 133, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "m/s", SIM_MODEL::PARAM::CATEGORY::DC, "", "", "Saturation velocity" );
    modelInfos[MODEL_TYPE::HFET2].modelParams.emplace_back( "vsigma", 134, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::DC, "", "", "" );
    modelInfos[MODEL_TYPE::HFET2].modelParams.emplace_back( "vsigmat", 135, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::DC, "", "", "" );
    modelInfos[MODEL_TYPE::HFET2].modelParams.emplace_back( "vt0", 138, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "V", SIM_MODEL::PARAM::CATEGORY::DC, "-2", "-2", "Pinch-off voltage" );
    modelInfos[MODEL_TYPE::HFET2].modelParams.emplace_back( "vto", 138, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "-2", "-2", "n.a." );
    modelInfos[MODEL_TYPE::HFET2].modelParams.emplace_back( "vt1", 136, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::DC, "", "", "" );
    modelInfos[MODEL_TYPE::HFET2].modelParams.emplace_back( "vt2", 137, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::DC, "", "", "" );
    // Instance parameters
    modelInfos[MODEL_TYPE::HFET2].instanceParams.emplace_back( "off", 6, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_BOOL, "", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "Device initialli OFF", true );
    modelInfos[MODEL_TYPE::HFET2].instanceParams.emplace_back( "m", 11, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::GEOMETRY, "", "", "Parallel Multiplier", true );
    modelInfos[MODEL_TYPE::HFET2].instanceParams.emplace_back( "l", 1, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "m", SIM_MODEL::PARAM::CATEGORY::GEOMETRY, "", "", "Length of device", true );
    modelInfos[MODEL_TYPE::HFET2].instanceParams.emplace_back( "w", 2, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "m", SIM_MODEL::PARAM::CATEGORY::GEOMETRY, "", "", "Width of device", true );
    modelInfos[MODEL_TYPE::HFET2].instanceParams.emplace_back( "icvds", 3, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "V", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "Initial D-S voltage", true );
    modelInfos[MODEL_TYPE::HFET2].instanceParams.emplace_back( "icvgs", 4, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "V", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "Initial G-S voltage", true );
    modelInfos[MODEL_TYPE::HFET2].instanceParams.emplace_back( "temp", 9, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "°C", SIM_MODEL::PARAM::CATEGORY::PRINCIPAL, "", "", "Instance temperature", true );
    modelInfos[MODEL_TYPE::HFET2].instanceParams.emplace_back( "dtemp", 10, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "°C", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "Instance temperature difference", true );
    modelInfos[MODEL_TYPE::HFET2].instanceParams.emplace_back( "dnode", 201, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_INT, "", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "Number of drain node", true );
    modelInfos[MODEL_TYPE::HFET2].instanceParams.emplace_back( "gnode", 202, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_INT, "", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "Number of gate node", true );
    modelInfos[MODEL_TYPE::HFET2].instanceParams.emplace_back( "snode", 203, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_INT, "", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "Number of source node", true );
    modelInfos[MODEL_TYPE::HFET2].instanceParams.emplace_back( "dprimenode", 204, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_INT, "", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "Number of internal drain node", true );
    modelInfos[MODEL_TYPE::HFET2].instanceParams.emplace_back( "sprimenode", 205, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_INT, "", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "Number of internal source node", true );
    modelInfos[MODEL_TYPE::HFET2].instanceParams.emplace_back( "vgs", 206, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_FLOAT, "V", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "Gate-Source voltage", true );
    modelInfos[MODEL_TYPE::HFET2].instanceParams.emplace_back( "vgd", 207, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_FLOAT, "V", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "Gate-Drain voltage", true );
    modelInfos[MODEL_TYPE::HFET2].instanceParams.emplace_back( "cg", 208, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_FLOAT, "F", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "Gate capacitance", true );
    modelInfos[MODEL_TYPE::HFET2].instanceParams.emplace_back( "cd", 209, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_FLOAT, "F", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "Drain capacitance", true );
    modelInfos[MODEL_TYPE::HFET2].instanceParams.emplace_back( "cgd", 210, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_FLOAT, "F", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "0", "0", "Gate_Drain capacitance", true );
    modelInfos[MODEL_TYPE::HFET2].instanceParams.emplace_back( "gm", 211, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "Transconductance", true );
    modelInfos[MODEL_TYPE::HFET2].instanceParams.emplace_back( "gds", 212, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "Drain-Source conductance", true );
    modelInfos[MODEL_TYPE::HFET2].instanceParams.emplace_back( "ggs", 213, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "Gate-Source conductance", true );
    modelInfos[MODEL_TYPE::HFET2].instanceParams.emplace_back( "ggd", 214, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "Gate-Drain conductance", true );
    modelInfos[MODEL_TYPE::HFET2].instanceParams.emplace_back( "qgs", 215, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "Gate-Source charge storage", true );
    modelInfos[MODEL_TYPE::HFET2].instanceParams.emplace_back( "cqgs", 216, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_FLOAT, "F", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "Capacitance due to gate-source charge storage", true );
    modelInfos[MODEL_TYPE::HFET2].instanceParams.emplace_back( "qgd", 217, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "Gate-Drain charge storage", true );
    modelInfos[MODEL_TYPE::HFET2].instanceParams.emplace_back( "cqgd", 218, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_FLOAT, "F", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "Capacitance due to gate-drain charge storage", true );
    modelInfos[MODEL_TYPE::HFET2].instanceParams.emplace_back( "cs", 7, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_FLOAT, "A", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "Source current", true );
    modelInfos[MODEL_TYPE::HFET2].instanceParams.emplace_back( "p", 8, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "Power dissipated by the mesfet", true );
}