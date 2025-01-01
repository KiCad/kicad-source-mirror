/*
 * This program is part of KiCad, a free EDA CAD application.
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

%warnfilter(325) NETINFO_MAPPING::iterator;
%ignore NETINFO_MAPPING;        // no code generation for this class

%feature("notabstract")     NETINFO_ITEM;

%include netinfo.h

%{
#include <netinfo.h>
%}

%extend NETINFO_ITEM
{
    %pythoncode
    %{
    def GetNetClassName(self):
        return self.GetNetClassSlow().GetName()
    %}
}

// http://swig.10945.n7.nabble.com/std-containers-and-pointers-td3728.html
%{
    namespace swig {
        template <>  struct traits<NETINFO_ITEM> {
            typedef pointer_category category;
            static const char* type_name() { return "NETINFO_ITEM"; }
        };
    }
%}
