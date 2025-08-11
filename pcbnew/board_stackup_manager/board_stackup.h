/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2019 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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
 * @file board_stackup.h
 */

#ifndef BOARD_STACKUP_H
#define BOARD_STACKUP_H


#include <vector>
#include <wx/string.h>
#include <layer_ids.h>
#include <lset.h>
#include <api/serializable.h>

class BOARD;
class BOARD_DESIGN_SETTINGS;
class OUTPUTFORMATTER;

// A enum to manage the different layers inside the stackup layers.
// Note the stackup layers include both dielectric and some layers handled by the board editor
// Therefore a stackup layer item is not exactly like a board layer
enum BOARD_STACKUP_ITEM_TYPE
{
    BS_ITEM_TYPE_UNDEFINED,     // For not yet initialized BOARD_STACKUP_ITEM item
    BS_ITEM_TYPE_COPPER,        // A initialized BOARD_STACKUP_ITEM item for copper layers
    BS_ITEM_TYPE_DIELECTRIC,    // A initialized BOARD_STACKUP_ITEM item for the
                                // dielectric between copper layers
    BS_ITEM_TYPE_SOLDERPASTE,   // A initialized BOARD_STACKUP_ITEM item for solder paste layers
    BS_ITEM_TYPE_SOLDERMASK,    // A initialized BOARD_STACKUP_ITEM item for solder mask layers
                                // note: this is a specialized dielectric material
    BS_ITEM_TYPE_SILKSCREEN,    // A initialized BOARD_STACKUP_ITEM item for silkscreen layers
};

// A enum to manage edge connector fab info
enum BS_EDGE_CONNECTOR_CONSTRAINTS
{
    BS_EDGE_CONNECTOR_NONE,     // No edge connector in board
    BS_EDGE_CONNECTOR_IN_USE,   // some edge connector in board
    BS_EDGE_CONNECTOR_BEVELLED  // Some connector in board, and the connector must be beveled
};


/**
 * A helper class to manage a dielectric layer set of parameters
 */
class DIELECTRIC_PRMS
{
public:
    DIELECTRIC_PRMS() :
        m_Thickness(0), m_ThicknessLocked( false ),
        m_EpsilonR( 1.0 ), m_LossTangent( 0.0 )
    {}

    bool operator==( const DIELECTRIC_PRMS& aOther ) const;
    bool operator!=( const DIELECTRIC_PRMS& aOther ) const { return !operator==( aOther ); }

private:
    friend class BOARD_STACKUP_ITEM;

    wxString m_Material;    /// type of material (for dielectric and solder mask)
    int m_Thickness;        /// the physical layer thickness in internal units
    bool m_ThicknessLocked; /// true for dielectric layers with a fixed thickness
                            /// (for impedance controlled purposes), unused for other layers
    double m_EpsilonR;      /// For dielectric (and solder mask) the dielectric constant
    double m_LossTangent;   /// For dielectric (and solder mask) the dielectric loss
    wxString m_Color;       /// mainly for silkscreen and solder mask
};


/**
 * Manage one layer needed to make a physical board.
 *
 * It can be a solder mask, silk screen, copper or a dielectric.
 */
class BOARD_STACKUP_ITEM
{
public:
    BOARD_STACKUP_ITEM( BOARD_STACKUP_ITEM_TYPE aType );
    BOARD_STACKUP_ITEM( const BOARD_STACKUP_ITEM& aOther );

    bool operator==( const BOARD_STACKUP_ITEM& aOther ) const;
    bool operator!=( const BOARD_STACKUP_ITEM& aOther ) const { return !operator==( aOther ); }

    /**
     * Add (insert) a DIELECTRIC_PRMS item to m_DielectricPrmsList
     * all values are set to default
     * @param aDielectricPrmsIdx is a index in m_DielectricPrmsList
     * the new item will be inserted at this position
     */
    void AddDielectricPrms( int aDielectricPrmsIdx );

    /**
     * Remove a DIELECTRIC_PRMS item from m_DielectricPrmsList
     * @param aDielectricPrmsIdx is the index of the parameters set
     * to remove in m_DielectricPrmsList
     */
    void RemoveDielectricPrms( int aDielectricPrmsIdx );

    /// @return true if the layer has a meaningful Epsilon R parameter
    /// namely dielectric layers: dielectric and solder mask
    bool HasEpsilonRValue() const;

    /// @return true if the layer has a meaningfully Dielectric Loss parameter
    /// namely dielectric layers: dielectric and solder mask
    bool HasLossTangentValue() const;

    /// @return true if the material is specified
    bool HasMaterialValue( int aDielectricSubLayer = 0 ) const;

    /// @return true if the material is editable
    bool IsMaterialEditable() const;

    /// @return true if the color is editable
    bool IsColorEditable() const;

    /// @return true if Thickness is editable
    bool IsThicknessEditable() const;

    /// @return a reasonable default value for a copper layer thickness
    static int GetCopperDefaultThickness();

    /// @return a reasonable default value for a solder mask layer thickness
    static int GetMaskDefaultThickness();

    /// @return a the number of sublayers in a dielectric layer.
    /// the count is >= 1 (there is at least one layer)
    int GetSublayersCount() const { return m_DielectricPrmsList.size(); }

    /// @return a wxString to print/display Epsilon R
    wxString FormatEpsilonR( int aDielectricSubLayer = 0 ) const;

    /// @return a wxString to print/display Loss Tangent
    wxString FormatLossTangent( int aDielectricSubLayer = 0 ) const;

    /// @return a wxString to print/display a dielectric name
    wxString FormatDielectricLayerName() const;

    // Getters:
    bool IsEnabled() const { return m_enabled; }

    BOARD_STACKUP_ITEM_TYPE GetType() const { return m_Type; }
    PCB_LAYER_ID GetBrdLayerId() const { return m_LayerId; }
    wxString GetLayerName() const { return m_LayerName; }
    wxString GetTypeName() const { return m_TypeName; }
    int GetDielectricLayerId() const { return m_DielectricLayerId; }

    wxString GetColor( int aDielectricSubLayer = 0 ) const;
    int GetThickness( int aDielectricSubLayer = 0 ) const;
    bool IsThicknessLocked( int aDielectricSubLayer = 0 ) const;
    double GetEpsilonR( int aDielectricSubLayer = 0 ) const;
    double GetLossTangent( int aDielectricSubLayer = 0 ) const;
    wxString GetMaterial( int aDielectricSubLayer = 0 ) const;

    // Setters:
    void SetEnabled( bool aEnable) { m_enabled = aEnable; }
    void SetBrdLayerId( PCB_LAYER_ID aBrdLayerId ) { m_LayerId = aBrdLayerId; }
    void SetLayerName( const wxString& aName ) { m_LayerName = aName; }
    void SetTypeName( const wxString& aName ) { m_TypeName = aName; }
    void SetDielectricLayerId( int aLayerId ) { m_DielectricLayerId = aLayerId; }

    void SetColor( const wxString& aColorName, int aDielectricSubLayer = 0 );
    void SetThickness( int aThickness, int aDielectricSubLayer = 0 );
    void SetThicknessLocked( bool aLocked, int aDielectricSubLayer = 0 );
    void SetEpsilonR( double aEpsilon, int aDielectricSubLayer = 0 );
    void SetLossTangent( double aTg, int aDielectricSubLayer = 0 );
    void SetMaterial( const wxString& aName, int aDielectricSubLayer = 0 );

private:
    BOARD_STACKUP_ITEM_TYPE m_Type;
    wxString m_LayerName;   /// name of layer as shown in layer manager. Useful to create reports
    wxString m_TypeName;    /// type name of layer (copper, silk screen, core, prepreg ...)
    PCB_LAYER_ID m_LayerId; /// the layer id (F.Cu to B.Cu, F.Silk, B.silk, F.Mask, B.Mask)
                            /// and UNDEFINED_LAYER (-1) for dielectric layers that are not
                            /// really layers for the board editor
    int m_DielectricLayerId;/// the "layer" id for dielectric layers,
                            /// from 1 (top) to 31 (bottom)
                            /// (only 31 dielectric layers for 32 copper layers)
    /// List of dielectric parameters
    /// usually only one item, but in complex (microwave) boards, one can have
    /// more than one dielectric layer between 2 copper layers, and therefore
    /// more than one item in list
    std::vector<DIELECTRIC_PRMS> m_DielectricPrmsList;

    bool m_enabled;         /// true if this stackup item must be taken in account,
                            /// false to ignore it. Mainly used in dialog stackup editor.
};


/**
 * Manage layers needed to make a physical board.
 *
 * They are solder mask, silk screen, copper and dielectric.  Some other layers, used in
 * fabrication, are not managed here because they are not used to make a physical board itself.
 *
 * @note There are a few other parameters related to the physical stackup like finish type,
 *       impedance control and a few others.
 */
class BOARD_STACKUP : public SERIALIZABLE
{
public:
    BOARD_STACKUP();
    BOARD_STACKUP( const BOARD_STACKUP& aOther );
    BOARD_STACKUP& operator=( const BOARD_STACKUP& aOther );

    bool operator==( const BOARD_STACKUP& aOther ) const;
    bool operator!=( const BOARD_STACKUP& aOther ) const { return !operator==( aOther ); }

    ~BOARD_STACKUP() { RemoveAll(); }

    void Serialize( google::protobuf::Any &aContainer ) const override;

    bool Deserialize( const google::protobuf::Any &aContainer ) override;

    const std::vector<BOARD_STACKUP_ITEM*>& GetList() const { return m_list; }

    /// @return a reference to the layer aIndex, or nullptr if not exists
    BOARD_STACKUP_ITEM* GetStackupLayer( int aIndex );

    /**
     * @return the board layers full mask allowed in the stackup list
     * i.e. the SilkS, Mask, Paste and all copper layers
     */
    static LSET StackupAllowedBrdLayers()
    {
        return LSET( { F_SilkS, F_Mask, F_Paste, B_SilkS, B_Mask, B_Paste } )
               | LSET::ExternalCuMask() | LSET::InternalCuMask();
    }


    /// Delete all items in list and clear the list
    void RemoveAll();

    /// @return the number of layers in the stackup
    int GetCount() const { return (int) m_list.size(); }

    /// @return the board thickness ( in UI) from the thickness of BOARD_STACKUP_ITEM list
    int BuildBoardThicknessFromStackup() const;

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
     * Create a default stackup, according to the current BOARD_DESIGN_SETTINGS settings.
     * @param aSettings is the current board setting.
     * if nullptr, build a full stackup (with 32 copper layers)
     * @param aActiveCopperLayersCount is used only if aSettings == nullptr is the number
     * of copper layers to use to calculate a default dielectric thickness.
     * ((<= 0 to use all copper layers)
     */
    void BuildDefaultStackupList( const BOARD_DESIGN_SETTINGS* aSettings,
            int aActiveCopperLayersCount = 0 );

    /**
     * Write the stackup info on board file
     * @param aFormatter is the OUTPUTFORMATTER used to create the file
     * @param aBoard is the board
     */
    void FormatBoardStackup( OUTPUTFORMATTER* aFormatter, const BOARD* aBoard ) const;

    /**
     * Calculate the distance (height) between the two given copper layers.
     *
     * This factors in the thickness of any dielectric and copper layers between the two given
     * layers, and half the height of the given start and end layers.  This half-height calculation
     * allows this to be used for consistent length measurements when calculating net length through
     * a series of vias.  A more advanced algorithm would be possible once we have a good concept of
     * the start and end for a length measurement, but for now this will do.
     * See https://gitlab.com/kicad/code/kicad/-/issues/8384 for more background.
     *
     * @param aFirstLayer is a copper layer
     * @param aSecondLayer is a different copper layer
     * @return the height (in IU) between the two layers
     */
    int GetLayerDistance( PCB_LAYER_ID aFirstLayer, PCB_LAYER_ID aSecondLayer ) const;

    /**
     * The name of external copper finish
     */
    wxString m_FinishType;

    /**
     * True if some layers have impedance controlled tracks or have specific
     * constrains for micro-wave applications
     * If the board has dielectric constrains, the .gbrjob will contain
     * info about dielectric constrains: loss tangent and Epsilon rel.
     * If not, these values will be not specified in job file.
     */
    bool m_HasDielectricConstrains;

    /**
     * True if some layers (copper and/or dielectric) have specific thickness
     */
    bool m_HasThicknessConstrains;

    /**
     * If the board has edge connector cards, some constrains can be specified
     * in job file:
     *  BS_EDGE_CONNECTOR_NONE = no edge connector
     *  BS_EDGE_CONNECTOR_IN_USE = board has edge connectors
     *  BS_EDGE_CONNECTOR_BEVELLED = edge connectors are beveled
     */
    BS_EDGE_CONNECTOR_CONSTRAINTS m_EdgeConnectorConstraints;

    bool m_EdgePlating;             ///< True if the edge board is plated

private:
    // The list of items describing the stackup for fabrication.
    // this is not just copper layers, but also mask dielectric layers
    std::vector<BOARD_STACKUP_ITEM*> m_list;
};


#endif      // BOARD_STACKUP_H
