/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
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
 * @file math.i
 * @brief wrappers for math helper classes
 */

%ignore VECTOR2<int>::ECOORD_MAX;
%ignore VECTOR2<int>::ECOORD_MIN;

#pragma SWIG nowarn=317


%rename(getWxPoint) operator wxPoint;
%rename(getWxSize) operator wxSize;
%{
#include <math/vector2d.h>
#include <math/vector3.h>
#include <math/box2.h>
%}
%include <math/vector2d.h>
%include <math/vector3.h>
%include <math/box2.h>

%template(VECTOR2I) VECTOR2<int>;
%template(VECTOR2L) VECTOR2<long long>;
%template(VECTOR2I_EXTENDED_TYPE) VECTOR2_TRAITS<int>;
%template(VECTOR3D) VECTOR3<double>;
%template(BOX2I) BOX2<VECTOR2I>;

%extend BOX2<VECTOR2I>
{
    BOX2I(const VECTOR2I& aPos, const VECTOR2I& aSize)
    {
        return new BOX2I(aPos, aSize);
    }
}

%extend VECTOR2<int>
{
    void Set(long x, long y) {  self->x = x;     self->y = y;  }

    PyObject* Get()
    {
        PyObject* tup = PyTuple_New(2);
        PyTuple_SET_ITEM(tup, 0, PyInt_FromLong(self->x));
        PyTuple_SET_ITEM(tup, 1, PyInt_FromLong(self->y));
        return tup;
    }

    %pythoncode
    %{
    def __eq__(self,other):            return (self.x==other.x and self.y==other.y)
    def __ne__(self,other):            return not (self==other)
    def __str__(self):                 return str(self.Get())
    def __repr__(self):                return 'VECTOR2I'+str(self.Get())
    def __len__(self):                 return len(self.Get())
    def __getitem__(self, index):      return self.Get()[index]
    def __setitem__(self, index, val):
        if index == 0:
            self.x = val
        elif index == 1:
            self.y = val
        else:
            raise IndexError
    def __nonzero__(self):               return self.Get() != (0,0)
    def __add__(self, other):            return VECTOR2I(self.x+other.x, self.y+other.y)
    def __sub__(self, other):            return VECTOR2I(self.x-other.x, self.y-other.y)

    %}
}

%extend VECTOR2<long long>
{
    void Set(long long x, long long y) {  self->x = x;     self->y = y;  }

    PyObject* Get()
    {
        PyObject* tup = PyTuple_New(2);
        PyTuple_SET_ITEM(tup, 0, PyLong_FromLongLong(self->x));
        PyTuple_SET_ITEM(tup, 1, PyLong_FromLongLong(self->y));
        return tup;
    }

    %pythoncode
    %{
    def __eq__(self,other):            return (self.x==other.x and self.y==other.y)
    def __ne__(self,other):            return not (self==other)
    def __str__(self):                 return str(self.Get())
    def __repr__(self):                return 'VECTOR2L'+str(self.Get())
    def __len__(self):                 return len(self.Get())
    def __getitem__(self, index):      return self.Get()[index]
    def __setitem__(self, index, val):
        if index == 0:
            self.x = val
        elif index == 1:
            self.y = val
        else:
            raise IndexError
    def __nonzero__(self):               return self.Get() != (0,0)
    def __add__(self, other):            return VECTOR2L(self.x+other.x, self.y+other.y)
    def __sub__(self, other):            return VECTOR2L(self.x-other.x, self.y-other.y)

    %}
}

%extend VECTOR3<double>
{
    void Set(double x, double y, double z) {  self->x = x; self->y = y; self->z = z; }

    PyObject* Get()
    {
        PyObject* tup = PyTuple_New(3);
        PyTuple_SET_ITEM(tup, 0, PyFloat_FromDouble(self->x));
        PyTuple_SET_ITEM(tup, 1, PyFloat_FromDouble(self->y));
        PyTuple_SET_ITEM(tup, 2, PyFloat_FromDouble(self->z));
        return tup;
    }

    %pythoncode
    %{
    def __eq__(self,other):            return (self.x==other.x and self.y==other.y and self.z==other.z)
    def __ne__(self,other):            return not (self==other)
    def __str__(self):                 return str(self.Get())
    def __repr__(self):                return 'VECTOR3D'+str(self.Get())
    def __len__(self):                 return len(self.Get())
    def __getitem__(self, index):      return self.Get()[index]
    def __setitem__(self, index, val):
        if index == 0:
            self.x = val
        elif index == 1:
            self.y = val
        elif index == 2:
            self.z = val
        else:
            raise IndexError
    def __nonzero__(self):               return self.Get() != (0, 0, 0)

    %}
}
