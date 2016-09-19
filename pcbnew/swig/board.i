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
 * @file board.i
 * @brief Specific BOARD extensions and templates
 */


/*

By default we do not translate exceptions for EVERY C++ function since not every
C++ function throws, and that would be unused and very bulky mapping code.
Therefore please help gather the subset of C++ functions for this class that do
throw and add them here, before the class declarations.

*/
HANDLE_EXCEPTIONS(BOARD::TracksInNetBetweenPoints)


%include <class_board_design_settings.h>
%{
#include <class_board_design_settings.h>
%}



%{
#include <class_board.h>
#include <layers_id_colors_and_visibility.h>
%}


%import dlist.h

// Organize the two forms of include side by side so that it is easier to
// migrate each grouping into a separate *.i file later.


%include class_board_item.h
%{
#include <class_board_item.h>
%}

%include class_board_connected_item.h
%{
#include <class_board_connected_item.h>
%}

%include pad_shapes.h

%include class_pad.h
%{
#include <class_pad.h>
%}

%include class_module.h
%{
#include <class_module.h>
%}

%include class_track.h
%{
#include <class_track.h>
%}

%include class_zone.h
%include zones.h
%{
#include <class_zone.h>
#include <zones.h>
%}


%include layers_id_colors_and_visibility.h

%include class_pcb_text.h
%{
#include <class_pcb_text.h>
%}

%include class_dimension.h
%{
#include <class_dimension.h>
%}


%include class_drawsegment.h
%{
#include <class_drawsegment.h>
%}

%include class_marker_pcb.h
%{
#include <class_marker_pcb.h>
%}


%include class_mire.h
%{
#include <class_mire.h>
%}


%include class_text_mod.h
%{
#include <class_text_mod.h>
%}

%include class_edge_mod.h
%{
#include <class_edge_mod.h>
%}

%include class_zone_settings.h
%{
#include <class_zone_settings.h>
%}

%include class_netinfo.h
%include class_netclass.h
%{
#include <class_netinfo.h>
#include <class_netclass.h>
%}


// this is to help python with the * accessor of DLIST templates
%rename(Get) operator BOARD_ITEM*;
%rename(Get) operator TRACK*;
%rename(Get) operator D_PAD*;
%rename(Get) operator MODULE*;

// we must translate C++ templates to scripting languages

%template(BOARD_ITEM_List) DLIST<BOARD_ITEM>;
%template(MODULE_List)     DLIST<MODULE>;
%template(TRACK_List)      DLIST<TRACK>;
%template(PAD_List)        DLIST<D_PAD>;

// std::vector templates

%template(VIA_DIMENSION_Vector) std::vector<VIA_DIMENSION>;
%template(RATSNEST_Vector)      std::vector<RATSNEST_ITEM>;


%include class_board.h


%extend BOARD
{
    %pythoncode
    %{

    def GetModules(self):             return self.m_Modules
    def GetDrawings(self):            return self.m_Drawings
    def GetTracks(self):              return self.m_Track
    def GetFullRatsnest(self):        return self.m_FullRatsnest

    def Save(self,filename):
        return SaveBoard(filename,self,IO_MGR.KICAD)

    #
    # add function, clears the thisown to avoid python from deleting
    # the object in the garbage collector
    #
    def Add(self,item):
        item.thisown=0
        self.AddNative(item)

    def GetNetClasses(self):
        return self.GetDesignSettings().m_NetClasses

    def GetCurrentNetClassName(self):
        return self.GetDesignSettings().m_CurrentNetClassName

    def GetViasDimensionsList(self):
        return self.GetDesignSettings().m_ViasDimensionsList

    def GetTrackWidthList(self):
        return self.GetDesignSettings().m_TrackWidthList

    def GetNetsByName(self):
        """
        Return a dictionary like object with key:wxString netname and value:NETINFO_ITEM
        """
        return self.GetNetInfo().NetsByName()

    def GetNetsByNetcode(self):
        """
        Return a dictionary like object with key:int netcode and value:NETINFO_ITEM
        """
        return self.GetNetInfo().NetsByNetcode()

    def GetNetcodeFromNetname(self,netname):
        """
        Given a netname, return its netcode
        """
        net = self.GetNetsByName()[netname]
        return net.GetNet()

    def GetAllNetClasses(self):
        """
        Return a dictionary like object with net_class_name as key and NETCLASSPRT as value
        GetNetClasses(BOARD self) -> { wxString net_class_name : NETCLASSPTR }
        Include the "Default" netclass also.
        """
        netclassmap = self.GetNetClasses().NetClasses()
        # add the Default one too
        netclassmap[ NETCLASS.Default ] = self.GetNetClasses().GetDefault()
        return netclassmap
    %}
}


%extend DRAWSEGMENT
{
    %pythoncode
    %{
    def GetShapeStr(self):
        return self.ShowShape(self.GetShape())
    %}
}

%extend BOARD_ITEM
{
    %pythoncode
    %{
    def SetPos(self,p):
        self.SetPosition(p)
        self.SetPos0(p)

    def SetStartEnd(self,start,end):
        self.SetStart(start)
        self.SetStart0(start)
        self.SetEnd(end)
        self.SetEnd0(end)
    %}
}


%feature("notabstract")     NETINFO_ITEM;

// http://swig.10945.n7.nabble.com/std-containers-and-pointers-td3728.html
%{
    namespace swig {
        template <>  struct traits<NETINFO_ITEM> {
            typedef pointer_category category;
            static const char* type_name() { return "NETINFO_ITEM"; }
        };
    }
%}
