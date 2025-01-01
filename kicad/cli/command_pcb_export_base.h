/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2022 Mark Roszko <mark.roszko@gmail.com>
 * Copyright (C) 1992-2023 KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef COMMAND_EXPORT_PCB_BASE_H
#define COMMAND_EXPORT_PCB_BASE_H

#include "command.h"
#include <layer_ids.h>
#include <lset.h>
#include <lseq.h>

namespace CLI
{
#define ARG_BLACKANDWHITE "--black-and-white"
#define ARG_BLACKANDWHITE_DESC "Black and white only"

#define ARG_SKETCH_PADS_ON_FAB_LAYERS "--sketch-pads-on-fab-layers"
#define ARG_SKETCH_PADS_ON_FAB_LAYERS_DESC "Draw pad outlines and their numbers on front and back fab layers"

#define ARG_HIDE_DNP_FPS_ON_FAB_LAYERS "--hide-DNP-footprints-on-fab-layers"
#define ARG_HIDE_DNP_FPS_ON_FAB_LAYERS_DESC "Don't plot text & graphics of DNP footprints on fab layers"
#define ARG_SKETCH_DNP_FPS_ON_FAB_LAYERS "--sketch-DNP-footprints-on-fab-layers"
#define ARG_SKETCH_DNP_FPS_ON_FAB_LAYERS_DESC "Plot graphics of DNP footprints in sketch mode on fab layers"
#define ARG_CROSSOUT_DNP_FPS_ON_FAB_LAYERS "--crossout-DNP-footprints-on-fab-layers"
#define ARG_CROSSOUT_DNP_FPS_ON_FAB_LAYERS_DESC "Plot an 'X' over the courtyard of DNP footprints on fab layers, and strikeout their reference designators"

#define ARG_DRILL_SHAPE_OPTION "--drill-shape-opt"
#define ARG_DRILL_SHAPE_OPTION_DESC "Set pad/via drill shape option (0 = no shape, 1 = small shape, 2 = actual shape)"

#define ARG_NEGATIVE "--negative"
#define ARG_NEGATIVE_SHORT "-n"
#define ARG_NEGATIVE_DESC "Plot as negative (useful for directly etching from the export)"

#define ARG_LAYERS "--layers"
#define ARG_EXCLUDE_REFDES "--exclude-refdes"
#define ARG_EXCLUDE_VALUE "--exclude-value"
#define ARG_THEME "--theme"
#define ARG_INCLUDE_BORDER_TITLE "--include-border-title"
#define ARG_MIRROR "--mirror"

#define ARG_PLOT_INVISIBLE_TEXT "--plot-invisible-text"
#define ARG_PLOT_INVISIBLE_TEXT_DESC "Force plotting of invisible values / refs"

#define ARG_FLIP_BOTTOM_PADS "--flip-bottom-pads"
#define ARG_UNIQUE_PINS "--unique-pins"
#define ARG_UNIQUE_FOOTPRINTS "--unique-footprints"
#define ARG_USE_DRILL_ORIGIN "--use-drill-origin"
#define ARG_STORE_ORIGIN_COORD "--store-origin-coord"

#define ARG_COMMON_LAYERS "--common-layers"

struct PCB_EXPORT_BASE_COMMAND : public COMMAND
{
    PCB_EXPORT_BASE_COMMAND( const std::string& aName, bool aInputIsDir = false,
                             bool aOutputIsDir = false );

protected:
    int  doPerform( KIWAY& aKiway ) override;
    LSEQ convertLayerStringList( wxString& aLayerString, bool& aLayerArgSet ) const;
    void addLayerArg( bool aRequire );

    // The list of canonical layer names used in .kicad_pcb files:
    std::map<std::string, LSET> m_layerMasks;

    // The list of canonical layer names used in GUI (not translated):
    std::map<std::string, LSET> m_layerGuiMasks;

    LSEQ                        m_selectedLayers;
    bool                        m_selectedLayersSet;

    bool                        m_hasLayerArg;
    bool                        m_requireLayers;
};
} // namespace CLI

#endif
