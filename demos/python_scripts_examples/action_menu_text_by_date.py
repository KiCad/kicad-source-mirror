#  text_by_date.py
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

import pcbnew
import re
import datetime


class text_by_date( pcbnew.ActionPlugin ):
    """
    test_by_date: A sample plugin as an example of ActionPlugin
    Add the date to any text field of the board where the content is '$date$'
    How to use:
    - Add a text on your board with the content '$date$'
    - Call the plugin
    - Automatically the date will be added to the text (format YYYY-MM-DD)
    """

    def defaults( self ):
        """
        Method defaults must be redefined
        self.name should be the menu label to use
        self.category should be the category (not yet used)
        self.description should be a comprehensive description
          of the plugin
        """
        self.name = "Add date on PCB"
        self.category = "Modify PCB"
        self.description = "Automatically add date on an existing PCB"

    def Run( self ):
        pcb = pcbnew.GetBoard()
        for draw in pcb.GetDrawings():
            if draw.GetClass() == 'PTEXT':
                txt = re.sub( "\$date\$ [0-9]{4}-[0-9]{2}-[0-9]{2}",
                                 "$date$", draw.GetText() )
                if txt == "$date$":
                    draw.SetText( "$date$ %s"%datetime.date.today() )


text_by_date().register()
