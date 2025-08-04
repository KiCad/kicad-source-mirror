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

#ifndef PCB_IO_EASYEDAPRO_PARSER_H_
#define PCB_IO_EASYEDAPRO_PARSER_H_

#include <io/easyedapro/easyedapro_parser.h>

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
#include <pcb_shape.h>

class BOARD;
class PROGRESS_REPORTER;

class PCB_IO_EASYEDAPRO_PARSER
{
public:
    explicit PCB_IO_EASYEDAPRO_PARSER( BOARD* aBoard, PROGRESS_REPORTER* aProgressReporter );
    ~PCB_IO_EASYEDAPRO_PARSER();

    PCB_LAYER_ID LayerToKi( int aLayer );

    template <typename T>
    static T ScaleSize( T aValue )
    {
        return KiROUND( aValue * 25400.0 / 500.0 ) * 500;
    }

    template <typename T>
    static VECTOR2<T> ScaleSize( VECTOR2<T> aValue )
    {
        return VECTOR2<T>( ScaleSize( aValue.x ), ScaleSize( aValue.y ) );
    }

    template <typename T>
    static VECTOR2<T> ScalePos( VECTOR2<T> aValue )
    {
        return VECTOR2<T>( ScaleSize( aValue.x ), -ScaleSize( aValue.y ) );
    }

    static double Convert( wxString aValue );

    FOOTPRINT* ParseFootprint( const nlohmann::json& aProject, const wxString& aFpUuid,
                               const std::vector<nlohmann::json>& aLines );

    void ParseBoard( BOARD* aBoard, const nlohmann::json& aProject,
                     std::map<wxString, std::unique_ptr<FOOTPRINT>>&    aFootprintMap,
                     const std::map<wxString, EASYEDAPRO::BLOB>&        aBlobMap,
                     const std::multimap<wxString, EASYEDAPRO::POURED>& aPouredMap,
                     const std::vector<nlohmann::json>& aLines, const wxString& aFpLibName );

    std::vector<std::unique_ptr<PCB_SHAPE>> ParsePoly( BOARD_ITEM_CONTAINER* aContainer,
                                                       nlohmann::json polyData, bool aClosed,
                                                       bool aInFill ) const;

    SHAPE_LINE_CHAIN ParseContour( nlohmann::json polyData, bool aInFill,
                                   int aMaxError = SHAPE_ARC::DefaultAccuracyForPCB() ) const;

private:
    BOARD*   m_board;
    VECTOR2D m_relOrigin;

    std::unique_ptr<PAD> createPAD( FOOTPRINT* aFootprint, const nlohmann::json& line );

    void fillFootprintModelInfo( FOOTPRINT* footprint, const wxString& modelUuid,
                                 const wxString& modelTitle, const wxString& modelTransform ) const;
};


#endif // PCB_IO_EASYEDAPRO_PARSER_H_
