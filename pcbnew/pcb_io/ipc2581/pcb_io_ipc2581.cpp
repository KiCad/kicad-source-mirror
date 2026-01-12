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

#include "pcb_io_ipc2581.h"
#include "ipc2581_types.h"

#include <base_units.h>
#include <bezier_curves.h>
#include <board.h>
#include <board_design_settings.h>
#include <board_stackup_manager/stackup_predefined_prms.h>
#include <build_version.h>
#include <callback_gal.h>
#include <connectivity/connectivity_data.h>
#include <connectivity/connectivity_algo.h>
#include <convert_basic_shapes_to_polygon.h>
#include <font/font.h>
#include <hash_eda.h>
#include <pcb_dimension.h>
#include <pcb_textbox.h>
#include <pgm_base.h>
#include <progress_reporter.h>
#include <settings/settings_manager.h>
#include <string_utils.h>
#include <wx_fstream_progress.h>

#include <geometry/shape_line_chain.h>
#include <geometry/shape_poly_set.h>
#include <geometry/shape_segment.h>

#include <wx/log.h>
#include <wx/numformatter.h>
#include <wx/xml/xml.h>


/**
 * Flag to enable IPC-2581 debugging output.
 *
 * @ingroup trace_env_vars
 */
static const wxChar traceIpc2581[] = wxT( "KICAD_IPC_2581" );


/**
 * Map KiCad surface finish strings to IPC-6012 surfaceFinishType enum.
 */
static const std::map<wxString, surfaceFinishType> surfaceFinishMap =
{
    { wxEmptyString,             surfaceFinishType::NONE },
    { wxT( "ENIG" ),             surfaceFinishType::ENIG_N },
    { wxT( "ENEPIG" ),           surfaceFinishType::ENEPIG_N },
    { wxT( "HAL SNPB" ),         surfaceFinishType::S },
    { wxT( "HAL LEAD-FREE" ),    surfaceFinishType::S },
    { wxT( "HARD GOLD" ),        surfaceFinishType::G },
    { wxT( "IMMERSION TIN" ),    surfaceFinishType::ISN },
    { wxT( "IMMERSION NICKEL" ), surfaceFinishType::N },
    { wxT( "IMMERSION SILVER" ), surfaceFinishType::IAG },
    { wxT( "IMMERSION GOLD" ),   surfaceFinishType::DIG },
    { wxT( "HT_OSP" ),           surfaceFinishType::HT_OSP },
    { wxT( "OSP" ),              surfaceFinishType::OSP },
    { wxT( "NONE" ),             surfaceFinishType::NONE },
    { wxT( "NOT SPECIFIED" ),    surfaceFinishType::NONE },
    { wxT( "USER DEFINED" ),     surfaceFinishType::NONE },
};


/**
 * Map surfaceFinishType enum to IPC-2581 XML string values.
 */
static const std::map<surfaceFinishType, wxString> surfaceFinishTypeToString =
{
    { surfaceFinishType::ENIG_N,   wxT( "ENIG-N" ) },
    { surfaceFinishType::ENEPIG_N, wxT( "ENEPIG-N" ) },
    { surfaceFinishType::OSP,      wxT( "OSP" ) },
    { surfaceFinishType::HT_OSP,   wxT( "HT_OSP" ) },
    { surfaceFinishType::IAG,      wxT( "IAg" ) },
    { surfaceFinishType::ISN,      wxT( "ISn" ) },
    { surfaceFinishType::G,        wxT( "G" ) },
    { surfaceFinishType::N,        wxT( "N" ) },
    { surfaceFinishType::DIG,      wxT( "DIG" ) },
    { surfaceFinishType::S,        wxT( "S" ) },
    { surfaceFinishType::OTHER,    wxT( "OTHER" ) },
};


static surfaceFinishType getSurfaceFinishType( const wxString& aFinish )
{
    auto it = surfaceFinishMap.find( aFinish.Upper() );
    return ( it != surfaceFinishMap.end() ) ? it->second : surfaceFinishType::OTHER;
}


PCB_IO_IPC2581::~PCB_IO_IPC2581()
{
    clearLoadedFootprints();
    delete m_xml_doc;
}


void PCB_IO_IPC2581::clearLoadedFootprints()
{
    for( FOOTPRINT* fp : m_loaded_footprints )
        delete fp;

    m_loaded_footprints.clear();
}


std::vector<FOOTPRINT*> PCB_IO_IPC2581::GetImportedCachedLibraryFootprints()
{
    std::vector<FOOTPRINT*> retval;

    for( FOOTPRINT* fp : m_loaded_footprints )
        retval.push_back( static_cast<FOOTPRINT*>( fp->Clone() ) );

    return retval;
}


void PCB_IO_IPC2581::insertNode( wxXmlNode* aParent, wxXmlNode* aNode )
{
    // insertNode places the node at the start of the list of children

    if( aParent->GetChildren() )
        aNode->SetNext( aParent->GetChildren() );
    else
        aNode->SetNext( nullptr );

    aParent->SetChildren( aNode );
    aNode->SetParent( aParent );
    m_total_bytes += 2 * aNode->GetName().size() + 5;
}


void PCB_IO_IPC2581::insertNodeAfter( wxXmlNode* aPrev, wxXmlNode* aNode )
{
    // insertNode places the node directly after aPrev

    aNode->SetNext( aPrev->GetNext() );
    aPrev->SetNext( aNode );
    aNode->SetParent( aPrev->GetParent() );
    m_total_bytes += 2 * aNode->GetName().size() + 5;
}


wxXmlNode* PCB_IO_IPC2581::insertNode( wxXmlNode* aParent, const wxString& aName )
{
    // Opening tag, closing tag, brackets and the closing slash
    m_total_bytes += 2 * aName.size() + 5;
    wxXmlNode* node = new wxXmlNode( wxXML_ELEMENT_NODE, aName );
    insertNode( aParent, node );
    return node;
}


void PCB_IO_IPC2581::appendNode( wxXmlNode* aParent, wxXmlNode* aNode )
{
    // AddChild iterates through the entire list of children, so we want to avoid
    // that if possible.  When we share a parent and our next sibling is null,
    // then we are the last child and can just append to the end of the list.

    static wxXmlNode* lastNode = nullptr;

    if( lastNode && lastNode->GetParent() == aParent && lastNode->GetNext() == nullptr )
    {
        aNode->SetParent( aParent );
        lastNode->SetNext( aNode );
    }
    else
    {
        aParent->AddChild( aNode );
    }

    lastNode = aNode;

    // Opening tag, closing tag, brackets and the closing slash
    m_total_bytes += 2 * aNode->GetName().size() + 5;
}


wxXmlNode* PCB_IO_IPC2581::appendNode( wxXmlNode* aParent, const wxString& aName )
{
    wxXmlNode* node = new wxXmlNode( wxXML_ELEMENT_NODE, aName );

    appendNode( aParent, node );
    return node;
}


wxString PCB_IO_IPC2581::sanitizeId( const wxString& aStr ) const
{
    wxString str;

    if( m_version == 'C' )
    {
        str = aStr;
        str.Replace( wxT( ":" ), wxT( "_" ) );
    }
    else
    {
        for( wxString::const_iterator iter = aStr.begin(); iter != aStr.end(); ++iter )
        {
            if( !m_acceptable_chars.count( *iter ) )
                str.Append( '_' );
            else
                str.Append( *iter );
        }
    }

    return str;
}


wxString PCB_IO_IPC2581::genString( const wxString& aStr, const char* aPrefix ) const
{
    // Build a key using the prefix and original string so that repeated calls for the same
    // element return the same generated name.
    wxString key = aPrefix ? wxString( aPrefix ) + wxT( ":" ) + aStr : aStr;

    auto it = m_generated_names.find( key );

    if( it != m_generated_names.end() )
        return it->second;

    wxString str = sanitizeId( aStr );

    wxString base = str;
    wxString name = base;
    int      suffix = 1;

    while( m_element_names.count( name ) )
        name = wxString::Format( "%s_%d", base, suffix++ );

    m_element_names.insert( name );
    m_generated_names[key] = name;

    return name;
}


wxString PCB_IO_IPC2581::genLayerString( PCB_LAYER_ID aLayer, const char* aPrefix ) const
{
    return genString( m_board->GetLayerName( aLayer ), aPrefix );
}


wxString PCB_IO_IPC2581::genLayersString( PCB_LAYER_ID aTop, PCB_LAYER_ID aBottom,
                                          const char* aPrefix ) const
{
    return genString( wxString::Format( wxS( "%s_%s" ),
                                        m_board->GetLayerName( aTop ),
                                        m_board->GetLayerName( aBottom ) ), aPrefix );
}


wxString PCB_IO_IPC2581::pinName( const PAD* aPad ) const
{
    wxString name = aPad->GetNumber();

    FOOTPRINT* fp = aPad->GetParentFootprint();
    size_t ii = 0;

    if( name.empty() && fp )
    {
        for( ii = 0; ii < fp->GetPadCount(); ++ii )
        {
            if( fp->Pads()[ii] == aPad )
                break;
        }
    }

    // Pins are required to have names, so if our pad doesn't have a name, we need to
    // generate one that is unique
    if( aPad->GetAttribute() == PAD_ATTRIB::NPTH )
        name = wxString::Format( "NPTH%zu", ii );
    else if( name.empty() )
        name = wxString::Format( "PAD%zu", ii );

    // Pins are scoped per-package, so we only sanitize; uniqueness is handled by
    // the per-package pin_nodes map in addPackage().
    return sanitizeId( name );
}


wxString PCB_IO_IPC2581::componentName( FOOTPRINT* aFootprint )
{
    auto tryInsert =
            [&]( const wxString& aName )
            {
                if( m_footprint_refdes_dict.count( aName ) )
                {
                    if( m_footprint_refdes_dict.at( aName ) != aFootprint )
                        return false;
                }
                else
                {
                    m_footprint_refdes_dict.insert( { aName, aFootprint } );
                }

                return true;
            };

    if( m_footprint_refdes_reverse_dict.count( aFootprint ) )
        return m_footprint_refdes_reverse_dict.at( aFootprint );

    wxString baseName = genString( aFootprint->GetReference(), "CMP" );
    wxString name = baseName;
    int      suffix = 1;

    while( !tryInsert( name ) )
        name = wxString::Format( "%s_%d", baseName, suffix++ );

    m_footprint_refdes_reverse_dict[aFootprint] = name;

    return name;
}


wxString PCB_IO_IPC2581::floatVal( double aVal, int aSigFig ) const
{
    wxString str = wxString::FromCDouble( aVal, aSigFig == -1 ? m_sigfig : aSigFig );

    // Remove all but the last trailing zeros from str
    while( str.EndsWith( wxT( "00" ) ) )
        str.RemoveLast();

    // We don't want to output -0.0 as this value is just 0 for fabs
    if( str == wxT( "-0.0" ) )
        return wxT( "0.0" );

    return str;
}


void PCB_IO_IPC2581::addXY( wxXmlNode* aNode, const VECTOR2I& aVec, const char* aXName,
                            const char* aYName )
{
    if( aXName )
        addAttribute( aNode,  aXName, floatVal( m_scale * aVec.x ) );
    else
        addAttribute( aNode,  "x", floatVal( m_scale * aVec.x ) );

    if( aYName )
        addAttribute( aNode,  aYName, floatVal( -m_scale * aVec.y ) );
    else
        addAttribute( aNode,  "y", floatVal( -m_scale * aVec.y ) );
}


void PCB_IO_IPC2581::addAttribute( wxXmlNode* aNode, const wxString& aName, const wxString& aValue )
{
    m_total_bytes += aName.size() + aValue.size() + 4;
    aNode->AddAttribute( aName, aValue );
}


wxXmlNode* PCB_IO_IPC2581::generateXmlHeader()
{
    wxXmlNode* xmlHeaderNode = new wxXmlNode(wxXML_ELEMENT_NODE, "IPC-2581");
    addAttribute( xmlHeaderNode, "revision", m_version);
    addAttribute( xmlHeaderNode, "xmlns", "http://webstds.ipc.org/2581");
    addAttribute( xmlHeaderNode, "xmlns:xsi", "http://www.w3.org/2001/XMLSchema-instance");
    addAttribute( xmlHeaderNode, "xmlns:xsd", "http://www.w3.org/2001/XMLSchema");

    if( m_version == 'B' )
    {
        addAttribute( xmlHeaderNode, "xsi:schemaLocation",
                      "http://webstds.ipc.org/2581 http://webstds.ipc.org/2581/IPC-2581B1.xsd" );
    }
    else
    {
        addAttribute( xmlHeaderNode, "xsi:schemaLocation",
                      "http://webstds.ipc.org/2581 http://webstds.ipc.org/2581/IPC-2581C.xsd" );
    }

    m_xml_doc->SetRoot( xmlHeaderNode );

    return xmlHeaderNode;
}


wxXmlNode* PCB_IO_IPC2581::generateContentSection()
{
    if( m_progressReporter )
        m_progressReporter->AdvancePhase( _( "Generating content section" ) );

    wxXmlNode* contentNode = appendNode( m_xml_root, "Content" );
    addAttribute( contentNode,  "roleRef", "Owner" );

    wxXmlNode* node = appendNode( contentNode, "FunctionMode" );
    addAttribute( node,  "mode", "ASSEMBLY" );

    // This element is deprecated in revision 'C' and later
    if( m_version == 'B' )
        addAttribute( node,  "level", "3" );

    node = appendNode( contentNode, "StepRef" );
    wxFileName fn( m_board->GetFileName() );
    addAttribute( node,  "name", genString( fn.GetName(), "BOARD" ) );

    wxXmlNode* color_node = generateContentStackup( contentNode );

    if( m_version == 'C' )
    {
        contentNode->AddChild( color_node );
        m_line_node = appendNode( contentNode, "DictionaryLineDesc" );
        addAttribute( m_line_node, "units", m_units_str );

        wxXmlNode* fillNode = appendNode( contentNode, "DictionaryFillDesc" );
        addAttribute( fillNode, "units", m_units_str );

        m_shape_std_node = appendNode( contentNode, "DictionaryStandard" );
        addAttribute( m_shape_std_node, "units", m_units_str );

        m_shape_user_node = appendNode( contentNode, "DictionaryUser" );
        addAttribute( m_shape_user_node, "units", m_units_str );
    }
    else
    {
        m_shape_std_node = appendNode( contentNode, "DictionaryStandard" );
        addAttribute( m_shape_std_node, "units", m_units_str );

        m_shape_user_node = appendNode( contentNode, "DictionaryUser" );
        addAttribute( m_shape_user_node, "units", m_units_str );

        m_line_node = appendNode( contentNode, "DictionaryLineDesc" );
        addAttribute( m_line_node, "units", m_units_str );

        contentNode->AddChild( color_node );
    }

    return contentNode;
}


void PCB_IO_IPC2581::addLocationNode( wxXmlNode* aNode, double aX, double aY )
{
    wxXmlNode* location_node = appendNode( aNode, "Location" );
    addXY( location_node, VECTOR2I( aX, aY ) );
}


void PCB_IO_IPC2581::addLocationNode( wxXmlNode* aNode, const PAD& aPad, bool aRelative )
{
    VECTOR2D pos{};

    if( aRelative )
        pos = aPad.GetFPRelativePosition();
    else
        pos = aPad.GetPosition();

    if( aPad.GetOffset( PADSTACK::ALL_LAYERS ).x != 0 || aPad.GetOffset( PADSTACK::ALL_LAYERS ).y != 0 )
        pos += aPad.GetOffset( PADSTACK::ALL_LAYERS );

    addLocationNode( aNode, pos.x, pos.y );
}


void PCB_IO_IPC2581::addLocationNode( wxXmlNode* aNode, const PCB_SHAPE& aShape )
{
    VECTOR2D pos{};

    switch( aShape.GetShape() )
    {
    // Rectangles in KiCad are mapped by their corner while IPC2581 uses the center
    case SHAPE_T::RECTANGLE:
        pos = aShape.GetPosition()
              + VECTOR2I( aShape.GetRectangleWidth() / 2.0, aShape.GetRectangleHeight() / 2.0 );
        break;
    // Both KiCad and IPC2581 use the center of the circle
    case SHAPE_T::CIRCLE:
        pos = aShape.GetPosition();
        break;

    // KiCad uses the exact points on the board, so we want the reference location to be 0,0
    case SHAPE_T::POLY:
    case SHAPE_T::BEZIER:
    case SHAPE_T::SEGMENT:
    case SHAPE_T::ARC:
        pos = VECTOR2D( 0, 0 );
        break;

    case SHAPE_T::UNDEFINED:
        wxFAIL;
    }

    addLocationNode( aNode, pos.x, pos.y );
}


size_t PCB_IO_IPC2581::lineHash( int aWidth, LINE_STYLE aDashType )
{
    size_t hash = hash_val( aWidth );
    hash_combine( hash, aDashType );

    return hash;
}


size_t PCB_IO_IPC2581::shapeHash( const PCB_SHAPE& aShape )
{
    return hash_fp_item( &aShape, HASH_POS | REL_COORD );
}


wxXmlNode* PCB_IO_IPC2581::generateContentStackup( wxXmlNode* aContentNode )
{

    BOARD_DESIGN_SETTINGS& bds = m_board->GetDesignSettings();
    BOARD_STACKUP& stackup = bds.GetStackupDescriptor();
    stackup.SynchronizeWithBoard( &bds );

    wxXmlNode* color_node = new wxXmlNode( wxXML_ELEMENT_NODE, "DictionaryColor" );

    for( BOARD_STACKUP_ITEM* item: stackup.GetList() )
    {
        wxString layer_name = item->GetLayerName();
        int sub_layer_count = 1;

        if( layer_name.empty() )
            layer_name = m_board->GetLayerName( item->GetBrdLayerId() );

        layer_name = genString( layer_name, "LAYER" );

        if( item->GetType() == BS_ITEM_TYPE_DIELECTRIC )
        {
            layer_name = genString( wxString::Format( "DIELECTRIC_%d", item->GetDielectricLayerId() ),
                                    "LAYER" );
            sub_layer_count = item->GetSublayersCount();
        }
        else
        {
            m_layer_name_map.emplace( item->GetBrdLayerId(), layer_name );
        }

        for( int sub_idx = 0; sub_idx < sub_layer_count; sub_idx++ )
        {
            wxString sub_layer_name = layer_name;

            if( sub_idx > 0 )
                sub_layer_name += wxString::Format( "_%d", sub_idx );

            wxXmlNode* node = appendNode( aContentNode, "LayerRef" );
            addAttribute( node,  "name", sub_layer_name );

            if( !IsPrmSpecified( item->GetColor( sub_idx ) ) )
                continue;

            wxXmlNode* entry_color = appendNode( color_node, "EntryColor" );
            addAttribute( entry_color,  "id", genString( sub_layer_name, "COLOR" ) );
            wxXmlNode* color = appendNode( entry_color, "Color" );

            wxString colorName = item->GetColor( sub_idx );

            if( colorName.StartsWith( wxT( "#" ) ) )    // This is a user defined color,
                                                        // not in standard color list.
            {
                COLOR4D layer_color( colorName );
                addAttribute( color,  "r", wxString::Format( "%d",
                                                             KiROUND( layer_color.r * 255 ) ) );
                addAttribute( color,  "g", wxString::Format( "%d",
                                                             KiROUND( layer_color.g * 255 ) ) );
                addAttribute( color,  "b", wxString::Format( "%d",
                                                             KiROUND( layer_color.b * 255 ) ) );
            }
            else
            {
                for( const FAB_LAYER_COLOR& fab_color : GetStandardColors( item->GetType() ) )
                {
                    if( fab_color.GetName() == colorName )
                    {
                        addAttribute( color,  "r", wxString::Format( "%d", KiROUND( fab_color.GetColor( item->GetType() ).r * 255 ) ) );
                        addAttribute( color,  "g", wxString::Format( "%d", KiROUND( fab_color.GetColor( item->GetType() ).g * 255 ) ) );
                        addAttribute( color,  "b", wxString::Format( "%d", KiROUND( fab_color.GetColor( item->GetType() ).b * 255 ) ) );
                        break;
                    }
                }
            }
        }
    }

    return color_node;
}


void PCB_IO_IPC2581::addFillDesc( wxXmlNode* aNode, FILL_T aFill, bool aForce )
{
    if( aFill == FILL_T::FILLED_SHAPE )
    {
        // By default, we do not fill shapes because FILL is the default value for most.
        // But for some outlines, we may need to force a fill.
        if( aForce )
        {
            wxXmlNode* fillDesc_node = appendNode( aNode, "FillDesc" );
            addAttribute( fillDesc_node,  "fillProperty", "FILL" );
        }
    }
    else
    {
        wxXmlNode* fillDesc_node = appendNode( aNode, "FillDesc" );
        addAttribute( fillDesc_node,  "fillProperty", "HOLLOW" );
    }
}


void PCB_IO_IPC2581::addLineDesc( wxXmlNode* aNode, int aWidth, LINE_STYLE aDashType, bool aForce )
{
    wxCHECK_RET( aNode, "aNode is null" );

    if( aWidth < 0 )
        return;

    wxXmlNode* entry_node = nullptr;

    if( !aForce )
    {
        size_t hash = lineHash( aWidth, aDashType );
        wxString name = wxString::Format( "LINE_%zu", m_line_dict.size() + 1 );
        auto[ iter, inserted ] = m_line_dict.emplace( hash, name );

        // Either add a new entry or reference an existing one
        wxXmlNode* lineDesc_node = appendNode( aNode, "LineDescRef" );
        addAttribute( lineDesc_node,  "id", iter->second );

        if( !inserted )
            return;

        entry_node = appendNode( m_line_node, "EntryLineDesc" );
        addAttribute( entry_node,  "id", name );
    }
    else
    {
        // Force the LineDesc to be added directly to the parent node
        entry_node = aNode;
    }

    wxXmlNode* line_node = appendNode( entry_node, "LineDesc" );
    addAttribute( line_node,  "lineWidth", floatVal( m_scale * aWidth ) );
    addAttribute( line_node,  "lineEnd", "ROUND" );

    switch( aDashType )
    {
    case LINE_STYLE::DOT:
        addAttribute( line_node,  "lineProperty", "DOTTED" );
        break;
    case LINE_STYLE::DASH:
        addAttribute( line_node,  "lineProperty", "DASHED" );
        break;
    case LINE_STYLE::DASHDOT:
        addAttribute( line_node,  "lineProperty", "CENTER" );
        break;
    case LINE_STYLE::DASHDOTDOT:
        addAttribute( line_node,  "lineProperty", "PHANTOM" );
        break;
    default:
        break;
    }
}


void PCB_IO_IPC2581::addKnockoutText( wxXmlNode* aContentNode, PCB_TEXT* aText )
{
    SHAPE_POLY_SET finalPoly;

    aText->TransformTextToPolySet( finalPoly, 0, ARC_HIGH_DEF, ERROR_INSIDE );
    finalPoly.Fracture();

    for( int ii = 0; ii < finalPoly.OutlineCount(); ++ii )
        addContourNode( aContentNode, finalPoly, ii );
}


void PCB_IO_IPC2581::addText( wxXmlNode* aContentNode, EDA_TEXT* aText,
                              const KIFONT::METRICS& aFontMetrics )
{
    KIGFX::GAL_DISPLAY_OPTIONS empty_opts;
    KIFONT::FONT*              font = aText->GetDrawFont( nullptr );
    TEXT_ATTRIBUTES            attrs = aText->GetAttributes();

    attrs.m_StrokeWidth = aText->GetEffectiveTextPenWidth();
    attrs.m_Angle = aText->GetDrawRotation();
    attrs.m_Multiline = false;

    wxXmlNode* text_node = appendNode( aContentNode, "UserSpecial" );

    std::list<VECTOR2I> pts;

    auto push_pts =
            [&]()
            {
                if( pts.size() < 2 )
                    return;

                wxXmlNode* line_node = nullptr;

                // Polylines are only allowed for more than 3 points (in version B).
                // Otherwise, we have to use a line
                if( pts.size() < 3 )
                {
                    line_node = appendNode( text_node, "Line" );
                    addXY( line_node, pts.front(), "startX", "startY" );
                    addXY( line_node, pts.back(), "endX", "endY" );
                }
                else
                {
                    line_node = appendNode( text_node, "Polyline" );
                    wxXmlNode* point_node = appendNode( line_node, "PolyBegin" );
                    addXY( point_node, pts.front() );

                    auto iter = pts.begin();

                    for( ++iter; iter != pts.end(); ++iter )
                    {
                        wxXmlNode* point_node = appendNode( line_node, "PolyStepSegment" );
                        addXY( point_node, *iter );
                    }

                }

                addLineDesc( line_node, attrs.m_StrokeWidth, LINE_STYLE::SOLID );
                pts.clear();
            };

    CALLBACK_GAL callback_gal( empty_opts,
            // Stroke callback
            [&]( const VECTOR2I& aPt1, const VECTOR2I& aPt2 )
            {
                if( !pts.empty() )
                {
                    if( aPt1 == pts.back() )
                        pts.push_back( aPt2 );
                    else if( aPt2 == pts.front() )
                        pts.push_front( aPt1 );
                    else if( aPt1 == pts.front() )
                        pts.push_front( aPt2 );
                    else if( aPt2 == pts.back() )
                        pts.push_back( aPt1 );
                    else
                    {
                        push_pts();
                        pts.push_back( aPt1 );
                        pts.push_back( aPt2 );
                    }
                }
                else
                {
                    pts.push_back( aPt1 );
                    pts.push_back( aPt2 );
                }
            },
            // Polygon callback
            [&]( const SHAPE_LINE_CHAIN& aPoly )
            {
                if( aPoly.PointCount() < 3 )
                    return;

                wxXmlNode* outline_node = appendNode( text_node, "Outline" );
                wxXmlNode* poly_node = appendNode( outline_node, "Polygon" );
                addLineDesc( outline_node, 0, LINE_STYLE::SOLID );

                const std::vector<VECTOR2I>& pts = aPoly.CPoints();
                wxXmlNode* point_node = appendNode( poly_node, "PolyBegin" );
                addXY( point_node, pts.front() );

                for( size_t ii = 1; ii < pts.size(); ++ii )
                {
                    wxXmlNode* point_node =
                            appendNode( poly_node, "PolyStepSegment" );
                    addXY( point_node, pts[ii] );
                }

                point_node = appendNode( poly_node, "PolyStepSegment" );
                addXY( point_node, pts.front() );
            } );

    //TODO: handle multiline text

    font->Draw( &callback_gal, aText->GetShownText( true ), aText->GetTextPos(), attrs,
                aFontMetrics );

    if( !pts.empty() )
        push_pts();

    if( text_node->GetChildren() == nullptr )
    {
        aContentNode->RemoveChild( text_node );
        delete text_node;
    }
}


void PCB_IO_IPC2581::addShape( wxXmlNode* aContentNode, const PAD& aPad, PCB_LAYER_ID aLayer )
{
    size_t hash = hash_fp_item( &aPad, 0 );
    auto   iter = m_std_shape_dict.find( hash );

    if( iter != m_std_shape_dict.end() )
    {
        wxXmlNode* shape_node = appendNode( aContentNode, "StandardPrimitiveRef" );
        addAttribute( shape_node,  "id", iter->second );
        return;
    }

    int      maxError = m_board->GetDesignSettings().m_MaxError;
    wxString name;
    VECTOR2I expansion{ 0, 0 };

    if( LSET( { F_Mask, B_Mask } ).Contains( aLayer ) )
        expansion.x = expansion.y = 2 * aPad.GetSolderMaskExpansion( PADSTACK::ALL_LAYERS );

    if( LSET( { F_Paste, B_Paste } ).Contains( aLayer ) )
        expansion = 2 * aPad.GetSolderPasteMargin( PADSTACK::ALL_LAYERS );

    switch( aPad.GetShape( PADSTACK::ALL_LAYERS ) )
    {
    case PAD_SHAPE::CIRCLE:
    {
        name = wxString::Format( "CIRCLE_%zu", m_std_shape_dict.size() + 1 );
        m_std_shape_dict.emplace( hash, name );

        wxXmlNode* entry_node = appendNode( m_shape_std_node, "EntryStandard" );
        addAttribute( entry_node,  "id", name );

        wxXmlNode* circle_node = appendNode( entry_node, "Circle" );
        circle_node->AddAttribute( "diameter",
                                   floatVal( m_scale * ( expansion.x + aPad.GetSizeX() ) ) );
        break;
    }

    case PAD_SHAPE::RECTANGLE:
    {
        name = wxString::Format( "RECT_%zu", m_std_shape_dict.size() + 1 );
        m_std_shape_dict.emplace( hash, name );

        wxXmlNode* entry_node = appendNode( m_shape_std_node, "EntryStandard" );
        addAttribute( entry_node,  "id", name );

        wxXmlNode* rect_node = appendNode( entry_node, "RectCenter" );
        VECTOR2D pad_size = aPad.GetSize( PADSTACK::ALL_LAYERS ) + expansion;
        addAttribute( rect_node,  "width", floatVal( m_scale * std::abs( pad_size.x ) ) );
        addAttribute( rect_node,  "height", floatVal( m_scale * std::abs( pad_size.y ) ) );

        break;

    }
    case PAD_SHAPE::OVAL:
    {
        name = wxString::Format( "OVAL_%zu", m_std_shape_dict.size() + 1 );
        m_std_shape_dict.emplace( hash, name );

        wxXmlNode* entry_node = appendNode( m_shape_std_node, "EntryStandard" );
        addAttribute( entry_node,  "id", name );

        wxXmlNode* oval_node = appendNode( entry_node, "Oval" );
        VECTOR2D pad_size = aPad.GetSize( PADSTACK::ALL_LAYERS ) + expansion;
        addAttribute( oval_node,  "width", floatVal( m_scale * pad_size.x ) );
        addAttribute( oval_node,  "height", floatVal( m_scale * pad_size.y ) );

        break;
    }

    case PAD_SHAPE::ROUNDRECT:
    {
        name = wxString::Format( "ROUNDRECT_%zu", m_std_shape_dict.size() + 1 );
        m_std_shape_dict.emplace( hash, name );

        wxXmlNode* entry_node = appendNode( m_shape_std_node, "EntryStandard" );
        addAttribute( entry_node,  "id", name );

        wxXmlNode* roundrect_node = appendNode( entry_node, "RectRound" );
        VECTOR2D pad_size = aPad.GetSize( PADSTACK::ALL_LAYERS ) + expansion;
        addAttribute( roundrect_node,  "width", floatVal( m_scale * pad_size.x ) );
        addAttribute( roundrect_node,  "height", floatVal( m_scale * pad_size.y ) );
        roundrect_node->AddAttribute( "radius",
                                      floatVal( m_scale * aPad.GetRoundRectCornerRadius( PADSTACK::ALL_LAYERS ) ) );
        addAttribute( roundrect_node,  "upperRight", "true" );
        addAttribute( roundrect_node,  "upperLeft", "true" );
        addAttribute( roundrect_node,  "lowerRight", "true" );
        addAttribute( roundrect_node,  "lowerLeft", "true" );

        break;
    }

    case PAD_SHAPE::CHAMFERED_RECT:
    {
        name = wxString::Format( "RECTCHAMFERED_%zu", m_std_shape_dict.size() + 1 );
        m_std_shape_dict.emplace( hash, name );

        wxXmlNode* entry_node = appendNode( m_shape_std_node, "EntryStandard" );
        addAttribute( entry_node,  "id", name );

        wxXmlNode* chamfered_node = appendNode( entry_node, "RectCham" );
        VECTOR2D pad_size = aPad.GetSize( PADSTACK::ALL_LAYERS ) + expansion;
        addAttribute( chamfered_node,  "width", floatVal( m_scale * pad_size.x ) );
        addAttribute( chamfered_node,  "height", floatVal( m_scale * pad_size.y ) );

        int shorterSide = std::min( pad_size.x, pad_size.y );
        int chamfer = std::max( 0, KiROUND( aPad.GetChamferRectRatio( PADSTACK::ALL_LAYERS ) * shorterSide ) );

        addAttribute( chamfered_node,  "chamfer", floatVal( m_scale * chamfer ) );

        int positions = aPad.GetChamferPositions( PADSTACK::ALL_LAYERS );

        if( positions & RECT_CHAMFER_TOP_LEFT )
            addAttribute( chamfered_node,  "upperLeft", "true" );
        if( positions & RECT_CHAMFER_TOP_RIGHT )
            addAttribute( chamfered_node,  "upperRight", "true" );
        if( positions & RECT_CHAMFER_BOTTOM_LEFT )
            addAttribute( chamfered_node,  "lowerLeft", "true" );
        if( positions & RECT_CHAMFER_BOTTOM_RIGHT )
            addAttribute( chamfered_node,  "lowerRight", "true" );

        break;
    }

    case PAD_SHAPE::TRAPEZOID:
    {
        name = wxString::Format( "TRAPEZOID_%zu", m_std_shape_dict.size() + 1 );
        m_std_shape_dict.emplace( hash, name );

        wxXmlNode* entry_node = appendNode( m_shape_std_node, "EntryStandard" );
        addAttribute( entry_node,  "id", name );

        VECTOR2I       pad_size = aPad.GetSize( PADSTACK::ALL_LAYERS );
        VECTOR2I       trap_delta = aPad.GetDelta( PADSTACK::ALL_LAYERS );
        SHAPE_POLY_SET outline;
        outline.NewOutline();
        int dx = pad_size.x / 2;
        int dy = pad_size.y / 2;
        int ddx = trap_delta.x / 2;
        int ddy = trap_delta.y / 2;

        outline.Append( -dx - ddy,  dy + ddx );
        outline.Append(  dx + ddy,  dy - ddx );
        outline.Append(  dx - ddy, -dy + ddx );
        outline.Append( -dx + ddy, -dy - ddx );

        // Shape polygon can have holes so use InflateWithLinkedHoles(), not Inflate()
        // which can create bad shapes if margin.x is < 0
        if( expansion.x )
        {
            outline.InflateWithLinkedHoles( expansion.x, CORNER_STRATEGY::ROUND_ALL_CORNERS,
                                            maxError );
        }

        addContourNode( entry_node, outline );

        break;
    }
    case PAD_SHAPE::CUSTOM:
    {
        name = wxString::Format( "CUSTOM_%zu", m_std_shape_dict.size() + 1 );
        m_std_shape_dict.emplace( hash, name );

        wxXmlNode* entry_node = appendNode( m_shape_std_node, "EntryStandard" );
        addAttribute( entry_node,  "id", name );

        SHAPE_POLY_SET shape;
        aPad.MergePrimitivesAsPolygon( PADSTACK::ALL_LAYERS, &shape );

        if( expansion != VECTOR2I( 0, 0 ) )
        {
            shape.InflateWithLinkedHoles( std::max( expansion.x, expansion.y ),
                                          CORNER_STRATEGY::ROUND_ALL_CORNERS, maxError );
        }

        addContourNode( entry_node, shape );
        break;
    }
    default:
        Report( _( "Pad has unsupported type; it was skipped." ), RPT_SEVERITY_WARNING );
        break;
    }

    if( !name.empty() )
    {
        m_std_shape_dict.emplace( hash, name );
        wxXmlNode* shape_node = appendNode( aContentNode, "StandardPrimitiveRef" );
        addAttribute( shape_node,  "id", name );
    }
}


void PCB_IO_IPC2581::addShape( wxXmlNode* aContentNode, const PCB_SHAPE& aShape, bool aInline )
{
    size_t hash = shapeHash( aShape );
    auto iter = m_user_shape_dict.find( hash );
    wxString name;

    // When not inline, check for existing shape in dictionary and reference it
    if( !aInline && iter != m_user_shape_dict.end() )
    {
        wxXmlNode* shape_node = appendNode( aContentNode, "UserPrimitiveRef" );
        addAttribute( shape_node,  "id", iter->second );
        return;
    }

    switch( aShape.GetShape() )
    {
    case SHAPE_T::CIRCLE:
    {
        if( aInline )
        {
            // For inline shapes (e.g., in Marking elements), output geometry directly as a
            // Polyline with two arcs forming a circle
            int radius = aShape.GetRadius();
            int width = aShape.GetStroke().GetWidth();
            LINE_STYLE dash = aShape.GetStroke().GetLineStyle();

            wxXmlNode* polyline_node = appendNode( aContentNode, "Polyline" );

            // Create a circle using two semicircular arcs
            // Start at the rightmost point of the circle
            VECTOR2I center = aShape.GetCenter();
            VECTOR2I start( center.x + radius, center.y );
            VECTOR2I mid( center.x - radius, center.y );

            wxXmlNode* begin_node = appendNode( polyline_node, "PolyBegin" );
            addXY( begin_node, start );

            // First arc from start to mid (top semicircle)
            wxXmlNode* arc1_node = appendNode( polyline_node, "PolyStepCurve" );
            addXY( arc1_node, mid );
            addXY( arc1_node, center, "centerX", "centerY" );
            addAttribute( arc1_node, "clockwise", "true" );

            // Second arc from mid back to start (bottom semicircle)
            wxXmlNode* arc2_node = appendNode( polyline_node, "PolyStepCurve" );
            addXY( arc2_node, start );
            addXY( arc2_node, center, "centerX", "centerY" );
            addAttribute( arc2_node, "clockwise", "true" );

            if( width > 0 )
                addLineDesc( polyline_node, width, dash, true );

            break;
        }

        name = wxString::Format( "UCIRCLE_%zu", m_user_shape_dict.size() + 1 );
        m_user_shape_dict.emplace( hash, name );
        int diameter = aShape.GetRadius() * 2.0;
        int width = aShape.GetStroke().GetWidth();
        LINE_STYLE dash = aShape.GetStroke().GetLineStyle();


        wxXmlNode* entry_node = appendNode( m_shape_user_node, "EntryUser" );
        addAttribute( entry_node,  "id", name );
        wxXmlNode* special_node = appendNode( entry_node, "UserSpecial" );

        wxXmlNode* circle_node = appendNode( special_node, "Circle" );

        if( aShape.GetFillMode() == FILL_T::NO_FILL )
        {
            addAttribute( circle_node,  "diameter", floatVal( m_scale * diameter ) );
            addLineDesc( circle_node, width, dash, true );
        }
        else
        {
            // IPC2581 does not allow strokes on filled elements
            addAttribute( circle_node,  "diameter", floatVal( m_scale * ( diameter + width ) ) );
        }

        addFillDesc( circle_node, aShape.GetFillMode() );

        break;
    }

    case SHAPE_T::RECTANGLE:
    {
        if( aInline )
        {
            // For inline shapes, output as a Polyline with the rectangle corners
            int stroke_width = aShape.GetStroke().GetWidth();
            LINE_STYLE dash = aShape.GetStroke().GetLineStyle();

            wxXmlNode* polyline_node = appendNode( aContentNode, "Polyline" );

            // Get the rectangle corners. Use GetRectCorners for proper handling
            std::vector<VECTOR2I> corners = aShape.GetRectCorners();

            wxXmlNode* begin_node = appendNode( polyline_node, "PolyBegin" );
            addXY( begin_node, corners[0] );

            for( size_t i = 1; i < corners.size(); ++i )
            {
                wxXmlNode* step_node = appendNode( polyline_node, "PolyStepSegment" );
                addXY( step_node, corners[i] );
            }

            // Close the rectangle
            wxXmlNode* close_node = appendNode( polyline_node, "PolyStepSegment" );
            addXY( close_node, corners[0] );

            if( stroke_width > 0 )
                addLineDesc( polyline_node, stroke_width, dash, true );

            break;
        }

        name = wxString::Format( "URECT_%zu", m_user_shape_dict.size() + 1 );
        m_user_shape_dict.emplace( hash, name );

        wxXmlNode* entry_node = appendNode( m_shape_user_node, "EntryUser" );
        addAttribute( entry_node,  "id", name );
        wxXmlNode* special_node = appendNode( entry_node, "UserSpecial" );

        int width = std::abs( aShape.GetRectangleWidth() );
        int height = std::abs( aShape.GetRectangleHeight() );
        int stroke_width = aShape.GetStroke().GetWidth();
        LINE_STYLE dash = aShape.GetStroke().GetLineStyle();

        wxXmlNode* rect_node = appendNode( special_node, "RectRound" );
        addLineDesc( rect_node, aShape.GetStroke().GetWidth(), aShape.GetStroke().GetLineStyle(),
                     true );

        if( aShape.GetFillMode() == FILL_T::NO_FILL )
        {
            addAttribute( rect_node,  "upperRight", "false" );
            addAttribute( rect_node,  "upperLeft", "false" );
            addAttribute( rect_node,  "lowerRight", "false" );
            addAttribute( rect_node,  "lowerLeft", "false" );
        }
        else
        {
            addAttribute( rect_node,  "upperRight", "true" );
            addAttribute( rect_node,  "upperLeft", "true" );
            addAttribute( rect_node,  "lowerRight", "true" );
            addAttribute( rect_node,  "lowerLeft", "true" );
            width += stroke_width;
            height += stroke_width;
        }

        addFillDesc( rect_node, aShape.GetFillMode() );

        addAttribute( rect_node,  "width", floatVal( m_scale * width ) );
        addAttribute( rect_node,  "height", floatVal( m_scale * height ) );
        addAttribute( rect_node,  "radius", floatVal( m_scale * ( stroke_width / 2.0 ) ) );

        break;
    }

    case SHAPE_T::POLY:
    {
        if( aInline )
        {
            // For inline shapes, output as Polyline elements directly
            const SHAPE_POLY_SET& poly_set = aShape.GetPolyShape();
            int stroke_width = aShape.GetStroke().GetWidth();
            LINE_STYLE dash = aShape.GetStroke().GetLineStyle();

            for( int ii = 0; ii < poly_set.OutlineCount(); ++ii )
            {
                const SHAPE_LINE_CHAIN& outline = poly_set.Outline( ii );

                if( outline.PointCount() < 2 )
                    continue;

                wxXmlNode* polyline_node = appendNode( aContentNode, "Polyline" );
                const std::vector<VECTOR2I>& pts = outline.CPoints();

                wxXmlNode* begin_node = appendNode( polyline_node, "PolyBegin" );
                addXY( begin_node, pts[0] );

                for( size_t jj = 1; jj < pts.size(); ++jj )
                {
                    wxXmlNode* step_node = appendNode( polyline_node, "PolyStepSegment" );
                    addXY( step_node, pts[jj] );
                }

                // Close the polygon if needed
                if( pts.size() > 2 && pts.front() != pts.back() )
                {
                    wxXmlNode* close_node = appendNode( polyline_node, "PolyStepSegment" );
                    addXY( close_node, pts[0] );
                }

                if( stroke_width > 0 )
                    addLineDesc( polyline_node, stroke_width, dash, true );
            }

            break;
        }

        name = wxString::Format( "UPOLY_%zu", m_user_shape_dict.size() + 1 );
        m_user_shape_dict.emplace( hash, name );

        wxXmlNode* entry_node = appendNode( m_shape_user_node, "EntryUser" );
        addAttribute( entry_node,  "id", name );

        // If we are stroking a polygon, we need two contours.  This is only allowed
        // inside a "UserSpecial" shape
        wxXmlNode* special_node = appendNode( entry_node, "UserSpecial" );

        const SHAPE_POLY_SET& poly_set = aShape.GetPolyShape();

        for( int ii = 0; ii < poly_set.OutlineCount(); ++ii )
        {
            if( aShape.GetFillMode() != FILL_T::NO_FILL )
            {
                // IPC2581 does not allow strokes on filled elements
                addContourNode( special_node, poly_set, ii, FILL_T::FILLED_SHAPE, 0,
                                LINE_STYLE::SOLID );
            }

            addContourNode( special_node, poly_set, ii, FILL_T::NO_FILL,
                            aShape.GetStroke().GetWidth(), aShape.GetStroke().GetLineStyle() );
        }

        break;
    }

    case SHAPE_T::ARC:
    {
        wxXmlNode* arc_node = appendNode( aContentNode, "Arc" );
        addXY( arc_node, aShape.GetStart(), "startX", "startY" );
        addXY( arc_node, aShape.GetEnd(), "endX", "endY" );
        addXY( arc_node, aShape.GetCenter(), "centerX", "centerY" );

        //N.B. because our coordinate system is flipped, we need to flip the arc direction
        addAttribute( arc_node,  "clockwise", !aShape.IsClockwiseArc() ? "true" : "false" );

        if( aShape.GetStroke().GetWidth() > 0 )
        {
            addLineDesc( arc_node, aShape.GetStroke().GetWidth(),
                         aShape.GetStroke().GetLineStyle(), true );
        }

        break;
    }

    case SHAPE_T::BEZIER:
    {
        wxXmlNode* polyline_node = appendNode( aContentNode, "Polyline" );
        std::vector<VECTOR2I> ctrlPoints = { aShape.GetStart(), aShape.GetBezierC1(),
                                             aShape.GetBezierC2(), aShape.GetEnd() };
        BEZIER_POLY converter( ctrlPoints );
        std::vector<VECTOR2I> points;
        converter.GetPoly( points, ARC_HIGH_DEF );

        wxXmlNode* point_node = appendNode( polyline_node, "PolyBegin" );
        addXY( point_node, points[0] );

        for( size_t i = 1; i < points.size(); i++ )
        {
            wxXmlNode* point_node = appendNode( polyline_node, "PolyStepSegment" );
            addXY( point_node, points[i] );
        }

        if( aShape.GetStroke().GetWidth() > 0 )
        {
            addLineDesc( polyline_node, aShape.GetStroke().GetWidth(),
                         aShape.GetStroke().GetLineStyle(), true );
        }

        break;
    }

    case SHAPE_T::SEGMENT:
    {
        wxXmlNode* line_node = appendNode( aContentNode, "Line" );
        addXY( line_node, aShape.GetStart(), "startX", "startY" );
        addXY( line_node, aShape.GetEnd(), "endX", "endY" );

        if( aShape.GetStroke().GetWidth() > 0 )
        {
            addLineDesc( line_node, aShape.GetStroke().GetWidth(),
                         aShape.GetStroke().GetLineStyle(), true );
        }

        break;
    }

    case SHAPE_T::UNDEFINED:
        wxFAIL;
    }

    // Only add UserPrimitiveRef when not in inline mode and a dictionary entry was created
    if( !aInline && !name.empty() )
    {
        wxXmlNode* shape_node = appendNode( aContentNode, "UserPrimitiveRef" );
        addAttribute( shape_node,  "id", name );
    }

}


void PCB_IO_IPC2581::addSlotCavity( wxXmlNode* aNode, const PAD& aPad, const wxString& aName )
{
    wxXmlNode* slotNode = appendNode( aNode, "SlotCavity" );
    addAttribute( slotNode, "name", aName );
    addAttribute( slotNode, "platingStatus", aPad.GetAttribute() == PAD_ATTRIB::PTH ? "PLATED"
                                                                                    : "NONPLATED" );
    addAttribute( slotNode, "plusTol", "0.0" );
    addAttribute( slotNode, "minusTol", "0.0" );

    if( m_version > 'B' )
        addLocationNode( slotNode, aPad, false );

    // Normally only oblong drill shapes should reach this code path since m_slot_holes
    // is filtered to pads where DrillSizeX != DrillSizeY. However, use a fallback to
    // ensure valid XML is always generated.
    if( aPad.GetDrillShape() == PAD_DRILL_SHAPE::OBLONG )
    {
        VECTOR2I  drill_size = aPad.GetDrillSize();
        EDA_ANGLE rotation = aPad.GetOrientation();

        // IPC-2581C requires width >= height for Oval primitive
        // Swap dimensions if needed and adjust rotation accordingly
        if( drill_size.y > drill_size.x )
        {
            std::swap( drill_size.x, drill_size.y );
            rotation += ANGLE_90;
        }

        // Add Xform if rotation is needed (must come before Feature per IPC-2581C schema)
        if( rotation != ANGLE_0 )
        {
            wxXmlNode* xformNode = appendNode( slotNode, "Xform" );
            addAttribute( xformNode, "rotation", floatVal( rotation.AsDegrees() ) );
        }

        // Use IPC-2581 Oval primitive for oblong slots
        wxXmlNode* ovalNode = appendNode( slotNode, "Oval" );
        addAttribute( ovalNode, "width", floatVal( m_scale * drill_size.x ) );
        addAttribute( ovalNode, "height", floatVal( m_scale * drill_size.y ) );
    }
    else
    {
        // Fallback to polygon outline for non-oblong shapes
        SHAPE_POLY_SET poly_set;
        int            maxError = m_board->GetDesignSettings().m_MaxError;
        aPad.TransformHoleToPolygon( poly_set, 0, maxError, ERROR_INSIDE );

        addOutlineNode( slotNode, poly_set );
    }
}


wxXmlNode* PCB_IO_IPC2581::generateLogisticSection()
{
    wxXmlNode* logisticNode = appendNode( m_xml_root, "LogisticHeader" );

    wxXmlNode* roleNode = appendNode( logisticNode, "Role" );
    addAttribute( roleNode,  "id", "Owner" );
    addAttribute( roleNode,  "roleFunction", "SENDER" );

    m_enterpriseNode = appendNode( logisticNode, "Enterprise" );
    addAttribute( m_enterpriseNode,  "id", "UNKNOWN" );
    addAttribute( m_enterpriseNode,  "code", "NONE" );

    wxXmlNode* personNode = appendNode( logisticNode, "Person" );
    addAttribute( personNode,  "name", "UNKNOWN" );
    addAttribute( personNode,  "enterpriseRef", "UNKNOWN" );
    addAttribute( personNode,  "roleRef", "Owner" );

    return logisticNode;
}


wxXmlNode* PCB_IO_IPC2581::generateHistorySection()
{
    if( m_progressReporter )
        m_progressReporter->AdvancePhase( _( "Generating history section" ) );

    wxXmlNode* historyNode = appendNode( m_xml_root, "HistoryRecord" );
    addAttribute( historyNode,  "number", "1" );
    addAttribute( historyNode,  "origination", wxDateTime::Now().FormatISOCombined() );
    addAttribute( historyNode,  "software", "KiCad EDA" );
    addAttribute( historyNode,  "lastChange", wxDateTime::Now().FormatISOCombined() );

    wxXmlNode* fileRevisionNode = appendNode( historyNode, "FileRevision" );
    addAttribute( fileRevisionNode,  "fileRevisionId", "1" );
    addAttribute( fileRevisionNode,  "comment", "NO COMMENT" );
    addAttribute( fileRevisionNode,  "label", "NO LABEL" );

    wxXmlNode* softwarePackageNode = appendNode( fileRevisionNode, "SoftwarePackage" );
    addAttribute( softwarePackageNode,  "name", "KiCad" );
    addAttribute( softwarePackageNode,  "revision", GetMajorMinorPatchVersion() );
    addAttribute( softwarePackageNode,  "vendor", "KiCad EDA" );

    wxXmlNode* certificationNode = appendNode( softwarePackageNode, "Certification" );
    addAttribute( certificationNode,  "certificationStatus", "SELFTEST" );

    return historyNode;
}


wxXmlNode* PCB_IO_IPC2581::generateBOMSection( wxXmlNode* aEcadNode )
{
    if( m_progressReporter )
        m_progressReporter->AdvancePhase( _( "Generating BOM section" ) );

    struct REFDES
    {
        wxString m_name;
        wxString m_pkg;
        bool     m_populate;
        wxString m_layer;
    };

    struct BOM_ENTRY
    {
        BOM_ENTRY()
        {
            m_refdes = new std::vector<REFDES>();
            m_props = new std::map<wxString, wxString>();
            m_count = 0;
            m_pads = 0;
        }

        ~BOM_ENTRY()
        {
            delete m_refdes;
            delete m_props;
        }

        wxString m_OEMDesignRef; // String combining LIB+FP+VALUE
        int      m_count;
        int      m_pads;
        wxString m_type;
        wxString m_description;

        std::vector<REFDES>* m_refdes;
        std::map<wxString, wxString>* m_props;
    };

    std::set<std::unique_ptr<struct BOM_ENTRY>,
             std::function<bool( const std::unique_ptr<struct BOM_ENTRY>&,
                                 const std::unique_ptr<struct BOM_ENTRY>& )>> bom_entries(
            []( const std::unique_ptr<struct BOM_ENTRY>& a,
                const std::unique_ptr<struct BOM_ENTRY>& b )
            {
                return a->m_OEMDesignRef < b->m_OEMDesignRef;
            } );

    for( FOOTPRINT* fp_it : m_board->Footprints() )
    {
        std::unique_ptr<FOOTPRINT> fp( static_cast<FOOTPRINT*>( fp_it->Clone() ) );
        fp->SetParentGroup( nullptr );
        fp->SetPosition( {0, 0} );
        fp->SetOrientation( ANGLE_0 );

        // Normalize to unflipped state to match hash computed in addPackage
        if( fp->IsFlipped() )
            fp->Flip( fp->GetPosition(), FLIP_DIRECTION::TOP_BOTTOM );

        size_t hash = hash_fp_item( fp.get(), HASH_POS | REL_COORD );
        auto iter = m_footprint_dict.find( hash );

        if( iter == m_footprint_dict.end() )
        {
            Report( wxString::Format( _( "Footprint %s not found in dictionary; BOM data may be incomplete." ),
                                      fp->GetFPID().GetLibItemName().wx_str() ),
                    RPT_SEVERITY_WARNING );
            continue;
        }

        auto entry = std::make_unique<struct BOM_ENTRY>();

        /// We assume that the m_OEMRef_dict is populated already by the generateComponents function
        /// This will either place a unique string in the dictionary or field reference.
        if( auto it = m_OEMRef_dict.find( fp_it ); it != m_OEMRef_dict.end() )
        {
            entry->m_OEMDesignRef = it->second;
        }
        else
        {
            Report( wxString::Format( _( "Component \"%s\" missing OEM reference; BOM entry will be skipped." ),
                                      fp->GetFPID().GetLibItemName().wx_str() ),
                    RPT_SEVERITY_WARNING );
        }

        entry->m_OEMDesignRef = genString( entry->m_OEMDesignRef, "REF" );
        entry->m_count = 1;
        entry->m_pads = fp->GetPadCount();

        // TODO: The options are "ELECTRICAL", "MECHANICAL", "PROGRAMMABLE", "DOCUMENT", "MATERIAL"
        //      We need to figure out how to determine this.
        const wxString variantName = m_board ? m_board->GetCurrentVariant() : wxString();

        if( entry->m_pads == 0 || fp_it->GetExcludedFromBOMForVariant( variantName ) )
            entry->m_type = "DOCUMENT";
        else
            entry->m_type = "ELECTRICAL";

        // Use the footprint's Description field if it exists
        const PCB_FIELD* descField = fp_it->GetField( FIELD_T::DESCRIPTION );

        if( descField && !descField->GetShownText( false ).IsEmpty() )
            entry->m_description = descField->GetShownText( false );

        auto[ bom_iter, inserted ] = bom_entries.insert( std::move( entry ) );

        if( !inserted )
            ( *bom_iter )->m_count++;

        REFDES refdes;
        refdes.m_name = componentName( fp_it );
        refdes.m_pkg = fp->GetFPID().GetLibItemName().wx_str();
        refdes.m_populate = !fp->GetDNPForVariant( variantName )
                && !fp->GetExcludedFromBOMForVariant( variantName );
        refdes.m_layer = m_layer_name_map[fp_it->GetLayer()];

        ( *bom_iter )->m_refdes->push_back( refdes );

        // TODO: This amalgamates all the properties from all the footprints.  We need to decide
        // if we want to group footprints by their properties
        for( PCB_FIELD* prop : fp->GetFields() )
        {
            // We don't include Reference, Datasheet, or Description in BOM characteristics.
            // Value and any user-defined fields are included.  Reference is captured above,
            // and Description is used for the BomItem description attribute.
            if( prop->IsMandatory() && !prop->IsValue() )
                continue;

            ( *bom_iter )->m_props->emplace( prop->GetName(), prop->GetShownText( false ) );
        }
    }

    if( bom_entries.empty() )
        return nullptr;

    wxFileName fn( m_board->GetFileName() );

    wxXmlNode* bomNode = new wxXmlNode( wxXML_ELEMENT_NODE, "Bom" );
    m_xml_root->InsertChild( bomNode, aEcadNode );
    addAttribute( bomNode,  "name", genString( fn.GetName(), "BOM" ) );

    wxXmlNode* bomHeaderNode = appendNode( bomNode, "BomHeader" );
    addAttribute( bomHeaderNode,  "revision", "1.0" );
    addAttribute( bomHeaderNode,  "assembly", genString( fn.GetName() ) );

    wxXmlNode* stepRefNode = appendNode( bomHeaderNode, "StepRef" );
    addAttribute( stepRefNode,  "name", genString( fn.GetName(), "BOARD" ) );

    for( const auto& entry : bom_entries )
    {
        wxXmlNode* bomEntryNode = appendNode( bomNode, "BomItem" );
        addAttribute( bomEntryNode,  "OEMDesignNumberRef", entry->m_OEMDesignRef );
        addAttribute( bomEntryNode,  "quantity", wxString::Format( "%d", entry->m_count ) );
        addAttribute( bomEntryNode,  "pinCount", wxString::Format( "%d", entry->m_pads ) );
        addAttribute( bomEntryNode,  "category", entry->m_type );

        if( !entry->m_description.IsEmpty() )
            addAttribute( bomEntryNode, "description", entry->m_description );

        for( const REFDES& refdes : *( entry->m_refdes ) )
        {
            wxXmlNode* refdesNode = appendNode( bomEntryNode, "RefDes" );
            addAttribute( refdesNode,  "name", refdes.m_name );
            addAttribute( refdesNode,  "packageRef", genString( refdes.m_pkg, "PKG" ) );
            addAttribute( refdesNode,  "populate", refdes.m_populate ? "true" : "false" );
            addAttribute( refdesNode,  "layerRef", refdes.m_layer );
        }

        wxXmlNode* characteristicsNode = appendNode( bomEntryNode, "Characteristics" );
        addAttribute( characteristicsNode,  "category", entry->m_type );

        for( const auto& prop : *( entry->m_props ) )
        {
            wxXmlNode* textualDefNode = appendNode( characteristicsNode, "Textual" );
            addAttribute( textualDefNode,  "definitionSource", "KICAD" );
            addAttribute( textualDefNode,  "textualCharacteristicName", prop.first );
            addAttribute( textualDefNode,  "textualCharacteristicValue", prop.second );
        }
    }

    return bomNode;
}


wxXmlNode* PCB_IO_IPC2581::generateEcadSection()
{
    if( m_progressReporter )
        m_progressReporter->AdvancePhase( _( "Generating CAD data" ) );

    wxXmlNode* ecadNode = appendNode( m_xml_root, "Ecad" );
    addAttribute( ecadNode,  "name", "Design" );

    addCadHeader( ecadNode );

    wxXmlNode* cadDataNode = appendNode( ecadNode, "CadData" );
    generateCadLayers( cadDataNode );
    generateDrillLayers( cadDataNode );
    generateAuxilliaryLayers( cadDataNode );
    generateStackup( cadDataNode );
    generateStepSection( cadDataNode );

    pruneUnusedBackdrillSpecs();

    return ecadNode;
}


void PCB_IO_IPC2581::generateCadSpecs( wxXmlNode* aCadLayerNode )
{
    BOARD_DESIGN_SETTINGS& dsnSettings = m_board->GetDesignSettings();
    BOARD_STACKUP&         stackup = dsnSettings.GetStackupDescriptor();
    stackup.SynchronizeWithBoard( &dsnSettings );

    std::vector<BOARD_STACKUP_ITEM*> layers = stackup.GetList();
    std::set<PCB_LAYER_ID> added_layers;

    for( int i = 0; i < stackup.GetCount(); i++ )
    {
        BOARD_STACKUP_ITEM* stackup_item = layers.at( i );

        for( int sublayer_id = 0; sublayer_id < stackup_item->GetSublayersCount(); sublayer_id++ )
        {
            wxString ly_name = stackup_item->GetLayerName();

            if( ly_name.IsEmpty() )
            {
                if( IsValidLayer( stackup_item->GetBrdLayerId() ) )
                    ly_name = m_board->GetLayerName( stackup_item->GetBrdLayerId() );

                if( ly_name.IsEmpty() && stackup_item->GetType() == BS_ITEM_TYPE_DIELECTRIC )
                {
                    ly_name = wxString::Format( "DIELECTRIC_%d", stackup_item->GetDielectricLayerId() );

                    if( sublayer_id > 0 )
                        ly_name += wxString::Format( "_%d", sublayer_id );
                }
            }

            ly_name = genString( ly_name, "SPEC_LAYER" );

            wxXmlNode* specNode = appendNode( aCadLayerNode, "Spec" );
            addAttribute( specNode,  "name", ly_name );
            wxXmlNode* generalNode = appendNode( specNode, "General" );
            addAttribute( generalNode,  "type", "MATERIAL" );
            wxXmlNode* propertyNode = appendNode( generalNode, "Property" );

            switch ( stackup_item->GetType() )
            {
                case BS_ITEM_TYPE_COPPER:
                {
                    addAttribute( propertyNode, "text", "COPPER" );
                    wxXmlNode* conductorNode = appendNode( specNode, "Conductor" );
                    addAttribute( conductorNode, "type", "CONDUCTIVITY" );
                    propertyNode = appendNode( conductorNode, "Property" );
                    addAttribute( propertyNode, "unit", wxT( "SIEMENS/M" ) );
                    addAttribute( propertyNode, "value", wxT( "5.959E7" ) );
                    break;
                }
                case BS_ITEM_TYPE_DIELECTRIC:
                {
                    addAttribute( propertyNode, "text", stackup_item->GetMaterial() );
                    propertyNode = appendNode( generalNode, "Property" );
                    addAttribute( propertyNode,  "text", wxString::Format( "Type : %s",
                                                                        stackup_item->GetTypeName() ) );
                    wxXmlNode* dielectricNode = appendNode( specNode, "Dielectric" );
                    addAttribute( dielectricNode, "type", "DIELECTRIC_CONSTANT" );
                    propertyNode = appendNode( dielectricNode, "Property" );
                    addAttribute( propertyNode, "value",
                                  floatVal( stackup_item->GetEpsilonR( sublayer_id ) ) );
                    dielectricNode = appendNode( specNode, "Dielectric" );
                    addAttribute( dielectricNode, "type", "LOSS_TANGENT" );
                    propertyNode = appendNode( dielectricNode, "Property" );
                    addAttribute( propertyNode, "value",
                                  floatVal( stackup_item->GetLossTangent( sublayer_id ) ) );
                    break;
                }
                case BS_ITEM_TYPE_SILKSCREEN:
                    addAttribute( propertyNode,  "text", stackup_item->GetTypeName() );
                    propertyNode = appendNode( generalNode, "Property" );
                    addAttribute( propertyNode,  "text", wxString::Format( "Color : %s",
                                                                        stackup_item->GetColor() ) );
                    propertyNode = appendNode( generalNode, "Property" );
                    addAttribute( propertyNode,  "text", wxString::Format( "Type : %s",
                                                                        stackup_item->GetTypeName() ) );
                    break;
                case BS_ITEM_TYPE_SOLDERMASK:
                {
                    addAttribute( propertyNode,  "text", "SOLDERMASK" );
                    propertyNode = appendNode( generalNode, "Property" );
                    addAttribute( propertyNode,  "text", wxString::Format( "Color : %s",
                                                                        stackup_item->GetColor() ) );
                    propertyNode = appendNode( generalNode, "Property" );
                    addAttribute( propertyNode,  "text", wxString::Format( "Type : %s",
                                                                        stackup_item->GetTypeName() ) );

                    // Generate Epsilon R if > 1.0 (value <= 1.0 means not specified)
                    if( stackup_item->GetEpsilonR( sublayer_id ) > 1.0 )
                    {
                        wxXmlNode* dielectricNode = appendNode( specNode, "Dielectric" );
                        addAttribute( dielectricNode, "type", "DIELECTRIC_CONSTANT" );
                        propertyNode = appendNode( dielectricNode, "Property" );
                        addAttribute( propertyNode, "value", floatVal( stackup_item->GetEpsilonR( sublayer_id ) ) );
                    }

                    // Generate LossTangent if > 0.0 (value <= 0.0 means not specified)
                    if( stackup_item->GetLossTangent( sublayer_id ) > 0.0 )
                    {
                        wxXmlNode* dielectricNode = appendNode( specNode, "Dielectric" );
                        addAttribute( dielectricNode, "type", "LOSS_TANGENT" );
                        propertyNode = appendNode( dielectricNode, "Property" );
                        addAttribute( propertyNode, "value", floatVal( stackup_item->GetLossTangent( sublayer_id ) ) );
                    }
                    break;
                }
                default:
                    break;
            }
        }
    }

    // Generate SurfaceFinish spec from board's copper finish setting
    surfaceFinishType finishType = getSurfaceFinishType( stackup.m_FinishType );

    if( finishType != surfaceFinishType::NONE )
    {
        wxXmlNode* specNode = appendNode( aCadLayerNode, "Spec" );
        addAttribute( specNode, "name", "SURFACE_FINISH" );

        wxXmlNode* surfaceFinishNode = appendNode( specNode, "SurfaceFinish" );
        wxXmlNode* finishNode = appendNode( surfaceFinishNode, "Finish" );
        addAttribute( finishNode, "type", surfaceFinishTypeToString.at( finishType ) );

        // Add original finish string as comment if it maps to OTHER
        if( finishType == surfaceFinishType::OTHER )
            addAttribute( finishNode, "comment", stackup.m_FinishType );
    }
}


void PCB_IO_IPC2581::addCadHeader( wxXmlNode* aEcadNode )
{
    wxXmlNode* cadHeaderNode = appendNode( aEcadNode, "CadHeader" );
    addAttribute( cadHeaderNode,  "units", m_units_str );

    m_cad_header_node = cadHeaderNode;

    generateCadSpecs( cadHeaderNode );
}


bool PCB_IO_IPC2581::isValidLayerFor2581( PCB_LAYER_ID aLayer )
{
    return ( aLayer >= F_Cu && aLayer <= User_9 ) || aLayer == UNDEFINED_LAYER;
}


void PCB_IO_IPC2581::addLayerAttributes( wxXmlNode* aNode, PCB_LAYER_ID aLayer )
{
    switch( aLayer )
    {
    case F_Adhes:
    case B_Adhes:
        addAttribute( aNode,  "layerFunction", "GLUE" );
        addAttribute( aNode,  "polarity", "POSITIVE" );
        addAttribute( aNode,  "side", aLayer == F_Adhes ? "TOP" : "BOTTOM" );
        break;
    case F_Paste:
    case B_Paste:
        addAttribute( aNode,  "layerFunction", "SOLDERPASTE" );
        addAttribute( aNode,  "polarity", "POSITIVE" );
        addAttribute( aNode,  "side", aLayer == F_Paste ? "TOP" : "BOTTOM" );
        break;
    case F_SilkS:
    case B_SilkS:
        addAttribute( aNode,  "layerFunction", "SILKSCREEN" );
        addAttribute( aNode,  "polarity", "POSITIVE" );
        addAttribute( aNode,  "side", aLayer == F_SilkS ? "TOP" : "BOTTOM" );
        break;
    case F_Mask:
    case B_Mask:
        addAttribute( aNode,  "layerFunction", "SOLDERMASK" );
        addAttribute( aNode,  "polarity", "POSITIVE" );
        addAttribute( aNode,  "side", aLayer == F_Mask ? "TOP" : "BOTTOM" );
        break;
    case Edge_Cuts:
        addAttribute( aNode,  "layerFunction", "BOARD_OUTLINE" );
        addAttribute( aNode,  "polarity", "POSITIVE" );
        addAttribute( aNode,  "side", "ALL" );
        break;
    case B_CrtYd:
    case F_CrtYd:
        addAttribute( aNode,  "layerFunction", "COURTYARD" );
        addAttribute( aNode,  "polarity", "POSITIVE" );
        addAttribute( aNode,  "side", aLayer == F_CrtYd ? "TOP" : "BOTTOM" );
        break;
    case B_Fab:
    case F_Fab:
        addAttribute( aNode,  "layerFunction", "ASSEMBLY" );
        addAttribute( aNode,  "polarity", "POSITIVE" );
        addAttribute( aNode,  "side", aLayer == F_Fab ? "TOP" : "BOTTOM" );
        break;
    case Dwgs_User:
    case Cmts_User:
    case Eco1_User:
    case Eco2_User:
    case Margin:
    case User_1:
    case User_2:
    case User_3:
    case User_4:
    case User_5:
    case User_6:
    case User_7:
    case User_8:
    case User_9:
        addAttribute( aNode,  "layerFunction", "DOCUMENT" );
        addAttribute( aNode,  "polarity", "POSITIVE" );
        addAttribute( aNode,  "side", "NONE" );
        break;

    default:
        if( IsCopperLayer( aLayer ) )
        {
            addAttribute( aNode, "layerFunction", "CONDUCTOR" );
            addAttribute( aNode, "polarity", "POSITIVE" );
            addAttribute( aNode, "side",
                          aLayer == F_Cu ? "TOP"
                                         : aLayer == B_Cu ? "BOTTOM"
                                                          : "INTERNAL" );
        }

        break; // Do not handle other layers
    }
}


void PCB_IO_IPC2581::generateStackup( wxXmlNode* aCadLayerNode )
{
    BOARD_DESIGN_SETTINGS& dsnSettings = m_board->GetDesignSettings();
    BOARD_STACKUP&         stackup = dsnSettings.GetStackupDescriptor();
    stackup.SynchronizeWithBoard( &dsnSettings );

    surfaceFinishType finishType = getSurfaceFinishType( stackup.m_FinishType );
    bool              hasCoating = ( finishType != surfaceFinishType::NONE );

    wxXmlNode* stackupNode = appendNode( aCadLayerNode, "Stackup" );
    addAttribute( stackupNode, "name", "Primary_Stackup" );
    addAttribute( stackupNode, "overallThickness", floatVal( m_scale * stackup.BuildBoardThicknessFromStackup() ) );
    addAttribute( stackupNode, "tolPlus", "0.0" );
    addAttribute( stackupNode, "tolMinus", "0.0" );
    addAttribute( stackupNode, "whereMeasured", "MASK" );

    if( m_version > 'B' )
        addAttribute( stackupNode, "stackupStatus", "PROPOSED" );

    wxXmlNode* stackupGroup = appendNode( stackupNode, "StackupGroup" );
    addAttribute( stackupGroup, "name", "Primary_Stackup_Group" );
    addAttribute( stackupGroup, "thickness", floatVal( m_scale * stackup.BuildBoardThicknessFromStackup() ) );
    addAttribute( stackupGroup, "tolPlus", "0.0" );
    addAttribute( stackupGroup, "tolMinus", "0.0" );

    std::vector<BOARD_STACKUP_ITEM*> layers = stackup.GetList();
    std::set<PCB_LAYER_ID> added_layers;
    int sequence = 0;

    for( int i = 0; i < stackup.GetCount(); i++ )
    {
        BOARD_STACKUP_ITEM* stackup_item = layers.at( i );

        for( int sublayer_id = 0; sublayer_id < stackup_item->GetSublayersCount(); sublayer_id++ )
        {
            PCB_LAYER_ID layer_id = stackup_item->GetBrdLayerId();

            // Insert top coating layer before F.Cu
            if( hasCoating && layer_id == F_Cu && sublayer_id == 0 )
            {
                wxXmlNode* coatingLayer = appendNode( stackupGroup, "StackupLayer" );
                addAttribute( coatingLayer, "layerOrGroupRef", "COATING_TOP" );
                addAttribute( coatingLayer, "thickness", "0.0" );
                addAttribute( coatingLayer, "tolPlus", "0.0" );
                addAttribute( coatingLayer, "tolMinus", "0.0" );
                addAttribute( coatingLayer, "sequence", wxString::Format( "%d", sequence++ ) );

                wxXmlNode* specRefNode = appendNode( coatingLayer, "SpecRef" );
                addAttribute( specRefNode, "id", "SURFACE_FINISH" );
            }

            wxXmlNode* stackupLayer = appendNode( stackupGroup, "StackupLayer" );
            wxString ly_name = stackup_item->GetLayerName();

            if( ly_name.IsEmpty() )
            {
                if( IsValidLayer( stackup_item->GetBrdLayerId() ) )
                    ly_name = m_board->GetLayerName( stackup_item->GetBrdLayerId() );

                if( ly_name.IsEmpty() && stackup_item->GetType() == BS_ITEM_TYPE_DIELECTRIC )
                {
                    ly_name = wxString::Format( "DIELECTRIC_%d", stackup_item->GetDielectricLayerId() );

                    if( sublayer_id > 0 )
                        ly_name += wxString::Format( "_%d", sublayer_id );
                }
            }

            wxString spec_name = genString( ly_name, "SPEC_LAYER" );
            ly_name = genString( ly_name, "LAYER" );

            addAttribute( stackupLayer,  "layerOrGroupRef", ly_name );
            addAttribute( stackupLayer,  "thickness", floatVal( m_scale * stackup_item->GetThickness() ) );
            addAttribute( stackupLayer,  "tolPlus", "0.0" );
            addAttribute( stackupLayer,  "tolMinus", "0.0" );
            addAttribute( stackupLayer,  "sequence", wxString::Format( "%d", sequence++ ) );

            wxXmlNode* specLayerNode = appendNode( stackupLayer, "SpecRef" );
            addAttribute( specLayerNode, "id", spec_name );

            // Insert bottom coating layer after B.Cu
            if( hasCoating && layer_id == B_Cu && sublayer_id == stackup_item->GetSublayersCount() - 1 )
            {
                wxXmlNode* coatingLayer = appendNode( stackupGroup, "StackupLayer" );
                addAttribute( coatingLayer, "layerOrGroupRef", "COATING_BOTTOM" );
                addAttribute( coatingLayer, "thickness", "0.0" );
                addAttribute( coatingLayer, "tolPlus", "0.0" );
                addAttribute( coatingLayer, "tolMinus", "0.0" );
                addAttribute( coatingLayer, "sequence", wxString::Format( "%d", sequence++ ) );

                wxXmlNode* specRefNode = appendNode( coatingLayer, "SpecRef" );
                addAttribute( specRefNode, "id", "SURFACE_FINISH" );
            }
        }
    }
}


void PCB_IO_IPC2581::generateCadLayers( wxXmlNode* aCadLayerNode )
{

    BOARD_DESIGN_SETTINGS& dsnSettings = m_board->GetDesignSettings();
    BOARD_STACKUP&         stackup = dsnSettings.GetStackupDescriptor();
    stackup.SynchronizeWithBoard( &dsnSettings );

    std::vector<BOARD_STACKUP_ITEM*> layers = stackup.GetList();
    std::set<PCB_LAYER_ID> added_layers;

    for( int i = 0; i < stackup.GetCount(); i++ )
    {
        BOARD_STACKUP_ITEM* stackup_item = layers.at( i );

        if( !isValidLayerFor2581( stackup_item->GetBrdLayerId() ) )
            continue;

        for( int sublayer_id = 0; sublayer_id < stackup_item->GetSublayersCount(); sublayer_id++ )
        {
            wxXmlNode* cadLayerNode = appendNode( aCadLayerNode, "Layer" );
            wxString ly_name = stackup_item->GetLayerName();

            if( ly_name.IsEmpty() )
            {

                if( IsValidLayer( stackup_item->GetBrdLayerId() ) )
                    ly_name = m_board->GetLayerName( stackup_item->GetBrdLayerId() );

                if( ly_name.IsEmpty() && stackup_item->GetType() == BS_ITEM_TYPE_DIELECTRIC )
                {
                    ly_name = wxString::Format( "DIELECTRIC_%d", stackup_item->GetDielectricLayerId() );

                    if( sublayer_id > 0 )
                        ly_name += wxString::Format( "_%d", sublayer_id );
                }
            }

            ly_name = genString( ly_name, "LAYER" );

            addAttribute( cadLayerNode,  "name", ly_name );

            if( stackup_item->GetType() == BS_ITEM_TYPE_DIELECTRIC )
            {
                if( stackup_item->GetTypeName() == KEY_CORE )
                    addAttribute( cadLayerNode,  "layerFunction", "DIELCORE" );
                else
                    addAttribute( cadLayerNode,  "layerFunction", "DIELPREG" );

                addAttribute( cadLayerNode,  "polarity", "POSITIVE" );
                addAttribute( cadLayerNode,  "side", "INTERNAL" );
                continue;
            }
            else
            {
                added_layers.insert( stackup_item->GetBrdLayerId() );
                addLayerAttributes( cadLayerNode, stackup_item->GetBrdLayerId() );
                m_layer_name_map.emplace( stackup_item->GetBrdLayerId(), ly_name );
            }
        }
    }

    LSEQ layer_seq = m_board->GetEnabledLayers().Seq();

    for( PCB_LAYER_ID layer : layer_seq )
    {
        if( added_layers.find( layer ) != added_layers.end() || !isValidLayerFor2581( layer ) )
            continue;

        wxString ly_name = genLayerString( layer, "LAYER" );
        m_layer_name_map.emplace( layer, ly_name );
        added_layers.insert( layer );
        wxXmlNode* cadLayerNode = appendNode( aCadLayerNode, "Layer" );
        addAttribute( cadLayerNode,  "name", ly_name );

        addLayerAttributes( cadLayerNode, layer );
    }

    // Generate COATINGCOND layers for surface finish if specified
    surfaceFinishType finishType = getSurfaceFinishType( stackup.m_FinishType );

    if( finishType != surfaceFinishType::NONE )
    {
        wxXmlNode* topCoatingNode = appendNode( aCadLayerNode, "Layer" );
        addAttribute( topCoatingNode, "name", "COATING_TOP" );
        addAttribute( topCoatingNode, "layerFunction", "COATINGCOND" );
        addAttribute( topCoatingNode, "side", "TOP" );
        addAttribute( topCoatingNode, "polarity", "POSITIVE" );

        wxXmlNode* botCoatingNode = appendNode( aCadLayerNode, "Layer" );
        addAttribute( botCoatingNode, "name", "COATING_BOTTOM" );
        addAttribute( botCoatingNode, "layerFunction", "COATINGCOND" );
        addAttribute( botCoatingNode, "side", "BOTTOM" );
        addAttribute( botCoatingNode, "polarity", "POSITIVE" );
    }
}


void PCB_IO_IPC2581::generateDrillLayers( wxXmlNode* aCadLayerNode )
{
    for( BOARD_ITEM* item : m_board->Tracks() )
    {
        if( item->Type() == PCB_VIA_T )
        {
            PCB_VIA* via = static_cast<PCB_VIA*>( item );
            m_drill_layers[std::make_pair( via->TopLayer(), via->BottomLayer() )].push_back( via );
        }
    }

    for( FOOTPRINT* fp : m_board->Footprints() )
    {
        for( PAD* pad : fp->Pads() )
        {
            if( pad->HasDrilledHole() )
                m_drill_layers[std::make_pair( F_Cu, B_Cu )].push_back( pad );
            else if( pad->HasHole() )
                m_slot_holes[std::make_pair( F_Cu, B_Cu )].push_back( pad );
        }
    }

    for( const auto& [layers, vec] : m_drill_layers )
    {
        wxXmlNode* drillNode = appendNode( aCadLayerNode, "Layer" );
        drillNode->AddAttribute( "name", genLayersString( layers.first, layers.second, "DRILL" ) );
        addAttribute( drillNode,  "layerFunction", "DRILL" );
        addAttribute( drillNode,  "polarity", "POSITIVE" );
        addAttribute( drillNode,  "side", "ALL" );

        wxXmlNode* spanNode = appendNode( drillNode, "Span" );
        addAttribute( spanNode,  "fromLayer", genLayerString( layers.first, "LAYER" ) );
        addAttribute( spanNode,  "toLayer", genLayerString( layers.second, "LAYER" ) );
    }

    for( const auto& [layers, vec] : m_slot_holes )
    {
        wxXmlNode* drillNode = appendNode( aCadLayerNode, "Layer" );
        drillNode->AddAttribute( "name", genLayersString( layers.first, layers.second, "SLOT" ) );

        addAttribute( drillNode,  "layerFunction", "ROUT" );
        addAttribute( drillNode,  "polarity", "POSITIVE" );
        addAttribute( drillNode,  "side", "ALL" );

        wxXmlNode* spanNode = appendNode( drillNode, "Span" );
        addAttribute( spanNode,  "fromLayer", genLayerString( layers.first, "LAYER" ) );
        addAttribute( spanNode,  "toLayer", genLayerString( layers.second, "LAYER" ) );
    }
}


void PCB_IO_IPC2581::generateAuxilliaryLayers( wxXmlNode* aCadLayerNode )
{
    for( BOARD_ITEM* item : m_board->Tracks() )
    {
        if( item->Type() != PCB_VIA_T )
            continue;

        PCB_VIA* via = static_cast<PCB_VIA*>( item );

        std::vector<std::tuple<auxLayerType, PCB_LAYER_ID, PCB_LAYER_ID>> new_layers;

        if( via->Padstack().IsFilled().value_or( false ) )
            new_layers.emplace_back( auxLayerType::FILLING, via->TopLayer(), via->BottomLayer() );

        if( via->Padstack().IsCapped().value_or( false ) )
            new_layers.emplace_back( auxLayerType::CAPPING, via->TopLayer(), via->BottomLayer() );

        for( PCB_LAYER_ID layer : { via->TopLayer(), via->BottomLayer() } )
        {
            if( via->Padstack().IsPlugged( layer ).value_or( false ) )
                new_layers.emplace_back( auxLayerType::PLUGGING, layer, UNDEFINED_LAYER );

            if( via->Padstack().IsCovered( layer ).value_or( false ) )
                new_layers.emplace_back( auxLayerType::COVERING, layer, UNDEFINED_LAYER );

            if( via->Padstack().IsTented( layer ).value_or( false ) )
                new_layers.emplace_back( auxLayerType::TENTING, layer, UNDEFINED_LAYER );
        }

        for( auto& tuple : new_layers )
            m_auxilliary_Layers[tuple].push_back( via );
    }

    for( const auto& [layers, vec] : m_auxilliary_Layers )
    {
        bool add_node = true;

        wxString name;
        wxString layerFunction;

        // clang-format off: suggestion is inconsitent
        switch( std::get<0>(layers) )
        {
        case auxLayerType::COVERING:
            name = "COVERING";
            layerFunction = "COATINGNONCOND";
            break;
        case auxLayerType::PLUGGING:
            name = "PLUGGING";
            layerFunction = "HOLEFILL";
            break;
        case auxLayerType::TENTING:
            name = "TENTING";
            layerFunction = "COATINGNONCOND";
            break;
        case auxLayerType::FILLING:
            name = "FILLING";
            layerFunction = "HOLEFILL";
            break;
        case auxLayerType::CAPPING:
            name = "CAPPING";
            layerFunction = "COATINGCOND";
            break;
        default:
            add_node = false;
            break;
        }
        // clang-format on: suggestion is inconsitent

        if( add_node && !vec.empty() )
        {
            wxXmlNode* node = appendNode( aCadLayerNode, "LAYER" );
            addAttribute( node, "layerFunction", layerFunction );
            addAttribute( node, "polarity", "POSITIVE" );

            if( std::get<2>( layers ) == UNDEFINED_LAYER )
            {
                addAttribute( node, "name", genLayerString( std::get<1>( layers ), TO_UTF8( name ) ) );
                addAttribute( node, "side", IsFrontLayer( std::get<1>( layers ) ) ? "TOP" : "BOTTOM" );
            }
            else
            {
                addAttribute( node, "name",
                              genLayersString( std::get<1>( layers ), std::get<2>( layers ), TO_UTF8( name ) ) );

                const bool first_external = std::get<1>( layers ) == F_Cu || std::get<1>( layers ) == B_Cu;
                const bool second_external = std::get<2>( layers ) == F_Cu || std::get<2>( layers ) == B_Cu;

                if( first_external )
                {
                    if( second_external )
                        addAttribute( node, "side", "ALL" );
                    else
                        addAttribute( node, "side", "FRONT" );
                }
                else
                {
                    if( second_external )
                        addAttribute( node, "side", "BACK" );
                    else
                        addAttribute( node, "side", "INTERNAL" );
                }

                wxXmlNode* spanNode = appendNode( node, "SPAN" );
                addAttribute( spanNode, "fromLayer", genLayerString( std::get<1>( layers ), "LAYER" ) );
                addAttribute( spanNode, "toLayer", genLayerString( std::get<2>( layers ), "LAYER" ) );
            }
        }
    }
}


void PCB_IO_IPC2581::generateStepSection( wxXmlNode* aCadNode )
{
    wxXmlNode* stepNode = appendNode( aCadNode, "Step" );
    wxFileName fn( m_board->GetFileName() );
    addAttribute( stepNode,  "name", genString( fn.GetName(), "BOARD" ) );

    if( m_version > 'B' )
        addAttribute( stepNode,  "type", "BOARD" );

    wxXmlNode* datumNode = appendNode( stepNode, "Datum" );
    addAttribute( datumNode,  "x", "0.0" );
    addAttribute( datumNode,  "y", "0.0" );

    generateProfile( stepNode );
    generateComponents( stepNode );

    m_last_padstack = insertNode( stepNode, "NonstandardAttribute" );
    addAttribute( m_last_padstack,  "name", "FOOTPRINT_COUNT" );
    addAttribute( m_last_padstack,  "type", "INTEGER" );
    addAttribute( m_last_padstack,  "value", wxString::Format( "%zu", m_board->Footprints().size() ) );

    generateLayerFeatures( stepNode );
    generateLayerSetDrill( stepNode );
    generateLayerSetAuxilliary( stepNode );
}


void PCB_IO_IPC2581::addPad( wxXmlNode* aContentNode, const PAD* aPad, PCB_LAYER_ID aLayer )
{
    wxXmlNode* padNode = appendNode( aContentNode, "Pad" );
    FOOTPRINT* fp = aPad->GetParentFootprint();

    addPadStack( padNode, aPad );

    if( aPad->GetOrientation() != ANGLE_0 )
    {
        wxXmlNode* xformNode = appendNode( padNode, "Xform" );
        EDA_ANGLE angle = aPad->GetOrientation().Normalize();

        xformNode->AddAttribute( "rotation", floatVal( angle.AsDegrees() ) );
    }

    addLocationNode( padNode, *aPad, false );
    addShape( padNode, *aPad, aLayer );

    if( fp )
    {
        wxXmlNode* pinRefNode = appendNode( padNode, "PinRef" );

        addAttribute( pinRefNode,  "componentRef", componentName( fp ) );
        addAttribute( pinRefNode,  "pin", pinName( aPad ) );
    }
}


void PCB_IO_IPC2581::addVia( wxXmlNode* aContentNode, const PCB_VIA* aVia, PCB_LAYER_ID aLayer )
{
    if( !aVia->FlashLayer( aLayer ) )
        return;

    wxXmlNode* padNode = appendNode( aContentNode, "Pad" );

    addPadStack( padNode, aVia );
    addLocationNode( padNode, aVia->GetPosition().x, aVia->GetPosition().y );

    PAD dummy( nullptr );
    int hole = aVia->GetDrillValue();
    dummy.SetDrillSize( VECTOR2I( hole, hole ) );
    dummy.SetPosition( aVia->GetStart() );
    dummy.SetSize( aLayer, VECTOR2I( aVia->GetWidth( aLayer ), aVia->GetWidth( aLayer ) ) );

    addShape( padNode, dummy, aLayer );
}


void PCB_IO_IPC2581::addPadStack( wxXmlNode* aPadNode, const PAD* aPad )
{
    size_t hash = hash_fp_item( aPad, 0 );
    wxString name = wxString::Format( "PADSTACK_%zu", m_padstack_dict.size() + 1 );
    auto [ th_pair, success ] = m_padstack_dict.emplace( hash, name );

    addAttribute( aPadNode,  "padstackDefRef", th_pair->second );

    // If we did not insert a new padstack, then we have already added it to the XML
    // and we don't need to add it again.
    if( !success )
        return;

    wxXmlNode* padStackDefNode = new wxXmlNode( wxXML_ELEMENT_NODE, "PadStackDef" );
    addAttribute( padStackDefNode,  "name", name );
    ensureBackdrillSpecs( name, aPad->Padstack() );
    m_padstacks.push_back( padStackDefNode );

    if( m_last_padstack )
    {
        insertNodeAfter( m_last_padstack, padStackDefNode );
        m_last_padstack = padStackDefNode;
    }

    // Only handle round holes here because IPC2581 does not support non-round holes
    // These will be handled in a slot layer
    if( aPad->HasDrilledHole() )
    {
        wxXmlNode* padStackHoleNode = appendNode( padStackDefNode, "PadstackHoleDef" );
        padStackHoleNode->AddAttribute( "name",
                                        wxString::Format( "%s%d_%d",
                                                          aPad->GetAttribute() == PAD_ATTRIB::PTH ? "PTH" : "NPTH",
                                                          aPad->GetDrillSizeX(), aPad->GetDrillSizeY() ) );

        addAttribute( padStackHoleNode,  "diameter", floatVal( m_scale * aPad->GetDrillSizeX() ) );
        addAttribute( padStackHoleNode,  "platingStatus",
                      aPad->GetAttribute() == PAD_ATTRIB::PTH ? "PLATED" : "NONPLATED" );
        addAttribute( padStackHoleNode,  "plusTol", "0.0" );
        addAttribute( padStackHoleNode,  "minusTol", "0.0" );
        addXY( padStackHoleNode, aPad->GetOffset( PADSTACK::ALL_LAYERS ) );
    }

    LSEQ layer_seq = aPad->GetLayerSet().Seq();

    for( PCB_LAYER_ID layer : layer_seq )
    {
        FOOTPRINT* fp = aPad->GetParentFootprint();

        if( !m_board->IsLayerEnabled( layer ) )
            continue;

        wxXmlNode* padStackPadDefNode = appendNode( padStackDefNode, "PadstackPadDef" );
        addAttribute( padStackPadDefNode,  "layerRef", m_layer_name_map[layer] );
        addAttribute( padStackPadDefNode,  "padUse", "REGULAR" );
        addLocationNode( padStackPadDefNode, aPad->GetOffset( PADSTACK::ALL_LAYERS ).x, aPad->GetOffset( PADSTACK::ALL_LAYERS ).y );

        if( aPad->HasHole() || !aPad->FlashLayer( layer ) )
        {
            PCB_SHAPE shape( nullptr, SHAPE_T::CIRCLE );
            shape.SetStart( aPad->GetOffset( PADSTACK::ALL_LAYERS ) );
            shape.SetEnd( shape.GetStart() + aPad->GetDrillSize() / 2 );
            addShape( padStackPadDefNode, shape );
        }
        else
        {
            addShape( padStackPadDefNode, *aPad, layer );
        }
    }
}


void PCB_IO_IPC2581::addPadStack( wxXmlNode* aContentNode, const PCB_VIA* aVia )
{
    size_t hash = hash_fp_item( aVia, 0 );
    wxString name = wxString::Format( "PADSTACK_%zu", m_padstack_dict.size() + 1 );
    auto [ via_pair, success ] = m_padstack_dict.emplace( hash, name );

    addAttribute( aContentNode,  "padstackDefRef", via_pair->second );

    // If we did not insert a new padstack, then we have already added it to the XML
    // and we don't need to add it again.
    if( !success )
        return;

    wxXmlNode* padStackDefNode = new wxXmlNode( wxXML_ELEMENT_NODE, "PadStackDef" );
    insertNodeAfter( m_last_padstack, padStackDefNode );
    m_last_padstack = padStackDefNode;
    addAttribute( padStackDefNode,  "name", name );
    ensureBackdrillSpecs( name, aVia->Padstack() );

    wxXmlNode* padStackHoleNode = appendNode( padStackDefNode, "PadstackHoleDef" );
    addAttribute( padStackHoleNode, "name", wxString::Format( "PH%d", aVia->GetDrillValue() ) );
    padStackHoleNode->AddAttribute( "diameter", floatVal( m_scale * aVia->GetDrillValue() ) );
    addAttribute( padStackHoleNode, "platingStatus", "VIA" );
    addAttribute( padStackHoleNode, "plusTol", "0.0" );
    addAttribute( padStackHoleNode, "minusTol", "0.0" );
    addAttribute( padStackHoleNode, "x", "0.0" );
    addAttribute( padStackHoleNode, "y", "0.0" );

    LSEQ layer_seq = aVia->GetLayerSet().Seq();

    auto addPadShape{ [&]( PCB_LAYER_ID layer, const PCB_VIA* aVia, const wxString& name,
                           bool drill ) -> void
                      {
                          PCB_SHAPE shape( nullptr, SHAPE_T::CIRCLE );

                          if( drill )
                              shape.SetEnd( { KiROUND( aVia->GetDrillValue() / 2.0 ), 0 } );
                          else
                              shape.SetEnd( { KiROUND( aVia->GetWidth( layer ) / 2.0 ), 0 } );

                          wxXmlNode* padStackPadDefNode =
                                  appendNode( padStackDefNode, "PadstackPadDef" );
                          addAttribute( padStackPadDefNode, "layerRef", name );
                          addAttribute( padStackPadDefNode, "padUse", "REGULAR" );

                          addLocationNode( padStackPadDefNode, 0.0, 0.0 );
                          addShape( padStackPadDefNode, shape );
                      } };

    for( PCB_LAYER_ID layer : layer_seq )
    {
        if( !aVia->FlashLayer( layer ) || !m_board->IsLayerEnabled( layer ) )
            continue;

        addPadShape( layer, aVia, m_layer_name_map[layer], false );
    }

    if( aVia->Padstack().IsFilled().value_or( false ) )
        addPadShape( UNDEFINED_LAYER, aVia, genLayersString( aVia->TopLayer(), aVia->BottomLayer(), "FILLING" ), true );

    if( aVia->Padstack().IsCapped().value_or( false ) )
        addPadShape( UNDEFINED_LAYER, aVia, genLayersString( aVia->TopLayer(), aVia->BottomLayer(), "CAPPING" ), true );

    for( PCB_LAYER_ID layer : { aVia->TopLayer(), aVia->BottomLayer() } )
    {
        if( aVia->Padstack().IsPlugged( layer ).value_or( false ) )
            addPadShape( layer, aVia, genLayerString( layer, "PLUGGING" ), true );

        if( aVia->Padstack().IsCovered( layer ).value_or( false ) )
            addPadShape( layer, aVia, genLayerString( layer, "COVERING" ), false );

        if( aVia->Padstack().IsTented( layer ).value_or( false ) )
            addPadShape( layer, aVia, genLayerString( layer, "TENTING" ), false );
    }
}


void PCB_IO_IPC2581::ensureBackdrillSpecs( const wxString& aPadstackName, const PADSTACK& aPadstack )
{
    if( m_padstack_backdrill_specs.find( aPadstackName ) != m_padstack_backdrill_specs.end() )
        return;

    const PADSTACK::DRILL_PROPS& secondary = aPadstack.SecondaryDrill();

    if( secondary.start == UNDEFINED_LAYER || secondary.end == UNDEFINED_LAYER )
        return;

    if( secondary.size.x <= 0 && secondary.size.y <= 0 )
        return;

    if( !m_cad_header_node )
        return;

    auto layerHasRef = [&]( PCB_LAYER_ID aLayer ) -> bool
    {
        return m_layer_name_map.find( aLayer ) != m_layer_name_map.end();
    };

    if( !layerHasRef( secondary.start ) || !layerHasRef( secondary.end ) )
        return;

    BOARD_DESIGN_SETTINGS& dsnSettings = m_board->GetDesignSettings();
    BOARD_STACKUP&         stackup = dsnSettings.GetStackupDescriptor();
    stackup.SynchronizeWithBoard( &dsnSettings );

    auto createSpec = [&]( const PADSTACK::DRILL_PROPS& aDrill, const wxString& aSpecName ) -> wxString
    {
        if( aDrill.start == UNDEFINED_LAYER || aDrill.end == UNDEFINED_LAYER )
            return wxString();

        auto startLayer = m_layer_name_map.find( aDrill.start );
        auto endLayer = m_layer_name_map.find( aDrill.end );

        if( startLayer == m_layer_name_map.end() || endLayer == m_layer_name_map.end() )
            return wxString();

        wxXmlNode* specNode = appendNode( m_cad_header_node, "Spec" );
        addAttribute( specNode,  "name", aSpecName );

        wxXmlNode* backdrillNode = appendNode( specNode, "Backdrill" );
        addAttribute( backdrillNode,  "startLayerRef", startLayer->second );
        addAttribute( backdrillNode,  "mustNotCutLayerRef", endLayer->second );

        int stubLength = stackup.GetLayerDistance( aDrill.start, aDrill.end );

        if( stubLength < 0 )
            stubLength = 0;

        addAttribute( backdrillNode,  "maxStubLength", floatVal( m_scale * stubLength ) );

        PAD_DRILL_POST_MACHINING_MODE pm_mode = PAD_DRILL_POST_MACHINING_MODE::UNKNOWN;

        if( aDrill.start == F_Cu )
            pm_mode = aPadstack.FrontPostMachining().mode.value_or( PAD_DRILL_POST_MACHINING_MODE::UNKNOWN );
        else if( aDrill.start == B_Cu )
            pm_mode = aPadstack.BackPostMachining().mode.value_or( PAD_DRILL_POST_MACHINING_MODE::UNKNOWN );

        bool isPostMachined = ( pm_mode == PAD_DRILL_POST_MACHINING_MODE::COUNTERBORE ||
                                pm_mode == PAD_DRILL_POST_MACHINING_MODE::COUNTERSINK );

        addAttribute( backdrillNode,  "postMachining", isPostMachined ? wxT( "true" )
                                                                      : wxT( "false" ) );

        m_backdrill_spec_nodes[aSpecName] = specNode;

        return aSpecName;
    };

    int specIndex = m_backdrill_spec_index + 1;

    const PADSTACK::DRILL_PROPS& primary = aPadstack.Drill();
    wxString primarySpec = createSpec( primary, wxString::Format( wxT( "BD_%dA" ), specIndex ) );

    wxString secondarySpec = createSpec( secondary, wxString::Format( wxT( "BD_%dB" ), specIndex ) );

    if( primarySpec.IsEmpty() && secondarySpec.IsEmpty() )
        return;

    m_backdrill_spec_index = specIndex;
    m_padstack_backdrill_specs.emplace( aPadstackName, std::make_pair( primarySpec, secondarySpec ) );
}


void PCB_IO_IPC2581::addBackdrillSpecRefs( wxXmlNode* aHoleNode, const wxString& aPadstackName )
{
    auto it = m_padstack_backdrill_specs.find( aPadstackName );

    if( it == m_padstack_backdrill_specs.end() )
        return;

    auto addRef = [&]( const wxString& aSpecName )
    {
        if( aSpecName.IsEmpty() )
            return;

        wxXmlNode* specRefNode = appendNode( aHoleNode, "SpecRef" );
        addAttribute( specRefNode,  "id", aSpecName );
        m_backdrill_spec_used.insert( aSpecName );
    };

    addRef( it->second.first );
    addRef( it->second.second );
}


void PCB_IO_IPC2581::pruneUnusedBackdrillSpecs()
{
    if( !m_cad_header_node )
        return;

    auto it = m_backdrill_spec_nodes.begin();

    while( it != m_backdrill_spec_nodes.end() )
    {
        if( m_backdrill_spec_used.find( it->first ) == m_backdrill_spec_used.end() )
        {
            wxXmlNode* specNode = it->second;

            if( specNode )
            {
                m_cad_header_node->RemoveChild( specNode );
                delete specNode;
            }

            it = m_backdrill_spec_nodes.erase( it );
        }
        else
        {
            ++it;
        }
    }
}


bool PCB_IO_IPC2581::addPolygonNode( wxXmlNode* aParentNode,
                                     const SHAPE_LINE_CHAIN& aPolygon, FILL_T aFillType,
                                     int aWidth, LINE_STYLE aDashType )
{
    wxXmlNode* polygonNode = nullptr;

    if( aPolygon.PointCount() < 3 )
        return false;

    auto make_node =
            [&]()
    {
        polygonNode = appendNode( aParentNode, "Polygon" );
        wxXmlNode* polybeginNode = appendNode( polygonNode, "PolyBegin" );

        const std::vector<VECTOR2I>& pts = aPolygon.CPoints();
        addXY( polybeginNode, pts[0] );

        for( size_t ii = 1; ii < pts.size(); ++ii )
        {
            wxXmlNode* polyNode = appendNode( polygonNode, "PolyStepSegment" );
            addXY( polyNode, pts[ii] );
        }

        wxXmlNode* polyendNode = appendNode( polygonNode, "PolyStepSegment" );
        addXY( polyendNode, pts[0] );
    };

    // Allow the case where we don't want line/fill information in the polygon
    if( aFillType == FILL_T::NO_FILL )
    {
        make_node();
        // If we specify a line width, we need to add a LineDescRef node and
        // since this is only valid for a non-filled polygon, we need to create
        // the fillNode as well
        if( aWidth > 0 )
            addLineDesc( polygonNode, aWidth, aDashType, true );
    }
    else
    {
        wxCHECK( aWidth == 0, false );
        make_node();
    }

    addFillDesc( polygonNode, aFillType );

    return true;
}


bool PCB_IO_IPC2581::addPolygonCutouts( wxXmlNode* aParentNode,
                                        const SHAPE_POLY_SET::POLYGON& aPolygon )
{
    for( size_t ii = 1; ii < aPolygon.size(); ++ii )
    {
        wxCHECK2( aPolygon[ii].PointCount() >= 3, continue );

        wxXmlNode* cutoutNode = appendNode( aParentNode, "Cutout" );
        wxXmlNode* polybeginNode = appendNode( cutoutNode, "PolyBegin" );

        const std::vector<VECTOR2I>& hole = aPolygon[ii].CPoints();
        addXY( polybeginNode, hole[0] );

        for( size_t jj = 1; jj < hole.size(); ++jj )
        {
            wxXmlNode* polyNode = appendNode( cutoutNode, "PolyStepSegment" );
            addXY( polyNode, hole[jj] );
        }

        wxXmlNode* polyendNode = appendNode( cutoutNode, "PolyStepSegment" );
        addXY( polyendNode, hole[0] );
    }

    return true;
}


bool PCB_IO_IPC2581::addOutlineNode( wxXmlNode* aParentNode, const SHAPE_POLY_SET& aPolySet,
                                     int aWidth, LINE_STYLE aDashType )
{
    if( aPolySet.OutlineCount() == 0 )
        return false;

    wxXmlNode* outlineNode = appendNode( aParentNode, "Outline" );

    // Outlines can only have one polygon according to the IPC-2581 spec, so
    // if there are more than one, we need to combine them into a single polygon
    const SHAPE_LINE_CHAIN* outline = &aPolySet.Outline( 0 );
    SHAPE_LINE_CHAIN        bbox_outline;
    BOX2I                   bbox = outline->BBox();

    if( aPolySet.OutlineCount() > 1 )
    {
        for( int ii = 1; ii < aPolySet.OutlineCount(); ++ii )
        {
            wxCHECK2( aPolySet.Outline( ii ).PointCount() >= 3, continue );
            bbox.Merge( aPolySet.Outline( ii ).BBox() );
        }

        bbox_outline.Append( bbox.GetLeft(), bbox.GetTop() );
        bbox_outline.Append( bbox.GetRight(), bbox.GetTop() );
        bbox_outline.Append( bbox.GetRight(), bbox.GetBottom() );
        bbox_outline.Append( bbox.GetLeft(), bbox.GetBottom() );
        outline = &bbox_outline;
    }


    if( !addPolygonNode( outlineNode, *outline ) )
        wxLogTrace( traceIpc2581, wxS( "Failed to add polygon to outline" ) );

    if( !outlineNode->GetChildren() )
    {
        aParentNode->RemoveChild( outlineNode );
        delete outlineNode;
        return false;
    }

    addLineDesc( outlineNode, aWidth, aDashType );

    return true;
}


bool PCB_IO_IPC2581::addContourNode( wxXmlNode* aParentNode, const SHAPE_POLY_SET& aPolySet,
                                     int aOutline, FILL_T aFillType, int aWidth, LINE_STYLE aDashType )
{
    if( aPolySet.OutlineCount() < ( aOutline + 1 ) )
        return false;

    wxXmlNode* contourNode = appendNode( aParentNode, "Contour" );

    if( addPolygonNode( contourNode, aPolySet.Outline( aOutline ), aFillType, aWidth, aDashType ) )
    {
        // Do not attempt to add cutouts to shapes that are already hollow
        if( aFillType != FILL_T::NO_FILL )
            addPolygonCutouts( contourNode, aPolySet.Polygon( aOutline ) );
    }
    else
    {
        aParentNode->RemoveChild( contourNode );
        delete contourNode;
        return false;
    }

    return true;
}


void PCB_IO_IPC2581::generateProfile( wxXmlNode* aStepNode )
{
    SHAPE_POLY_SET board_outline;

    if( ! m_board->GetBoardPolygonOutlines( board_outline, false ) )
    {
        Report( _( "Board outline is invalid or missing.  Please run DRC." ), RPT_SEVERITY_ERROR );
        return;
    }

    wxXmlNode* profileNode = appendNode( aStepNode, "Profile" );

    if( !addPolygonNode( profileNode, board_outline.Outline( 0 ) ) )
    {
        wxLogTrace( traceIpc2581, wxS( "Failed to add polygon to profile" ) );
        aStepNode->RemoveChild( profileNode );
        delete profileNode;
    }
}


static bool isOppositeSideSilk( const FOOTPRINT* aFootprint, PCB_LAYER_ID aLayer )
{
    if( !aFootprint )
        return false;

    if( aLayer != F_SilkS && aLayer != B_SilkS )
        return false;

    if( aFootprint->IsFlipped() )
        return aLayer == F_SilkS;

    return aLayer == B_SilkS;
}


wxXmlNode* PCB_IO_IPC2581::addPackage( wxXmlNode* aContentNode, FOOTPRINT* aFp )
{
    std::unique_ptr<FOOTPRINT> fp( static_cast<FOOTPRINT*>( aFp->Clone() ) );
    fp->SetParentGroup( nullptr );
    fp->SetPosition( { 0, 0 } );
    fp->SetOrientation( ANGLE_0 );

    // Track original flipped state before normalization. This is needed to correctly
    // determine OtherSideView content per IPC-2581C. After flipping, layer IDs swap,
    // so for bottom components, B_SilkS/B_Fab after flip is actually the primary view.
    bool wasFlipped = fp->IsFlipped();

    // Normalize package geometry to the unflipped footprint coordinate system.
    if( fp->IsFlipped() )
        fp->Flip( fp->GetPosition(), FLIP_DIRECTION::TOP_BOTTOM );

    size_t hash = hash_fp_item( fp.get(), HASH_POS | REL_COORD );
    wxString name = genString( wxString::Format( "%s_%zu",
                                                 fp->GetFPID().GetLibItemName().wx_str(),
                                                 m_footprint_dict.size() + 1 ) );

    auto [ iter, success ] = m_footprint_dict.emplace( hash, name );
    addAttribute( aContentNode,  "packageRef", iter->second );

    if( !success)
        return nullptr;

    // Package and Component nodes are at the same level, so we need to find the parent
    // which should be the Step node
    wxXmlNode* packageNode = new wxXmlNode( wxXML_ELEMENT_NODE, "Package" );
    wxXmlNode* otherSideViewNode = nullptr; // Only set this if we have elements on the back side

    addAttribute( packageNode,  "name", name );
    addAttribute( packageNode,  "type", "OTHER" ); // TODO: Replace with actual package type once we encode this

    // We don't specially identify pin 1 in our footprints, so we need to guess
    if( fp->FindPadByNumber( "1" ) )
        addAttribute( packageNode,  "pinOne", "1" );
    else if ( fp->FindPadByNumber( "A1" ) )
        addAttribute( packageNode,  "pinOne", "A1" );
    else if ( fp->FindPadByNumber( "A" ) )
        addAttribute( packageNode,  "pinOne", "A" );
    else if ( fp->FindPadByNumber( "a" ) )
        addAttribute( packageNode,  "pinOne", "a" );
    else if ( fp->FindPadByNumber( "a1" ) )
        addAttribute( packageNode,  "pinOne", "a1" );
    else if ( fp->FindPadByNumber( "Anode" ) )
        addAttribute( packageNode,  "pinOne", "Anode" );
    else if ( fp->FindPadByNumber( "ANODE" ) )
        addAttribute( packageNode,  "pinOne", "ANODE" );
    else
        addAttribute( packageNode,  "pinOne", "UNKNOWN" );

    addAttribute( packageNode,  "pinOneOrientation", "OTHER" );

    // After normalization: F_CrtYd is top, B_CrtYd is bottom.
    // For bottom components (wasFlipped), these are swapped from original orientation.
    const SHAPE_POLY_SET& courtyard_primary = wasFlipped ? fp->GetCourtyard( B_CrtYd )
                                                         : fp->GetCourtyard( F_CrtYd );
    const SHAPE_POLY_SET& courtyard_other = wasFlipped ? fp->GetCourtyard( F_CrtYd )
                                                       : fp->GetCourtyard( B_CrtYd );

    if( courtyard_primary.OutlineCount() > 0 )
        addOutlineNode( packageNode, courtyard_primary, courtyard_primary.Outline( 0 ).Width(),
                        LINE_STYLE::SOLID );

    if( courtyard_other.OutlineCount() > 0 )
    {
        if( m_version > 'B' )
        {
            otherSideViewNode = appendNode( packageNode, "OtherSideView" );
            addOutlineNode( otherSideViewNode, courtyard_other, courtyard_other.Outline( 0 ).Width(),
                            LINE_STYLE::SOLID );
        }
    }

    if( !courtyard_primary.OutlineCount() && !courtyard_other.OutlineCount() )
    {
        SHAPE_POLY_SET bbox = fp->GetBoundingHull();
        addOutlineNode( packageNode, bbox );
    }

    wxXmlNode* pickupPointNode = appendNode( packageNode, "PickupPoint" );
    addAttribute( pickupPointNode,  "x", "0.0" );
    addAttribute( pickupPointNode,  "y", "0.0" );

    std::map<PCB_LAYER_ID, std::map<bool, std::vector<BOARD_ITEM*>>> elements;

    for( BOARD_ITEM* item : fp->GraphicalItems() )
    {
        PCB_LAYER_ID layer = item->GetLayer();

        /// IPC2581 only supports the documentation layers for production and post-production
        /// All other layers are ignored
        /// TODO: Decide if we should place the other layers from footprints on the board
        if( layer != F_SilkS && layer != B_SilkS && layer != F_Fab && layer != B_Fab )
            continue;

        if( m_version == 'B' && isOppositeSideSilk( fp.get(), layer ) )
            continue;

        bool is_abs = true;

        if( item->Type() == PCB_SHAPE_T )
        {
            PCB_SHAPE* shape = static_cast<PCB_SHAPE*>( item );

            // Circles and Rectanges only have size information so we need to place them in
            // a separate node that has a location
            if( shape->GetShape() == SHAPE_T::CIRCLE || shape->GetShape() == SHAPE_T::RECTANGLE )
                is_abs = false;
        }

        elements[item->GetLayer()][is_abs].push_back( item );
    }

    auto add_base_node =
            [&]( PCB_LAYER_ID aLayer ) -> wxXmlNode*
            {
                wxXmlNode* parent = packageNode;

                // Determine if this layer content should go in OtherSideView.
                // Per IPC-2581C, OtherSideView contains geometry visible from the opposite
                // side of the package body from the primary view.
                //
                // For non-flipped (top) components: B_SilkS/B_Fab → OtherSideView
                // For flipped (bottom) components after normalization: F_SilkS/F_Fab → OtherSideView
                //   (because after flip, B_SilkS/B_Fab contains the original primary graphics)
                bool is_other_side = wasFlipped ? ( aLayer == F_SilkS || aLayer == F_Fab )
                                                : ( aLayer == B_SilkS || aLayer == B_Fab );

                if( is_other_side && m_version > 'B' )
                {
                    if( !otherSideViewNode )
                        otherSideViewNode = new wxXmlNode( wxXML_ELEMENT_NODE, "OtherSideView" );

                    parent = otherSideViewNode;
                }

                wxString name;

                if( aLayer == F_SilkS || aLayer == B_SilkS )
                    name = "SilkScreen";
                else if( aLayer == F_Fab || aLayer == B_Fab )
                    name = "AssemblyDrawing";
                else
                    wxASSERT( false );

                wxXmlNode* new_node = appendNode( parent, name );
                return new_node;
            };

    auto add_marking_node =
            [&]( wxXmlNode* aNode ) -> wxXmlNode*
            {
                wxXmlNode* marking_node = appendNode( aNode, "Marking" );
                addAttribute( marking_node,  "markingUsage", "NONE" );
                return marking_node;
            };

    std::map<PCB_LAYER_ID, wxXmlNode*> layer_nodes;
    std::map<PCB_LAYER_ID, BOX2I> layer_bbox;

    for( auto layer : { F_Fab, B_Fab } )
    {
        if( elements.find( layer ) != elements.end() )
        {
            if( elements[layer][true].size() > 0 )
                layer_bbox[layer] = elements[layer][true][0]->GetBoundingBox();
            else if( elements[layer][false].size() > 0 )
                layer_bbox[layer] = elements[layer][false][0]->GetBoundingBox();
        }
    }

    for( auto& [layer, map] : elements )
    {
        wxXmlNode* layer_node = add_base_node( layer );
        wxXmlNode* marking_node = add_marking_node( layer_node );
        wxXmlNode* group_node = appendNode( marking_node, "UserSpecial" );
        bool update_bbox = false;

        if( layer == F_Fab || layer == B_Fab )
        {
            layer_nodes[layer] = layer_node;
            update_bbox = true;
        }

        for( auto& [is_abs, vec] : map )
        {
            for( BOARD_ITEM* item : vec )
            {
                wxXmlNode* output_node = nullptr;

                if( update_bbox )
                    layer_bbox[layer].Merge( item->GetBoundingBox() );

                if( !is_abs )
                    output_node = add_marking_node( layer_node );
                else
                    output_node = group_node;

                switch( item->Type() )
                {
                case PCB_TEXT_T:
                {
                    PCB_TEXT* text = static_cast<PCB_TEXT*>( item );

                    if( text->IsKnockout() )
                        addKnockoutText( output_node, text );
                    else
                        addText( output_node, text, text->GetFontMetrics() );

                    break;
                }

                case PCB_TEXTBOX_T:
                {
                    PCB_TEXTBOX* text = static_cast<PCB_TEXTBOX*>( item );
                    addText( output_node, text, text->GetFontMetrics() );

                    // We want to force this to be a polygon to get absolute coordinates
                    if( text->IsBorderEnabled() )
                    {
                        SHAPE_POLY_SET poly_set;
                        text->GetEffectiveShape()->TransformToPolygon( poly_set, 0, ERROR_INSIDE );
                        addContourNode( output_node, poly_set, 0, FILL_T::NO_FILL,
                                        text->GetBorderWidth() );
                    }

                    break;
                }

                case PCB_SHAPE_T:
                {
                    if( !is_abs )
                        addLocationNode( output_node, *static_cast<PCB_SHAPE*>( item ) );

                    // When in Marking context (!is_abs), use inline geometry to avoid
                    // unresolved UserPrimitiveRef errors in validators like Vu2581
                    addShape( output_node, *static_cast<PCB_SHAPE*>( item ), !is_abs );

                    break;
                }

                default: break;
                }
            }
        }

        if( group_node->GetChildren() == nullptr )
        {
            marking_node->RemoveChild( group_node );
            layer_node->RemoveChild( marking_node );
            delete group_node;
            delete marking_node;
        }
    }

    for( auto&[layer, bbox] : layer_bbox )
    {
        if( bbox.GetWidth() > 0 )
        {
            wxXmlNode* outlineNode = insertNode( layer_nodes[layer], "Outline" );

            SHAPE_LINE_CHAIN outline;
            std::vector<VECTOR2I> points( 4 );
            points[0] = bbox.GetPosition();
            points[2] = bbox.GetEnd();
            points[1].x = points[0].x;
            points[1].y = points[2].y;
            points[3].x = points[2].x;
            points[3].y = points[0].y;

            outline.Append( points );
            addPolygonNode( outlineNode, outline, FILL_T::NO_FILL, 0 );
            addLineDesc( outlineNode, 0, LINE_STYLE::SOLID );
        }
    }

    std::map<wxString, wxXmlNode*> pin_nodes;

    for( size_t ii = 0; ii < fp->Pads().size(); ++ii )
    {
        PAD* pad = fp->Pads()[ii];
        wxString name = pinName( pad );
        wxXmlNode* pinNode = nullptr;

        auto [ it, inserted ] = pin_nodes.emplace( name, nullptr );

        if( inserted )
        {
            pinNode = appendNode( packageNode, "Pin" );
            it->second = pinNode;

            addAttribute( pinNode,  "number", name );

            m_net_pin_dict[pad->GetNetCode()].emplace_back(
                    genString( fp->GetReference(), "CMP" ), name );

            if( pad->GetAttribute() == PAD_ATTRIB::NPTH )
                addAttribute( pinNode,  "electricalType", "MECHANICAL" );
            else if( pad->IsOnCopperLayer() )
                addAttribute( pinNode,  "electricalType", "ELECTRICAL" );
            else
                addAttribute( pinNode,  "electricalType", "UNDEFINED" );

            if( pad->HasHole() )
                addAttribute( pinNode,  "type", "THRU" );
            else
                addAttribute( pinNode,  "type", "SURFACE" );

            if( pad->GetFPRelativeOrientation() != ANGLE_0 )//|| fp->IsFlipped() )
            {
                wxXmlNode* xformNode = appendNode( pinNode, "Xform" );
                EDA_ANGLE pad_angle = pad->GetFPRelativeOrientation().Normalize();

                if( fp->IsFlipped() )
                    pad_angle = pad_angle.Invert().Normalize();

                if( pad_angle != ANGLE_0 )
                    xformNode->AddAttribute( "rotation", floatVal( pad_angle.AsDegrees() ) );
            }
        }
        else
        {
            pinNode = it->second;
        }

        addLocationNode( pinNode, *pad, true );
        addShape( pinNode, *pad, pad->GetLayer() );

        // We just need the padstack, we don't need the reference here.  The reference will be
        // created in the LayerFeature set
        wxXmlNode dummy;
        addPadStack( &dummy, pad );
    }

    return packageNode;
}


void PCB_IO_IPC2581::generateComponents( wxXmlNode* aStepNode )
{
    std::vector<wxXmlNode*> componentNodes;
    std::vector<wxXmlNode*> packageNodes;
    std::set<wxString> packageNames;

    bool generate_unique = m_OEMRef.empty();

    for( FOOTPRINT* fp : m_board->Footprints() )
    {
        wxXmlNode* componentNode = new wxXmlNode( wxXML_ELEMENT_NODE, "Component" );
        addAttribute( componentNode,  "refDes", componentName( fp ) );
        wxXmlNode* pkg = addPackage( componentNode, fp );

        if( pkg )
            packageNodes.push_back( pkg );

        wxString name;

        PCB_FIELD* field = nullptr;

        if( !generate_unique )
            field = fp->GetField( m_OEMRef );

        if( field && !field->GetText().empty() )
        {
            name = field->GetShownText( false );
        }
        else
        {
            name = wxString::Format( "%s_%s_%s", fp->GetFPID().GetFullLibraryName(),
                                     fp->GetFPID().GetLibItemName().wx_str(),
                                     fp->GetValue() );
        }

        if( !m_OEMRef_dict.emplace( fp, name ).second )
            Report( _( "Duplicate footprint pointers encountered; IPC-2581 output may be incorrect." ),
                    RPT_SEVERITY_ERROR );

        addAttribute( componentNode,  "part", genString( name, "REF" ) );
        addAttribute( componentNode,  "layerRef", m_layer_name_map[fp->GetLayer()] );

        if( fp->GetAttributes() & FP_THROUGH_HOLE )
            addAttribute( componentNode,  "mountType", "THMT" );
        else if( fp->GetAttributes() & FP_SMD )
            addAttribute( componentNode,  "mountType", "SMT" );
        else
            addAttribute( componentNode,  "mountType", "OTHER" );

        if( fp->GetOrientation() != ANGLE_0 || fp->IsFlipped() )
        {
            wxXmlNode* xformNode = appendNode( componentNode, "Xform" );

            EDA_ANGLE fp_angle = fp->GetOrientation().Normalize();

            if( fp->IsFlipped() )
                fp_angle = fp_angle.Invert().Normalize();

            if( fp_angle != ANGLE_0 )
                addAttribute( xformNode, "rotation", floatVal( fp_angle.AsDegrees(), 2 ) );

            if( fp->IsFlipped() )
                addAttribute( xformNode,  "mirror", "true" );
        }

        addLocationNode( componentNode, fp->GetPosition().x, fp->GetPosition().y );

        componentNodes.push_back( componentNode );
    }

    for( wxXmlNode* padstack : m_padstacks )
    {
        insertNode( aStepNode, padstack );
        m_last_padstack = padstack;
    }

    for( wxXmlNode* pkg : packageNodes )
        aStepNode->AddChild( pkg );

    for( wxXmlNode* cmp : componentNodes )
        aStepNode->AddChild( cmp );
}


void PCB_IO_IPC2581::generateLogicalNets( wxXmlNode* aStepNode )
{
    for( auto& [ net, pin_pair] : m_net_pin_dict )
    {
        wxXmlNode* netNode = appendNode( aStepNode, "LogicalNet" );
        addAttribute( netNode,  "name",
                      genString( m_board->GetNetInfo().GetNetItem( net )->GetNetname(), "NET" ) ) ;

        for( auto& [cmp, pin] : pin_pair )
        {
            wxXmlNode* netPinNode = appendNode( netNode, "PinRef" );
            addAttribute( netPinNode,  "componentRef", cmp );
            addAttribute( netPinNode,  "pin", pin );
        }
        //TODO: Finish
    }
}

//TODO: Add PhyNetGroup section

void PCB_IO_IPC2581::generateLayerFeatures( wxXmlNode* aStepNode )
{
    LSEQ layers = m_board->GetEnabledLayers().Seq();
    const NETINFO_LIST& nets = m_board->GetNetInfo();
    std::vector<std::unique_ptr<FOOTPRINT>> footprints;

    // To avoid the overhead of repeatedly cycling through the layers and nets,
    // we pre-sort the board items into a map of layer -> net -> items
    std::map<PCB_LAYER_ID, std::map<int, std::vector<BOARD_ITEM*>>> elements;

    std::for_each( m_board->Tracks().begin(), m_board->Tracks().end(),
            [&layers, &elements]( PCB_TRACK* aTrack )
            {
                if( aTrack->Type() == PCB_VIA_T )
                {
                    PCB_VIA* via = static_cast<PCB_VIA*>( aTrack );

                    for( PCB_LAYER_ID layer : layers )
                    {
                        if( via->FlashLayer( layer ) )
                            elements[layer][via->GetNetCode()].push_back( via );
                    }
                }
                else
                {
                    elements[aTrack->GetLayer()][aTrack->GetNetCode()].push_back( aTrack );
                }
            } );

    std::for_each( m_board->Zones().begin(), m_board->Zones().end(),
            [ &elements ]( ZONE* zone )
            {
                LSEQ zone_layers = zone->GetLayerSet().Seq();

                for( PCB_LAYER_ID layer : zone_layers )
                    elements[layer][zone->GetNetCode()].push_back( zone );
            } );

    for( BOARD_ITEM* item : m_board->Drawings() )
    {
        if( BOARD_CONNECTED_ITEM* conn_it = dynamic_cast<BOARD_CONNECTED_ITEM*>( item ) )
            elements[conn_it->GetLayer()][conn_it->GetNetCode()].push_back( conn_it );
        else
            elements[item->GetLayer()][0].push_back( item );
    }

    for( FOOTPRINT* fp : m_board->Footprints() )
    {
        for( PCB_FIELD* field : fp->GetFields() )
            elements[field->GetLayer()][0].push_back( field );

        for( BOARD_ITEM* item : fp->GraphicalItems() )
            elements[item->GetLayer()][0].push_back( item );

        for( PAD* pad : fp->Pads() )
        {
            LSEQ pad_layers = pad->GetLayerSet().Seq();

            for( PCB_LAYER_ID layer : pad_layers )
            {
                if( pad->FlashLayer( layer ) )
                    elements[layer][pad->GetNetCode()].push_back( pad );
            }
        }
    }

    for( PCB_LAYER_ID layer : layers )
    {
        if( m_progressReporter )
            m_progressReporter->SetMaxProgress( nets.GetNetCount() * layers.size() );

        wxXmlNode* layerNode = appendNode( aStepNode, "LayerFeature" );
        addAttribute( layerNode,  "layerRef", m_layer_name_map[layer] );

        auto process_net = [&] ( int net )
        {
            std::vector<BOARD_ITEM*>& vec = elements[layer][net];

            if( vec.empty() )
                return;

            std::stable_sort( vec.begin(), vec.end(),
                       []( BOARD_ITEM* a, BOARD_ITEM* b )
                       {
                            if( a->GetParentFootprint() == b->GetParentFootprint() )
                                return a->Type() < b->Type();

                            return a->GetParentFootprint() < b->GetParentFootprint();
                       } );

            generateLayerSetNet( layerNode, layer, vec );
        };

        for( const NETINFO_ITEM* net : nets )
        {
            if( m_progressReporter )
            {
                m_progressReporter->Report( wxString::Format( _( "Exporting Layer %s, Net %s" ),
                                                               m_board->GetLayerName( layer ),
                                                               net->GetNetname() ) );
                m_progressReporter->AdvanceProgress();
            }

            process_net( net->GetNetCode() );
        }

        if( layerNode->GetChildren() == nullptr )
        {
            aStepNode->RemoveChild( layerNode );
            delete layerNode;
        }
    }
}


void PCB_IO_IPC2581::generateLayerSetDrill( wxXmlNode* aLayerNode )
{
    int hole_count = 1;

    for( const auto& [layers, vec] : m_drill_layers )
    {
        wxXmlNode* layerNode = appendNode( aLayerNode, "LayerFeature" );
        layerNode->AddAttribute( "layerRef", genLayersString( layers.first, layers.second, "DRILL" ) );

        for( BOARD_ITEM* item : vec )
        {
            if( item->Type() == PCB_VIA_T )
            {
                PCB_VIA* via = static_cast<PCB_VIA*>( item );
                auto it = m_padstack_dict.find( hash_fp_item( via, 0 ) );

                if( it == m_padstack_dict.end() )
                {
                    Report( _( "Via uses unsupported padstack; omitted from drill data." ),
                            RPT_SEVERITY_WARNING );
                    continue;
                }

                wxXmlNode* padNode = appendNode( layerNode, "Set" );
                addAttribute( padNode,  "geometry", it->second );

                if( via->GetNetCode() > 0 )
                    addAttribute( padNode,  "net", genString( via->GetNetname(), "NET" ) );

                wxXmlNode* holeNode = appendNode( padNode, "Hole" );
                addAttribute( holeNode,  "name", wxString::Format( "H%d", hole_count++ ) );
                addAttribute( holeNode,  "diameter", floatVal( m_scale * via->GetDrillValue() ) );
                addAttribute( holeNode,  "platingStatus", "VIA" );
                addAttribute( holeNode,  "plusTol", "0.0" );
                addAttribute( holeNode,  "minusTol", "0.0" );
                addXY( holeNode, via->GetPosition() );
                addBackdrillSpecRefs( holeNode, it->second );
            }
            else if( item->Type() == PCB_PAD_T )
            {
                PAD* pad = static_cast<PAD*>( item );
                auto it = m_padstack_dict.find( hash_fp_item( pad, 0 ) );

                if( it == m_padstack_dict.end() )
                {
                    Report( _( "Pad uses unsupported padstack; hole was omitted from drill data." ),
                            RPT_SEVERITY_WARNING );
                    continue;
                }

                wxXmlNode* padNode = appendNode( layerNode, "Set" );
                addAttribute( padNode,  "geometry", it->second );

                if( pad->GetNetCode() > 0 )
                    addAttribute( padNode,  "net", genString( pad->GetNetname(), "NET" ) );

                wxXmlNode* holeNode = appendNode( padNode, "Hole" );
                addAttribute( holeNode,  "name", wxString::Format( "H%d", hole_count++ ) );
                addAttribute( holeNode,  "diameter", floatVal( m_scale * pad->GetDrillSizeX() ) );
                addAttribute( holeNode,  "platingStatus",
                              pad->GetAttribute() == PAD_ATTRIB::PTH ? "PLATED" : "NONPLATED" );
                addAttribute( holeNode,  "plusTol", "0.0" );
                addAttribute( holeNode,  "minusTol", "0.0" );
                addXY( holeNode, pad->GetPosition() );
                addBackdrillSpecRefs( holeNode, it->second );
            }
        }
    }

    hole_count = 1;

    for( const auto& [layers, vec] : m_slot_holes )
    {
        wxXmlNode* layerNode = appendNode( aLayerNode, "LayerFeature" );
        layerNode->AddAttribute( "layerRef", genLayersString( layers.first, layers.second, "SLOT" ) );

        for( PAD* pad : vec )
        {
            wxXmlNode* padNode = appendNode( layerNode, "Set" );

            if( pad->GetNetCode() > 0 )
                addAttribute( padNode,  "net", genString( pad->GetNetname(), "NET" ) );

            addSlotCavity( padNode, *pad, wxString::Format( "SLOT%d", hole_count++ ) );
        }
    }
}


void PCB_IO_IPC2581::generateLayerSetNet( wxXmlNode* aLayerNode, PCB_LAYER_ID aLayer,
                                          std::vector<BOARD_ITEM*>& aItems )
{
    auto it = aItems.begin();
    wxXmlNode* layerSetNode = appendNode( aLayerNode, "Set" );
    wxXmlNode* featureSetNode = appendNode( layerSetNode, "Features" );
    wxXmlNode* specialNode = appendNode( featureSetNode, "UserSpecial" );

    bool has_via = false;
    bool has_pad = false;

    wxXmlNode* padSetNode = nullptr;

    wxXmlNode* viaSetNode = nullptr;

    wxXmlNode* teardropLayerSetNode = nullptr;
    wxXmlNode* teardropFeatureSetNode = nullptr;

    bool teardrop_warning = false;

    if( BOARD_CONNECTED_ITEM* item = dynamic_cast<BOARD_CONNECTED_ITEM*>( *it );
        IsCopperLayer( aLayer ) && item )
    {
        if( item->GetNetCode() > 0 )
            addAttribute( layerSetNode,  "net", genString( item->GetNetname(), "NET" ) );
    }

    auto add_track =
            [&]( PCB_TRACK* track )
            {
                if( track->Type() == PCB_TRACE_T )
                {
                    PCB_SHAPE shape( nullptr, SHAPE_T::SEGMENT );
                    shape.SetStart( track->GetStart() );
                    shape.SetEnd( track->GetEnd() );
                    shape.SetWidth( track->GetWidth() );
                    addShape( specialNode, shape );
                }
                else if( track->Type() == PCB_ARC_T )
                {
                    PCB_ARC* arc = static_cast<PCB_ARC*>( track );
                    PCB_SHAPE shape( nullptr, SHAPE_T::ARC );
                    shape.SetArcGeometry( arc->GetStart(), arc->GetMid(), arc->GetEnd() );
                    shape.SetWidth( arc->GetWidth() );
                    addShape( specialNode, shape );
                }
                else
                {
                    if( !viaSetNode )
                    {
                        if( !has_pad )
                        {
                            viaSetNode = layerSetNode;
                            has_via = true;
                        }
                        else
                        {
                            viaSetNode = appendNode( layerSetNode, "Set" );

                            if( track->GetNetCode() > 0 )
                                addAttribute( viaSetNode,  "net", genString( track->GetNetname(), "NET" ) );
                        }

                        addAttribute( viaSetNode,  "padUsage", "VIA" );
                    }

                    addVia( viaSetNode, static_cast<PCB_VIA*>( track ), aLayer );
                }
            };

    auto add_zone =
            [&]( ZONE* zone )
            {
                wxXmlNode* zoneFeatureNode = specialNode;

                if( zone->IsTeardropArea() )
                {
                    if( m_version > 'B' )
                    {
                        if( !teardropFeatureSetNode )
                        {
                            teardropLayerSetNode = appendNode( aLayerNode, "Set" );
                            addAttribute( teardropLayerSetNode,  "geometryUsage", "TEARDROP" );

                            if( zone->GetNetCode() > 0 )
                            {
                                addAttribute( teardropLayerSetNode,  "net",
                                              genString( zone->GetNetname(), "NET" ) );
                            }

                            wxXmlNode* new_teardrops = appendNode( teardropLayerSetNode, "Features" );
                            addLocationNode( new_teardrops, 0.0, 0.0 );
                            teardropFeatureSetNode = appendNode( new_teardrops, "UserSpecial" );
                        }

                        zoneFeatureNode = teardropFeatureSetNode;
                    }
                    else if( !teardrop_warning )
                    {
                        Report( _( "Teardrops are not supported in IPC-2581 revision B; they were exported as zones." ),
                                RPT_SEVERITY_WARNING );
                        teardrop_warning = true;
                    }
                }
                else
                {
                    if( FOOTPRINT* fp = zone->GetParentFootprint() )
                    {
                        wxXmlNode* tempSetNode = appendNode( aLayerNode, "Set" );
                        wxString refDes = componentName( zone->GetParentFootprint() );
                        addAttribute( tempSetNode,  "componentRef", refDes );
                        wxXmlNode* newFeatures = appendNode( tempSetNode, "Features" );
                        addLocationNode( newFeatures, 0.0, 0.0 );
                        zoneFeatureNode = appendNode( newFeatures, "UserSpecial" );
                    }
                }

                SHAPE_POLY_SET& zone_shape = *zone->GetFilledPolysList( aLayer );

                for( int ii = 0; ii < zone_shape.OutlineCount(); ++ii )
                    addContourNode( zoneFeatureNode, zone_shape, ii );
            };

    auto add_shape =
            [&] ( PCB_SHAPE* shape )
            {
                FOOTPRINT* fp = shape->GetParentFootprint();

                if( fp )
                {
                    wxXmlNode* tempSetNode = appendNode( aLayerNode, "Set" );

                    if( m_version > 'B' )
                        addAttribute( tempSetNode,  "geometryUsage", "GRAPHIC" );

                    bool link_to_component = true;

                    if( m_version == 'B' && isOppositeSideSilk( fp, shape->GetLayer() ) )
                        link_to_component = false;

                    if( link_to_component )
                        addAttribute( tempSetNode,  "componentRef", componentName( fp ) );

                    wxXmlNode* tempFeature = appendNode( tempSetNode, "Features" );

                    // Per IPC-2581 schema, element order in Features must be: Xform, Location, Feature
                    EDA_ANGLE fp_angle = fp->GetOrientation().Normalize();

                    if( fp_angle != ANGLE_0 )
                    {
                        wxXmlNode* xformNode = appendNode( tempFeature, "Xform" );
                        addAttribute( xformNode, "rotation", floatVal( fp_angle.AsDegrees(), 2 ) );
                    }

                    addLocationNode( tempFeature, *shape );
                    addShape( tempFeature, *shape );
                }
                else if( shape->GetShape() == SHAPE_T::CIRCLE
                        || shape->GetShape() == SHAPE_T::RECTANGLE
                        || shape->GetShape() == SHAPE_T::POLY )
                {
                    wxXmlNode* tempSetNode = appendNode( aLayerNode, "Set" );

                    if( shape->GetNetCode() > 0 )
                        addAttribute( tempSetNode,  "net", genString( shape->GetNetname(), "NET" ) );

                    wxXmlNode* tempFeature = appendNode( tempSetNode, "Features" );
                    addLocationNode( tempFeature, *shape );
                    addShape( tempFeature, *shape );
                }
                else
                {
                    addShape( specialNode, *shape );
                }
            };

    auto add_text =
            [&] ( BOARD_ITEM* text )
            {
                EDA_TEXT* text_item;
                FOOTPRINT* fp = text->GetParentFootprint();

                if( PCB_TEXT* tmp_text = dynamic_cast<PCB_TEXT*>( text ) )
                    text_item = static_cast<EDA_TEXT*>( tmp_text );
                else if( PCB_TEXTBOX* tmp_text = dynamic_cast<PCB_TEXTBOX*>( text ) )
                    text_item = static_cast<EDA_TEXT*>( tmp_text );

                if( !text_item->IsVisible() || text_item->GetShownText( false ).empty() )
                    return;

                wxXmlNode* tempSetNode = appendNode( aLayerNode, "Set" );

                if( m_version > 'B' )
                    addAttribute( tempSetNode,  "geometryUsage", "TEXT" );

                bool link_to_component = fp != nullptr;

                if( m_version == 'B' && fp && isOppositeSideSilk( fp, text->GetLayer() ) )
                    link_to_component = false;

                if( link_to_component )
                    addAttribute( tempSetNode, "componentRef", componentName( fp ) );

                wxXmlNode* nonStandardAttributeNode = appendNode( tempSetNode, "NonstandardAttribute" );
                addAttribute( nonStandardAttributeNode,  "name", "TEXT" );
                addAttribute( nonStandardAttributeNode,  "value", text_item->GetShownText( false ) );
                addAttribute( nonStandardAttributeNode,  "type", "STRING" );

                wxXmlNode* tempFeature = appendNode( tempSetNode, "Features" );
                addLocationNode( tempFeature, 0.0, 0.0 );

                if( text->Type() == PCB_TEXT_T && static_cast<PCB_TEXT*>( text )->IsKnockout() )
                    addKnockoutText( tempFeature, static_cast<PCB_TEXT*>( text ) );
                else
                    addText( tempFeature, text_item, text->GetFontMetrics() );

                if( text->Type() == PCB_TEXTBOX_T )
                {
                    PCB_TEXTBOX* textbox = static_cast<PCB_TEXTBOX*>( text );

                    if( textbox->IsBorderEnabled() )
                        addShape( tempFeature, *static_cast<PCB_SHAPE*>( textbox ) );
                }
            };

    auto add_pad =
            [&]( PAD* pad )
            {
                if( !padSetNode )
                {
                    if( !has_via )
                    {
                        padSetNode = layerSetNode;
                        has_pad = true;
                    }
                    else
                    {
                        padSetNode = appendNode( aLayerNode, "Set" );

                        if( pad->GetNetCode() > 0 )
                            addAttribute( padSetNode,  "net", genString( pad->GetNetname(), "NET" ) );
                    }
                }

                FOOTPRINT* fp = pad->GetParentFootprint();

                if( fp && fp->IsFlipped() )
                    addPad( padSetNode, pad, FlipLayer( aLayer ) );
                else
                    addPad( padSetNode, pad, aLayer );
            };

    for( BOARD_ITEM* item : aItems )
    {
        switch( item->Type() )
        {
        case PCB_TRACE_T:
        case PCB_ARC_T:
        case PCB_VIA_T:
            add_track( static_cast<PCB_TRACK*>( item ) );
            break;

        case PCB_ZONE_T:
            add_zone( static_cast<ZONE*>( item ) );
            break;

        case PCB_PAD_T:
            add_pad( static_cast<PAD*>( item ) );
            break;

        case PCB_SHAPE_T:
            add_shape( static_cast<PCB_SHAPE*>( item ) );
            break;

        case PCB_TEXT_T:
        case PCB_TEXTBOX_T:
        case PCB_FIELD_T:
            add_text( item );
            break;

        case PCB_DIMENSION_T:
        case PCB_TARGET_T:
        case PCB_DIM_ALIGNED_T:
        case PCB_DIM_LEADER_T:
        case PCB_DIM_CENTER_T:
        case PCB_DIM_RADIAL_T:
        case PCB_DIM_ORTHOGONAL_T:
            //TODO: Add support for dimensions
            break;

        default:
            wxLogTrace( traceIpc2581, wxS( "Unhandled type %s" ),
                        ENUM_MAP<KICAD_T>::Instance().ToString( item->Type() ) );
        }
    }

    if( specialNode->GetChildren() == nullptr )
    {
        featureSetNode->RemoveChild( specialNode );
        delete specialNode;
    }

    if( featureSetNode->GetChildren() == nullptr )
    {
        layerSetNode->RemoveChild( featureSetNode );
        delete featureSetNode;
    }

    if( layerSetNode->GetChildren() == nullptr )
    {
        aLayerNode->RemoveChild( layerSetNode );
        delete layerSetNode;
    }
}

void PCB_IO_IPC2581::generateLayerSetAuxilliary( wxXmlNode* aStepNode )
{
    int hole_count = 1;

    for( const auto& [layers, vec] : m_auxilliary_Layers )
    {
        hole_count = 1;
        bool add_node = true;

        wxString name;
        bool     hole = false;

        // clang-format off: suggestion is inconsitent
        switch( std::get<0>(layers) )
        {
        case auxLayerType::COVERING:
            name = "COVERING";
            break;
        case auxLayerType::PLUGGING:
            name = "PLUGGING";
            hole = true;
            break;
        case auxLayerType::TENTING:
            name = "TENTING";
            break;
        case auxLayerType::FILLING:
            name = "FILLING";
            hole = true;
            break;
        case auxLayerType::CAPPING:
            name = "CAPPING";
            hole = true;
            break;
        default:
            add_node = false;
            break;
        }
        // clang-format on: suggestion is inconsitent

        if( !add_node )
            continue;

        wxXmlNode* layerNode = appendNode( aStepNode, "LayerFeature" );
        if( std::get<2>( layers ) == UNDEFINED_LAYER )
            layerNode->AddAttribute( "layerRef", genLayerString( std::get<1>( layers ), TO_UTF8( name ) ) );
        else
            layerNode->AddAttribute( "layerRef", genLayersString( std::get<1>( layers ),
                                                                  std::get<2>( layers ), TO_UTF8( name ) ) );

        for( BOARD_ITEM* item : vec )
        {
            if( item->Type() == PCB_VIA_T )
            {
                PCB_VIA* via = static_cast<PCB_VIA*>( item );

                PCB_SHAPE shape( nullptr, SHAPE_T::CIRCLE );

                if( hole )
                    shape.SetEnd( { KiROUND( via->GetDrillValue() / 2.0 ), 0 } );
                else
                    shape.SetEnd( { KiROUND( via->GetWidth( std::get<1>( layers ) ) / 2.0 ), 0 } );

                wxXmlNode* padNode = appendNode( layerNode, "Pad" );
                addPadStack( padNode, via );

                addLocationNode( padNode, 0.0, 0.0 );
                addShape( padNode, shape );
            }
        }
    }
}


wxXmlNode* PCB_IO_IPC2581::generateAvlSection()
{
    if( m_progressReporter )
        m_progressReporter->AdvancePhase( _( "Generating BOM section" ) );

    // Per IPC-2581 schema, Avl requires at least one AvlItem child element.
    // Don't emit Avl section if there are no items.
    if( m_OEMRef_dict.empty() )
        return nullptr;

    wxXmlNode* avl = appendNode( m_xml_root, "Avl" );
    addAttribute( avl,  "name", "Primary_Vendor_List" );

    wxXmlNode* header = appendNode( avl, "AvlHeader" );
    addAttribute( header,  "title", "BOM" );
    addAttribute( header,  "source", "KiCad" );
    addAttribute( header,  "author", "OWNER" );
    addAttribute( header,  "datetime", wxDateTime::Now().FormatISOCombined() );
    addAttribute( header,  "version", "1" );

    std::set<wxString> unique_parts;
    std::map<wxString,wxString> unique_vendors;

    for( auto& [fp, name] : m_OEMRef_dict )
    {
        auto [ it, success ] = unique_parts.insert( name );

        if( !success )
            continue;

        wxXmlNode* part = appendNode( avl, "AvlItem" );
        addAttribute( part,  "OEMDesignNumber", genString( name, "REF" ) );

        PCB_FIELD* nums[2] = { fp->GetField( m_mpn ), fp->GetField( m_distpn ) };
        PCB_FIELD* company[2] = { fp->GetField( m_mfg ), nullptr };
        wxString company_name[2] = { m_mfg, m_dist };

        for ( int ii = 0; ii < 2; ++ii )
        {
            if( nums[ii] )
            {
                wxString mpn_name = nums[ii]->GetShownText( false );

                if( mpn_name.empty() )
                    continue;

                wxXmlNode* vmpn = appendNode( part, "AvlVmpn" );
                addAttribute( vmpn,  "qualified", "false" );
                addAttribute( vmpn,  "chosen", "false" );

                wxXmlNode* mpn = appendNode( vmpn, "AvlMpn" );
                addAttribute( mpn,  "name", mpn_name );

                wxXmlNode* vendor = appendNode( vmpn, "AvlVendor" );

                wxString name = wxT( "UNKNOWN" );

                // If the field resolves, then use that field content unless it is empty
                if( !ii && company[ii] )
                {
                    wxString tmp = company[ii]->GetShownText( false );

                    if( !tmp.empty() )
                        name = tmp;
                }
                // If it doesn't resolve but there is content from the dialog, use the static content
                else if( !ii && !company_name[ii].empty() )
                {
                    name = company_name[ii];
                }
                else if( ii && !m_dist.empty() )
                {
                    name = m_dist;
                }

                auto [vendor_id, inserted] = unique_vendors.emplace(
                        name,
                        wxString::Format( "VENDOR_%zu", unique_vendors.size() ) );

                addAttribute( vendor,  "enterpriseRef", vendor_id->second );

                if( inserted )
                {
                    wxXmlNode* new_vendor = new wxXmlNode( wxXML_ELEMENT_NODE, "Enterprise" );
                    addAttribute( new_vendor,  "id", vendor_id->second );
                    addAttribute( new_vendor,  "name", name );
                    addAttribute( new_vendor,  "code", "NONE" );
                    insertNodeAfter( m_enterpriseNode, new_vendor );
                    m_enterpriseNode = new_vendor;
                }
            }
        }
    }

    return avl;
}


void PCB_IO_IPC2581::SaveBoard( const wxString& aFileName, BOARD* aBoard,
                                const std::map<std::string, UTF8>* aProperties )
{
    // Clean up any previous export state to allow multiple exports per plugin instance
    delete m_xml_doc;
    m_xml_doc = nullptr;
    m_xml_root = nullptr;

    m_board = aBoard;
    m_padstack_backdrill_specs.clear();
    m_backdrill_spec_nodes.clear();
    m_backdrill_spec_used.clear();
    m_backdrill_spec_index = 0;
    m_cad_header_node = nullptr;
    m_layer_name_map.clear();

    // Clear all internal dictionaries and caches
    m_user_shape_dict.clear();
    m_shape_user_node = nullptr;
    m_std_shape_dict.clear();
    m_shape_std_node = nullptr;
    m_line_dict.clear();
    m_line_node = nullptr;
    m_padstack_dict.clear();
    m_padstacks.clear();
    m_last_padstack = nullptr;
    m_footprint_dict.clear();
    m_footprint_refdes_dict.clear();
    m_footprint_refdes_reverse_dict.clear();
    m_OEMRef_dict.clear();
    m_net_pin_dict.clear();
    m_drill_layers.clear();
    m_slot_holes.clear();
    m_auxilliary_Layers.clear();
    m_element_names.clear();
    m_generated_names.clear();
    m_acceptable_chars.clear();
    m_total_bytes = 0;

    m_units_str = "MILLIMETER";
    m_scale = 1.0 / PCB_IU_PER_MM;
    m_sigfig = 6;

    if( auto it = aProperties->find( "units" ); it != aProperties->end() )
    {
        if( it->second == "inch" )
        {
            m_units_str = "INCH";
            m_scale = ( 1.0 / 25.4 ) / PCB_IU_PER_MM;
        }
    }

    if( auto it = aProperties->find( "sigfig" ); it != aProperties->end() )
        m_sigfig = std::stoi( it->second );

    if( auto it = aProperties->find( "version" ); it != aProperties->end() )
        m_version = it->second.c_str()[0];

    if( auto it = aProperties->find( "OEMRef" ); it != aProperties->end() )
        m_OEMRef = it->second.wx_str();

    if( auto it = aProperties->find( "mpn" ); it != aProperties->end() )
        m_mpn = it->second.wx_str();

    if( auto it = aProperties->find( "mfg" ); it != aProperties->end() )
        m_mfg = it->second.wx_str();

    if( auto it = aProperties->find( "dist" ); it != aProperties->end() )
        m_dist = it->second.wx_str();

    if( auto it = aProperties->find( "distpn" ); it != aProperties->end() )
        m_distpn = it->second.wx_str();

    if( m_version == 'B' )
    {
        for( char c = 'a'; c <= 'z'; ++c )
            m_acceptable_chars.insert( c );

        for( char c = 'A'; c <= 'Z'; ++c )
            m_acceptable_chars.insert( c );

        for( char c = '0'; c <= '9'; ++c )
            m_acceptable_chars.insert( c );

        // Add special characters
        std::string specialChars = "_\\-.+><";

        for( char c : specialChars )
            m_acceptable_chars.insert( c );
    }

    m_xml_doc = new wxXmlDocument();
    m_xml_root = generateXmlHeader();

    generateContentSection();

    if( m_progressReporter )
    {
        m_progressReporter->SetNumPhases( 7 );
        m_progressReporter->BeginPhase( 1 );
        m_progressReporter->Report( _( "Generating logistic section" ) );
    }

    generateLogisticSection();
    generateHistorySection();

    wxXmlNode* ecad_node = generateEcadSection();
    generateBOMSection( ecad_node );
    generateAvlSection();

    if( m_progressReporter )
    {
        m_progressReporter->AdvancePhase( _( "Saving file" ) );
    }

    wxFileOutputStreamWithProgress out_stream( aFileName );
    double written_bytes = 0.0;
    double last_yield = 0.0;

    // This is a rough estimation of the size of the spaces in the file
    // We just need to total to be slightly larger than the value of the
    // progress bar, so accurately counting spaces is not terribly important
    m_total_bytes += m_total_bytes / 10;

    auto update_progress = [&]( size_t aBytes )
    {
        written_bytes += aBytes;
        double percent = written_bytes / static_cast<double>( m_total_bytes );

        if( m_progressReporter )
        {
            // Only update every percent
            if( last_yield + 0.01 < percent )
            {
                last_yield = percent;
                m_progressReporter->SetCurrentProgress( percent );
            }
        }
    };

    out_stream.SetProgressCallback( update_progress );

    if( !m_xml_doc->Save( out_stream ) )
    {
        Report( _( "Failed to save IPC-2581 data to buffer." ), RPT_SEVERITY_ERROR );
        return;
    }

    size_t size = out_stream.GetSize();
}
