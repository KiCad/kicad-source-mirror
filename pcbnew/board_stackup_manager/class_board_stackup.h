/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2019 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2009-2019 KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 3
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/**
 * @file class_board_stackup.h
 */

#ifndef CLASS_BOARD_STACKUP_H
#define CLASS_BOARD_STACKUP_H


#include <vector>
#include <wx/string.h>
#include <layers_id_colors_and_visibility.h>

class BOARD_DESIGN_SETTINGS;
class OUTPUTFORMATTER;

// A enum to manage the different layers inside the stackup layers.
// Note the stackup layers include both dielectric and some layers handled by the board editor
// Therfore a stackup layer item is not exactely like a board layer
enum BOARD_STACKUP_ITEM_TYPE
{
    BS_ITEM_TYPE_UNDEFINED,     // For not yet initialized BOARD_STACKUP_ITEM item
    BS_ITEM_TYPE_COPPER,        // A initialized BOARD_STACKUP_ITEM item for copper layers
    BS_ITEM_TYPE_DIELECTRIC,    // A initialized BOARD_STACKUP_ITEM item for the
                                // dielectric between copper layers
    BS_ITEM_TYPE_SOLDERPASTE,   // A initialized BOARD_STACKUP_ITEM item for solder paste layers
    BS_ITEM_TYPE_SOLDERMASK,    // A initialized BOARD_STACKUP_ITEM item for solder mask layers
    BS_ITEM_TYPE_SILKSCREEN,    // A initialized BOARD_STACKUP_ITEM item for silkscreen layers
};

// A enum to manage edge connector fab info
enum BS_EDGE_CONNECTOR_CONSTRAINTS
{
    BS_EDGE_CONNECTOR_NONE,     // No edge connector in board
    BS_EDGE_CONNECTOR_IN_USE,   // some edge connector in board
    BS_EDGE_CONNECTOR_BEVELLED  // Some connector in board, and the connector must be bevelled
};

/**
 * this class manage one layer needed to make a physical board
 * it can be a solder mask, silk screen, copper or a dielectric
 */
class BOARD_STACKUP_ITEM
{
public:
    BOARD_STACKUP_ITEM( BOARD_STACKUP_ITEM_TYPE aType );
    BOARD_STACKUP_ITEM( BOARD_STACKUP_ITEM& aOther );

    BOARD_STACKUP_ITEM_TYPE m_Type;
    wxString m_TypeName;    /// type name of layer (copper, silk screen, core, prepreg ...)
    wxString m_Material;    /// type of material (has meaning only for dielectric
    int m_DielectricLayerId;/// the "layer" id for dielectric layers, from 1 (top) to 32 (bottom)
    wxString m_Color;       /// mainly for silkscreen and solder mask
    int m_Thickness;        /// the physical layer thickness in internal units
    double m_EpsilonR;      /// For dielectric (and solder mask) the dielectric constant
    double m_LossTangent;   /// For dielectric (and solder mask) the dielectric loss
    PCB_LAYER_ID m_LayerId; /// the layer id (F.Cu to B.Cu, F.Silk, B.silk, F.Mask, B.Mask)
                            /// and UNDEFINED_LAYER (-1) for dielectic layers that are not
                            /// really layers for the board editor

    /// @return true if the layer has a meaningfull Epsilon R parameter
    /// namely dielectric layers: dielectric and solder mask
    bool HasEpsilonRValue();

    /// @return true if the layer has a meaningfull Dielectric Loss parameter
    /// namely dielectric layers: dielectric and solder mask
    bool HasLossTangentValue();

    /// @return true if the material is editable
    bool IsMaterialEditable();

    /// @return true if the color is editable
    bool IsColorEditable();

    /// @return true if Thickness is editable
    bool IsThicknessEditable();
};


/**
 * this class manage the layers needed to make a physical board
 * they are solder mask, silk screen, copper and dielectric
 * Some other layers, used in fabrication, are not managed here because they
 * are not used to make a physical board itself
 * Note also there are a few other parameters realed to the physical stackup,
 * like finish type, impedance control and a few others
 */
class BOARD_STACKUP
{
    // The list of items describing the stackup for fabrication.
    // this is not just copper layers, but also mask dielectric layers
    std::vector<BOARD_STACKUP_ITEM*> m_list;

public:
    /** The name of external copper finish
     */
    wxString m_FinishType;

    /** True if some layers have impedance controlled tracks or have specific
     * constrains for micro-wave applications
     * If the board has dielectric constrains, the .gbrjob will contain
     * info about dielectric constrains: loss tangent and Epsilon rel.
     * If not, these values will be not specified in job file.
     */
    bool m_HasDielectricConstrains;

    /** True if some layers (copper and/or dielectric) have specific thickness
     */
    bool m_HasThicknessConstrains;

    /** If the board has edge connector cards, some constrains can be specifed
     * in job file:
     *  BS_EDGE_CONNECTOR_NONE = no edge connector
     *  BS_EDGE_CONNECTOR_IN_USE = board has edge connectors
     *  BS_EDGE_CONNECTOR_BEVELLED = edge connectors are bevelled
     */
    BS_EDGE_CONNECTOR_CONSTRAINTS m_EdgeConnectorConstraints;

    bool m_CastellatedPads;         ///< True if castellated pads exist
    bool m_EdgePlating;             ///< True if the edge board is plated

public:
    BOARD_STACKUP();
    BOARD_STACKUP( BOARD_STACKUP& aOther );
    BOARD_STACKUP& operator=( const BOARD_STACKUP& aOther );

    ~BOARD_STACKUP() { RemoveAll(); }

    std::vector<BOARD_STACKUP_ITEM*>& GetList() { return m_list; }

    /// @return a reference to the layer aIndex, or nullptr if not exists
    BOARD_STACKUP_ITEM* GetStackupLayer( int aIndex );

    /// Delete all items in list and clear the list
    void RemoveAll();

    /// @return the number of layers in the stackup
    int GetCount() { return (int) m_list.size(); }

    /// @return the board thickness ( in UI) from the thickness of BOARD_STACKUP_ITEM list
    int BuildBoardTicknessFromStackup() const;

    /// Add a new item in stackup layer
    void Add( BOARD_STACKUP_ITEM* aItem ) { m_list.push_back( aItem ); }

    /**
     * Synchronize the BOARD_STACKUP_ITEM* list with the board.
     * Not enabled layers are removed
     * Missing layers are added
     * @param aSettings, is the current board setting.
     * @return true if changes are made
     */
    bool SynchronizeWithBoard( BOARD_DESIGN_SETTINGS* aSettings );

    /**
     * Creates a default stackup, according to the current BOARD_DESIGN_SETTINGS settings.
     * @param aSettings is the current board setting.
     */
    void BuildDefaultStackupList( BOARD_DESIGN_SETTINGS* aSettings );

    /**
     * Writes the stackup info on board file
     * @param aFormatter is the OUTPUTFORMATTER used to create the file
     * @param aBoard is the board
     * @param aNestLevel is the index to nest level to indent the lines in file
     */
    void FormatBoardStackup( OUTPUTFORMATTER* aFormatter,
                             BOARD* aBoard, int aNestLevel ) const;
};


#endif      // #ifndef CLASS_BOARD_STACKUP_H
