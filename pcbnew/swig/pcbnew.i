/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2012 NBEE Embedded Systems, Miguel Angel Ajo <miguelangel@nbee.es>
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
 * @file pcbnew.i
 * @brief Specific pcbnew wrappers
 */


%module pcbnew

%feature("autodoc", "1");
#ifdef ENABLE_DOCSTRINGS_FROM_DOXYGEN
%include "docstrings.i"
#endif


%include kicad.i

%include <convert_to_biu.h>

%{
#include <lib_id.h>
%}
%include lib_id.h


// ignore a couple of items that generate warnings from swig built code
%ignore BOARD_ITEM::ZeroOffset;
%ignore D_PAD::m_PadSketchModePenSize;

class BASE_SET {};
%ignore BASE_SET;



// this is what it must be included in the wrapper .cxx code to compile

%{
#include <wx_python_helpers.h>
#include <colors.h>

#include <pcbnew_scripting_helpers.h>

#include <plotcontroller.h>
#include <pcb_plot_params.h>
#include <exporters/gendrill_Excellon_writer.h>

BOARD *GetBoard(); /* get current editor board */
%}


// ignore RELEASER as nested classes are still unsupported by swig
%ignore IO_MGR::RELEASER;
%include <io_mgr.h>
%{
#include <io_mgr.h>
%}


/*

By default we do not translate exceptions for EVERY C++ function since not every
C++ function throws, and that would be unused and very bulky mapping code.
Therefore please help gather the subset of C++ functions for this class that do
throw and add them here, before the class declarations.

*/
HANDLE_EXCEPTIONS(PLUGIN::Load)
HANDLE_EXCEPTIONS(PLUGIN::Save)
HANDLE_EXCEPTIONS(PLUGIN::FootprintEnumerate)
HANDLE_EXCEPTIONS(PLUGIN::FootprintLoad)
HANDLE_EXCEPTIONS(PLUGIN::FootprintSave)
HANDLE_EXCEPTIONS(PLUGIN::FootprintDelete)
%include <kicad_plugin.h>
%{
#include <kicad_plugin.h>
%}


%include <plotcontroller.h>
%include <pcb_plot_params.h>
%include <plot_common.h>
%include <exporters/gendrill_Excellon_writer.h>
%include <colors.h>

%include <pcbnew_scripting_helpers.h>



%include board.i
%include module.i
%include plugins.i
%include units.i


