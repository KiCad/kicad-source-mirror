/****************************************************************************
**     From Qucs Attenuator Synthesis
**     attenuator_classes.cpp
**
**     since 2006/6/14
**
*****************************************************************************/
#include <cmath>

#include <attenuator_classes.h>

// Bitmaps:
#include <att_pi.xpm>
#include <att_tee.xpm>
#include <att_bridge.xpm>
#include <att_splitter.xpm>
#include <pi_formula.xpm>
#include <tee_formula.xpm>
#include <bridged_tee_formula.xpm>
#include <splitter_formula.xpm>

#ifndef NULL
#define NULL 0
#endif

ATTENUATOR::ATTENUATOR( ATTENUATORS_TYPE aTopology )
{
    m_Name        = wxT("att_base");
    m_Error       = false;
    m_Topology    = aTopology;
    m_ResultCount = 3;              // If 3 values must be calculated
    m_Zin = 50;                     // Ohms
    m_Zin_Enable = true;
    m_Zout = 50;                    // Ohms
    m_Attenuation = 6.0;            // dB
    m_Attenuation_Enable = true;
    m_MinimumATT    = 0.0;          // dB
    m_SchBitMap     = NULL;
    m_FormulaBitMap = NULL;
}


ATTENUATOR::~ATTENUATOR()
{
    delete m_SchBitMap;
    delete m_FormulaBitMap;
}


#define KEYWORD_ATTENUATOR_ATT  wxT( "Attenuation" )
#define KEYWORD_ATTENUATOR_ZIN  wxT( "Zin" )
#define KEYWORD_ATTENUATOR_ZOUT wxT( "Zout" )
#define KEYWORD_ATTENUATORS     wxT( "Attenuators/" )

void ATTENUATOR::ReadConfig( wxConfigBase* aConfig )
{
    aConfig->SetPath( KEYWORD_ATTENUATORS + m_Name );
    if( m_Attenuation_Enable )
        aConfig->Read( KEYWORD_ATTENUATOR_ATT, &m_Attenuation, 6.0 );
    aConfig->Read( KEYWORD_ATTENUATOR_ZIN, &m_Zin, 50.0 );
    aConfig->Read( KEYWORD_ATTENUATOR_ZOUT, &m_Zout, 50.0 );
    aConfig->SetPath( wxT( "../.." ) );
}


void ATTENUATOR::WriteConfig( wxConfigBase* aConfig )
{
    aConfig->SetPath( KEYWORD_ATTENUATORS + m_Name );
    aConfig->Write( KEYWORD_ATTENUATOR_ATT, m_Attenuation );
    aConfig->Write( KEYWORD_ATTENUATOR_ZIN, m_Zin );
    aConfig->Write( KEYWORD_ATTENUATOR_ZOUT, m_Zout );
    aConfig->SetPath( wxT( "../.." ) );
}


ATTENUATOR_PI::ATTENUATOR_PI() : ATTENUATOR( PI_TYPE )
{
    m_Name = wxT("att_pi");
    m_SchBitMap     = new wxBitmap( att_pi_xpm );
    m_FormulaBitMap = new wxBitmap( pi_formula_xpm );
}


bool ATTENUATOR_PI::Calculate()
{
    if( !ATTENUATOR::Calculate() )
        return false;

    m_R2 = ( (L - 1) / 2 ) * sqrt( m_Zin * m_Zout / L );
    m_R1 = 1 / ( ( (A / m_Zin) ) - (1 / m_R2) );
    m_R3 = 1 / ( ( (A / m_Zout) ) - (1 / m_R2) );

    return true;
}


ATTENUATOR_TEE::ATTENUATOR_TEE() : ATTENUATOR( TEE_TYPE )
{
    m_Name = wxT("att_tee");
    m_SchBitMap     = new wxBitmap( att_tee_xpm );
    m_FormulaBitMap = new wxBitmap( tee_formula_xpm );
}


bool ATTENUATOR_TEE::Calculate()
{
    if( !ATTENUATOR::Calculate() )
        return false;

    m_R2 = ( 2 * sqrt( L * m_Zin * m_Zout ) ) / (L - 1);
    m_R1 = m_Zin * A - m_R2;
    m_R3 = m_Zout * A - m_R2;

    return true;
}


ATTENUATOR_BRIDGE::ATTENUATOR_BRIDGE() : ATTENUATOR( BRIDGE_TYPE )
{
    m_Name = wxT("att_bridge");
    m_Zin_Enable    = false;
    m_ResultCount   = 2;
    m_SchBitMap     = new wxBitmap( att_bridge_xpm );
    m_FormulaBitMap = new wxBitmap( bridged_tee_formula_xpm );
}


bool ATTENUATOR_BRIDGE::Calculate()
{
    m_Zin = m_Zout;
    if( !ATTENUATOR::Calculate() )
        return false;

    L    = pow( 10, m_Attenuation / 20 );
    m_R1 = m_Zin * (L - 1);
    m_R2 = m_Zin / (L - 1);

    return true;
}


ATTENUATOR_SPLITTER::ATTENUATOR_SPLITTER() : ATTENUATOR( SPLITTER_TYPE )
{
    m_Name = wxT("att_splitter");
    m_Attenuation_Enable = false;
    m_Attenuation = 6.0;
    m_MinimumATT    = 6.0;
    m_Zin_Enable    = false;
    m_SchBitMap     = new wxBitmap( att_splitter_xpm );
    m_FormulaBitMap = new wxBitmap( splitter_formula_xpm );
}


bool ATTENUATOR_SPLITTER::Calculate()
{
    m_Attenuation = 6.0;
    m_Zin = m_Zout;
    m_R1  = m_R2 = m_R3 = m_Zout / 3.0;
    return true;
}


bool ATTENUATOR::Calculate()
{
    L = pow( 10, m_Attenuation / 10 );

    A = (L + 1) / (L - 1);

    if( m_Zin > m_Zout )
    {
        Lmin = (2 * m_Zin / m_Zout) - 1 + 2 *
               sqrt( m_Zin / m_Zout * (m_Zin / m_Zout - 1) );
    }
    else
    {
        Lmin = (2 * m_Zout / m_Zin) - 1 + 2 *
               sqrt( m_Zout / m_Zin * (m_Zout / m_Zin - 1) );
    }
    m_MinimumATT = 10 * log10( Lmin );

    if( m_MinimumATT > m_Attenuation )
    {
        m_Error = true;
        return false;
    }

    m_Error = false;
    return true;
}
