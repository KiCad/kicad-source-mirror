/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2016-2022 CERN
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * @author Maciej Suminski <maciej.suminski@cern.ch>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 3
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you may find one here:
 * https://www.gnu.org/licenses/gpl-3.0.html
 * or you may search the http://www.gnu.org website for the version 3 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#ifndef SPICE_CIRCUIT_MODEL_H
#define SPICE_CIRCUIT_MODEL_H

#include <netlist_exporters/netlist_exporter_spice.h>
#include <vector>

#include "sim_types.h"
#include "spice_simulator.h"
#include "spice_value.h"


struct SPICE_DC_PARAMS
{
    wxString m_source;
    SPICE_VALUE m_vstart;
    SPICE_VALUE m_vend;
    SPICE_VALUE m_vincrement;
};


struct SPICE_PZ_ANALYSES
{
    bool m_Poles;
    bool m_Zeros;
};

/// Special netlist exporter flavor that allows one to override simulation commands
class SPICE_CIRCUIT_MODEL : public NETLIST_EXPORTER_SPICE, public SIMULATION_MODEL
{
public:
    SPICE_CIRCUIT_MODEL( SCHEMATIC* aSchematic ) :
            NETLIST_EXPORTER_SPICE( aSchematic )
    {}

    virtual ~SPICE_CIRCUIT_MODEL() {}

    /**
     * Return name of Spice dataset for a specific trace.
     *
     * @param aVector is name of the vector produced by ngspice
     * @param [out] aSignal is output in form: V(R1), Ib(Q2), I(L8)
     * @return [SPT_VOLTAGE, SPT_CURRENT]. Otherwise SPT_UNKNOWN if vector is of different,
     *                                     unsupported type.
     */
    SIM_TRACE_TYPE VectorToSignal( const std::string& aVector, wxString& aSignal ) const;

    bool GetNetlist( const wxString& aCommand, unsigned aOptions, OUTPUTFORMATTER* aFormatter,
                     REPORTER& aReporter )
    {
        return SPICE_CIRCUIT_MODEL::DoWriteNetlist( aCommand, aOptions, *aFormatter, aReporter );
    }

    /**
     * Return simulation command directives placed in schematic sheets (if any).
     */
    wxString GetSchTextSimCommand();

    /**
     * Parse a two-source .dc command directive into its symbols.
     *
     * @param aCmd is the input command string
     * @return true if the command was parsed successfully
     */
    bool ParseDCCommand( const wxString& aCmd, SPICE_DC_PARAMS* aSource1,
                         SPICE_DC_PARAMS* aSource2 );

    bool ParsePZCommand( const wxString& aCmd, wxString* transferFunction, wxString* input,
                         wxString* inputRef, wxString* output, wxString* outputRef,
                         SPICE_PZ_ANALYSES* analyses );

    bool ParseNoiseCommand( const wxString& aCmd, wxString* aOutput, wxString* aRef,
                            wxString* aSource, wxString* aScale, SPICE_VALUE* aPts,
                            SPICE_VALUE* aFStart, SPICE_VALUE* aFStop, bool* aSaveAll );

    /**
     * Determine if a directive is a simulation command.
     */
    static bool IsSimCommand( const wxString& aCmd )
    {
        return CommandToSimType( aCmd ) != ST_UNKNOWN;
    }

    /**
     * Return simulation type basing on a simulation command directive.
     */
    static SIM_TYPE CommandToSimType( const wxString& aCmd );

protected:
    void WriteDirectives( const wxString& aSimCommand, unsigned aSimOptions,
                          OUTPUTFORMATTER& aFormatter ) const override;
};

#endif /* SPICE_CIRCUIT_MODEL_H */
