/**
 * @file  class_footprint_wizard.cpp
 * @brief Class FOOTPRINT_WIZARD and FOOTPRINT_WIZARDS
 */

#include "class_footprint_wizard.h"
#include <stdio.h>

void FOOTPRINT_WIZARD::register_wizard()
{
    FOOTPRINT_WIZARDS::register_wizard( this );
}

/**
 * FOOTPRINT_WIZARD system wide static list
 */
std::vector<FOOTPRINT_WIZARD*>  FOOTPRINT_WIZARDS::m_FootprintWizards;


FOOTPRINT_WIZARD* FOOTPRINT_WIZARDS::GetWizard( int aIndex )
{
    return m_FootprintWizards[aIndex];
}

FOOTPRINT_WIZARD* FOOTPRINT_WIZARDS::GetWizard( wxString aName )
{
    int max = GetSize();
    
    for( int i=0; i<max; i++ )
    {
        FOOTPRINT_WIZARD *wizard =  GetWizard( i );
        
        wxString name = wizard->GetName();
        
        if ( name.Cmp( aName ) )
                return wizard;     
    }
   
    return NULL;
}

int FOOTPRINT_WIZARDS::GetSize()
{
    return m_FootprintWizards.size();
}

void FOOTPRINT_WIZARDS::register_wizard(FOOTPRINT_WIZARD *aWizard)
{
    
    wxString name = aWizard->GetName(); 
    m_FootprintWizards.push_back( aWizard );
     
}




