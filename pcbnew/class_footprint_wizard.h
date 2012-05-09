/**
 * @file  pcbnew_footprint_wizards.h
 * @brief Class PCBNEW_FOOTPRINT_WIZARDS
 */

#ifndef CLASS_FOOTPRINT_WIZARD_H
#define	CLASS_FOOTPRINT_WIZARD_H
#include <vector>
#include <wxPcbStruct.h>

/* This is the parent class from where any footprint wiizard class must 
 * derive */
class FOOTPRINT_WIZARD
{

public:
    FOOTPRINT_WIZARD() {}
    ~FOOTPRINT_WIZARD() {}
    virtual wxString      GetName()=0;
    virtual wxString      GetImage()=0;  
    virtual wxString      GetDescription()=0;
    virtual int           GetNumParameterPages()=0;
    virtual wxString      GetParameterPageName(int aPage)=0;
    virtual wxArrayString GetParameterNames(int aPage)=0;
    virtual wxArrayString GetParameterValues(int aPage)=0;
    virtual wxString      SetParameterValues(int aPage,wxArrayString& aValues)=0;
    virtual MODULE       *GetModule()=0;
    void register_wizard();
        
};


class FOOTPRINT_WIZARDS 
{
private:
    static    std::vector<FOOTPRINT_WIZARD*>  m_FootprintWizards;

public:
    static void register_wizard(FOOTPRINT_WIZARD *wizard);

};

#endif	/* PCBNEW_FOOTPRINT_WIZARDS_H */

