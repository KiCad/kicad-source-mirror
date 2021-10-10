/****************************************************************************
**     From Qucs Attenuator Synthesis
**     attenuator_classes.cpp
**
**     since 2006/6/14
**
*****************************************************************************/

#include <wx/bitmap.h>

#include <i18n_utility.h>
#include <kiface_base.h>
#include <bitmaps.h>

#include "attenuator_classes.h"
#include "pcb_calculator_settings.h"

// Html texts showing the formulas
wxString pi_formula =
#include "attenuators/pi_formula.h"


wxString tee_formula =
#include "attenuators/tee_formula.h"


wxString bridget_tee_formula =
#include "attenuators/bridget_tee_formula.h"


wxString splitter_formula =
#include "attenuators/splitter_formula.h"


ATTENUATOR::ATTENUATOR( ATTENUATORS_TYPE aTopology )
{
    m_Name               = wxT( "att_base" );
    m_Error              = false;
    m_Topology           = aTopology;
    m_ResultCount        = 3;           // If 3 values must be calculated
    m_Zin                = 50;          // Ohms
    m_Zin_Enable         = true;
    m_Zout               = 50;          // Ohms
    m_Attenuation        = 6.0;         // dB
    m_Attenuation_Enable = true;
    m_MinimumATT         = 0.0;         // dB
    m_SchBitmapName      = BITMAPS::INVALID_BITMAP;
    m_FormulaName        = nullptr;

    // Initialize these variables mainly to avoid warnings from a static analyzer
    m_R1 = 0.0;
    m_R2 = 0.0;
    m_R3 = 0.0;
    Lmin = L = A = 0.0;     // internal variable for temporary use
}


ATTENUATOR::~ATTENUATOR()
{
}


void ATTENUATOR::ReadConfig()
{
    auto cfg = static_cast<PCB_CALCULATOR_SETTINGS*>( Kiface().KifaceSettings() );
    std::string name = m_Name.ToStdString();

    wxASSERT( cfg->m_Attenuators.attenuators.count( name ) );

    m_Attenuation = cfg->m_Attenuators.attenuators.at( name ).attenuation;
    m_Zin         = cfg->m_Attenuators.attenuators.at( name ).zin;
    m_Zout        = cfg->m_Attenuators.attenuators.at( name ).zout;
}


void ATTENUATOR::WriteConfig()
{
    auto cfg = static_cast<PCB_CALCULATOR_SETTINGS*>( Kiface().KifaceSettings() );
    std::string name = m_Name.ToStdString();

    cfg->m_Attenuators.attenuators[ name ].attenuation = m_Attenuation;
    cfg->m_Attenuators.attenuators[ name ].zin         = m_Zin;
    cfg->m_Attenuators.attenuators[ name ].zout        = m_Zout;
}


ATTENUATOR_PI::ATTENUATOR_PI() : ATTENUATOR( PI_TYPE )
{
    m_Name          = wxT( "att_pi" );
    m_SchBitmapName = BITMAPS::att_pi;
    m_FormulaName   = &pi_formula;
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
    m_Name          = wxT( "att_tee" );
    m_SchBitmapName = BITMAPS::att_tee;
    m_FormulaName   = &tee_formula;
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
    m_Name          = wxT( "att_bridge" );
    m_Zin_Enable    = false;
    m_ResultCount   = 2;
    m_SchBitmapName = BITMAPS::att_bridge;
    m_FormulaName   = &bridget_tee_formula;
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
    m_Name               = wxT( "att_splitter" );
    m_Attenuation_Enable = false;
    m_Attenuation        = 6.0;
    m_MinimumATT         = 6.0;
    m_Zin_Enable         = false;
    m_SchBitmapName      = BITMAPS::att_splitter;
    m_FormulaName        = &splitter_formula;
}


bool ATTENUATOR_SPLITTER::Calculate()
{
    m_Attenuation = 6.0;
    m_Zin         = m_Zout;
    m_R1          = m_R2 = m_R3 = m_Zout / 3.0;
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
