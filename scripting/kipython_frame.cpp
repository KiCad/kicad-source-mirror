/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.TXT for contributors.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 3
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/gpl-3.0.html
 * or you may search the http://www.gnu.org website for the version 3 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#include <frame_type.h>
#include <kipython_frame.h>
#include <kiway_player.h>
#include <python_scripting.h>

#include <pybind11/embed.h>
#include <sstream>
#include <pybind11/embed.h>

void KIPYTHON_FRAME::SetupPythonEditor()
{
    using namespace pybind11::literals;

    // As always, first grab the GIL
    PyLOCK lock;

    // Make sure the kicad_pyshell module is in the path
    wxString sysPath = SCRIPTING::PyScriptingPath( SCRIPTING::PATH_TYPE::STOCK );
    auto     locals  = pybind11::dict( "stock_path"_a = sysPath.ToStdString() );

    pybind11::exec( R"(
import sys
sys.path.append( stock_path )
    )", pybind11::globals(), locals );

    // passing window ids instead of pointers is because wxPython is not
    // exposing the needed c++ apis to make that possible.
    std::stringstream pcbnew_pyshell_one_step;
    pcbnew_pyshell_one_step << "import kicad_pyshell\n";
    pcbnew_pyshell_one_step << "newshell = kicad_pyshell.makePcbnewShellWindow( " << GetId() << " )\n";

    // Execute the code to make the makeWindow function we defined above
    PyRun_SimpleString( pcbnew_pyshell_one_step.str().c_str() );

    /// For unknown reasons, some mac builds don't automatically layout the Python window until resized
    /// so force the fit here
    Layout();
    Fit();
}


void KIPYTHON_FRAME::redirectStdio()
{
    // This is a helpful little tidbit to help debugging and such.  It
    // redirects Python's stdout and stderr to a window that will popup
    // only on demand when something is printed, like a traceback.

    PyLOCK      lock;

    using namespace pybind11::literals;
    int id = GetId();
    auto locals = pybind11::dict( "parent_id"_a= id );

    pybind11::exec( R"(
import sys
import wx
output = wx.PyOnDemandOutputWindow()
_original_stderr = sys.__stderr__
class _OutputProxy:
    def __init__( self, output_window, original_stream ):
        self._output = output_window
        self._original = original_stream

    def write( self, data ):
        self._output.write( data )

    def flush( self ):
        if self._original:
            self._original.flush()

sys.stderr = _OutputProxy( output, _original_stderr )
parent = wx.Window.FindWindowById( parent_id )
output.SetParent( parent )
    )", pybind11::globals(), locals );
}


KIPYTHON_FRAME::KIPYTHON_FRAME( KIWAY* aKiway, wxWindow* aParent ) :
            KIWAY_PLAYER( aKiway, aParent, FRAME_PYTHON, wxT( "KiPython" ), wxDefaultPosition,
                          wxDefaultSize, KICAD_DEFAULT_DRAWFRAME_STYLE, wxT( "KiPython" ),
                          unityScale )
{
    CallAfter( [this]()
               {
                   SetupPythonEditor();
               } );

    redirectStdio();
}


KIPYTHON_FRAME::~KIPYTHON_FRAME()
{
}
