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


#include "ibis_parser.h"

#include <sstream>
#include <cstring> //for memcmp
#include <iterator>
#include <locale_io.h> // KiCad header
#include <wx/log.h>

// _() is used here to mark translatable strings in IBIS_REPORTER::Report()
// However, currently non ASCII7 chars are nor correctly handled when printing messages
// So we disable translations
#if 0
#include <wx/intl.h>    // for _() macro and wxGetTranslation()
#else
#undef _
#define _( x ) x
#endif


bool IbisParser::compareIbisWord( const std::string& stra, const std::string& strb )
{
    return std::equal( stra.begin(), stra.end(),
                        strb.begin(), strb.end(),
                        [](char a, char b)
                        {
                            return std::tolower(a) == std::tolower(b);
                        });
}


bool IBIS_MATRIX::Check()
{
    if( ( m_rows < 1 ) || ( m_cols < 1 ) )
    {
        Report( _( "Dimension of matrices should be >= 1." ), RPT_SEVERITY_ERROR );
        return false;
    }

    for( size_t i = 0; i < m_data.size(); i++ )
    {
        if( std::isnan( m_data[i] ) )
        {
            Report( _( "There are NaN elements in a matrix." ), RPT_SEVERITY_ERROR );
            return false;
        }
    }

    return true;
}


bool isNumberNA( double aNumber )
{
    double NA = nan( NAN_NA );
    return std::memcmp( &aNumber, &NA, sizeof NA ) == 0;
}


bool TypMinMaxValue::isNA() const
{
    return isNumberNA( value[IBIS_CORNER::TYP] );
}


bool TypMinMaxValue::Check()
{
    bool status = true;

    if( std::isnan( value[IBIS_CORNER::TYP] ) && !isNumberNA( value[IBIS_CORNER::TYP] ) )
        status = false;

    if( std::isnan( value[IBIS_CORNER::MIN] ) && !isNumberNA( value[IBIS_CORNER::MIN] ) )
        status = false;

    if( std::isnan( value[IBIS_CORNER::MAX] ) && !isNumberNA( value[IBIS_CORNER::MAX] ) )
        status = false;

    return status;
}


void TypMinMaxValue::Add( const TypMinMaxValue& aValue )
{
    if( !aValue.isNA() )
    {
        value[IBIS_CORNER::TYP] += aValue.value[IBIS_CORNER::TYP];
        if( isNumberNA( value[IBIS_CORNER::MIN] ) )
            value[IBIS_CORNER::MIN] += aValue.value[IBIS_CORNER::TYP];
        else
            value[IBIS_CORNER::MIN] += aValue.value[IBIS_CORNER::MIN];
        if( isNumberNA( value[IBIS_CORNER::MAX] ) )
            value[IBIS_CORNER::MAX] += aValue.value[IBIS_CORNER::TYP];
        else
            value[IBIS_CORNER::MAX] += aValue.value[IBIS_CORNER::MAX];
    }
}


bool IbisComponentPackage::Check()
{
    bool status =  true;

    if( !m_Rpkg.Check() || m_Rpkg.isNA() )
    {
        Report( _( "Missing or invalid R_pkg value." ), RPT_SEVERITY_ERROR );
        status = false;
    }

    if( !m_Lpkg.Check() || m_Lpkg.isNA() )
    {
        Report( _( "Missing or invalid L_pkg value." ), RPT_SEVERITY_ERROR );
        status = false;
    }

    if( !m_Cpkg.Check() || m_Cpkg.isNA() )
    {
        Report( _( "Missing or invalid C_pkg value." ), RPT_SEVERITY_ERROR );
        status = false;
    }

    return status;
}


bool IbisComponentPin::Check()
{
    bool status = true;

    if( !m_dummy )
    {
        std::stringstream message;
        message << _( "Checking pin " ) << m_pinName;

        if( m_pinName.empty() )
        {
            if( status )
                Report( message.str(), RPT_SEVERITY_ACTION );

            Report( _( "Pin name cannot be empty." ), RPT_SEVERITY_ERROR );
            status = false;
        }

        if( m_signalName.empty() )
        {
            if( status )
                Report( message.str(), RPT_SEVERITY_ACTION );

            Report( _( "Signal name cannot be empty." ), RPT_SEVERITY_ERROR );
            status = false;
        }

        if( m_modelName.empty() )
        {
            if( status )
                Report( message.str(), RPT_SEVERITY_ACTION );

            Report( _( "Model name cannot be empty." ), RPT_SEVERITY_ERROR );
            status = false;
        }

        if( std::isnan( m_Rpin ) && !isNumberNA( m_Rpin ) )
        {
            if( status )
                Report( message.str(), RPT_SEVERITY_ACTION );

           Report( _( "Rpin is not valid." ), RPT_SEVERITY_ERROR );
           status = false;
        }

        if( std::isnan( m_Lpin )&& !isNumberNA( m_Lpin )  )
        {
            if( status )
                Report( message.str(), RPT_SEVERITY_ACTION );

            Report( _( "Lpin is not valid." ), RPT_SEVERITY_ERROR );
            status = false;
        }

        if( std::isnan( m_Cpin )&& !isNumberNA( m_Cpin ) )
        {
            if( status )
                Report( message.str(), RPT_SEVERITY_ACTION );

            Report( _( "Cpin is not valid." ), RPT_SEVERITY_ERROR );
            status = false;
        }
    }

    return status;
}


bool IbisComponent::Check()
{
    bool status = true;

    std::stringstream message;
    message << _( "Checking component " ) << m_name;

    if( m_name.empty() )
    {
        if( status )
            Report( message.str(), RPT_SEVERITY_ACTION );

        Report( _( "Component: name cannot be empty." ), RPT_SEVERITY_ERROR );
        status = false;
    }

    if( m_manufacturer.empty() )
    {
        if( status )
            Report( message.str(), RPT_SEVERITY_ACTION );

        Report( _( "Component: manufacturer cannot be empty." ), RPT_SEVERITY_ERROR );
        status = false;
    }

    if( !m_package.Check() )
    {
        if( status )
            Report( message.str(), RPT_SEVERITY_ACTION );

        Report( _( "Component: invalid package." ), RPT_SEVERITY_ERROR );
        status = false;
    }

    if( m_pins.size() < 1 )
    {
        if( status )
            Report( message.str(), RPT_SEVERITY_ACTION );

        Report( _( "Component: no pin" ), RPT_SEVERITY_ERROR );
        status = false;
    }

    for( IbisComponentPin& pin : m_pins )
        status &= pin.Check();

    return status;
}


bool IbisModelSelector::Check()
{
    return true;
}


std::string IBIS_BASE::doubleToString( double aNumber )
{
    std::ostringstream ss;
    ss.setf( std::ios_base::scientific, std::ios_base::floatfield );
    ss << aNumber;
    return ss.str();
}


std::string IVtable::Spice( int aN, const std::string& aPort1, const std::string& aPort2, bool aNegateI,
                            const std::string& aModelName, IBIS_CORNER aCorner ) const
{
    std::string result = "";

    if( m_entries.size() > 0 )
    {
        result += "a";
        result += std::to_string( aN );
        result += " %vd(";
        result += aPort1;
        result += " ";
        result += aPort2;
        result += ") %id(";
        result += aNegateI ? aPort2 : aPort1;
        result += " ";
        result += aNegateI ? aPort1 : aPort2;
        result += ") ";
        result += aModelName;
        result += "\n";
        result += "\n";
        result += ".model ";
        result += aModelName;
        result += " pwl(\n+ x_array=[";

        for( const IVtableEntry& entry : m_entries )
        {
            if( !isNumberNA( entry.I.value[aCorner] ) )
            {
                result += doubleToString( entry.V );
                result += "\n+";
            }
        }

        result += "]\n+ y_array=[";

        for( const IVtableEntry& entry : m_entries )
        {
            if( !isNumberNA( entry.I.value[aCorner] ) )
            {
                result += doubleToString( entry.I.value[aCorner] );
                result += "\n+";
            }
        }

        result += "]\n+ input_domain=0.05 fraction=TRUE)\n\n";
    }
    return result;
}

double IVtable::InterpolatedI( double aV, IBIS_CORNER aCorner ) const
{
    // @TODO change this algorithm

    if( m_entries.back().V > m_entries.at( 0 ).V )
    {
        if( aV >= m_entries.back().V )
            return m_entries.back().I.value[aCorner];

        if( aV <= m_entries.at( 0 ).V )
            return m_entries.at( 0 ).I.value[aCorner];

        for( size_t i = 1; i < m_entries.size(); i++ )
        {
            if( m_entries.at( i ).V > aV )
            {
                return m_entries.at( i - 1 ).I.value[aCorner]
                       + ( m_entries.at( i ).I.value[aCorner]
                           - m_entries.at( i - 1 ).I.value[aCorner] )
                                 / ( m_entries.at( i ).V - m_entries.at( i - 1 ).V )
                                 * ( aV - m_entries.at( i - 1 ).V );
            }
        }

        Report( _( "Cannot interpolate the current based on this IV table." ), RPT_SEVERITY_ERROR );
        return nan( "" );
    }
    else
    {
        // exiting the function here would mean the IV table is reversed.
        return nan( "" );
    }
    // @TODO prefer another method such as a dichotomy
}

bool IVtable::Check()
{
    bool status = true;

    for( IVtableEntry& entry : m_entries )
    {
        if( std::isnan( entry.V ) )
        {
            Report( _( "There is an invalid voltage in an IV table" ), RPT_SEVERITY_ERROR );
            status = false;
            break;
        }

        if( !entry.I.Check() )
        {
            Report( _( "There is an invalid current in an IV table" ), RPT_SEVERITY_ERROR );
            status = false;
            break;
        }
    }

    // TODO: Check if the IV table is monotonic :
    // IBIS standard defines 8 criteria for an IV table to be monotonic
    // Issue a warning, not an error

    return status;
}


bool dvdtTypMinMax::Check()
{
    bool status = true;

    if( std::isnan( value[IBIS_CORNER::TYP].m_dv ) )
        status = false;

    if( std::isnan( value[IBIS_CORNER::TYP].m_dt ) )
        status = false;

    if( std::isnan( value[IBIS_CORNER::MIN].m_dv ) && !isNumberNA( value[IBIS_CORNER::MIN].m_dv ) )
        status = false;

    if( std::isnan( value[IBIS_CORNER::MIN].m_dt ) && !isNumberNA( value[IBIS_CORNER::MIN].m_dt ) )
        status = false;

    if( std::isnan( value[IBIS_CORNER::MIN].m_dv ) && !isNumberNA( value[IBIS_CORNER::MIN].m_dv ) )
        status = false;

    if( std::isnan( value[IBIS_CORNER::MIN].m_dt ) && !isNumberNA( value[IBIS_CORNER::MIN].m_dt ) )
        status = false;

    return status;
}

bool IbisRamp::Check()
{
    bool status = true;

    if( std::isnan( m_Rload ) )
    {
        status = false;
        Report( _( "Invalid R_load." ), RPT_SEVERITY_ERROR );
    }

    if( !m_falling.Check() )
    {
        Report( _( "Invalid falling dV/dt." ), RPT_SEVERITY_ERROR );
        status = false;
    }

    if( !m_rising.Check() )
    {
        Report( _( "Invalid rising dV/dt." ), RPT_SEVERITY_ERROR );
        status = false;
    }

    return status;
}



bool IbisModel::Check()
{
    bool status = true;

    if( m_name.empty() )
    {
        Report( _( "Model name cannot be empty" ), RPT_SEVERITY_ERROR );
        status = false;
    }

    std::stringstream message;
    message << _( "Checking model " ) << m_name;

    if( m_type == IBIS_MODEL_TYPE::UNDEFINED)
    {
        if( status )
            Report( message.str(), RPT_SEVERITY_ACTION );

        Report( _( "Undefined model type." ), RPT_SEVERITY_ERROR );
        status = false;
    }

    if( std::isnan( m_vinh ) && !isNumberNA( m_vinh ) )
    {
        if( status )
            Report( message.str(), RPT_SEVERITY_ACTION );

        Report( _( "Invalid Vinh value." ), RPT_SEVERITY_ERROR );
        status = false;
    }

    if( std::isnan( m_vinl ) && !isNumberNA( m_vinl ) )
    {
        if( status )
            Report( message.str(), RPT_SEVERITY_ACTION );

        Report( _( "Invalid Vinl value." ), RPT_SEVERITY_ERROR );
        status = false;
    }

    if( std::isnan( m_rref ) && !isNumberNA( m_rref ) )
    {
        if( status )
            Report( message.str(), RPT_SEVERITY_ACTION );

        Report( _( "Invalid R_ref value." ), RPT_SEVERITY_ERROR );
        status = false;
    }

    if( std::isnan( m_cref ) && !isNumberNA( m_cref ) )
    {
        if( status )
            Report( message.str(), RPT_SEVERITY_ACTION );

        Report( _( "Invalid C_ref value." ), RPT_SEVERITY_ERROR );
        status = false;
    }

    if( std::isnan( m_vref ) && !isNumberNA( m_vref ) )
    {
        if( status )
            Report( message.str(), RPT_SEVERITY_ACTION );

        Report( _( "Invalid V_ref value." ), RPT_SEVERITY_ERROR );
        status = false;
    }

    if( std::isnan( m_vmeas ) && !isNumberNA( m_vmeas ) )
    {
        if( status )
            Report( message.str(), RPT_SEVERITY_ACTION );

        Report( _( "Invalid V_meas value." ), RPT_SEVERITY_ERROR );
        status = false;
    }

    if( !m_C_comp.Check() )
    {
        if( status )
            Report( message.str(), RPT_SEVERITY_ACTION );

        Report( _( "Missing or invalid C_comp value." ), RPT_SEVERITY_ERROR );
        status = false;
    }

    if( !m_C_comp_gnd_clamp.Check() )
    {
        if( status )
            Report( message.str(), RPT_SEVERITY_ACTION );

        Report( _( "Missing or invalid C_comp_gnd_clamp value." ), RPT_SEVERITY_ERROR );
        status = false;
    }

    if( !m_C_comp_power_clamp.Check() )
    {
        if( status )
            Report( message.str(), RPT_SEVERITY_ACTION );

        Report( _( "Missing or invalid C_comp_power_clamp value." ), RPT_SEVERITY_ERROR );
        status = false;
    }

    if( !m_C_comp_pullup.Check() )
    {
        if( status )
            Report( message.str(), RPT_SEVERITY_ACTION );

        Report( _( "Missing or invalid C_comp_pullup value." ), RPT_SEVERITY_ERROR );
        status = false;
    }

    if( !m_C_comp_pulldown.Check() )
    {
        if( status )
            Report( message.str(), RPT_SEVERITY_ACTION );

        Report( _( "Missing or invalid C_comp_pulldown value." ), RPT_SEVERITY_ERROR );
        status = false;
    }

    if( m_C_comp.isNA() && m_C_comp_gnd_clamp.isNA() && m_C_comp_power_clamp.isNA()
        && m_C_comp_pullup.isNA() && m_C_comp_pulldown.isNA() )
    {
        if( status )
            Report( message.str(), RPT_SEVERITY_ACTION );

        Report( _( "No C_comp values are available." ), RPT_SEVERITY_ERROR );
        status = false;
    }

    if( !m_temperatureRange.Check() || m_temperatureRange.isNA() )
    {
        if( status )
            Report( message.str(), RPT_SEVERITY_ACTION );

        Report( _( "Missing or invalid Temperature Range value." ), RPT_SEVERITY_ERROR );
        status = false;
    }

    if( !m_voltageRange.Check() )
    {
        if( status )
            Report( message.str(), RPT_SEVERITY_ACTION );

        Report( _( "Missing or invalid Voltage Range value." ), RPT_SEVERITY_ERROR );
        status = false;
    }

    if( !m_pullupReference.Check() )
    {
        if( status )
            Report( message.str(), RPT_SEVERITY_ACTION );

        Report( _( "Missing or invalid Pullup Reference value." ), RPT_SEVERITY_ERROR );
        status = false;
    }

    if( !m_pulldownReference.Check() )
    {
        if( status )
            Report( message.str(), RPT_SEVERITY_ACTION );

        Report( _( "Missing or invalid Pulldown Reference value." ), RPT_SEVERITY_ERROR );
        status = false;
    }

    if( !m_GNDClampReference.Check() )
    {
        if( status )
            Report( message.str(), RPT_SEVERITY_ACTION );

        Report( _( "Missing or invalid GND Clamp Reference value." ), RPT_SEVERITY_ERROR );
        status = false;
    }

    if( !m_POWERClampReference.Check() )
    {
        if( status )
            Report( message.str(), RPT_SEVERITY_ACTION );

        Report( _( "Missing or invalid POWER Clamp Reference value." ), RPT_SEVERITY_ERROR );
        status = false;
    }

    if ( m_voltageRange.isNA() && ( m_pullupReference.isNA()   || m_pulldownReference.isNA()
                                 || m_GNDClampReference.isNA() || m_POWERClampReference.isNA() ) )
    {
        if( status )
            Report( message.str(), RPT_SEVERITY_ACTION );

        Report( _( "Voltage Range or Reference is missing." ), RPT_SEVERITY_ERROR );
        status = false;
    }

    if( !m_pulldown.Check() )
    {
        Report( _( "Invalid Pulldown table." ), RPT_SEVERITY_ERROR );
        status = false;
    }

    if( !m_pullup.Check() )
    {
        Report( _( "Invalid Pullup table." ), RPT_SEVERITY_ERROR );
        status = false;
    }

    if( !m_ISSO_PD.Check() )
    {
        Report( _( "Invalid ISSO_PD table." ), RPT_SEVERITY_ERROR );
        status = false;
    }

    if( !m_ISSO_PU.Check() )
    {
        Report( _( "Invalid ISSO_PU table." ), RPT_SEVERITY_ERROR );
        status = false;
    }

    if( !m_compositeCurrent.Check() )
    {
        Report( _( "Invalid Composite Current table." ), RPT_SEVERITY_ERROR );
        status = false;
    }

    if( !m_POWERClamp.Check() )
    {
        Report( _( "Invalid POWER Clamp table." ), RPT_SEVERITY_ERROR );
        status = false;
    }

    if( !m_GNDClamp.Check() )
    {
        Report( _( "Invalid GND Clamp table." ), RPT_SEVERITY_ERROR );
        status = false;
    }

    if( m_type != IBIS_MODEL_TYPE::INPUT_ECL && m_type != IBIS_MODEL_TYPE::INPUT_STD
        && m_type != IBIS_MODEL_TYPE::TERMINATOR && m_type != IBIS_MODEL_TYPE::SERIES
        && m_type != IBIS_MODEL_TYPE::SERIES_SWITCH )
    {
        if( !m_ramp.Check() )
            Report( _( "Invalid Ramp specification." ), RPT_SEVERITY_ERROR );
    }

    return status;
}


bool IbisSubmodel::Check()
{
    bool status = true;

    if( m_name.empty() )
    {
        Report( _( "Submodel name cannot be empty." ), RPT_SEVERITY_ERROR );
        status = false;
    }

    std::stringstream message;
    message << _( "Checking submodel " ) << m_name;

    if( m_type == IBIS_SUBMODEL_TYPE::UNDEFINED )
    {
        if( status )
            Report( message.str(), RPT_SEVERITY_ACTION );

        Report( _( "The submodel has no type." ), RPT_SEVERITY_ERROR );
        status = false;
    }

    if( !m_VtriggerR.Check() )
    {
        if( status )
            Report( message.str(), RPT_SEVERITY_ACTION );

        Report( _( "Missing or invalid V_trigger_r value." ), RPT_SEVERITY_ERROR );
        status = false;
    }
    if( !m_VtriggerF.Check() )
    {
        if( status )
            Report( message.str(), RPT_SEVERITY_ACTION );

        Report( _( "Missing or invalid V_trigger_f value." ), RPT_SEVERITY_ERROR );
        status = false;
    }
    if( !m_offDelay.Check() )
    {
        if( status )
            Report( message.str(), RPT_SEVERITY_ACTION );

        Report( _( "Missing or invalid Off_delay value." ), RPT_SEVERITY_ERROR );
        status = false;
    }
    if( !m_pullup.Check() )
    {
        Report( _( "Invalid Pullup table." ), RPT_SEVERITY_ERROR );
        status = false;
    }
    if( !m_pulldown.Check() )
    {
        Report( _( "Invalid Pulldown table." ), RPT_SEVERITY_ERROR );
        status = false;
    }
    if( !m_GNDClamp.Check() )
    {
        Report( _( "Invalid GND Clamp table." ), RPT_SEVERITY_ERROR );
        status = false;
    }
    if( !m_POWERClamp.Check() )
    {
        Report( _( "Invalid POWER Clamp table." ), RPT_SEVERITY_ERROR );
        status = false;
    }
    if( !m_GNDPulse.Check() )
    {
        Report( _( "Invalid GND Pulse table." ), RPT_SEVERITY_ERROR );
        status = false;
    }
    if( !m_POWERPulse.Check() )
    {
        Report( _( "Invalid POWER Pulse table." ), RPT_SEVERITY_ERROR );
        status = false;
    }
    if( !m_ramp.Check() )
    {
        Report( _( "Invalid Ramp specification." ), RPT_SEVERITY_ERROR );
        status = false;
    }
    if( m_type != IBIS_SUBMODEL_TYPE::DYNAMIC_CLAMP )
    {
        std::stringstream err_msg;
        switch( m_type )
        {
        case IBIS_SUBMODEL_TYPE::BUS_HOLD: err_msg << "Bus Hold"; break;
        case IBIS_SUBMODEL_TYPE::FALL_BACK: err_msg << "Fall Back"; break;
        default: wxLogMessage( "Invalid submodel type: %i", (int) m_type ); return false;
        }
        err_msg << _( " submodels are not yet supported." );
        Report( err_msg.str(), RPT_SEVERITY_ERROR );
        status = false;
    }
    else if( !m_VtriggerR.isNA() || !m_VtriggerF.isNA() )
    {
        Report( _( "Triggered mode dynamic clamp submodels are not yet supported." ), RPT_SEVERITY_ERROR );
        status = false;
    }
    return status;
}


bool IbisHeader::Check()
{
    bool status = true;

    std::stringstream message;
    message << _( "Checking Header..." );

    if( m_ibisVersion == -1 )
    {
        if( status )
            Report( message.str(), RPT_SEVERITY_ACTION );

        Report( _( "Missing [IBIS Ver]" ), RPT_SEVERITY_ERROR );
        status = false;
    }

    if( m_ibisVersion > IBIS_MAX_VERSION )
    {
        if( status )
            Report( message.str(), RPT_SEVERITY_ACTION );

        Report( _( "The parser does not handle this IBIS version" ), RPT_SEVERITY_ERROR );
        status = false;
    }

    if( m_fileRevision == -1 )
    {
        if( status )
            Report( message.str(), RPT_SEVERITY_ACTION );

        Report( _( "Missing [File Rev]" ), RPT_SEVERITY_ERROR );
        status = false;
    }

    if( m_fileName.empty() )
    {
        if( status )
            Report( message.str(), RPT_SEVERITY_ACTION );

        Report( _( "Missing [File Name]" ), RPT_SEVERITY_ERROR );
        status = false;
    }

    std::string ext = m_fileName.substr( m_fileName.length() - 4 );

    if( !( !strcmp( ext.c_str(), ".ibs" ) || !strcmp( ext.c_str(), ".pkg" )
           || !strcmp( ext.c_str(), ".ebd" ) || !strcmp( ext.c_str(), ".ims" ) ) )
    {
        if( status )
            Report( message.str(), RPT_SEVERITY_ACTION );

        Report( "Invalid file extension in [File Name]", RPT_SEVERITY_ERROR );
        status = false;
    }

    return status;
}

bool IbisPackageModel::Check()
{
    bool status = true;

    if( m_name.empty() )
    {
        Report( _( "Package model name cannot be empty." ), RPT_SEVERITY_ERROR );
        status = false;
    }

    std::stringstream message;
    message << _( "Checking package model " ) << m_name;

    if( m_manufacturer.empty() )
    {
        if( status )
            Report( message.str(), RPT_SEVERITY_ACTION );

        Report( _( "Manufacturer cannot be empty." ), RPT_SEVERITY_ERROR );
        status = false;
    }

    if( m_OEM.empty() )
    {
        if( status )
            Report( message.str(), RPT_SEVERITY_ACTION );

        Report( _( "OEM cannot be empty." ), RPT_SEVERITY_ERROR );
        status = false;
    }

    if( m_numberOfPins < 0 )
    {
        if( status )
            Report( message.str(), RPT_SEVERITY_ACTION );

        Report( _( "Negative number of pins." ), RPT_SEVERITY_ERROR );
        status = false;
    }

    if( (int)m_pins.size() != m_numberOfPins )
    {
        if( status )
            Report( message.str(), RPT_SEVERITY_ACTION );

        Report( "Number of pins does not match [Pin Numbers] size.", RPT_SEVERITY_ERROR );
        status = false;
    }

    // resistance matrix is not required

    if( !( m_resistanceMatrix )->Check() )
    {
        if( status )
            Report( message.str(), RPT_SEVERITY_ACTION );

        Report( _( "Resistance matrix is incorrect." ), RPT_SEVERITY_ERROR );
        status = false;
    }

    if( m_capacitanceMatrix != nullptr )
    {
        if( m_capacitanceMatrix->m_type == IBIS_MATRIX_TYPE::UNDEFINED )
        {
            if( status )
                Report( message.str(), RPT_SEVERITY_ACTION );

            Report( _( "Capacitance matrix is undefined." ), RPT_SEVERITY_ERROR );
            status = false;
        }

        if( !m_capacitanceMatrix->Check() )
        {
            if( status )
                Report( message.str(), RPT_SEVERITY_ACTION );

            Report( _( "Capacitance matrix is incorrect." ), RPT_SEVERITY_ERROR );
            status = false;
        }
    }
    else
    {
        if( status )
            Report( message.str(), RPT_SEVERITY_ACTION );

        Report( _( "Capacitance matrix is nullptr." ), RPT_SEVERITY_ERROR );
        status = false;
    }

    if( m_inductanceMatrix != nullptr )
    {
        if( m_inductanceMatrix->m_type == IBIS_MATRIX_TYPE::UNDEFINED )
        {
            if( status )
                Report( message.str(), RPT_SEVERITY_ACTION );

            Report( _( "Inductance matrix is undefined." ), RPT_SEVERITY_ERROR );
            status = false;
        }

        if( !m_inductanceMatrix->Check() )
        {
            if( status )
                Report( message.str(), RPT_SEVERITY_ACTION );

            Report( _( "Inductance matrix is incorrect." ), RPT_SEVERITY_ERROR );
            status = false;
        }
    }
    else
    {
        if( status )
            Report( message.str(), RPT_SEVERITY_ACTION );

        Report( _( "Inductance matrix is nullptr." ), RPT_SEVERITY_ERROR );
        status = false;
    }
    return status;
}


bool IbisParser::ParseFile( const std::string& aFileName )
{
    std::stringstream err_msg;

    std::ifstream ibisFile;
    ibisFile.open( aFileName );

    if( !ibisFile.is_open() )
    {
        err_msg << _( "Cannot open file " ) << aFileName;
        Report( err_msg.str(), RPT_SEVERITY_ERROR );
        return false;
    }

    std::ostringstream ss;
    ss << ibisFile.rdbuf();
    const std::string& s = ss.str();
    m_buffer = std::vector<char>( s.begin(), s.end() );
    m_buffer.push_back( 0 );

    long size = m_buffer.size();

    m_lineCounter = 0;
    m_bufferIndex = 0;

    bool status = true;

    LOCALE_IO toggle;   // Temporary switch the locale to standard C to r/w floats

    while( ( m_bufferIndex < size ) && status )
    {
        if( !getNextLine() )
        {
            Report( _( "Unexpected end of file. Missing [END] ?" ), RPT_SEVERITY_ERROR );
            status = false;
        }

        if( status && m_parrot )
            printLine();

        if( status && !onNewLine() )
        {
            err_msg.clear();
            err_msg << _( "Error on line " ) << std::to_string( m_lineCounter );
            Report( err_msg.str(), RPT_SEVERITY_ERROR );
            status = false;
        }

        if( m_context == IBIS_PARSER_CONTEXT::END )
            break;
    }

    if( status )
    {
        for( const IbisModel& model : m_ibisFile.m_models )
        {
            for( const IbisSubmodelMode& submodel : model.m_submodels )
            {
                if( m_ibisFile.m_submodels.count( submodel.m_name ) == 0 )
                {
                    err_msg.clear();
                    err_msg << submodel.m_name << _( " submodel not found." );
                    Report( err_msg.str(), RPT_SEVERITY_ERROR );
                    status = false;
                    goto done;
                }
            }
        }
    }

done:
    m_buffer.clear();
    return status;
}

void IbisParser::skipWhitespaces()
{
    while( isspace( m_buffer[m_lineOffset + m_lineIndex] ) && m_lineIndex < m_lineLength )
        m_lineIndex++;
}


bool IbisParser::checkEndofLine()
{
    skipWhitespaces();

    if( m_lineIndex < m_lineLength )
    {
        Report( _( "A line did not end properly." ), RPT_SEVERITY_ERROR );
        return false;
    }

    return true;
}


bool IbisParser::isLineEmptyFromCursor()
{
    int cursor = m_lineIndex;

    while( isspace( m_buffer[m_lineOffset + cursor] ) && cursor < m_lineLength )
        cursor++;

    return ( cursor >= m_lineLength );
}


bool IbisParser::parseDvdt( dvdt& aDest, std::string& aString )
{
    bool status = true;

    if( aString == "NA" )
    {
        aDest.m_dv = nan( NAN_NA );
        aDest.m_dt = nan( NAN_NA );

        return status;
    }

    int i = 0;

    for( i = 1; i < (int)aString.length(); i++ )
    {
        if( aString.at( i ) == '/' )
            break;
    }

    if( aString.at( i ) == '/' )
    {
        std::string str1 = aString.substr( 0, i );
        std::string str2 = aString.substr( i + 1, aString.size() - i - 1 );

        if( !parseDouble( aDest.m_dv, str1, true ) || !parseDouble( aDest.m_dt, str2, true ) )
            status = false;
    }

    return status;
}


bool IbisParser::readDvdt( dvdt& aDest )
{
    bool        status = true;
    std::string str;

    if( readWord( str ) )
    {
        if( !parseDvdt( aDest, str ) )
        {
            std::stringstream message;
            message << str << _( " is not a valid dV/dt value." );
            Report( message.str(), RPT_SEVERITY_WARNING );
            status = false;
        }
    }
    else
    {
        Report( _( "A dV/dt value is missing." ), RPT_SEVERITY_WARNING );
        status = false;
    }

    return status;
}


bool IbisParser::parseDouble( double& aDest, std::string& aStr, bool aAllowModifiers )
{
    // "  an entry of the C matrix could be given as 1.23e-12 or as 1.23p or 1.23pF."
    // Kibis: This implementation will also allow 1.23e-3n

    skipWhitespaces();
    bool status = true;
    bool converted = false;

    std::string str = aStr;

    double result;
    size_t size = 0;


    if( str == "NA" )
    {
        result = nan( NAN_NA );
    }
    else
    {
        try
        {
            result = std::stod( str, &size );
            converted = true;
        }
        catch( ... )
        {
            result = nan( NAN_INVALID );
            status = false;
        }
    }

    if( converted && ( size < str.length() ) )
    {
        switch( static_cast<char>( str.at( size ) ) )
        {
        case 'T': result *= 1e12;  break;
        case 'G': result *= 1e9;   break;
        case 'M': result *= 1e6;   break;
        case 'k': result *= 1e3;   break;
        case 'm': result *= 1e-3;  break;
        case 'u': result *= 1e-6;  break;
        case 'n': result *= 1e-9;  break;
        case 'p': result *= 1e-12; break;
        case 'f': result *= 1e-15; break;
        default:
            break;
            // In some files, "vinh = 3.0V", therefore we can't return false in the default case
        }
    }

    aDest = result;

    return status;
}


bool IbisParser::getNextLine()
{
    m_lineCounter++;

    long tmpIndex = m_bufferIndex;

    m_lineOffset = m_bufferIndex;

    if( m_bufferIndex >= (int)m_buffer.size() )
        return false;

    char c = m_buffer[m_bufferIndex++];

    int i = 1;

    while( c != m_commentChar && c != 0 && c != '\n' && i < IBIS_MAX_LINE_LENGTH )
    {
        c = m_buffer[m_bufferIndex++];
        i++;
    }

    if( i == IBIS_MAX_LINE_LENGTH )
    {
        Report( _( "Line exceeds maximum length." ), RPT_SEVERITY_ERROR );
        return false;
    }

    m_lineLength = m_bufferIndex - tmpIndex - 1; // Don't add the end of line condition
    m_lineIndex = 0;

    if( c == m_commentChar )
    {
        while( c != 0 && c != '\n' )
            c = m_buffer[m_bufferIndex++];
    }

    if( i == IBIS_MAX_LINE_LENGTH )
    {
        Report( _( "Line exceeds maximum length." ), RPT_SEVERITY_ERROR );
        return false;
    }

    return true;
}


void IbisParser::printLine()
{
    for( int i = 0; i < m_lineLength; i++ )
        std::cout << m_buffer[m_lineOffset + i];

    std::cout << std::endl;
}

bool IbisParser::readDouble( double& aDest )
{
    bool        status = true;
    std::string str;

    if( readWord( str ) )
    {
        if( !parseDouble( aDest, str, true ) )
        {
            Report( _( "Failed to read a double." ), RPT_SEVERITY_WARNING );
            status = false;
        }
    }
    else
    {
        Report( _( "Failed to read a word." ), RPT_SEVERITY_WARNING );
        status = false;
    }

    return status;
}

bool IbisParser::readInt( int& aDest )
{
    bool        status = true;
    std::string str;

    if( readWord( str ) )
    {
        double result;
        size_t size = 0;

        try
        {
            result = std::stoi( str, &size );
        }
        catch( ... )
        {
            if( str == "NA" )
            {
                result = nan( NAN_NA );
            }
            else
            {
                result = nan( NAN_INVALID );
                status = false;
            }
        }

        if( size != str.size() )
        {
            status = false;
            Report( _( "Number is not an integer" ), RPT_SEVERITY_WARNING );
        }

        aDest = result;
    }
    else
    {
        Report( _( "Failed to read a word." ), RPT_SEVERITY_WARNING );
        status = false;
    }

    return status;
}

bool IbisParser::readWord( std::string& aDest )
{
    skipWhitespaces();

    int startIndex = m_lineIndex;

    while( !isspace( m_buffer[m_lineOffset + m_lineIndex] ) && m_lineIndex < m_lineLength )
        m_lineIndex++;

    std::vector<char>::iterator start = std::next( m_buffer.begin(), m_lineOffset + startIndex );
    std::vector<char>::iterator end = std::next( m_buffer.begin(), m_lineOffset + m_lineIndex );
    aDest = std::string( start, end );

    return ( aDest.size() > 0 );
}

bool IbisParser::readString( std::string& aDest )
{
    while( m_lineIndex < m_lineLength )
        aDest += m_buffer[m_lineOffset + m_lineIndex++];

    // Remove extra whitespace characters
    int  len = aDest.length();

    if( len < 1 )
        return true;        // The data is likely on the next line. Skip.

    char c = aDest[len - 1];
    int i = 0;

    while( isspace( c ) && ( i < len ) )
    {
        c = aDest[len - 1 - i];
        i++;
    }

    aDest = aDest.substr( 0, len - i + 1 );

    return true;
}

bool IbisParser::storeString( std::string& aDest, bool aMultiline )
{
    bool status = true;

    skipWhitespaces();

    status &= readString( aDest );

    m_continue = aMultiline ? IBIS_PARSER_CONTINUE::STRING : IBIS_PARSER_CONTINUE::NONE;
    m_continuingString = &aDest;

    status &= checkEndofLine();
    return status;
}


bool IbisParser::changeCommentChar()
{
    skipWhitespaces();

    std::string strChar = "";

    // We cannot stop at m_lineLength here, because lineLength could stop before |_char
    // if the char remains the same

    char c = m_buffer[m_lineOffset + m_lineIndex++];
    char d = c;

    if( !( c == '!' || c == '"' || c == '#' || c == '$' || c == '%' || c == '&' || c == '\''
           || c == '(' || c == ')' || c == '*' || c == ',' || c == ':' || c == ';' || c == '<'
           || c == '>' || c == '?' || c == '@' || c == '\\' || c == '^' || c == '`' || c == '{'
           || c == '|' || c == '}' || c == '~' || c == ')' ) )
    {
        Report( _( "New comment character is invalid." ), RPT_SEVERITY_ERROR );
    }

    c = m_buffer[m_lineOffset + m_lineIndex++];

    while( ( !isspace( c ) ) && c != 0 && c != '\n' )
    {
        strChar += c;
        c = m_buffer[m_lineOffset + m_lineIndex++];
    }

    if( strChar != "_char" )
    {
        Report( _( "Invalid syntax. Should be |_char or &_char, etc..." ), RPT_SEVERITY_ERROR );
        return false;
    }

    while( isspace( c ) && c != 0 && c != '\n' && c != d )
        c = m_buffer[m_lineOffset + m_lineIndex++];

    if( ( !isspace( c ) ) && c != d )
    {
        Report( _( "No extra argument was expected" ), RPT_SEVERITY_ERROR );
        return false;
    }

    m_commentChar = d;

    m_continue = IBIS_PARSER_CONTINUE::NONE;

    return true;
}

std::string IbisParser::getKeyword()
{
    std::string keyword = "";
    //"Keywords must be enclosed in square brackets, “[]”, and must start in column 1 of the line."
    //"No space or tab is allowed immediately after the opening bracket “[” or immediately"
    // "before the closing bracket “]"

    if( m_buffer[m_lineOffset + m_lineIndex] != '[' )
    {
        // We return an empty keyword, this should stop the parser.
        return "";
    }

    m_lineIndex++;

    char c;
    c = m_buffer[m_lineOffset + m_lineIndex++];

    while( ( c != ']' )
           && ( m_lineIndex
                < m_lineLength ) ) // We know the maximum keyword length, we could add a condition.
    {
        // "Underscores and spaces are equivalent in keywords"
        if( c == ' ' )
            c = '_';

        keyword += c;
        c = m_buffer[m_lineOffset + m_lineIndex++];
    }

    return keyword;
}

bool IbisParser::changeContext( std::string& aKeyword )
{
    bool status = true;

    if( status )
    {
        switch( m_context )
        {
        case IBIS_PARSER_CONTEXT::HEADER: status &= m_ibisFile.m_header.Check(); break;
        case IBIS_PARSER_CONTEXT::COMPONENT: status &= m_currentComponent->Check(); break;
        case IBIS_PARSER_CONTEXT::MODEL: status &= m_currentModel->Check(); break;
        case IBIS_PARSER_CONTEXT::SUBMODEL: status &= m_currentSubmodel->Check(); break;
        case IBIS_PARSER_CONTEXT::MODELSELECTOR: status &= m_currentModelSelector->Check(); break;
        case IBIS_PARSER_CONTEXT::PACKAGEMODEL: status &= m_currentPackageModel->Check(); break;
        case IBIS_PARSER_CONTEXT::END:
            Report( "Cannot change context after [END]" );
            status = false;
            break;
        default:
            Report( "Changing context from an undefined context" );
        }
    }

    if( !compareIbisWord( aKeyword.c_str(), "End" ) && status )
    {
        //New context
        if( compareIbisWord( aKeyword.c_str(), "Component" ) )
        {
            m_ibisFile.m_components.push_back( IbisComponent( m_Reporter ) );
            m_currentComponent = &( m_ibisFile.m_components.back() );
            status &= storeString( m_currentComponent->m_name, false );
            m_context = IBIS_PARSER_CONTEXT::COMPONENT;
        }
        else if( compareIbisWord( aKeyword.c_str(), "Model_Selector" ) )
        {
            IbisModelSelector MS( m_Reporter );
            status &= storeString( MS.m_name, false );
            m_ibisFile.m_modelSelectors.push_back( MS );
            m_currentModelSelector = &( m_ibisFile.m_modelSelectors.back() );
            m_context = IBIS_PARSER_CONTEXT::MODELSELECTOR;
            m_continue = IBIS_PARSER_CONTINUE::MODELSELECTOR;
        }
        else if( compareIbisWord( aKeyword.c_str(), "Model" ) )
        {
            IbisModel model( m_Reporter );
            model.m_temperatureRange.value[IBIS_CORNER::MIN] = 0;
            model.m_temperatureRange.value[IBIS_CORNER::TYP] = 50;
            model.m_temperatureRange.value[IBIS_CORNER::MAX] = 100;
            status &= storeString( model.m_name, false );
            m_ibisFile.m_models.push_back( model );
            m_currentModel = &( m_ibisFile.m_models.back() );
            m_context = IBIS_PARSER_CONTEXT::MODEL;
            m_continue = IBIS_PARSER_CONTINUE::MODEL;
        }
        else if( compareIbisWord( aKeyword.c_str(), "Submodel" ) )
        {
            IbisSubmodel model( m_Reporter );
            status &= storeString( model.m_name, false );
            auto [it, success] = m_ibisFile.m_submodels.emplace( model.m_name, model );

            if( !success )
            {
                std::stringstream message;
                message << _( "Submodel name is not unique: " ) << model.m_name;
                Report( message.str(), RPT_SEVERITY_ERROR );
                status = false;
            }

            m_currentSubmodel = &( it->second );
            m_context = IBIS_PARSER_CONTEXT::SUBMODEL;
            m_continue = IBIS_PARSER_CONTINUE::SUBMODEL;
        }
        else if( compareIbisWord( aKeyword.c_str(), "Define_Package_Model" ) )
        {
            IbisPackageModel PM( m_Reporter );
            PM.m_resistanceMatrix = std::unique_ptr<IBIS_MATRIX>( new IBIS_MATRIX( m_Reporter ) );
            PM.m_capacitanceMatrix = std::unique_ptr<IBIS_MATRIX>( new IBIS_MATRIX( m_Reporter ) );
            PM.m_inductanceMatrix = std::unique_ptr<IBIS_MATRIX>( new IBIS_MATRIX( m_Reporter ) );

            status &= storeString( PM.m_name, false );

            m_ibisFile.m_packageModels.push_back( PM );
            m_currentPackageModel = &( m_ibisFile.m_packageModels.back() );
            m_context = IBIS_PARSER_CONTEXT::PACKAGEMODEL;
        }
        else if( compareIbisWord( aKeyword.c_str(), "End_Package_Model" ) )
        {
            if( m_currentComponent != nullptr )
            {
                m_context = IBIS_PARSER_CONTEXT::COMPONENT;
                m_continue = IBIS_PARSER_CONTINUE::NONE;
            }
            else // .pkg file, we just go back to header, to get the [END] keyword
            {    // This will cause the header to be checked twice.
                m_context = IBIS_PARSER_CONTEXT::HEADER;
                m_continue = IBIS_PARSER_CONTINUE::NONE;
            }
        }
        else
        {
            status = false;
            std::string context_string;

            switch( m_context )
            {
            case IBIS_PARSER_CONTEXT::HEADER:                 context_string += "HEADER";                   break;
            case IBIS_PARSER_CONTEXT::COMPONENT:              context_string += "COMPONENT";                break;
            case IBIS_PARSER_CONTEXT::MODELSELECTOR:          context_string += "MODEL_SELECTOR";           break;
            case IBIS_PARSER_CONTEXT::MODEL:                  context_string += "MODEL";                    break;
            case IBIS_PARSER_CONTEXT::PACKAGEMODEL:           context_string += "PACKAGE_MODEL";            break;
            case IBIS_PARSER_CONTEXT::PACKAGEMODEL_MODELDATA: context_string += "PACKAGE_MODEL_MODEL_DATA"; break;
            default:                                          context_string += "???";                      break;
            }

            std::stringstream message;
            message << _( "Unknown keyword in " ) << context_string << _( " context: " ) << aKeyword;
            Report( message.str(), RPT_SEVERITY_ERROR );
        }
    }
    else
    {
        m_context = IBIS_PARSER_CONTEXT::END;
    }

    return status;
}


bool IbisParser::parseModelSelector( std::string& aKeyword )
{
    bool status = true;

    if( !changeContext( aKeyword ) )
        status = false;

    return status;
}


bool IbisParser::readRampdvdt( dvdtTypMinMax& aDest )
{
    bool status = true;

    status &= readDvdt( aDest.value[IBIS_CORNER::TYP] );
    status &= readDvdt( aDest.value[IBIS_CORNER::MIN] );
    status &= readDvdt( aDest.value[IBIS_CORNER::MAX] );

    return status;
}

bool IbisParser::readRamp()
{
    bool status = true;
    m_continue = IBIS_PARSER_CONTINUE::RAMP;

    IbisRamp* ramp = nullptr;
    switch( m_context )
    {
    case IBIS_PARSER_CONTEXT::MODEL:
        ramp = &m_currentModel->m_ramp;
        break;
    case IBIS_PARSER_CONTEXT::SUBMODEL:
        ramp = &m_currentSubmodel->m_ramp;
        break;
    default:
        wxLogMessage( "Invalid context for ramp: %i.", (int) m_context );
        return false;
    }

    if( !readNumericSubparam( std::string( "R_load" ), ramp->m_Rload ) )
    {
        std::string str;

        if( readWord( str ) )
        {
            if( !strcmp( str.c_str(), "dV/dt_r" ) )
            {
                status &= readRampdvdt( ramp->m_rising );
            }
            else if( !strcmp( str.c_str(), "dV/dt_f" ) )
            {
                status &= readRampdvdt( ramp->m_falling );
            }
            else
            {
                Report( _( "Invalid ramp data" ), RPT_SEVERITY_ERROR );
                status = false;
            }
        }
    }

    return status;
}


bool IbisParser::readModelSpec()
{
    bool status = true;

    m_continue = IBIS_PARSER_CONTINUE::MODEL_SPEC;

    // TODO
    // readTypMinMaxValueSubparam( std::string( "Vmeas" ), m_currentModel->... )
    // ...

    return status;
}


bool IbisParser::readReceiverThresholds()
{
    bool status = true;

    m_continue = IBIS_PARSER_CONTINUE::RX_THRESHOLDS;

    // TODO
    // readTypMinMaxValueSubparam( std::string( "Vth" ), m_currentModel->... )
    // ...

    return status;
}


bool IbisParser::readAlgorithmicModel()
{
    bool status = true;

    // TODO: ???

    return status;
}


bool IbisParser::parseAlgorithmicModel( std::string& aKeyword )
{
    bool status = true;

    if( compareIbisWord( aKeyword.c_str(), "End_Algorithmic_Model" ) )
    {
        m_context = IBIS_PARSER_CONTEXT::MODEL;
        m_continue = IBIS_PARSER_CONTINUE::NONE;
    }
    else
    {
        status = changeContext( aKeyword );
    }
    return status;
}


bool IbisParser::readAddSubmodel()
{
    bool status = true;

    m_continue = IBIS_PARSER_CONTINUE::ADD_SUBMODEL;

    skipWhitespaces();

    if( m_lineIndex < m_lineLength )
    {
        std::string name;
        if( !readWord( name ) )
        {
            Report( _( "The submodel name is missing." ) );
            return false;
        }

        std::string str;
        if( !readWord( str ) )
        {
            Report( _( "The submodel mode is missing." ) );
            return false;
        }

        IBIS_SUBMODEL_MODE mode = IBIS_SUBMODEL_MODE::ALL;
        if( !strcmp( str.c_str(), "All" ) )
        {
            mode = IBIS_SUBMODEL_MODE::ALL;
        }
        else if( !strcmp( str.c_str(), "Driving" ) )
        {
            mode = IBIS_SUBMODEL_MODE::DRIVING;
        }
        else if( !strcmp( str.c_str(), "Non-Driving" ) )
        {
            mode = IBIS_SUBMODEL_MODE::NON_DRIVING;
        }
        else
        {
            std::stringstream message;
            message << str << _( " is not a recognised submodel mode." );
            Report( message.str(), RPT_SEVERITY_WARNING );
            return false;
        }

        m_currentModel->m_submodels.push_back( IbisSubmodelMode( name, mode ) );
    }

    return status;
}


bool IbisParser::parseModel( std::string& aKeyword )
{
    bool status = false;

    if( compareIbisWord( aKeyword.c_str(), "Voltage_Range" ) )
        status = readTypMinMaxValue( m_currentModel->m_voltageRange );
    else if( compareIbisWord( aKeyword.c_str(), "Temperature_Range" ) )
        status = readTypMinMaxValue( m_currentModel->m_temperatureRange );
    else if( compareIbisWord( aKeyword.c_str(), "GND_Clamp" ) )
        status = readIVtableEntry( m_currentModel->m_GNDClamp );
    else if( compareIbisWord( aKeyword.c_str(), "POWER_Clamp" ) )
        status = readIVtableEntry( m_currentModel->m_POWERClamp );
    else if( compareIbisWord( aKeyword.c_str(), "Pulldown" ) )
        status = readIVtableEntry( m_currentModel->m_pulldown );
    else if( compareIbisWord( aKeyword.c_str(), "Pullup" ) )
        status = readIVtableEntry( m_currentModel->m_pullup );
    else if( compareIbisWord( aKeyword.c_str(), "ISSO_PD" ) )
        status = readIVtableEntry( m_currentModel->m_ISSO_PD );
    else if( compareIbisWord( aKeyword.c_str(), "ISSO_PU" ) )
        status = readIVtableEntry( m_currentModel->m_ISSO_PU );
    else if( compareIbisWord( aKeyword.c_str(), "Composite_Current" ) )
        status = readIVtableEntry( m_currentModel->m_compositeCurrent );
    else if( compareIbisWord( aKeyword.c_str(), "Rising_Waveform" ) )
        status = readWaveform( nullptr, IBIS_WAVEFORM_TYPE::RISING );
    else if( compareIbisWord( aKeyword.c_str(), "Falling_Waveform" ) )
        status = readWaveform( nullptr, IBIS_WAVEFORM_TYPE::FALLING );
    else if( compareIbisWord( aKeyword.c_str(), "Ramp" ) )
        status = readRamp();
    else if( compareIbisWord( aKeyword.c_str(), "Model_Spec" ) )
        status = readModelSpec();
    else if( compareIbisWord( aKeyword.c_str(), "Receiver_Thresholds" ) )
        status = readReceiverThresholds();
    else if( compareIbisWord( aKeyword.c_str(), "Pullup_Reference" ) )
        status = readTypMinMaxValue( m_currentModel->m_pullupReference );
    else if( compareIbisWord( aKeyword.c_str(), "Pulldown_Reference" ) )
        status = readTypMinMaxValue( m_currentModel->m_pulldownReference );
    else if( compareIbisWord( aKeyword.c_str(), "POWER_Clamp_Reference" ) )
        status = readTypMinMaxValue( m_currentModel->m_POWERClampReference );
    else if( compareIbisWord( aKeyword.c_str(), "GND_Clamp_Reference" ) )
        status = readTypMinMaxValue( m_currentModel->m_GNDClampReference );
    else if( compareIbisWord( aKeyword.c_str(), "Rac" ) )
        status = readTypMinMaxValue( m_currentModel->m_Rac );
    else if( compareIbisWord( aKeyword.c_str(), "Cac" ) )
        status = readTypMinMaxValue( m_currentModel->m_Cac );
    else if( compareIbisWord( aKeyword.c_str(), "Rpower" ) )
        status = readTypMinMaxValue( m_currentModel->m_Rpower );
    else if( compareIbisWord( aKeyword.c_str(), "Rgnd" ) )
        status = readTypMinMaxValue( m_currentModel->m_Rgnd );
    else if( compareIbisWord( aKeyword.c_str(), "Algorithmic_Model" ) )
    {
        m_context = IBIS_PARSER_CONTEXT::ALGORITHMIC_MODEL;
        m_continue = IBIS_PARSER_CONTINUE::ALGORITHMIC_MODEL;
        status = true;
    }
    else if( compareIbisWord( aKeyword.c_str(), "Add_Submodel" ) )
        status = readAddSubmodel();
    else
    {
        status = changeContext( aKeyword );
    }
    return status;
}


bool IbisParser::readSubmodelSpec()
{
    bool status = true;

    m_continue = IBIS_PARSER_CONTINUE::SUBMODEL_SPEC;

    skipWhitespaces();

    std::string subparam;
    if( readWord( subparam ) )
    {
        if( !strcmp( subparam.c_str(), "Off_delay" ) )
            status = readTypMinMaxValue( m_currentSubmodel->m_offDelay );
        else if( !strcmp( subparam.c_str(), "V_trigger_r" ) )
            status = readTypMinMaxValue( m_currentSubmodel->m_VtriggerR );
        else if( !strcmp( subparam.c_str(), "V_trigger_f" ) )
            status = readTypMinMaxValue( m_currentSubmodel->m_VtriggerF );
        else
        {
            std::stringstream message;
            message << subparam << _( " is not a recognised Submodel Spec subparameter." );
            Report( message.str(), RPT_SEVERITY_ERROR );
            status = false;
        }
    }

    return status;
}


bool IbisParser::parseSubmodel( std::string& aKeyword )
{
    bool status = false;

    if( compareIbisWord( aKeyword.c_str(), "Submodel_Spec" ) )
        status = readSubmodelSpec();
    else if( compareIbisWord( aKeyword.c_str(), "Pullup" ) )
        status = readIVtableEntry( m_currentSubmodel->m_pullup );
    else if( compareIbisWord( aKeyword.c_str(), "Pulldown" ) )
        status = readIVtableEntry( m_currentSubmodel->m_pulldown );
    else if( compareIbisWord( aKeyword.c_str(), "GND_Clamp" ) )
        status = readIVtableEntry( m_currentSubmodel->m_GNDClamp );
    else if( compareIbisWord( aKeyword.c_str(), "POWER_Clamp" ) )
        status = readIVtableEntry( m_currentSubmodel->m_POWERClamp );
    else if( compareIbisWord( aKeyword.c_str(), "GND_Pulse" ) )
        status = readIVtableEntry( m_currentSubmodel->m_GNDPulse );
    else if( compareIbisWord( aKeyword.c_str(), "POWER_Pulse" ) )
        status = readIVtableEntry( m_currentSubmodel->m_POWERPulse );
    else if( compareIbisWord( aKeyword.c_str(), "Rising_Waveform" ) )
        status = readWaveform( nullptr, IBIS_WAVEFORM_TYPE::RISING );
    else if( compareIbisWord( aKeyword.c_str(), "Falling_Waveform" ) )
        status = readWaveform( nullptr, IBIS_WAVEFORM_TYPE::FALLING );
    else if( compareIbisWord( aKeyword.c_str(), "Ramp" ) )
        status = readRamp();
    else
        status = changeContext( aKeyword );

    return status;
}


bool IbisParser::readPackageModelPins()
{
    m_continue = IBIS_PARSER_CONTINUE::PACKAGEMODEL_PINS;
    std::string str;

    if( readWord( str ) )
        m_currentPackageModel->m_pins[str] = m_currentPackageModel->m_pins.size();

    return true;
}

bool IbisParser::readMatrixPinIndex( int& aDest )
{
    aDest = 0;

    std::string str;
    if( !readWord( str ) )
    {
        Report( _( "Missing pin name/number for matrix row." ) );
        return false;
    }
    auto pin = m_currentPackageModel->m_pins.find( str );
    if( pin == m_currentPackageModel->m_pins.end() )
    {
        Report( _( "Invalid pin name/number in matrix row." ) );
        return false;
    }
    aDest = pin->second;
    return true;
}


bool IbisParser::readMatrixType( std::shared_ptr<IBIS_MATRIX>& aDest )
{
    m_currentMatrix = aDest;
    m_currentMatrix->m_type = IBIS_MATRIX_TYPE::UNDEFINED;
    m_currentMatrix->m_rows = -1;
    m_currentMatrix->m_cols = -1;
    m_currentMatrixRow = -1;
    m_currentMatrixCol = -1;
    m_continue = IBIS_PARSER_CONTINUE::NONE;

    std::string str;
    if( !readWord( str ) )
    {
        Report( _( "The matrix type is missing." ), RPT_SEVERITY_ERROR );
        return false;
    }
    if( compareIbisWord( str.c_str(), "Banded_matrix" ) )
    {
        m_currentMatrix->m_type = IBIS_MATRIX_TYPE::BANDED;
    }
    else if( compareIbisWord( str.c_str(), "Full_matrix" ) )
    {
        m_currentMatrix->m_type = IBIS_MATRIX_TYPE::FULL;
    }
    else if( compareIbisWord( str.c_str(), "Sparse_matrix" ) )
    {
        m_currentMatrix->m_type = IBIS_MATRIX_TYPE::SPARSE;
    }
    else
    {
        std::stringstream message;
        message << str << _( " is not a recognised matrix type." );
        Report( str, RPT_SEVERITY_WARNING );
        return false;
    }

    if( m_currentMatrix->m_type != IBIS_MATRIX_TYPE::BANDED )
    {
        int numPins = m_currentPackageModel->m_numberOfPins;
        m_currentMatrix->m_rows = numPins;
        m_currentMatrix->m_cols = numPins;
        m_currentMatrix->m_data.resize( numPins * numPins );
    }
    return true;
}


bool IbisParser::readMatrixBandwidth()
{
    if( ( m_currentMatrix == nullptr )
        || ( m_currentMatrix->m_type != IBIS_MATRIX_TYPE::BANDED )
        || ( m_currentMatrix->m_cols > 0 ) )
    {
        Report( _( "Unexpected [Bandwidth] keyword." ), RPT_SEVERITY_ERROR );
        return false;
    }

    int  bandwidth;
    bool status = readInt( bandwidth );
    if( status )
    {
        if( bandwidth < 0 )
        {
            Report( _( "Matrix bandwidth must be >= 0." ), RPT_SEVERITY_ERROR );
            return false;
        }
        int numPins = m_currentPackageModel->m_numberOfPins;
        m_currentMatrix->m_rows = numPins;
        m_currentMatrix->m_cols = 1 + bandwidth;
        m_currentMatrix->m_data.resize( numPins * ( 1 + bandwidth ) );
    }
    return status;
}

bool IbisParser::readMatrixRow()
{
    if( ( m_currentMatrix == nullptr ) || ( m_currentMatrix->m_rows < 0 ) )
    {
        Report( _( "Unexpected [Row] keyword." ), RPT_SEVERITY_ERROR );
        return false;
    }
    int  index;
    bool status = readMatrixPinIndex( index );
    if( status )
    {
        m_currentMatrixRow = index;
        if( m_currentMatrix->m_type == IBIS_MATRIX_TYPE::FULL )
        {
            m_currentMatrixCol = index;
        }
        else
        {
            m_currentMatrixCol = 0;
        }
        m_continue = IBIS_PARSER_CONTINUE::MATRIX;
    }
    return status;
}

bool IbisParser::readMatrixBandedOrFull()
{
    std::vector<std::string> values;
    bool                     status = readTableLine( values );

    for( size_t i = 0; i < values.size(); i++ )
    {
        if( m_currentMatrixCol >= m_currentMatrix->m_cols )
        {
            Report( _( "Too much data for this matrix row." ), RPT_SEVERITY_ERROR );
            return false;
        }
        int index = m_currentMatrixRow * m_currentMatrix->m_cols + m_currentMatrixCol;
        if( ( index < 0 ) || ( index >= (int)m_currentMatrix->m_data.size() ) )
        {
            Report( _( "Element location is outside the matrix." ), RPT_SEVERITY_ERROR );
            return false;
        }
        if( !parseDouble( m_currentMatrix->m_data[index], values.at( i ), true ) )
        {
            Report( _( "Failed to read a matrix element." ), RPT_SEVERITY_ERROR );
            return false;
        }
        m_currentMatrixCol++;
    }
    return status;
}

bool IbisParser::readMatrixSparse()
{
    int index;
    if( !readMatrixPinIndex( index ) )
    {
        Report( _( "Failed to read a matrix pin name." ), RPT_SEVERITY_ERROR );
        return false;
    }
    double value;
    if( !readDouble( value ) )
    {
        Report( _( "Failed to read a matrix element." ), RPT_SEVERITY_ERROR );
        return false;
    }
    index += m_currentMatrixRow * m_currentMatrix->m_cols;
    if( ( index < 0 ) || (index >= (int)m_currentMatrix->m_data.size() ) )
    {
        Report( _( "Element location is outside the matrix." ), RPT_SEVERITY_ERROR );
        return false;
    }
    m_currentMatrix->m_data[index] = value;
    return true;
}

bool IbisParser::readMatrixData()
{
    int status = false;
    if( m_currentMatrix != nullptr )
    {
        switch( m_currentMatrix->m_type )
        {
        case IBIS_MATRIX_TYPE::BANDED:
        case IBIS_MATRIX_TYPE::FULL:
            status = readMatrixBandedOrFull();
            break;
        case IBIS_MATRIX_TYPE::SPARSE:
            status = readMatrixSparse();
            break;
        case IBIS_MATRIX_TYPE::UNDEFINED:
        default:
            wxLogMessage( "Invalid matrix type: %i", (int) m_currentMatrix->m_type );
            break;
        }
    }
    else
    {
        wxLogMessage( "Matrix pointer is null" );
    }
    return status;
}

bool IbisParser::parsePackageModelModelData( std::string& aKeyword )
{
    bool status = true;

    if( compareIbisWord( aKeyword.c_str(), "Resistance_Matrix" ) )
    {
        status = readMatrixType( m_currentPackageModel->m_resistanceMatrix );
    }
    else if( compareIbisWord( aKeyword.c_str(), "Capacitance_Matrix" ) )
    {
        status = readMatrixType( m_currentPackageModel->m_capacitanceMatrix );
    }
    else if( compareIbisWord( aKeyword.c_str(), "Inductance_Matrix" ) )
    {
        status = readMatrixType( m_currentPackageModel->m_inductanceMatrix );
    }
    else if( compareIbisWord( aKeyword.c_str(), "Bandwidth" ) )
    {
        status = readMatrixBandwidth();
    }
    else if( compareIbisWord( aKeyword.c_str(), "Row" ) )
    {
        status = readMatrixRow();
    }
    else if( compareIbisWord( aKeyword.c_str(), "End_Model_Data" ) )
    {
        m_context = IBIS_PARSER_CONTEXT::PACKAGEMODEL;
        m_continue = IBIS_PARSER_CONTINUE::NONE;
    }
    else if( !changeContext( aKeyword ) )
    {
        status = false;
    }

    return status;
}

bool IbisParser::parsePackageModel( std::string& aKeyword )
{
    bool status = true;

    if( compareIbisWord( aKeyword.c_str(), "Manufacturer" ) )
        status &= storeString( m_currentPackageModel->m_manufacturer, false );
    else if( compareIbisWord( aKeyword.c_str(), "OEM" ) )
        status &= storeString( m_currentPackageModel->m_OEM, false );
    else if( compareIbisWord( aKeyword.c_str(), "Description" ) )
        status &= storeString( m_currentPackageModel->m_description, false );
    else if( compareIbisWord( aKeyword.c_str(), "Number_of_Pins" ) )
        status &= readInt( m_currentPackageModel->m_numberOfPins );
    else if( compareIbisWord( aKeyword.c_str(), "Pin_Numbers" ) )
        status &= readPackageModelPins();
    else if( compareIbisWord( aKeyword.c_str(), "Model_Data" ) )
    {
        m_context = IBIS_PARSER_CONTEXT::PACKAGEMODEL_MODELDATA;
        m_continue = IBIS_PARSER_CONTINUE::NONE;
    }
    else if( !changeContext( aKeyword ) )
    {
        status = false;
    }

    return status;
}

bool IbisParser::readModelSelector()
{
    bool status = true;

    IbisModelSelectorEntry model;

    if( !readWord( model.m_modelName ) )
        return false;

    status &= readString( model.m_modelDescription );
    m_currentModelSelector->m_models.push_back( model );

    return status;
}

bool IbisParser::readNumericSubparam( const std::string& aSubparam, double& aDest )
{
    std::string paramName;
    bool        status = true;

    if( aSubparam.size() >= (size_t)m_lineLength )
    {
        // Continuing would result in an overflow
        return false;
    }

    int old_index = m_lineIndex;
    m_lineIndex = 0;

    for( size_t i = 0; i < aSubparam.size(); i++ )
        paramName += m_buffer[m_lineOffset + m_lineIndex++];

    if( strcmp( paramName.c_str(), aSubparam.c_str() ) )
    {
        m_lineIndex = old_index;
        return false;
    }

    skipWhitespaces();

    status &= m_buffer[m_lineOffset + m_lineIndex++] == '=';

    if( status )
    {
        skipWhitespaces();
        status &= readDouble( aDest );
    }

    if( !status )
        m_lineIndex = old_index;

    return status;
}


bool IbisParser::readTypMinMaxValue( TypMinMaxValue& aDest )
{
    bool status = true;

    skipWhitespaces();

    std::string strValue;

    if( !readDouble( aDest.value[IBIS_CORNER::TYP] ) )
    {
        Report( _( "Typ-Min-Max Values requires at least Typ." ), RPT_SEVERITY_ERROR );
        return false;
    }

    readDouble( aDest.value[IBIS_CORNER::MIN] );
    readDouble( aDest.value[IBIS_CORNER::MAX] );

    return status;
}

bool IbisParser::readTypMinMaxValueSubparam( const std::string& aSubparam, TypMinMaxValue& aDest )
{
    std::string paramName;
    bool        status = true;

    m_lineIndex = 0; // rewind

    if( aSubparam.size() < (size_t)m_lineLength )
    {
        for( size_t i = 0; i < aSubparam.size(); i++ )
            paramName += m_buffer[m_lineOffset + m_lineIndex++];

        if( ( m_buffer[m_lineOffset + m_lineIndex] == ' ' ) && !strcmp( paramName.c_str(), aSubparam.c_str() ) )
            readTypMinMaxValue( aDest );
        else
            status = false;
    }
    else
    {
        status = false;
    }

    return status;
}

bool IbisParser::readModel()
{
    bool        status = true;
    std::string subparam;

    if( readWord( subparam ) )
    {
        switch( m_continue )
        {
        case IBIS_PARSER_CONTINUE::MODEL:

            if( !strcmp( subparam.substr( 0, 10 ).c_str(), "Model_type" ) )
            {
                if( readWord( subparam ) )
                {
                    if( compareIbisWord( subparam.c_str(), "Input" ) )
                        m_currentModel->m_type = IBIS_MODEL_TYPE::INPUT_STD;
                    else if( compareIbisWord( subparam.c_str(), "Output" ) )
                        m_currentModel->m_type = IBIS_MODEL_TYPE::OUTPUT;
                    else if( compareIbisWord( subparam.c_str(), "I/O" ) )
                        m_currentModel->m_type = IBIS_MODEL_TYPE::IO;
                    else if( compareIbisWord( subparam.c_str(), "3-state" ) )
                        m_currentModel->m_type = IBIS_MODEL_TYPE::THREE_STATE;
                    else if( compareIbisWord( subparam.c_str(), "Open_drain" ) )
                        m_currentModel->m_type = IBIS_MODEL_TYPE::OPEN_DRAIN;
                    else if( compareIbisWord( subparam.c_str(), "I/O_Open_drain" ) )
                        m_currentModel->m_type = IBIS_MODEL_TYPE::IO_OPEN_DRAIN;
                    else if( compareIbisWord( subparam.c_str(), "Open_sink" ) )
                        m_currentModel->m_type = IBIS_MODEL_TYPE::OPEN_SINK;
                    else if( compareIbisWord( subparam.c_str(), "I/O_open_sink" ) )
                        m_currentModel->m_type = IBIS_MODEL_TYPE::IO_OPEN_SINK;
                    else if( compareIbisWord( subparam.c_str(), "Open_source" ) )
                        m_currentModel->m_type = IBIS_MODEL_TYPE::OPEN_SOURCE;
                    else if( compareIbisWord( subparam.c_str(), "I/O_open_source" ) )
                        m_currentModel->m_type = IBIS_MODEL_TYPE::IO_OPEN_SOURCE;
                    else if( compareIbisWord( subparam.c_str(), "Input_ECL" ) )
                        m_currentModel->m_type = IBIS_MODEL_TYPE::INPUT_ECL;
                    else if( compareIbisWord( subparam.c_str(), "Output_ECL" ) )
                        m_currentModel->m_type = IBIS_MODEL_TYPE::OUTPUT_ECL;
                    else if( compareIbisWord( subparam.c_str(), "I/O_ECL" ) )
                        m_currentModel->m_type = IBIS_MODEL_TYPE::IO_ECL;
                    else if( compareIbisWord( subparam.c_str(), "3-state_ECL" ) )
                        m_currentModel->m_type = IBIS_MODEL_TYPE::THREE_STATE_ECL;
                    else if( compareIbisWord( subparam.c_str(), "Terminator" ) )
                        m_currentModel->m_type = IBIS_MODEL_TYPE::TERMINATOR;
                    else if( compareIbisWord( subparam.c_str(), "Series" ) )
                        m_currentModel->m_type = IBIS_MODEL_TYPE::SERIES;
                    else if( compareIbisWord( subparam.c_str(), "Series_switch" ) )
                        m_currentModel->m_type = IBIS_MODEL_TYPE::SERIES_SWITCH;
                    else
                    {
                        std::stringstream message;
                        message << _( "Unknown Model_type " ) << subparam;
                        Report( message.str(), RPT_SEVERITY_ERROR );
                        status = false;
                    }
                }
                else
                {
                    Report( _( "Internal Error while reading model_type" ), RPT_SEVERITY_ERROR );
                    status = false;
                }
            }
            else if( !strcmp( subparam.substr( 0, 7 ).c_str(), "Enable" ) )
            {
                if( readWord( subparam ) )
                {
                    if( !strcmp( subparam.c_str(), "Active-High" ) )
                        m_currentModel->m_enable = IBIS_MODEL_ENABLE::ACTIVE_HIGH;
                    else if( !strcmp( subparam.c_str(), "Active-Low" ) )
                        m_currentModel->m_enable = IBIS_MODEL_ENABLE::ACTIVE_LOW;
                    else
                    {
                        std::stringstream message;
                        message << _( "Unknown Enable: " ) << subparam;
                        Report( message.str(), RPT_SEVERITY_ERROR );
                        status = false;
                    }
                }
                else
                {
                    Report( _( "Internal Error while reading Enable" ), RPT_SEVERITY_ERROR );
                    status = false;
                }
            }
            else if( subparam.substr( 0, 9 ) == "Polarity" )
            {
                if( readWord( subparam ) )
                {
                    if( subparam == "Inverting" )
                        m_currentModel->m_polarity = IBIS_MODEL_POLARITY::INVERTING;
                    else if( subparam == "Non-Inverting" )
                        m_currentModel->m_polarity = IBIS_MODEL_POLARITY::NON_INVERTING;
                    else
                    {
                        std::stringstream message;
                        message << _( "Unknown polarity " ) << subparam;
                        Report( message.str(), RPT_SEVERITY_ERROR );
                        status = false;
                    }
                }
                else
                {
                    Report( _( "Internal Error while reading Enable" ), RPT_SEVERITY_ERROR );
                    status = false;
                }
            }
            else if( readNumericSubparam( std::string( "Vinl" ), m_currentModel->m_vinl ) )
                ;
            else if( readNumericSubparam( std::string( "Vinh" ), m_currentModel->m_vinh ) )
                ;
            else if( readNumericSubparam( std::string( "Vref" ), m_currentModel->m_vref ) )
                ;
            else if( readNumericSubparam( std::string( "Rref" ), m_currentModel->m_rref ) )
                ;
            else if( readNumericSubparam( std::string( "Cref" ), m_currentModel->m_cref ) )
                ;
            else if( readNumericSubparam( std::string( "Vmeas" ), m_currentModel->m_vmeas ) )
                ;
            else if( readTypMinMaxValueSubparam( std::string( "C_comp" ), m_currentModel->m_C_comp ) )
                ;
            else if( readTypMinMaxValueSubparam( std::string( "C_comp_gnd_clamp" ),
                                                 m_currentModel->m_C_comp_gnd_clamp ) )
                ;
            else if( readTypMinMaxValueSubparam( std::string( "C_comp_power_clamp" ),
                                                 m_currentModel->m_C_comp_power_clamp ) )
                ;
            else if( readTypMinMaxValueSubparam( std::string( "C_comp_pullup" ),
                                                 m_currentModel->m_C_comp_pullup ) )
                ;
            else if( readTypMinMaxValueSubparam( std::string( "C_comp_pulldown" ),
                                                 m_currentModel->m_C_comp_pulldown ) )
                ;
            else
                status = false;

            m_continue = IBIS_PARSER_CONTINUE::MODEL;

            break;

        default:
            status = false;
            Report( _( "Continued reading a model that did not begin. ( internal error )" ),
                    RPT_SEVERITY_ERROR );
        }
    }

    return status;
}


bool IbisParser::readSubmodel()
{
    bool status = true;

    std::string subparam;
    if( readWord( subparam ) )
    {
        if( !strcmp( subparam.c_str(), "Submodel_type" ) )
        {
            if( readWord( subparam ) )
            {
                if( !strcmp( subparam.c_str(), "Dynamic_clamp" ) )
                    m_currentSubmodel->m_type = IBIS_SUBMODEL_TYPE::DYNAMIC_CLAMP;
                else if( !strcmp( subparam.c_str(), "Bus_hold" ) )
                    m_currentSubmodel->m_type = IBIS_SUBMODEL_TYPE::BUS_HOLD;
                else if( !strcmp( subparam.c_str(), "Fall_back" ) )
                    m_currentSubmodel->m_type = IBIS_SUBMODEL_TYPE::FALL_BACK;
                else
                {
                    std::stringstream message;
                    message << subparam << _( " is not a recognised submodel type." );
                    Report( message.str(), RPT_SEVERITY_ERROR );
                    status = false;
                }
            }
            else
            {
                Report( _( "The Submodel_type value is missing." ), RPT_SEVERITY_ERROR );
                status = false;
            }
        }
        else
        {
            std::stringstream message;
            message << subparam << _( " is not a recognised Submodel subparameter." );
            Report( message.str(), RPT_SEVERITY_ERROR );
            status = false;
        }
    }

    return status;
}


bool IbisParser::parseHeader( std::string& aKeyword )
{
    bool status = true;

    if( compareIbisWord( aKeyword.c_str(), "IBIS_Ver" ) )
    {
        status &= readDouble( m_ibisFile.m_header.m_ibisVersion );
    }
    else if( compareIbisWord( aKeyword.c_str(), "Comment_char" ) )
    {
        status &= changeCommentChar();
    }
    else if( compareIbisWord( aKeyword.c_str(), "File_Name" ) )
    {
        status &= storeString( m_ibisFile.m_header.m_fileName, false );
    }
    else if( compareIbisWord( aKeyword.c_str(), "File_Rev" ) )
    {
        status &= readDouble( m_ibisFile.m_header.m_fileRevision );
    }
    else if( compareIbisWord( aKeyword.c_str(), "Source" ) )
    {
        status &= storeString( m_ibisFile.m_header.m_source, true );
    }
    else if( compareIbisWord( aKeyword.c_str(), "Notes" ) )
    {
        status &= storeString( m_ibisFile.m_header.m_notes, true );
    }
    else if( compareIbisWord( aKeyword.c_str(), "Disclaimer" ) )
    {
        status &= storeString( m_ibisFile.m_header.m_disclaimer, true );
    }
    else if( compareIbisWord( aKeyword.c_str(), "Copyright" ) )
    {
        status &= storeString( m_ibisFile.m_header.m_copyright, true );
    }
    else if( compareIbisWord( aKeyword.c_str(), "Date" ) )
    {
        status &= storeString( m_ibisFile.m_header.m_date, false );
    }
    else if( !changeContext( aKeyword ) )
    {
        status = false;
    }

    return status;
}


bool IbisParser::parseComponent( std::string& aKeyword )
{
    bool status = true;

    if( compareIbisWord( aKeyword.c_str(), "Manufacturer" ) )
    {
        status &= storeString( m_currentComponent->m_manufacturer, true );
    }
    else if( compareIbisWord( aKeyword.c_str(), "Package" ) )
    {
        status &= readPackage();
    }
    else if( compareIbisWord( aKeyword.c_str(), "Pin" ) )
    {
        status &= readPin();
    }
    else if( compareIbisWord( aKeyword.c_str(), "Pin_Mapping" ) )
    {
        status &= readPinMapping();
    }
    else if( compareIbisWord( aKeyword.c_str(), "Diff_Pin" ) )
    {
        status &= readDiffPin();
    }
    /*
    // Not supported yet
    else if( aKeyword == "Die_Supply_Pads" )
    {
        status &= ReadDieSupplyPads();
    }*/
    else if( compareIbisWord( aKeyword.c_str(), "Package_Model" ) )
    {
        status &= storeString( m_currentComponent->m_packageModel, true );
    }
    else if( !changeContext( aKeyword ) )
    {
        status = false;
    }

    return status;
}

bool IbisParser::readTableLine( std::vector<std::string>& aDest )
{
    aDest.clear();

    while( m_lineIndex < m_lineLength )
    {
        std::string str;

        while( !isspace( m_buffer[m_lineOffset + m_lineIndex] ) && m_lineIndex < m_lineLength )
            str += m_buffer[m_lineOffset + m_lineIndex++];

        if( str.size() > 0 )
            aDest.push_back( str );

        while( isspace( m_buffer[m_lineOffset + m_lineIndex] ) && m_lineIndex < m_lineLength )
            m_lineIndex++;
    }

    return true;
}

bool IbisParser::readPackage()
{
    bool status = true;

    std::vector<std::string> fields;

    TypMinMaxValue* R = &( m_currentComponent->m_package.m_Rpkg );
    TypMinMaxValue* L = &( m_currentComponent->m_package.m_Lpkg );
    TypMinMaxValue* C = &( m_currentComponent->m_package.m_Cpkg );

    readTableLine( fields );

    int extraArg = ( m_continue == IBIS_PARSER_CONTINUE::NONE ) ? 1 : 0;

    if( (int)fields.size() == ( 4 + extraArg ) )
    {
        TypMinMaxValue* cValue;

        if( fields.at( 0 ) == "R_pkg" )
            cValue = R;
        else if( fields.at( 0 ) == "L_pkg" )
            cValue = L;
        else if( fields.at( 0 ) == "C_pkg" )
            cValue = C;
        else
        {
            Report( "Invalid field in [Package]" );
            return false;
        }
        status &= parseDouble( cValue->value[IBIS_CORNER::TYP], fields.at( 1 ), true );
        // Min / max values are optional, so don't update the status
        parseDouble( cValue->value[IBIS_CORNER::MIN], fields.at( 2 ), true );
        parseDouble( cValue->value[IBIS_CORNER::MAX], fields.at( 3 ), true );
    }
    else
    {
        if( fields.size() != 0 )
        {
            Report( _( "A [Package] line requires exactly 4 elements." ), RPT_SEVERITY_ERROR );
            status = false;
        }
    }

    m_continue = IBIS_PARSER_CONTINUE::COMPONENT_PACKAGE;

    return status;
}


bool IbisParser::readPin()
{
    bool status = true;

    std::vector<std::string> fields;

    m_lineIndex = 0;
    status &= readTableLine( fields );

    IbisComponentPin pin( m_Reporter );

    if( ( fields.size() == 3 ) )
    {
        if( m_continue == IBIS_PARSER_CONTINUE::COMPONENT_PIN ) // No info on first line
        {
            pin.m_pinName = fields.at( 0 );
            pin.m_signalName = fields.at( 1 );
            pin.m_modelName = fields.at( 2 );
            pin.m_Rcol = m_currentComponent->m_pins.back().m_Rcol;
            pin.m_Lcol = m_currentComponent->m_pins.back().m_Lcol;
            pin.m_Ccol = m_currentComponent->m_pins.back().m_Ccol;
        }
        else
        {
            pin.m_dummy = true;
        }
    }
    else
    {
        if( m_continue == IBIS_PARSER_CONTINUE::COMPONENT_PIN ) // Not on the first line
        {
            pin.m_pinName = fields.at( 0 );
            pin.m_signalName = fields.at( 1 );
            pin.m_modelName = fields.at( 2 );

            pin.m_Rcol = m_currentComponent->m_pins.back().m_Rcol;
            pin.m_Lcol = m_currentComponent->m_pins.back().m_Lcol;
            pin.m_Ccol = m_currentComponent->m_pins.back().m_Ccol;

            if( pin.m_Rcol == 0 || pin.m_Lcol == 0 || pin.m_Ccol == 0 )
            {
                Report( _( "Invalid pin entry: 6 values from a table with only 3." ),
                        RPT_SEVERITY_ERROR );
                status = false; // Did we just try to go from a 3 column table to a 6 ?
            }
            else
            {
                if( !parseDouble( pin.m_Rpin, fields.at( pin.m_Rcol ), true )
                    || !parseDouble( pin.m_Lpin, fields.at( pin.m_Lcol ), true )
                    || !parseDouble( pin.m_Cpin, fields.at( pin.m_Ccol ), true ) )
                {
                    Report( _( "Can't read a R, L or C value for a pin." ), RPT_SEVERITY_ERROR );
                    status = false;
                }
            }
        }
        else
        {
            for( int i = 3; i < 6; i++ )
            {
                if( fields.at( i ) == "R_pin" )
                {
                    pin.m_Rcol = i;
                }
                else if( fields.at( i ) == "L_pin" )
                {
                    pin.m_Lcol = i;
                }
                else if( fields.at( i ) == "C_pin" )
                {
                    pin.m_Ccol = i;
                }
                else
                {
                    Report( _( "Invalid field name in [Pin]" ), RPT_SEVERITY_ERROR );
                    status = false;
                }
            }

            if( pin.m_Rcol == 0 || pin.m_Lcol == 0 || pin.m_Ccol == 0 )
            {
                Report( _( "Missing argument in [Pin]" ), RPT_SEVERITY_ERROR );
                status = false;
            }

            pin.m_dummy = true;
        }
    }

    m_currentComponent->m_pins.push_back( pin );
    m_continue = IBIS_PARSER_CONTINUE::COMPONENT_PIN;


    return status;
}


bool IbisParser::readPinMapping()
{
    bool status = true;

    std::vector<std::string> fields;

    status &= readTableLine( fields );

    IbisComponentPinMapping pinMapping( m_Reporter );

    if( m_continue == IBIS_PARSER_CONTINUE::NONE ) // No info on first line
    {
        m_continue = IBIS_PARSER_CONTINUE::COMPONENT_PINMAPPING;
    }
    else
    {
        if( fields.size() != 0 )
        {
            if( fields.size() > 6 || fields.size() < 3 )
            {
                Report( _( "Wrong number of columns for pin mapping." ), RPT_SEVERITY_ERROR );
                status = false;
            }
            else
            {
                pinMapping.m_pinName = fields.at( 0 );
                pinMapping.m_PDref = fields.at( 1 );
                pinMapping.m_PUref = fields.at( 2 );

                if( fields.size() > 3 )
                    pinMapping.m_GNDClampRef = fields.at( 3 );

                if( fields.size() > 4 )
                    pinMapping.m_POWERClampRef = fields.at( 4 );

                if( fields.size() > 5 )
                    pinMapping.m_extRef = fields.at( 5 );
            }
            m_currentComponent->m_pinMappings.push_back( pinMapping );
        }
    }

    return status;
}


bool IbisParser::readDiffPin()
{
    bool status = true;

    m_lineIndex = 0; // rewind
    IbisDiffPinEntry entry( m_Reporter );

    if( m_continue == IBIS_PARSER_CONTINUE::NONE ) // No info on first line
    {
        m_continue = IBIS_PARSER_CONTINUE::COMPONENT_DIFFPIN;
    }
    else
    {
        if( !readWord( entry.pinA ) )
        {
            Report( _( "Incorrect diff pin name" ), RPT_SEVERITY_ERROR );
            status = false;
        }

        if( !readWord( entry.pinB ) )
        {
            Report( _( "Incorrect inv_pin name" ), RPT_SEVERITY_ERROR );
            status = false;
        }

        if( status )
            m_currentComponent->m_diffPin.m_entries.push_back( entry );
    }

    return status;
}


bool IbisParser::readIVtableEntry( IVtable& aDest )
{
    bool status = true;

    skipWhitespaces();

    IVtableEntry entry( m_Reporter );

    if( m_lineIndex < m_lineLength )
    {
        std::string str;

        status &= readDouble( entry.V );

        if( status && readTypMinMaxValue( entry.I ) )
            aDest.m_entries.push_back( entry );
    }

    m_continue = IBIS_PARSER_CONTINUE::IV_TABLE;
    m_currentIVtable = &aDest;

    return status;
}

bool IbisParser::readVTtableEntry( VTtable& aDest )
{
    bool status = true;
    skipWhitespaces();

    VTtableEntry entry( m_Reporter );

    if( m_lineIndex < m_lineLength )
    {
        std::string str;
        status &= readDouble( entry.t );
        status &= readTypMinMaxValue( entry.V );
    }

    m_continue = IBIS_PARSER_CONTINUE::IV_TABLE;
    m_currentVTtable = &aDest;

    if( status )
        aDest.m_entries.push_back( entry );

    return status;
}

bool IbisParser::readWaveform( IbisWaveform* aDest, IBIS_WAVEFORM_TYPE aType )
{
    bool status = true;

    IbisWaveform* wf;

    if( m_continue != IBIS_PARSER_CONTINUE::WAVEFORM )
    {
        wf = new IbisWaveform( m_Reporter );
        wf->m_type = aType;

        switch( m_context )
        {
        case IBIS_PARSER_CONTEXT::MODEL:
            switch( aType )
            {
            case IBIS_WAVEFORM_TYPE::FALLING:
                m_currentModel->m_fallingWaveforms.push_back( wf );
                break;
            case IBIS_WAVEFORM_TYPE::RISING:
                m_currentModel->m_risingWaveforms.push_back( wf );
                break;
            default:
                wxLogMessage( "Invalid waveform type: %i", (int) aType );
                status = false;
            }
            break;
        case IBIS_PARSER_CONTEXT::SUBMODEL:
            switch( aType )
            {
            case IBIS_WAVEFORM_TYPE::FALLING:
                m_currentSubmodel->m_fallingWaveforms.push_back( wf );
                break;
            case IBIS_WAVEFORM_TYPE::RISING:
                m_currentSubmodel->m_risingWaveforms.push_back( wf );
                break;
            default:
                wxLogMessage( "Invalid waveform type: %i", (int) aType );
                status = false;
            }
            break;
        default:
            wxLogMessage( "Invalid context for waveform: %i", (int) m_context );
            return false;
        }
    }
    else
    {
        if( aDest != nullptr )
        {
            wf = aDest;
        }
        else
        {
            Report( _( "Internal error detected, a waveform should exist" ), RPT_SEVERITY_ERROR );
            return false;
        }
    }


    if( status & !isLineEmptyFromCursor() )
    {
        // readNumericSubparam() returns true if it could read the subparameter and store it
        // Testing all subparameters
        if( readNumericSubparam( std::string( "R_fixture" ), wf->m_R_fixture ) )
            ;
        else if( readNumericSubparam( std::string( "L_fixture" ), wf->m_L_fixture ) )
            ;
        else if( readNumericSubparam( std::string( "C_fixture" ), wf->m_C_fixture ) )
            ;
        else if( readNumericSubparam( std::string( "V_fixture" ), wf->m_V_fixture ) )
            ;
        else if( readNumericSubparam( std::string( "V_fixture_min" ), wf->m_V_fixture_min ) )
            ;
        else if( readNumericSubparam( std::string( "V_fixture_max" ), wf->m_V_fixture_max ) )
            ;
        else if( readNumericSubparam( std::string( "R_dut" ), wf->m_R_fixture ) )
            ;
        else if( readNumericSubparam( std::string( "L_dut" ), wf->m_L_fixture ) )
            ;
        else if( readNumericSubparam( std::string( "C_dut" ), wf->m_C_fixture ) )
            ;
        // The line is not a subparameter, then let's try to read a VT table entry
        else if( !readVTtableEntry( m_currentWaveform->m_table ) )
            status = false;
    }

    m_currentWaveform = wf;
    m_continue = IBIS_PARSER_CONTINUE::WAVEFORM;
    return status;
}

bool IbisParser::onNewLine()
{
    bool      status = true;
    char      c;
    std::string keyword = getKeyword();

    if( keyword.size() > 0 ) // New keyword
    {
        if( m_continue != IBIS_PARSER_CONTINUE::NONE )
            m_continue = IBIS_PARSER_CONTINUE::NONE;

        switch( m_context )
        {
        case IBIS_PARSER_CONTEXT::HEADER:
            status &= parseHeader( keyword );
            break;
        case IBIS_PARSER_CONTEXT::COMPONENT:
            status &= parseComponent( keyword );
            break;
        case IBIS_PARSER_CONTEXT::MODELSELECTOR:
            status &= parseModelSelector( keyword );
            break;
        case IBIS_PARSER_CONTEXT::MODEL:
            status &= parseModel( keyword );
            break;
        case IBIS_PARSER_CONTEXT::SUBMODEL:
            status &= parseSubmodel( keyword );
            break;
        case IBIS_PARSER_CONTEXT::PACKAGEMODEL:
            status &= parsePackageModel( keyword );
            break;
        case IBIS_PARSER_CONTEXT::PACKAGEMODEL_MODELDATA:
            status &= parsePackageModelModelData( keyword );
            break;
        case IBIS_PARSER_CONTEXT::ALGORITHMIC_MODEL:
            status &= parseAlgorithmicModel( keyword );
            break;
        default:
            status = false;
            Report( _( "Internal error: Bad parser context." ), RPT_SEVERITY_ERROR );
        }
    }
    else
    {
        skipWhitespaces();
        if( m_lineIndex == m_lineLength )
        {
            // That was an empty line
            return true;
        }

        // No new keyword ? Then it is the continuation of the previous one !
        switch( m_continue )
        {
        case IBIS_PARSER_CONTINUE::STRING:
            skipWhitespaces();
            *m_continuingString += '\n';
            status &= readString( *m_continuingString );
            break;
        case IBIS_PARSER_CONTINUE::COMPONENT_PACKAGE:
            status &= readPackage();
            break;
        case IBIS_PARSER_CONTINUE::COMPONENT_PIN:
            status &= readPin();
            break;
        case IBIS_PARSER_CONTINUE::COMPONENT_PINMAPPING:
            status &= readPinMapping();
            break;
        case IBIS_PARSER_CONTINUE::COMPONENT_DIFFPIN:
            status &= readDiffPin();
            break;
        case IBIS_PARSER_CONTINUE::MODELSELECTOR:
            status &= readModelSelector();
            break;
        case IBIS_PARSER_CONTINUE::MODEL:
            status &= readModel();
            break;
        case IBIS_PARSER_CONTINUE::SUBMODEL:
            status &= readSubmodel();
            break;
        case IBIS_PARSER_CONTINUE::IV_TABLE:
            status &= readIVtableEntry( *m_currentIVtable );
            break;
        case IBIS_PARSER_CONTINUE::VT_TABLE:
            status &= readVTtableEntry( *m_currentVTtable );
            break;
        case IBIS_PARSER_CONTINUE::WAVEFORM:
            status &= readWaveform( m_currentWaveform, m_currentWaveform->m_type );
            break;
        case IBIS_PARSER_CONTINUE::RAMP:
            status &= readRamp();
            break;
        case IBIS_PARSER_CONTINUE::MODEL_SPEC:
            status &= readModelSpec();
            break;
        case IBIS_PARSER_CONTINUE::RX_THRESHOLDS:
            status &= readReceiverThresholds();
            break;
        case IBIS_PARSER_CONTINUE::ALGORITHMIC_MODEL:
            status &= readAlgorithmicModel();
            break;
        case IBIS_PARSER_CONTINUE::ADD_SUBMODEL:
            status &= readAddSubmodel();
            break;
        case IBIS_PARSER_CONTINUE::SUBMODEL_SPEC:
            status &= readSubmodelSpec();
            break;
        case IBIS_PARSER_CONTINUE::PACKAGEMODEL_PINS:
            status &= readPackageModelPins();
            break;
        case IBIS_PARSER_CONTINUE::MATRIX:
            status &= readMatrixData();
            break;
        case IBIS_PARSER_CONTINUE::NONE:
        default:
            Report( _( "Missing keyword." ), RPT_SEVERITY_ERROR );
            return false;
            break;
        }
    }

    c = m_buffer[m_lineOffset + m_lineIndex];

    while( ( c != '\n' ) && ( c != 0 ) ) // Go to the end of line
        c = m_buffer[m_lineOffset + m_lineIndex++];

    return status;
}
