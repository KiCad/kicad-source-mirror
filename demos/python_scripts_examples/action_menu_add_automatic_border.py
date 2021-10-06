#  add_automatic_border.py
#
# Copyright (C) 2017 KiCad Developers, see AUTHORS.TXT for contributors.
#
#  This program is free software; you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation; either version 2 of the License, or
#  (at your option) any later version.
#
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with this program; if not, write to the Free Software
#  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
#  MA 02110-1301, USA.
#
#

from pcbnew import *
import re
import datetime


class add_automatic_border( ActionPlugin ):
    """
    add_automatic_border: An other sample plugin as an example of ActionPlugin
    Build PCB edges to include all PCB elements
    How to use:
    - add all your modules/track/area/text...
    - Call the plugin
    - An including PCB edge will be created automatically
    """

    def defaults( self ):
        """
        Method defaults must be redefined
        self.name should be the menu label to use
        self.category should be the category (not yet used)
        self.description should be a comprehensive description
          of the plugin
        """
        self.name = "Add or update automatic PCB edges"
        self.category = "Modify PCB"
        self.description = "Automatically add or update edges on an existing PCB"
        # Offset between existing elements and edge we will add/update (fixed at 2.54mm)
        self.offset = FromMM( 2.54 )
        # Attach to a grid step (fixed at 2.54mm)
        self.grid = FromMM( 2.54 )

    def min( self, a, b ):
        """
        Method min: Found min between a and b even is one is None
        """
        if a is None:
            return b
        if b is None:
            return a
        if a < b:
            return a
        return b

    def max( self, a, b ):
        """
        Method max: Found max between a and b even is one is None
        """
        if a is None:
            return b
        if b is None:
            return a
        if a > b:
            return a
        return b

    def Run( self ):
        """
        Method Run is called by Action menu
        """
        pcb = GetBoard()
        # Find including area on min/max x/y
        min_x = None
        max_x = None
        min_y = None
        max_y = None

        # Enum all area
        for i in range( pcb.GetAreaCount() ):
            min_x = self.min( min_x, pcb.GetArea( i ).GetBoundingBox().GetX() )
            min_y = self.min( min_y, pcb.GetArea( i ).GetBoundingBox().GetY() )
            max_x = self.max( max_x, \
                                  pcb.GetArea( i ).GetBoundingBox().GetX() \
                                  + pcb.GetArea( i ).GetBoundingBox().GetWidth() )
            max_y = self.max( max_y, \
                                  pcb.GetArea( i ).GetBoundingBox().GetY() \
                                  + pcb.GetArea( i ).GetBoundingBox().GetHeight() )

        # Same with track
        for track in pcb.GetTracks():
            min_x = self.min( min_x, track.GetStart().x )
            min_y = self.min( min_y, track.GetStart().y )
            max_x = self.max( max_x, track.GetEnd().x )
            max_y = self.max( max_y, track.GetEnd().y )

            min_x = self.min( min_x, track.GetEnd().x )
            min_y = self.min( min_y, track.GetEnd().y )
            max_x = self.max( max_x, track.GetStart().x )
            max_y = self.max( max_y, track.GetStart().y )

        # Variable to store PCB edges we found
        west = None
        north = None
        east = None
        south = None

        for draw in pcb.GetDrawings():
            edge_candidate = False
            #  Detect if current drawing is a PCB edge
            # and a candicate for north/south/east or west
            if draw.GetClass() == 'PCB_SHAPE' \
              and draw.GetLayer() == Edge_Cuts:
                # Try candicate for east/west ?
                if draw.GetStart().x == draw.GetEnd().x:
                    if west is None and east is None:
                        west = draw
                        edge_candidate = True
                    elif west is None: # east is not none
                        if draw.GetStart().x < east.GetStart().x:
                            west = draw
                            edge_candidate = True
                        else:
                            west = east
                            east = draw
                            edge_candidate = True
                    elif east is None: # west is not none
                        if draw.GetStart().x > west.GetStart().x:
                            east = draw
                            edge_candidate = True
                        else:
                            east = west
                            west = draw
                            edge_candidate = True
                    else:
                        None # west and east are already found...

                # Try candicate for north/south ?
                if draw.GetStart().y == draw.GetEnd().y:
                    if north is None and south is None:
                        north = draw
                        edge_candidate = True
                    elif north is None: # south is not none
                        if draw.GetStart().y < south.GetStart().y:
                            north = draw
                            edge_candidate = True
                        else:
                            north = south
                            south = draw
                            edge_candidate = True
                    elif south is None: # north is not none
                        if draw.GetStart().y > north.GetStart().y:
                            south = draw
                            edge_candidate = True
                        else:
                            south = north
                            north = draw
                            edge_candidate = True
                    else:
                        None # north and south are already found...
            # Not a edge candidate: use it to find including edges
            if not edge_candidate:
                bbox = draw.GetBoundingBox()
                min_x = self.min( min_x, bbox.GetX() )
                min_y = self.min( min_y, bbox.GetY() )
                max_x = self.max( max_x, bbox.GetX() + bbox.GetWidth() )
                max_y = self.max( max_y, bbox.GetY() + bbox.GetHeight() )

        # Same with modules: Find including area
        for module in pcb.GetFootprints():
            bbox = module.GetBoundingBox()
            min_x = self.min( min_x, bbox.GetX() )
            min_y = self.min( min_y, bbox.GetY() )
            max_x = self.max( max_x, bbox.GetX() + bbox.GetWidth() )
            max_y = self.max( max_y, bbox.GetY() + bbox.GetHeight() )

        # Add a space between including area and edge (3mm)
        min_x = min_x - self.offset
        min_y = min_y - self.offset
        max_x = max_x + self.offset
        max_y = max_y + self.offset

        # Fix on the defined grid
        min_x = min_x - (min_x % self.grid)
        min_y = min_y - (min_y % self.grid)
        if ( max_x % self.grid ) != 0:
            max_x = max_x - (max_x % self.grid) + self.grid
        if ( max_y % self.grid ) != 0:
            max_y = max_y - (max_y % self.grid) + self.grid

        # Add or update all edges
        need_add = False
        if west is None:
            need_add = True
            west = PCB_SHAPE()
            west.SetLayer( Edge_Cuts )

        west.SetStart( wxPoint( min_x, min_y ) )
        west.SetEnd( wxPoint( min_x, max_y ) )
        if need_add:
            pcb.Add( west )

        need_add = False
        if north is None:
            need_add = True
            north = PCB_SHAPE()
            north.SetLayer( Edge_Cuts )

        north.SetStart( wxPoint( min_x, min_y ) )
        north.SetEnd( wxPoint( max_x, min_y ) )
        if need_add:
            pcb.Add( north )

        need_add = False
        if east is None:
            need_add = True
            east = PCB_SHAPE()
            east.SetLayer( Edge_Cuts )

        east.SetStart( wxPoint( max_x, min_y ) )
        east.SetEnd( wxPoint( max_x, max_y ) )
        if need_add:
            pcb.Add( east )

        need_add = False
        if south is None:
            need_add = True
            south = PCB_SHAPE()
            south.SetLayer( Edge_Cuts )

        south.SetStart( wxPoint( min_x, max_y ) )
        south.SetEnd( wxPoint( max_x, max_y ) )
        if need_add:
            pcb.Add( south )


# Register the action
add_automatic_border().register()
