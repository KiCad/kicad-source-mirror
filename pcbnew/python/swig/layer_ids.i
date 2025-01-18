
%ignore operator++(SCH_LAYER_ID&);

%ignore operator++(GAL_LAYER_ID&);

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
 * @file layer_ids.i
 * @brief layers IDs, draw layers IDs, layers set and templates
 * these IDs are used as physical layers or draw layers (for colors, visibility...)
 */
%ignore operator+(const GAL_LAYER_ID&, int);

// Force the GAL_LAYER_ID version to be exposed
%ignore GAL_SET::set(int, bool);
%ignore GAL_SET::set(int);

// Disable warning 476 (Initialization using std::initializer_list).
// (SWIG doc say it is mainly a info message)
#pragma SWIG nowarn=476

%{
#include <layer_ids.h>
#include <lseq.h>
#include <lset.h>
#include <pcbnew_scripting_helpers.h>
%}

// wrapper of BASE_SEQ (see typedef std::vector<PCB_LAYER_ID> BASE_SEQ;)
%template(base_seqVect) std::vector<enum PCB_LAYER_ID>;

%include layer_ids.h
%include lseq.h
%include lset.h

// Extend LSET by 2 methods to add or remove layers from the layer list
// Mainly used to add or remove layers of a pad layer list
%extend LSET
{
    LSET addLayer( PCB_LAYER_ID aLayer)    { return self->set(aLayer); }
    LSET removeLayer( PCB_LAYER_ID aLayer) { return self->reset(aLayer); }
    LSET addLayerSet( LSET aLayerSet)    { return *self |= aLayerSet; }
    LSET removeLayerSet( LSET aLayerSet) { return *self &= ~aLayerSet; }
    std::string FmtHex() { return self->FmtHex(); }
    std::string FmtBin() { return self->FmtBin(); }
    int ParseHex( const std::string& aString ) { return self->ParseHex( aString ); }

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
