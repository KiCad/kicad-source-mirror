
%ignore operator++(SCH_LAYER_ID&);

%ignore operator++(GAL_LAYER_ID&);

/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 1992-2020 KiCad Developers, see AUTHORS.txt for contributors.
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
 * @file layers_id_colors_and_visibility.i
 * @brief layers IDs, draw layers IDs, layers set and templates
 * these IDs are used as physical layers or draw layers (for colors, visibility...)
 */
%ignore operator+(const GAL_LAYER_ID&, int);

// Force the GAL_LAYER_ID version to be exposed
%ignore GAL_SET::set(int, bool);
%ignore GAL_SET::set(int);

%include layers_id_colors_and_visibility.h

// Extend LSET by 2 methods to add or remove layers from the layer list
// Mainly used to add or remove layers of a pad layer list
%extend LSET
{
    LSET addLayer( PCB_LAYER_ID aLayer)    { return self->set(aLayer); }
    LSET removeLayer( PCB_LAYER_ID aLayer) { return self->reset(aLayer); }
    LSET addLayerSet( LSET aLayerSet)    { return *self |= aLayerSet; }
    LSET removeLayerSet( LSET aLayerSet) { return *self &= ~aLayerSet; }

    %pythoncode
    %{
    def AddLayer(self, layer):
        return self.addLayer( layer )

    def AddLayerSet(self, layers):
        return self.addLayerSet( layers )

    def RemoveLayer(self, layer):
        return self.removeLayer( layer )

    def RemoveLayerSet(self, layers):
        return self.removeLayerSet( layers )
    %}
}
%{
#include <layers_id_colors_and_visibility.h>
#include <pcbnew_scripting_helpers.h>
%}
