/**
 * @file  pcbnew_footprint_wizards.h
 * @brief Class PCBNEW_FOOTPRINT_WIZARDS
 */

#ifndef CLASS_FOOTPRINT_WIZARD_H
#define	CLASS_FOOTPRINT_WIZARD_H
#include <vector>
#include <wxPcbStruct.h>

/**
 * Class FOOTPRINT_WIZARD
 * This is the parent class from where any footprint wizard class must 
 * derive */
class FOOTPRINT_WIZARD
{

public:
    FOOTPRINT_WIZARD() {}
    ~FOOTPRINT_WIZARD() {}
    
    /** 
     * Function GetName
     * @return the name of the wizard
     */
    virtual wxString      GetName()=0;
    
    /**
     * Function GetImage
     * @return an svg image of the wizard to be rendered
     */
    virtual wxString      GetImage()=0;  
    
    /**
     * Function GetDescription
     * @return a description of the footprint wizard 
     */
    virtual wxString      GetDescription()=0;
    
    /** 
     * Function GetNumParameterPages
     * @return the number of parameter pages that this wizard will show to the user
     */
    virtual int           GetNumParameterPages()=0;
    
    /**
     * Function GetParameterPageName
     * @param aPage is the page we want the name of
     * @return a string with the page name
     */
    virtual wxString      GetParameterPageName(int aPage)=0;
    
    /**
     * Function GetParameterNames
     * @param aPage is the page we want the parameter names of
     * @return an array string with the parameter names on a certain page 
     */
    virtual wxArrayString GetParameterNames(int aPage)=0;
    
    /**
     * Function GetParameterValues
     * @param aPage is the page we want the parameter values of
     * @return an array of parameter values
     */
    virtual wxArrayString GetParameterValues(int aPage)=0;
    
    /**
     * Function GetParameterErrors
     * @param aPAge is the page we want to know the errors of
     * @return an array of errors (if any) for the parameters, empty strings for OK parameters
     */
    virtual wxArrayString GetParameterErrors(int aPage)=0;
    
    /**
     * Function SetParameterValues
     * @param aPage is the page we want to set the parameters in
     * @param aValues are the values we want to set into the parameters
     * @return an array of parameter values
     */
    virtual wxString      SetParameterValues(int aPage,wxArrayString& aValues)=0;
    
    /** 
     * Function GetModule
     * This method builds the module itself and returns it to the caller function 
     * @return  PCB module built from the parameters given to the class
     */
    virtual MODULE       *GetModule()=0;
    
    /**
     * Function register_wizard
     * It's the standard method of a "FOOTPRINT_WIZARD" to register itself into
     * the FOOTPRINT_WIZARDS singleton manager
     * 
     */
    void register_wizard();
        
};


class FOOTPRINT_WIZARDS 
{
private:
    static    std::vector<FOOTPRINT_WIZARD*>  m_FootprintWizards;

public:
    
    /**
     * Function register_wizard
     * A footprint wizard calls this static method when it wants to register itself
     * into the system wizards
     * 
     * @param aWizard is the footprint wizard to be registered
     * 
     */
    static void register_wizard(FOOTPRINT_WIZARD *wizard);
    
    /**
     * Function GetWizard
     * @return a wizard object by it's name or NULL if it isn't available.
     * 
     */
    static FOOTPRINT_WIZARD* GetWizard(wxString aName);
    
    /**
     * Function GetWizard
     * @return a wizard object by it's number or NULL if it isn't available.
     * 
     */
    static FOOTPRINT_WIZARD* GetWizard(int aIndex);
    
    /** 
     * Function GetSize
     * @return the number of wizards available into the system
     */
    static int GetSize();

};

#endif	/* PCBNEW_FOOTPRINT_WIZARDS_H */

