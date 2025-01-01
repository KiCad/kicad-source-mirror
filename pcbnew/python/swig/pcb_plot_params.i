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
 * @file pcb_plot_params.i
 */

%include plotprint_opts.h
%include pcb_plot_params.h

%extend PCB_PLOT_PARAMS
{
    %pythoncode
    %{
    # add SetUseGerberAttributes() for compatibility with old python scripts
    # after renaming it to SetUseGerberX2format()
    def SetUseGerberAttributes( self, aUse ):
        return self.SetUseGerberX2format( aUse )
    %}
}

%{
#include <pcb_plot_params.h>
%}

