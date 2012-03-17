/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2012 Miguel Angel Ajo <miguelangel@nbee.es>
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
 * @file wx.i
 * @brief wx wrappers for basic things, wxString, wxPoint, wxRect, etc..
 */

%{
#include <wx_python_helpers.h>
%}

// encoding setup, ascii by default ///////////////////////////////////////////

void wxSetDefaultPyEncoding(const char* encoding);
const char* wxGetDefaultPyEncoding();

// wxPoint class wrapper to (xx,yy) tuple /////////////////////////////////////

class wxPoint
{	
public:
    int x, y;
    wxPoint(int xx, int yy);
    ~wxPoint();
    %extend {
        wxPoint __add__(const wxPoint& pt) {   return *self + pt;  }
        wxPoint __sub__(const wxPoint& pt) {   return *self - pt;  }

        void Set(long x, long y) {  self->x = x;     self->y = y;  }
        PyObject* Get() 
	{
            PyObject* tup = PyTuple_New(2);
            PyTuple_SET_ITEM(tup, 0, PyInt_FromLong(self->x));
            PyTuple_SET_ITEM(tup, 1, PyInt_FromLong(self->y));
            return tup;
        }
    }

    %pythoncode { 
    def __eq__(self,other):            return (self.x==other.x and self.y==other.y)
    def __ne__(self,other):            return not (self==other)
    def __str__(self):                 return str(self.Get())
    def __repr__(self):                return 'wxPoint'+str(self.Get())
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

    }
};


// wxChar wrappers ///////////////////////////////////////////////////////////

%typemap(in) wxChar {   wxString str = Py2wxString($input); $1 = str[0];   }
%typemap(out) wxChar {  wxString str($1);      $result = wx2PyString(str); }

// wxString wrappers /////////////////////////////////////////////////////////

%typemap(out) wxString& 
{
%#if wxUSE_UNICODE
    $result = PyUnicode_FromWideChar($1->c_str(), $1->Len());
%#else
    $result = PyString_FromStringAndSize($1->c_str(), $1->Len());
%#endif
}

%apply wxString& { wxString* }

%typemap(out) wxString {
%#if wxUSE_UNICODE
    $result = PyUnicode_FromWideChar($1.c_str(), $1.Len());
%#else
    $result = PyString_FromStringAndSize($1.c_str(), $1.Len());
%#endif
}

%typemap(varout) wxString {
%#if wxUSE_UNICODE
    $result = PyUnicode_FromWideChar($1.c_str(), $1.Len());
%#else
    $result = PyString_FromStringAndSize($1.c_str(), $1.Len());
%#endif
}

%typemap(in) wxString& (bool temp=false) 
{
    $1 = newWxStringFromPy($input);
    if ($1 == NULL) SWIG_fail;
    temp = true;
}

%typemap(freearg) wxString& 
{
    if (temp$argnum)
        delete $1;
}


%typemap(in) wxString {
    wxString* sptr = newWxStringFromPy($input);
    if (sptr == NULL) SWIG_fail;
    $1 = *sptr;
    delete sptr;
}

%typemap(typecheck, precedence=SWIG_TYPECHECK_POINTER) wxString& {
    $1 = PyString_Check($input) || PyUnicode_Check($input);
}





