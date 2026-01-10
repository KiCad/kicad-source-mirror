/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2012 NBEE Embedded Systems, Miguel Angel Ajo <miguelangel@nbee.es>
 * Copyright (C) 2016 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
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
 * @file board.i
 * @brief Specific BOARD extensions and templates
 */


/*

By default we do not translate exceptions for EVERY C++ function since not every
C++ function throws, and that would be unused and very bulky mapping code.
Therefore please help gather the subset of C++ functions for each class that do
throw and add each in a separate HANDLE_EXCEPTIONS() invocation before its
respective class declaration.  i.e. put them inside of their respective *.i
file near the top; only class BOARD functions go in board.i.

*/
HANDLE_EXCEPTIONS(BOARD::TracksInNetBetweenPoints)


%include layer_ids.i
%include board_item.i
%include board_item_container.i
%include board_connected_item.i
%include board_design_settings.i
%include connectivity.i
%include pcb_item_containers.i
%include pad.i
%include track.i
%include zone.i
%include zone_settings.i
%include pcb_shape.i
%include pcb_text.i
%include pcb_field.i
%include pcb_dimension.i
%include pcb_marker.i
%include pcb_target.i
%include pcb_table.i
%include netinfo.i
%include netclass.i
%include pcb_plot_params.i
%include pcb_group.i
%include pcb_reference_image.i

// std::vector templates
%template(VIA_DIMENSION_Vector) std::vector<VIA_DIMENSION>;

// Do not permit default BOARD ctor since it won't initialize the project
%ignore BOARD::BOARD();

// Do not wrap internal-only structures
%ignore BOARD::m_CachesMutex;
%ignore BOARD::m_IntersectsCourtyardCache;
%ignore BOARD::m_IntersectsFCourtyardCache;
%ignore BOARD::m_IntersectsBCourtyardCache;
%ignore BOARD::m_IntersectsAreaCache;
%ignore BOARD::m_EnclosedByAreaCache;
%ignore BOARD::m_LayerExpressionCache;
%ignore BOARD::m_CopperZoneRTreeCache;
%ignore BOARD::m_CopperItemRTreeCache;
%ignore BOARD::m_ItemNetclassCache;
%ignore BOARD::m_DRCZones;
%ignore BOARD::m_DRCCopperZones;
%ignore BOARD::m_DRCMaxClearance;
%ignore BOARD::m_DRCMaxPhysicalClearance;
%ignore BOARD::m_SolderMask;


%include board.h
%{
#include <board.h>
%}

%extend std::deque<BOARD_ITEM *>
{
    %pythoncode
    %{
        def __iter__(self):
            it = self.iterator()
            try:
                while True:
                    item = it.next()  # throws StopIteration when iterator reached the end.
                    yield item.Cast()
            except StopIteration:
                return
    %}
}
%extend std::deque<PCB_TRACK *>
{
    %pythoncode
    %{
        def __iter__(self):
            it = self.iterator()
            try:
                while True:
                    item = it.next()  # throws StopIteration when iterator reached the end.
                    yield item.Cast()
            except StopIteration:
                return
    %}
}

%extend BOARD
{
    // NOTE: this does not generate a ctor, despite swig docs saying it should.  Not sure why.
    // Because of this, we use the __init__ override hack below.
    // BOARD()
    // {
    //     return CreateEmptyBoard();
    // }

    // BOARD_ITEM_CONTAINER's interface functions will be implemented by SWIG
    // automatically and inherited by the python wrapper class.

    %pythoncode
    %{
    def __init__(self, *args):
        this = CreateEmptyBoard()

        try:
            self.this.append(this)
        except:
            self.this = this

    # Convert these to lists to keep users from using them to delete
    # items in the iterable while looping over it
    def GetFootprints(self):          return list(self.Footprints())
    def GetDrawings(self):            return list(self.Drawings())
    def GetTracks(self):              return list(self.Tracks())

    def Save(self,filename):
        return SaveBoard(filename,self)

    def GetNetClasses(self):
        return self.GetDesignSettings().m_NetSettings.GetNetclasses()

    def GetCurrentNetClassName(self):
        return self.GetDesignSettings().GetCurrentNetClassName()

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
        return net.GetNetCode()

    def GetAllNetClasses(self):
        """
        Return a dictionary like object with net_class_name as key and NETCLASSPTR as value
        GetNetClasses(BOARD self) -> { wxString net_class_name : NETCLASSPTR }
        Include the "Default" netclass also.
        """

        # Copy the NETCLASS_MAP so the one in the BOARD isn't modified
        # when we add the Default net class.
        netclassmap = {k:v for k,v in self.GetNetClasses().items()}
        netclassmap['Default'] = self.GetDesignSettings().m_NetSettings.GetDefaultNetclass()
        return netclassmap
    %}
}
