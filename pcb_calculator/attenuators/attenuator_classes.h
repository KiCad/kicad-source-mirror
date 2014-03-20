/**
 *     @file attenuator_classes.h
 */

/*
 *     Attenuator Synthesis
 *
 *     From Qucs
 *      Modified for Kicad
 */

#ifndef ATTENUATORFUNC_H
#define ATTENUATORFUNC_H

#include <wx/config.h>
#include <wx/bitmap.h>

enum ATTENUATORS_TYPE {
    PI_TYPE,
    TEE_TYPE,
    BRIDGE_TYPE,
    SPLITTER_TYPE
};

class ATTENUATOR
{
protected:
    ATTENUATORS_TYPE m_Topology;
public:
    wxString         m_Name;                // Identifier for config
    int m_ResultCount;                      // Number of value to calculate, and therefore display
    bool             m_Error;               // Set to true if values acnnot be calculated
    double           m_Zin;                 // Impedance of source
    bool             m_Zin_Enable;          // Set to true when impedance of source has meaning
    double           m_Zout;                // Impedance of load
    double           m_Attenuation;         // Attenuation in dB
    bool             m_Attenuation_Enable;  // Set to true when Attenuatiopn has meaning
    double           m_MinimumATT;          // Minimun attenuation in dB from parameters
    double           m_R1;                  // value of R1
    double           m_R2;                  // value of R2
    double           m_R3;                  // value of R3 (if any)
    wxBitmap*        m_SchBitMap;           // The schema of this attenuator
    wxBitmap*        m_FormulaBitMap;       // The formula used to calcualte this attenuator

protected:
    double           Lmin, L, A; // internal variable for temporary use


protected:
    // The constructor is protected, because this class is not intended to be instancied
    ATTENUATOR( ATTENUATORS_TYPE Topology );
public:
    virtual ~ATTENUATOR();

    /**
     * Function Calculate
     * calculates the values of components in attenuator
     * @return true if ok, false if some values cannot be calculated
     */
    virtual bool Calculate();

    /**
     * Function ReadConfig
     * Read values stored in config for this attenuator
     * @param aConfig = the config to use
     */
    void         ReadConfig( wxConfigBase* aConfig );

    /**
     * Function WriteConfig
     * Read values stored in config for this attenuator
     * @param aConfig = the config to use
     */
    void         WriteConfig( wxConfigBase* aConfig );
};

class ATTENUATOR_PI : public ATTENUATOR
{
public:
    ATTENUATOR_PI();
    ~ATTENUATOR_PI(){};
    virtual bool Calculate();
};

class ATTENUATOR_TEE : public ATTENUATOR
{
public:
    ATTENUATOR_TEE();
    ~ATTENUATOR_TEE(){};
    virtual bool Calculate();
};

class ATTENUATOR_BRIDGE : public ATTENUATOR
{
public:
    ATTENUATOR_BRIDGE();
    ~ATTENUATOR_BRIDGE(){};
    virtual bool Calculate();
};

class ATTENUATOR_SPLITTER : public ATTENUATOR
{
public:
    ATTENUATOR_SPLITTER();
    ~ATTENUATOR_SPLITTER(){};
    virtual bool Calculate();
};

#endif  // ATTENUATORFUNC_H
