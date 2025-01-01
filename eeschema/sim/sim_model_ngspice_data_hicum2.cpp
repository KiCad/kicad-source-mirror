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


void NGSPICE_MODEL_INFO_MAP::addHICUM2()
{
    modelInfos[MODEL_TYPE::HICUM2] = { "hicum2", "NPN", "PNP", { "C", "B", "E", "S", "TJ" }, "High Current Model for BJT" , {}, {} };
    // Model parameters
    modelInfos[MODEL_TYPE::HICUM2].modelParams.emplace_back( "type", 305, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_STRING, "", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "npn", "pnp", "For transistor type NPN(+1) or PNP (-1)" );
    modelInfos[MODEL_TYPE::HICUM2].modelParams.emplace_back( "npn", 101, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_BOOL, "", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "NaN", "NaN", "NPN type device" );
    modelInfos[MODEL_TYPE::HICUM2].modelParams.emplace_back( "pnp", 102, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_BOOL, "", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "NaN", "NaN", "PNP type device" );
    modelInfos[MODEL_TYPE::HICUM2].modelParams.emplace_back( "tnom", 103, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "°C", SIM_MODEL::PARAM::CATEGORY::TEMPERATURE, "300.15", "300.15", "Temperature at which parameters are specified" );
    modelInfos[MODEL_TYPE::HICUM2].modelParams.emplace_back( "tref", 103, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "300.15", "300.15", "n.a." );
    modelInfos[MODEL_TYPE::HICUM2].modelParams.emplace_back( "version", 104, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_STRING, "", SIM_MODEL::PARAM::CATEGORY::DC, "2.4.0", "2.4.0", "parameter for model version" );
    modelInfos[MODEL_TYPE::HICUM2].modelParams.emplace_back( "c10", 105, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::DC, "2e-30", "2e-30", "GICCR constant" );
    modelInfos[MODEL_TYPE::HICUM2].modelParams.emplace_back( "qp0", 106, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::DC, "2e-14", "2e-14", "Zero-bias hole charge" );
    modelInfos[MODEL_TYPE::HICUM2].modelParams.emplace_back( "ich", 107, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "A", SIM_MODEL::PARAM::CATEGORY::DC, "0", "0", "High-current correction for 2D and 3D effects" );
    modelInfos[MODEL_TYPE::HICUM2].modelParams.emplace_back( "hf0", 108, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "A", SIM_MODEL::PARAM::CATEGORY::DC, "1", "1", "Weight factor for the low current minority charge" );
    modelInfos[MODEL_TYPE::HICUM2].modelParams.emplace_back( "hfe", 109, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::DC, "1", "1", "Emitter minority charge weighting factor in HBTs" );
    modelInfos[MODEL_TYPE::HICUM2].modelParams.emplace_back( "hfc", 110, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::DC, "1", "1", "Collector minority charge weighting factor in HBTs" );
    modelInfos[MODEL_TYPE::HICUM2].modelParams.emplace_back( "hjei", 111, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::DC, "1", "1", "B-E depletion charge weighting factor in HBTs" );
    modelInfos[MODEL_TYPE::HICUM2].modelParams.emplace_back( "ahjei", 112, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::DC, "0", "0", "Parameter describing the slope of hjEi(VBE)" );
    modelInfos[MODEL_TYPE::HICUM2].modelParams.emplace_back( "rhjei", 113, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "V", SIM_MODEL::PARAM::CATEGORY::DC, "1", "1", "Smoothing parameter for hjEi(VBE) at high voltage" );
    modelInfos[MODEL_TYPE::HICUM2].modelParams.emplace_back( "hjci", 114, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::DC, "1", "1", "B-C depletion charge weighting factor in HBTs" );
    modelInfos[MODEL_TYPE::HICUM2].modelParams.emplace_back( "ibeis", 115, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "A", SIM_MODEL::PARAM::CATEGORY::DC, "1e-18", "1e-18", "Internal B-E saturation current" );
    modelInfos[MODEL_TYPE::HICUM2].modelParams.emplace_back( "mbei", 116, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "A", SIM_MODEL::PARAM::CATEGORY::DC, "1", "1", "Internal B-E current ideality factor" );
    modelInfos[MODEL_TYPE::HICUM2].modelParams.emplace_back( "ireis", 117, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "A", SIM_MODEL::PARAM::CATEGORY::DC, "0", "0", "Internal B-E recombination saturation current" );
    modelInfos[MODEL_TYPE::HICUM2].modelParams.emplace_back( "mrei", 118, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "A", SIM_MODEL::PARAM::CATEGORY::DC, "2", "2", "Internal B-E recombination current ideality factor" );
    modelInfos[MODEL_TYPE::HICUM2].modelParams.emplace_back( "ibeps", 119, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "A", SIM_MODEL::PARAM::CATEGORY::DC, "0", "0", "Peripheral B-E saturation current" );
    modelInfos[MODEL_TYPE::HICUM2].modelParams.emplace_back( "mbep", 120, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "A", SIM_MODEL::PARAM::CATEGORY::DC, "1", "1", "Peripheral B-E current ideality factor" );
    modelInfos[MODEL_TYPE::HICUM2].modelParams.emplace_back( "ireps", 121, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "A", SIM_MODEL::PARAM::CATEGORY::DC, "0", "0", "Peripheral B-E recombination saturation current" );
    modelInfos[MODEL_TYPE::HICUM2].modelParams.emplace_back( "mrep", 122, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "A", SIM_MODEL::PARAM::CATEGORY::DC, "2", "2", "Peripheral B-E recombination current ideality factor" );
    modelInfos[MODEL_TYPE::HICUM2].modelParams.emplace_back( "mcf", 123, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::DC, "1", "1", "Non-ideality factor for III-V HBTs" );
    modelInfos[MODEL_TYPE::HICUM2].modelParams.emplace_back( "tbhrec", 124, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "A", SIM_MODEL::PARAM::CATEGORY::DC, "0", "0", "Base current recombination time constant at B-C barrier for high forward injection" );
    modelInfos[MODEL_TYPE::HICUM2].modelParams.emplace_back( "ibcis", 125, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "A", SIM_MODEL::PARAM::CATEGORY::DC, "1e-16", "1e-16", "Internal B-C saturation current" );
    modelInfos[MODEL_TYPE::HICUM2].modelParams.emplace_back( "mbci", 126, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "A", SIM_MODEL::PARAM::CATEGORY::DC, "1", "1", "Internal B-C current ideality factor" );
    modelInfos[MODEL_TYPE::HICUM2].modelParams.emplace_back( "ibcxs", 127, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "A", SIM_MODEL::PARAM::CATEGORY::DC, "0", "0", "External B-C saturation current" );
    modelInfos[MODEL_TYPE::HICUM2].modelParams.emplace_back( "mbcx", 128, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "A", SIM_MODEL::PARAM::CATEGORY::DC, "1", "1", "External B-C current ideality factor" );
    modelInfos[MODEL_TYPE::HICUM2].modelParams.emplace_back( "ibets", 129, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "A", SIM_MODEL::PARAM::CATEGORY::DC, "0", "0", "B-E tunneling saturation current" );
    modelInfos[MODEL_TYPE::HICUM2].modelParams.emplace_back( "abet", 130, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::DC, "40", "40", "Exponent factor for tunneling current" );
    modelInfos[MODEL_TYPE::HICUM2].modelParams.emplace_back( "tunode", 131, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_INT, "", SIM_MODEL::PARAM::CATEGORY::DC, "0", "0", "Specifies the base node connection for the tunneling current" );
    modelInfos[MODEL_TYPE::HICUM2].modelParams.emplace_back( "favl", 132, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "A", SIM_MODEL::PARAM::CATEGORY::DC, "0", "0", "Avalanche current factor" );
    modelInfos[MODEL_TYPE::HICUM2].modelParams.emplace_back( "qavl", 133, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::DC, "0", "0", "Exponent factor for avalanche current" );
    modelInfos[MODEL_TYPE::HICUM2].modelParams.emplace_back( "kavl", 134, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::DC, "0", "0", "Flag/factor for turning strong avalanche on" );
    modelInfos[MODEL_TYPE::HICUM2].modelParams.emplace_back( "alfav", 135, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::DC, "0", "0", "Relative TC for FAVL" );
    modelInfos[MODEL_TYPE::HICUM2].modelParams.emplace_back( "alqav", 136, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::DC, "0", "0", "Relative TC for QAVL" );
    modelInfos[MODEL_TYPE::HICUM2].modelParams.emplace_back( "alkav", 137, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::DC, "0", "0", "Relative TC for KAVL" );
    modelInfos[MODEL_TYPE::HICUM2].modelParams.emplace_back( "rbi0", 138, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "ohm", SIM_MODEL::PARAM::CATEGORY::DC, "0", "0", "Zero bias internal base resistance" );
    modelInfos[MODEL_TYPE::HICUM2].modelParams.emplace_back( "rbx", 139, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "ohm", SIM_MODEL::PARAM::CATEGORY::DC, "0", "0", "External base series resistance" );
    modelInfos[MODEL_TYPE::HICUM2].modelParams.emplace_back( "fgeo", 140, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "A", SIM_MODEL::PARAM::CATEGORY::DC, "0.6557", "0.6557", "Factor for geometry dependence of emitter current crowding" );
    modelInfos[MODEL_TYPE::HICUM2].modelParams.emplace_back( "fdqr0", 141, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::DC, "0", "0", "Correction factor for modulation by B-E and B-C space charge layer" );
    modelInfos[MODEL_TYPE::HICUM2].modelParams.emplace_back( "fcrbi", 142, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "F", SIM_MODEL::PARAM::CATEGORY::DC, "0", "0", "Ratio of HF shunt to total internal capacitance (lateral NQS effect)" );
    modelInfos[MODEL_TYPE::HICUM2].modelParams.emplace_back( "fqi", 143, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::DC, "1", "1", "Ration of internal to total minority charge" );
    modelInfos[MODEL_TYPE::HICUM2].modelParams.emplace_back( "re", 144, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "ohm", SIM_MODEL::PARAM::CATEGORY::DC, "0", "0", "Emitter series resistance" );
    modelInfos[MODEL_TYPE::HICUM2].modelParams.emplace_back( "rcx", 145, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "ohm", SIM_MODEL::PARAM::CATEGORY::DC, "0", "0", "External collector series resistance" );
    modelInfos[MODEL_TYPE::HICUM2].modelParams.emplace_back( "itss", 146, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "A", SIM_MODEL::PARAM::CATEGORY::DC, "0", "0", "Substrate transistor transfer saturation current" );
    modelInfos[MODEL_TYPE::HICUM2].modelParams.emplace_back( "msf", 147, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "A", SIM_MODEL::PARAM::CATEGORY::DC, "1", "1", "Forward ideality factor of substrate transfer current" );
    modelInfos[MODEL_TYPE::HICUM2].modelParams.emplace_back( "iscs", 148, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "A", SIM_MODEL::PARAM::CATEGORY::DC, "0", "0", "C-S diode saturation current" );
    modelInfos[MODEL_TYPE::HICUM2].modelParams.emplace_back( "msc", 149, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "A", SIM_MODEL::PARAM::CATEGORY::DC, "1", "1", "Ideality factor of C-S diode current" );
    modelInfos[MODEL_TYPE::HICUM2].modelParams.emplace_back( "tsf", 150, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "s", SIM_MODEL::PARAM::CATEGORY::DC, "0", "0", "Transit time for forward operation of substrate transistor" );
    modelInfos[MODEL_TYPE::HICUM2].modelParams.emplace_back( "rsu", 151, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "ohm", SIM_MODEL::PARAM::CATEGORY::DC, "0", "0", "Substrate series resistance" );
    modelInfos[MODEL_TYPE::HICUM2].modelParams.emplace_back( "csu", 152, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "F", SIM_MODEL::PARAM::CATEGORY::DC, "0", "0", "Substrate shunt capacitance" );
    modelInfos[MODEL_TYPE::HICUM2].modelParams.emplace_back( "cjei0", 153, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "F", SIM_MODEL::PARAM::CATEGORY::DC, "1e-20", "1e-20", "Internal B-E zero-bias depletion capacitance" );
    modelInfos[MODEL_TYPE::HICUM2].modelParams.emplace_back( "vdei", 154, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "V", SIM_MODEL::PARAM::CATEGORY::DC, "0.9", "0.9", "Internal B-E built-in potential" );
    modelInfos[MODEL_TYPE::HICUM2].modelParams.emplace_back( "zei", 155, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::DC, "0.5", "0.5", "Internal B-E grading coefficient" );
    modelInfos[MODEL_TYPE::HICUM2].modelParams.emplace_back( "ajei", 156, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "F", SIM_MODEL::PARAM::CATEGORY::DC, "2.5", "2.5", "Ratio of maximum to zero-bias value of internal B-E capacitance" );
    modelInfos[MODEL_TYPE::HICUM2].modelParams.emplace_back( "aljei", 156, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "2.5", "2.5", "n.a." );
    modelInfos[MODEL_TYPE::HICUM2].modelParams.emplace_back( "cjep0", 157, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "F", SIM_MODEL::PARAM::CATEGORY::DC, "1e-20", "1e-20", "Peripheral B-E zero-bias depletion capacitance" );
    modelInfos[MODEL_TYPE::HICUM2].modelParams.emplace_back( "vdep", 158, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "V", SIM_MODEL::PARAM::CATEGORY::DC, "0.9", "0.9", "Peripheral B-E built-in potential" );
    modelInfos[MODEL_TYPE::HICUM2].modelParams.emplace_back( "zep", 159, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::DC, "0.5", "0.5", "Peripheral B-E grading coefficient" );
    modelInfos[MODEL_TYPE::HICUM2].modelParams.emplace_back( "ajep", 160, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "F", SIM_MODEL::PARAM::CATEGORY::DC, "2.5", "2.5", "Ratio of maximum to zero-bias value of peripheral B-E capacitance" );
    modelInfos[MODEL_TYPE::HICUM2].modelParams.emplace_back( "aljep", 160, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "2.5", "2.5", "n.a." );
    modelInfos[MODEL_TYPE::HICUM2].modelParams.emplace_back( "cjci0", 161, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "F", SIM_MODEL::PARAM::CATEGORY::DC, "1e-20", "1e-20", "Internal B-C zero-bias depletion capacitance" );
    modelInfos[MODEL_TYPE::HICUM2].modelParams.emplace_back( "vdci", 162, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "V", SIM_MODEL::PARAM::CATEGORY::DC, "0.7", "0.7", "Internal B-C built-in potential" );
    modelInfos[MODEL_TYPE::HICUM2].modelParams.emplace_back( "zci", 163, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::DC, "0.4", "0.4", "Internal B-C grading coefficient" );
    modelInfos[MODEL_TYPE::HICUM2].modelParams.emplace_back( "vptci", 164, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "V", SIM_MODEL::PARAM::CATEGORY::DC, "100", "100", "Internal B-C punch-through voltage" );
    modelInfos[MODEL_TYPE::HICUM2].modelParams.emplace_back( "cjcx0", 165, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "F", SIM_MODEL::PARAM::CATEGORY::DC, "1e-20", "1e-20", "External B-C zero-bias depletion capacitance" );
    modelInfos[MODEL_TYPE::HICUM2].modelParams.emplace_back( "vdcx", 166, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "V", SIM_MODEL::PARAM::CATEGORY::DC, "0.7", "0.7", "External B-C built-in potential" );
    modelInfos[MODEL_TYPE::HICUM2].modelParams.emplace_back( "zcx", 167, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::DC, "0.4", "0.4", "External B-C grading coefficient" );
    modelInfos[MODEL_TYPE::HICUM2].modelParams.emplace_back( "vptcx", 168, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "V", SIM_MODEL::PARAM::CATEGORY::DC, "100", "100", "External B-C punch-through voltage" );
    modelInfos[MODEL_TYPE::HICUM2].modelParams.emplace_back( "fbcpar", 169, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::DC, "0", "0", "Partitioning factor of parasitic B-C cap" );
    modelInfos[MODEL_TYPE::HICUM2].modelParams.emplace_back( "fbc", 169, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "0", "0", "n.a." );
    modelInfos[MODEL_TYPE::HICUM2].modelParams.emplace_back( "fbepar", 170, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::DC, "1", "1", "Partitioning factor of parasitic B-E cap" );
    modelInfos[MODEL_TYPE::HICUM2].modelParams.emplace_back( "fbe", 170, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "1", "1", "n.a." );
    modelInfos[MODEL_TYPE::HICUM2].modelParams.emplace_back( "cjs0", 171, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "F", SIM_MODEL::PARAM::CATEGORY::DC, "0", "0", "C-S zero-bias depletion capacitance" );
    modelInfos[MODEL_TYPE::HICUM2].modelParams.emplace_back( "vds", 172, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "V", SIM_MODEL::PARAM::CATEGORY::DC, "0.6", "0.6", "C-S built-in potential" );
    modelInfos[MODEL_TYPE::HICUM2].modelParams.emplace_back( "zs", 173, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::DC, "0.5", "0.5", "C-S grading coefficient" );
    modelInfos[MODEL_TYPE::HICUM2].modelParams.emplace_back( "vpts", 174, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "V", SIM_MODEL::PARAM::CATEGORY::DC, "100", "100", "C-S punch-through voltage" );
    modelInfos[MODEL_TYPE::HICUM2].modelParams.emplace_back( "cscp0", 175, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "F", SIM_MODEL::PARAM::CATEGORY::DC, "0", "0", "Perimeter S-C zero-bias depletion capacitance" );
    modelInfos[MODEL_TYPE::HICUM2].modelParams.emplace_back( "vdsp", 176, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "V", SIM_MODEL::PARAM::CATEGORY::DC, "0.6", "0.6", "Perimeter S-C built-in potential" );
    modelInfos[MODEL_TYPE::HICUM2].modelParams.emplace_back( "zsp", 177, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::DC, "0.5", "0.5", "Perimeter S-C grading coefficient" );
    modelInfos[MODEL_TYPE::HICUM2].modelParams.emplace_back( "vptsp", 178, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "V", SIM_MODEL::PARAM::CATEGORY::DC, "100", "100", "Perimeter S-C punch-through voltage" );
    modelInfos[MODEL_TYPE::HICUM2].modelParams.emplace_back( "t0", 179, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "A", SIM_MODEL::PARAM::CATEGORY::DC, "0", "0", "Low current forward transit time at VBC=0V" );
    modelInfos[MODEL_TYPE::HICUM2].modelParams.emplace_back( "dt0h", 180, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "m", SIM_MODEL::PARAM::CATEGORY::DC, "0", "0", "Time constant for base and B-C space charge layer width modulation" );
    modelInfos[MODEL_TYPE::HICUM2].modelParams.emplace_back( "tbvl", 181, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "s", SIM_MODEL::PARAM::CATEGORY::DC, "0", "0", "Time constant for modeling carrier jam at low VCE" );
    modelInfos[MODEL_TYPE::HICUM2].modelParams.emplace_back( "tef0", 182, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "s", SIM_MODEL::PARAM::CATEGORY::DC, "0", "0", "Neutral emitter storage time" );
    modelInfos[MODEL_TYPE::HICUM2].modelParams.emplace_back( "gtfe", 183, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::DC, "1", "1", "Exponent factor for current dependence of neutral emitter storage time" );
    modelInfos[MODEL_TYPE::HICUM2].modelParams.emplace_back( "thcs", 184, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "A", SIM_MODEL::PARAM::CATEGORY::DC, "0", "0", "Saturation time constant at high current densities" );
    modelInfos[MODEL_TYPE::HICUM2].modelParams.emplace_back( "ahc", 185, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "A", SIM_MODEL::PARAM::CATEGORY::DC, "0.1", "0.1", "Smoothing factor for current dependence of base and collector transit time" );
    modelInfos[MODEL_TYPE::HICUM2].modelParams.emplace_back( "alhc", 185, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "0.1", "0.1", "n.a." );
    modelInfos[MODEL_TYPE::HICUM2].modelParams.emplace_back( "fthc", 186, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::DC, "0", "0", "Partitioning factor for base and collector portion" );
    modelInfos[MODEL_TYPE::HICUM2].modelParams.emplace_back( "rci0", 187, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "ohm", SIM_MODEL::PARAM::CATEGORY::DC, "150", "150", "Internal collector resistance at low electric field" );
    modelInfos[MODEL_TYPE::HICUM2].modelParams.emplace_back( "vlim", 188, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "V", SIM_MODEL::PARAM::CATEGORY::DC, "0.5", "0.5", "Voltage separating ohmic and saturation velocity regime" );
    modelInfos[MODEL_TYPE::HICUM2].modelParams.emplace_back( "vces", 189, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "V", SIM_MODEL::PARAM::CATEGORY::DC, "0.1", "0.1", "Internal C-E saturation voltage" );
    modelInfos[MODEL_TYPE::HICUM2].modelParams.emplace_back( "vpt", 190, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "V", SIM_MODEL::PARAM::CATEGORY::DC, "100", "100", "Collector punch-through voltage" );
    modelInfos[MODEL_TYPE::HICUM2].modelParams.emplace_back( "aick", 191, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::DC, "0.001", "0.001", "Smoothing term for ICK" );
    modelInfos[MODEL_TYPE::HICUM2].modelParams.emplace_back( "delck", 192, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "A", SIM_MODEL::PARAM::CATEGORY::DC, "2", "2", "Fitting factor for critical current" );
    modelInfos[MODEL_TYPE::HICUM2].modelParams.emplace_back( "tr", 193, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "s", SIM_MODEL::PARAM::CATEGORY::DC, "0", "0", "Storage time for inverse operation" );
    modelInfos[MODEL_TYPE::HICUM2].modelParams.emplace_back( "vcbar", 194, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "V", SIM_MODEL::PARAM::CATEGORY::DC, "0", "0", "Barrier voltage" );
    modelInfos[MODEL_TYPE::HICUM2].modelParams.emplace_back( "icbar", 195, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::DC, "0", "0", "Normalization parameter" );
    modelInfos[MODEL_TYPE::HICUM2].modelParams.emplace_back( "acbar", 196, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "V", SIM_MODEL::PARAM::CATEGORY::DC, "0.01", "0.01", "Smoothing parameter for barrier voltage" );
    modelInfos[MODEL_TYPE::HICUM2].modelParams.emplace_back( "cbepar", 197, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "F", SIM_MODEL::PARAM::CATEGORY::DC, "0", "0", "Total parasitic B-E capacitance" );
    modelInfos[MODEL_TYPE::HICUM2].modelParams.emplace_back( "ceox", 197, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "0", "0", "n.a." );
    modelInfos[MODEL_TYPE::HICUM2].modelParams.emplace_back( "cbcpar", 198, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "F", SIM_MODEL::PARAM::CATEGORY::DC, "0", "0", "Total parasitic B-C capacitance" );
    modelInfos[MODEL_TYPE::HICUM2].modelParams.emplace_back( "ccox", 198, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "0", "0", "n.a." );
    modelInfos[MODEL_TYPE::HICUM2].modelParams.emplace_back( "alqf", 199, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "s", SIM_MODEL::PARAM::CATEGORY::DC, "0.167", "0.167", "Factor for additional delay time of minority charge" );
    modelInfos[MODEL_TYPE::HICUM2].modelParams.emplace_back( "alit", 200, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "A", SIM_MODEL::PARAM::CATEGORY::DC, "0.333", "0.333", "Factor for additional delay time of transfer current" );
    modelInfos[MODEL_TYPE::HICUM2].modelParams.emplace_back( "flnqs", 201, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_INT, "", SIM_MODEL::PARAM::CATEGORY::DC, "0", "0", "Flag for turning on and off of vertical NQS effect" );
    modelInfos[MODEL_TYPE::HICUM2].modelParams.emplace_back( "kf", 202, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::NOISE, "0", "0", "Flicker noise coefficient" );
    modelInfos[MODEL_TYPE::HICUM2].modelParams.emplace_back( "af", 203, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::NOISE, "2", "2", "Flicker noise exponent factor" );
    modelInfos[MODEL_TYPE::HICUM2].modelParams.emplace_back( "cfbe", 204, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_INT, "", SIM_MODEL::PARAM::CATEGORY::NOISE, "0", "0", "Flag for determining where to tag the flicker noise source" );
    modelInfos[MODEL_TYPE::HICUM2].modelParams.emplace_back( "flcono", 205, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_INT, "", SIM_MODEL::PARAM::CATEGORY::NOISE, "0", "0", "Flag for turning on and off of correlated noise implementation" );
    modelInfos[MODEL_TYPE::HICUM2].modelParams.emplace_back( "kfre", 206, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "ohm", SIM_MODEL::PARAM::CATEGORY::NOISE, "0", "0", "Emitter resistance flicker noise coefficient" );
    modelInfos[MODEL_TYPE::HICUM2].modelParams.emplace_back( "afre", 207, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::NOISE, "2", "2", "Emitter resistance flicker noise exponent factor" );
    modelInfos[MODEL_TYPE::HICUM2].modelParams.emplace_back( "latb", 208, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "m", SIM_MODEL::PARAM::CATEGORY::DC, "0", "0", "Scaling factor for collector minority charge in direction of emitter width" );
    modelInfos[MODEL_TYPE::HICUM2].modelParams.emplace_back( "latl", 209, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "m", SIM_MODEL::PARAM::CATEGORY::DC, "0", "0", "Scaling factor for collector minority charge in direction of emitter length" );
    modelInfos[MODEL_TYPE::HICUM2].modelParams.emplace_back( "vgb", 210, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "V", SIM_MODEL::PARAM::CATEGORY::DC, "1.17", "1.17", "Bandgap voltage extrapolated to 0 K" );
    modelInfos[MODEL_TYPE::HICUM2].modelParams.emplace_back( "alt0", 211, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::DC, "0", "0", "First order relative TC of parameter T0" );
    modelInfos[MODEL_TYPE::HICUM2].modelParams.emplace_back( "kt0", 212, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::DC, "0", "0", "Second order relative TC of parameter T0" );
    modelInfos[MODEL_TYPE::HICUM2].modelParams.emplace_back( "zetaci", 213, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::TEMPERATURE, "0", "0", "Temperature exponent for RCI0" );
    modelInfos[MODEL_TYPE::HICUM2].modelParams.emplace_back( "alvs", 214, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "m/s", SIM_MODEL::PARAM::CATEGORY::DC, "0", "0", "Relative TC of saturation drift velocity" );
    modelInfos[MODEL_TYPE::HICUM2].modelParams.emplace_back( "alces", 215, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::DC, "0", "0", "Relative TC of VCES" );
    modelInfos[MODEL_TYPE::HICUM2].modelParams.emplace_back( "zetarbi", 216, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::TEMPERATURE, "0", "0", "Temperature exponent of internal base resistance" );
    modelInfos[MODEL_TYPE::HICUM2].modelParams.emplace_back( "zetarbx", 217, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::TEMPERATURE, "0", "0", "Temperature exponent of external base resistance" );
    modelInfos[MODEL_TYPE::HICUM2].modelParams.emplace_back( "zetarcx", 218, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::TEMPERATURE, "0", "0", "Temperature exponent of external collector resistance" );
    modelInfos[MODEL_TYPE::HICUM2].modelParams.emplace_back( "zetare", 219, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::TEMPERATURE, "0", "0", "Temperature exponent of emitter resistance" );
    modelInfos[MODEL_TYPE::HICUM2].modelParams.emplace_back( "zetacx", 220, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::TEMPERATURE, "1", "1", "Temperature exponent of mobility in substrate transistor transit time" );
    modelInfos[MODEL_TYPE::HICUM2].modelParams.emplace_back( "vge", 221, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "V", SIM_MODEL::PARAM::CATEGORY::DC, "1.17", "1.17", "Effective emitter bandgap voltage" );
    modelInfos[MODEL_TYPE::HICUM2].modelParams.emplace_back( "vgc", 222, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "V", SIM_MODEL::PARAM::CATEGORY::DC, "1.17", "1.17", "Effective collector bandgap voltage" );
    modelInfos[MODEL_TYPE::HICUM2].modelParams.emplace_back( "vgs", 223, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "V", SIM_MODEL::PARAM::CATEGORY::DC, "1.17", "1.17", "Effective substrate bandgap voltage" );
    modelInfos[MODEL_TYPE::HICUM2].modelParams.emplace_back( "f1vg", 224, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::DC, "-0.000102377", "-0.000102377", "Coefficient K1 in T-dependent band-gap equation" );
    modelInfos[MODEL_TYPE::HICUM2].modelParams.emplace_back( "f2vg", 225, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::DC, "0.00043215", "0.00043215", "Coefficient K2 in T-dependent band-gap equation" );
    modelInfos[MODEL_TYPE::HICUM2].modelParams.emplace_back( "zetact", 226, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::TEMPERATURE, "3", "3", "Exponent coefficient in transfer current temperature dependence" );
    modelInfos[MODEL_TYPE::HICUM2].modelParams.emplace_back( "zetabet", 227, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::TEMPERATURE, "3.5", "3.5", "Exponent coefficient in B-E junction current temperature dependence" );
    modelInfos[MODEL_TYPE::HICUM2].modelParams.emplace_back( "alb", 228, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "A", SIM_MODEL::PARAM::CATEGORY::DC, "0", "0", "Relative TC of forward current gain for V2.1 model" );
    modelInfos[MODEL_TYPE::HICUM2].modelParams.emplace_back( "dvgbe", 229, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::DC, "0", "0", "Bandgap difference between B and B-E junction used for hjEi0 and hf0" );
    modelInfos[MODEL_TYPE::HICUM2].modelParams.emplace_back( "zetahjei", 230, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "°C", SIM_MODEL::PARAM::CATEGORY::TEMPERATURE, "1", "1", "Temperature coefficient for ahjEi" );
    modelInfos[MODEL_TYPE::HICUM2].modelParams.emplace_back( "zetavgbe", 231, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "°C", SIM_MODEL::PARAM::CATEGORY::TEMPERATURE, "1", "1", "Temperature coefficient for hjEi0" );
    modelInfos[MODEL_TYPE::HICUM2].modelParams.emplace_back( "flsh", 232, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_INT, "", SIM_MODEL::PARAM::CATEGORY::DC, "0", "0", "Flag for turning on and off self-heating effect" );
    modelInfos[MODEL_TYPE::HICUM2].modelParams.emplace_back( "rth", 233, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "ohm", SIM_MODEL::PARAM::CATEGORY::TEMPERATURE, "0", "0", "Thermal resistance" );
    modelInfos[MODEL_TYPE::HICUM2].modelParams.emplace_back( "zetarth", 234, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "°C", SIM_MODEL::PARAM::CATEGORY::TEMPERATURE, "0", "0", "Temperature coefficient for Rth" );
    modelInfos[MODEL_TYPE::HICUM2].modelParams.emplace_back( "alrth", 235, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::DC, "0", "0", "First order relative TC of parameter Rth" );
    modelInfos[MODEL_TYPE::HICUM2].modelParams.emplace_back( "cth", 236, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "F", SIM_MODEL::PARAM::CATEGORY::DC, "0", "0", "Thermal capacitance" );
    modelInfos[MODEL_TYPE::HICUM2].modelParams.emplace_back( "flcomp", 237, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::DC, "0", "0", "Flag for compatibility with v2.1 model (0=v2.1)" );
    modelInfos[MODEL_TYPE::HICUM2].modelParams.emplace_back( "vbe_max", 238, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "V", SIM_MODEL::PARAM::CATEGORY::LIMITING_VALUES, "1e+99", "1e+99", "maximum voltage B-E junction" );
    modelInfos[MODEL_TYPE::HICUM2].modelParams.emplace_back( "vbc_max", 239, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "V", SIM_MODEL::PARAM::CATEGORY::LIMITING_VALUES, "1e+99", "1e+99", "maximum voltage B-C junction" );
    modelInfos[MODEL_TYPE::HICUM2].modelParams.emplace_back( "vce_max", 240, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "V", SIM_MODEL::PARAM::CATEGORY::LIMITING_VALUES, "1e+99", "1e+99", "maximum voltage C-E branch" );
    // Instance parameters
    modelInfos[MODEL_TYPE::HICUM2].instanceParams.emplace_back( "area", 1, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "Area factor", true );
    modelInfos[MODEL_TYPE::HICUM2].instanceParams.emplace_back( "off", 2, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_BOOL, "", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "Device initially off", true );
    modelInfos[MODEL_TYPE::HICUM2].instanceParams.emplace_back( "ic", 3, SIM_MODEL::PARAM::DIR_IN, SIM_VALUE::TYPE_FLOAT_VECTOR, "", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "Initial condition vector", true );
    modelInfos[MODEL_TYPE::HICUM2].instanceParams.emplace_back( "m", 6, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::GEOMETRY, "", "", "Multiplier", true );
    modelInfos[MODEL_TYPE::HICUM2].instanceParams.emplace_back( "temp", 4, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "°C", SIM_MODEL::PARAM::CATEGORY::PRINCIPAL, "", "", "Instance temperature", true );
    modelInfos[MODEL_TYPE::HICUM2].instanceParams.emplace_back( "dt", 5, SIM_MODEL::PARAM::DIR_IN, SIM_VALUE::TYPE_FLOAT, "°C", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "Instance delta temperature", true );
    modelInfos[MODEL_TYPE::HICUM2].instanceParams.emplace_back( "tk", 264, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "°C", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "Actual device temperature", true );
    modelInfos[MODEL_TYPE::HICUM2].instanceParams.emplace_back( "dtsh", 265, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "°C", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "Temperature increase due to self-heating", true );
    modelInfos[MODEL_TYPE::HICUM2].instanceParams.emplace_back( "it", 284, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "A", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "transfer current", true );
    modelInfos[MODEL_TYPE::HICUM2].instanceParams.emplace_back( "collnode", 251, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_INT, "", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "Number of collector node", true );
    modelInfos[MODEL_TYPE::HICUM2].instanceParams.emplace_back( "basenode", 252, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_INT, "", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "Number of base node", true );
    modelInfos[MODEL_TYPE::HICUM2].instanceParams.emplace_back( "emitnode", 253, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_INT, "", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "Number of emitter node", true );
    modelInfos[MODEL_TYPE::HICUM2].instanceParams.emplace_back( "subsnode", 254, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_INT, "", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "Number of substrate node", true );
    modelInfos[MODEL_TYPE::HICUM2].instanceParams.emplace_back( "tempnode", 255, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_INT, "", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "Number of temperature node", true );
    modelInfos[MODEL_TYPE::HICUM2].instanceParams.emplace_back( "collcinode", 256, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_INT, "", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "Internal collector node", true );
    modelInfos[MODEL_TYPE::HICUM2].instanceParams.emplace_back( "basebpnode", 257, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_INT, "", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "External base node", true );
    modelInfos[MODEL_TYPE::HICUM2].instanceParams.emplace_back( "basebinode", 258, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_INT, "", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "Internal base node", true );
    modelInfos[MODEL_TYPE::HICUM2].instanceParams.emplace_back( "emiteinode", 259, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_INT, "", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "Internal emitter node", true );
    modelInfos[MODEL_TYPE::HICUM2].instanceParams.emplace_back( "subssinode", 260, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_INT, "", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "Internal substrate node", true );
    modelInfos[MODEL_TYPE::HICUM2].instanceParams.emplace_back( "xfnode", 261, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_INT, "", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "Internal phase node xf", true );
    modelInfos[MODEL_TYPE::HICUM2].instanceParams.emplace_back( "xf1node", 262, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_INT, "", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "Internal phase node xf1", true );
    modelInfos[MODEL_TYPE::HICUM2].instanceParams.emplace_back( "xf2node", 263, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_INT, "", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "Internal phase node xf2", true );
    modelInfos[MODEL_TYPE::HICUM2].instanceParams.emplace_back( "vbe", 266, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_FLOAT, "V", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "External BE voltage", true );
    modelInfos[MODEL_TYPE::HICUM2].instanceParams.emplace_back( "vbbp", 267, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_FLOAT, "V", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "BBP voltage", true );
    modelInfos[MODEL_TYPE::HICUM2].instanceParams.emplace_back( "vbc", 268, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_FLOAT, "V", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "External BC voltage", true );
    modelInfos[MODEL_TYPE::HICUM2].instanceParams.emplace_back( "vce", 269, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_FLOAT, "V", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "External CE voltage", true );
    modelInfos[MODEL_TYPE::HICUM2].instanceParams.emplace_back( "vsc", 270, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_FLOAT, "V", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "External SC voltage", true );
    modelInfos[MODEL_TYPE::HICUM2].instanceParams.emplace_back( "vbiei", 271, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_FLOAT, "V", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "Internal BE voltage", true );
    modelInfos[MODEL_TYPE::HICUM2].instanceParams.emplace_back( "vbpbi", 272, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_FLOAT, "V", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "Peripheral Base to internal Base voltage", true );
    modelInfos[MODEL_TYPE::HICUM2].instanceParams.emplace_back( "vbici", 273, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_FLOAT, "V", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "Internal BC voltage", true );
    modelInfos[MODEL_TYPE::HICUM2].instanceParams.emplace_back( "vciei", 274, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_FLOAT, "V", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "Internal CE voltage", true );
    modelInfos[MODEL_TYPE::HICUM2].instanceParams.emplace_back( "ic", 275, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_FLOAT, "A", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "Collector current", true );
    modelInfos[MODEL_TYPE::HICUM2].instanceParams.emplace_back( "iavl", 276, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_FLOAT, "A", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "Avalanche current", true );
    modelInfos[MODEL_TYPE::HICUM2].instanceParams.emplace_back( "ib", 277, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_FLOAT, "A", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "Base current", true );
    modelInfos[MODEL_TYPE::HICUM2].instanceParams.emplace_back( "ibei", 280, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_FLOAT, "A", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "Intenral Base Emitter current", true );
    modelInfos[MODEL_TYPE::HICUM2].instanceParams.emplace_back( "ibci", 281, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_FLOAT, "A", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "Internal Base Collector current", true );
    modelInfos[MODEL_TYPE::HICUM2].instanceParams.emplace_back( "ie", 278, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_FLOAT, "A", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "Emitter current", true );
    modelInfos[MODEL_TYPE::HICUM2].instanceParams.emplace_back( "is", 279, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_FLOAT, "A", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "Substrate current", true );
    modelInfos[MODEL_TYPE::HICUM2].instanceParams.emplace_back( "rcx_t", 282, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_FLOAT, "ohm", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "External (saturated) collector series resistance", true );
    modelInfos[MODEL_TYPE::HICUM2].instanceParams.emplace_back( "re_t", 283, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_FLOAT, "ohm", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "Emitter series resistance", true );
    modelInfos[MODEL_TYPE::HICUM2].instanceParams.emplace_back( "rbi", 285, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_FLOAT, "ohm", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "Internal base resistance as calculated in the model", true );
    modelInfos[MODEL_TYPE::HICUM2].instanceParams.emplace_back( "rb", 286, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_FLOAT, "ohm", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "Total base resistance as calculated in the model", true );
    modelInfos[MODEL_TYPE::HICUM2].instanceParams.emplace_back( "betadc", 287, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_FLOAT, "A", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "Common emitter forward current gain", true );
    modelInfos[MODEL_TYPE::HICUM2].instanceParams.emplace_back( "gmi", 288, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "Internal transconductance", true );
    modelInfos[MODEL_TYPE::HICUM2].instanceParams.emplace_back( "gms", 289, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "Transconductance of the parasitic substrate PNP", true );
    modelInfos[MODEL_TYPE::HICUM2].instanceParams.emplace_back( "rpii", 290, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_FLOAT, "ohm", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "Internal base-emitter (input) resistance", true );
    modelInfos[MODEL_TYPE::HICUM2].instanceParams.emplace_back( "rpix", 291, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_FLOAT, "ohm", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "External base-emitter (input) resistance", true );
    modelInfos[MODEL_TYPE::HICUM2].instanceParams.emplace_back( "rmui", 292, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_FLOAT, "ohm", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "Internal feedback resistance", true );
    modelInfos[MODEL_TYPE::HICUM2].instanceParams.emplace_back( "rmux", 293, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_FLOAT, "ohm", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "External feedback resistance", true );
    modelInfos[MODEL_TYPE::HICUM2].instanceParams.emplace_back( "roi", 294, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_FLOAT, "ohm", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "Output resistance", true );
    modelInfos[MODEL_TYPE::HICUM2].instanceParams.emplace_back( "cpii", 295, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_FLOAT, "F", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "Total internal BE capacitance", true );
    modelInfos[MODEL_TYPE::HICUM2].instanceParams.emplace_back( "cpix", 296, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_FLOAT, "F", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "Total external BE capacitance", true );
    modelInfos[MODEL_TYPE::HICUM2].instanceParams.emplace_back( "cmui", 297, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_FLOAT, "F", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "Total internal BC capacitance", true );
    modelInfos[MODEL_TYPE::HICUM2].instanceParams.emplace_back( "cmux", 298, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_FLOAT, "F", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "Total external BC capacitance", true );
    modelInfos[MODEL_TYPE::HICUM2].instanceParams.emplace_back( "ccs", 299, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_FLOAT, "F", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "CS junction capacitance", true );
    modelInfos[MODEL_TYPE::HICUM2].instanceParams.emplace_back( "betaac", 300, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_FLOAT, "A", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "Small signal current gain", true );
    modelInfos[MODEL_TYPE::HICUM2].instanceParams.emplace_back( "crbi", 301, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_FLOAT, "F", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "Shunt capacitance across RBI as calculated in the model", true );
    modelInfos[MODEL_TYPE::HICUM2].instanceParams.emplace_back( "tf", 302, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_FLOAT, "s", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "Forward transit time", true );
    modelInfos[MODEL_TYPE::HICUM2].instanceParams.emplace_back( "ft", 303, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_FLOAT, "Hz", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "Transit frequency", true );
    modelInfos[MODEL_TYPE::HICUM2].instanceParams.emplace_back( "ick", 304, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_FLOAT, "Hz", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "Transit frequency", true );
    modelInfos[MODEL_TYPE::HICUM2].instanceParams.emplace_back( "p", 305, SIM_MODEL::PARAM::DIR_OUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "Power dissipation", true );
}