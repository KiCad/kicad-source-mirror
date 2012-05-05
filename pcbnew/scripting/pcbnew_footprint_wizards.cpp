/**
 * @file  pcbnew_footprint_wizards.cpp
 * @brief Class PCBNEW_FOOTPRINT_WIZARDS
 */

#include "pcbnew_footprint_wizards.h"
#include <stdio.h>

FOOTPRINT_WIZARD::FOOTPRINT_WIZARD(PyObject *aWizard)
{
    this->py_wizard = aWizard;
    Py_XINCREF(aWizard);    
}

FOOTPRINT_WIZARD::~FOOTPRINT_WIZARD()
{
    Py_XDECREF(this->py_wizard);
}

PyObject* FOOTPRINT_WIZARD::CallMethod(const char* aMethod, PyObject *aArglist)
{
     PyObject *pFunc;
    
    /* pFunc is a new reference to the desired method */
    pFunc = PyObject_GetAttrString(this->py_wizard, aMethod);
    
    if (pFunc && PyCallable_Check(pFunc)) 
    {
        PyObject *result;
        
        result = PyObject_CallObject(pFunc, aArglist);
        
        if (PyErr_Occurred())
        {
            PyObject *t, *v, *b;
            PyErr_Fetch(&t, &v, &b);
            printf ("calling %s()\n",aMethod);
            printf ("Exception: %s\n",PyString_AsString(PyObject_Str(v)));
            printf ("         : %s\n",PyString_AsString(PyObject_Str(b)));
        }
        
        
        if (result)
        {
                Py_XDECREF(pFunc);
                return result;
        }
        
    }
    else
    {
        printf("method not found, or not callable: %s\n",aMethod);
    }
    
    if (pFunc) Py_XDECREF(pFunc);
    
    return NULL;
}

wxString FOOTPRINT_WIZARD::CallRetStrMethod(const char* aMethod, PyObject *aArglist)
{
    wxString ret;
    PyObject *result = CallMethod(aMethod,aArglist);
    if (result)
    {
         const char *str_res = PyString_AsString(result);
         ret  = wxString::FromUTF8(str_res);
         Py_DECREF(result);    
    }
    else
    {
        printf("method not found, or not callable: %s\n",aMethod);
    }
    
    return ret;
}
wxString FOOTPRINT_WIZARD::GetName()
{
    return CallRetStrMethod("GetName");
}

wxString FOOTPRINT_WIZARD::GetImage()
{
    return CallRetStrMethod("GetImage");
}

wxString FOOTPRINT_WIZARD::GetDescription()
{
    return CallRetStrMethod("GetDescription");
}

int FOOTPRINT_WIZARD::GetNumParameterPages()
{
    int ret;
    PyObject *result;
    
    /* Time to call the callback */
    result = CallMethod("GetNumParameterPages",NULL);
   
    if (result)
    {
         if (!PyInt_Check(result)) return -1;
         ret = PyInt_AsLong(result); 
         Py_DECREF(result);    
    }
    return ret;
}

wxString FOOTPRINT_WIZARD::GetParameterPageName(int aPage)
{
    wxString ret;
    PyObject *arglist;
    PyObject *result;
    
    /* Time to call the callback */
    arglist = Py_BuildValue("(i)", aPage);
    result = CallMethod("GetParameterPageName",arglist);
    Py_DECREF(arglist);
    
    if (result)
    {
         const char *str_res = PyString_AsString(result);
         ret  = wxString::FromUTF8(str_res);
         Py_DECREF(result);    
    }
    return ret;
}

wxArrayString FOOTPRINT_WIZARD::GetParameterNames(int aPage)
{
    wxArrayString a;
    return a;
}

wxArrayString FOOTPRINT_WIZARD::GetParameterValues(int aPage)
{
    wxArrayString a;
    return a;
}

wxString FOOTPRINT_WIZARD::SetParameterValues(int aPage,wxArrayString& aValues)
{
    wxString ret;
    return ret;
}

MODULE FOOTPRINT_WIZARD::*GetModule()
{
    return NULL;
}

std::vector<FOOTPRINT_WIZARD*>  FOOTPRINT_WIZARDS::m_FootprintWizards;

void FOOTPRINT_WIZARDS::register_wizard(PyObject* wizard)
{
    FOOTPRINT_WIZARD *fw;
    
    fw = new FOOTPRINT_WIZARD(wizard);
    m_FootprintWizards.push_back(fw);
    
    printf("Registered python footprint wizard '%s'\n",
            (const char*)fw->GetName().mb_str());
    
    int pages = fw->GetNumParameterPages();
    printf("             %d pages\n",pages);
    
    for (int n=0; n<pages; n++)
    {
        printf("                      page %d->'%s'\n",n,
            (const char*)fw->GetParameterPageName(n).mb_str());
    }
    
    
    
}




