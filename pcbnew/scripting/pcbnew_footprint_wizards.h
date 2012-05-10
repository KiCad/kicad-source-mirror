/**
 * @file  pcbnew_footprint_wizards.h
 * @brief Class PCBNEW_FOOTPRINT_WIZARDS
 */

#ifndef PCBNEW_FOOTPRINT_WIZARDS_H
#define	PCBNEW_FOOTPRINT_WIZARDS_H
#include <Python.h>
#include <vector>
#include <class_footprint_wizard.h>



class PYTHON_FOOTPRINT_WIZARD: public FOOTPRINT_WIZARD
{

    PyObject *m_PyWizard;
    PyObject *CallMethod(const char *aMethod, PyObject *aArglist=NULL);
    wxString CallRetStrMethod(const char *aMethod, PyObject *aArglist=NULL);
    wxArrayString CallRetArrayStrMethod(const char *aMethod, 
                                          PyObject *aArglist=NULL);

public:
    PYTHON_FOOTPRINT_WIZARD(PyObject *wizard);
    ~PYTHON_FOOTPRINT_WIZARD();
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


class PYTHON_FOOTPRINT_WIZARDS 
{
public:
    static void register_wizard(PyObject *aPyWizard);

};

#endif	/* PCBNEW_FOOTPRINT_WIZARDS_H */

