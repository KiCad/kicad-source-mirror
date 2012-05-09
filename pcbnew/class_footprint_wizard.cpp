/**
 * @file  class_footprint_wizard.cpp
 * @brief Class FOOTPRINT_WIZARD and FOOTPRINT_WIZARDS
 */

#include "class_footprint_wizard.h"
#include <stdio.h>

void FOOTPRINT_WIZARD::register_wizard()
{
    FOOTPRINT_WIZARDS::register_wizard(this);
}

std::vector<FOOTPRINT_WIZARD*>  FOOTPRINT_WIZARDS::m_FootprintWizards;

void FOOTPRINT_WIZARDS::register_wizard(FOOTPRINT_WIZARD *aWizard)
{
    
    wxString name = aWizard->GetName(); 
    m_FootprintWizards.push_back(aWizard);
    
    printf("Registered footprint wizard '%s'\n",(const char*)name.mb_str() );

#if 0 
    /* just to test if it works correctly */
    int pages = fw->GetNumParameterPages();
    printf("             %d pages\n",pages);
    
    for (int n=0; n<pages; n++)
    {
        printf("                      page %d->'%s'\n",n,
            (const char*)fw->GetParameterPageName(n).mb_str());
    }
#endif    
    
    
}




