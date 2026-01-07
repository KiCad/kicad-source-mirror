/**
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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

#ifndef PCB_IO_IPC2581_H_
#define PCB_IO_IPC2581_H_

#include <pcb_io/pcb_io.h>
#include <pcb_io/pcb_io_mgr.h>
#include <pcb_io/common/plugin_common_layer_mapping.h>

#include "ipc2581_types.h"

#include <eda_shape.h>
#include <layer_ids.h> // PCB_LAYER_ID
#include <font/font.h>
#include <geometry/shape_segment.h>
#include <stroke_params.h>

#include <wx/xml/xml.h>
#include <memory>
#include <map>
#include <set>

class BOARD;
class BOARD_ITEM;
class EDA_TEXT;
class FOOTPRINT;
class PROGRESS_REPORTER;
class NETINFO_ITEM;
class PAD;
class PADSTACK;
class PCB_SHAPE;
class PCB_VIA;
class PCB_TEXT;
class PROGRESS_REPORTER;
class SHAPE_POLY_SET;
class SHAPE_SEGMENT;

class PCB_IO_IPC2581 : public PCB_IO
{
public:
    PCB_IO_IPC2581() : PCB_IO( wxS( "IPC-2581" ) )
    {
        m_total_bytes = 0;
        m_scale = 1.0;
        m_sigfig = 3;
        m_version = 'B';
        m_enterpriseNode = nullptr;
        m_board = nullptr;
        m_props = nullptr;
        m_shape_user_node = nullptr;
        m_shape_std_node = nullptr;
        m_line_node = nullptr;
        m_last_padstack = nullptr;
        m_backdrill_spec_index = 0;
        m_cad_header_node = nullptr;
        m_progress_reporter = nullptr;
        m_xml_doc = nullptr;
        m_xml_root = nullptr;
    }

    ~PCB_IO_IPC2581() override;

    // BOARD* LoadBoard( const wxString& aFileName, BOARD* aAppendToMe,
    //                   const std::map<std::string, UTF8>* aProperties = nullptr,
    //                   PROJECT* aProject = nullptr ) override;

    void SaveBoard( const wxString& aFileName, BOARD* aBoard,
                    const std::map<std::string, UTF8>* aProperties = nullptr ) override;

    const IO_BASE::IO_FILE_DESC GetBoardFileDesc() const override
    {
        return IO_BASE::IO_FILE_DESC( wxEmptyString, {}, {}, false, false, true );
    }

    const IO_BASE::IO_FILE_DESC GetLibraryDesc() const override
    {
        // No library description for this plugin
        return IO_BASE::IO_FILE_DESC( wxEmptyString, {} );
    }

    std::vector<FOOTPRINT*> GetImportedCachedLibraryFootprints() override;

    long long GetLibraryTimestamp( const wxString& aLibraryPath ) const override
    {
        return 0;
    }

    // Reading currently disabled
    bool CanReadBoard( const wxString& aFileName ) const override
    {
        return false;
    }

    // Reading currently disabled
    bool CanReadFootprint( const wxString& aFileName ) const override
    {
        return false;
    }

    // Reading currently disabled
    bool CanReadLibrary( const wxString& aFileName ) const override
    {
        return false;
    }


private:

    /**
     * Frees the memory allocated for the loaded footprints in #m_loaded_footprints.
     */
    void clearLoadedFootprints();

    /**
     * Creates the XML header for IPC-2581
     */
    wxXmlNode* generateXmlHeader();

    /**
     * Creates the Content section of the XML file.  This holds the overview of
     * the rest of the board data.  Includes references to the step, bom, and layers
     * as well as the content dictionaries
     */
    wxXmlNode* generateContentSection();

    /**
     * Creates the logistical data header.  This section defines the organization and person
     * creating the file.  Can be used for contact information and config management
     */
    wxXmlNode* generateLogisticSection();

    /**
     * Creates the history section.  This section defines the history of the file, the revision
     * number, and the date of the revision as well as software used to create the file.  Optionally,
     * the data could include information about the git revision and tag
     */
    wxXmlNode* generateHistorySection();

    /**
     * Creates the BOM section.  This section defines the BOM data for the board.  This includes
     * the part number, manufacturer, and distributor information for each component on the board.
     */
    wxXmlNode* generateBOMSection( wxXmlNode* aEcadNode );

    /**
     * Creates the ECAD section.  This describes the layout, layers, and design as well as
     * component placement and netlist information
     */
    wxXmlNode* generateEcadSection();

    /**
     * Creates the Approved Vendor List section.  If the user chooses, this will associate
     * BOM items with vendor numbers and names.
     */
    wxXmlNode* generateAvlSection();

    void generateCadLayers( wxXmlNode* aCadLayerNode );

    void generateCadSpecs( wxXmlNode* aCadLayerNode );

    void generateStackup( wxXmlNode* aCadLayerNode );

    void generateDrillLayers( wxXmlNode* aCadLayerNode );

    void generateAuxilliaryLayers( wxXmlNode* aCadLayerNode );

    void generateStepSection( wxXmlNode* aCadNode );

    void generateProfile( wxXmlNode* aStepNode );

    void generateLogicalNets( wxXmlNode* aStepNode );

    void generatePhyNetGroup( wxXmlNode* aStepNode );

    void generateLayerFeatures( wxXmlNode* aStepNode );

    void generateLayerSetDrill( wxXmlNode* aStepNode );

    void generateLayerSetNet( wxXmlNode* aLayerNode, PCB_LAYER_ID aLayer, std::vector<BOARD_ITEM*>& aItems );

    void generateLayerSetAuxilliary( wxXmlNode* aStepNode );

    wxXmlNode* generateContentStackup( wxXmlNode* aContentNode );

    void generateComponents( wxXmlNode* aStepNode );

    void addCadHeader( wxXmlNode* aEcadNode );

    wxXmlNode* addPackage( wxXmlNode* aStepNode, FOOTPRINT* aFootprint );

    void addPad( wxXmlNode* aContentNode, const PAD* aPad, PCB_LAYER_ID aLayer );

    void addVia( wxXmlNode* aContentNode, const PCB_VIA* aVia, PCB_LAYER_ID aLayer );

    void addPadStack( wxXmlNode* aContentNode, const PAD* aPad );

    void addPadStack( wxXmlNode* aContentNode, const PCB_VIA* aVia );

    void ensureBackdrillSpecs( const wxString& aPadstackName, const PADSTACK& aPadstack );

    void addBackdrillSpecRefs( wxXmlNode* aHoleNode, const wxString& aPadstackName );

    void pruneUnusedBackdrillSpecs();

    void addLocationNode( wxXmlNode* aContentNode, double aX, double aY );

    void addLocationNode( wxXmlNode* aContentNode, const PAD& aPad, bool aRelative );

    void addLocationNode( wxXmlNode* aContentNode, const PCB_SHAPE& aShape );

    void addShape( wxXmlNode* aContentNode, const PCB_SHAPE& aShape, bool aInline = false );

    void addShape( wxXmlNode* aContentNode, const PAD& aPad, PCB_LAYER_ID aLayer );

    void addSlotCavity( wxXmlNode* aContentNode, const PAD& aPad, const wxString& aName );

    void addKnockoutText( wxXmlNode* aContentNode, PCB_TEXT* aText );

    void addText( wxXmlNode* aContentNode, EDA_TEXT* aShape, const KIFONT::METRICS& aFontMetrics );

    void addLineDesc( wxXmlNode* aNode, int aWidth, LINE_STYLE aDashType, bool aForce = false );

    void addFillDesc( wxXmlNode* aNode, FILL_T aFillType, bool aForce = false );

    bool addPolygonNode( wxXmlNode* aParentNode, const SHAPE_LINE_CHAIN& aPolygon,
                         FILL_T aFillType = FILL_T::FILLED_SHAPE, int aWidth = 0,
                         LINE_STYLE aDashType = LINE_STYLE::SOLID );

    bool addPolygonCutouts( wxXmlNode* aParentNode, const SHAPE_POLY_SET::POLYGON& aPolygon );

    bool addOutlineNode( wxXmlNode* aParentNode, const SHAPE_POLY_SET& aPolySet, int aWidth = 0,
                         LINE_STYLE aDashType = LINE_STYLE::SOLID );

    bool addContourNode( wxXmlNode* aParentNode, const SHAPE_POLY_SET& aPolySet, int aOutline = 0,
                         FILL_T aFillType = FILL_T::FILLED_SHAPE, int aWidth = 0,
                         LINE_STYLE aDashType = LINE_STYLE::SOLID );

    size_t lineHash( int aWidth, LINE_STYLE aDashType );

    size_t shapeHash( const PCB_SHAPE& aShape );

    wxString sanitizeId( const wxString& aStr ) const;
    wxString genString( const wxString& aStr, const char* aPrefix = nullptr ) const;
    wxString genLayerString( PCB_LAYER_ID aLayer, const char* aPrefix ) const;
    wxString genLayersString( PCB_LAYER_ID aTop, PCB_LAYER_ID aBottom, const char* aPrefix ) const;

    wxString floatVal( double aVal, int aSigFig = -1 ) const;

    wxString pinName( const PAD* aPad ) const;

    wxString componentName( FOOTPRINT* aFootprint );

    void addXY( wxXmlNode* aNode, const VECTOR2I& aVec, const char* aXName = nullptr,
                const char* aYName = nullptr );

    void addAttribute( wxXmlNode* aNode, const wxString& aName, const wxString& aValue );

    wxXmlNode* insertNode( wxXmlNode* aParent, const wxString& aName );

    wxXmlNode* appendNode( wxXmlNode* aParent, const wxString& aName );

    void appendNode( wxXmlNode* aParent, wxXmlNode* aNode );

    void insertNode( wxXmlNode* aParent, wxXmlNode* aNode );

    void insertNodeAfter( wxXmlNode* aPrev, wxXmlNode* aNode );

    void addLayerAttributes( wxXmlNode* aNode, PCB_LAYER_ID aLayer );

    bool isValidLayerFor2581( PCB_LAYER_ID aLayer );
private:

    size_t                  m_total_bytes;  //<! Total number of bytes to be written

    wxString                m_units_str;    //<! Output string for units
    double                  m_scale;        //<! Scale factor from IU to IPC2581 units (mm, micron, in)
    int                     m_sigfig;       //<! Max number of digits past the decimal point
    char                    m_version;      //<! Currently, either 'B' or 'C' for the IPC2581 version
    wxString                m_OEMRef;       //<! If set, field name containing the internal ID of parts
    wxString                m_mpn;          //<! If set, field name containing the manufacturer part number
    wxString                m_mfg;          //<! If set, field name containing the part manufacturer
    wxString                m_distpn;       //<! If set, field name containing the distributor part number
    wxString                m_dist;         //<! If set, field name containing the distributor name

    // Node pointer to the main enterprise node to be used for adding
    // enterprises later when forming the AVL
    wxXmlNode*              m_enterpriseNode;

    BOARD*                  m_board;
    std::vector<FOOTPRINT*> m_loaded_footprints;
    const std::map<std::string, UTF8>*  m_props;

    std::map<size_t, wxString> m_user_shape_dict;   //<! Map between shape hash values and reference id string
    wxXmlNode*                 m_shape_user_node;   //<! Output XML node for reference shapes in UserDict

    std::map<size_t, wxString> m_std_shape_dict;    //<! Map between shape hash values and reference id string
    wxXmlNode*                 m_shape_std_node;    //<! Output XML node for reference shapes in StandardDict

    std::map<size_t, wxString> m_line_dict;         //<! Map between line hash values and reference id string
    wxXmlNode*                 m_line_node;         //<! Output XML node for reference lines in LineDict

    std::map<size_t, wxString> m_padstack_dict;     //<! Map between padstack hash values and reference id string (PADSTACK_##)
    std::vector<wxXmlNode*>    m_padstacks;         //<! Holding vector for padstacks.  These need to be inserted prior to the components
    wxXmlNode*                 m_last_padstack;     //<! Pointer to padstack list where we can insert the VIA padstacks once we process tracks

    std::map<wxString, std::pair<wxString, wxString>> m_padstack_backdrill_specs;
    std::map<wxString, wxXmlNode*>                    m_backdrill_spec_nodes;
    std::set<wxString>                                m_backdrill_spec_used;
    int                                               m_backdrill_spec_index;
    wxXmlNode*                                        m_cad_header_node;

    std::map<size_t, wxString>
            m_footprint_dict; //<! Map between the footprint hash values and reference id string (<fpid>_##)

    std::map<wxString, FOOTPRINT*>
            m_footprint_refdes_dict; //<! Map between sanitized refdes and footprint pointer

    std::map<FOOTPRINT*, wxString>
            m_footprint_refdes_reverse_dict; //<! Reverse lookup for

    std::map<FOOTPRINT*, wxString>
            m_OEMRef_dict; //<! Reverse map from the footprint pointer to the reference id string assigned for components

    std::map<int, std::vector<std::pair<wxString, wxString>>>
            m_net_pin_dict; //<! Map from netcode to the component/pin pairs in the net

    std::map<PCB_LAYER_ID, wxString>
            m_layer_name_map; //<! Mapping layer name in 2581 to the internal layer id

    std::map<std::pair<PCB_LAYER_ID, PCB_LAYER_ID>, std::vector<BOARD_ITEM*>>
            m_drill_layers; //<! Drill sets are output as layers (to/from pairs)

    std::map<std::pair<PCB_LAYER_ID, PCB_LAYER_ID>, std::vector<PAD*>>
            m_slot_holes; //<! Storage vector of slotted holes that need to be output as cutouts

    std::map<std::tuple<auxLayerType, PCB_LAYER_ID, PCB_LAYER_ID>, std::vector<BOARD_ITEM*>>
            m_auxilliary_Layers;

    PROGRESS_REPORTER*      m_progress_reporter;

    mutable std::set<wxString>           m_element_names;   //<! Track generated element names
    mutable std::map<wxString, wxString> m_generated_names; //<! Map input keys to unique names

    std::set<wxUniChar>     m_acceptable_chars;     //<! IPC2581B and C have differing sets of allowed characters in names

    wxXmlDocument*          m_xml_doc;
    wxXmlNode*              m_xml_root;
};

#endif // PCB_IO_IPC2581_H_