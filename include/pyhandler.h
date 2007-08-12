/****************************/
/*		  pyhandler.h		*/
/****************************/

#ifndef PYHANDLER_H
#define PYHANDLER_H

#include <wx/string.h>

#include <Python.h>
#include <wx/wxPython/wxPython.h>
#include <vector>

/* Use the boost library : */
#include <boost/python.hpp>


class PyHandler
{
    typedef void (*initfunc_t )();

private:
    static PyHandler*   m_instance;
    bool                m_ModulesLoaded;
    int                 m_current;
    PyThreadState*      m_mainTState;

protected:
    PyHandler();

    wxString m_appName;
    void RunBaseScripts( const wxString& base );

    // Modules
    struct ModuleRecord
    {
        wxString                  name;
        std::vector< initfunc_t > registry;

        ModuleRecord( const wxString &modName ) : 
            name( modName ) 
        {
        }

    };
    
    std::vector< ModuleRecord > m_ModuleRegistry;
    
    void DoInitModules();

    // Events
    struct Event
    {
        wxString                             key;
        std::vector< boost::python::object > functors;

        Event( const wxString &strKey ) : 
            key( strKey ) 
        {
        }
    };
    
    std::vector< Event > m_EventRegistry;

public:

    // Singletton handling:
    static PyHandler*               GetInstance();

    ~PyHandler();

    // Scope params/handling:
    void                            SetAppName( const wxString& name );

    void                            AddToModule( const wxString& name, initfunc_t initfunc );
    int                             GetModuleIndex( const wxString& name ) const;

    // Script and direct call
    void                            RunScripts();
    bool                            RunScript( const wxString& name );
    bool                            RunSimpleString( const wxString& code );

    // Common Informations
    const char*                     GetVersion();

    void                            InitNextModule();

    // Event triggering

    // - C++ interface
    void                            DeclareEvent( const wxString& key );
    void                            TriggerEvent( const wxString& key );
    void                            TriggerEvent( const wxString& key,
                                                  const boost::python::object& param );
    int                             GetEventIndex( const wxString& key );

    // - Py Interface
    void                            RegisterCallback( const wxString& key,
                                                      const boost::python::object& obj );
    void                            UnRegisterCallback( const wxString& key,
                                                        const boost::python::object& obj );

    // Object conversions

    // - Py -> C++
    static wxString                 MakeStr( const boost::python::object& objStr );

    // - C++ -> Py
    static boost::python::object    Convert( const wxString& wxStr );
};


#define KICAD_PY_BIND_MODULE( mod ) PyHandler::GetInstance()->AddModule( init # mod )

#endif  //PYHANDLER_H
