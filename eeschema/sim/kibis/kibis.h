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


#ifndef KIBIS_H
#define KIBIS_H

#include "ibis_parser.h"

class KIBIS_PIN;
class KIBIS_FILE;
class KIBIS_MODEL;
class KIBIS_COMPONENT;
class KIBIS;

class KIBIS_BASE : public IBIS_BASE
{
protected:
    KIBIS_BASE( KIBIS* aTopLevel );

    /**
     * Ctor for when a reporter is not available in the top level object
     * (e.g. when the top level object itself is under construction)
     */
    KIBIS_BASE( KIBIS* aTopLevel, REPORTER* aReporter );

public:
    KIBIS* m_topLevel;
    bool   m_valid;
};

enum class KIBIS_WAVEFORM_TYPE
{
    NONE = 0, // Used for three state
    PRBS,
    RECTANGULAR,
    STUCK_HIGH,
    STUCK_LOW,
    HIGH_Z
};


class KIBIS_WAVEFORM : public KIBIS_BASE
{
public:
    KIBIS_WAVEFORM( KIBIS& aTopLevel ) :
            KIBIS_BASE{ &aTopLevel }
    {
        m_valid = true;
    };

    KIBIS_WAVEFORM_TYPE GetType() const { return m_type; };
    virtual double      GetDuration() const { return 1; };
    bool                inverted = false; // Used for differential drivers
    virtual ~KIBIS_WAVEFORM() {};

    virtual std::vector<std::pair<int, double>> GenerateBitSequence() const
    {
        std::vector<std::pair<int, double>> bits;
        return bits;
    };

    // Check function if using waveform data
    virtual bool Check( const IbisWaveform* aRisingWf, const IbisWaveform* aFallingWf ) const
    {
        return true;
    };
    // Check function if using ramp data
    virtual bool Check( const dvdtTypMinMax& aRisingRp, const dvdtTypMinMax& aFallingRp ) const
    {
        return true;
    };

protected:
    KIBIS_WAVEFORM_TYPE m_type = KIBIS_WAVEFORM_TYPE::NONE;
};

class KIBIS_WAVEFORM_RECTANGULAR : public KIBIS_WAVEFORM
{
public:
    KIBIS_WAVEFORM_RECTANGULAR( KIBIS& aTopLevel ) :
            KIBIS_WAVEFORM( aTopLevel )
    {
        m_type = KIBIS_WAVEFORM_TYPE::RECTANGULAR;
    };

    double m_ton = 100e-9;
    double m_toff = 100e-9;
    double m_delay = 0;
    int    m_cycles = 1;


    std::vector<std::pair<int, double>> GenerateBitSequence() const override;
    bool Check( const IbisWaveform* aRisingWf, const IbisWaveform* aFallingWf ) const override;
    bool Check( const dvdtTypMinMax& aRisingRp, const dvdtTypMinMax& aFallingRp ) const override;

    double GetDuration() const override { return ( m_ton + m_toff ) * m_cycles; };
};

// For now, we only support PRBS7
class KIBIS_WAVEFORM_PRBS : public KIBIS_WAVEFORM
{
public:
    KIBIS_WAVEFORM_PRBS( KIBIS& aTopLevel ) :
            KIBIS_WAVEFORM( aTopLevel )
    {
        m_type = KIBIS_WAVEFORM_TYPE::PRBS;
    };

    double m_bitrate = 10e6;
    double m_delay = 0;
    int m_bits = 10;

    std::vector<std::pair<int, double>> GenerateBitSequence() const override;
    bool Check( const IbisWaveform* aRisingWf, const IbisWaveform* aFallingWf ) const override;
    bool Check( const dvdtTypMinMax& aRisingRp, const dvdtTypMinMax& aFallingRp ) const override;

    void SetBits( int aBits ) { m_bits = std::abs( aBits ); };

    double GetDuration() const override { return m_bits / m_bitrate; };
};

class KIBIS_WAVEFORM_STUCK_HIGH : public KIBIS_WAVEFORM
{
public:
    KIBIS_WAVEFORM_STUCK_HIGH( KIBIS& aTopLevel ) :
            KIBIS_WAVEFORM( aTopLevel )
    {
        m_type = KIBIS_WAVEFORM_TYPE::STUCK_HIGH;
    };

    std::vector<std::pair<int, double>> GenerateBitSequence() const override;
};

class KIBIS_WAVEFORM_STUCK_LOW : public KIBIS_WAVEFORM
{
public:
    KIBIS_WAVEFORM_STUCK_LOW( KIBIS& aTopLevel ) :
            KIBIS_WAVEFORM( aTopLevel )
    {
        m_type = KIBIS_WAVEFORM_TYPE::STUCK_LOW;
    };

    std::vector<std::pair<int, double>> GenerateBitSequence() const override;
};

class KIBIS_WAVEFORM_HIGH_Z : public KIBIS_WAVEFORM
{
public:
    KIBIS_WAVEFORM_HIGH_Z( KIBIS& aTopLevel ) :
            KIBIS_WAVEFORM( aTopLevel )
    {
        m_type = KIBIS_WAVEFORM_TYPE::HIGH_Z;
    };

    std::vector<std::pair<int, double>> GenerateBitSequence() const override;
};

/** Accuracy level.
 *
 * Level 0 is faster, but not as accurate
 *
 * Level 0 :
 *      - Driver: Don't use waveform
 *      - Driver: Don't use _DUT info
 * Level 1 :
 *      _ Driver: Use up to one waveform
 *      _ Driver: Don't use _DUT info
 * Level 2 :
 *      _ Driver: Use up to two waveforms
 *      _ Driver: Don't use _DUT info
 *
 * Level 3 : ( not implemented, fallback to level 2 )
 *      _ Driver: Use up to two waveforms
 *      _ Driver: Use _DUT info if at least one waveform
*/
enum class KIBIS_ACCURACY
{
    LEVEL_0,
    LEVEL_1,
    LEVEL_2,
    LEVEL_3,
};

class KIBIS_PARAMETER
{
public:
    IBIS_CORNER    m_Rpin = IBIS_CORNER::TYP;
    IBIS_CORNER    m_Lpin = IBIS_CORNER::TYP;
    IBIS_CORNER    m_Cpin = IBIS_CORNER::TYP;
    IBIS_CORNER    m_Ccomp = IBIS_CORNER::TYP;
    IBIS_CORNER    m_supply = IBIS_CORNER::TYP;
    KIBIS_WAVEFORM* m_waveform = nullptr;
    KIBIS_ACCURACY m_accuracy = KIBIS_ACCURACY::LEVEL_2;

    void SetCornerFromString( IBIS_CORNER& aCorner, const std::string& aString );
};


class KIBIS_FILE : KIBIS_BASE
{
public:
    KIBIS_FILE( KIBIS& aTopLevel );

    std::string m_fileName;
    double      m_fileRev;
    double      m_ibisVersion;
    std::string m_date;
    std::string m_source;
    std::string m_notes;
    std::string m_disclaimer;
    std::string m_copyright;

    bool Init( const IbisParser& aParser );
};

class KIBIS_SUBMODEL : public KIBIS_BASE
{
public:
    KIBIS_SUBMODEL( KIBIS& aTopLevel, const IbisSubmodel& aSource, IBIS_SUBMODEL_MODE aMode );

    std::string        m_name;
    IBIS_SUBMODEL_TYPE m_type = IBIS_SUBMODEL_TYPE::UNDEFINED;
    IBIS_SUBMODEL_MODE m_mode = IBIS_SUBMODEL_MODE::ALL;

    // Currently we only support static mode dynamic clamp submodels.
    IVtable m_GNDClamp;
    IVtable m_POWERClamp;

    /** @brief Return true if the model has a clamp diode to the gnd net */
    bool HasGNDClamp() const;
    /** @brief Return true if the model has a clamp diode to the power net */
    bool HasPOWERClamp() const;
};

class KIBIS_MODEL : public KIBIS_BASE
{
public:
    KIBIS_MODEL( KIBIS& aTopLevel, const IbisModel& aSource, IbisParser& aParser );

    std::string        m_name;
    std::string        m_description;
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

    TypMinMaxValue              m_C_comp;
    TypMinMaxValue              m_voltageRange;
    TypMinMaxValue              m_temperatureRange;
    TypMinMaxValue              m_pullupReference;
    TypMinMaxValue              m_pulldownReference;
    TypMinMaxValue              m_GNDClampReference;
    TypMinMaxValue              m_POWERClampReference;
    TypMinMaxValue              m_Rgnd;
    TypMinMaxValue              m_Rpower;
    TypMinMaxValue              m_Rac;
    TypMinMaxValue              m_Cac;
    IVtable                     m_GNDClamp;
    IVtable                     m_POWERClamp;
    IVtable                     m_pullup;
    IVtable                     m_pulldown;
    std::vector<IbisWaveform*>  m_risingWaveforms;
    std::vector<IbisWaveform*>  m_fallingWaveforms;
    IbisRamp                    m_ramp;
    std::vector<KIBIS_SUBMODEL> m_submodels;

    /** @brief Return true if the model has a pulldown transistor */
    bool HasPulldown() const;
    /** @brief Return true if the model has a pullup transistor */
    bool HasPullup() const;
    /** @brief Return true if the model has a clamp diode to the gnd net */
    bool HasGNDClamp() const;
    /** @brief Return true if the model has a clamp diode to the power net */
    bool HasPOWERClamp() const;

    /** @brief Generate the spice directive to simulate the die
     *
     *  @param aParam Parameters
     *  @param aIndex Index used to offset spice nodes / directives
     *  @param aDriver Generate a driver model when true, a device model when false
     *  @return A multiline string with spice directives
     */
    std::string SpiceDie( const KIBIS_PARAMETER& aParam, int aIndex, bool aDriver ) const;

    /** @brief Create waveform pairs
     *
     *  For maximum accuracy, we need a waveform pair.
     *  This function creates the pairs based on the fixture.
     *  The first element is the rising edge, the second is the falling edge.
     *
     *  @return a vector of waveform pairs
     */
    std::vector<std::pair<IbisWaveform*, IbisWaveform*>> waveformPairs();


    /** @brief Generate a square waveform
     *
     *  For maximum accuracy, we need a waveform pair.
     *  This function creates the pairs based on the fixture.
     *
     *  @param aNode1 node where the voltage is applied
     *  @param aNode2 Reference node
     *  @param aBits The first member is the bit value ( 1 or 0 ).
     *  The second member is the time of the transition edge.
     *  @param aPair @see waveformPairs()
     *  @param aParam Parameters
     *  @return A multiline string with spice directives
     */
    std::string generateSquareWave( const std::string& aNode1, const std::string& aNode2,
                                    const std::vector<std::pair<int, double>>&     aBits,
                                    const std::pair<IbisWaveform*, IbisWaveform*>& aPair,
                                    const KIBIS_PARAMETER&                         aParam );


    /** @brief Copy a waveform, and substract the first value to all samples
     *
     *
     * @param aIn Input waveform
     * @return Output waveform
     */
    IbisWaveform TrimWaveform( const IbisWaveform& aIn ) const;
};

class KIBIS_PIN : public KIBIS_BASE
{
public:
    KIBIS_PIN( KIBIS& aTopLevel, const IbisComponentPin& aPin, const IbisComponentPackage& aPackage,
               IbisParser& aParser, KIBIS_COMPONENT* aParent, std::vector<KIBIS_MODEL>& aModels );
    /** @brief Name of the pin
     *  Examples : "VCC", "GPIOA", "CLK", etc...
     */
    std::string m_signalName;
    /** @brief Pin Number
     * Examples : 1, 2, 3 ( or for BGA ), A1, A2, A3, etc...
     */
    std::string m_pinNumber;

    /** @brief Resistance from die to pin */
    TypMinMaxValue m_Rpin;
    /** @brief Inductance from die to pin */
    TypMinMaxValue m_Lpin;
    /** @brief Capacitance from pin to GND */
    TypMinMaxValue m_Cpin;

    KIBIS_COMPONENT* m_parent;

    std::vector<double> m_t, m_Ku, m_Kd;

    std::vector<KIBIS_MODEL*> m_models;

    bool writeSpiceDriver( std::string& aDest, const std::string& aName, KIBIS_MODEL& aModel,
                           const KIBIS_PARAMETER& aParam );
    bool writeSpiceDiffDriver( std::string& aDest, const std::string& aName, KIBIS_MODEL& aModel,
                               const KIBIS_PARAMETER& aParam );
    bool writeSpiceDevice( std::string& aDest, const std::string& aName, KIBIS_MODEL& aModel,
                           const KIBIS_PARAMETER& aParam );
    bool writeSpiceDiffDevice( std::string& aDest, const std::string& aName, KIBIS_MODEL& aModel,
                               const KIBIS_PARAMETER& aParam );

    /** @brief Update m_Ku, m_Kd using no falling / rising waveform inputs ( low accuracy )
     *  @param aModel Model to be used
     *  @param aParam Parameters
     */
    void getKuKdNoWaveform( KIBIS_MODEL& aModel, const KIBIS_PARAMETER& aParam );

    /** @brief Update m_Ku, m_Kd using with a single waveform input
     *  @param aModel Model to be used
     *  @param aPair @see waveformPairs()
     *  @param aParam Parameters
     */
    void getKuKdOneWaveform( KIBIS_MODEL&                                   aModel,
                             const std::pair<IbisWaveform*, IbisWaveform*>& aPair,
                             const KIBIS_PARAMETER&                         aParam );

    /** @brief Update m_Ku, m_Kd using with two waveform inputs
     *
     *  The order of aPair1 and aPair2 is not important.
     *  @param aModel Model to be used
     *  @param aPair1 @see waveformPairs()
     *  @param aPair2 @see waveformPairs()
     *  @param aParam Parameters
     *  @param aIndex Index for numbering spice .SUBCKT
     */
    void getKuKdTwoWaveforms( KIBIS_MODEL&                                   aModel,
                              const std::pair<IbisWaveform*, IbisWaveform*>& aPair1,
                              const std::pair<IbisWaveform*, IbisWaveform*>& aPair2,
                              const KIBIS_PARAMETER&                         aParam );

    /** @brief Update m_Ku, m_Kd using with two waveform inputs
     *
     *  The order of aPair1 and aPair2 is not important.
     *  @param aModel Model to be used
     *  @param aPair @see waveformPairs()
     *  @param aParam Parameters
     *  @param aIndex Index for numbering spice .SUBCKT
     *
     *  @return A multiline string with spice directives
     */
    std::string KuKdDriver( KIBIS_MODEL&                                   aModel,
                            const std::pair<IbisWaveform*, IbisWaveform*>& aPair,
                            const KIBIS_PARAMETER& aParam, int aIndex );

    /** @brief Generate the spice directive to simulate the die for Ku/Kd estimation
     *
     *  DO NOT use it in order to generate a model.
     *  It sole purpose is to run the internal simulation to get Ku/Kd
     *
     *  @param aModel Model to be used
     *  @param aParam Parameters
     *  @param aIndex Index for numbering ports
     *  @return A multiline string with spice directives
     */
    std::string addDie( KIBIS_MODEL& aModel, const KIBIS_PARAMETER& aParam, int aIndex );


    /** @brief Update m_Ku, m_Kd using with two waveform inputs
     *
     * Runs a simulation. The simulation creates a specific file with Ku/Kd values
     * This function then reads the output file and updates m_Ku / m_Kd.
     * This function probably needs a rewrite.
     *
     * @param aSimul The simulation to run, multiline spice directives
     */
    void getKuKdFromFile( const std::string& aSimul );

    KIBIS_PIN* m_complementaryPin = nullptr;

    bool isDiffPin() const { return m_complementaryPin != nullptr; };
};

class KIBIS_COMPONENT : public KIBIS_BASE
{
public:
    KIBIS_COMPONENT( KIBIS& aToplevel, const IbisComponent& aSource, IbisParser& aParser );
    /** @brief Name of the component */
    std::string m_name;
    /** @brief Name of the manufacturer */
    std::string m_manufacturer;

    std::vector<KIBIS_PIN> m_pins;

    /** @brief Get a pin by its number ( 1, 2, A1, A2, ... )
     *
     *  @param aPinNumber pin number
     *  @return pointer to a KIBIS_PIN, or nullptr if there is no matching pin
     */
    KIBIS_PIN* GetPin( const std::string& aPinNumber );
};

class KIBIS : public KIBIS_BASE
{
public:
    /** @brief Constructor for unitialized KIBIS members */
    KIBIS() :
            KIBIS_BASE( this, nullptr ),
            m_file( *this )
    {};

    KIBIS( const std::string& aFileName, REPORTER* aReporter = nullptr );

    std::vector<KIBIS_COMPONENT> m_components;
    std::vector<KIBIS_MODEL>     m_models;
    KIBIS_FILE                   m_file;

    /** @brief Absolute path of the directory that will be used for caching.  */
    std::string m_cacheDir = "";

    /** @brief Return the model with name aName . Nullptr if not found */
    KIBIS_MODEL* GetModel( const std::string& aName );
    /** @brief Return the component with name aName . Nullptr if not found */
    KIBIS_COMPONENT* GetComponent( const std::string& aName );
};

#endif
