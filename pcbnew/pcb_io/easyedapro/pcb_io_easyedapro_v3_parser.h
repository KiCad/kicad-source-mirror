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

#ifndef PCB_IO_EASYEDAPRO_V3_PARSER_H_
#define PCB_IO_EASYEDAPRO_V3_PARSER_H_

#include <io/easyedapro/easyedapro_parser.h>
#include <io/easyedapro/easyedapro_v3_parser.h>

#include "pcb_io_easyedapro_parser.h"


class PCB_IO_EASYEDAPRO_V3_PARSER
{
public:
    explicit PCB_IO_EASYEDAPRO_V3_PARSER( BOARD*             aBoard,
                                          PROGRESS_REPORTER* aProgressReporter );

    FOOTPRINT* ParseFootprint( const nlohmann::json& aProject, const wxString& aFpUuid,
                               const std::map<wxString, EASYEDAPRO::BLOB>& aBlobMap,
                               const EASYEDAPRO::V3_DOC_RAW&               aDoc );

    void ParseBoard( BOARD* aBoard, const nlohmann::json& aProject,
                     std::map<wxString, std::unique_ptr<FOOTPRINT>>&    aFootprintMap,
                     const std::map<wxString, EASYEDAPRO::BLOB>&        aBlobMap,
                     const std::multimap<wxString, EASYEDAPRO::POURED>& aPouredMap, const EASYEDAPRO::V3_DOC_RAW& aDoc,
                     const wxString& aFpLibName );

private:
    BOARD* m_board;

    PCB_IO_EASYEDAPRO_PARSER m_v2Parser;

    std::unique_ptr<PAD> createV3PAD( FOOTPRINT* aFootprint, const EASYEDAPRO::V3_ROW& aRow );
};


#endif // PCB_IO_EASYEDAPRO_V3_PARSER_H_
