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


void NGSPICE_MODEL_INFO_MAP::addBSIM1()
{
    modelInfos[MODEL_TYPE::BSIM1] = { "BSIM1", "NMOS", "PMOS", { "D", "G", "S", "B" }, "Berkeley Short Channel IGFET Model", {}, {} };
    // Model parameters
    modelInfos[MODEL_TYPE::BSIM1].modelParams.emplace_back( "vfb",  101, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "V", SIM_MODEL::PARAM::CATEGORY::DC, "0", "0", "Flat band voltage" );
    modelInfos[MODEL_TYPE::BSIM1].modelParams.emplace_back( "lvfb",  102, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "m", SIM_MODEL::PARAM::CATEGORY::DC, "0", "0", "Length dependence of vfb" );
    modelInfos[MODEL_TYPE::BSIM1].modelParams.emplace_back( "wvfb",  103, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "m", SIM_MODEL::PARAM::CATEGORY::DC, "0", "0", "Width dependence of vfb" );
    modelInfos[MODEL_TYPE::BSIM1].modelParams.emplace_back( "phi",  104, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "V", SIM_MODEL::PARAM::CATEGORY::DC, "0", "0", "Strong inversion surface potential" );
    modelInfos[MODEL_TYPE::BSIM1].modelParams.emplace_back( "lphi",  105, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "m", SIM_MODEL::PARAM::CATEGORY::DC, "0", "0", "Length dependence of phi" );
    modelInfos[MODEL_TYPE::BSIM1].modelParams.emplace_back( "wphi",  106, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "m", SIM_MODEL::PARAM::CATEGORY::DC, "0", "0", "Width dependence of phi" );
    modelInfos[MODEL_TYPE::BSIM1].modelParams.emplace_back( "k1",  107, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "sqrt V", SIM_MODEL::PARAM::CATEGORY::DC, "0", "0", "Bulk effect coefficient 1" );
    modelInfos[MODEL_TYPE::BSIM1].modelParams.emplace_back( "lk1",  108, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "m", SIM_MODEL::PARAM::CATEGORY::DC, "0", "0", "Length dependence of k1" );
    modelInfos[MODEL_TYPE::BSIM1].modelParams.emplace_back( "wk1",  109, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "m", SIM_MODEL::PARAM::CATEGORY::DC, "0", "0", "Width dependence of k1" );
    modelInfos[MODEL_TYPE::BSIM1].modelParams.emplace_back( "k2",  110, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::DC, "0", "0", "Bulk effect coefficient 2" );
    modelInfos[MODEL_TYPE::BSIM1].modelParams.emplace_back( "lk2",  111, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "m", SIM_MODEL::PARAM::CATEGORY::DC, "0", "0", "Length dependence of k2" );
    modelInfos[MODEL_TYPE::BSIM1].modelParams.emplace_back( "wk2",  112, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "m", SIM_MODEL::PARAM::CATEGORY::DC, "0", "0", "Width dependence of k2" );
    modelInfos[MODEL_TYPE::BSIM1].modelParams.emplace_back( "eta",  113, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "V", SIM_MODEL::PARAM::CATEGORY::DC, "0", "0", "VDS dependence of threshold voltage" );
    modelInfos[MODEL_TYPE::BSIM1].modelParams.emplace_back( "leta",  114, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "m", SIM_MODEL::PARAM::CATEGORY::DC, "0", "0", "Length dependence of eta" );
    modelInfos[MODEL_TYPE::BSIM1].modelParams.emplace_back( "weta",  115, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "m", SIM_MODEL::PARAM::CATEGORY::DC, "0", "0", "Width dependence of eta" );
    modelInfos[MODEL_TYPE::BSIM1].modelParams.emplace_back( "x2e",  116, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::DC, "0", "0", "VBS dependence of eta" );
    modelInfos[MODEL_TYPE::BSIM1].modelParams.emplace_back( "lx2e",  117, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "m", SIM_MODEL::PARAM::CATEGORY::DC, "0", "0", "Length dependence of x2e" );
    modelInfos[MODEL_TYPE::BSIM1].modelParams.emplace_back( "wx2e",  118, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "m", SIM_MODEL::PARAM::CATEGORY::DC, "0", "0", "Width dependence of x2e" );
    modelInfos[MODEL_TYPE::BSIM1].modelParams.emplace_back( "x3e",  119, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::DC, "0", "0", "VDS dependence of eta" );
    modelInfos[MODEL_TYPE::BSIM1].modelParams.emplace_back( "lx3e",  120, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "m", SIM_MODEL::PARAM::CATEGORY::DC, "0", "0", "Length dependence of x3e" );
    modelInfos[MODEL_TYPE::BSIM1].modelParams.emplace_back( "wx3e",  121, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "m", SIM_MODEL::PARAM::CATEGORY::DC, "0", "0", "Width dependence of x3e" );
    modelInfos[MODEL_TYPE::BSIM1].modelParams.emplace_back( "dl",  122, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "um", SIM_MODEL::PARAM::CATEGORY::DC, "0", "0", "Channel length reduction in um" );
    modelInfos[MODEL_TYPE::BSIM1].modelParams.emplace_back( "dw",  123, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "um", SIM_MODEL::PARAM::CATEGORY::DC, "0", "0", "Channel width reduction in um" );
    modelInfos[MODEL_TYPE::BSIM1].modelParams.emplace_back( "muz",  124, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "cm²/V s", SIM_MODEL::PARAM::CATEGORY::DC, "0", "0", "Zero field mobility at VDS=0 VGS=VTH" );
    modelInfos[MODEL_TYPE::BSIM1].modelParams.emplace_back( "x2mz",  125, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::DC, "0", "0", "VBS dependence of muz" );
    modelInfos[MODEL_TYPE::BSIM1].modelParams.emplace_back( "lx2mz",  126, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "m", SIM_MODEL::PARAM::CATEGORY::DC, "0", "0", "Length dependence of x2mz" );
    modelInfos[MODEL_TYPE::BSIM1].modelParams.emplace_back( "wx2mz",  127, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "m", SIM_MODEL::PARAM::CATEGORY::DC, "0", "0", "Width dependence of x2mz" );
    modelInfos[MODEL_TYPE::BSIM1].modelParams.emplace_back( "mus",  128, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "1/V", SIM_MODEL::PARAM::CATEGORY::DC, "0", "0", "Mobility at VDS=VDD VGS=VTH, channel length modulation" );
    modelInfos[MODEL_TYPE::BSIM1].modelParams.emplace_back( "lmus",  129, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "m", SIM_MODEL::PARAM::CATEGORY::DC, "0", "0", "Length dependence of mus" );
    modelInfos[MODEL_TYPE::BSIM1].modelParams.emplace_back( "wmus",  130, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "m", SIM_MODEL::PARAM::CATEGORY::DC, "0", "0", "Width dependence of mus" );
    modelInfos[MODEL_TYPE::BSIM1].modelParams.emplace_back( "x2ms",  131, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::DC, "0", "0", "VBS dependence of mus" );
    modelInfos[MODEL_TYPE::BSIM1].modelParams.emplace_back( "lx2ms",  132, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "m", SIM_MODEL::PARAM::CATEGORY::DC, "0", "0", "Length dependence of x2ms" );
    modelInfos[MODEL_TYPE::BSIM1].modelParams.emplace_back( "wx2ms",  133, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "m", SIM_MODEL::PARAM::CATEGORY::DC, "0", "0", "Width dependence of x2ms" );
    modelInfos[MODEL_TYPE::BSIM1].modelParams.emplace_back( "x3ms",  134, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::DC, "0", "0", "VDS dependence of mus" );
    modelInfos[MODEL_TYPE::BSIM1].modelParams.emplace_back( "lx3ms",  135, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "m", SIM_MODEL::PARAM::CATEGORY::DC, "0", "0", "Length dependence of x3ms" );
    modelInfos[MODEL_TYPE::BSIM1].modelParams.emplace_back( "wx3ms",  136, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "m", SIM_MODEL::PARAM::CATEGORY::DC, "0", "0", "Width dependence of x3ms" );
    modelInfos[MODEL_TYPE::BSIM1].modelParams.emplace_back( "u0",  137, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::DC, "0", "0", "VGS dependence of mobility" );
    modelInfos[MODEL_TYPE::BSIM1].modelParams.emplace_back( "lu0",  138, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "m", SIM_MODEL::PARAM::CATEGORY::DC, "0", "0", "Length dependence of u0" );
    modelInfos[MODEL_TYPE::BSIM1].modelParams.emplace_back( "wu0",  139, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "m", SIM_MODEL::PARAM::CATEGORY::DC, "0", "0", "Width dependence of u0" );
    modelInfos[MODEL_TYPE::BSIM1].modelParams.emplace_back( "x2u0",  140, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::DC, "0", "0", "VBS dependence of u0" );
    modelInfos[MODEL_TYPE::BSIM1].modelParams.emplace_back( "lx2u0",  141, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "m", SIM_MODEL::PARAM::CATEGORY::DC, "0", "0", "Length dependence of x2u0" );
    modelInfos[MODEL_TYPE::BSIM1].modelParams.emplace_back( "wx2u0",  142, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "m", SIM_MODEL::PARAM::CATEGORY::DC, "0", "0", "Width dependence of x2u0" );
    modelInfos[MODEL_TYPE::BSIM1].modelParams.emplace_back( "u1",  143, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "m/s", SIM_MODEL::PARAM::CATEGORY::DC, "0", "0", "VDS depence of mobility, velocity saturation" );
    modelInfos[MODEL_TYPE::BSIM1].modelParams.emplace_back( "lu1",  144, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "m", SIM_MODEL::PARAM::CATEGORY::DC, "0", "0", "Length dependence of u1" );
    modelInfos[MODEL_TYPE::BSIM1].modelParams.emplace_back( "wu1",  145, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "m", SIM_MODEL::PARAM::CATEGORY::DC, "0", "0", "Width dependence of u1" );
    modelInfos[MODEL_TYPE::BSIM1].modelParams.emplace_back( "x2u1",  146, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::DC, "0", "0", "VBS depence of u1" );
    modelInfos[MODEL_TYPE::BSIM1].modelParams.emplace_back( "lx2u1",  147, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "m", SIM_MODEL::PARAM::CATEGORY::DC, "0", "0", "Length depence of x2u1" );
    modelInfos[MODEL_TYPE::BSIM1].modelParams.emplace_back( "wx2u1",  148, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "m", SIM_MODEL::PARAM::CATEGORY::DC, "0", "0", "Width depence of x2u1" );
    modelInfos[MODEL_TYPE::BSIM1].modelParams.emplace_back( "x3u1",  149, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "um/V²", SIM_MODEL::PARAM::CATEGORY::DC, "0", "0", "VDS depence of u1" );
    modelInfos[MODEL_TYPE::BSIM1].modelParams.emplace_back( "lx3u1",  150, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "m", SIM_MODEL::PARAM::CATEGORY::DC, "0", "0", "Length dependence of x3u1" );
    modelInfos[MODEL_TYPE::BSIM1].modelParams.emplace_back( "wx3u1",  151, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "m", SIM_MODEL::PARAM::CATEGORY::DC, "0", "0", "Width depence of x3u1" );
    modelInfos[MODEL_TYPE::BSIM1].modelParams.emplace_back( "n0",  152, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::DC, "0", "0", "Subthreshold slope" );
    modelInfos[MODEL_TYPE::BSIM1].modelParams.emplace_back( "ln0",  153, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "m", SIM_MODEL::PARAM::CATEGORY::DC, "0", "0", "Length dependence of n0" );
    modelInfos[MODEL_TYPE::BSIM1].modelParams.emplace_back( "wn0",  154, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "m", SIM_MODEL::PARAM::CATEGORY::DC, "0", "0", "Width dependence of n0" );
    modelInfos[MODEL_TYPE::BSIM1].modelParams.emplace_back( "nb",  155, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::DC, "0", "0", "VBS dependence of subthreshold slope" );
    modelInfos[MODEL_TYPE::BSIM1].modelParams.emplace_back( "lnb",  156, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "m", SIM_MODEL::PARAM::CATEGORY::DC, "0", "0", "Length dependence of nb" );
    modelInfos[MODEL_TYPE::BSIM1].modelParams.emplace_back( "wnb",  157, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "m", SIM_MODEL::PARAM::CATEGORY::DC, "0", "0", "Width dependence of nb" );
    modelInfos[MODEL_TYPE::BSIM1].modelParams.emplace_back( "nd",  158, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::DC, "0", "0", "VDS dependence of subthreshold slope" );
    modelInfos[MODEL_TYPE::BSIM1].modelParams.emplace_back( "lnd",  159, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "m", SIM_MODEL::PARAM::CATEGORY::DC, "0", "0", "Length dependence of nd" );
    modelInfos[MODEL_TYPE::BSIM1].modelParams.emplace_back( "wnd",  160, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "m", SIM_MODEL::PARAM::CATEGORY::DC, "0", "0", "Width dependence of nd" );
    modelInfos[MODEL_TYPE::BSIM1].modelParams.emplace_back( "tox",  161, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "um", SIM_MODEL::PARAM::CATEGORY::DC, "0", "0", "Gate oxide thickness in um" );
    modelInfos[MODEL_TYPE::BSIM1].modelParams.emplace_back( "temp",  162, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "°C", SIM_MODEL::PARAM::CATEGORY::PRINCIPAL, "0", "0", "Temperature in degree Celcius" );
    modelInfos[MODEL_TYPE::BSIM1].modelParams.emplace_back( "vdd",  163, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "V", SIM_MODEL::PARAM::CATEGORY::DC, "0", "0", "Supply voltage to specify mus" );
    modelInfos[MODEL_TYPE::BSIM1].modelParams.emplace_back( "cgso",  164, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "F/m", SIM_MODEL::PARAM::CATEGORY::CAPACITANCE, "0", "0", "Gate source overlap capacitance per unit channel width(m)" );
    modelInfos[MODEL_TYPE::BSIM1].modelParams.emplace_back( "cgdo",  165, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "F/m", SIM_MODEL::PARAM::CATEGORY::CAPACITANCE, "0", "0", "Gate drain overlap capacitance per unit channel width(m)" );
    modelInfos[MODEL_TYPE::BSIM1].modelParams.emplace_back( "cgbo",  166, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "F/m", SIM_MODEL::PARAM::CATEGORY::CAPACITANCE, "0", "0", "Gate bulk overlap capacitance per unit channel length(m)" );
    modelInfos[MODEL_TYPE::BSIM1].modelParams.emplace_back( "xpart",  167, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_BOOL, "", SIM_MODEL::PARAM::CATEGORY::FLAGS, "0", "0", "Flag for channel charge partitioning" );
    modelInfos[MODEL_TYPE::BSIM1].modelParams.emplace_back( "rsh",  168, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "Ω/m", SIM_MODEL::PARAM::CATEGORY::DC, "0", "0", "Source drain diffusion sheet resistance in ohm per square" );
    modelInfos[MODEL_TYPE::BSIM1].modelParams.emplace_back( "js",  169, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "A", SIM_MODEL::PARAM::CATEGORY::DC, "0", "0", "Source drain junction saturation current per unit area" );
    modelInfos[MODEL_TYPE::BSIM1].modelParams.emplace_back( "pb",  170, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "V", SIM_MODEL::PARAM::CATEGORY::DC, "0.1", "0.1", "Source drain junction built in potential" );
    modelInfos[MODEL_TYPE::BSIM1].modelParams.emplace_back( "mj",  171, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "F", SIM_MODEL::PARAM::CATEGORY::CAPACITANCE, "0", "0", "Source drain bottom junction capacitance grading coefficient" );
    modelInfos[MODEL_TYPE::BSIM1].modelParams.emplace_back( "pbsw",  172, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "F", SIM_MODEL::PARAM::CATEGORY::CAPACITANCE, "0.1", "0.1", "Source drain side junction capacitance built in potential" );
    modelInfos[MODEL_TYPE::BSIM1].modelParams.emplace_back( "mjsw",  173, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "F", SIM_MODEL::PARAM::CATEGORY::CAPACITANCE, "0", "0", "Source drain side junction capacitance grading coefficient" );
    modelInfos[MODEL_TYPE::BSIM1].modelParams.emplace_back( "cj",  174, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "F", SIM_MODEL::PARAM::CATEGORY::CAPACITANCE, "0", "0", "Source drain bottom junction capacitance per unit area" );
    modelInfos[MODEL_TYPE::BSIM1].modelParams.emplace_back( "cjsw",  175, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "F", SIM_MODEL::PARAM::CATEGORY::CAPACITANCE, "0", "0", "Source drain side junction capacitance per unit area" );
    modelInfos[MODEL_TYPE::BSIM1].modelParams.emplace_back( "wdf",  176, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "um", SIM_MODEL::PARAM::CATEGORY::DC, "0", "0", "Default width of source drain diffusion in um" );
    modelInfos[MODEL_TYPE::BSIM1].modelParams.emplace_back( "dell",  177, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "m", SIM_MODEL::PARAM::CATEGORY::DC, "0", "0", "Length reduction of source drain diffusion" );
    modelInfos[MODEL_TYPE::BSIM1].modelParams.emplace_back( "kf",  180, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::NOISE, "0", "0", "Flicker noise coefficient" );
    modelInfos[MODEL_TYPE::BSIM1].modelParams.emplace_back( "af",  181, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::NOISE, "1", "1", "Flicker noise exponent" );
    modelInfos[MODEL_TYPE::BSIM1].modelParams.emplace_back( "nmos",  178, SIM_MODEL::PARAM::DIR_IN, SIM_VALUE::TYPE_BOOL, "", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "Flag to indicate NMOS" );
    modelInfos[MODEL_TYPE::BSIM1].modelParams.emplace_back( "pmos",  179, SIM_MODEL::PARAM::DIR_IN, SIM_VALUE::TYPE_BOOL, "", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "Flag to indicate PMOS" );
    // Instance parameters
    modelInfos[MODEL_TYPE::BSIM1].instanceParams.emplace_back( "l",  2, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "m", SIM_MODEL::PARAM::CATEGORY::GEOMETRY, "", "", "Length", true );
    modelInfos[MODEL_TYPE::BSIM1].instanceParams.emplace_back( "w",  1, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "m", SIM_MODEL::PARAM::CATEGORY::GEOMETRY, "", "", "Width", true );
    modelInfos[MODEL_TYPE::BSIM1].instanceParams.emplace_back( "m",  14, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::GEOMETRY, "", "", "Parallel Multiplier", true );
    modelInfos[MODEL_TYPE::BSIM1].instanceParams.emplace_back( "ad",  4, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::GEOMETRY, "", "", "Drain area", true );
    modelInfos[MODEL_TYPE::BSIM1].instanceParams.emplace_back( "as",  3, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::GEOMETRY, "", "", "Source area", true );
    modelInfos[MODEL_TYPE::BSIM1].instanceParams.emplace_back( "pd",  6, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::GEOMETRY, "", "", "Drain perimeter", true );
    modelInfos[MODEL_TYPE::BSIM1].instanceParams.emplace_back( "ps",  5, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::GEOMETRY, "", "", "Source perimeter", true );
    modelInfos[MODEL_TYPE::BSIM1].instanceParams.emplace_back( "nrd",  8, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::GEOMETRY, "", "", "Number of squares in drain", true );
    modelInfos[MODEL_TYPE::BSIM1].instanceParams.emplace_back( "nrs",  7, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::GEOMETRY, "", "", "Number of squares in source", true );
    modelInfos[MODEL_TYPE::BSIM1].instanceParams.emplace_back( "off",  9, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_BOOL, "", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "Device is initially off", true );
    modelInfos[MODEL_TYPE::BSIM1].instanceParams.emplace_back( "vds",  11, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "V", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "Initial D-S voltage", true );
    modelInfos[MODEL_TYPE::BSIM1].instanceParams.emplace_back( "vgs",  12, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "V", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "Initial G-S voltage", true );
    modelInfos[MODEL_TYPE::BSIM1].instanceParams.emplace_back( "vbs",  10, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "V", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "Initial B-S voltage", true );
    modelInfos[MODEL_TYPE::BSIM1].instanceParams.emplace_back( "ic",  13, SIM_MODEL::PARAM::DIR_IN, SIM_VALUE::TYPE_FLOAT_VECTOR /*SIM_VALUE::TYPE::VECTOR*/, "", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "Vector of DS,GS,BS initial voltages", true );
}