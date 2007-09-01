#include "wx/wxprec.h"
#include <id.h>

#ifdef __BORLANDC__
#pragma hdrstop
#endif

// for all others, include the necessary headers (this file is usually all you
// need because it includes almost all "standard" wxWindows headers
#ifndef WX_PRECOMP
#include <wx/wx.h>
#endif

#include <wx/dir.h>
#include <wx/utils.h>

#include <pyhandler.h>
#include <iostream>

#include "fctsys.h"
#include "common.h"

using namespace boost::python;


/*****************************************************************************/
/*                        Common Python Binding                              */
/*****************************************************************************/

static int GetLastID() { return ID_END_LIST; }

static object ChooseFile( str objTitle, str objMask, object objOpen )
{
    wxString mask = PyHandler::MakeStr( objMask );
    int open  = extract<int>( objOpen );

    wxString script = EDA_FileSelector( PyHandler::MakeStr( objTitle ),
            wxEmptyString,		  		/* Chemin par defaut */
            wxEmptyString,				/* nom fichier par defaut */
            mask,			            /* extension par defaut */
            mask,                       /* Masque d'affichage */
            NULL,
            open ? wxFD_OPEN : wxFD_SAVE,
            TRUE
            );

    return PyHandler::Convert( script );
}

static void Print( str message ) { std::cout << extract<char *>(message) << std::endl; }
static void Clear() {}

static void RegisterCb( str objKey, object callback )
{ PyHandler::GetInstance()->RegisterCallback( PyHandler::MakeStr(objKey), callback ); }
static void UnRegisterCb( str objKey, object callback )
{ PyHandler::GetInstance()->UnRegisterCallback( PyHandler::MakeStr(objKey), callback ); }

static void init_base_utils(void)
{
    def ( "ChooseFile",         &ChooseFile );
    def ( "RegisterCallback",   &RegisterCb );
    def ( "UnRegisterCallback", &UnRegisterCb );
    def ( "GetLastID",          &GetLastID );

    def ( "Print",              &Print );
    def ( "Clear",              &Clear);
}

static void InitPyModules() { PyHandler::GetInstance()->InitNextModule(); } // Dummy boost callback

/*****************************************************************************/
/*                                PyHandler                                  */
/*****************************************************************************/

// std::vector< T > -> python object implicit conversion
template <typename T> struct std_vector_to_tuple
{
	static PyObject * makeItem( const wxString & str ) { return boost::python::incref( PyHandler::Convert( str ).ptr() ); }
	static PyObject * makeItem( const std::string & str ) { return boost::python::incref( boost::python::str( str.c_str() ).ptr() ); }
	static PyObject * makeItem( int item ) { return boost::python::incref( PyInt_FromLong( item ) ); }

	static PyObject * convert( const T& vect )
	{
		PyObject * tuple = PyTuple_New( vect.size() );
		for ( unsigned int i = 0; i < vect.size() ; i++ )
		{
			PyTuple_SET_ITEM( tuple, i, makeItem( vect[i] ) );
		}
		return tuple;
	}
};

PyHandler* PyHandler::m_instance = NULL;

PyHandler * PyHandler::GetInstance()
/* Singleton implementation */
{
	if ( !PyHandler::m_instance )
	{
		PyHandler::m_instance = new PyHandler();
	}
	return PyHandler::m_instance;
}

PyHandler::PyHandler()
/* Init the Python env */
{
    Py_Initialize();
    PyEval_InitThreads();
    m_ModulesLoaded = false;
    m_current = 0;
    if ( !wxPyCoreAPI_IMPORT() )
    {
        std::cerr << "Can't get wx Python binding\n" ;
        PyErr_Print();
    }
//    m_mainTState = wxPyBeginAllowThreads(); // I can't figure out why this make py crash ...
    m_mainTState = NULL;

    // Make the console appear in a window:
    wxString initConsole;
    initConsole += wxT( "import sys\n" );
    initConsole += wxT( "import wx\n" );
    initConsole += wxT( "output = wx.PyOnDemandOutputWindow()\n" );
    initConsole += wxT( "sys.stdout = sys.stderr = output\n" );
    RunSimpleString( initConsole );

    AddToModule  ( wxT( "common" ), &init_base_utils );

    // Register converters

    to_python_converter < std::vector< std::string >, std_vector_to_tuple< const std::vector < std::string > >  > ();
    to_python_converter < std::vector< wxString >, std_vector_to_tuple< const std::vector < wxString > >  > ();
}

void PyHandler::DoInitModules()
{
    if ( m_ModulesLoaded ) return;
    m_ModulesLoaded = true;

    for ( unsigned int i = 0; i < m_ModuleRegistry.size(); i ++ )
    {
        detail::init_module( m_ModuleRegistry[i].name.fn_str(), &InitPyModules );
    }
}

int PyHandler::GetModuleIndex( const wxString & name ) const
/* Returns the module index in the registry, -1 if not found*/
{
    for ( unsigned int i = 0; i < m_ModuleRegistry.size(); i ++ )
    {
        if ( m_ModuleRegistry[i].name == name ) return i;
    }
    return -1;
}

void PyHandler::AddToModule( const wxString & name, PyHandler::initfunc_t initfunc )
/* Adds an init function to a python module */
{
    if (!initfunc) return;
    int i = GetModuleIndex( name );

    if ( -1 == i )
    {
        m_ModuleRegistry.push_back( ModuleRecord( name ) );
        i = m_ModuleRegistry.size() - 1;
    }

    m_ModuleRegistry[i].registry.push_back( initfunc );
}


void PyHandler::InitNextModule()
/* Called to initialize a module on py 'import module' */
{
    for ( unsigned int j = 0; j < m_ModuleRegistry[m_current].registry.size() ; j ++ )
    {
        m_ModuleRegistry[m_current].registry[j]();
    }
    m_current++;
}

PyHandler::~PyHandler()
/* Closes the Python env */
{
    wxPyEndAllowThreads(m_mainTState);
	Py_Finalize();
}

void PyHandler::RunBaseScripts( const wxString & base )
/* Run scripts looking in 'base' directory */
{
    const wxString sep = wxFileName().GetPathSeparator();

	// check if we can have a kicad_startup.py around ?
	wxString script = base + wxT( "scripts" ) + sep + wxT( "kicad_startup.py" );
	if ( wxFileExists( script ) ) RunScript( script );

	// First find scripts/<name>.py and run it if found :

	script = base + wxString::FromAscii( "scripts" ) + sep + m_appName + wxString::FromAscii(".py");
	if ( wxFileExists(  script ) ) RunScript( script );

	// Now lets see if we can find a suitable plugin directory (plugin/<name>) somewhere

	wxString pluginDir = base + wxT( "plugins" ) + sep + m_appName;
	if ( wxDirExists( pluginDir ) )
	{
		// We do have a systemwide plugin dir, let's find files in it
		wxArrayString pluginList;
		wxDir::GetAllFiles( pluginDir, &pluginList, wxT("*.py") );

		for ( unsigned int i = 0; i < pluginList.Count() ; i++ )
		{
			RunScript( pluginList[i] );
		}
	}
}

void PyHandler::RunScripts()
/* Run application startup scripts */
{
	// SYSTEMWIDE:

    const wxString sep = wxFileName().GetPathSeparator();

	wxString dataPath = ReturnKicadDatasPath();
	if ( wxDirExists( dataPath ) )	RunBaseScripts( dataPath );

	// USER Scripts:
	wxString userDir = wxGetUserHome() + sep + wxString::FromAscii(".kicad.d") + sep;
	if ( wxDirExists( userDir ) ) RunBaseScripts( userDir );
	userDir = wxGetUserHome() + sep + wxString::FromAscii("_kicad_d") + sep;
	if ( wxDirExists( userDir ) ) RunBaseScripts( userDir );

}

bool PyHandler::RunScript( const wxString & name )
/* Run the script specified by 'name' */
{
    DoInitModules();

    object module( handle<>(borrowed(PyImport_AddModule("__main__"))));
    object ns = module.attr( "__dict__" );
    bool ret = true;

    FILE * file = fopen( name.fn_str(), "r" );

    wxPyBlock_t blocked = wxPyBeginBlockThreads();

    if ( !file )
    {
    	// do something
    	std::cout << "Unable to Load " << name.fn_str()  << "\n";
    	ret = false;
    }
    else
    {
        wxString currDir = wxGetCwd();

        wxFileName fname( name );
        wxString pyDir = fname.GetPath();

        wxSetWorkingDirectory( pyDir );
        try
        {
            ns["currentScript"] = Convert( name );
            handle<> ignored( PyRun_File( file, name.fn_str(), Py_file_input,  ns.ptr(), ns.ptr() ) );
        }
        catch ( error_already_set )
        {
            PyErr_Print(); // should be printed into an error message ...
            ret = false;
        }
        wxSetWorkingDirectory( currDir );
    }

	fclose( file );
    wxPyEndBlockThreads(blocked);
	return ret;
}

bool PyHandler::RunSimpleString( const wxString & code )
/* Run the code in 'code' */
{
    wxPyBlock_t blocked = wxPyBeginBlockThreads();
	try
	{
    	PyRun_SimpleString( code.fn_str() );
	}
	catch ( error_already_set )
	{
		PyErr_Print(); // should be printed into an error message ...
        wxPyEndBlockThreads(blocked);
		return false;
	}

    wxPyEndBlockThreads(blocked);
	return true;
}


void PyHandler::SetAppName( const wxString & name )
/* Set the application name in the python scope */
{
	m_appName = name;
    object module(( handle<>(borrowed(PyImport_AddModule("__main__")))));
    object ns = module.attr( "__dict__" );
	try {
		ns["kicadApp"] = std::string( name.ToAscii() );
	}
	catch (error_already_set)
	{
		PyErr_Print();
	}
}

const char * PyHandler::GetVersion() { return Py_GetVersion(); }

// Event handling :

void PyHandler::DeclareEvent( const wxString & key ) { m_EventRegistry.push_back( Event( key ) ); }

int PyHandler::GetEventIndex( const wxString & key )
{
    for ( unsigned int i = 0; i < m_EventRegistry.size(); i ++ )
    {
        if ( m_EventRegistry[i].key == key ) return i;
    }
    return -1;
}

void PyHandler::TriggerEvent( const wxString & key ) { TriggerEvent( key, str( "" ) ); }
void PyHandler::TriggerEvent( const wxString & key, const object & param )
{

    int i = GetEventIndex( key );
    if ( -1 == i ) return;

    wxPyBlock_t blocked = wxPyBeginBlockThreads();
    for ( unsigned int j = 0; j < m_EventRegistry[i].functors.size(); j++ )
    {
        try
        {
            m_EventRegistry[i].functors[j]( param );
        }
        catch (error_already_set)
        {
            std::cout << "Error in event " << key.fn_str() << " callback" << std::endl;
            PyErr_Print();
        }
    }
    wxPyEndBlockThreads(blocked);
}

void PyHandler::RegisterCallback( const wxString & key, const object & callback )
{
    int i = GetEventIndex( key );
    if ( -1 == i ) return;
    m_EventRegistry[i].functors.push_back( callback );
}

void PyHandler::UnRegisterCallback( const wxString & key, const object & callback )
{
    int i = GetEventIndex( key );
    if ( -1 == i ) return;
    for ( unsigned int j = 0; j < m_EventRegistry[i].functors.size() ; j++ )
    {
        if ( callback == m_EventRegistry[i].functors[j] )
        {
            m_EventRegistry[i].functors.erase( m_EventRegistry[i].functors.begin() + j );
            break;
        }
    }
}

// Object conversion:

wxString PyHandler::MakeStr( const object & objStr ) { return wxString( extract<const char *>( objStr ), wxConvLocal ); }

object PyHandler::Convert( const wxString & wxStr ) { return str( std::string( wxStr.fn_str() ).c_str() ); }

// vim: set tabstop=4 :
