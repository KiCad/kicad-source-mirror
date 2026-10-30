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

#ifndef TARGET3001_PCB_BUILDER_H
#define TARGET3001_PCB_BUILDER_H

#include <map>
#include <memory>
#include <optional>
#include <set>
#include <vector>

#include <geometry/shape_poly_set.h>
#include <layer_ids.h>
#include <lset.h>
#include <math/box2.h>
#include <math/vector2d.h>
#include <wx/string.h>

#include <io/target3001/target3001_fonts.h>
#include <io/target3001/target3001_parser.h>

class BOARD;
class BOARD_CONNECTED_ITEM;
class BOARD_ITEM;
class BOARD_ITEM_CONTAINER;
class FOOTPRINT;
class NETINFO_ITEM;
class PAD;
class PCB_SHAPE;
class PCB_TEXT;
class REPORTER;
class ZONE;


/**
 * Builds a KiCad board from a parsed TARGET 3001! project.
 *
 * Board items are stored in absolute board coordinates in the project, including the pads and
 * graphics that belong to a footprint, so footprints are placed first and their items added at
 * their stored positions.
 */
class TARGET3001_PCB_BUILDER
{
public:
    TARGET3001_PCB_BUILDER( BOARD* aBoard, const TARGET3001::PROJECT& aProject, REPORTER* aReporter );

    void Build();

    /// Custom rules that carry what KiCad cannot store on the items themselves, empty when none apply
    const wxString& DesignRules() const { return m_designRules; }

    /// Function of a TARGET 3001! board layer in this project
    TARGET3001::LAYER_FUNCTION LayerFunction( int aLayer ) const;

private:
    void buildLayerMap();
    void buildNets();
    void buildComponents();
    void buildSignalCopper();

    /// Turn the spokes an old style area stores into thermal reliefs on the pads they start in
    void buildAreaSpokes( const std::vector<const TARGET3001::ELEMENT*>& aSpokes );

    void buildFreeItems();
    void buildPourAreas();
    void buildPolygonPours();

    /// Thermal gaps the stored fill leaves around spoked pads, as zone defaults and pad or footprint overrides
    void buildThermalGaps();

    void addFootprint( const TARGET3001::ELEMENT& aComponent );

    /// Add one board element to @a aParent; footprint items are passed with their footprint
    void addItem( const TARGET3001::ELEMENT& aElem, BOARD_ITEM_CONTAINER* aParent, FOOTPRINT* aFootprint,
                  bool aIsTrack );

    /// Add the graphic or track form of @a aElem on one KiCad layer
    void addShape( const TARGET3001::ELEMENT& aElem, BOARD_ITEM_CONTAINER* aParent, FOOTPRINT* aFootprint,
                   bool aIsTrack, PCB_LAYER_ID aLayer );

    /// Strokes and discs that draw a fiducial mark, not yet assigned a layer
    std::vector<std::unique_ptr<PCB_SHAPE>> fiducialShapes( const TARGET3001::ELEMENT& aElem,
                                                            BOARD_ITEM_CONTAINER* aParent ) const;

    void addPad( const TARGET3001::ELEMENT& aElem, FOOTPRINT* aFootprint );
    void addVia( const TARGET3001::ELEMENT& aElem );
    void addText( const TARGET3001::ELEMENT& aElem, BOARD_ITEM_CONTAINER* aParent, FOOTPRINT* aFootprint,
                  PCB_LAYER_ID aLayer );
    void configureText( PCB_TEXT* aText, const TARGET3001::ELEMENT& aElem, PCB_LAYER_ID aLayer );

    /// Copper or graphic layer for a board element, if KiCad has a place for it
    std::optional<PCB_LAYER_ID> mapLayer( int aLayer ) const;

    /// Copper layer that an area or deletion layer applies to
    std::optional<PCB_LAYER_ID> pourCopperLayer( int aLayer ) const;

    /// Copper layer of a signal polygon that can become a zone
    std::optional<PCB_LAYER_ID> polygonLayer( const TARGET3001::POLYGON_POUR& aPoly ) const;

    bool isCopper( int aLayer ) const;

    /// Clearance rules between copper areas and each signal's tracks and vias
    void buildAreaClearanceRules();

    /// Rules that keep areas the aura distance from pads and vias, except on their NO_AURA layers
    void buildAuraRules();

    /// Rules for the board contour and foreign polygon distances each signal polygon keeps
    void buildPolygonRules();

    /// Tag a pad or via with its aura so the aura rules can match it
    void tagAura( BOARD_CONNECTED_ITEM& aItem, const TARGET3001::ELEMENT_BASE& aBase, int aAura );

    NETINFO_ITEM* net( uint16_t aSignal ) const;

    VECTOR2I toKi( int32_t aX, int32_t aY ) const { return VECTOR2I( aX, -aY ); }

    /// Outline of a filled element (rectangle, triangle, disc, stroked line or arc) in KiCad coordinates
    bool elementOutline( const TARGET3001::ELEMENT& aElem, SHAPE_POLY_SET& aOut ) const;

    /// Grow the board contour box by the defining points of an element, ignoring line widths
    void mergeContour( const TARGET3001::ELEMENT& aElem );

    /// Copper, mask and paste layers of the padstack a pad layer >= 101 selects
    std::optional<LSET> padstackLayers( int aLayer ) const;

    /// Cut @a aItem, grown per layer, from the pours of the deletion layers padstack @a aPadstack lists
    void addPadstackCuts( const BOARD_ITEM& aItem, int aPadstack );

    struct POUR_AREA
    {
        uint16_t       signal;
        PCB_LAYER_ID   layer;
        VECTOR2I       anchor;
        SHAPE_POLY_SET outline;
    };

    BOARD*                     m_board;
    const TARGET3001::PROJECT& m_project;
    REPORTER*                  m_reporter;

    std::map<int, PCB_LAYER_ID>              m_layerMap;
    std::vector<int>                         m_copperStack;     ///< TARGET layers, top to bottom
    std::map<uint16_t, NETINFO_ITEM*>        m_nets;            ///< by signal ID
    int                                      m_freePadCount = 0;
    std::map<std::pair<int, int>, SHAPE_POLY_SET> m_copperFills; ///< (net, KiCad layer) -> filled copper
    std::vector<POUR_AREA>                   m_pourAreas;
    std::optional<int>                       m_contourLayer;    ///< first TARGET layer holding the contour
    BOX2I                                    m_contourBox;
    bool                                     m_contourCarvesNothing = false; ///< a contour stroke has no clearance
    std::map<int, SHAPE_POLY_SET>            m_pourKeepouts;    ///< KiCad layer -> pour keepout
    std::map<int64_t, SHAPE_POLY_SET>        m_polygonFills;    ///< POLYGONID -> stored fill without spokes
    std::map<int64_t, SHAPE_POLY_SET>        m_polygonSpokes;   ///< POLYGONID -> stored thermal spokes
    std::vector<std::pair<ZONE*, int64_t>>   m_polygonZones;    ///< thermal polygon zones and their POLYGONID
    std::set<int>                            m_polygonEdgeClearances;
    std::set<int>                            m_polygonAuras;
    bool                                     m_hasPolygonPours = false;

    /// Track and via clearances counted per signal, then per clearance value
    std::map<uint16_t, std::map<int, int>>   m_signalClearances;
    std::set<int>                            m_auraValues;      ///< distinct aura distances above zero
    std::set<PCB_LAYER_ID>                   m_noAuraLayers;    ///< copper layers some NO_AURA list names
    std::set<std::pair<uint16_t, PCB_LAYER_ID>> m_pourLayers; ///< (signal, KiCad layer) holding an area
    wxString                                 m_designRules;
    TARGET3001::FONTS                        m_fonts;
};

#endif // TARGET3001_PCB_BUILDER_H
