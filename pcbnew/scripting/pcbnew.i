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
%include "kicad.i"

// ignore a couple of items that generate warnings from swig built code

%ignore BOARD_ITEM::ZeroOffset;
%ignore D_PAD::m_PadSketchModePenSize;

// this is what it must be included in the wrapper .cxx code to compile

%{ 
  #include <wx_python_helpers.h>
  #include <class_board_item.h>
  #include <class_board_connected_item.h>
  #include <class_board.h>
  #include <class_module.h>
  #include <class_track.h>  
  #include <class_pad.h>
  #include <class_netinfo.h>
  #include <class_pcb_text.h>
  #include <class_dimension.h>
  #include <class_drawsegment.h>
  #include <class_marker_pcb.h>
  #include <class_text_mod.h>
  #include <class_edge_mod.h>
  #include <dlist.h>
  #include <class_zone_settings.h>
  #include <class_netclass.h>
  #include <class_netinfo.h>
  #include <layers_id_colors_and_visibility.h>
  #include <pcbnew_scripting_helpers.h>
  
      
  BOARD *GetBoard(); /* get current editor board */
%}

#ifdef BUILD_WITH_PLUGIN
%{
  #include <io_mgr.h>
  #include <kicad_plugin.h>
%}
#endif

%include <class_board_item.h>
%include <class_board_connected_item.h>
%include <class_board.h>
%include <class_module.h>
%include <class_track.h>
%include <class_pad.h>
%include <class_netinfo.h>
%include <class_pcb_text.h>
%include <class_dimension.h>
%include <class_drawsegment.h>
%include <class_marker_pcb.h>
%include <class_text_mod.h>
%include <class_edge_mod.h>
%include <dlist.h>
%include <class_zone_settings.h>
%include <class_netclass.h>
%include <class_netinfo.h>
%include <layers_id_colors_and_visibility.h>

%include "board_item.i"

%include <pcbnew_scripting_helpers.h>

#ifdef BUILD_WITH_PLUGIN
  %include <io_mgr.h>
  %include <kicad_plugin.h>
#endif



%include "board.i"
%include "units.i"
