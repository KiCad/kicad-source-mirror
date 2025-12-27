/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2022 Fabien Corona f.corona<at>laposte.net
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its contributors may be used
 * to endorse or promote products derived from this software without specific
 * prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */


#include "kibis.h"
#include "ibis_parser.h"
#include <sstream>
#include <sim/spice_simulator.h>


// _() is used here to mark translatable strings in IBIS_REPORTER::Report()
// However, currently non ASCII7 chars are nor correctly handled when printing messages
// So we disable translations
#if 0
#include <wx/intl.h>    // for _() macro and wxGetTranslation()
#else
#undef _
#define _( x ) x
#endif


std::vector<std::pair<int, double>>
SimplifyBitSequence( const std::vector<std::pair<int, double>>& bits )
{
    std::vector<std::pair<int, double>> result;
    int                                 prevbit = -1;

    for( const std::pair<int, double>& bit : bits )
    {
        if( prevbit != bit.first )
            result.push_back( bit );

        prevbit = bit.first;
    }

    return result;
}


KIBIS_BASE::KIBIS_BASE( KIBIS* aTopLevel ) :
        KIBIS_BASE( aTopLevel, aTopLevel->m_Reporter )
{
}


KIBIS_BASE::KIBIS_BASE( KIBIS* aTopLevel, REPORTER* aReporter ) :
        IBIS_BASE( aReporter ),
        m_topLevel( aTopLevel ),
        m_valid( false )
{
}


IBIS_CORNER ReverseLogic( IBIS_CORNER aIn )
{
    IBIS_CORNER out = IBIS_CORNER::TYP;

    if( aIn == IBIS_CORNER::MIN )
        out = IBIS_CORNER::MAX;
    else if( aIn == IBIS_CORNER::MAX )
        out = IBIS_CORNER::MIN;

    return out;
}

KIBIS::KIBIS( const std::string& aFileName, REPORTER* aReporter ) :
        KIBIS_BASE( this, aReporter ),
        m_file( *this )
{
    IbisParser parser( m_Reporter );
    bool       status = true;

    parser.m_parrot = false;
    status &= parser.ParseFile( aFileName );

    status &= m_file.Init( parser );

    for( const IbisModel& iModel : parser.m_ibisFile.m_models )
    {
        KIBIS_MODEL& kModel = m_models.emplace_back( *this, iModel, parser );
        status &= kModel.m_valid;
    }

    for( const IbisComponent& iComponent : parser.m_ibisFile.m_components )
    {
        KIBIS_COMPONENT& kComponent = m_components.emplace_back( *this, iComponent, parser );
        status &= kComponent.m_valid;

        for( KIBIS_PIN& pin : kComponent.m_pins )
            pin.m_parent = &kComponent;

        for( const IbisDiffPinEntry& dpEntry : iComponent.m_diffPin.m_entries )
        {
            KIBIS_PIN* pinA = kComponent.GetPin( dpEntry.pinA );
            KIBIS_PIN* pinB = kComponent.GetPin( dpEntry.pinB );

            if( pinA && pinB )
            {
                pinA->m_complementaryPin = pinB;
                pinB->m_complementaryPin = pinA;
            }
        }
    }

    m_valid = status;
}


KIBIS_FILE::KIBIS_FILE( KIBIS& aTopLevel ) :
        KIBIS_BASE( &aTopLevel )
{
    m_fileRev = -1;
    m_ibisVersion = -1;
}


bool KIBIS_FILE::Init( const IbisParser& aParser )
{
    bool status = true;
    m_fileName = aParser.m_ibisFile.m_header.m_fileName;
    m_fileRev = aParser.m_ibisFile.m_header.m_fileRevision;
    m_ibisVersion = aParser.m_ibisFile.m_header.m_ibisVersion;
    m_date = aParser.m_ibisFile.m_header.m_date;
    m_notes = aParser.m_ibisFile.m_header.m_notes;
    m_disclaimer = aParser.m_ibisFile.m_header.m_disclaimer;
    m_copyright = aParser.m_ibisFile.m_header.m_copyright;

    m_valid = status;

    return status;
}


KIBIS_PIN::KIBIS_PIN( KIBIS& aTopLevel, const IbisComponentPin& aPin,
                      const IbisComponentPackage& aPackage, IbisParser& aParser,
                      KIBIS_COMPONENT* aParent, std::vector<KIBIS_MODEL>& aModels ) :
        KIBIS_BASE( &aTopLevel ),
        m_Rpin( aTopLevel.m_Reporter ),
        m_Lpin( aTopLevel.m_Reporter ),
        m_Cpin( aTopLevel.m_Reporter )
{
    m_signalName = aPin.m_signalName;
    m_pinNumber = aPin.m_pinName;
    m_parent = aParent;

    m_Rpin = aPackage.m_Rpkg;
    m_Lpin = aPackage.m_Lpkg;
    m_Cpin = aPackage.m_Cpkg;

    // The values listed in the [Pin] description section override the default
    // values defined in [Package]

    // @TODO : Reading the IBIS standard, I can't figure out if we are supposed
    // to replace typ, min, and max, or just the typ ?

    if( !std::isnan( aPin.m_Rpin ) )
    {
        m_Rpin.value[IBIS_CORNER::TYP] = aPin.m_Rpin;
        m_Rpin.value[IBIS_CORNER::MIN] = aPin.m_Rpin;
        m_Rpin.value[IBIS_CORNER::MAX] = aPin.m_Rpin;
    }

    if( !std::isnan( aPin.m_Lpin ) )
    {
        m_Lpin.value[IBIS_CORNER::TYP] = aPin.m_Lpin;
        m_Lpin.value[IBIS_CORNER::MIN] = aPin.m_Lpin;
        m_Lpin.value[IBIS_CORNER::MAX] = aPin.m_Lpin;
    }

    if( !std::isnan( aPin.m_Cpin ) )
    {
        m_Cpin.value[IBIS_CORNER::TYP] = aPin.m_Cpin;
        m_Cpin.value[IBIS_CORNER::MIN] = aPin.m_Cpin;
        m_Cpin.value[IBIS_CORNER::MAX] = aPin.m_Cpin;
    }

    bool                     modelSelected = false;
    std::vector<std::string> listOfModels;

    for( const IbisModelSelector& modelSelector : aParser.m_ibisFile.m_modelSelectors )
    {
        if( !strcmp( modelSelector.m_name.c_str(), aPin.m_modelName.c_str() ) )
        {
            for( const IbisModelSelectorEntry& model : modelSelector.m_models )
                listOfModels.push_back( model.m_modelName );

            modelSelected = true;
            break;
        }
    }

    if( !modelSelected )
        listOfModels.push_back( aPin.m_modelName );

    for( const std::string& modelName : listOfModels )
    {
        for( KIBIS_MODEL& model : aModels )
        {
            if( !strcmp( model.m_name.c_str(), modelName.c_str() ) )
                m_models.push_back( &model );
        }
    }

    m_valid = true;
}


KIBIS_SUBMODEL::KIBIS_SUBMODEL( KIBIS& aTopLevel, const IbisSubmodel& aSource, IBIS_SUBMODEL_MODE aMode ) :
        KIBIS_BASE( &aTopLevel ),
        m_GNDClamp( aTopLevel.m_Reporter ),
        m_POWERClamp( aTopLevel.m_Reporter )
{
    m_name = aSource.m_name;
    m_type = aSource.m_type;
    m_mode = aMode;

    m_GNDClamp = aSource.m_GNDClamp;
    m_POWERClamp = aSource.m_POWERClamp;

    m_valid = true;
}

bool KIBIS_SUBMODEL::HasGNDClamp() const
{
    return m_GNDClamp.m_entries.size() > 0;
}


bool KIBIS_SUBMODEL::HasPOWERClamp() const
{
    return m_POWERClamp.m_entries.size() > 0;
}


KIBIS_MODEL::KIBIS_MODEL( KIBIS& aTopLevel, const IbisModel& aSource, IbisParser& aParser ) :
        KIBIS_BASE( &aTopLevel ),
        m_C_comp( aTopLevel.m_Reporter ),
        m_voltageRange( aTopLevel.m_Reporter ),
        m_temperatureRange( aTopLevel.m_Reporter ),
        m_pullupReference( aTopLevel.m_Reporter ),
        m_pulldownReference( aTopLevel.m_Reporter ),
        m_GNDClampReference( aTopLevel.m_Reporter ),
        m_POWERClampReference( aTopLevel.m_Reporter ),
        m_Rgnd( aTopLevel.m_Reporter ),
        m_Rpower( aTopLevel.m_Reporter ),
        m_Rac( aTopLevel.m_Reporter ),
        m_Cac( aTopLevel.m_Reporter ),
        m_GNDClamp( aTopLevel.m_Reporter ),
        m_POWERClamp( aTopLevel.m_Reporter ),
        m_pullup( aTopLevel.m_Reporter ),
        m_pulldown( aTopLevel.m_Reporter ),
        m_ramp( aTopLevel.m_Reporter )
{
    bool status = true;

    m_name = aSource.m_name;
    m_type = aSource.m_type;

    m_description = std::string( "No description available." );

    for( const IbisModelSelector& modelSelector : aParser.m_ibisFile.m_modelSelectors )
    {
        for( const IbisModelSelectorEntry& entry : modelSelector.m_models )
        {
            if( !strcmp( entry.m_modelName.c_str(), m_name.c_str() ) )
                m_description = entry.m_modelDescription;
        }
    }

    m_vinh = aSource.m_vinh;
    m_vinl = aSource.m_vinl;
    m_vref = aSource.m_vref;
    m_rref = aSource.m_rref;
    m_cref = aSource.m_cref;
    m_vmeas = aSource.m_vmeas;

    m_enable = aSource.m_enable;
    m_polarity = aSource.m_polarity;

    m_ramp = aSource.m_ramp;
    m_risingWaveforms = aSource.m_risingWaveforms;
    m_fallingWaveforms = aSource.m_fallingWaveforms;
    m_GNDClamp = aSource.m_GNDClamp;
    m_GNDClampReference = aSource.m_GNDClampReference;
    m_POWERClamp = aSource.m_POWERClamp;
    m_POWERClampReference = aSource.m_POWERClampReference;

    if( aSource.m_C_comp.isNA() )
    {
        m_C_comp.value[IBIS_CORNER::TYP] = 0.0;
        m_C_comp.value[IBIS_CORNER::MIN] = 0.0;
        m_C_comp.value[IBIS_CORNER::MAX] = 0.0;
        m_C_comp.Add( aSource.m_C_comp_pullup );
        m_C_comp.Add( aSource.m_C_comp_pulldown );
        m_C_comp.Add( aSource.m_C_comp_gnd_clamp );
        m_C_comp.Add( aSource.m_C_comp_power_clamp );
    }
    else
    {
        m_C_comp = aSource.m_C_comp;
    }
    m_voltageRange = aSource.m_voltageRange;
    m_temperatureRange = aSource.m_temperatureRange;
    m_pullupReference = aSource.m_pullupReference;
    m_pulldownReference = aSource.m_pulldownReference;

    m_Rgnd = aSource.m_Rgnd;
    m_Rpower = aSource.m_Rpower;
    m_Rac = aSource.m_Rac;
    m_Cac = aSource.m_Cac;
    m_pullup = aSource.m_pullup;
    m_pulldown = aSource.m_pulldown;

    for( const IbisSubmodelMode& submodel : aSource.m_submodels )
    {
        auto it = aParser.m_ibisFile.m_submodels.find( submodel.m_name );

        if( it != aParser.m_ibisFile.m_submodels.end() )
        {
            const IbisSubmodel& submodelSource = it->second;

            // For now, we only support static mode dynamic clamp submodels.
            if( submodelSource.m_type != IBIS_SUBMODEL_TYPE::DYNAMIC_CLAMP
                || !submodelSource.m_VtriggerR.isNA()
                || !submodelSource.m_VtriggerF.isNA() )
            {
                break;
            }

            m_submodels.emplace_back( aTopLevel, submodelSource, submodel.m_mode );
        }
    }

    m_valid = status;
}


KIBIS_COMPONENT::KIBIS_COMPONENT( KIBIS& aTopLevel, const IbisComponent& aSource,
                                  IbisParser& aParser ) :
        KIBIS_BASE( &aTopLevel )
{
    bool status = true;

    m_name = aSource.m_name;
    m_manufacturer = aSource.m_manufacturer;

    for( const IbisComponentPin& iPin : aSource.m_pins )
    {
        if( iPin.m_dummy )
            continue;

        KIBIS_PIN kPin( aTopLevel, iPin, aSource.m_package, aParser, nullptr,
                        m_topLevel->m_models );
        status &= kPin.m_valid;
        m_pins.push_back( kPin );
    }

    m_valid = status;
}


KIBIS_PIN* KIBIS_COMPONENT::GetPin( const std::string& aPinNumber )
{
    for( KIBIS_PIN& pin : m_pins )
    {
        if( pin.m_pinNumber == aPinNumber )
            return &pin;
    }

    return nullptr;
}

std::vector<std::pair<IbisWaveform*, IbisWaveform*>> KIBIS_MODEL::waveformPairs()
{
    std::vector<std::pair<IbisWaveform*, IbisWaveform*>> pairs;
    IbisWaveform*                                        wf1;
    IbisWaveform*                                        wf2;

    for( size_t i = 0; i < m_risingWaveforms.size(); i++ )
    {
        for( size_t j = 0; j < m_fallingWaveforms.size(); j++ )
        {
            wf1 = m_risingWaveforms.at( i );
            wf2 = m_fallingWaveforms.at( j );

            if( wf1->m_R_fixture == wf2->m_R_fixture && wf1->m_L_fixture == wf2->m_L_fixture
                && wf1->m_C_fixture == wf2->m_C_fixture && wf1->m_V_fixture == wf2->m_V_fixture
                && wf1->m_V_fixture_min == wf2->m_V_fixture_min
                && wf1->m_V_fixture_max == wf2->m_V_fixture_max )
            {
                pairs.emplace_back( wf1, wf2 );
            }
        }
    }

    return pairs;
}


std::string KIBIS_MODEL::SpiceDie( const KIBIS_PARAMETER& aParam, int aIndex, bool aDriver ) const
{
    std::string result;

    std::string GC_GND = "GC_GND";
    std::string PC_PWR = "PC_PWR";
    std::string PU_PWR = "PU_PWR";
    std::string PD_GND = "PD_GND";
    std::string DIE = "DIE";
    std::string DIEBUFF = "DIEBUFF";

    IBIS_CORNER supply = aParam.m_supply;
    IBIS_CORNER ccomp = aParam.m_Ccomp;

    GC_GND += std::to_string( aIndex );
    PC_PWR += std::to_string( aIndex );
    PU_PWR += std::to_string( aIndex );
    PD_GND += std::to_string( aIndex );
    DIE += std::to_string( aIndex );
    DIEBUFF += std::to_string( aIndex );


    std::string GC = "GC";
    std::string PC = "PC";
    std::string PU = "PU";
    std::string PD = "PD";

    GC += std::to_string( aIndex );
    PC += std::to_string( aIndex );
    PU += std::to_string( aIndex );
    PD += std::to_string( aIndex );

    result = "\n";
    result += "VPWR POWER GND ";
    result += doubleToString( m_voltageRange.value[supply] );
    result += "\n";
    result += "CCPOMP " + DIE + " GND ";
    result += doubleToString( m_C_comp.value[ccomp] );
    result += "\n";

    if( HasGNDClamp() )
    {
        result += m_GNDClamp.Spice( aIndex * 4 + 1, DIE, GC_GND, false, GC, supply );
        result += "Vmeas" + GC + " GND " + GC_GND + " 0\n";
    }

    if( HasPOWERClamp() )
    {
        result += m_POWERClamp.Spice( aIndex * 4 + 2, PC_PWR, DIE, true, PC, supply );
        result += "Vmeas" + PC + " POWER " + PC_PWR + " 0\n";
    }

    if( aDriver && HasPulldown() )
    {
        result += m_pulldown.Spice( aIndex * 4 + 3, DIEBUFF, PD_GND, false, PD, supply );
        result += "VmeasPD GND " + PD_GND + " 0\n";
        result += "BKD GND " + DIE + " i=( i(VmeasPD) * v(KD) )\n";
    }

    if( aDriver && HasPullup() )
    {
        result += m_pullup.Spice( aIndex * 4 + 4, PU_PWR, DIEBUFF, false, PU, supply );
        result += "VmeasPU POWER " + PU_PWR + " 0\n";
        result += "BKU POWER " + DIE + " i=( -i(VmeasPU) * v(KU) )\n";
    }

    if( aDriver && ( HasPullup() || HasPulldown() ) )
        result += "BDIEBUFF " + DIEBUFF + " GND v=v(" + DIE + ")\n";

    for( const KIBIS_SUBMODEL& submodel : m_submodels )
    {
        if( aDriver && ( submodel.m_mode == IBIS_SUBMODEL_MODE::NON_DRIVING ) )
            continue;

        if( !aDriver && ( submodel.m_mode == IBIS_SUBMODEL_MODE::DRIVING ) )
            continue;

        aIndex++;

        GC_GND = "GC_GND" + std::to_string( aIndex );
        PC_PWR = "PC_PWR" + std::to_string( aIndex );

        GC = "GC" + std::to_string( aIndex );
        PC = "PC" + std::to_string( aIndex );

        if( submodel.HasGNDClamp() )
        {
            result += submodel.m_GNDClamp.Spice( aIndex * 4 + 1, DIE, GC_GND, false, GC, supply );
            result += "Vmeas" + GC + " GND " + GC_GND + " 0\n";
        }

        if( submodel.HasPOWERClamp() )
        {
            result += submodel.m_POWERClamp.Spice( aIndex * 4 + 2, PC_PWR, DIE, true, PC, supply );
            result += "Vmeas" + PC + " POWER " + PC_PWR + " 0\n";
        }
    }

    return result;
}


IbisWaveform KIBIS_MODEL::TrimWaveform( const IbisWaveform& aIn ) const
{
    IbisWaveform out( aIn.m_Reporter );

    const int nbPoints = aIn.m_table.m_entries.size();

    if( nbPoints < 2 )
    {
        Report( _( "waveform has less than two points" ), RPT_SEVERITY_ERROR );
        return out;
    }

    const double DCtyp = aIn.m_table.m_entries[0].V.value[IBIS_CORNER::TYP];
    const double DCmin = aIn.m_table.m_entries[0].V.value[IBIS_CORNER::MIN];
    const double DCmax = aIn.m_table.m_entries[0].V.value[IBIS_CORNER::MAX];

    if( nbPoints == 2 )
        return out;

    out.m_table.m_entries.clear();

    for( int i = 0; i < nbPoints; i++ )
    {
        const VTtableEntry& inEntry = aIn.m_table.m_entries.at( i );
        VTtableEntry&       outEntry = out.m_table.m_entries.emplace_back( out.m_Reporter );

        outEntry.t = inEntry.t;
        outEntry.V.value[IBIS_CORNER::TYP] = inEntry.V.value[IBIS_CORNER::TYP] - DCtyp;
        outEntry.V.value[IBIS_CORNER::MIN] = inEntry.V.value[IBIS_CORNER::MIN] - DCmin;
        outEntry.V.value[IBIS_CORNER::MAX] = inEntry.V.value[IBIS_CORNER::MAX] - DCmax;
    }

    return out;
}


bool KIBIS_MODEL::HasPulldown() const
{
    return m_pulldown.m_entries.size() > 0;
}


bool KIBIS_MODEL::HasPullup() const
{
    return m_pullup.m_entries.size() > 0;
}


bool KIBIS_MODEL::HasGNDClamp() const
{
    return m_GNDClamp.m_entries.size() > 0;
}


bool KIBIS_MODEL::HasPOWERClamp() const
{
    return m_POWERClamp.m_entries.size() > 0;
}


std::string KIBIS_MODEL::generateSquareWave( const std::string& aNode1, const std::string& aNode2,
                                             const std::vector<std::pair<int, double>>&     aBits,
                                             const std::pair<IbisWaveform*, IbisWaveform*>& aPair,
                                             const KIBIS_PARAMETER&                         aParam )
{
    const IBIS_CORNER supply = aParam.m_supply;
    std::string       simul;
    std::vector<int>  stimuliIndex;

    IbisWaveform risingWF = TrimWaveform( *( aPair.first ) );
    IbisWaveform fallingWF = TrimWaveform( *( aPair.second ) );

    double deltaR = risingWF.m_table.m_entries.back().V.value[supply]
                    - risingWF.m_table.m_entries.at( 0 ).V.value[supply];
    double deltaF = fallingWF.m_table.m_entries.back().V.value[supply]
                    - fallingWF.m_table.m_entries.at( 0 ).V.value[supply];

    // Ideally, delta should be equal to zero.
    // It can be different from zero if the falling waveform does not start were the rising one ended.
    double delta = deltaR + deltaF;

    int i = 0;

    int prevBit = 2;

    for( const std::pair<int, double>& bit : aBits )
    {
        IbisWaveform* WF;
        double        timing = bit.second;


        if ( bit.first != prevBit )
        {
            if( bit.first == 1 )
                WF = &risingWF;
            else
                WF = &fallingWF;

            stimuliIndex.push_back( i );

            simul += "Vstimuli";
            simul += std::to_string( i );
            simul += " stimuli";
            simul += std::to_string( i );
            simul += " ";
            simul += aNode2;
            simul += " pwl ( \n+";

            if( i != 0 )
            {
                simul += "0 0 ";
                VTtableEntry entry0 = WF->m_table.m_entries.at( 0 );
                VTtableEntry entry1 = WF->m_table.m_entries.at( 1 );
                double       deltaT = entry1.t - entry0.t;

                simul += doubleToString( entry0.t + timing - deltaT );
                simul += " ";
                simul += "0";
                simul += "\n+";
            }

            for( const VTtableEntry& entry : WF->m_table.m_entries )
            {
                simul += doubleToString( entry.t + timing );
                simul += " ";
                simul += doubleToString( entry.V.value[supply] - delta );
                simul += "\n+";
            }

            simul += ")\n";
        }

        i++;
        prevBit = bit.first;
    }

    simul += "bin ";
    simul += aNode1;
    simul += " ";
    simul += aNode2;
    simul += " v=(";

    for( int ii: stimuliIndex )
    {
        simul += " v( stimuli";
        simul += std::to_string( ii );
        simul += " ) +\n+";
    }

    // Depending on the first bit, we add a different DC value
    // The DC value we add is the first value of the first bit.
    if( ( aBits.size() > 0 ) && ( aBits[0].first == 0 ) )
        simul += doubleToString( aPair.second->m_table.m_entries.at( 0 ).V.value[supply] );
    else
        simul += doubleToString( aPair.first->m_table.m_entries.at( 0 ).V.value[supply] );

    simul += ")\n";
    return simul;
}


std::string KIBIS_PIN::addDie( KIBIS_MODEL& aModel, const KIBIS_PARAMETER& aParam, int aIndex )
{
    const IBIS_CORNER supply = aParam.m_supply;
    std::string simul;

    std::string GC_GND = "GC_GND";
    std::string PC_PWR = "PC_PWR";
    std::string PU_PWR = "PU_PWR";
    std::string PD_GND = "PD_GND";
    std::string DIE = "DIE";

    GC_GND += std::to_string( aIndex );
    PC_PWR += std::to_string( aIndex );
    PU_PWR += std::to_string( aIndex );
    PD_GND += std::to_string( aIndex );
    DIE += std::to_string( aIndex );


    std::string GC = "GC";
    std::string PC = "PC";
    std::string PU = "PU";
    std::string PD = "PD";

    GC += std::to_string( aIndex );
    PC += std::to_string( aIndex );
    PU += std::to_string( aIndex );
    PD += std::to_string( aIndex );

    if( aModel.HasGNDClamp() )
        simul += aModel.m_GNDClamp.Spice( aIndex * 4 + 1, DIE, GC_GND, false, GC, supply );

    if( aModel.HasPOWERClamp() )
        simul += aModel.m_POWERClamp.Spice( aIndex * 4 + 2, PC_PWR, DIE, true, PC, supply );

    if( aModel.HasPulldown() )
        simul += aModel.m_pulldown.Spice( aIndex * 4 + 3, DIE, PD_GND, false, PD, supply );

    if( aModel.HasPullup() )
        simul += aModel.m_pullup.Spice( aIndex * 4 + 4, PU_PWR, DIE, false, PU, supply );

    return simul;
}


void KIBIS_PIN::getKuKdFromFile( const std::string& aSimul )
{
    const std::string outputFileName = m_topLevel->m_cacheDir + "temp_output.spice";

    if( std::remove( outputFileName.c_str() ) )
        Report( _( "Cannot remove temporary output file" ), RPT_SEVERITY_WARNING );

    std::shared_ptr<SPICE_SIMULATOR> ng = SIMULATOR::CreateInstance( "ng-kibis" );

    if( !ng )
        throw std::runtime_error( "Could not create simulator instance" );

    ng->Init();
    ng->LoadNetlist( aSimul );

    std::ifstream KuKdfile;
    KuKdfile.open( outputFileName );

    std::vector<double> ku, kd, t;

    if( KuKdfile )
    {
        std::string line;
        bool        valuesTagFound = false;

        while( std::getline( KuKdfile, line ) ) // skip ngspice output header
        {
            if( line.find( "Values:" ) != std::string::npos )
            {
                valuesTagFound = true;
                break;
            }
        }

        if( !valuesTagFound )
        {
            Report( _( "Missing 'Values:' tag in temporary file" ), RPT_SEVERITY_ERROR );
        }

        int    i = 0;
        double t_v, ku_v, kd_v;

        try
        {
            while( KuKdfile )
            {
                std::getline( KuKdfile, line );

                if( line.empty() )
                    continue;

                switch( i )
                {
                case 0:
                    line = line.substr( line.find_first_of( "\t" ) + 1 );
                    t_v = std::stod( line );
                    break;
                case 1: ku_v = std::stod( line ); break;
                case 2:
                    kd_v = std::stod( line );
                    ku.push_back( ku_v );
                    kd.push_back( kd_v );
                    t.push_back( t_v );
                    break;
                default: Report( _( "Error while reading temporary file" ), RPT_SEVERITY_ERROR );
                }

                i = ( i + 1 ) % 3;
            }
        }
        catch( const std::exception& )
        {
            Report( _( "Error while reading temporary file" ), RPT_SEVERITY_ERROR );
        }

        std::getline( KuKdfile, line );
    }
    else
    {
        Report( _( "Error while creating temporary output file" ), RPT_SEVERITY_ERROR );
    }

    if( std::remove( outputFileName.c_str() ) )
    {
        Report( _( "Cannot remove temporary output file" ), RPT_SEVERITY_WARNING );
    }

    m_Ku = std::move( ku );
    m_Kd = std::move( kd );
    m_t = std::move( t );
}


std::string KIBIS_PIN::KuKdDriver( KIBIS_MODEL&                                   aModel,
                                   const std::pair<IbisWaveform*, IbisWaveform*>& aPair,
                                   const KIBIS_PARAMETER& aParam, int aIndex )
{
    IBIS_CORNER    supply = aParam.m_supply;
    IBIS_CORNER    ccomp = aParam.m_Ccomp;
    KIBIS_WAVEFORM* wave = aParam.m_waveform;

    std::string simul = "";

    simul += "*THIS IS NOT A VALID SPICE MODEL.\n";
    simul += "*This part is intended to be executed by Kibis internally.\n";
    simul += "*You should not be able to read this.\n\n";

    simul += ".SUBCKT DRIVER";
    simul += std::to_string( aIndex );
    simul += " POWER GND PIN \n"; // 1: POWER, 2:GND, 3:PIN

    simul += "Vdummy 2 PIN 0\n";

    if( ( aPair.first->m_R_dut != 0 ) || ( aPair.first->m_L_dut != 0 )
        || ( aPair.first->m_C_dut != 0 ) )
    {
        Report( _( "Kibis does not support DUT values yet. "
                   "https://ibis.org/summits/nov16a/chen.pdf" ),
                RPT_SEVERITY_WARNING );
    }

    simul += "\n";
    simul += "CCPOMP 2 GND ";
    simul += doubleToString( aModel.m_C_comp.value[ccomp] ); //@TODO: Check the corner ?
    simul += "\n";

    const IbisWaveform* risingWF = aPair.first;
    const IbisWaveform* fallingWF = aPair.second;

    switch( wave->GetType() )
    {
    case KIBIS_WAVEFORM_TYPE::RECTANGULAR:
    case KIBIS_WAVEFORM_TYPE::PRBS:
    {
        wave->Check( risingWF, fallingWF );
        std::vector<std::pair<int, double>> bits = wave->GenerateBitSequence();
        bits = SimplifyBitSequence( bits );
        simul += aModel.generateSquareWave( "DIE0", "GND", bits, aPair, aParam );
        break;
    }
    case KIBIS_WAVEFORM_TYPE::STUCK_HIGH:
    {
        const IbisWaveform* waveform = wave->inverted ? risingWF : fallingWF;
        simul += "Vsig DIE0 GND ";
        simul += doubleToString( waveform->m_table.m_entries.at( 0 ).V.value[supply] );
        simul += "\n";
        break;
    }
    case KIBIS_WAVEFORM_TYPE::STUCK_LOW:
    {
        const IbisWaveform* waveform = wave->inverted ? fallingWF : risingWF;
        simul += "Vsig DIE0 GND ";
        simul += doubleToString( waveform->m_table.m_entries.at( 0 ).V.value[supply] );
        simul += "\n";
        break;
    }
    case KIBIS_WAVEFORM_TYPE::NONE:
    case KIBIS_WAVEFORM_TYPE::HIGH_Z:
    default:
        break;
    }

    simul += addDie( aModel, aParam, 0 );

    simul += "\n.ENDS DRIVER\n\n";
    return simul;
}


void KIBIS_PIN::getKuKdOneWaveform( KIBIS_MODEL&                                   aModel,
                                    const std::pair<IbisWaveform*, IbisWaveform*>& aPair,
                                    const KIBIS_PARAMETER&                         aParam )
{
    IBIS_CORNER           supply = aParam.m_supply;
    const KIBIS_WAVEFORM* wave = aParam.m_waveform;

    std::string simul = "";

    if( !wave )
        return;

    if( wave->GetType() == KIBIS_WAVEFORM_TYPE::NONE )
    {
        //@TODO , there could be some current flowing through pullup / pulldown transistors, even when off
        std::vector<double> ku, kd, t;
        ku.push_back( 0 );
        kd.push_back( 0 );
        t.push_back( 0 );
        m_Ku = std::move( ku );
        m_Kd = std::move( kd );
        m_t = std::move( t );
    }
    else
    {
        simul += KuKdDriver( aModel, aPair, aParam, 0 );
        simul += "\n x1 3 0 1 DRIVER0 \n";

        simul += "VCC 3 0 ";
        simul += doubleToString( aModel.m_voltageRange.value[supply] );
        simul += "\n";
        //simul += "Vpin x1.DIE 0 1 \n"
        simul += "Lfixture 1 4 ";
        simul += doubleToString( aPair.first->m_L_fixture );
        simul += "\n";
        simul += "Rfixture 4 5 ";
        simul += doubleToString( aPair.first->m_R_fixture );
        simul += "\n";
        simul += "Cfixture 4 0 ";
        simul += doubleToString( aPair.first->m_C_fixture );
        simul += "\n";
        simul += "Vfixture 5 0 ";
        simul += doubleToString( aPair.first->m_V_fixture );
        simul += "\n";
        simul += "VmeasIout x1.DIE0 x1.2 0\n";
        simul += "VmeasPD 0 x1.PD_GND0 0\n";
        simul += "VmeasPU x1.PU_PWR0 3 0\n";
        simul += "VmeasPC x1.PC_PWR0 3 0\n";
        simul += "VmeasGC 0 x1.GC_GND0 0\n";

        if( aModel.HasPullup() && aModel.HasPulldown() )
        {
            Report( _( "Model has only one waveform pair, reduced accuracy" ),
                    RPT_SEVERITY_WARNING );
            simul += "Bku KU 0 v=( (i(VmeasIout)-i(VmeasPC)-i(VmeasGC)-i(VmeasPD) "
                     ")/(i(VmeasPU)-i(VmeasPD)))\n";
            simul += "Bkd KD 0 v=(1-v(KU))\n";
        }

        else if( !aModel.HasPullup() && aModel.HasPulldown() )
        {
            simul += "Bku KD 0 v=( ( i(VmeasIout)+i(VmeasPC)+i(VmeasGC) )/(i(VmeasPD)))\n";
            simul += "Bkd KU 0 v=0\n";
        }

        else if( aModel.HasPullup() && !aModel.HasPulldown() )
        {
            simul += "Bku KU 0 v=( ( i(VmeasIout)+i(VmeasPC)+i(VmeasGC) )/(i(VmeasPU)))\n";
            simul += "Bkd KD 0 v=0\n";
        }
        else
        {
            Report( _( "Driver needs at least a pullup or a pulldown" ), RPT_SEVERITY_ERROR );
        }

        switch( wave->GetType() )
        {
        case KIBIS_WAVEFORM_TYPE::PRBS:
        case KIBIS_WAVEFORM_TYPE::RECTANGULAR:
        {
            double duration = wave->GetDuration();
            simul += ".tran 0.1n ";
            simul += doubleToString( duration );
            simul += "\n";
            break;
        }
        case KIBIS_WAVEFORM_TYPE::HIGH_Z:
        case KIBIS_WAVEFORM_TYPE::STUCK_LOW:
        case KIBIS_WAVEFORM_TYPE::STUCK_HIGH:
        default:
            simul += ".tran 0.5 1 \n"; //
        }

        //simul += ".dc Vpin -5 5 0.1\n";
        simul += ".control run \n";
        simul += "set filetype=ascii\n";
        simul += "run \n";
        //simul += "plot v(x1.DIE0) i(VmeasIout) i(VmeasPD) i(VmeasPU) i(VmeasPC) i(VmeasGC)\n";
        //simul += "plot v(KU) v(KD)\n";

        std::string outputFileName = m_topLevel->m_cacheDir + "temp_output.spice";
        simul += "write '" + outputFileName + "' v(KU) v(KD)\n";
        simul += "quit\n";
        simul += ".endc \n";
        simul += ".end \n";

        getKuKdFromFile( simul );
    }
}


void KIBIS_PIN::getKuKdNoWaveform( KIBIS_MODEL& aModel, const KIBIS_PARAMETER& aParam )
{
    std::vector<double>   ku, kd, t;
    const KIBIS_WAVEFORM* wave = aParam.m_waveform;
    const IBIS_CORNER&    supply = aParam.m_supply;

    if( !wave )
        return;

    switch( wave->GetType() )
    {
    case KIBIS_WAVEFORM_TYPE::RECTANGULAR:
    case KIBIS_WAVEFORM_TYPE::PRBS:
    {
        wave->Check( aModel.m_ramp.m_rising, aModel.m_ramp.m_falling );
        std::vector<std::pair<int, double>> bits = wave->GenerateBitSequence();
        bits = SimplifyBitSequence( bits );

        for( const std::pair<int, double>& bit : bits )
        {
            ku.push_back( bit.first ? 0 : 1 );
            kd.push_back( bit.first ? 1 : 0 );
            t.push_back( bit.second );
            ku.push_back( bit.first ? 1 : 0 );
            kd.push_back( bit.first ? 0 : 1 );
            t.push_back( bit.second
                         + ( bit.first ? +aModel.m_ramp.m_rising.value[supply].m_dt
                                       : aModel.m_ramp.m_falling.value[supply].m_dt )
                                   / 0.6 );
            // 0.6 because ibis only gives 20%-80% time
        }
        break;
    }
    case KIBIS_WAVEFORM_TYPE::STUCK_HIGH:
    {
        ku.push_back( wave->inverted ? 0 : 1 );
        kd.push_back( wave->inverted ? 1 : 0 );
        t.push_back( 0 );
        break;
    }
    case KIBIS_WAVEFORM_TYPE::STUCK_LOW:
    {
        ku.push_back( wave->inverted ? 1 : 0 );
        kd.push_back( wave->inverted ? 0 : 1 );
        t.push_back( 0 );
        break;
    }
    case KIBIS_WAVEFORM_TYPE::HIGH_Z:
    case KIBIS_WAVEFORM_TYPE::NONE:
    default:
        ku.push_back( 0 );
        kd.push_back( 0 );
        t.push_back( 0 );
    }

    m_Ku = std::move( ku );
    m_Kd = std::move( kd );
    m_t = std::move( t );
}


void KIBIS_PIN::getKuKdTwoWaveforms( KIBIS_MODEL&                                   aModel,
                                     const std::pair<IbisWaveform*, IbisWaveform*>& aPair1,
                                     const std::pair<IbisWaveform*, IbisWaveform*>& aPair2,
                                     const KIBIS_PARAMETER&                         aParam )
{
    std::string           simul = "";
    const IBIS_CORNER     supply = aParam.m_supply;
    const KIBIS_WAVEFORM* wave = aParam.m_waveform;

    if( !wave )
        return;

    if( wave->GetType() == KIBIS_WAVEFORM_TYPE::NONE )
    {
        //@TODO , there could be some current flowing through pullup / pulldown transistors, even when off
        std::vector<double> ku, kd, t;
        ku.push_back( 0 );
        kd.push_back( 0 );
        t.push_back( 0 );
        m_Ku = std::move( ku );
        m_Kd = std::move( kd );
        m_t = std::move( t );
    }
    else
    {
        simul += KuKdDriver( aModel, aPair1, aParam, 0 );
        simul += KuKdDriver( aModel, aPair2, aParam, 1 );
        simul += "\n x1 3 0 1 DRIVER0 \n";

        simul += "VCC 3 0 ";
        simul += doubleToString( aModel.m_voltageRange.value[supply] );
        simul += "\n";
        //simul += "Vpin x1.DIE 0 1 \n"
        simul += "Lfixture0 1 4 ";
        simul += doubleToString( aPair1.first->m_L_fixture );
        simul += "\n";
        simul += "Rfixture0 4 5 ";
        simul += doubleToString( aPair1.first->m_R_fixture );
        simul += "\n";
        simul += "Cfixture0 4 0 ";
        simul += doubleToString( aPair1.first->m_C_fixture );
        simul += "\n";
        simul += "Vfixture0 5 0 ";
        simul += doubleToString( aPair1.first->m_V_fixture );
        simul += "\n";
        simul += "VmeasIout0 x1.2 x1.DIE0 0\n";
        simul += "VmeasPD0 0 x1.PD_GND0 0\n";
        simul += "VmeasPU0 x1.PU_PWR0 3 0\n";
        simul += "VmeasPC0 x1.PC_PWR0 3 0\n";
        simul += "VmeasGC0 0 x1.GC_GND0 0\n";


        simul += "\n x2 3 0 7 DRIVER1 \n";
        //simul += "Vpin x1.DIE 0 1 \n"
        simul += "Lfixture1 7 8 ";
        simul += doubleToString( aPair2.first->m_L_fixture );
        simul += "\n";
        simul += "Rfixture1 8 9 ";
        simul += doubleToString( aPair2.first->m_R_fixture );
        simul += "\n";
        simul += "Cfixture1 8 0 ";
        simul += doubleToString( aPair2.first->m_C_fixture );
        simul += "\n";
        simul += "Vfixture1 9 0 ";
        simul += doubleToString( aPair2.first->m_V_fixture );
        simul += "\n";
        simul += "VmeasIout1 x2.2 x2.DIE0 0\n";
        simul += "VmeasPD1 0 x2.PD_GND0 0\n";
        simul += "VmeasPU1 x2.PU_PWR0 3 0\n";
        simul += "VmeasPC1 x2.PC_PWR0 3 0\n";
        simul += "VmeasGC1 0 x2.GC_GND0 0\n";

        if( aModel.HasPullup() && aModel.HasPulldown() )
        {
            simul +=
                    "Bku KU 0 v=(  ( i(VmeasPD1) * ( i(VmeasIout0) + i(VmeasPC0) + i(VmeasGC0) ) - "
                    "i(VmeasPD0) * ( i(VmeasIout1) + i(VmeasPC1) + i(VmeasGC1) )  )/ ( i(VmeasPU1) "
                    "* "
                    "i(VmeasPD0) - i(VmeasPU0) * i(VmeasPD1)  ) )\n";
            simul +=
                    "Bkd KD 0 v=(  ( i(VmeasPU1) * ( i(VmeasIout0) + i(VmeasPC0) + i(VmeasGC0) ) - "
                    "i(VmeasPU0) * ( i(VmeasIout1) + i(VmeasPC1) + i(VmeasGC1) )  )/ ( i(VmeasPD1) "
                    "* "
                    "i(VmeasPU0) - i(VmeasPD0) * i(VmeasPU1)  ) )\n";
            //simul += "Bkd KD 0 v=(1-v(KU))\n";
        }

        else if( !aModel.HasPullup() && aModel.HasPulldown() )
        {
            Report( _( "There are two waveform pairs, but only one transistor. More equations than "
                       "unknowns." ),
                    RPT_SEVERITY_WARNING );
            simul += "Bku KD 0 v=( ( i(VmeasIout0)+i(VmeasPC0)+i(VmeasGC0) )/(i(VmeasPD0)))\n";
            simul += "Bkd KU 0 v=0\n";
        }

        else if( aModel.HasPullup() && !aModel.HasPulldown() )
        {
            Report( _( "There are two waveform pairs, but only one transistor. More equations than "
                       "unknowns." ),
                    RPT_SEVERITY_WARNING );
            simul += "Bku KU 0 v=( ( i(VmeasIout)+i(VmeasPC)+i(VmeasGC) )/(i(VmeasPU)))\n";
            simul += "Bkd KD 0 v=0\n";
        }
        else
        {
            Report( _( "Driver needs at least a pullup or a pulldown" ), RPT_SEVERITY_ERROR );
        }

        switch( wave->GetType() )
        {
        case KIBIS_WAVEFORM_TYPE::RECTANGULAR:
        case KIBIS_WAVEFORM_TYPE::PRBS:
        {
            double duration = wave->GetDuration();
            simul += ".tran 0.1n ";
            simul += doubleToString( duration );
            simul += "\n";
            break;
        }
        case KIBIS_WAVEFORM_TYPE::HIGH_Z:
        case KIBIS_WAVEFORM_TYPE::STUCK_LOW:
        case KIBIS_WAVEFORM_TYPE::STUCK_HIGH:
        default:
            simul += ".tran 0.5 1 \n"; //
        }

        //simul += ".dc Vpin -5 5 0.1\n";
        simul += ".control run \n";
        simul += "set filetype=ascii\n";
        simul += "run \n";
        simul += "plot v(KU) v(KD)\n";
        //simul += "plot v(x1.DIE0) \n";
        std::string outputFileName = m_topLevel->m_cacheDir + "temp_output.spice";
        simul += "write '" + outputFileName + "' v(KU) v(KD)\n";
        simul += "quit\n";
        simul += ".endc \n";
        simul += ".end \n";

        getKuKdFromFile( simul );
    }
}


bool KIBIS_PIN::writeSpiceDriver( std::string& aDest, const std::string& aName, KIBIS_MODEL& aModel,
                                  const KIBIS_PARAMETER& aParam )
{
    bool status = true;

    switch( aModel.m_type )
    {
    case IBIS_MODEL_TYPE::OUTPUT:
    case IBIS_MODEL_TYPE::IO:
    case IBIS_MODEL_TYPE::THREE_STATE:
    case IBIS_MODEL_TYPE::OPEN_DRAIN:
    case IBIS_MODEL_TYPE::IO_OPEN_DRAIN:
    case IBIS_MODEL_TYPE::OPEN_SINK:
    case IBIS_MODEL_TYPE::IO_OPEN_SINK:
    case IBIS_MODEL_TYPE::OPEN_SOURCE:
    case IBIS_MODEL_TYPE::IO_OPEN_SOURCE:
    case IBIS_MODEL_TYPE::OUTPUT_ECL:
    case IBIS_MODEL_TYPE::IO_ECL:
    case IBIS_MODEL_TYPE::THREE_STATE_ECL:
    {
        std::string result;
        std::string tmp;

        result = "\n*Driver model generated by Kicad using Ibis data. ";

        result += "\n*Pin number: ";
        result += m_pinNumber;
        result += "\n*Signal name: ";
        result += m_signalName;
        result += "\n*Model: ";
        result += aModel.m_name;
        result += "\n.SUBCKT ";
        result += aName;
        result += " GND PIN \n";
        result += "\n";

        result += "RPIN 1 PIN ";
        result += doubleToString( m_Rpin.value[aParam.m_Rpin] );
        result += "\n";
        result += "LPIN DIE0 1 ";
        result += doubleToString( m_Lpin.value[aParam.m_Lpin] );
        result += "\n";
        result += "CPIN PIN GND ";
        result += doubleToString( m_Cpin.value[aParam.m_Cpin] );
        result += "\n";

        std::vector<std::pair<IbisWaveform*, IbisWaveform*>> wfPairs = aModel.waveformPairs();
        KIBIS_ACCURACY                                       accuracy = aParam.m_accuracy;

        if( wfPairs.size() < 1 || accuracy <= KIBIS_ACCURACY::LEVEL_0 )
        {
            if( accuracy > KIBIS_ACCURACY::LEVEL_0 )
            {
                Report( _( "Model has no waveform pair, using [Ramp] instead, poor accuracy" ),
                        RPT_SEVERITY_INFO );
            }

            getKuKdNoWaveform( aModel, aParam );
        }
        else if( wfPairs.size() == 1 || accuracy <= KIBIS_ACCURACY::LEVEL_1 )
        {
            getKuKdOneWaveform( aModel, wfPairs.at( 0 ), aParam );
        }
        else
        {
            if( wfPairs.size() > 2 || accuracy <= KIBIS_ACCURACY::LEVEL_2 )
            {
                Report( _( "Model has more than 2 waveform pairs, using the first two." ),
                        RPT_SEVERITY_WARNING );
            }

            getKuKdTwoWaveforms( aModel, wfPairs.at( 0 ), wfPairs.at( 1 ), aParam );
        }

        result += "Vku KU GND pwl ( ";

        for( size_t i = 0; i < m_t.size(); i++ )
        {
            result += doubleToString( m_t.at( i ) );
            result += " ";
            result += doubleToString( m_Ku.at( i ) );
            result += " ";
        }

        result += ") \n";


        result += "Vkd KD GND pwl ( ";

        for( size_t i = 0; i < m_t.size(); i++ )
        {
            result += doubleToString( m_t.at( i ) );
            result += " ";
            result += doubleToString( m_Kd.at( i ) );
            result += " ";
        }

        result += ") \n";

        result += aModel.SpiceDie( aParam, 0, true );

        result += "\n.ENDS DRIVER\n\n";

        aDest += result;
        break;
    }
    default:
        Report( _( "Invalid model type for a driver." ), RPT_SEVERITY_ERROR );
        status = false;
    }

    return status;
}


bool KIBIS_PIN::writeSpiceDevice( std::string& aDest, const std::string& aName, KIBIS_MODEL& aModel,
                                  const KIBIS_PARAMETER& aParam )
{
    bool status = true;

    switch( aModel.m_type )
    {
    case IBIS_MODEL_TYPE::INPUT_STD:
    case IBIS_MODEL_TYPE::IO:
    case IBIS_MODEL_TYPE::IO_OPEN_DRAIN:
    case IBIS_MODEL_TYPE::IO_OPEN_SINK:
    case IBIS_MODEL_TYPE::IO_OPEN_SOURCE:
    case IBIS_MODEL_TYPE::IO_ECL:
    {
        std::string result;

        result += "\n";
        result = "*Device model generated by Kicad using Ibis data.";
        result += "\n.SUBCKT ";
        result += aName;
        result += " GND PIN\n";
        result += "\n";
        result += "\n";
        result += "RPIN 1 PIN ";
        result += doubleToString( m_Rpin.value[aParam.m_Rpin] );
        result += "\n";
        result += "LPIN DIE0 1 ";
        result += doubleToString( m_Lpin.value[aParam.m_Lpin] );
        result += "\n";
        result += "CPIN PIN GND ";
        result += doubleToString( m_Cpin.value[aParam.m_Cpin] );
        result += "\n";

        result += aModel.SpiceDie( aParam, 0, false );

        result += "\n.ENDS DEVICE\n\n";

        aDest = std::move( result );
        break;
    }
    default:
        Report( _( "Invalid model type for a device" ), RPT_SEVERITY_ERROR );
        status = false;
    }

    return status;
}


bool KIBIS_PIN::writeSpiceDiffDriver( std::string& aDest, const std::string& aName,
                                      KIBIS_MODEL& aModel, const KIBIS_PARAMETER& aParam )
{
    bool status = true;
    KIBIS_WAVEFORM* wave = aParam.m_waveform;

    if( !wave )
        return false;

    std::string result;
    result = "\n*Differential driver model generated by Kicad using Ibis data. ";

    result += "\n.SUBCKT ";
    result += aName;
    result += " GND PIN_P PIN_N\n";
    result += "\n";

    status &= writeSpiceDriver( result, aName + "_P", aModel, aParam );
    wave->inverted = !wave->inverted;
    status &= writeSpiceDriver( result, aName + "_N", aModel, aParam );
    wave->inverted = !wave->inverted;


    result += "\n";
    result += "x1 GND PIN_P " + aName + "_P \n";
    result += "x2 GND PIN_N " + aName + "_N \n";
    result += "\n";

    result += "\n.ENDS " + aName + "\n\n";

    if( status )
        aDest += result;

    return status;
}


bool KIBIS_PIN::writeSpiceDiffDevice( std::string& aDest, const std::string& aName,
                                      KIBIS_MODEL& aModel, const KIBIS_PARAMETER& aParam )
{
    bool status = true;

    std::string result;
    result = "\n*Differential device model generated by Kicad using Ibis data. ";

    result += "\n.SUBCKT ";
    result += aName;
    result += " GND PIN_P PIN_N\n";
    result += "\n";

    status &= writeSpiceDevice( result, aName + "_P", aModel, aParam );
    status &= writeSpiceDevice( result, aName + "_N", aModel, aParam );

    result += "\n";
    result += "x1 GND PIN_P " + aName + "_P \n";
    result += "x2 GND PIN_N " + aName + "_N \n";
    result += "\n";

    result += "\n.ENDS " + aName + "\n\n";

    if( status )
        aDest += result;

    return status;
}


KIBIS_MODEL* KIBIS::GetModel( const std::string& aName )
{
    for( KIBIS_MODEL& model : m_models )
    {
        if( model.m_name == aName )
            return &model;
    }

    return nullptr;
}


KIBIS_COMPONENT* KIBIS::GetComponent( const std::string& aName )
{
    for( KIBIS_COMPONENT& cmp : m_components )
    {
        if( cmp.m_name == aName )
            return &cmp;
    }

    return nullptr;
}


void KIBIS_PARAMETER::SetCornerFromString( IBIS_CORNER& aCorner, const std::string& aString )
{
    if( aString == "MIN" )
        aCorner = IBIS_CORNER::MIN;
    else if( aString == "MAX" )
        aCorner = IBIS_CORNER::MAX;
    else
        aCorner = IBIS_CORNER::TYP;
}


std::vector<std::pair<int, double>> KIBIS_WAVEFORM_STUCK_HIGH::GenerateBitSequence() const
{
    std::vector<std::pair<int, double>> bits;
    std::pair<int, double>              bit;
    bit.first = inverted ? 1 : 0;
    bit.second = 0;
    return bits;
}


std::vector<std::pair<int, double>> KIBIS_WAVEFORM_STUCK_LOW::GenerateBitSequence() const
{
    std::vector<std::pair<int, double>> bits;
    std::pair<int, double>              bit;
    bit.first = inverted ? 0 : 1;
    bit.second = 0;
    return bits;
}

std::vector<std::pair<int, double>> KIBIS_WAVEFORM_HIGH_Z::GenerateBitSequence() const
{
    std::vector<std::pair<int, double>> bits;
    return bits;
}

std::vector<std::pair<int, double>> KIBIS_WAVEFORM_RECTANGULAR::GenerateBitSequence() const
{
    std::vector<std::pair<int, double>> bits;

    for( int i = 0; i < m_cycles; i++ )
    {
        std::pair<int, double> bit;
        bit.first = inverted ? 0 : 1;
        bit.second = ( m_ton + m_toff ) * i + m_delay;
        bits.push_back( bit );

        bit.first = inverted ? 1 : 0;
        bit.second = ( m_ton + m_toff ) * i + m_delay + m_ton;
        bits.push_back( bit );
    }

    return bits;
}


std::vector<std::pair<int, double>> KIBIS_WAVEFORM_PRBS::GenerateBitSequence() const
{
    std::vector<std::pair<int, double>> bitSequence;
    uint8_t polynomial = 0b1100000;
    //1100000 = x^7+x^6+1
    //10100 = x^5+x^3+1
    //110 = x^3+x^2+1
    uint8_t seed = 0x12; // Any non zero state
    uint8_t lfsr = seed;

    if ( m_bitrate == 0 )
        return bitSequence;

    double period = 1/m_bitrate;
    double t = 0;

    wxASSERT( m_bits > 0 );

    int bits = 0;

    do
    {
        uint8_t lsb = lfsr & 0x01;
        bitSequence.emplace_back( ( static_cast<uint8_t>( inverted ) ^ lsb ? 1 : 0 ), t );
        lfsr = lfsr >> 1;

        if ( lsb )
            lfsr ^= polynomial;

        t += period;

    } while ( ++bits < m_bits );

    return bitSequence;
}

bool KIBIS_WAVEFORM_RECTANGULAR::Check( const IbisWaveform* aRisingWf,
                                        const IbisWaveform* aFallingWf ) const
{
    bool status = true;

    if( m_cycles < 1 )
    {
        status = false;
        Report( _( "Number of cycles should be greater than 0." ), RPT_SEVERITY_ERROR );
    }

    if( m_ton <= 0 )
    {
        status = false;
        Report( _( "ON time should be greater than 0." ), RPT_SEVERITY_ERROR );
    }

    if( m_toff <= 0 )
    {
        status = false;
        Report( _( "OFF time should be greater than 0." ), RPT_SEVERITY_ERROR );
    }

    if( aRisingWf )
    {
        if( m_ton < aRisingWf->m_table.m_entries.back().t )
        {
            status = false;
            Report( _( "Rising edge is longer than on time." ), RPT_SEVERITY_WARNING );
        }
    }

    if( aFallingWf )
    {
        if( m_toff < aFallingWf->m_table.m_entries.back().t )
        {
            status = false;
            Report( _( "Falling edge is longer than off time." ), RPT_SEVERITY_WARNING );
        }
    }

    status &= aRisingWf && aFallingWf;

    return status;
}


bool KIBIS_WAVEFORM_RECTANGULAR::Check( const dvdtTypMinMax& aRisingRp,
                                        const dvdtTypMinMax& aFallingRp ) const
{
    bool status = true;

    if( m_cycles < 1 )
    {
        status = false;
        Report( _( "Number of cycles should be greater than 0." ), RPT_SEVERITY_ERROR );
    }

    if( m_ton <= 0 )
    {
        status = false;
        Report( _( "ON time should be greater than 0." ), RPT_SEVERITY_ERROR );
    }

    if( m_toff <= 0 )
    {
        status = false;
        Report( _( "OFF time should be greater than 0." ), RPT_SEVERITY_ERROR );
    }

    if( ( m_ton < aRisingRp.value[IBIS_CORNER::TYP].m_dt / 0.6 )
        || ( m_ton < aRisingRp.value[IBIS_CORNER::MIN].m_dt / 0.6 )
        || ( m_ton < aRisingRp.value[IBIS_CORNER::MAX].m_dt / 0.6 ) )
    {
        status = false;
        Report( _( "Rising edge is longer than ON time." ), RPT_SEVERITY_ERROR );
    }

    if( ( m_toff < aFallingRp.value[IBIS_CORNER::TYP].m_dt / 0.6 )
        || ( m_toff < aFallingRp.value[IBIS_CORNER::MIN].m_dt / 0.6 )
        || ( m_toff < aFallingRp.value[IBIS_CORNER::MAX].m_dt / 0.6 ) )
    {
        status = false;
        Report( _( "Falling edge is longer than OFF time." ), RPT_SEVERITY_ERROR );
    }

    return status;
}


bool KIBIS_WAVEFORM_PRBS::Check( const dvdtTypMinMax& aRisingRp,
                                 const dvdtTypMinMax& aFallingRp ) const
{
    bool status = true;

    if( m_bitrate <= 0 )
    {
        status = false;
        Report( _( "Bitrate should be greater than 0." ), RPT_SEVERITY_ERROR );
    }

    if( m_bits <= 0 )
    {
        status = false;
        Report( _( "Number of bits should be greater than 0." ), RPT_SEVERITY_ERROR );
    }

    if( m_bitrate
        && ( ( 1 / m_bitrate ) < ( aRisingRp.value[IBIS_CORNER::TYP].m_dt / 0.6
                                   + aFallingRp.value[IBIS_CORNER::TYP].m_dt / 0.6 ) )
        && ( ( 1 / m_bitrate ) < ( aRisingRp.value[IBIS_CORNER::TYP].m_dt / 0.6
                                   + aFallingRp.value[IBIS_CORNER::TYP].m_dt / 0.6 ) )
        && ( ( 1 / m_bitrate ) < ( aRisingRp.value[IBIS_CORNER::TYP].m_dt / 0.6
                                   + aFallingRp.value[IBIS_CORNER::TYP].m_dt / 0.6 ) ) )
    {
        status = false;
        Report( _( "Bitrate is too high for rising / falling edges" ), RPT_SEVERITY_ERROR );
    }

    return status;
}

bool KIBIS_WAVEFORM_PRBS::Check( const IbisWaveform* aRisingWf,
                                 const IbisWaveform* aFallingWf ) const
{
    bool status = true;

    if( m_bitrate <= 0 )
    {
        status = false;
        Report( _( "Bitrate should be greater than 0." ), RPT_SEVERITY_ERROR );
    }

    if( m_bits <= 0 )
    {
        status = false;
        Report( _( "Number of bits should be greater than 0." ), RPT_SEVERITY_ERROR );
    }

    if( m_bitrate && aRisingWf && aFallingWf
        && ( ( 1 / m_bitrate ) < ( aRisingWf->m_table.m_entries.back().t
                                   + aFallingWf->m_table.m_entries.back().t ) ) )
    {
        status = false;
        Report( _( "Bitrate could be too high for rising / falling edges" ), RPT_SEVERITY_WARNING );
    }

    return status;
}
