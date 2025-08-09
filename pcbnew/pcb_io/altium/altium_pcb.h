/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2019-2020 Thomas Pointhuber <thomas.pointhuber@gmx.at>
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

#ifndef ALTIUM_PCB_H
#define ALTIUM_PCB_H

#include <functional>
#include <layer_ids.h>
#include <vector>
#include <pcb_io/common/plugin_common_layer_mapping.h>


#include <altium_parser_pcb.h>


enum class ALTIUM_PCB_DIR
{
    FILE_HEADER,

    ADVANCEDPLACEROPTIONS6,
    ARCS6,
    BOARD6,
    BOARDREGIONS,
    CLASSES6,
    COMPONENTBODIES6,
    COMPONENTS6,
    CONNECTIONS6,
    COORDINATES6,
    DESIGNRULECHECKEROPTIONS6,
    DIFFERENTIALPAIRS6,
    DIMENSIONS6,
    EMBEDDEDBOARDS6,
    EMBEDDEDFONTS6,
    EMBEDDEDS6,
    EXTENDPRIMITIVEINFORMATION,
    FILEVERSIONINFO,
    FILLS6,
    FROMTOS6,
    MODELS,
    MODELSNOEMBED,
    NETS6,
    PADS6,
    PADVIALIBRARY,
    PADVIALIBRARYCACHE,
    PADVIALIBRARYLINKS,
    PINSWAPOPTIONS6,
    PINPAIRSSECTION,
    POLYGONS6,
    REGIONS6,
    RULES6,
    SHAPEBASEDCOMPONENTBODIES6,
    SHAPEBASEDREGIONS6,
    SIGNALCLASSES,
    SMARTUNIONS,
    TEXTS,
    TEXTS6,
    TEXTURES,
    TRACKS6,
    UNIONNAMES,
    UNIQUEIDPRIMITIVEINFORMATION,
    VIAS6,
    WIDESTRINGS6
};


class BOARD;
class FP_SHAPE;
class PCB_SHAPE;
class PCB_TEXTBOX;
class FOOTPRINT;
class ZONE;
class PCB_DIM_RADIAL;
class PROGRESS_REPORTER;

namespace CFB
{
struct COMPOUND_FILE_ENTRY;
} // namespace CFB

class ALTIUM_PCB_COMPOUND_FILE;

// Structure for storing embedded model data
struct ALTIUM_EMBEDDED_MODEL_DATA
{
    wxString m_modelname;
    VECTOR3D m_rotation;
    double   m_z_offset;
    std::vector<char> m_data;

    // Constructor
    ALTIUM_EMBEDDED_MODEL_DATA(const wxString& name, const VECTOR3D& rotation, double z_offset, std::vector<char>&& data)
        : m_modelname(name), m_rotation(rotation), m_z_offset(z_offset), m_data(std::move(data)) {}
};

// type declaration required for a helper method
class ALTIUM_PCB;
typedef std::function<void( const ALTIUM_PCB_COMPOUND_FILE&, const CFB::COMPOUND_FILE_ENTRY* )>
        PARSE_FUNCTION_POINTER_fp;

class ALTIUM_PCB
{
public:
    explicit ALTIUM_PCB( BOARD* aBoard, PROGRESS_REPORTER* aProgressReporter,
                         LAYER_MAPPING_HANDLER& aLayerMappingHandler,
                         REPORTER* aReporter = nullptr,
                         const wxString& aLibrary = wxEmptyString,
                         const wxString& aFootprintName = wxEmptyString);
    ~ALTIUM_PCB();

    void Parse( const ALTIUM_PCB_COMPOUND_FILE&                  aAltiumPcbFile,
                const std::map<ALTIUM_PCB_DIR, std::string>& aFileMapping );

    FOOTPRINT* ParseFootprint( ALTIUM_PCB_COMPOUND_FILE& altiumLibFile,
                               const wxString&       aFootprintName );

private:
    void checkpoint();

    PCB_LAYER_ID  GetKicadLayer( ALTIUM_LAYER aAltiumLayer ) const;
    std::vector<PCB_LAYER_ID> GetKicadLayersToIterate( ALTIUM_LAYER aAltiumLayer ) const;
    int           GetNetCode( uint16_t aId ) const;
    const ARULE6* GetRule( ALTIUM_RULE_KIND aKind, const wxString& aName ) const;
    const ARULE6* GetRuleDefault( ALTIUM_RULE_KIND aKind ) const;

    void ParseFileHeader( const ALTIUM_PCB_COMPOUND_FILE&     aAltiumPcbFile,
                          const CFB::COMPOUND_FILE_ENTRY* aEntry );

    // Text Format
    void ParseBoard6Data( const ALTIUM_PCB_COMPOUND_FILE&     aAltiumPcbFile,
                          const CFB::COMPOUND_FILE_ENTRY* aEntry );
    void ParseClasses6Data( const ALTIUM_PCB_COMPOUND_FILE&     aAltiumPcbFile,
                            const CFB::COMPOUND_FILE_ENTRY* aEntry );
    void ParseComponents6Data( const ALTIUM_PCB_COMPOUND_FILE&     aAltiumPcbFile,
                               const CFB::COMPOUND_FILE_ENTRY* aEntry );
    void ParseDimensions6Data( const ALTIUM_PCB_COMPOUND_FILE&     aAltiumPcbFile,
                               const CFB::COMPOUND_FILE_ENTRY* aEntry );
    void ParseModelsData( const ALTIUM_PCB_COMPOUND_FILE&     aAltiumPcbFile,
                          const CFB::COMPOUND_FILE_ENTRY* aEntry,
                          const std::vector<std::string>& aRootDir );
    void ParseNets6Data( const ALTIUM_PCB_COMPOUND_FILE&     aAltiumPcbFile,
                         const CFB::COMPOUND_FILE_ENTRY* aEntry );
    void ParsePolygons6Data( const ALTIUM_PCB_COMPOUND_FILE&     aAltiumPcbFile,
                             const CFB::COMPOUND_FILE_ENTRY* aEntry );
    void ParseRules6Data( const ALTIUM_PCB_COMPOUND_FILE&     aAltiumPcbFile,
                          const CFB::COMPOUND_FILE_ENTRY* aEntry );

    // Binary Format
    void ParseArcs6Data( const ALTIUM_PCB_COMPOUND_FILE&     aAltiumPcbFile,
                         const CFB::COMPOUND_FILE_ENTRY* aEntry );
    void ConvertArcs6ToPcbShape( const AARC6& aElem, PCB_SHAPE* aShape );
    void ConvertArcs6ToBoardItem( const AARC6& aElem, const int aPrimitiveIndex );
    void ConvertArcs6ToFootprintItem( FOOTPRINT* aFootprint, const AARC6& aElem,
                                      const int aPrimitiveIndex, const bool aIsBoardImport );
    void ConvertArcs6ToBoardItemOnLayer( const AARC6& aElem, PCB_LAYER_ID aLayer );
    void ConvertArcs6ToFootprintItemOnLayer( FOOTPRINT* aFootprint, const AARC6& aElem,
                                             PCB_LAYER_ID aLayer );
    void ParseComponentsBodies6Data( const ALTIUM_PCB_COMPOUND_FILE&     aAltiumPcbFile,
                                     const CFB::COMPOUND_FILE_ENTRY* aEntry );
    void ConvertComponentBody6ToFootprintItem( const ALTIUM_PCB_COMPOUND_FILE& aAltiumPcbFile,
                                              FOOTPRINT* aFootprint,
                                              const ACOMPONENTBODY6& aElem );
    void ParsePads6Data( const ALTIUM_PCB_COMPOUND_FILE&     aAltiumPcbFile,
                         const CFB::COMPOUND_FILE_ENTRY* aEntry );
    void ConvertPads6ToBoardItem( const APAD6& aElem );
    void ConvertPads6ToFootprintItem( FOOTPRINT* aFootprint, const APAD6& aElem );
    void ConvertPads6ToBoardItemOnNonCopper( const APAD6& aElem );
    void ConvertPads6ToFootprintItemOnCopper( FOOTPRINT* aFootprint, const APAD6& aElem );
    void ConvertPads6ToFootprintItemOnNonCopper( FOOTPRINT* aFootprint, const APAD6& aElem );
    void ParseVias6Data( const ALTIUM_PCB_COMPOUND_FILE&     aAltiumPcbFile,
                         const CFB::COMPOUND_FILE_ENTRY* aEntry );
    void ConvertVias6ToFootprintItem( FOOTPRINT* aFootprint, const AVIA6& aElem );
    void ParseTracks6Data( const ALTIUM_PCB_COMPOUND_FILE&     aAltiumPcbFile,
                           const CFB::COMPOUND_FILE_ENTRY* aEntry );
    void ConvertTracks6ToBoardItem( const ATRACK6& aElem, const int aPrimitiveIndex );
    void ConvertTracks6ToFootprintItem( FOOTPRINT* aFootprint, const ATRACK6& aElem,
                                        const int aPrimitiveIndex, const bool aIsBoardImport );
    void ConvertTracks6ToBoardItemOnLayer( const ATRACK6& aElem, PCB_LAYER_ID aLayer );
    void ConvertTracks6ToFootprintItemOnLayer( FOOTPRINT* aFootprint, const ATRACK6& aElem,
                                               PCB_LAYER_ID aLayer );
    void ParseTexts6Data( const ALTIUM_PCB_COMPOUND_FILE&     aAltiumPcbFile,
                          const CFB::COMPOUND_FILE_ENTRY* aEntry );
    void ConvertTexts6ToBoardItem( const ATEXT6& aElem );
    void ConvertTexts6ToFootprintItem( FOOTPRINT* aFootprint, const ATEXT6& aElem );
    void ConvertTexts6ToBoardItemOnLayer( const ATEXT6& aElem, PCB_LAYER_ID aLayer );
    void ConvertTexts6ToFootprintItemOnLayer( FOOTPRINT* aFootprint, const ATEXT6& aElem,
                                              PCB_LAYER_ID aLayer );
    void ConvertBarcodes6ToBoardItemOnLayer( const ATEXT6& aElem, PCB_LAYER_ID aLayer );
    void ConvertBarcodes6ToFootprintItemOnLayer( FOOTPRINT* aFootprint, const ATEXT6& aElem,
                                                 PCB_LAYER_ID aLayer );
    void ConvertTexts6ToEdaTextSettings( const ATEXT6& aElem, EDA_TEXT& aEdaText );
    void ParseFills6Data( const ALTIUM_PCB_COMPOUND_FILE&     aAltiumPcbFile,
                          const CFB::COMPOUND_FILE_ENTRY* aEntry );
    void ConvertFills6ToBoardItem( const AFILL6& aElem );
    void ConvertFills6ToFootprintItem( FOOTPRINT* aFootprint, const AFILL6& aElem,
                                       const bool aIsBoardImport );
    void ConvertFills6ToBoardItemOnLayer( const AFILL6& aElem, PCB_LAYER_ID aLayer );
    void ConvertFills6ToFootprintItemOnLayer( FOOTPRINT* aFootprint, const AFILL6& aElem,
                                              PCB_LAYER_ID aLayer );
    void ParseBoardRegionsData( const ALTIUM_PCB_COMPOUND_FILE&     aAltiumPcbFile,
                                const CFB::COMPOUND_FILE_ENTRY* aEntry );
    void ParseShapeBasedRegions6Data( const ALTIUM_PCB_COMPOUND_FILE&     aAltiumPcbFile,
                                      const CFB::COMPOUND_FILE_ENTRY* aEntry );
    void ConvertShapeBasedRegions6ToBoardItem( const AREGION6& aElem );
    void ConvertShapeBasedRegions6ToFootprintItem( FOOTPRINT* aFootprint, const AREGION6& aElem,
                                                   const int aPrimitiveIndex );
    void ConvertShapeBasedRegions6ToBoardItemOnLayer( const AREGION6& aElem, PCB_LAYER_ID aLayer );
    void ConvertShapeBasedRegions6ToFootprintItemOnLayer( FOOTPRINT*      aFootprint,
                                                          const AREGION6& aElem,
                                                          PCB_LAYER_ID    aLayer,
                                                          const int       aPrimitiveIndex );
    void ParseExtendedPrimitiveInformationData( const ALTIUM_PCB_COMPOUND_FILE&     aAltiumPcbFile,
                                                const CFB::COMPOUND_FILE_ENTRY* aEntry );
    void ParseRegions6Data( const ALTIUM_PCB_COMPOUND_FILE&     aAltiumPcbFile,
                            const CFB::COMPOUND_FILE_ENTRY* aEntry );
    void ParseWideStrings6Data( const ALTIUM_PCB_COMPOUND_FILE&     aAltiumPcbFile,
                                const CFB::COMPOUND_FILE_ENTRY* aEntry );

    // Helper Functions
    void HelperParseDimensions6Linear( const ADIMENSION6& aElem );
    void HelperParseDimensions6Radial( const ADIMENSION6& aElem );
    void HelperParseDimensions6Leader( const ADIMENSION6& aElem );
    void HelperParseDimensions6Datum( const ADIMENSION6& aElem );
    void HelperParseDimensions6Center( const ADIMENSION6& aElem );

    void HelperParsePad6NonCopper( const APAD6& aElem, PCB_LAYER_ID aLayer, PCB_SHAPE* aShape );

    void HelperCreateBoardOutline( const std::vector<ALTIUM_VERTICE>& aVertices );

    void HelperSetZoneLayers( ZONE& aZone, const ALTIUM_LAYER aAltiumLayer );
    void HelperSetZoneKeepoutRestrictions( ZONE& aZone, const uint8_t aKeepoutRestrictions );
    void HelperPcpShapeAsBoardKeepoutRegion( const PCB_SHAPE&   aShape,
                                             const ALTIUM_LAYER aAltiumLayer,
                                             const uint8_t      aKeepoutRestrictions );
    void HelperPcpShapeAsFootprintKeepoutRegion( FOOTPRINT* aFootprint, const PCB_SHAPE& aShape,
                                                 const ALTIUM_LAYER aAltiumLayer,
                                                 const uint8_t      aKeepoutRestrictions );

    void HelperSetTextboxAlignmentAndPos( const ATEXT6& aElem, PCB_TEXTBOX* aPcbTextbox );
    void HelperSetTextAlignmentAndPos( const ATEXT6& aElem, EDA_TEXT* aEdaText );

    std::vector<std::pair<PCB_LAYER_ID, int>>
    HelperGetSolderAndPasteMaskExpansions( const ALTIUM_RECORD aType, const int aPrimitiveIndex,
                                           const ALTIUM_LAYER aAltiumLayer );

    FOOTPRINT* HelperGetFootprint( uint16_t aComponent ) const;

    void remapUnsureLayers( std::vector<ABOARD6_LAYER_STACKUP>& aStackup );

    BOARD*                               m_board;
    std::vector<FOOTPRINT*>              m_components;
    std::vector<ZONE*>                   m_polygons;
    std::vector<PCB_DIM_RADIAL*>         m_radialDimensions;
    std::map<uint32_t, wxString>         m_unicodeStrings;
    std::vector<int>                     m_altiumToKicadNetcodes;
    std::map<ALTIUM_LAYER, PCB_LAYER_ID> m_layermap; // used to correctly map copper layers
    std::map<ALTIUM_LAYER, wxString>     m_layerNames;

    std::map<wxString, ALTIUM_EMBEDDED_MODEL_DATA>  m_EmbeddedModels;
    std::map<ALTIUM_RULE_KIND, std::vector<ARULE6>> m_rules;
    std::map<ALTIUM_RECORD, std::multimap<int, const AEXTENDED_PRIMITIVE_INFORMATION>>
            m_extendedPrimitiveInformationMaps;

    std::map<ALTIUM_LAYER, ZONE*>        m_outer_plane;

    LAYER_MAPPING_HANDLER   m_layerMappingHandler;

    PROGRESS_REPORTER* m_progressReporter;   ///< optional; may be nullptr
    REPORTER*          m_reporter;           ///< optional; may be nullptr
    unsigned           m_doneCount;
    unsigned           m_lastProgressCount;
    unsigned           m_totalCount;         ///< for progress reporting

    wxString           m_library;            ///< for footprint library loading error reporting
    wxString           m_footprintName;      ///< for footprint library loading error reporting

    /// Altium stores pour order across all layers
    int m_highest_pour_index;
};


#endif //ALTIUM_PCB_H
