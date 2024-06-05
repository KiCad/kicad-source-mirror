/**
* This program source code file is part of KiCad, a free EDA CAD application.
*
* Copyright (C) 2023 KiCad Developers, see AUTHORS.txt for contributors.
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
#include <footprint.h>
#include <hash_eda.h>
#include <pad.h>
#include <pcb_dimension.h>
#include <pcb_shape.h>
#include <pcb_text.h>
#include <pcb_textbox.h>
#include <pcb_track.h>
#include <pcbnew_settings.h>
#include <pgm_base.h>
#include <progress_reporter.h>
#include <settings/settings_manager.h>
#include <string_utf8_map.h>
#include <wx_fstream_progress.h>

#include <geometry/shape_circle.h>
#include <geometry/shape_line_chain.h>
#include <geometry/shape_poly_set.h>
#include <geometry/shape_segment.h>

#include <wx/log.h>
#include <wx/numformatter.h>
#include <wx/mstream.h>
#include <wx/xml/xml.h>

PCB_IO_IPC2581::~PCB_IO_IPC2581()
{
    clearLoadedFootprints();
}


void PCB_IO_IPC2581::clearLoadedFootprints()
{
    for( FOOTPRINT* fp : m_loaded_footprints )
    {
        delete fp;
    }

    m_loaded_footprints.clear();
}


std::vector<FOOTPRINT*> PCB_IO_IPC2581::GetImportedCachedLibraryFootprints()
{
    std::vector<FOOTPRINT*> retval;

    for( FOOTPRINT* fp : m_loaded_footprints )
    {
        retval.push_back( static_cast<FOOTPRINT*>( fp->Clone() ) );
    }

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
}


void PCB_IO_IPC2581::insertNodeAfter( wxXmlNode* aPrev, wxXmlNode* aNode )
{
    // insertNode places the node directly after aPrev

    aNode->SetNext( aPrev->GetNext() );
    aPrev->SetNext( aNode );
    aNode->SetParent( aPrev->GetParent() );
}


wxXmlNode* PCB_IO_IPC2581::insertNode( wxXmlNode* aParent, const wxString& aName )
{
    // Opening tag, closing tag, brackets and the closing slash
    m_total_bytes += 2 * aName.size() + 5;
    wxXmlNode* node = new wxXmlNode( wxXML_ELEMENT_NODE, aName );
    insertNode( aParent, node );
    return node;
}


wxXmlNode* PCB_IO_IPC2581::appendNode( wxXmlNode* aParent, const wxString& aName )
{
    // AddChild iterates through the entire list of children, so we want to avoid
    // that if possible.  When we share a parent and our next sibling is null,
    // then we are the last child and can just append to the end of the list.

    static wxXmlNode* lastNode = nullptr;
    wxXmlNode* node = new wxXmlNode( wxXML_ELEMENT_NODE, aName );

    if( lastNode && lastNode->GetParent() == aParent && lastNode->GetNext() == nullptr )
    {
        node->SetParent( aParent );
        lastNode->SetNext( node );
    }
    else
    {
        aParent->AddChild( node );
    }

    lastNode = node;

    // Opening tag, closing tag, brackets and the closing slash
    m_total_bytes += 2 * aName.size() + 5;

    return node;
}


wxString PCB_IO_IPC2581::genString( const wxString& aStr, const char* aPrefix ) const
{
    wxString str;

    if( m_version == 'C' )
    {
        str = aStr;
        str.Replace( wxT( ":" ), wxT( "_" ) );

        if( aPrefix )
            str.Prepend( wxString( aPrefix ) + wxT( ":" ) );
        else
            str.Prepend( wxT( "KI:" ) );
    }
    else
    {
        if( aPrefix )
            str = wxString::Format( "%s_", aPrefix );

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

    return genString( name, "PIN" );
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
    {
        name = wxString::Format( "%s%d", name, suffix );
    }

    m_footprint_refdes_reverse_dict[aFootprint] = name;

    return name;
}


wxString PCB_IO_IPC2581::floatVal( double aVal )
{
    wxString str = wxString::FromCDouble( aVal, m_sigfig );

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
        addAttribute( xmlHeaderNode, "xsi:schemaLocation", "http://webstds.ipc.org/2581 http://webstds.ipc.org/2581/IPC-2581B1.xsd");
    else
        addAttribute( xmlHeaderNode, "xsi:schemaLocation", "http://webstds.ipc.org/2581 http://webstds.ipc.org/2581/IPC-2581C.xsd");

    m_xml_doc->SetRoot(xmlHeaderNode);

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
        addAttribute( m_line_node,  "units", m_units_str );

        wxXmlNode* fillNode = appendNode( contentNode, "DictionaryFillDesc" );
        addAttribute( fillNode,  "units", m_units_str );

        m_shape_std_node = appendNode( contentNode, "DictionaryStandard" );
        addAttribute( m_shape_std_node,  "units", m_units_str );

        m_shape_user_node = appendNode( contentNode, "DictionaryUser" );
        addAttribute( m_shape_user_node,  "units", m_units_str );
    }
    else
    {
        m_shape_std_node = appendNode( contentNode, "DictionaryStandard" );
        addAttribute( m_shape_std_node,  "units", m_units_str );

        m_shape_user_node = appendNode( contentNode, "DictionaryUser" );
        addAttribute( m_shape_user_node,  "units", m_units_str );

        m_line_node = appendNode( contentNode, "DictionaryLineDesc" );
        addAttribute( m_line_node,  "units", m_units_str );

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

    if( aPad.GetOffset().x != 0 || aPad.GetOffset().y != 0 )
        pos += aPad.GetOffset();

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
            layer_name = genString(
                    wxString::Format( "DIELECTRIC_%d", item->GetDielectricLayerId() ), "LAYER" );
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
                addAttribute( color,  "r", wxString::Format( "%d", KiROUND( layer_color.r * 255 ) ) );
                addAttribute( color,  "g", wxString::Format( "%d", KiROUND( layer_color.g * 255 ) ) );
                addAttribute( color,  "b", wxString::Format( "%d", KiROUND( layer_color.b * 255 ) ) );
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
    finalPoly.Fracture( SHAPE_POLY_SET::PM_FAST );

    for( int ii = 0; ii < finalPoly.OutlineCount(); ++ii )
        addContourNode( aContentNode, finalPoly, ii );
}


void PCB_IO_IPC2581::addText( wxXmlNode* aContentNode, EDA_TEXT* aText,
                              const KIFONT::METRICS& aFontMetrics )
{
    if( !aText->IsVisible() )
            return;

    KIGFX::GAL_DISPLAY_OPTIONS empty_opts;
    KIFONT::FONT*              font = aText->GetFont();
    TEXT_ATTRIBUTES            attrs = aText->GetAttributes();

    attrs.m_StrokeWidth = aText->GetEffectiveTextPenWidth();
    attrs.m_Angle = aText->GetDrawRotation();
    attrs.m_Multiline = false;

    wxXmlNode* text_node = appendNode( aContentNode, "UserSpecial" );

    if( !font )
        font = KIFONT::FONT::GetFont();

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

    if( LSET( 2, F_Mask, B_Mask ).Contains( aLayer ) )
        expansion.x = expansion.y = 2 * aPad.GetSolderMaskExpansion();

    if( LSET( 2, F_Paste, B_Paste ).Contains( aLayer ) )
        expansion = 2 * aPad.GetSolderPasteMargin();

    switch( aPad.GetShape() )
    {
    case PAD_SHAPE::CIRCLE:
    {
        name = wxString::Format( "CIRCLE_%zu", m_std_shape_dict.size() + 1 );
        m_std_shape_dict.emplace( hash, name );

        wxXmlNode* entry_node = appendNode( m_shape_std_node, "EntryStandard" );
        addAttribute( entry_node,  "id", name );

        wxXmlNode* circle_node = appendNode( entry_node, "Circle" );
        circle_node->AddAttribute(
                "diameter",
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
        VECTOR2D pad_size = aPad.GetSize() + expansion;
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
        VECTOR2D pad_size = aPad.GetSize() + expansion;
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
        VECTOR2D pad_size = aPad.GetSize() + expansion;
        addAttribute( roundrect_node,  "width", floatVal( m_scale * pad_size.x ) );
        addAttribute( roundrect_node,  "height", floatVal( m_scale * pad_size.y ) );
        roundrect_node->AddAttribute( "radius",
                                      floatVal( m_scale * aPad.GetRoundRectCornerRadius() ) );
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
        VECTOR2D pad_size = aPad.GetSize() + expansion;
        addAttribute( chamfered_node,  "width", floatVal( m_scale * pad_size.x ) );
        addAttribute( chamfered_node,  "height", floatVal( m_scale * pad_size.y ) );

        int shorterSide = std::min( pad_size.x, pad_size.y );
        int chamfer = std::max( 0, KiROUND( aPad.GetChamferRectRatio() * shorterSide ) );

        addAttribute( chamfered_node,  "chamfer", floatVal( m_scale * chamfer ) );

        int positions = aPad.GetChamferPositions();

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

        VECTOR2I       pad_size = aPad.GetSize();
        VECTOR2I       trap_delta = aPad.GetDelta();
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
                                            maxError, SHAPE_POLY_SET::PM_FAST );
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
        aPad.MergePrimitivesAsPolygon( &shape );

        if( expansion != VECTOR2I( 0, 0 ) )
        {
            shape.InflateWithLinkedHoles( std::max( expansion.x, expansion.y ),
                                            CORNER_STRATEGY::ROUND_ALL_CORNERS, maxError,
                                            SHAPE_POLY_SET::PM_FAST );
        }

        addContourNode( entry_node, shape );
        break;
    }
    default:
        wxLogError( "Unknown pad type" );
        break;
    }

    if( !name.empty() )
    {
        m_std_shape_dict.emplace( hash, name );
        wxXmlNode* shape_node = appendNode( aContentNode, "StandardPrimitiveRef" );
        addAttribute( shape_node,  "id", name );
    }
}


void PCB_IO_IPC2581::addShape( wxXmlNode* aContentNode, const PCB_SHAPE& aShape )
{
    size_t hash = shapeHash( aShape );
    auto iter = m_user_shape_dict.find( hash );
    wxString name;

    if( iter != m_user_shape_dict.end() )
    {
        wxXmlNode* shape_node = appendNode( aContentNode, "UserPrimitiveRef" );
        addAttribute( shape_node,  "id", iter->second );
        return;
    }

    switch( aShape.GetShape() )
    {
    case SHAPE_T::CIRCLE:
    {
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
        addLineDesc( rect_node, aShape.GetStroke().GetWidth(), aShape.GetStroke().GetLineStyle(), true );

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
        converter.GetPoly( points, aShape.GetStroke().GetWidth() );

        wxXmlNode* point_node = appendNode( polyline_node, "PolyBegin" );
        addXY( point_node, points[0] );

        for( size_t i = 1; i < points.size(); i++ )
        {
            wxXmlNode* point_node =
                    appendNode( polyline_node, "PolyStepSegment" );
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
    }

    if( !name.empty() )
    {
        wxXmlNode* shape_node = appendNode( aContentNode, "UserPrimitiveRef" );
        addAttribute( shape_node,  "id", name );
    }

}


void PCB_IO_IPC2581::addSlotCavity( wxXmlNode* aNode, const PAD& aPad, const wxString& aName )
{
    wxXmlNode* slotNode = appendNode( aNode, "SlotCavity" );
    addAttribute( slotNode,  "name", aName );
    addAttribute( slotNode,  "platingStatus", aPad.GetAttribute() == PAD_ATTRIB::PTH ? "PLATED" : "NONPLATED" );
    addAttribute( slotNode,  "plusTol", "0.0" );
    addAttribute( slotNode,  "minusTol", "0.0" );

    if( m_version > 'B' )
        addLocationNode( slotNode, 0.0, 0.0 );

    SHAPE_POLY_SET poly_set;
    aPad.GetEffectiveShape()->TransformToPolygon( poly_set, 0, ERROR_INSIDE );

    addOutlineNode( slotNode, poly_set );
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
        std::vector<REFDES>* m_refdes;
        std::map<wxString, wxString>* m_props;
    };

    std::set<std::unique_ptr<struct BOM_ENTRY>, std::function<bool( const std::unique_ptr<struct BOM_ENTRY>&, const std::unique_ptr<struct BOM_ENTRY>& )>> bom_entries(
            []( const std::unique_ptr<struct BOM_ENTRY>& a, const std::unique_ptr<struct BOM_ENTRY>& b )
            {
                return a->m_OEMDesignRef < b->m_OEMDesignRef;
            } );

    for( FOOTPRINT* fp_it : m_board->Footprints() )
    {
        if( fp_it->GetAttributes() & FP_EXCLUDE_FROM_BOM )
            continue;

        std::unique_ptr<FOOTPRINT> fp( static_cast<FOOTPRINT*>( fp_it->Clone() ) );
        fp->SetParentGroup( nullptr );
        fp->SetPosition( {0, 0} );

        if( fp->GetLayer() != F_Cu )
            fp->Flip( fp->GetPosition(), false );

        fp->SetOrientation( ANGLE_0 );

        size_t hash = hash_fp_item( fp.get(), HASH_POS | REL_COORD );
        auto iter = m_footprint_dict.find( hash );

        if( iter == m_footprint_dict.end() )
        {
            wxLogError( "Footprint %s not found in dictionary", fp->GetFPID().GetLibItemName().wx_str() );
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
            wxLogError( "Footprint %s not found in OEMRef dictionary", fp->GetFPID().GetLibItemName().wx_str() );
        }

        entry->m_OEMDesignRef = genString( entry->m_OEMDesignRef, "REF" );
        entry->m_count = 1;
        entry->m_pads = fp->GetPadCount();

        // TODO: The options are "ELECTRICAL", "MECHANICAL", "PROGRAMMABLE", "DOCUMENT", "MATERIAL"
        //      We need to figure out how to determine this.
        if( entry->m_pads == 0 )
            entry->m_type = "DOCUMENT";
        else
            entry->m_type = "ELECTRICAL";

        auto[ bom_iter, inserted ] = bom_entries.insert( std::move( entry ) );

        if( !inserted )
            ( *bom_iter )->m_count++;

        REFDES refdes;
        refdes.m_name = fp->Reference().GetShownText( false );
        refdes.m_pkg = fp->GetFPID().GetLibItemName().wx_str();
        refdes.m_populate = !fp->IsDNP();
        refdes.m_layer = m_layer_name_map[fp_it->GetLayer()];

        ( *bom_iter )->m_refdes->push_back( refdes );

        // TODO: This amalgamates all the properties from all the footprints.  We need to decide
        // if we want to group footprints by their properties
        for( PCB_FIELD* prop : fp->GetFields() )
        {
            // We don't need ref, footprint or datasheet in the BOM characteristics.  Just value
            // and any additional fields the user has added.  Ref and footprint are captured above.
            if( prop->IsMandatoryField() && !prop->IsValue() )
                continue;

            ( *bom_iter )->m_props->emplace( prop->GetName(), prop->GetText() );
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

        for( const REFDES& refdes : *( entry->m_refdes ) )
        {
            wxXmlNode* refdesNode = appendNode( bomEntryNode, "RefDes" );
            addAttribute( refdesNode,  "name", genString( refdes.m_name, "CMP" ) );
            addAttribute( refdesNode,  "packageRef", genString( refdes.m_pkg, "PKG" ) );
            addAttribute( refdesNode,  "populate", refdes.m_populate ? "true" : "false" );
            addAttribute( refdesNode,  "layerRef", refdes.m_layer );
        }

        wxXmlNode* characteristicsNode = appendNode( bomEntryNode, "Characteristics" );
        addAttribute( characteristicsNode,  "category", "ELECTRICAL" );

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
    generateDrillLayers( cadDataNode);
    generateStepSection( cadDataNode );

    return ecadNode;
}


void PCB_IO_IPC2581::addCadHeader( wxXmlNode* aEcadNode )
{
    wxXmlNode* cadHeaderNode = appendNode( aEcadNode, "CadHeader" );
    addAttribute( cadHeaderNode,  "units", m_units_str );
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
                          aLayer == F_Cu   ? "TOP"
                          : aLayer == B_Cu ? "BOTTOM"
                                           : "INTERNAL" );
        }

        break; // Do not handle other layers
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
                    ly_name = wxString::Format( "DIELECTRIC_%d", stackup_item->GetDielectricLayerId() );
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

        wxString ly_name = genString( m_board->GetLayerName( layer ), "LAYER" );
        m_layer_name_map.emplace( layer, ly_name );
        added_layers.insert( layer );
        wxXmlNode* cadLayerNode = appendNode( aCadLayerNode, "Layer" );
        addAttribute( cadLayerNode,  "name", ly_name );

        addLayerAttributes( cadLayerNode, layer );
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
            if( pad->HasHole() && pad->GetDrillSizeX() != pad->GetDrillSizeY() )
                m_slot_holes[std::make_pair( F_Cu, B_Cu )].push_back( pad );
            else if( pad->HasHole() )
                m_drill_layers[std::make_pair( F_Cu, B_Cu )].push_back( pad );
        }
    }

    for( const auto& [layer_pair, vec] : m_drill_layers )
    {
        wxXmlNode* drillNode = appendNode( aCadLayerNode, "Layer" );
        drillNode->AddAttribute( "name", genString( wxString::Format( "%s_%s",
                                                              m_board->GetLayerName( layer_pair.first ),
                                                              m_board->GetLayerName( layer_pair.second ) ), "DRILL" ) );
        addAttribute( drillNode,  "layerFunction", "DRILL" );
        addAttribute( drillNode,  "polarity", "POSITIVE" );
        addAttribute( drillNode,  "side", "ALL" );

        wxXmlNode* spanNode = appendNode( drillNode, "Span" );
        addAttribute( spanNode,  "fromLayer", genString( m_board->GetLayerName( layer_pair.first ), "LAYER" ) );
        addAttribute( spanNode,  "toLayer", genString( m_board->GetLayerName( layer_pair.second ), "LAYER" ) );
    }

    for( const auto& [layer_pair, vec] : m_slot_holes )
    {
        wxXmlNode* drillNode = appendNode( aCadLayerNode, "Layer" );
        drillNode->AddAttribute( "name", genString( wxString::Format( "%s_%s",
                                                              m_board->GetLayerName( layer_pair.first ),
                                                              m_board->GetLayerName( layer_pair.second ) ), "SLOT" ) );

        addAttribute( drillNode,  "layerFunction", "ROUT" );
        addAttribute( drillNode,  "polarity", "POSITIVE" );
        addAttribute( drillNode,  "side", "ALL" );

        wxXmlNode* spanNode = appendNode( drillNode, "Span" );
        addAttribute( spanNode,  "fromLayer", genString( m_board->GetLayerName( layer_pair.first ), "LAYER" ) );
        addAttribute( spanNode,  "toLayer", genString( m_board->GetLayerName( layer_pair.second ), "LAYER" ) );
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
}


void PCB_IO_IPC2581::addPad( wxXmlNode* aContentNode, const PAD* aPad, PCB_LAYER_ID aLayer )
{
    wxXmlNode* padNode = appendNode( aContentNode, "Pad" );
    FOOTPRINT* fp = aPad->GetParentFootprint();

    addPadStack( padNode, aPad );

    if( aPad->GetOrientation() != ANGLE_0 )
    {
        wxXmlNode* xformNode = appendNode( padNode, "Xform" );
        xformNode->AddAttribute( "rotation",
                                 floatVal( aPad->GetOrientation().Normalize().AsDegrees() ) );

        if( fp && fp->IsFlipped() )
            addAttribute( xformNode,  "mirror", "true" );
    }

    addLocationNode( padNode, *aPad, false );
    addShape( padNode, *aPad, aLayer );

    if( fp )
    {
        wxXmlNode* pinRefNode = appendNode( padNode, "PinRef" );

        addAttribute( pinRefNode,  "pin", pinName( aPad ) );
        addAttribute( pinRefNode,  "componentRef", componentName( fp ) );
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
    dummy.SetSize( VECTOR2I( aVia->GetWidth(), aVia->GetWidth() ) );

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
    m_padstacks.push_back( padStackDefNode );

    // Only handle round holes here because IPC2581 does not support non-round holes
    // These will be handled in a slot layer
    if( aPad->HasHole() && aPad->GetDrillSizeX() == aPad->GetDrillSizeY() )
    {
        wxXmlNode* padStackHoleNode = appendNode( padStackDefNode, "PadstackHoleDef" );
        padStackHoleNode->AddAttribute( "name", wxString::Format( "%s%d_%d",
                                aPad->GetAttribute() == PAD_ATTRIB::PTH ? "PTH" : "NPTH",
                                aPad->GetDrillSizeX(), aPad->GetDrillSizeY() ) );

        addAttribute( padStackHoleNode,  "diameter", floatVal( m_scale * aPad->GetDrillSizeX() ) );
        addAttribute( padStackHoleNode,  "platingStatus", aPad->GetAttribute() == PAD_ATTRIB::PTH ? "PLATED" : "NONPLATED" );
        addAttribute( padStackHoleNode,  "plusTol", "0.0" );
        addAttribute( padStackHoleNode,  "minusTol", "0.0" );
        addXY( padStackHoleNode, aPad->GetOffset() );
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
        addLocationNode( padStackPadDefNode, aPad->GetOffset().x, aPad->GetOffset().y );

        if( aPad->HasHole() || !aPad->FlashLayer( layer ) )
        {
            PCB_SHAPE shape( nullptr, SHAPE_T::CIRCLE );
            shape.SetStart( aPad->GetOffset() );
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

    wxXmlNode* padStackHoleNode =
            appendNode( padStackDefNode, "PadstackHoleDef" );
    addAttribute( padStackHoleNode,  "name", wxString::Format( "PH%d", aVia->GetDrillValue() ) );
    padStackHoleNode->AddAttribute( "diameter",
                                    floatVal( m_scale * aVia->GetDrillValue() ) );
    addAttribute( padStackHoleNode,  "platingStatus", "VIA" );
    addAttribute( padStackHoleNode,  "plusTol", "0.0" );
    addAttribute( padStackHoleNode,  "minusTol", "0.0" );
    addAttribute( padStackHoleNode,  "x", "0.0" );
    addAttribute( padStackHoleNode,  "y", "0.0" );

    LSEQ layer_seq = aVia->GetLayerSet().Seq();

    for( PCB_LAYER_ID layer : layer_seq )
    {
        if( !aVia->FlashLayer( layer ) || !m_board->IsLayerEnabled( layer ) )
            continue;

        PCB_SHAPE shape( nullptr, SHAPE_T::CIRCLE );

        shape.SetEnd( { KiROUND( aVia->GetWidth() / 2.0 ), 0 } );

        wxXmlNode* padStackPadDefNode =
                appendNode( padStackDefNode, "PadstackPadDef" );
        addAttribute( padStackPadDefNode,  "layerRef", m_layer_name_map[layer] );
        addAttribute( padStackPadDefNode,  "padUse", "REGULAR" );

        addLocationNode( padStackPadDefNode, 0.0, 0.0 );
        addShape( padStackPadDefNode, shape );
    }
}


bool PCB_IO_IPC2581::addPolygonNode( wxXmlNode* aParentNode,
                                     const SHAPE_POLY_SET::POLYGON& aPolygon, FILL_T aFillType,
                                     int aWidth, LINE_STYLE aDashType )
{
    wxXmlNode* polygonNode = nullptr;

    if( aPolygon.empty() || aPolygon[0].PointCount() < 3 )
        return false;

    auto make_node =
            [&]()
    {
        polygonNode = appendNode( aParentNode, "Polygon" );
        wxXmlNode* polybeginNode = appendNode( polygonNode, "PolyBegin" );

        const std::vector<VECTOR2I>& pts = aPolygon[0].CPoints();
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


bool PCB_IO_IPC2581::addPolygonCutouts( wxXmlNode* aParentNode, const SHAPE_POLY_SET::POLYGON& aPolygon )
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


bool PCB_IO_IPC2581::addOutlineNode( wxXmlNode* aParentNode, const SHAPE_POLY_SET& aPolySet, int aWidth, LINE_STYLE aDashType )
{
    if( aPolySet.OutlineCount() == 0 )
        return false;

    wxXmlNode* outlineNode = appendNode( aParentNode, "Outline" );

    for( int ii = 0; ii < aPolySet.OutlineCount(); ++ii )
    {
        wxCHECK2( aPolySet.Outline( ii ).PointCount() >= 3, continue );

        if( !addPolygonNode( outlineNode, aPolySet.Polygon( ii ) ) )
            wxLogDebug( "Failed to add polygon to outline" );
    }

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
                                     int aOutline, FILL_T aFillType, int aWidth,
                                     LINE_STYLE aDashType )
{
    if( aPolySet.OutlineCount() < ( aOutline + 1 ) )
        return false;

    wxXmlNode* contourNode = appendNode( aParentNode, "Contour" );

    if( addPolygonNode( contourNode, aPolySet.Polygon( aOutline ), aFillType, aWidth, aDashType ) )
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

    if( ! m_board->GetBoardPolygonOutlines( board_outline ) )
    {
        wxLogError( "Failed to get board outline" );
        return;
    }

    wxXmlNode* profileNode = appendNode( aStepNode, "Profile" );
    if( !addPolygonNode( profileNode, board_outline.Polygon( 0 ) ) )
    {
        wxLogDebug( "Failed to add polygon to profile" );
        aStepNode->RemoveChild( profileNode );
        delete profileNode;
    }
}


wxXmlNode* PCB_IO_IPC2581::addPackage( wxXmlNode* aContentNode, FOOTPRINT* aFp )
{
    std::unique_ptr<FOOTPRINT> fp( static_cast<FOOTPRINT*>( aFp->Clone() ) );
    fp->SetParentGroup( nullptr );
    fp->SetPosition( { 0, 0 } );

    if( fp->GetLayer() != F_Cu )
        fp->Flip( fp->GetPosition(), false );

    fp->SetOrientation( ANGLE_0 );


    size_t hash = hash_fp_item( fp.get(), HASH_POS | REL_COORD );
    wxString name = genString( wxString::Format( "%s_%zu", fp->GetFPID().GetLibItemName().wx_str(),
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

    const SHAPE_POLY_SET& courtyard = fp->GetCourtyard( F_CrtYd );
    const SHAPE_POLY_SET& courtyard_back = fp->GetCourtyard( B_CrtYd );

    if( courtyard.OutlineCount() > 0 )
        addOutlineNode( packageNode, courtyard, courtyard.Outline( 0 ).Width(), LINE_STYLE::SOLID );

    if( courtyard_back.OutlineCount() > 0 )
    {
        otherSideViewNode = appendNode( packageNode, "OtherSideView" );
        addOutlineNode( otherSideViewNode, courtyard_back, courtyard_back.Outline( 0 ).Width(), LINE_STYLE::SOLID );
    }

    if( !courtyard.OutlineCount() && !courtyard_back.OutlineCount() )
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
                bool is_back = aLayer == B_SilkS || aLayer == B_Fab;

                if( is_back )
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

                    addShape( output_node, *static_cast<PCB_SHAPE*>( item ) );

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

    for( auto&[layer, bbox] : layer_bbox)
    {
        if( bbox.GetWidth() > 0 )
        {
            wxXmlNode* outlineNode = insertNode( layer_nodes[layer], "Outline" );

            SHAPE_POLY_SET::POLYGON outline( 1 );
            std::vector<VECTOR2I> points( 4 );
            points[0] = bbox.GetPosition();
            points[2] = bbox.GetEnd();
            points[1].x = points[0].x;
            points[1].y = points[2].y;
            points[3].x = points[2].x;
            points[3].y = points[0].y;

            outline[0].Append( points );
            addPolygonNode( outlineNode, outline, FILL_T::NO_FILL, 0 );
            addLineDesc( outlineNode, 0, LINE_STYLE::SOLID );
        }
    }

    for( size_t ii = 0; ii < fp->Pads().size(); ++ii )
    {
        PAD* pad = fp->Pads()[ii];
        wxXmlNode* pinNode = appendNode( packageNode, "Pin" );
        wxString name = pinName( pad );

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

        if( pad->GetFPRelativeOrientation() != ANGLE_0 )
        {
            wxXmlNode* xformNode = appendNode( pinNode, "Xform" );
            xformNode->AddAttribute(
                    "rotation",
                    floatVal( pad->GetFPRelativeOrientation().Normalize().AsDegrees() ) );
        }

        addLocationNode( pinNode, *pad, true );
        addShape( pinNode, *pad, pad->GetLayer() );

        // We just need the padstack, we don't need the reference here.  The reference will be created
        // in the LayerFeature set
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
            field = fp->GetFieldByName( m_OEMRef );

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
            wxLogError( "Duplicate footprint pointers.  Please report this bug." );

        addAttribute( componentNode,  "part", genString( name, "REF" ) );
        addAttribute( componentNode,  "layerRef", m_layer_name_map[fp->GetLayer()] );

        if( fp->GetAttributes() & FP_THROUGH_HOLE )
            addAttribute( componentNode,  "mountType", "THMT" );
        else if( fp->GetAttributes() & FP_SMD )
            addAttribute( componentNode,  "mountType", "SMT" );
        else
            addAttribute( componentNode,  "mountType", "OTHER" );

        if( fp->GetOrientation() != ANGLE_0 || fp->GetLayer() != F_Cu )
        {
            wxXmlNode* xformNode = appendNode( componentNode, "Xform" );

            if( fp->GetOrientation() != ANGLE_0 )
                addAttribute( xformNode,  "rotation", floatVal( fp->GetOrientation().Normalize().AsDegrees() ) );

            if( fp->GetLayer() != F_Cu )
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
        addAttribute( netNode,  "name", genString( m_board->GetNetInfo().GetNetItem( net )->GetNetname(), "NET" ) ) ;

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

    std::for_each(
        m_board->Tracks().begin(), m_board->Tracks().end(),
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
                        {
                            elements[layer][zone->GetNetCode()].push_back( zone );
                        }
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

        for( const NETINFO_ITEM* net : nets )
        {
            if( m_progressReporter )
            {
                m_progressReporter->Report( wxString::Format( _( "Exporting Layer %s, Net %s" ),
                                                               m_board->GetLayerName( layer ),
                                                               net->GetNetname() ) );
                m_progressReporter->AdvanceProgress();
            }

            std::vector<BOARD_ITEM*>& vec = elements[layer][net->GetNetCode()];

            std::stable_sort( vec.begin(), vec.end(),
                       []( BOARD_ITEM* a, BOARD_ITEM* b )
                       {
                            if( a->GetParentFootprint() == b->GetParentFootprint() )
                                return a->Type() < b->Type();

                            return a->GetParentFootprint() < b->GetParentFootprint();
                       } );

            if( vec.empty() )
                continue;

            generateLayerSetNet( layerNode, layer, vec );
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
    for( const auto& [layer_pair, vec] : m_drill_layers )
    {
        wxXmlNode* layerNode = appendNode( aLayerNode, "LayerFeature" );
        layerNode->AddAttribute( "layerRef", genString(
                                                wxString::Format( "%s_%s",
                                                    m_board->GetLayerName( layer_pair.first ),
                                                    m_board->GetLayerName( layer_pair.second ) ),
                                                "DRILL" ) );

        for( BOARD_ITEM* item : vec )
        {
            if( item->Type() == PCB_VIA_T )
            {
                PCB_VIA* via = static_cast<PCB_VIA*>( item );
                auto it = m_padstack_dict.find( hash_fp_item( via, 0 ) );

                if( it == m_padstack_dict.end() )
                {
                    wxLogError( "Failed to find padstack for via" );
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
            }

            else if( item->Type() == PCB_PAD_T )
            {
                PAD* pad = static_cast<PAD*>( item );
                auto it = m_padstack_dict.find( hash_fp_item( pad, 0 ) );

                if( it == m_padstack_dict.end() )
                {
                    wxLogError( "Failed to find padstack for pad" );
                    continue;
                }

                wxXmlNode* padNode = appendNode( layerNode, "Set" );
                addAttribute( padNode,  "geometry", it->second );

                if( pad->GetNetCode() > 0 )
                    addAttribute( padNode,  "net", genString( pad->GetNetname(), "NET" ) );

                wxXmlNode* holeNode = appendNode( padNode, "Hole" );
                addAttribute( holeNode,  "name", wxString::Format( "H%d", hole_count++ ) );
                addAttribute( holeNode,  "diameter", floatVal( m_scale * pad->GetDrillSizeX() ) );
                addAttribute( holeNode,  "platingStatus", pad->GetAttribute() == PAD_ATTRIB::PTH ? "PLATED" : "NONPLATED" );
                addAttribute( holeNode,  "plusTol", "0.0" );
                addAttribute( holeNode,  "minusTol", "0.0" );
                addXY( holeNode, pad->GetPosition() );
            }
        }
    }

    hole_count = 1;
    for( const auto& [layer_pair, vec] : m_slot_holes )
    {
        wxXmlNode* layerNode = appendNode( aLayerNode, "LayerFeature" );
        layerNode->AddAttribute( "layerRef", genString(
                                                wxString::Format( "%s_%s",
                                                    m_board->GetLayerName( layer_pair.first ),
                                                    m_board->GetLayerName( layer_pair.second ) ),
                                                "SLOT" ) );

        for( PAD* pad : vec )
        {
            wxXmlNode* padNode = appendNode( layerNode, "Set" );

            if( pad->GetNetCode() > 0 )
                addAttribute( padNode,  "net", genString( pad->GetNetname(), "NET" ) );

            addSlotCavity( padNode, *pad, wxString::Format( "SLOT%d", hole_count++ )  );
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

    if( BOARD_CONNECTED_ITEM* item = dynamic_cast<BOARD_CONNECTED_ITEM*>( *it ); IsCopperLayer( aLayer ) && item )
    {
        if( item->GetNetCode() > 0 )
            addAttribute( layerSetNode,  "net", genString( item->GetNetname(), "NET" ) );
    }
    auto add_track = [&]( PCB_TRACK* track )
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

    auto add_zone = [&]( ZONE* zone )
    {
        wxXmlNode* zoneFeatureNode = specialNode;

        if( zone->IsTeardropArea() && m_version > 'B' )
        {
            if( !teardropFeatureSetNode )
            {
                teardropLayerSetNode = appendNode( aLayerNode, "Set" );
                addAttribute( teardropLayerSetNode,  "geometryUsage", "TEARDROP" );

                if( zone->GetNetCode() > 0 )
                    addAttribute( teardropLayerSetNode,  "net", genString( zone->GetNetname(), "NET" ) );

                wxXmlNode* new_teardrops = appendNode( teardropLayerSetNode, "Features" );
                addLocationNode( new_teardrops, 0.0, 0.0 );
                teardropFeatureSetNode = appendNode( new_teardrops, "UserSpecial" );
            }

            zoneFeatureNode = teardropFeatureSetNode;
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

    auto add_shape = [&] ( PCB_SHAPE* shape )
    {
        FOOTPRINT* fp = shape->GetParentFootprint();

        if( fp )
        {
            wxXmlNode* tempSetNode = appendNode( aLayerNode, "Set" );

            if( m_version > 'B' )
                addAttribute( tempSetNode,  "geometryUsage", "GRAPHIC" );

            addAttribute( tempSetNode,  "componentRef", componentName( fp ) );

            wxXmlNode* tempFeature = appendNode( tempSetNode, "Features" );
            addLocationNode( tempFeature, *shape );

            addShape( tempFeature, *shape );
        }
        else if( shape->GetShape() == SHAPE_T::CIRCLE || shape->GetShape() == SHAPE_T::RECTANGLE || shape->GetShape() == SHAPE_T::POLY )
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

    auto add_text = [&] ( BOARD_ITEM* text )
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

        if( fp )
            addAttribute( tempSetNode, "componentRef", componentName( fp ) );

        wxXmlNode* nonStandardAttributeNode = appendNode( tempSetNode, "NonstandardAttribute" );
        addAttribute( nonStandardAttributeNode,  "name", "TEXT" );
        addAttribute( nonStandardAttributeNode,  "value", text_item->GetShownText( false ) );
        addAttribute( nonStandardAttributeNode,  "type", "STRING" );

        wxXmlNode* tempFeature = appendNode( tempSetNode, "Features" );
        addLocationNode( tempFeature, 0.0, 0.0 );
        addText( tempFeature, text_item, text->GetFontMetrics() );

        if( text->Type() == PCB_TEXTBOX_T )
        {
            PCB_TEXTBOX* textbox = static_cast<PCB_TEXTBOX*>( text );

            if( textbox->IsBorderEnabled() )
                addShape( tempFeature, *static_cast<PCB_SHAPE*>( textbox ) );
        }
    };

    auto add_pad = [&]( PAD* pad )
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
            wxLogDebug( "Unhandled type %s",
                        ENUM_MAP<KICAD_T>::Instance().ToString( item->Type() ) );
        }
    }

    if( specialNode->GetChildren() == nullptr )
    {
        featureSetNode->RemoveChild( specialNode );
        layerSetNode->RemoveChild( featureSetNode );
        aLayerNode->RemoveChild( layerSetNode );
        delete specialNode;
        delete featureSetNode;
        delete layerSetNode;
    }
}


wxXmlNode* PCB_IO_IPC2581::generateAvlSection()
{
    if( m_progressReporter )
        m_progressReporter->AdvancePhase( _( "Generating BOM section" ) );

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

        PCB_FIELD* nums[2] = { fp->GetFieldByName( m_mpn ), fp->GetFieldByName( m_distpn ) };
        PCB_FIELD* company[2] = { fp->GetFieldByName( m_mfg ), nullptr };
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
                                const STRING_UTF8_MAP* aProperties )
{
    m_board = aBoard;
    m_units_str = "MILLIMETER";
    m_scale = 1.0 / PCB_IU_PER_MM;
    m_sigfig = 4;

    if( auto it = aProperties->find( "units" ); it != aProperties->end() )
    {
        if( it->second == "inch" )
        {
            m_units_str = "INCH";
            m_scale = 25.4 / PCB_IU_PER_MM;
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
        wxLogError( _( "Failed to save file to buffer" ) );
        return;
    }

    size_t size = out_stream.GetSize();

}
