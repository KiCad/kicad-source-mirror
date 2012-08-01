/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2012 Jean-Pierre Charras, jean-pierre.charras@ujf-grenoble.fr
 * Copyright (C) 2012 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright (C) 1992-2012 KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

/**
 * @file pcbnew.cpp
 * @brief Pcbnew main program.
 */

#ifdef KICAD_SCRIPTING
#include <pcbnew_scripting_helpers.h>
#endif
#include <fctsys.h>
#include <appl_wxstruct.h>
#include <confirm.h>
#include <macros.h>
#include <class_drawpanel.h>
#include <wxPcbStruct.h>
#include <eda_dde.h>
#include <pcbcommon.h>
#include <colors_selection.h>
#include <gr_basic.h>

#include <wx/file.h>
#include <wx/snglinst.h>

#include <pcbnew.h>
#include <protos.h>
#include <hotkeys.h>
#include <wildcards_and_files_ext.h>
#include <class_board.h>

#include <dialogs/dialog_scripting.h>

#ifdef KICAD_SCRIPTING
#include <python_scripting.h>
#endif


// Colors for layers and items
COLORS_DESIGN_SETTINGS g_ColorsSettings;

bool           Drc_On = true;
bool           g_AutoDeleteOldTrack = true;
bool           g_Drag_Pistes_On;
bool           g_Show_Module_Ratsnest;
bool           g_Raccord_45_Auto = true;
bool 	       g_Alternate_Track_Posture = false;
bool           g_Track_45_Only_Allowed = true;  // True to allow horiz, vert. and 45deg only tracks
bool           Segments_45_Only;                // True to allow horiz, vert. and 45deg only graphic segments
bool           g_TwoSegmentTrackBuild = true;

int            Route_Layer_TOP;
int            Route_Layer_BOTTOM;
int            g_MaxLinksShowed;
int            g_MagneticPadOption   = capture_cursor_in_track_tool;
int            g_MagneticTrackOption = capture_cursor_in_track_tool;

wxPoint        g_Offset_Module;     /* Distance to offset module trace when moving. */

/* Name of the document footprint list
 * usually located in share/modules/footprints_doc
 * this is of the responsibility to users to create this file
 * if they want to have a list of footprints
 */
wxString g_DocModulesFileName = wxT( "footprints_doc/footprints.pdf" );

wxArrayString g_LibraryNames;

// wxWindow* DoPythonStuff(wxWindow* parent); // declaration

IMPLEMENT_APP( EDA_APP )


/* MacOSX: Needed for file association
 * http://wiki.wxwidgets.org/WxMac-specific_topics
 */
void EDA_APP::MacOpenFile( const wxString& fileName )
{
    wxFileName      filename = fileName;
    PCB_EDIT_FRAME* frame    = ( (PCB_EDIT_FRAME*) GetTopWindow() );

    if( !filename.FileExists() )
        return;

    frame->LoadOnePcbFile( fileName, false );
}

#ifdef KICAD_SCRIPTING_EXPERIMENT

class MyFrame : public wxFrame	{
	public:
	    MyFrame(const wxString& title, const wxPoint& pos, const wxSize& size);
	    void RedirectStdio();
	    wxWindow* DoPythonStuff(wxWindow* parent);
	    void OnExit(wxCommandEvent& event);
	    void OnPyFrame(wxCommandEvent& event);
	    void ScriptingSetPcbEditFrame(PCB_EDIT_FRAME *aPCBEdaFrame);
	private:
	    DECLARE_EVENT_TABLE()
            PCB_EDIT_FRAME *PcbEditFrame ;
};

enum
	{
	    ID_EXIT=1001,
	    ID_PYFRAME
	};

BEGIN_EVENT_TABLE(MyFrame, wxFrame)
    EVT_MENU(ID_EXIT,      MyFrame::OnExit)
    EVT_MENU(ID_PYFRAME,   MyFrame::OnPyFrame)
END_EVENT_TABLE()

#endif

bool EDA_APP::OnInit()
{
    wxFileName      fn;
    PCB_EDIT_FRAME* frame = NULL;

#ifdef KICAD_SCRIPTING    
    if ( !pcbnewInitPythonScripting(&m_mainTState) ) 
    {
       return false;
    }
#endif    
#ifdef KICAD_SCRIPTING_EXPERIMENT
    MyFrame *zz = new MyFrame(_T("Embedded wxPython Test"),wxDefaultPosition, wxSize(700, 600));
    zz->Show(true);
#endif
    
    InitEDA_Appl( wxT( "Pcbnew" ), APP_PCBNEW_T );

    if( m_Checker && m_Checker->IsAnotherRunning() )
    {
        if( !IsOK( NULL, _( "Pcbnew is already running, Continue?" ) ) )
            return false;
    }

    // read current setup and reopen last directory if no filename to open in command line
    bool reopenLastUsedDirectory = argc == 1;
    GetSettings( reopenLastUsedDirectory );

    if( argc > 1 )
    {
        fn = argv[1];

        if( fn.GetExt() != PcbFileExtension )
        {
            wxLogDebug( wxT( "Pcbnew file <%s> has the wrong extension.  \
Changing extension to .brd." ), GetChars( fn.GetFullPath() ) );
            fn.SetExt( PcbFileExtension );
        }

        if( fn.IsOk() && fn.DirExists() )
            wxSetWorkingDirectory( fn.GetPath() );
    }

    g_DrawBgColor = BLACK;

    /* Must be called before creating the main frame in order to
     * display the real hotkeys in menus or tool tips */
    ReadHotkeyConfig( wxT( "PcbFrame" ), g_Board_Editor_Hokeys_Descr );

    frame = new PCB_EDIT_FRAME( NULL, wxT( "Pcbnew" ), wxPoint( 0, 0 ), wxSize( 600, 400 ) );

    #ifdef KICAD_SCRIPTING    
    ScriptingSetPcbEditFrame(frame); /* give the scripting helpers access to our frame */
    #endif    
    
    frame->UpdateTitle();

    SetTopWindow( frame );
    frame->Show( true );

    if( CreateServer( frame, KICAD_PCB_PORT_SERVICE_NUMBER ) )
    {
        SetupServerFunction( RemoteCommand );
    }

    frame->Zoom_Automatique( true );

    // Load config and default values before loading a board file
    // Some will be overwritten after loading the board file
    frame->LoadProjectSettings( fn.GetFullPath() );

    /* Load file specified in the command line. */
    if( fn.IsOk() )
    {
        /* Note the first time Pcbnew is called after creating a new project
         * the board file may not exist so we load settings only.
         */
        if( fn.FileExists() )
        {
            frame->LoadOnePcbFile( fn.GetFullPath() );
        }
        else
        {   // File does not exists: prepare an empty board
            wxSetWorkingDirectory( fn.GetPath() );
            frame->GetScreen()->SetFileName( fn.GetFullPath( wxPATH_UNIX ) );
            frame->UpdateTitle();
            frame->UpdateFileHistory( frame->GetScreen()->GetFileName() );
            frame->OnModify();          // Ready to save the new empty board

            wxString msg;
            msg.Printf( _( "File <%s> does not exist.\nThis is normal for a new project" ),
                        GetChars( frame->GetScreen()->GetFileName() ) );
            wxMessageBox( msg );
        }
    }

    // update the layer names in the listbox
    frame->ReCreateLayerBox( NULL );

    /* For an obscure reason the focus is lost after loading a board file
     * when starting (i.e. only at this point)
     * (seems due to the recreation of the layer manager after loading the file)
     * give focus to main window and Drawpanel
     * must be done for these 2 windows (for an obscure reason ...)
     * Linux specific
     * This is more a workaround than a fix.
     */
    frame->SetFocus();
    frame->GetCanvas()->SetFocus();
#ifdef KICAD_SCRIPTING_EXPERIMENT
    zz->ScriptingSetPcbEditFrame(frame); // make the frame available to my python thing
    // now to find a way to use it for something useful
#endif
    return true;
}
#if 0
// for some reason KiCad classes do not implement OnExit
// if I add it in the declaration, I need to fix it in every application
// so for now make a note TODO TODO
// we need to clean up python when the application exits
int EDA_APP::OnExit() {
    // Restore the thread state and tell Python to cleanup after itself.
    // wxPython will do its own cleanup as part of that process.  This is done
    // in OnExit instead of ~MyApp because OnExit is only called if OnInit is
    // successful.
#if KICAD_SCRIPTING_EXPERIMENT
    wxPyEndAllowThreads(m_mainTState);
    Py_Finalize();
#endif
    return 0;    
}
#endif

#ifdef KICAD_SCRIPTING_EXPERIMENT
// stuff copied from WxPython examples
bool EDA_APP::Init_wxPython() {
    // Initialize Python
    Py_Initialize();
    PyEval_InitThreads();

    // Load the wxPython core API.  Imports the wx._core_ module and sets a
    // local pointer to a function table located there.  The pointer is used
    // internally by the rest of the API functions.
    if ( ! wxPyCoreAPI_IMPORT() ) {
        wxLogError(wxT("***** Error importing the wxPython API! *****"));
        PyErr_Print();
        Py_Finalize();
        return false;
    }        
    
    // Save the current Python thread state and release the
    // Global Interpreter Lock.
    m_mainTState = wxPyBeginAllowThreads();

    return true;
}


//
MyFrame::MyFrame(const wxString& title, const wxPoint& pos, const wxSize& size)
    : wxFrame(NULL, -1, title, pos, size,
              wxDEFAULT_FRAME_STYLE|wxNO_FULL_REPAINT_ON_RESIZE)
{
//    SetIcon(wxICON(mondrian));

    wxMenuBar* mbar = new wxMenuBar;
    wxMenu*    menu = new wxMenu;
    menu->Append(ID_PYFRAME, _T("Make wx&Python frame"));
    menu->AppendSeparator();
    menu->Append(ID_EXIT, _T("&Close Frame\tAlt-X"));
    mbar->Append(menu, _T("&File"));
    SetMenuBar(mbar);

    CreateStatusBar();
    RedirectStdio();

    // Make some child windows from C++
    wxSplitterWindow* sp = new wxSplitterWindow(this, -1);
    wxPanel* p1 = new wxPanel(sp, -1, wxDefaultPosition, wxDefaultSize, wxSUNKEN_BORDER);

    new wxStaticText(p1, -1,
                 _T("The frame, menu, splitter, this panel and this text were created in C++..."),
                 wxPoint(10,10));

    // And get a panel from Python
    wxWindow* p2 = DoPythonStuff(sp);

    if (p2)
        sp->SplitHorizontally(p1, p2, GetClientSize().y/4);
}

void MyFrame::OnExit(wxCommandEvent& event)
{
    Close();
}

void MyFrame::ScriptingSetPcbEditFrame(PCB_EDIT_FRAME *aPCBEdaFrame)
{
	PcbEditFrame = aPCBEdaFrame;
}

//----------------------------------------------------------------------
// This is where the fun begins...


const char* python_code1 = "\
import wx\n\
f = wx.Frame(None, -1, 'Hello from wxPython!', size=(250, 150))\n\
f.Show()\n\
";

void MyFrame::OnPyFrame(wxCommandEvent& event)
{
    // For simple Python code that doesn't have to interact with the
    // C++ code in any way, you can execute it with PyRun_SimpleString.


    // First, whenever you do anything with Python objects or code, you
    // *MUST* aquire the Global Interpreter Lock and block other
    // Python threads from running.
    wxPyBlock_t blocked = wxPyBeginBlockThreads();

    // Execute the code in the __main__ module
    PyRun_SimpleString(python_code1);

    // Finally, release the GIL and let other Python threads run.
    wxPyEndBlockThreads(blocked);
}


void MyFrame::RedirectStdio() {
    // This is a helpful little tidbit to help debugging and such.  It
    // redirects Python's stdout and stderr to a window that will popup
    // only on demand when something is printed, like a traceback.
    const char* python_redirect = "import sys\n\
import wx\n\
output = wx.PyOnDemandOutputWindow()\n\
sys.stdin = sys.stderr = output\n";
    
    wxPyBlock_t blocked = wxPyBeginBlockThreads();
    PyRun_SimpleString(python_redirect);
    wxPyEndBlockThreads(blocked);
}

const char* python_code2 = "\
import sys\nimport os\n\
sys.path.append(os.path.expanduser('~/.kicad_plugins'))\n\
import embedded_sample\n\
\n\
def makeWindow(parent):\n\
    win = embedded_sample.MyPanel(parent)\n\
    return win\n\
";

wxWindow* MyFrame::DoPythonStuff(wxWindow* parent)
{
    // More complex embedded situations will require passing C++ objects to
    // Python and/or returning objects from Python to be used in C++.  This
    // sample shows one way to do it.  NOTE: The above code could just have
    // easily come from a file, or the whole thing could be in the Python
    // module that is imported and manipulated directly in this C++ code.  See
    // the Python API for more details.

    wxWindow* window = NULL;
    PyObject* result;

    // As always, first grab the GIL
    wxPyBlock_t blocked = wxPyBeginBlockThreads();

    // Now make a dictionary to serve as the global namespace when the code is
    // executed.  Put a reference to the builtins module in it.  (Yes, the
    // names are supposed to be different, I don't know why...)
    PyObject* globals = PyDict_New();
    PyObject* builtins = PyImport_ImportModule("__builtin__");
    PyDict_SetItemString(globals, "__builtins__", builtins);
    Py_DECREF(builtins);

    // Execute the code to make the makeWindow function
    result = PyRun_String(python_code2, Py_file_input, globals, globals);
    // Was there an exception?
    if (! result) {
        PyErr_Print();
        wxPyEndBlockThreads(blocked);
        return NULL;
    }
    Py_DECREF(result);

    // Now there should be an object named 'makeWindow' in the dictionary that
    // we can grab a pointer to:
    PyObject* func = PyDict_GetItemString(globals, "makeWindow");
    wxASSERT(PyCallable_Check(func));

    // Now build an argument tuple and call the Python function.  Notice the
    // use of another wxPython API to take a wxWindows object and build a
    // wxPython object that wraps it.
    PyObject* arg = wxPyMake_wxObject(parent, false);
    wxASSERT(arg != NULL);
    PyObject* tuple = PyTuple_New(1);
    PyTuple_SET_ITEM(tuple, 0, arg);
    result = PyEval_CallObject(func, tuple);

    // Was there an exception?
    if (! result)
        PyErr_Print();
    else {
        // Otherwise, get the returned window out of Python-land and
        // into C++-ville...
        bool success = wxPyConvertSwigPtr(result, (void**)&window, _T("wxWindow"));
        wxASSERT_MSG(success, _T("Returned object was not a wxWindow!"));
        Py_DECREF(result);
    }

    // Release the python objects we still have
    Py_DECREF(globals);
    Py_DECREF(tuple);

    // Finally, after all Python stuff is done, release the GIL
    wxPyEndBlockThreads(blocked);

    return window;
}

#endif
