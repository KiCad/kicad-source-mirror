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


#ifndef IBIS_PARSER_H
#define IBIS_PARSER_H

#define NAN_NA "1"
#define NAN_INVALID "0"

#define IBIS_MAX_VERSION 7.0      // Up to v7.0, IBIS is fully backward compatible
#define IBIS_MAX_LINE_LENGTH 2048 // official limit is 1024

#include <wx/string.h>
#include <reporter.h>
#include "widgets/wx_html_report_panel.h"
#include <iostream>
#include <fstream>
#include <vector>
#include <map>
#include <math.h>
#include <cstring>


class IBIS_BASE
{
public:
    IBIS_BASE( REPORTER* aReporter )
    {
        m_Reporter = aReporter;
    };

    virtual ~IBIS_BASE() = default;

    /** @brief Print a message
     *
     * Call m_Reporter->Report if m_Reporter exists.
     *
     * @param aMsg Message
     * @param aSeverity Message sevirity
     */
    void Report( const std::string& aMsg, SEVERITY aSeverity = RPT_SEVERITY_INFO ) const
    {
        if( m_Reporter )
            m_Reporter->Report( aMsg, aSeverity );
    };

public:
    REPORTER* m_Reporter;

protected:
    /** @brief Convert a double to string using scientific notation
     *
     * @param aNumber Number
     * @return Output string
     */
    static std::string doubleToString( double aNumber );
};


class IBIS_INPUT : public IBIS_BASE
{
public:
    IBIS_INPUT( REPORTER* aReporter ) :
            IBIS_BASE( aReporter )
    {};

    /** @brief Check if the data held by the object is valid.
     *
     * @return true in case of success
     */
    bool virtual Check() { return false; };
};


enum IBIS_CORNER
{
    TYP = 0,
    MIN,
    MAX
};


enum class IBIS_MATRIX_TYPE
{
    // All matrices are supposed to be symmetrical, only upper right triangle is given
    UNDEFINED,
    BANDED, // Give the main diagonal + [bandwidth] elements on the right
    SPARSE, // Only give non-zero values.
    FULL,   // Give the whole upper triangle.
};


class IBIS_MATRIX : public IBIS_INPUT
{
public:
    IBIS_MATRIX( REPORTER* aReporter ) :
            IBIS_INPUT( aReporter )
    {};

    virtual ~IBIS_MATRIX(){};

    IBIS_MATRIX_TYPE    m_type = IBIS_MATRIX_TYPE::UNDEFINED;
    int                 m_rows = -1;
    int                 m_cols = -1;
    std::vector<double> m_data;

    bool Check() override;
};


class IBIS_SECTION : public IBIS_INPUT
{
public:
    IBIS_SECTION( REPORTER* aReporter ) :
            IBIS_INPUT( aReporter )
    {};
};


class IbisHeader : IBIS_SECTION
{
public:
    IbisHeader( REPORTER* aReporter ) :
            IBIS_SECTION( aReporter )
    {};

    double      m_ibisVersion = -1;
    double      m_fileRevision = -1;
    std::string m_fileName;
    std::string m_source;
    std::string m_date;
    std::string m_notes;
    std::string m_disclaimer;
    std::string m_copyright;

    bool Check() override;
};


class TypMinMaxValue : public IBIS_INPUT
{
public:
    TypMinMaxValue( REPORTER* aReporter ) :
            IBIS_INPUT( aReporter )
    {};

    double value[3] = { nan( NAN_NA ), nan( NAN_NA ), nan( NAN_NA ) };

    bool isNA() const;

    bool Check() override;

    void Add( const TypMinMaxValue& aValue );
};


class IbisComponentPackage : public IBIS_INPUT
{
public:
    IbisComponentPackage( REPORTER* aReporter ) :
            IBIS_INPUT( aReporter ),
            m_Rpkg( aReporter ),
            m_Lpkg( aReporter ),
            m_Cpkg( aReporter )
    {};

    TypMinMaxValue m_Rpkg;
    TypMinMaxValue m_Lpkg;
    TypMinMaxValue m_Cpkg;

    bool Check() override;
};


class IbisComponentPin : public IBIS_INPUT
{
public:
    IbisComponentPin( REPORTER* aReporter ) :
            IBIS_INPUT( aReporter )
    {};

    virtual ~IbisComponentPin()
    {};

    std::string m_pinName;
    std::string m_signalName;
    std::string m_modelName;
    double   m_Rpin = nan( NAN_NA );
    double   m_Lpin = nan( NAN_NA );
    double   m_Cpin = nan( NAN_NA );

    int m_Rcol = 0;
    int m_Lcol = 0;
    int m_Ccol = 0;

    bool Check() override;

    bool m_dummy = false;
};


class IbisComponentPinMapping : public IBIS_INPUT
{
public:
    IbisComponentPinMapping( REPORTER* aReporter ) :
            IBIS_INPUT( aReporter )
    {};

    virtual ~IbisComponentPinMapping()
    {};

    std::string m_pinName;
    std::string m_PDref;
    std::string m_PUref;
    std::string m_GNDClampRef;
    std::string m_POWERClampRef;
    std::string m_extRef;
};


class IbisDiffPinEntry : public IBIS_INPUT
{
public:
    IbisDiffPinEntry( REPORTER* aReporter ) :
            IBIS_INPUT( aReporter ),
            tdelay( aReporter )
    {};

    virtual ~IbisDiffPinEntry()
    {};

    std::string     pinA;
    std::string     pinB;
    double          Vdiff = 0.2; // ignored for input
    TypMinMaxValue  tdelay = 0;  // ignored for outputs
};


class IbisDiffPin : IBIS_INPUT
{
public:
    IbisDiffPin( REPORTER* aReporter ) :
            IBIS_INPUT( aReporter )
    {};

    std::vector<IbisDiffPinEntry> m_entries;
};

class IbisComponent : public IBIS_INPUT
{
public:
    IbisComponent( REPORTER* aReporter ) :
            IBIS_INPUT( aReporter ),
            m_package( aReporter ),
            m_diffPin( aReporter )
    {};

    virtual ~IbisComponent()
    {};

    std::string                             m_name = "";
    std::string                             m_manufacturer = "";
    IbisComponentPackage                    m_package;
    std::vector<IbisComponentPin>           m_pins;
    std::vector<IbisComponentPinMapping>    m_pinMappings;
    std::string                             m_packageModel;
    std::string                             m_busLabel;
    std::string                             m_dieSupplyPads;
    IbisDiffPin                             m_diffPin;

    bool Check() override;
};


class IbisModelSelectorEntry
{
public:
    std::string m_modelName;
    std::string m_modelDescription;
};


class IbisModelSelector : public IBIS_INPUT
{
public:
    IbisModelSelector( REPORTER* aReporter ) :
            IBIS_INPUT( aReporter )
    {};

    virtual ~IbisModelSelector()
    {};

    std::string                            m_name;
    std::vector<IbisModelSelectorEntry> m_models;

    bool Check() override;
};


class IVtableEntry : public IBIS_INPUT
{
public:
    IVtableEntry( REPORTER* aReporter ) :
            IBIS_INPUT( aReporter ),
            I( aReporter )
    {};

    virtual ~IVtableEntry()
    {};

    double         V = 0;
    TypMinMaxValue I;
};


class IVtable : public IBIS_INPUT
{
public:
    IVtable( REPORTER* aReporter ) :
            IBIS_INPUT( aReporter )
    {};

    std::vector<IVtableEntry> m_entries;

    bool Check() override;

    /** @brief Interpolate the IV table
     *
     * Linear interpolation to find the current for voltage aV
     *
     * @param aV voltage
     * @param aCorner Power supply corner
     * @return current
     */
    double InterpolatedI( double aV, IBIS_CORNER aCorner ) const;

    /** @brief Interpolate the IV table
     *
     * Generate the spice directive needed to define a model defined by its IV table.
     * The order of aPort1 and aPort2 is important. ( Inverting them will reverse the component )
     *
     * @param aN Index of the 'a' device
     * @param aPort1 Spice node
     * @param aPort2 Spice node
     * @param aNegateI Negate I values
     * @param aModelName Name of the generated model
     * @param aCorner Power supply corner
     * @return Multline spice directives
     */
    std::string Spice( int aN, const std::string& aPort1, const std::string& aPort2, bool aNegateI,
                       const std::string& aModelName, IBIS_CORNER aCorner ) const;

private:
};

class VTtableEntry : public IBIS_INPUT
{
public:
    VTtableEntry( REPORTER* aReporter ) :
            IBIS_INPUT( aReporter ),
            V( aReporter )
    {};

    virtual ~VTtableEntry()
    {};

    double         t = 0;
    TypMinMaxValue V = 0;
};


class VTtable : public IBIS_INPUT
{
public:
    VTtable( REPORTER* aReporter ) :
            IBIS_INPUT( aReporter )
    {};

    std::vector<VTtableEntry> m_entries;
};

/*
Model_type must be one of the following:
Input, Output, I/O, 3-state, Open_drain, I/O_open_drain, Open_sink, I/O_open_sink,
Open_source, I/O_open_source, Input_ECL, Output_ECL, I/O_ECL, 3-state_ECL, Terminator,
Series, and Series_switch.
*/

enum class IBIS_MODEL_TYPE
{
    UNDEFINED,
    INPUT_STD,       // Do not use INPUT: it can conflict with a windows header on MSYS2
    OUTPUT,
    IO,
    THREE_STATE,
    OPEN_DRAIN,
    IO_OPEN_DRAIN,
    OPEN_SINK,
    IO_OPEN_SINK,
    OPEN_SOURCE,
    IO_OPEN_SOURCE,
    INPUT_ECL,
    OUTPUT_ECL,
    IO_ECL,
    THREE_STATE_ECL,
    TERMINATOR,
    SERIES,
    SERIES_SWITCH
};

enum class IBIS_MODEL_ENABLE
{
    UNDEFINED,
    ACTIVE_HIGH,
    ACTIVE_LOW
};


class dvdt
{
public:
    double m_dv = 1;
    double m_dt = 1;
};


class dvdtTypMinMax : public IBIS_INPUT
{
public:
    dvdtTypMinMax( REPORTER* aReporter ) : IBIS_INPUT( aReporter ){};
    dvdt value[3];

    bool Check() override;
};


class IbisRamp : public IBIS_INPUT
{
public:
    IbisRamp( REPORTER* aReporter ) :
            IBIS_INPUT( aReporter ),
            m_falling( aReporter ),
            m_rising( aReporter )
    {};

    dvdtTypMinMax m_falling;
    dvdtTypMinMax m_rising;
    double m_Rload = 50; // The R_load subparameter is optional if the default 50 ohm load is used

    bool Check() override;
};


enum class IBIS_WAVEFORM_TYPE
{
    RISING,
    FALLING
};


class IbisWaveform : public IBIS_INPUT
{
public:
    IbisWaveform( REPORTER* aReporter ) :
            IBIS_INPUT( aReporter ),
            m_table( aReporter )
    {};

    VTtable            m_table;
    IBIS_WAVEFORM_TYPE m_type = IBIS_WAVEFORM_TYPE::RISING;
    double             m_R_dut = 0;
    double             m_C_dut = 0;
    double             m_L_dut = 0;
    double             m_R_fixture = 0;
    double             m_C_fixture = 0;
    double             m_L_fixture = 0;
    double             m_V_fixture = 0;
    double             m_V_fixture_min = 0;
    double             m_V_fixture_max = 0;
};


enum class IBIS_MODEL_POLARITY
{
    UNDEFINED,
    INVERTING,
    NON_INVERTING
};


enum class IBIS_SUBMODEL_MODE
{
    ALL,
    DRIVING,
    NON_DRIVING
};


class IbisSubmodelMode
{
public:
    IbisSubmodelMode( std::string name, IBIS_SUBMODEL_MODE mode ) :
            m_name( name ),
            m_mode( mode )
    {};

    std::string        m_name;
    IBIS_SUBMODEL_MODE m_mode;
};


class IbisModel : IBIS_INPUT
{
public:
    IbisModel( REPORTER* aReporter ) :
            IBIS_INPUT( aReporter ),
            m_C_comp( aReporter ),
            m_C_comp_gnd_clamp( aReporter ),
            m_C_comp_power_clamp( aReporter ),
            m_C_comp_pullup( aReporter ),
            m_C_comp_pulldown( aReporter ),
            m_voltageRange( aReporter ),
            m_temperatureRange( aReporter ),
            m_pullupReference( aReporter ),
            m_pulldownReference( aReporter ),
            m_GNDClampReference( aReporter ),
            m_POWERClampReference( aReporter ),
            m_Rgnd( aReporter ),
            m_Rpower( aReporter ),
            m_Rac( aReporter ),
            m_Cac( aReporter ),
            m_GNDClamp( aReporter ),
            m_POWERClamp( aReporter ),
            m_pullup( aReporter ),
            m_pulldown( aReporter ),
            m_ISSO_PU( aReporter ),
            m_ISSO_PD( aReporter ),
            m_compositeCurrent( aReporter ),
            m_ramp( aReporter )
    {};

    virtual ~IbisModel() = default;

    std::string        m_name;
    IBIS_MODEL_TYPE m_type = IBIS_MODEL_TYPE::UNDEFINED;
    /* The Polarity, Enable, Vinl, Vinh, Vmeas, Cref, Rref, and Vref subparameters are optional. */
    /* the default values of Vinl = 0.8 V and Vinh = 2.0 V are assumed. */
    double              m_vinl = 0.8;
    double              m_vinh = 2;
    double              m_vref = 0;
    double              m_rref = 0;
    double              m_cref = 0;
    double              m_vmeas = 0;
    IBIS_MODEL_ENABLE   m_enable = IBIS_MODEL_ENABLE::UNDEFINED;
    IBIS_MODEL_POLARITY m_polarity = IBIS_MODEL_POLARITY::UNDEFINED;
    // End of optional subparameters

    TypMinMaxValue             m_C_comp;
    TypMinMaxValue             m_C_comp_gnd_clamp;
    TypMinMaxValue             m_C_comp_power_clamp;
    TypMinMaxValue             m_C_comp_pullup;
    TypMinMaxValue             m_C_comp_pulldown;
    TypMinMaxValue             m_voltageRange;
    TypMinMaxValue             m_temperatureRange;
    TypMinMaxValue             m_pullupReference;
    TypMinMaxValue             m_pulldownReference;
    TypMinMaxValue             m_GNDClampReference;
    TypMinMaxValue             m_POWERClampReference;
    TypMinMaxValue             m_Rgnd;
    TypMinMaxValue             m_Rpower;
    TypMinMaxValue             m_Rac;
    TypMinMaxValue             m_Cac;
    IVtable                    m_GNDClamp;
    IVtable                    m_POWERClamp;
    IVtable                    m_pullup;
    IVtable                    m_pulldown;
    IVtable                    m_ISSO_PU;
    IVtable                    m_ISSO_PD;
    IVtable                    m_compositeCurrent;
    std::vector<IbisWaveform*> m_risingWaveforms;
    std::vector<IbisWaveform*> m_fallingWaveforms;
    IbisRamp                   m_ramp;

    std::vector<IbisSubmodelMode> m_submodels;

    bool Check() override;
};


enum class IBIS_SUBMODEL_TYPE
{
    UNDEFINED,
    DYNAMIC_CLAMP,
    BUS_HOLD,
    FALL_BACK
};


class IbisSubmodel : IBIS_INPUT
{
public:
    IbisSubmodel( REPORTER* aReporter ) :
            IBIS_INPUT( aReporter ),
            m_VtriggerR( aReporter ),
            m_VtriggerF( aReporter ),
            m_offDelay( aReporter ),
            m_pullup( aReporter ),
            m_pulldown( aReporter ),
            m_GNDClamp( aReporter ),
            m_POWERClamp( aReporter ),
            m_GNDPulse( aReporter ),
            m_POWERPulse( aReporter ),
            m_ramp( aReporter )
    {};

    virtual ~IbisSubmodel() = default;

    std::string                m_name;
    IBIS_SUBMODEL_TYPE         m_type = IBIS_SUBMODEL_TYPE::UNDEFINED;
    TypMinMaxValue             m_VtriggerR;
    TypMinMaxValue             m_VtriggerF;
    TypMinMaxValue             m_offDelay;
    IVtable                    m_pullup;
    IVtable                    m_pulldown;
    IVtable                    m_GNDClamp;
    IVtable                    m_POWERClamp;
    IVtable                    m_GNDPulse;
    IVtable                    m_POWERPulse;
    IbisRamp                   m_ramp;
    std::vector<IbisWaveform*> m_risingWaveforms;
    std::vector<IbisWaveform*> m_fallingWaveforms;

    bool Check() override;
};


class IbisPackageModel : public IBIS_INPUT
{
public:
    IbisPackageModel( REPORTER* aReporter ) :
            IBIS_INPUT( aReporter )
    {};

    virtual ~IbisPackageModel()
    {};

    std::string                m_name;
    std::string                m_manufacturer;
    std::string                m_OEM;
    std::string                m_description;
    int                        m_numberOfPins = 0;
    std::map<std::string, int> m_pins;

    std::shared_ptr<IBIS_MATRIX> m_resistanceMatrix;
    std::shared_ptr<IBIS_MATRIX> m_capacitanceMatrix;
    std::shared_ptr<IBIS_MATRIX> m_inductanceMatrix;

    bool Check() override;
};


class IbisFile : public IBIS_INPUT
{
public:
    IbisFile( REPORTER* aReporter ) :
            IBIS_INPUT( aReporter ),
            m_header( aReporter )
    {};

    virtual ~IbisFile()
    {};

    IbisHeader                          m_header;
    std::vector<IbisComponent>          m_components;
    std::vector<IbisModelSelector>      m_modelSelectors;
    std::vector<IbisModel>              m_models;
    std::vector<IbisPackageModel>       m_packageModels;
    std::map<std::string, IbisSubmodel> m_submodels;
};


enum class IBIS_PARSER_CONTINUE
{
    NONE,
    STRING,
    COMPONENT_PACKAGE,
    COMPONENT_PINMAPPING,
    COMPONENT_DIFFPIN,
    COMPONENT_DIESUPPLYPADS,
    COMPONENT_PIN,
    MATRIX,
    MODELSELECTOR,
    MODEL,
    MODEL_SPEC,
    SUBMODEL,
    SUBMODEL_SPEC,
    RX_THRESHOLDS,
    ALGORITHMIC_MODEL,
    ADD_SUBMODEL,
    IV_TABLE,
    VT_TABLE,
    RAMP,
    WAVEFORM,
    PACKAGEMODEL_PINS
};

enum class IBIS_PARSER_CONTEXT
{
    HEADER,
    COMPONENT,
    MODELSELECTOR,
    MODEL,
    SUBMODEL,
    PACKAGEMODEL,
    PACKAGEMODEL_MODELDATA,
    ALGORITHMIC_MODEL,
    END
};


class IbisParser : public IBIS_INPUT
{
public:
    IbisParser( REPORTER* aReporter ) :
            IBIS_INPUT( aReporter ),
            m_ibisFile( aReporter )
    {};

    bool m_parrot = true; // Write back all lines.

    long  m_lineCounter = 0;
    char  m_commentChar = '|';
    std::vector<char> m_buffer;
    int   m_bufferIndex = 0;
    int   m_lineOffset = 0;
    int   m_lineIndex = 0;
    int   m_lineLength = 0;

    IbisFile           m_ibisFile;
    IbisComponent*     m_currentComponent = nullptr;
    IbisModelSelector* m_currentModelSelector = nullptr;
    IbisModel*         m_currentModel = nullptr;
    IbisSubmodel*      m_currentSubmodel = nullptr;
    IbisPackageModel*  m_currentPackageModel = nullptr;
    std::shared_ptr<IBIS_MATRIX> m_currentMatrix = nullptr;
    int                m_currentMatrixRow = 0;
    int                m_currentMatrixCol = 0;
    IVtable*           m_currentIVtable = nullptr;
    VTtable*           m_currentVTtable = nullptr;
    IbisWaveform*      m_currentWaveform = nullptr;

    /** @brief Parse a file
     *
     * This is the entry point to parse a file
     *
     * @param aFileName input file name
     * @return True in case of success
     */
    bool ParseFile( const std::string& aFileName );

private:
    std::string* m_continuingString = nullptr;

    /** @brief compare two strings without being case sensitive
     *
     * Ibis: "The content of the files is case sensitive, except for reserved words and keywords."
     *
     * @param a string to compare
     * @param b string to compare
     * @return true if the string are equal
     */
    bool compareIbisWord( const std::string& a, const std::string& b );

    /** @brief Parse a single keyword in the header context
     *
     * @param aKeyword Keyword
     * @return True in case of success
     */
    bool parseHeader( std::string& aKeyword );

    /** @brief Parse a single keyword in the component context
     *
     * @param aKeyword Keyword
     * @return True in case of success
     */
    bool parseComponent( std::string& aKeyword );

    /** @brief Parse a single keyword in the model selector context
     *
     * @param aKeyword Keyword
     * @return True in case of success
     */
    bool parseModelSelector( std::string& aKeyword );

    /** @brief Parse a single keyword in the model context
     *
     * @param aKeyword Keyword
     * @return True in case of success
     */
    bool parseModel( std::string& aKeyword );

    /** @brief Parse a single keyword in the submodel context
     *
     * @param aKeyword Keyword
     * @return True in case of success
     */
    bool parseSubmodel( std::string& aKeyword );

    /** @brief Parse a single keyword in the package model context
     *
     * @param aKeyword Keyword
     * @return True in case of success
     */
    bool parsePackageModel( std::string& aKeyword );

    /** @brief Parse a single keyword in the package model model data context
     *
     * @param aKeyword Keyword
     * @return True in case of success
     */
    bool parsePackageModelModelData( std::string& aKeyword );

    /** @brief Parse a single keyword in the algorithmic model context
     *
     * @param aKeyword Keyword
     * @return True in case of success
     */
    bool parseAlgorithmicModel( std::string& aKeyword );

    /** @brief Parse a double according to the ibis standard
     *
     * @param aDest Where the double should be stored
     * @param aStr The string to parse
     * @param aAllowModifiers Allows modifiers ( p for pico, f for femto, k for kilo, ... )
     * @return True in case of success
     */
    bool parseDouble( double& aDest, std::string& aStr, bool aAllowModifiers = false );

    /** @brief Parse a dV/dt value according to the ibis standard
     *
     * @param aDest Where the dV/dt value should be stored
     * @param aStr The string to parse
     * @return True in case of success
     */
    bool parseDvdt( dvdt& aDest, std::string& aStr );

    /** @brief Parse the current line
     *
     * @return True in case of success
     */
    bool onNewLine(); // Gets rid of comments ( except on a comment character change command...)

    /** @brief Load the next line
     *
     * @return True in case of success
     */
    bool getNextLine();

    /** @brief Print the current line */
    void printLine();

    void      skipWhitespaces();
    bool      checkEndofLine(); // To be used when there cannot be any character left on the line
    bool      isLineEmptyFromCursor();
    std::string getKeyword();

    bool readInt( int& aDest );
    bool readDouble( double& aDest );
    bool readWord( std::string& aDest );
    bool readDvdt( dvdt& aDest );
    bool readMatrixPinIndex( int& aDest );
    bool readMatrixType( std::shared_ptr<IBIS_MATRIX>& aDest );
    bool readMatrixBandwidth();
    bool readMatrixRow();
    bool readMatrixBandedOrFull();
    bool readMatrixSparse();
    bool readMatrixData();
    bool readRampdvdt( dvdtTypMinMax& aDest );
    bool readRamp();
    bool readModelSpec();
    bool readSubmodelSpec();
    bool readReceiverThresholds();
    bool readAlgorithmicModel();
    bool readAddSubmodel();
    bool readWaveform( IbisWaveform* aDest, IBIS_WAVEFORM_TYPE aType );
    bool readString( std::string& aDest );
    bool storeString( std::string& aDest, bool aMultiline );
    bool readTableLine( std::vector<std::string>& aDest );

    bool readNumericSubparam( const std::string& aSubparam, double& aDest );
    bool readIVtableEntry( IVtable& aTable );
    bool readVTtableEntry( VTtable& aTable );
    bool readTypMinMaxValue( TypMinMaxValue& aDest );
    bool readTypMinMaxValueSubparam( const std::string& aSubparam, TypMinMaxValue& aDest );
    //bool ReadDieSupplyPads();

    bool readPackage();
    bool readPin();
    bool readPinMapping();
    bool readDiffPin();
    bool readModelSelector();
    bool readModel();
    bool readSubmodel();
    bool readPackageModelPins();

    /** @brief Ibis can change the character used for comments */
    bool changeCommentChar();
    bool changeContext( std::string& aKeyword );

    IBIS_PARSER_CONTINUE m_continue = IBIS_PARSER_CONTINUE::NONE;
    IBIS_PARSER_CONTEXT  m_context = IBIS_PARSER_CONTEXT::HEADER;
};

#endif
