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

/* DLIST python iteration code, to allow standard iteration over DLIST */

%extend DLIST
{
    %pythoncode
    {
        class DLISTIter:
            def __init__(self,aList):
                self.last = aList   # last item is the start of list

            def next(self):         # get the next item

                item = self.last
                try:
                  item = item.Get()
                except:
                  pass

                if item is None:    # if the item is None, then finish the iteration
                    raise StopIteration
                else:
                    ret = None

                    # first item in list has "Get" as a DLIST
                    try:
                        ret = self.last.Get()
                    except:
                        ret = self.last # next items do not..

                    self.last = self.last.Next()

                    # when the iterated object can be casted down in inheritance, just do it..

                    if 'Cast' in dir(ret):
                        ret = ret.Cast()

                    return ret

        def __iter__(self):
            return self.DLISTIter(self)

    }
}
