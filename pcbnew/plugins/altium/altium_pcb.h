/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2019-2020 Thomas Pointhuber <thomas.pointhuber@gmx.at>
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
class PCB_SHAPE;
class FOOTPRINT;
class ZONE;
class PCB_DIMENSION_BASE;
class PROGRESS_REPORTER;


/**
 * Helper method which opens a Altium Board File and parses it.
 *
 * @param aBoard board the pcb should be appended to
 * @param aFileName file name of board file
 * @param aProgressReporter report import progress, might be a nullptr.
 * @param aFileMapping mapping how altium stream names are mapped
 */
void ParseAltiumPcb( BOARD* aBoard, const wxString& aFileName, PROGRESS_REPORTER* aProgressReporter,
                     const std::map<ALTIUM_PCB_DIR, std::string>& aFileMapping );


namespace CFB
{
class CompoundFileReader;
struct COMPOUND_FILE_ENTRY;
} // namespace CFB


// type declaration required for a helper method
class ALTIUM_PCB;
typedef std::function<void( const CFB::CompoundFileReader&, const CFB::COMPOUND_FILE_ENTRY* )>
        PARSE_FUNCTION_POINTER_fp;


class ALTIUM_PCB
{
public:
    explicit ALTIUM_PCB( BOARD* aBoard, PROGRESS_REPORTER* aProgressReporter );
    ~ALTIUM_PCB();

    void Parse( const CFB::CompoundFileReader&               aReader,
                const std::map<ALTIUM_PCB_DIR, std::string>& aFileMapping );

private:
    void checkpoint();

    PCB_LAYER_ID  GetKicadLayer( ALTIUM_LAYER aAltiumLayer ) const;
    int           GetNetCode( uint16_t aId ) const;
    const ARULE6* GetRule( ALTIUM_RULE_KIND aKind, const wxString& aName ) const;
    const ARULE6* GetRuleDefault( ALTIUM_RULE_KIND aKind ) const;

    void ParseFileHeader(
            const CFB::CompoundFileReader& aReader, const CFB::COMPOUND_FILE_ENTRY* aEntry );

    // Text Format
    void ParseBoard6Data(
            const CFB::CompoundFileReader& aReader, const CFB::COMPOUND_FILE_ENTRY* aEntry );
    void ParseClasses6Data(
            const CFB::CompoundFileReader& aReader, const CFB::COMPOUND_FILE_ENTRY* aEntry );
    void ParseComponents6Data(
            const CFB::CompoundFileReader& aReader, const CFB::COMPOUND_FILE_ENTRY* aEntry );
    void ParseDimensions6Data(
            const CFB::CompoundFileReader& aReader, const CFB::COMPOUND_FILE_ENTRY* aEntry );
    void ParseModelsData( const CFB::CompoundFileReader& aReader,
            const CFB::COMPOUND_FILE_ENTRY* aEntry, const wxString& aRootDir );
    void ParseNets6Data(
            const CFB::CompoundFileReader& aReader, const CFB::COMPOUND_FILE_ENTRY* aEntry );
    void ParsePolygons6Data(
            const CFB::CompoundFileReader& aReader, const CFB::COMPOUND_FILE_ENTRY* aEntry );
    void ParseRules6Data(
            const CFB::CompoundFileReader& aReader, const CFB::COMPOUND_FILE_ENTRY* aEntry );

    // Binary Format
    void ParseArcs6Data(
            const CFB::CompoundFileReader& aReader, const CFB::COMPOUND_FILE_ENTRY* aEntry );
    void ParseComponentsBodies6Data(
            const CFB::CompoundFileReader& aReader, const CFB::COMPOUND_FILE_ENTRY* aEntry );
    void ParsePads6Data(
            const CFB::CompoundFileReader& aReader, const CFB::COMPOUND_FILE_ENTRY* aEntry );
    void ParseVias6Data(
            const CFB::CompoundFileReader& aReader, const CFB::COMPOUND_FILE_ENTRY* aEntry );
    void ParseTracks6Data(
            const CFB::CompoundFileReader& aReader, const CFB::COMPOUND_FILE_ENTRY* aEntry );
    void ParseTexts6Data(
            const CFB::CompoundFileReader& aReader, const CFB::COMPOUND_FILE_ENTRY* aEntry );
    void ParseFills6Data(
            const CFB::CompoundFileReader& aReader, const CFB::COMPOUND_FILE_ENTRY* aEntry );
    void ParseBoardRegionsData(
            const CFB::CompoundFileReader& aReader, const CFB::COMPOUND_FILE_ENTRY* aEntry );
    void ParseShapeBasedRegions6Data(
            const CFB::CompoundFileReader& aReader, const CFB::COMPOUND_FILE_ENTRY* aEntry );
    void ParseRegions6Data(
            const CFB::CompoundFileReader& aReader, const CFB::COMPOUND_FILE_ENTRY* aEntry );

    // Helper Functions
    void HelperParseDimensions6Linear( const ADIMENSION6& aElem );
    void HelperParseDimensions6Radial( const ADIMENSION6& aElem );
    void HelperParseDimensions6Leader( const ADIMENSION6& aElem );
    void HelperParseDimensions6Datum( const ADIMENSION6& aElem );
    void HelperParseDimensions6Center( const ADIMENSION6& aElem );

    void HelperParsePad6NonCopper( const APAD6& aElem );

    void HelperCreateBoardOutline( const std::vector<ALTIUM_VERTICE>& aVertices );

    PCB_SHAPE* HelperCreateAndAddDrawsegment( uint16_t aComponent );
    void HelperDrawsegmentSetLocalCoord( PCB_SHAPE* aShape, uint16_t aComponent );

    BOARD*                               m_board;
    std::vector<FOOTPRINT*>              m_components;
    std::vector<ZONE*>                   m_polygons;
    std::vector<PCB_DIMENSION_BASE*>     m_radialDimensions;
    std::map<wxString, wxString>         m_models;
    size_t                               m_num_nets;
    std::map<ALTIUM_LAYER, PCB_LAYER_ID> m_layermap; // used to correctly map copper layers
    std::map<ALTIUM_RULE_KIND, std::vector<ARULE6>> m_rules;

    std::map<ALTIUM_LAYER, ZONE*>        m_outer_plane;

    PROGRESS_REPORTER* m_progressReporter; ///< optional; may be nullptr
    unsigned           m_doneCount;
    unsigned           m_lastProgressCount;
    unsigned           m_totalCount; ///< for progress reporting

    /// Altium stores pour order across all layers
    int m_highest_pour_index;
};


#endif //ALTIUM_PCB_H
