/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2024 KiCad Developers, see AUTHORS.txt for contributors.
 * Author: SYSUEric <jzzhuang666@gmail.com>.
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


#ifndef _PCB_IO_ODBPP_H_
#define _PCB_IO_ODBPP_H_

#include <pcb_io/pcb_io.h>
#include <pcb_io/pcb_io_mgr.h>
#include <pcb_io/common/plugin_common_layer_mapping.h>

#include <eda_shape.h>
#include <layer_ids.h> // PCB_LAYER_ID
#include <font/font.h>
#include <geometry/shape_segment.h>
#include <stroke_params.h>
#include <memory>
#include "odb_entity.h"

class BOARD;
class BOARD_ITEM;
class EDA_TEXT;
class FOOTPRINT;
class PROGRESS_REPORTER;
class NETINFO_ITEM;
class PAD;
class PCB_SHAPE;
class PCB_VIA;
class PROGRESS_REPORTER;
class SHAPE_POLY_SET;
class SHAPE_SEGMENT;
class EDA_DATA::SUB_NET;


class PCB_IO_ODBPP : public PCB_IO
{
public:
    PCB_IO_ODBPP() : PCB_IO( wxS( "ODBPlusPlus" ) ) { m_board = nullptr; }

    ~PCB_IO_ODBPP() override;

    void                        SaveBoard( const wxString& aFileName, BOARD* aBoard,
                                           const std::map<std::string, UTF8>* aProperties = nullptr ) override;
    const IO_BASE::IO_FILE_DESC GetBoardFileDesc() const override
    {
        return IO_BASE::IO_FILE_DESC( _HKI( "ODB++ Production File" ), { "ZIP" } );
    }

    const IO_BASE::IO_FILE_DESC GetLibraryDesc() const override
    {
        // No library description for this plugin
        return IO_BASE::IO_FILE_DESC( wxEmptyString, {} );
    }


    std::vector<FOOTPRINT*> GetImportedCachedLibraryFootprints() override;

    long long GetLibraryTimestamp( const wxString& aLibraryPath ) const override { return 0; }

    // Reading currently disabled
    bool CanReadBoard( const wxString& aFileName ) const override { return false; }

    // Reading currently disabled
    bool CanReadFootprint( const wxString& aFileName ) const override { return false; }

    // Reading currently disabled
    bool CanReadLibrary( const wxString& aFileName ) const override { return false; }

public:
    inline std::vector<std::pair<PCB_LAYER_ID, wxString>>& GetLayerNameList()
    {
        return m_layer_name_list;
    }

    inline std::map<PCB_LAYER_ID, std::map<int, std::vector<BOARD_ITEM*>>>& GetLayerElementsMap()
    {
        return m_layer_elements;
    }

    inline std::vector<std::shared_ptr<FOOTPRINT>>& GetLoadedFootprintList()
    {
        return m_loaded_footprints;
    }

    inline std::map<std::pair<PCB_LAYER_ID, PCB_LAYER_ID>, std::vector<BOARD_ITEM*>>&
    GetDrillLayerItemsMap()
    {
        return m_drill_layers;
    }

    inline std::map<std::pair<PCB_LAYER_ID, PCB_LAYER_ID>, std::vector<BOARD_ITEM*>>&
    GetSlotHolesMap()
    {
        return m_slot_holes;
    }

    inline std::map<const PAD*, EDA_DATA::SUB_NET_TOEPRINT*>& GetPadSubnetMap()
    {
        return m_topeprint_subnets;
    }

    inline std::map<std::pair<PCB_LAYER_ID, ZONE*>, EDA_DATA::SUB_NET_PLANE*>& GetPlaneSubnetMap()
    {
        return m_plane_subnets;
    }

    inline std::map<PCB_TRACK*, EDA_DATA::SUB_NET*>& GetViaTraceSubnetMap()
    {
        return m_via_trace_subnets;
    }


    std::shared_ptr<ODB_TREE_WRITER> m_writer;

    bool GenerateFiles( ODB_TREE_WRITER& writer );
    bool ExportODB( const wxString& aFileName );
    void CreateEntity();

    bool CreateDirectories( ODB_TREE_WRITER& writer );

    // Frees the memory allocated for the loaded footprints in #m_loaded_footprints.
    void ClearLoadedFootprints();

    static double      m_scale;
    static double      m_symbolScale;
    static int         m_sigfig;
    static std::string m_unitsStr;

private:
    template <typename T, typename... Args>
    void Make( Args&&... args )
    {
        std::shared_ptr<ODB_ENTITY_BASE> entity =
                std::make_shared<T>( std::forward<Args>( args )... );

        if( entity )
            m_entities.push_back( entity );
    }

    BOARD* m_board;

    std::vector<std::shared_ptr<FOOTPRINT>> m_loaded_footprints;

    std::vector<std::pair<PCB_LAYER_ID, wxString>>
            m_layer_name_list; //<! layer name in matrix entity to the internal layer id

    std::map<std::pair<PCB_LAYER_ID, PCB_LAYER_ID>, std::vector<BOARD_ITEM*>>
            m_drill_layers; //<! Drill sets are output as layers (to/from pairs)

    std::map<std::pair<PCB_LAYER_ID, PCB_LAYER_ID>, std::vector<BOARD_ITEM*>>
            m_slot_holes; //<! Storage vector of slotted holes that need to be output as cutouts

    std::map<PCB_LAYER_ID, std::map<int, std::vector<BOARD_ITEM*>>>
            m_layer_elements; //<! Storage map of layer to element list

    std::map<const PAD*, EDA_DATA::SUB_NET_TOEPRINT*> m_topeprint_subnets;

    std::map<std::pair<PCB_LAYER_ID, ZONE*>, EDA_DATA::SUB_NET_PLANE*> m_plane_subnets;

    std::map<PCB_TRACK*, EDA_DATA::SUB_NET*> m_via_trace_subnets;

    std::vector<std::shared_ptr<ODB_ENTITY_BASE>> m_entities;
};

#endif // _PCB_IO_ODBPP_H_