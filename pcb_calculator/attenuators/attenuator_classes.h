/**
 * @file attenuator_classes.h
 */

/*
 *     Attenuator Synthesis
 *
 *     From Qucs
 *      Modified for KiCad
 */

#ifndef ATTENUATORFUNC_H
#define ATTENUATORFUNC_H

// Forward declare the bitmaps enum from bitmaps/bitmap_list.h
enum class BITMAPS : unsigned int;

class wxString;

enum ATTENUATORS_TYPE {
    PI_TYPE,
    TEE_TYPE,
    BRIDGE_TYPE,
    SPLITTER_TYPE
};

class ATTENUATOR
{
public:
    virtual ~ATTENUATOR();

    /**
     * Calculates the values of components in attenuator.
     *
     * @return true if OK, false if some values cannot be calculated.
     */
    virtual bool Calculate();

    /**
     * Read values stored in config for this attenuator.
     */
    void         ReadConfig();

    /**
     * Read values stored in config for this attenuator.
     */
    void         WriteConfig();

protected:
    // The constructor is protected, because this class is not intended to be instantiated.
    ATTENUATOR( ATTENUATORS_TYPE Topology );

public:
    wxString         m_Name;                // Identifier for configuration.
    int              m_ResultCount;         // Number of value to calculate, and therefore display
    bool             m_Error;               // Set to true if values cannot be calculated
    double           m_Zin;                 // Impedance of source
    bool             m_Zin_Enable;          // Set to true when impedance of source has meaning
    double           m_Zout;                // Impedance of load
    double           m_Attenuation;         // Attenuation in dB
    bool             m_Attenuation_Enable;  // Set to true when Attenuation has meaning
    double           m_MinimumATT;          // Minimum attenuation in dB from parameters
    double           m_R1;                  // value of R1
    double           m_R2;                  // value of R2
    double           m_R3;                  // value of R3 (if any)
    BITMAPS          m_SchBitmapName;       // The schema of this attenuator
    wxString*        m_FormulaName;         // The HTML/markdown text name of the formula

protected:
    ATTENUATORS_TYPE m_Topology;
    double           Lmin, L, A;            // internal variable for temporary use
};

class ATTENUATOR_PI : public ATTENUATOR
{
public:
    ATTENUATOR_PI();
    ~ATTENUATOR_PI(){};
    virtual bool Calculate() override;
};

class ATTENUATOR_TEE : public ATTENUATOR
{
public:
    ATTENUATOR_TEE();
    ~ATTENUATOR_TEE(){};
    virtual bool Calculate() override;
};

class ATTENUATOR_BRIDGE : public ATTENUATOR
{
public:
    ATTENUATOR_BRIDGE();
    ~ATTENUATOR_BRIDGE(){};
    virtual bool Calculate() override;
};

class ATTENUATOR_SPLITTER : public ATTENUATOR
{
public:
    ATTENUATOR_SPLITTER();
    ~ATTENUATOR_SPLITTER(){};
    virtual bool Calculate() override;
};

#endif  // ATTENUATORFUNC_H
