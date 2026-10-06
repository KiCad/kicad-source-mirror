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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef EASYPC_PCB_BUILDER_H_
#define EASYPC_PCB_BUILDER_H_

#include <pcb_io/common/plugin_common_layer_mapping.h>

class BOARD;
class REPORTER;

namespace EASYPC
{
struct DESIGN_DOCUMENT;
}


namespace EASYPC_PCB
{

/**
 * Fill aBoard from a loaded Easy-PC / DesignSpark PCB design.  aLayerMapping, when set, overrides the default
 * layer mapping.
 *
 * @return custom rules for the spacings that net classes and board minimums cannot carry
 */
wxString BuildBoard( const EASYPC::DESIGN_DOCUMENT& aDocument, BOARD& aBoard, REPORTER* aReporter,
                     const LAYER_MAPPING_HANDLER& aLayerMapping = nullptr );

} // namespace EASYPC_PCB

#endif // EASYPC_PCB_BUILDER_H_
