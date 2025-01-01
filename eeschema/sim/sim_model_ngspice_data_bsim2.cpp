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


void NGSPICE_MODEL_INFO_MAP::addBSIM2()
{
    modelInfos[MODEL_TYPE::BSIM2] = { "BSIM2", "NMOS", "PMOS", { "D", "G", "S", "B" }, "Berkeley Short Channel IGFET Model", {}, {} };
    // Model parameters
    modelInfos[MODEL_TYPE::BSIM2].modelParams.emplace_back( "vfb",  101, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "V", SIM_MODEL::PARAM::CATEGORY::DC, "-1", "-1", "Flat band voltage" );
    modelInfos[MODEL_TYPE::BSIM2].modelParams.emplace_back( "lvfb",  102, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "m", SIM_MODEL::PARAM::CATEGORY::DC, "0", "0", "Length dependence of vfb" );
    modelInfos[MODEL_TYPE::BSIM2].modelParams.emplace_back( "wvfb",  103, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "m", SIM_MODEL::PARAM::CATEGORY::DC, "0", "0", "Width dependence of vfb" );
    modelInfos[MODEL_TYPE::BSIM2].modelParams.emplace_back( "phi",  104, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "V", SIM_MODEL::PARAM::CATEGORY::DC, "0.75", "0.75", "Strong inversion surface potential" );
    modelInfos[MODEL_TYPE::BSIM2].modelParams.emplace_back( "lphi",  105, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "m", SIM_MODEL::PARAM::CATEGORY::DC, "0", "0", "Length dependence of phi" );
    modelInfos[MODEL_TYPE::BSIM2].modelParams.emplace_back( "wphi",  106, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "m", SIM_MODEL::PARAM::CATEGORY::DC, "0", "0", "Width dependence of phi" );
    modelInfos[MODEL_TYPE::BSIM2].modelParams.emplace_back( "k1",  107, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "sqrt V", SIM_MODEL::PARAM::CATEGORY::DC, "0.8", "0.8", "Bulk effect coefficient 1" );
    modelInfos[MODEL_TYPE::BSIM2].modelParams.emplace_back( "lk1",  108, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "m", SIM_MODEL::PARAM::CATEGORY::DC, "0", "0", "Length dependence of k1" );
    modelInfos[MODEL_TYPE::BSIM2].modelParams.emplace_back( "wk1",  109, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "m", SIM_MODEL::PARAM::CATEGORY::DC, "0", "0", "Width dependence of k1" );
    modelInfos[MODEL_TYPE::BSIM2].modelParams.emplace_back( "k2",  110, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::DC, "0", "0", "Bulk effect coefficient 2" );
    modelInfos[MODEL_TYPE::BSIM2].modelParams.emplace_back( "lk2",  111, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "m", SIM_MODEL::PARAM::CATEGORY::DC, "0", "0", "Length dependence of k2" );
    modelInfos[MODEL_TYPE::BSIM2].modelParams.emplace_back( "wk2",  112, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "m", SIM_MODEL::PARAM::CATEGORY::DC, "0", "0", "Width dependence of k2" );
    modelInfos[MODEL_TYPE::BSIM2].modelParams.emplace_back( "eta0",  113, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "V", SIM_MODEL::PARAM::CATEGORY::DC, "0", "0", "VDS dependence of threshold voltage at VDD=0" );
    modelInfos[MODEL_TYPE::BSIM2].modelParams.emplace_back( "leta0",  114, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "m", SIM_MODEL::PARAM::CATEGORY::DC, "0", "0", "Length dependence of eta0" );
    modelInfos[MODEL_TYPE::BSIM2].modelParams.emplace_back( "weta0",  115, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "m", SIM_MODEL::PARAM::CATEGORY::DC, "0", "0", "Width dependence of eta0" );
    modelInfos[MODEL_TYPE::BSIM2].modelParams.emplace_back( "etab",  116, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::DC, "0", "0", "VBS dependence of eta" );
    modelInfos[MODEL_TYPE::BSIM2].modelParams.emplace_back( "letab",  117, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "m", SIM_MODEL::PARAM::CATEGORY::DC, "0", "0", "Length dependence of etab" );
    modelInfos[MODEL_TYPE::BSIM2].modelParams.emplace_back( "wetab",  118, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "m", SIM_MODEL::PARAM::CATEGORY::DC, "0", "0", "Width dependence of etab" );
    modelInfos[MODEL_TYPE::BSIM2].modelParams.emplace_back( "dl",  119, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "um", SIM_MODEL::PARAM::CATEGORY::DC, "1.23516e-322", "1.23516e-322", "Channel length reduction in um" );
    modelInfos[MODEL_TYPE::BSIM2].modelParams.emplace_back( "dw",  120, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "um", SIM_MODEL::PARAM::CATEGORY::DC, "1.23516e-322", "1.23516e-322", "Channel width reduction in um" );
    modelInfos[MODEL_TYPE::BSIM2].modelParams.emplace_back( "mu0",  121, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::DC, "400", "400", "Low-field mobility, at VDS=0 VGS=VTH" );
    modelInfos[MODEL_TYPE::BSIM2].modelParams.emplace_back( "mu0b",  122, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::DC, "0", "0", "VBS dependence of low-field mobility" );
    modelInfos[MODEL_TYPE::BSIM2].modelParams.emplace_back( "lmu0b",  123, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "m", SIM_MODEL::PARAM::CATEGORY::DC, "0", "0", "Length dependence of mu0b" );
    modelInfos[MODEL_TYPE::BSIM2].modelParams.emplace_back( "wmu0b",  124, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "m", SIM_MODEL::PARAM::CATEGORY::DC, "0", "0", "Width dependence of mu0b" );
    modelInfos[MODEL_TYPE::BSIM2].modelParams.emplace_back( "mus0",  125, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::DC, "500", "500", "Mobility at VDS=VDD VGS=VTH" );
    modelInfos[MODEL_TYPE::BSIM2].modelParams.emplace_back( "lmus0",  126, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "m", SIM_MODEL::PARAM::CATEGORY::DC, "0", "0", "Length dependence of mus0" );
    modelInfos[MODEL_TYPE::BSIM2].modelParams.emplace_back( "wmus0",  127, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "m", SIM_MODEL::PARAM::CATEGORY::DC, "0", "0", "Width dependence of mus" );
    modelInfos[MODEL_TYPE::BSIM2].modelParams.emplace_back( "musb",  128, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::DC, "0", "0", "VBS dependence of mus" );
    modelInfos[MODEL_TYPE::BSIM2].modelParams.emplace_back( "lmusb",  129, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "m", SIM_MODEL::PARAM::CATEGORY::DC, "0", "0", "Length dependence of musb" );
    modelInfos[MODEL_TYPE::BSIM2].modelParams.emplace_back( "wmusb",  130, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "m", SIM_MODEL::PARAM::CATEGORY::DC, "0", "0", "Width dependence of musb" );
    modelInfos[MODEL_TYPE::BSIM2].modelParams.emplace_back( "mu20",  131, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::DC, "1.5", "1.5", "VDS dependence of mu in tanh term" );
    modelInfos[MODEL_TYPE::BSIM2].modelParams.emplace_back( "lmu20",  132, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "m", SIM_MODEL::PARAM::CATEGORY::DC, "0", "0", "Length dependence of mu20" );
    modelInfos[MODEL_TYPE::BSIM2].modelParams.emplace_back( "wmu20",  133, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "m", SIM_MODEL::PARAM::CATEGORY::DC, "0", "0", "Width dependence of mu20" );
    modelInfos[MODEL_TYPE::BSIM2].modelParams.emplace_back( "mu2b",  134, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::DC, "0", "0", "VBS dependence of mu2" );
    modelInfos[MODEL_TYPE::BSIM2].modelParams.emplace_back( "lmu2b",  135, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "m", SIM_MODEL::PARAM::CATEGORY::DC, "0", "0", "Length dependence of mu2b" );
    modelInfos[MODEL_TYPE::BSIM2].modelParams.emplace_back( "wmu2b",  136, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "m", SIM_MODEL::PARAM::CATEGORY::DC, "0", "0", "Width dependence of mu2b" );
    modelInfos[MODEL_TYPE::BSIM2].modelParams.emplace_back( "mu2g",  137, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::DC, "0", "0", "VGS dependence of mu2" );
    modelInfos[MODEL_TYPE::BSIM2].modelParams.emplace_back( "lmu2g",  138, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "m", SIM_MODEL::PARAM::CATEGORY::DC, "0", "0", "Length dependence of mu2g" );
    modelInfos[MODEL_TYPE::BSIM2].modelParams.emplace_back( "wmu2g",  139, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "m", SIM_MODEL::PARAM::CATEGORY::DC, "0", "0", "Width dependence of mu2g" );
    modelInfos[MODEL_TYPE::BSIM2].modelParams.emplace_back( "mu30",  140, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::DC, "10", "10", "VDS dependence of mu in linear term" );
    modelInfos[MODEL_TYPE::BSIM2].modelParams.emplace_back( "lmu30",  141, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "m", SIM_MODEL::PARAM::CATEGORY::DC, "0", "0", "Length dependence of mu30" );
    modelInfos[MODEL_TYPE::BSIM2].modelParams.emplace_back( "wmu30",  142, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "m", SIM_MODEL::PARAM::CATEGORY::DC, "0", "0", "Width dependence of mu30" );
    modelInfos[MODEL_TYPE::BSIM2].modelParams.emplace_back( "mu3b",  143, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::DC, "0", "0", "VBS dependence of mu3" );
    modelInfos[MODEL_TYPE::BSIM2].modelParams.emplace_back( "lmu3b",  144, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "m", SIM_MODEL::PARAM::CATEGORY::DC, "0", "0", "Length dependence of mu3b" );
    modelInfos[MODEL_TYPE::BSIM2].modelParams.emplace_back( "wmu3b",  145, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "m", SIM_MODEL::PARAM::CATEGORY::DC, "0", "0", "Width dependence of mu3b" );
    modelInfos[MODEL_TYPE::BSIM2].modelParams.emplace_back( "mu3g",  146, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::DC, "0", "0", "VGS dependence of mu3" );
    modelInfos[MODEL_TYPE::BSIM2].modelParams.emplace_back( "lmu3g",  147, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "m", SIM_MODEL::PARAM::CATEGORY::DC, "0", "0", "Length dependence of mu3g" );
    modelInfos[MODEL_TYPE::BSIM2].modelParams.emplace_back( "wmu3g",  148, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "m", SIM_MODEL::PARAM::CATEGORY::DC, "0", "0", "Width dependence of mu3g" );
    modelInfos[MODEL_TYPE::BSIM2].modelParams.emplace_back( "mu40",  149, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::DC, "0", "0", "VDS dependence of mu in linear term" );
    modelInfos[MODEL_TYPE::BSIM2].modelParams.emplace_back( "lmu40",  150, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "m", SIM_MODEL::PARAM::CATEGORY::DC, "0", "0", "Length dependence of mu40" );
    modelInfos[MODEL_TYPE::BSIM2].modelParams.emplace_back( "wmu40",  151, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "m", SIM_MODEL::PARAM::CATEGORY::DC, "0", "0", "Width dependence of mu40" );
    modelInfos[MODEL_TYPE::BSIM2].modelParams.emplace_back( "mu4b",  152, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::DC, "0", "0", "VBS dependence of mu4" );
    modelInfos[MODEL_TYPE::BSIM2].modelParams.emplace_back( "lmu4b",  153, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "m", SIM_MODEL::PARAM::CATEGORY::DC, "0", "0", "Length dependence of mu4b" );
    modelInfos[MODEL_TYPE::BSIM2].modelParams.emplace_back( "wmu4b",  154, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "m", SIM_MODEL::PARAM::CATEGORY::DC, "0", "0", "Width dependence of mu4b" );
    modelInfos[MODEL_TYPE::BSIM2].modelParams.emplace_back( "mu4g",  155, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::DC, "0", "0", "VGS dependence of mu4" );
    modelInfos[MODEL_TYPE::BSIM2].modelParams.emplace_back( "lmu4g",  156, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "m", SIM_MODEL::PARAM::CATEGORY::DC, "0", "0", "Length dependence of mu4g" );
    modelInfos[MODEL_TYPE::BSIM2].modelParams.emplace_back( "wmu4g",  157, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "m", SIM_MODEL::PARAM::CATEGORY::DC, "0", "0", "Width dependence of mu4g" );
    modelInfos[MODEL_TYPE::BSIM2].modelParams.emplace_back( "ua0",  158, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::DC, "0.2", "0.2", "Linear VGS dependence of mobility" );
    modelInfos[MODEL_TYPE::BSIM2].modelParams.emplace_back( "lua0",  159, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "m", SIM_MODEL::PARAM::CATEGORY::DC, "0", "0", "Length dependence of ua0" );
    modelInfos[MODEL_TYPE::BSIM2].modelParams.emplace_back( "wua0",  160, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "m", SIM_MODEL::PARAM::CATEGORY::DC, "0", "0", "Width dependence of ua0" );
    modelInfos[MODEL_TYPE::BSIM2].modelParams.emplace_back( "uab",  161, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::DC, "0", "0", "VBS dependence of ua" );
    modelInfos[MODEL_TYPE::BSIM2].modelParams.emplace_back( "luab",  162, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "m", SIM_MODEL::PARAM::CATEGORY::DC, "0", "0", "Length dependence of uab" );
    modelInfos[MODEL_TYPE::BSIM2].modelParams.emplace_back( "wuab",  163, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "m", SIM_MODEL::PARAM::CATEGORY::DC, "0", "0", "Width dependence of uab" );
    modelInfos[MODEL_TYPE::BSIM2].modelParams.emplace_back( "ub0",  164, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::DC, "0", "0", "Quadratic VGS dependence of mobility" );
    modelInfos[MODEL_TYPE::BSIM2].modelParams.emplace_back( "lub0",  165, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "m", SIM_MODEL::PARAM::CATEGORY::DC, "0", "0", "Length dependence of ub0" );
    modelInfos[MODEL_TYPE::BSIM2].modelParams.emplace_back( "wub0",  166, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "m", SIM_MODEL::PARAM::CATEGORY::DC, "0", "0", "Width dependence of ub0" );
    modelInfos[MODEL_TYPE::BSIM2].modelParams.emplace_back( "ubb",  167, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::DC, "0", "0", "VBS dependence of ub" );
    modelInfos[MODEL_TYPE::BSIM2].modelParams.emplace_back( "lubb",  168, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "m", SIM_MODEL::PARAM::CATEGORY::DC, "0", "0", "Length dependence of ubb" );
    modelInfos[MODEL_TYPE::BSIM2].modelParams.emplace_back( "wubb",  169, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "m", SIM_MODEL::PARAM::CATEGORY::DC, "0", "0", "Width dependence of ubb" );
    modelInfos[MODEL_TYPE::BSIM2].modelParams.emplace_back( "u10",  170, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::DC, "0.1", "0.1", "VDS depence of mobility" );
    modelInfos[MODEL_TYPE::BSIM2].modelParams.emplace_back( "lu10",  171, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "m", SIM_MODEL::PARAM::CATEGORY::DC, "0", "0", "Length dependence of u10" );
    modelInfos[MODEL_TYPE::BSIM2].modelParams.emplace_back( "wu10",  172, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "m", SIM_MODEL::PARAM::CATEGORY::DC, "0", "0", "Width dependence of u10" );
    modelInfos[MODEL_TYPE::BSIM2].modelParams.emplace_back( "u1b",  173, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::DC, "0", "0", "VBS depence of u1" );
    modelInfos[MODEL_TYPE::BSIM2].modelParams.emplace_back( "lu1b",  174, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "m", SIM_MODEL::PARAM::CATEGORY::DC, "0", "0", "Length depence of u1b" );
    modelInfos[MODEL_TYPE::BSIM2].modelParams.emplace_back( "wu1b",  175, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "m", SIM_MODEL::PARAM::CATEGORY::DC, "0", "0", "Width depence of u1b" );
    modelInfos[MODEL_TYPE::BSIM2].modelParams.emplace_back( "u1d",  176, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "um/V²", SIM_MODEL::PARAM::CATEGORY::DC, "0", "0", "VDS depence of u1" );
    modelInfos[MODEL_TYPE::BSIM2].modelParams.emplace_back( "lu1d",  177, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "m", SIM_MODEL::PARAM::CATEGORY::DC, "0", "0", "Length depence of u1d" );
    modelInfos[MODEL_TYPE::BSIM2].modelParams.emplace_back( "wu1d",  178, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "m", SIM_MODEL::PARAM::CATEGORY::DC, "0", "0", "Width depence of u1d" );
    modelInfos[MODEL_TYPE::BSIM2].modelParams.emplace_back( "n0",  179, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::DC, "1.4", "1.4", "Subthreshold slope at VDS=0 VBS=0" );
    modelInfos[MODEL_TYPE::BSIM2].modelParams.emplace_back( "ln0",  180, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "m", SIM_MODEL::PARAM::CATEGORY::DC, "0", "0", "Length dependence of n0" );
    modelInfos[MODEL_TYPE::BSIM2].modelParams.emplace_back( "wn0",  181, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "m", SIM_MODEL::PARAM::CATEGORY::DC, "0", "0", "Width dependence of n0" );
    modelInfos[MODEL_TYPE::BSIM2].modelParams.emplace_back( "nb",  182, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::DC, "0.5", "0.5", "VBS dependence of n" );
    modelInfos[MODEL_TYPE::BSIM2].modelParams.emplace_back( "lnb",  183, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "m", SIM_MODEL::PARAM::CATEGORY::DC, "0", "0", "Length dependence of nb" );
    modelInfos[MODEL_TYPE::BSIM2].modelParams.emplace_back( "wnb",  184, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "m", SIM_MODEL::PARAM::CATEGORY::DC, "0", "0", "Width dependence of nb" );
    modelInfos[MODEL_TYPE::BSIM2].modelParams.emplace_back( "nd",  185, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::DC, "0", "0", "VDS dependence of n" );
    modelInfos[MODEL_TYPE::BSIM2].modelParams.emplace_back( "lnd",  186, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "m", SIM_MODEL::PARAM::CATEGORY::DC, "0", "0", "Length dependence of nd" );
    modelInfos[MODEL_TYPE::BSIM2].modelParams.emplace_back( "wnd",  187, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "m", SIM_MODEL::PARAM::CATEGORY::DC, "0", "0", "Width dependence of nd" );
    modelInfos[MODEL_TYPE::BSIM2].modelParams.emplace_back( "vof0",  188, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "V", SIM_MODEL::PARAM::CATEGORY::DC, "1.8", "1.8", "Threshold voltage offset AT VDS=0 VBS=0" );
    modelInfos[MODEL_TYPE::BSIM2].modelParams.emplace_back( "lvof0",  189, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "m", SIM_MODEL::PARAM::CATEGORY::DC, "0", "0", "Length dependence of vof0" );
    modelInfos[MODEL_TYPE::BSIM2].modelParams.emplace_back( "wvof0",  190, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "m", SIM_MODEL::PARAM::CATEGORY::DC, "0", "0", "Width dependence of vof0" );
    modelInfos[MODEL_TYPE::BSIM2].modelParams.emplace_back( "vofb",  191, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::DC, "0", "0", "VBS dependence of vof" );
    modelInfos[MODEL_TYPE::BSIM2].modelParams.emplace_back( "lvofb",  192, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "m", SIM_MODEL::PARAM::CATEGORY::DC, "0", "0", "Length dependence of vofb" );
    modelInfos[MODEL_TYPE::BSIM2].modelParams.emplace_back( "wvofb",  193, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "m", SIM_MODEL::PARAM::CATEGORY::DC, "0", "0", "Width dependence of vofb" );
    modelInfos[MODEL_TYPE::BSIM2].modelParams.emplace_back( "vofd",  194, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::DC, "0", "0", "VDS dependence of vof" );
    modelInfos[MODEL_TYPE::BSIM2].modelParams.emplace_back( "lvofd",  195, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "m", SIM_MODEL::PARAM::CATEGORY::DC, "0", "0", "Length dependence of vofd" );
    modelInfos[MODEL_TYPE::BSIM2].modelParams.emplace_back( "wvofd",  196, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "m", SIM_MODEL::PARAM::CATEGORY::DC, "0", "0", "Width dependence of vofd" );
    modelInfos[MODEL_TYPE::BSIM2].modelParams.emplace_back( "ai0",  197, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::DC, "0", "0", "Pre-factor of hot-electron effect." );
    modelInfos[MODEL_TYPE::BSIM2].modelParams.emplace_back( "lai0",  198, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "m", SIM_MODEL::PARAM::CATEGORY::DC, "0", "0", "Length dependence of ai0" );
    modelInfos[MODEL_TYPE::BSIM2].modelParams.emplace_back( "wai0",  199, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "m", SIM_MODEL::PARAM::CATEGORY::DC, "0", "0", "Width dependence of ai0" );
    modelInfos[MODEL_TYPE::BSIM2].modelParams.emplace_back( "aib",  200, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::DC, "0", "0", "VBS dependence of ai" );
    modelInfos[MODEL_TYPE::BSIM2].modelParams.emplace_back( "laib",  201, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "m", SIM_MODEL::PARAM::CATEGORY::DC, "0", "0", "Length dependence of aib" );
    modelInfos[MODEL_TYPE::BSIM2].modelParams.emplace_back( "waib",  202, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "m", SIM_MODEL::PARAM::CATEGORY::DC, "0", "0", "Width dependence of aib" );
    modelInfos[MODEL_TYPE::BSIM2].modelParams.emplace_back( "bi0",  203, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::DC, "0", "0", "Exponential factor of hot-electron effect." );
    modelInfos[MODEL_TYPE::BSIM2].modelParams.emplace_back( "lbi0",  204, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "m", SIM_MODEL::PARAM::CATEGORY::DC, "0", "0", "Length dependence of bi0" );
    modelInfos[MODEL_TYPE::BSIM2].modelParams.emplace_back( "wbi0",  205, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "m", SIM_MODEL::PARAM::CATEGORY::DC, "0", "0", "Width dependence of bi0" );
    modelInfos[MODEL_TYPE::BSIM2].modelParams.emplace_back( "bib",  206, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::DC, "0", "0", "VBS dependence of bi" );
    modelInfos[MODEL_TYPE::BSIM2].modelParams.emplace_back( "lbib",  207, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "m", SIM_MODEL::PARAM::CATEGORY::DC, "0", "0", "Length dependence of bib" );
    modelInfos[MODEL_TYPE::BSIM2].modelParams.emplace_back( "wbib",  208, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "m", SIM_MODEL::PARAM::CATEGORY::DC, "0", "0", "Width dependence of bib" );
    modelInfos[MODEL_TYPE::BSIM2].modelParams.emplace_back( "vghigh",  209, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::DC, "0.2", "0.2", "Upper bound of the cubic spline function." );
    modelInfos[MODEL_TYPE::BSIM2].modelParams.emplace_back( "lvghigh",  210, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "m", SIM_MODEL::PARAM::CATEGORY::DC, "0", "0", "Length dependence of vghigh" );
    modelInfos[MODEL_TYPE::BSIM2].modelParams.emplace_back( "wvghigh",  211, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "m", SIM_MODEL::PARAM::CATEGORY::DC, "0", "0", "Width dependence of vghigh" );
    modelInfos[MODEL_TYPE::BSIM2].modelParams.emplace_back( "vglow",  212, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::DC, "-0.15", "-0.15", "Lower bound of the cubic spline function." );
    modelInfos[MODEL_TYPE::BSIM2].modelParams.emplace_back( "lvglow",  213, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "m", SIM_MODEL::PARAM::CATEGORY::DC, "0", "0", "Length dependence of vglow" );
    modelInfos[MODEL_TYPE::BSIM2].modelParams.emplace_back( "wvglow",  214, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "m", SIM_MODEL::PARAM::CATEGORY::DC, "0", "0", "Width dependence of vglow" );
    modelInfos[MODEL_TYPE::BSIM2].modelParams.emplace_back( "tox",  215, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "um", SIM_MODEL::PARAM::CATEGORY::DC, "0.03", "0.03", "Gate oxide thickness in um" );
    modelInfos[MODEL_TYPE::BSIM2].modelParams.emplace_back( "temp",  216, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "°C", SIM_MODEL::PARAM::CATEGORY::PRINCIPAL, "27", "27", "Temperature in degree Celcius" );
    modelInfos[MODEL_TYPE::BSIM2].modelParams.emplace_back( "vdd",  217, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::DC, "5", "5", "Maximum Vds" );
    modelInfos[MODEL_TYPE::BSIM2].modelParams.emplace_back( "vgg",  218, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::DC, "5", "5", "Maximum Vgs" );
    modelInfos[MODEL_TYPE::BSIM2].modelParams.emplace_back( "vbb",  219, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::DC, "5", "5", "Maximum Vbs" );
    modelInfos[MODEL_TYPE::BSIM2].modelParams.emplace_back( "cgso",  220, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "F/m", SIM_MODEL::PARAM::CATEGORY::CAPACITANCE, "0", "0", "Gate source overlap capacitance per unit channel width(m)" );
    modelInfos[MODEL_TYPE::BSIM2].modelParams.emplace_back( "cgdo",  221, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "F/m", SIM_MODEL::PARAM::CATEGORY::CAPACITANCE, "0", "0", "Gate drain overlap capacitance per unit channel width(m)" );
    modelInfos[MODEL_TYPE::BSIM2].modelParams.emplace_back( "cgbo",  222, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "F/m", SIM_MODEL::PARAM::CATEGORY::CAPACITANCE, "0", "0", "Gate bulk overlap capacitance per unit channel length(m)" );
    modelInfos[MODEL_TYPE::BSIM2].modelParams.emplace_back( "xpart",  223, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::DC, "0", "0", "Flag for channel charge partitioning" );
    modelInfos[MODEL_TYPE::BSIM2].modelParams.emplace_back( "rsh",  224, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "Ω/m", SIM_MODEL::PARAM::CATEGORY::DC, "0", "0", "Source drain diffusion sheet resistance in ohm per square" );
    modelInfos[MODEL_TYPE::BSIM2].modelParams.emplace_back( "js",  225, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "A", SIM_MODEL::PARAM::CATEGORY::DC, "0", "0", "Source drain junction saturation current per unit area" );
    modelInfos[MODEL_TYPE::BSIM2].modelParams.emplace_back( "pb",  226, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "V", SIM_MODEL::PARAM::CATEGORY::DC, "0.1", "0.1", "Source drain junction built in potential" );
    modelInfos[MODEL_TYPE::BSIM2].modelParams.emplace_back( "mj",  227, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "F", SIM_MODEL::PARAM::CATEGORY::CAPACITANCE, "0", "0", "Source drain bottom junction capacitance grading coefficient" );
    modelInfos[MODEL_TYPE::BSIM2].modelParams.emplace_back( "pbsw",  228, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "F", SIM_MODEL::PARAM::CATEGORY::CAPACITANCE, "0.1", "0.1", "Source drain side junction capacitance built in potential" );
    modelInfos[MODEL_TYPE::BSIM2].modelParams.emplace_back( "mjsw",  229, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "F", SIM_MODEL::PARAM::CATEGORY::CAPACITANCE, "0", "0", "Source drain side junction capacitance grading coefficient" );
    modelInfos[MODEL_TYPE::BSIM2].modelParams.emplace_back( "cj",  230, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "F", SIM_MODEL::PARAM::CATEGORY::CAPACITANCE, "0", "0", "Source drain bottom junction capacitance per unit area" );
    modelInfos[MODEL_TYPE::BSIM2].modelParams.emplace_back( "cjsw",  231, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "F", SIM_MODEL::PARAM::CATEGORY::CAPACITANCE, "0", "0", "Source drain side junction capacitance per unit area" );
    modelInfos[MODEL_TYPE::BSIM2].modelParams.emplace_back( "wdf",  232, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "um", SIM_MODEL::PARAM::CATEGORY::DC, "10", "10", "Default width of source drain diffusion in um" );
    modelInfos[MODEL_TYPE::BSIM2].modelParams.emplace_back( "dell",  233, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "m", SIM_MODEL::PARAM::CATEGORY::DC, "0", "0", "Length reduction of source drain diffusion" );
    modelInfos[MODEL_TYPE::BSIM2].modelParams.emplace_back( "kf",  236, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::NOISE, "0", "0", "Flicker noise coefficient" );
    modelInfos[MODEL_TYPE::BSIM2].modelParams.emplace_back( "af",  237, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::NOISE, "0", "0", "Flicker noise exponent" );
    modelInfos[MODEL_TYPE::BSIM2].modelParams.emplace_back( "nmos",  234, SIM_MODEL::PARAM::DIR_IN, SIM_VALUE::TYPE_BOOL, "", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "Flag to indicate NMOS" );
    modelInfos[MODEL_TYPE::BSIM2].modelParams.emplace_back( "pmos",  235, SIM_MODEL::PARAM::DIR_IN, SIM_VALUE::TYPE_BOOL, "", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "Flag to indicate PMOS" );
    // Instance parameters
    modelInfos[MODEL_TYPE::BSIM2].instanceParams.emplace_back( "l",  2, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "m", SIM_MODEL::PARAM::CATEGORY::GEOMETRY, "", "", "Length", true );
    modelInfos[MODEL_TYPE::BSIM2].instanceParams.emplace_back( "w",  1, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "m", SIM_MODEL::PARAM::CATEGORY::GEOMETRY, "", "", "Width", true );
    modelInfos[MODEL_TYPE::BSIM2].instanceParams.emplace_back( "m",  14, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::GEOMETRY, "", "", "Parallel Multiplier", true );
    modelInfos[MODEL_TYPE::BSIM2].instanceParams.emplace_back( "ad",  4, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::GEOMETRY, "", "", "Drain area", true );
    modelInfos[MODEL_TYPE::BSIM2].instanceParams.emplace_back( "as",  3, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::GEOMETRY, "", "", "Source area", true );
    modelInfos[MODEL_TYPE::BSIM2].instanceParams.emplace_back( "pd",  6, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::GEOMETRY, "", "", "Drain perimeter", true );
    modelInfos[MODEL_TYPE::BSIM2].instanceParams.emplace_back( "ps",  5, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::GEOMETRY, "", "", "Source perimeter", true );
    modelInfos[MODEL_TYPE::BSIM2].instanceParams.emplace_back( "nrd",  8, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::GEOMETRY, "", "", "Number of squares in drain", true );
    modelInfos[MODEL_TYPE::BSIM2].instanceParams.emplace_back( "nrs",  7, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "", SIM_MODEL::PARAM::CATEGORY::GEOMETRY, "", "", "Number of squares in source", true );
    modelInfos[MODEL_TYPE::BSIM2].instanceParams.emplace_back( "off",  9, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_BOOL, "", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "Device is initially off", true );
    modelInfos[MODEL_TYPE::BSIM2].instanceParams.emplace_back( "vds",  11, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "V", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "Initial D-S voltage", true );
    modelInfos[MODEL_TYPE::BSIM2].instanceParams.emplace_back( "vgs",  12, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "V", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "Initial G-S voltage", true );
    modelInfos[MODEL_TYPE::BSIM2].instanceParams.emplace_back( "vbs",  10, SIM_MODEL::PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, "V", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "Initial B-S voltage", true );
    modelInfos[MODEL_TYPE::BSIM2].instanceParams.emplace_back( "ic",  13, SIM_MODEL::PARAM::DIR_IN, SIM_VALUE::TYPE_FLOAT_VECTOR /*SIM_VALUE::TYPE::VECTOR*/, "", SIM_MODEL::PARAM::CATEGORY::SUPERFLUOUS, "", "", "Vector of DS,GS,BS initial voltages", true );
}