/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2012 NBEE Embedded Systems, Miguel Angel Ajo <miguelangel@nbee.es>
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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
 * @file pcbnew.i
 * @brief Specific pcbnew wrappers
 */


%module pcbnew

%pythoncode
{
import warnings
warnings.warn("The SWIG-based Python interface to the PCB editor is deprecated and will be removed "
              "in a future version of KiCad.  Please plan to move to the new IPC API and/or make "
              "use of the kicad-cli tool for your KiCad automation needs.",
               category=DeprecationWarning)
}

%feature("autodoc", "1");
#ifdef ENABLE_DOCSTRINGS_FROM_DOXYGEN
%include "docstrings.i"
#endif

// support for wchar_t
%include "cwstring.i"

%include kicad.i

// mostly for KICAD_T
%include typeinfo.i


%{
#include <lib_id.h>
%}
%include lib_id.h


// ignore a couple of items that generate warnings from swig built code
%ignore BOARD_ITEM::ZeroOffset;
%ignore PAD::m_PadSketchModePenSize;

class BASE_SET {};
%ignore BASE_SET;



// this is what it must be included in the wrapper .cxx code to compile

%{
#include <wx_python_helpers.h>
#include <gal/color4d.h>

#include <pcbnew_scripting_helpers.h>

#include <plotcontroller.h>
#include <pcb_plot_params.h>
#include <exporters/export_d356.h>
#include <exporters/place_file_exporter.h>
#include <exporters/export_vrml.h>
#include <exporters/gendrill_writer_base.h>
#include <exporters/gendrill_excellon_writer.h>
#include <exporters/gendrill_gerber_writer.h>
#include <exporters/gerber_jobfile_writer.h>
#include <pcb_io/pcb_io_mgr.h>
#include <pcb_io/pcb_io.h>

BOARD *GetBoard(); /* get current editor board */
wxArrayString GetFootprintLibraries();
wxArrayString GetFootprints(const wxString& aNickName);
%}


// Tell SWIG that the caller owns the PCB_IO* returned by FindPlugin and is
// responsible for deleting it. This fixes memory leaks in Python wrapper
// functions like FootprintLoad that call GetPluginForPath -> FindPlugin.
%newobject PCB_IO_MGR::FindPlugin;

%include <pcb_io/pcb_io_mgr.h>
%include <pcb_io/pcb_io.h>


/*

By default we do not translate exceptions for EVERY C++ function since not every
C++ function throws, and that would be unused and very bulky mapping code.
Therefore please help gather the subset of C++ functions for this class that do
throw and add them here, before the class declarations.

*/
HANDLE_EXCEPTIONS(PLUGIN::LoadBoard)
HANDLE_EXCEPTIONS(PLUGIN::SaveBoard)
HANDLE_EXCEPTIONS(PLUGIN::ImportFootprint)
HANDLE_EXCEPTIONS(PLUGIN::FootprintEnumerate)
HANDLE_EXCEPTIONS(PLUGIN::FootprintLoad)
HANDLE_EXCEPTIONS(PLUGIN::FootprintSave)
HANDLE_EXCEPTIONS(PLUGIN::FootprintDelete)
%include <pcb_io/kicad_sexpr/pcb_io_kicad_sexpr.h>
%{
#include <pcb_io/kicad_sexpr/pcb_io_kicad_sexpr.h>
%}


%include <plotcontroller.h>
%include <plotprint_opts.h>
%include <pcb_plot_params.h>
%include <plotters/plotter.h>
%include <exporters/export_d356.h>
%include <exporters/place_file_exporter.h>
%include <exporters/export_vrml.h>
%include <exporters/gendrill_writer_base.h>
%include <exporters/gendrill_excellon_writer.h>
%include <exporters/gendrill_gerber_writer.h>
%include <exporters/gerber_jobfile_writer.h>
%include <gal/color4d.h>
%include <id.h>

HANDLE_EXCEPTIONS(LoadBoard)
HANDLE_EXCEPTIONS(WriteDRCReport)
%include <pcbnew_scripting_helpers.h>



%include board.i
%include footprint.i
%include plugins.i
%include units.i
%include version.i


