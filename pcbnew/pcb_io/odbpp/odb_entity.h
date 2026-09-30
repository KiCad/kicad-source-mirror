/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef _ODB_ENTITY_H_
#define _ODB_ENTITY_H_


#include <optional>
#include <vector>
#include <map>
#include <layer_ids.h>
#include <wx/string.h>
#include <iostream>
#include <functional>
#include "odb_feature.h"
#include "odb_util.h"
#include "odb_eda_data.h"
#include "odb_netlist.h"
#include "odb_component.h"


class BOARD;
class ODB_TREE_WRITER;
class BOARD_ITEM;
class BOARD_STACKUP_ITEM;
class PCB_IO_ODBPP;

class ODB_ENTITY_BASE
{
public:
    ODB_ENTITY_BASE( BOARD* aBoard, PCB_IO_ODBPP* aPlugin ) : m_board( aBoard ), m_plugin( aPlugin )
    {
    }

    ODB_ENTITY_BASE() : m_board( nullptr ), m_plugin( nullptr ) {}

    virtual ~ODB_ENTITY_BASE() = default;
    virtual void        GenerateFiles( ODB_TREE_WRITER& writer ) {}
    virtual bool        CreateDirectoryTree( ODB_TREE_WRITER& writer );
    virtual std::string GetEntityName() = 0;
    virtual void        InitEntityData() {}


protected:
    BOARD*                   m_board;
    std::vector<std::string> m_fileName;
    PCB_IO_ODBPP*            m_plugin;
};

enum class ODB_SUBTYPE;
enum class ODB_POLARITY;
enum class ODB_CONTEXT;
enum class ODB_TYPE;

struct ODB_LAYER_NAME
{
    PCB_LAYER_ID                      m_layer = PCB_LAYER_ID::UNDEFINED_LAYER;
    wxString                          m_name;
    ODB_LAYER_ROLE                    m_role = ODB_LAYER_ROLE::BOARD_LAYER;
    std::optional<ODB_AUX_LAYER_TYPE> m_auxType;
    std::optional<ODB_AUX_LAYER_KEY>  m_auxKey;
    const BOARD_STACKUP_ITEM*         m_stackupItem = nullptr; // Owned by the export stackup snapshot
    int                               m_sublayer = 0;
    std::optional<DRILL_SPAN>         m_drillSpan;
    // F_Cu or B_Cu for a component row, which has no board layer of its own
    PCB_LAYER_ID                      m_componentSide = PCB_LAYER_ID::UNDEFINED_LAYER;
};

class ODB_MATRIX_ENTITY : public ODB_ENTITY_BASE
{
public:
    ODB_MATRIX_ENTITY( BOARD* aBoard, PCB_IO_ODBPP* aPlugin ) : ODB_ENTITY_BASE( aBoard, aPlugin )
    {
    }

    virtual ~ODB_MATRIX_ENTITY() = default;

    inline virtual std::string GetEntityName() override { return "matrix"; }

    struct MATRIX_LAYER
    {
        std::optional<std::pair<wxString, wxString>> m_span; // !< start, end
        std::optional<ODB_SUBTYPE>                   m_addType;
        std::optional<ODB_DIELECTRIC_TYPE>           m_diType;
        std::optional<uint32_t>                      m_ref;
        std::optional<uint32_t>                      m_cuTop;
        std::optional<uint32_t>                      m_cuBottom;
        wxString                                     m_dielectricName;
        uint32_t                                     m_uid = 0;

        uint32_t     m_rowNumber;
        wxString     m_layerName;
        ODB_CONTEXT  m_context  = ODB_CONTEXT::BOARD;
        ODB_TYPE     m_type     = ODB_TYPE::UNDEFINED;
        ODB_POLARITY m_polarity = ODB_POLARITY::POSITIVE;

        ODB_LAYER_NAME m_info;

        MATRIX_LAYER( uint32_t aRow, const wxString& aLayerName ) :
                m_rowNumber( aRow ), m_layerName( ODB::GenLegalEntityName( aLayerName ) )
        {
        }
    };

    virtual void GenerateFiles( ODB_TREE_WRITER& writer ) override;
    virtual void InitEntityData() override;
    void         InitMatrixLayerData();
    void         GenerateStackupFile( ODB_TREE_WRITER& writer );

    const std::vector<MATRIX_LAYER>& GetMatrixLayers() const { return m_matrixLayers; }

    void AddStep( const wxString& aStepName );
    void AddMatrixLayerField( MATRIX_LAYER& aMLayer, PCB_LAYER_ID aLayer );
    void AddDrillMatrixLayer();
    void AddRoutMatrixLayer();
    void AddAuxilliaryMatrixLayer();
    void AddCOMPMatrixLayer( PCB_LAYER_ID aCompSide );

    void EnsureUniqueLayerNames();

private:
    std::map<wxString, std::pair<unsigned int, uint32_t>> m_matrixSteps;
    std::vector<MATRIX_LAYER>        m_matrixLayers;
    unsigned int                     m_row = 1;
    unsigned int                     m_col = 1;
    bool                             m_hasTopComp = false;
    bool                             m_hasBotComp = false;
};

class ODB_MISC_ENTITY : public ODB_ENTITY_BASE
{
public:
    ODB_MISC_ENTITY( BOARD* aBoard, PCB_IO_ODBPP* aPlugin );
    virtual ~ODB_MISC_ENTITY() = default;
    inline virtual std::string GetEntityName() override { return "misc"; }

    //TODO
    // bool AddSysAttrFiles();
    virtual void GenerateFiles( ODB_TREE_WRITER& writer ) override;
    void GenerateUserAttrFile( ODB_TREE_WRITER& writer );
    void GenerateAttrListFile( ODB_TREE_WRITER& writer );
    void GenerateMetadataFile( ODB_TREE_WRITER& writer );

private:
    std::vector<std::pair<wxString, wxString>> m_info;
};

class FEATURES_MANAGER;
class ODB_LAYER_ENTITY;
class ODB_STEP_ENTITY : public ODB_ENTITY_BASE
{
public:
    static constexpr const char* STEP_NAME = "pcb";

    ODB_STEP_ENTITY( BOARD* aBoard, PCB_IO_ODBPP* aPlugin ) :
            ODB_ENTITY_BASE( aBoard, aPlugin ), m_profile( nullptr ), m_netlist( aBoard, aPlugin )
    {
    }

    virtual ~ODB_STEP_ENTITY() = default;

    inline virtual std::string GetEntityName() override { return STEP_NAME; }

    void InitEdaData();
    void InitPackage();
    void InitNetListData();
    void MakeLayerEntity();
    bool AddNetList();
    bool AddProfile();
    bool AddStepHeader();

    virtual bool CreateDirectoryTree( ODB_TREE_WRITER& writer ) override;

    virtual void InitEntityData() override;
    void         GenerateLayerFiles( ODB_TREE_WRITER& writer );
    void         GenerateEdaFiles( ODB_TREE_WRITER& writer );
    void         GenerateNetlistsFiles( ODB_TREE_WRITER& writer );
    void         GenerateProfileFile( ODB_TREE_WRITER& writer );
    void         GenerateStepHeaderFile( ODB_TREE_WRITER& writer );
    void         GenerateAttrListFile( ODB_TREE_WRITER& writer );

    virtual void GenerateFiles( ODB_TREE_WRITER& writer ) override;

private:
    // ODB_ATTRLIST m_attrList;
    std::map<wxString, std::shared_ptr<ODB_LAYER_ENTITY>> m_layerEntityMap;
    std::unique_ptr<FEATURES_MANAGER>                     m_profile;

    EDA_DATA                                   m_edaData;
    std::vector<std::pair<wxString, wxString>> m_stephdr;
    ODB_NET_LIST                               m_netlist;
};


class ODB_LAYER_ENTITY : public ODB_ENTITY_BASE
{
public:
    ODB_LAYER_ENTITY( BOARD* aBoard, PCB_IO_ODBPP* aPlugin,
                      const std::map<int, std::vector<BOARD_ITEM*>>& aMap,
                      const ODB_LAYER_NAME& aLayer );

    virtual ~ODB_LAYER_ENTITY() = default;

    inline virtual std::string GetEntityName() override { return "layers"; }
    virtual void               InitEntityData() override;
    void                       InitFeatureData();
    void                       InitDrillData();
    void                       InitRoutData();
    void                       InitAuxilliaryData();

    ODB_COMPONENT& InitComponentData( const FOOTPRINT* aFp, const EDA_DATA::PACKAGE& aPkg );

    void AddLayerFeatures();


    void GenAttrList( ODB_TREE_WRITER& writer );
    void GenComponents( ODB_TREE_WRITER& writer );
    void GenTools( ODB_TREE_WRITER& writer );

    void GenFeatures( ODB_TREE_WRITER& writer );

    virtual void GenerateFiles( ODB_TREE_WRITER& writer ) override;

private:
    std::map<int, std::vector<BOARD_ITEM*>> m_layerItems;
    PCB_LAYER_ID                            m_layerID;
    ODB_LAYER_NAME                          m_layer;
    // ODB_ATTRLIST m_attrList;
    std::optional<ODB_DRILL_TOOLS>    m_tools;
    std::optional<COMPONENTS_MANAGER> m_components;
    std::unique_ptr<FEATURES_MANAGER> m_featuresMgr;
};

class ODB_SYMBOLS_ENTITY : public ODB_ENTITY_BASE
{
public:
    ODB_SYMBOLS_ENTITY() = default;

    virtual ~ODB_SYMBOLS_ENTITY() = default;

    inline virtual std::string GetEntityName() override { return "symbols"; }

    //TODO
    // virtual void GenerateFiles( ODB_TREE_WRITER& writer );
};

class ODB_FONTS_ENTITY : public ODB_ENTITY_BASE
{
public:
    ODB_FONTS_ENTITY() = default;
    virtual ~ODB_FONTS_ENTITY() = default;

    inline virtual std::string GetEntityName() override { return "fonts"; }

    virtual void GenerateFiles( ODB_TREE_WRITER& writer ) override;
};

class ODB_WHEELS_ENTITY : public ODB_ENTITY_BASE
{
public:
    ODB_WHEELS_ENTITY() = default;
    virtual ~ODB_WHEELS_ENTITY() = default;

    inline virtual std::string GetEntityName() override { return "wheels"; }

    // TODO
    // virtual void GenerateFiles( ODB_TREE_WRITER& writer );
};

class ODB_INPUT_ENTITY : public ODB_ENTITY_BASE
{
public:
    ODB_INPUT_ENTITY() = default;
    virtual ~ODB_INPUT_ENTITY() = default;

    inline virtual std::string GetEntityName() override { return "input"; }

    // TODO
    // virtual void GenerateFiles( ODB_TREE_WRITER &writer );
};

class ODB_USER_ENTITY : public ODB_ENTITY_BASE
{
public:
    ODB_USER_ENTITY() = default;
    virtual ~ODB_USER_ENTITY() = default;

    inline virtual std::string GetEntityName() override { return "user"; }

    // TODO
    // virtual void GenerateFiles( ODB_TREE_WRITER &writer );
};

#endif // _ODB_ENTITY_H_
