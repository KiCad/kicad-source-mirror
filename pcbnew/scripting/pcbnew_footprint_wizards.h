/**
 * @file  pcbnew_footprint_wizards.h
 * @brief Class PCBNEW_FOOTPRINT_WIZARDS
 */

#ifndef PCBNEW_FOOTPRINT_WIZARDS_H
#define	PCBNEW_FOOTPRINT_WIZARDS_H
#include <Python.h>
#include <vector>
#include <wxPcbStruct.h>

class FOOTPRINT_WIZARD
{

    PyObject *py_wizard;
    PyObject *CallMethod(const char *aMethod, PyObject *aArglist=NULL);
    wxString CallRetStrMethod(const char *aMethod, PyObject *aArglist=NULL);
public:
    FOOTPRINT_WIZARD(PyObject *wizard);
    ~FOOTPRINT_WIZARD();
    wxString      GetName();
    wxString      GetImage();  
    wxString      GetDescription();
    int           GetNumParameterPages();
    wxString      GetParameterPageName(int aPage);
    wxArrayString GetParameterNames(int aPage);
    wxArrayString GetParameterValues(int aPage);
    wxString      SetParameterValues(int aPage,wxArrayString& aValues);
    MODULE       *GetModule();
        
};


class FOOTPRINT_WIZARDS 
{
private:
    static    std::vector<FOOTPRINT_WIZARD*>  m_FootprintWizards;

public:
    static void register_wizard(PyObject *wizard);

};

#endif	/* PCBNEW_FOOTPRINT_WIZARDS_H */

