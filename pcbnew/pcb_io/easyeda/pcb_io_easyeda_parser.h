/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2023 Alex Shvartzkop <dudesuchamazing@gmail.com>
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

#ifndef PCB_IO_EASYEDA_PARSER_H_
#define PCB_IO_EASYEDA_PARSER_H_

#include <io/easyeda/easyeda_parser_base.h>

#include <cstdint>
#include <cstring>
#include <memory>
#include <vector>
#include <map>
#include <optional>

#include <wx/string.h>
#include <wx/arrstr.h>

#include <json_common.h>
#include <board_item_container.h>

class BOARD;
class PROGRESS_REPORTER;

class PCB_IO_EASYEDA_PARSER : public EASYEDA_PARSER_BASE
{
public:
    explicit PCB_IO_EASYEDA_PARSER( PROGRESS_REPORTER* aProgressReporter );
    ~PCB_IO_EASYEDA_PARSER();

    PCB_LAYER_ID LayerToKi( const wxString& aLayer );

    double ScaleSize( double aValue ) override
    { //
        return KiROUND( ( aValue * 254000.0 ) / 100.0 ) * 100;
    }

    void ParseBoard( BOARD* aBoard, const VECTOR2D& aOrigin,
                     std::map<wxString, std::unique_ptr<FOOTPRINT>>& aFootprintMap,
                     wxArrayString                                   aShapes );

    FOOTPRINT* ParseFootprint( const VECTOR2D& aOrigin, const EDA_ANGLE& aOrientation, int aLayer,
                               BOARD* aParent, std::map<wxString, wxString> aParams,
                               std::map<wxString, std::unique_ptr<FOOTPRINT>>& aFootprintMap,
                               wxArrayString                                   aShapes );

    void ParseToBoardItemContainer( BOARD_ITEM_CONTAINER* aContainer, BOARD* aParent,
                                    std::map<wxString, wxString>                    paramMap,
                                    std::map<wxString, std::unique_ptr<FOOTPRINT>>& aFootprintMap,
                                    wxArrayString                                   shapes );
};


#endif // PCB_IO_EASYEDA_PARSER_H_
