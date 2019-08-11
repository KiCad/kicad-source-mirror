/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2017 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2015 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright (C) 2015 Wayne Stambaugh <stambaughw@gmail.com>
 * Copyright (C) 1992-2019 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <fctsys.h>
#include <gr_basic.h>
#include <plotter.h>
#include <trigo.h>
#include <confirm.h>
#include <kicad_string.h>
#include <pcbnew.h>
#include <refdes_utils.h>
#include <richio.h>
#include <filter_reader.h>
#include <macros.h>
#include <msgpanel.h>
#include <bitmaps.h>
#include <unordered_set>
#include <pcb_edit_frame.h>
#include <class_board.h>
#include <class_edge_mod.h>
#include <class_module.h>
#include <convert_basic_shapes_to_polygon.h>
#include <view/view.h>

MODULE::MODULE( BOARD* parent ) :
    BOARD_ITEM_CONTAINER( (BOARD_ITEM*) parent, PCB_MODULE_T ),
    m_initial_comments( 0 )
{
    m_Attributs    = MOD_DEFAULT;
    m_Layer        = F_Cu;
    m_Orient       = 0;
    m_ModuleStatus = MODULE_PADS_LOCKED;
    m_arflag = 0;
    m_CntRot90 = m_CntRot180 = 0;
    m_Link     = 0;
    m_LastEditTime  = 0;
    m_LocalClearance = 0;
    m_LocalSolderMaskMargin  = 0;
    m_LocalSolderPasteMargin = 0;
    m_LocalSolderPasteMarginRatio = 0.0;
    m_ZoneConnection = PAD_ZONE_CONN_INHERITED; // Use zone setting by default
    m_ThermalWidth = 0;     // Use zone setting by default
    m_ThermalGap = 0;       // Use zone setting by default

    // These are special and mandatory text fields
    m_Reference = new TEXTE_MODULE( this, TEXTE_MODULE::TEXT_is_REFERENCE );
    m_Value = new TEXTE_MODULE( this, TEXTE_MODULE::TEXT_is_VALUE );

    m_3D_Drawings.clear();
}


MODULE::MODULE( const MODULE& aModule ) :
    BOARD_ITEM_CONTAINER( aModule )
{
    m_Pos = aModule.m_Pos;
    m_fpid = aModule.m_fpid;
    m_Attributs = aModule.m_Attributs;
    m_ModuleStatus = aModule.m_ModuleStatus;
    m_Orient = aModule.m_Orient;
    m_BoundaryBox = aModule.m_BoundaryBox;
    m_CntRot90 = aModule.m_CntRot90;
    m_CntRot180 = aModule.m_CntRot180;
    m_LastEditTime = aModule.m_LastEditTime;
    m_Link = aModule.m_Link;
    m_Path = aModule.m_Path;              // is this correct behavior?

    m_LocalClearance = aModule.m_LocalClearance;
    m_LocalSolderMaskMargin = aModule.m_LocalSolderMaskMargin;
    m_LocalSolderPasteMargin = aModule.m_LocalSolderPasteMargin;
    m_LocalSolderPasteMarginRatio = aModule.m_LocalSolderPasteMarginRatio;
    m_ZoneConnection = aModule.m_ZoneConnection;
    m_ThermalWidth = aModule.m_ThermalWidth;
    m_ThermalGap = aModule.m_ThermalGap;

    // Copy reference and value.
    m_Reference = new TEXTE_MODULE( *aModule.m_Reference );
    m_Reference->SetParent( this );
    m_Value = new TEXTE_MODULE( *aModule.m_Value );
    m_Value->SetParent( this );

    // Copy auxiliary data: Pads
    for( auto pad : aModule.Pads() )
    {
        Add( new D_PAD( *pad ) );
    }

    // Copy auxiliary data: Drawings
    for( auto item : aModule.GraphicalItems() )
    {
        switch( item->Type() )
        {
        case PCB_MODULE_TEXT_T:
        case PCB_MODULE_EDGE_T:
            Add( static_cast<BOARD_ITEM*>( item->Clone() ) );
            break;

        default:
            wxLogMessage( wxT( "Class MODULE copy constructor internal error: unknown type" ) );
            break;
        }
    }

    // Copy auxiliary data: 3D_Drawings info
    m_3D_Drawings = aModule.m_3D_Drawings;

    m_Doc     = aModule.m_Doc;
    m_KeyWord = aModule.m_KeyWord;

    m_arflag = 0;

    // Ensure auxiliary data is up to date
    CalculateBoundingBox();

    m_initial_comments = aModule.m_initial_comments ?
                            new wxArrayString( *aModule.m_initial_comments ) : 0;
}


MODULE::~MODULE()
{
    // Clean up the owned elements
    delete m_Reference;
    delete m_Value;
    delete m_initial_comments;

    for( auto p : m_pads )
        delete p;

    m_pads.clear();

    for( auto d : m_drawings )
        delete d;

    m_drawings.clear();
}


MODULE& MODULE::operator=( const MODULE& aOther )
{
    BOARD_ITEM::operator=( aOther );

    m_Pos           = aOther.m_Pos;
    m_fpid          = aOther.m_fpid;
    m_Attributs     = aOther.m_Attributs;
    m_ModuleStatus  = aOther.m_ModuleStatus;
    m_Orient        = aOther.m_Orient;
    m_BoundaryBox   = aOther.m_BoundaryBox;
    m_CntRot90      = aOther.m_CntRot90;
    m_CntRot180     = aOther.m_CntRot180;
    m_LastEditTime  = aOther.m_LastEditTime;
    m_Link          = aOther.m_Link;
    m_Path          = aOther.m_Path; //is this correct behavior?

    m_LocalClearance                = aOther.m_LocalClearance;
    m_LocalSolderMaskMargin         = aOther.m_LocalSolderMaskMargin;
    m_LocalSolderPasteMargin        = aOther.m_LocalSolderPasteMargin;
    m_LocalSolderPasteMarginRatio   = aOther.m_LocalSolderPasteMarginRatio;
    m_ZoneConnection                = aOther.m_ZoneConnection;
    m_ThermalWidth                  = aOther.m_ThermalWidth;
    m_ThermalGap                    = aOther.m_ThermalGap;

    // Copy reference and value
    *m_Reference = *aOther.m_Reference;
    m_Reference->SetParent( this );
    *m_Value = *aOther.m_Value;
    m_Value->SetParent( this );

    // Copy auxiliary data: Pads
    m_pads.clear();

    for( auto pad : aOther.Pads() )
    {
        Add( new D_PAD( *pad ) );
    }

    // Copy auxiliary data: Drawings
    m_drawings.clear();

    for( auto item : aOther.GraphicalItems() )
    {
        switch( item->Type() )
        {
        case PCB_MODULE_TEXT_T:
        case PCB_MODULE_EDGE_T:
            Add( static_cast<BOARD_ITEM*>( item->Clone() ) );
            break;

        default:
            wxLogMessage( wxT( "MODULE::operator=() internal error: unknown type" ) );
            break;
        }
    }

    // Copy auxiliary data: 3D_Drawings info
    m_3D_Drawings.clear();
    m_3D_Drawings = aOther.m_3D_Drawings;
    m_Doc         = aOther.m_Doc;
    m_KeyWord     = aOther.m_KeyWord;

    // Ensure auxiliary data is up to date
    CalculateBoundingBox();

    return *this;
}


void MODULE::ClearAllNets()
{
    // Force the ORPHANED dummy net info for all pads.
    // ORPHANED dummy net does not depend on a board
    for( auto pad : m_pads )
        pad->SetNetCode( NETINFO_LIST::ORPHANED );
}


void MODULE::Add( BOARD_ITEM* aBoardItem, ADD_MODE aMode )
{
    switch( aBoardItem->Type() )
    {
    case PCB_MODULE_TEXT_T:
        // Only user texts can be added this way. Reference and value are not hold in the DLIST.
        assert( static_cast<TEXTE_MODULE*>( aBoardItem )->GetType() == TEXTE_MODULE::TEXT_is_DIVERS );

        // no break

    case PCB_MODULE_EDGE_T:
        if( aMode == ADD_APPEND )
            m_drawings.push_back( aBoardItem );
        else
            m_drawings.push_front( aBoardItem );
        break;

    case PCB_PAD_T:
        if( aMode == ADD_APPEND )
            m_pads.push_back( static_cast<D_PAD*>( aBoardItem ) );
        else
            m_pads.push_front( static_cast<D_PAD*>( aBoardItem ) );
        break;

    default:
    {
        wxString msg;
        msg.Printf( wxT( "MODULE::Add() needs work: BOARD_ITEM type (%d) not handled" ),
                    aBoardItem->Type() );
        wxFAIL_MSG( msg );

        return;
    }
    }

    aBoardItem->ClearEditFlags();
    aBoardItem->SetParent( this );
}


void MODULE::Remove( BOARD_ITEM* aBoardItem )
{
    switch( aBoardItem->Type() )
    {
    case PCB_MODULE_TEXT_T:
        // Only user texts can be removed this way. Reference and value are not hold in the DLIST.
        wxCHECK_RET(
                static_cast<TEXTE_MODULE*>( aBoardItem )->GetType() == TEXTE_MODULE::TEXT_is_DIVERS,
                "Please report this bug: Invalid remove operation on required text" );

        // no break

    case PCB_MODULE_EDGE_T:
        for( auto it = m_drawings.begin(); it != m_drawings.end(); ++it )
        {
            if( *it == aBoardItem )
            {
                m_drawings.erase( it );
                break;
            }
        }

        break;

    case PCB_PAD_T:
        for( auto it = m_pads.begin(); it != m_pads.end(); ++it )
        {
            if( *it == static_cast<D_PAD*>( aBoardItem ) )
            {
                m_pads.erase( it );
                break;
            }
        }

        break;

    default:
    {
        wxString msg;
        msg.Printf( wxT( "MODULE::Remove() needs work: BOARD_ITEM type (%d) not handled" ),
                    aBoardItem->Type() );
        wxFAIL_MSG( msg );
    }
    }
}


void MODULE::Print( PCB_BASE_FRAME* aFrame, wxDC* aDC, const wxPoint& aOffset )
{
    for( auto pad : m_pads )
        pad->Print( aFrame, aDC, aOffset );

    BOARD* brd = GetBoard();

    // Draw graphic items
    if( brd->IsElementVisible( LAYER_MOD_REFERENCES ) )
        m_Reference->Print( aFrame, aDC, aOffset );

    if( brd->IsElementVisible( LAYER_MOD_VALUES ) )
        m_Value->Print( aFrame, aDC, aOffset );

    for( auto item : m_drawings )
    {
        switch( item->Type() )
        {
        case PCB_MODULE_TEXT_T:
        case PCB_MODULE_EDGE_T:
            item->Print( aFrame, aDC, aOffset );
            break;

        default:
            break;
        }
    }
}


void MODULE::CalculateBoundingBox()
{
    m_BoundaryBox = GetFootprintRect();
}


double MODULE::GetArea( int aPadding ) const
{
    double w = std::abs( m_BoundaryBox.GetWidth() ) + aPadding;
    double h = std::abs( m_BoundaryBox.GetHeight() ) + aPadding;
    return w * h;
}


EDA_RECT MODULE::GetFootprintRect() const
{
    EDA_RECT area;

    area.SetOrigin( m_Pos );
    area.SetEnd( m_Pos );
    area.Inflate( Millimeter2iu( 0.25 ) );   // Give a min size to the area

    for( auto item : m_drawings )
    {
        if( item->Type() == PCB_MODULE_EDGE_T )
            area.Merge( item->GetBoundingBox() );
    }

    for( auto pad : m_pads )
        area.Merge( pad->GetBoundingBox() );

    return area;
}


const EDA_RECT MODULE::GetBoundingBox() const
{
    EDA_RECT area = GetFootprintRect();

    // Add in items not collected by GetFootprintRect():
    for( auto item : m_drawings )
    {
        if( item->Type() != PCB_MODULE_EDGE_T )
            area.Merge( item->GetBoundingBox() );
    }

    area.Merge( m_Value->GetBoundingBox() );
    area.Merge( m_Reference->GetBoundingBox() );

    return area;
}


/**
 * This is a bit hacky right now for performance reasons.
 *
 * We assume that most footprints will have features aligned to the axes in
 * the zero-rotation state.  Therefore, if the footprint is rotated, we
 * temporarily rotate back to zero, get the bounding box (excluding reference
 * and value text) and then rotate the resulting poly back to the correct
 * orientation.
 *
 * This is more accurate than using the AABB when most footprints are rotated
 * off of the axes, but less accurate than computing some kind of bounding hull.
 * We should consider doing that instead at some point in the future if we can
 * use a performant algorithm and cache the result to avoid extra computing.
 */
SHAPE_POLY_SET MODULE::GetBoundingPoly() const
{
    SHAPE_POLY_SET poly;

    double orientation = GetOrientationRadians();

    MODULE temp = *this;
    temp.SetOrientation( 0.0 );
    BOX2I area = temp.GetFootprintRect();

    poly.NewOutline();

    VECTOR2I p = area.GetPosition();
    poly.Append( p );
    p.x = area.GetRight();
    poly.Append( p );
    p.y = area.GetBottom();
    poly.Append( p );
    p.x = area.GetX();
    poly.Append( p );

    BOARD* board = GetBoard();
    if( board )
    {
        int biggest_clearance = board->GetDesignSettings().GetBiggestClearanceValue();
        poly.Inflate( biggest_clearance, 4 );
    }

    poly.Inflate( Millimeter2iu( 0.01 ), 4 );
    poly.Rotate( -orientation, m_Pos );

    return poly;
}


void MODULE::GetMsgPanelInfo( EDA_UNITS_T aUnits, std::vector< MSG_PANEL_ITEM >& aList )
{
    wxString msg;

    aList.emplace_back( MSG_PANEL_ITEM( m_Reference->GetShownText(), m_Value->GetShownText(), DARKCYAN ) );

    // Display last date the component was edited (useful in Module Editor).
    wxDateTime date( static_cast<time_t>( m_LastEditTime ) );

    if( m_LastEditTime && date.IsValid() )
    // Date format: see http://www.cplusplus.com/reference/ctime/strftime
        msg = date.Format( wxT( "%b %d, %Y" ) ); // Abbreviated_month_name Day, Year
    else
        msg = _( "Unknown" );

    aList.emplace_back( MSG_PANEL_ITEM( _( "Last Change" ), msg, BROWN ) );

    // display schematic path
    aList.emplace_back( MSG_PANEL_ITEM( _( "Netlist Path" ), m_Path, BROWN ) );

    // display the board side placement
    aList.emplace_back( MSG_PANEL_ITEM( _( "Board Side" ),
                        IsFlipped()? _( "Back (Flipped)" ) : _( "Front" ), RED ) );


    msg.Printf( wxT( "%zu" ), m_pads.size() );
    aList.emplace_back( MSG_PANEL_ITEM( _( "Pads" ), msg, BLUE ) );

    msg = wxT( ".." );

    if( IsLocked() )
        msg[0] = 'L';

    if( m_ModuleStatus & MODULE_is_PLACED )
        msg[1] = 'P';

    aList.emplace_back( MSG_PANEL_ITEM( _( "Status" ), msg, MAGENTA ) );

    msg.Printf( wxT( "%.1f" ), GetOrientationDegrees() );
    aList.emplace_back( MSG_PANEL_ITEM( _( "Rotation" ), msg, BROWN ) );

    // Controls on right side of the dialog
    switch( m_Attributs & 255 )
    {
    case 0:
        msg = _( "Normal" );
        break;

    case MOD_CMS:
        msg = _( "Insert" );
        break;

    case MOD_VIRTUAL:
        msg = _( "Virtual" );
        break;

    default:
        msg = wxT( "???" );
        break;
    }

    aList.emplace_back( MSG_PANEL_ITEM( _( "Attributes" ), msg, BROWN ) );
    aList.emplace_back( MSG_PANEL_ITEM( _( "Footprint" ), FROM_UTF8( m_fpid.Format().c_str() ), BLUE ) );

    if( m_3D_Drawings.empty() )
        msg = _( "No 3D shape" );
    else
        msg = m_3D_Drawings.front().m_Filename;

    // Search the first active 3D shape in list

    aList.emplace_back( MSG_PANEL_ITEM( _( "3D-Shape" ), msg, RED ) );

    wxString doc, keyword;
    doc.Printf( _( "Doc: %s" ), m_Doc );
    keyword.Printf( _( "Key Words: %s" ), m_KeyWord );
    aList.emplace_back( MSG_PANEL_ITEM( doc, keyword, BLACK ) );
}


bool MODULE::HitTest( const wxPoint& aPosition, int aAccuracy ) const
{
    EDA_RECT rect = m_BoundaryBox;
    return rect.Inflate( aAccuracy ).Contains( aPosition );
}


bool MODULE::HitTestAccurate( const wxPoint& aPosition, int aAccuracy ) const
{
    return GetBoundingPoly().Collide( aPosition, aAccuracy );
}


bool MODULE::HitTest( const EDA_RECT& aRect, bool aContained, int aAccuracy ) const
{
    EDA_RECT arect = aRect;
    arect.Inflate( aAccuracy );

    if( aContained )
        return arect.Contains( m_BoundaryBox );
    else
    {
        // If the rect does not intersect the bounding box, skip any tests
        if( !aRect.Intersects( GetBoundingBox() ) )
            return false;

        // Determine if any elements in the MODULE intersect the rect
        for( auto pad : m_pads )
        {
            if( pad->HitTest( arect, false, 0 ) )
                return true;
        }

        for( auto item : m_drawings )
        {
            if( item->HitTest( arect, false, 0 ) )
                return true;
        }

        // No items were hit
        return false;
    }
}


D_PAD* MODULE::FindPadByName( const wxString& aPadName ) const
{
    for( auto pad : m_pads )
    {
        if( pad->GetName() == aPadName )
            return pad;
    }

    return NULL;
}


D_PAD* MODULE::GetPad( const wxPoint& aPosition, LSET aLayerMask )
{
    for( auto pad : m_pads )
    {
        // ... and on the correct layer.
        if( !( pad->GetLayerSet() & aLayerMask ).any() )
            continue;

        if( pad->HitTest( aPosition ) )
            return pad;
    }

    return NULL;
}


D_PAD* MODULE::GetTopLeftPad()
{
    D_PAD* topLeftPad = GetFirstPad();

    for( auto p : m_pads )
    {
        wxPoint pnt = p->GetPosition(); // GetPosition() returns the center of the pad

        if( ( pnt.x < topLeftPad->GetPosition().x ) ||
            ( ( topLeftPad->GetPosition().x == pnt.x ) &&
              ( pnt.y < topLeftPad->GetPosition().y ) ) )
        {
            topLeftPad = p;
        }
    }

    return topLeftPad;
}


unsigned MODULE::GetPadCount( INCLUDE_NPTH_T aIncludeNPTH ) const
{
    if( aIncludeNPTH )
        return m_pads.size();

    unsigned cnt = 0;

    for( auto pad : m_pads )
    {
        if( pad->GetAttribute() == PAD_ATTRIB_HOLE_NOT_PLATED )
            continue;

        cnt++;
    }

    return cnt;
}


unsigned MODULE::GetUniquePadCount( INCLUDE_NPTH_T aIncludeNPTH ) const
{
    std::set<wxString> usedNames;

    // Create a set of used pad numbers
    for( auto pad : m_pads )
    {
        // Skip pads not on copper layers (used to build complex
        // solder paste shapes for instance)
        if( ( pad->GetLayerSet() & LSET::AllCuMask() ).none() )
            continue;

        // Skip pads with no name, because they are usually "mechanical"
        // pads, not "electrical" pads
        if( pad->GetName().IsEmpty() )
            continue;

        if( !aIncludeNPTH )
        {
            // skip NPTH
            if( pad->GetAttribute() == PAD_ATTRIB_HOLE_NOT_PLATED )
            {
                continue;
            }
        }

        usedNames.insert( pad->GetName() );
    }

    return usedNames.size();
}


void MODULE::Add3DModel( MODULE_3D_SETTINGS* a3DModel )
{
    if( NULL == a3DModel )
        return;

    if( !a3DModel->m_Filename.empty() )
        m_3D_Drawings.push_back( *a3DModel );

    delete a3DModel;
}


// see class_module.h
SEARCH_RESULT MODULE::Visit( INSPECTOR inspector, void* testData, const KICAD_T scanTypes[] )
{
    KICAD_T        stype;
    SEARCH_RESULT  result = SEARCH_CONTINUE;
    const KICAD_T* p    = scanTypes;
    bool           done = false;

#if 0 && defined(DEBUG)
    std::cout << GetClass().mb_str() << ' ';
#endif

    while( !done )
    {
        stype = *p;

        switch( stype )
        {
        case PCB_MODULE_T:
            result = inspector( this, testData );  // inspect me
            ++p;
            break;

        case PCB_PAD_T:
            result = IterateForward<D_PAD*>( m_pads, inspector, testData, p );
            ++p;
            break;

        case PCB_MODULE_TEXT_T:
            result = inspector( m_Reference, testData );

            if( result == SEARCH_QUIT )
                break;

            result = inspector( m_Value, testData );

            if( result == SEARCH_QUIT )
                break;

        // m_Drawings can hold TYPETEXTMODULE also, so fall thru

        case PCB_MODULE_EDGE_T:
            result = IterateForward<BOARD_ITEM*>( m_drawings, inspector, testData, p );

            // skip over any types handled in the above call.
            for( ; ; )
            {
                switch( stype = *++p )
                {
                case PCB_MODULE_TEXT_T:
                case PCB_MODULE_EDGE_T:
                    continue;

                default:
                    ;
                }

                break;
            }

            break;

        default:
            done = true;
            break;
        }

        if( result == SEARCH_QUIT )
            break;
    }

    return result;
}


wxString MODULE::GetSelectMenuText( EDA_UNITS_T aUnits ) const
{
    wxString reference = GetReference();

    if( reference.IsEmpty() )
        reference = _( "<no reference>" );

    return wxString::Format( _( "Footprint %s on %s" ), reference, GetLayerName() );
}


BITMAP_DEF MODULE::GetMenuImage() const
{
    return module_xpm;
}


EDA_ITEM* MODULE::Clone() const
{
    return new MODULE( *this );
}


void MODULE::RunOnChildren( const std::function<void (BOARD_ITEM*)>& aFunction )
{
    try
    {
        for( auto pad : m_pads )
            aFunction( static_cast<BOARD_ITEM*>( pad ) );

        for( auto drawing : m_drawings )
            aFunction( static_cast<BOARD_ITEM*>( drawing ) );

        aFunction( static_cast<BOARD_ITEM*>( m_Reference ) );
        aFunction( static_cast<BOARD_ITEM*>( m_Value ) );
    }
    catch( std::bad_function_call& )
    {
        DisplayError( NULL, wxT( "Error running MODULE::RunOnChildren" ) );
    }
}


void MODULE::GetAllDrawingLayers( int aLayers[], int& aCount, bool aIncludePads ) const
{
    std::unordered_set<int> layers;

    for( auto item : m_drawings )
    {
        layers.insert( static_cast<int>( item->GetLayer() ) );
    }

    if( aIncludePads )
    {
        for( auto pad : m_pads )
        {
            int pad_layers[KIGFX::VIEW::VIEW_MAX_LAYERS], pad_layers_count;
            pad->ViewGetLayers( pad_layers, pad_layers_count );

            for( int i = 0; i < pad_layers_count; i++ )
                layers.insert( pad_layers[i] );
        }
    }

    aCount = layers.size();
    int i = 0;

    for( auto layer : layers )
        aLayers[i++] = layer;
}


void MODULE::ViewGetLayers( int aLayers[], int& aCount ) const
{
    aCount = 2;
    aLayers[0] = LAYER_ANCHOR;

    switch( m_Layer )
    {

    default:
        wxASSERT_MSG( false, "Illegal layer" );    // do you really have modules placed on other layers?
        // pass through
    case F_Cu:
        aLayers[1] = LAYER_MOD_FR;
        break;

    case B_Cu:
        aLayers[1] = LAYER_MOD_BK;
        break;
    }

    // If there are no pads, and only drawings on a silkscreen layer, then
    // report the silkscreen layer as well so that the component can be edited
    // with the silkscreen layer
    bool f_silk = false, b_silk = false, non_silk = false;

    for( auto item : m_drawings )
    {
        if( item->GetLayer() == F_SilkS )
            f_silk = true;
        else if( item->GetLayer() == B_SilkS )
            b_silk = true;
        else
            non_silk = true;
    }

    if( ( f_silk || b_silk ) && !non_silk && m_pads.empty() )
    {
        if( f_silk )
            aLayers[ aCount++ ] = F_SilkS;

        if( b_silk )
            aLayers[ aCount++ ] = B_SilkS;
    }
}


unsigned int MODULE::ViewGetLOD( int aLayer, KIGFX::VIEW* aView ) const
{
    int layer = ( m_Layer == F_Cu ) ? LAYER_MOD_FR :
                ( m_Layer == B_Cu ) ? LAYER_MOD_BK : LAYER_ANCHOR;

    // Currently it is only for anchor layer
    if( aView->IsLayerVisible( layer ) )
        return 3;

    return std::numeric_limits<unsigned int>::max();
}


const BOX2I MODULE::ViewBBox() const
{
    EDA_RECT area = GetFootprintRect();

    // Calculate extended area including text fields
    area.Merge( m_Reference->GetBoundingBox() );
    area.Merge( m_Value->GetBoundingBox() );

    // Add the Clearance shape size: (shape around the pads when the
    // clearance is shown.  Not optimized, but the draw cost is small
    // (perhaps smaller than optimization).
    BOARD* board = GetBoard();
    if( board )
    {
        int biggest_clearance = board->GetDesignSettings().GetBiggestClearanceValue();
        area.Inflate( biggest_clearance );
    }

    return area;
}


bool MODULE::IsLibNameValid( const wxString & aName )
{
    const wxChar * invalids = StringLibNameInvalidChars( false );

    if( aName.find_first_of( invalids ) != std::string::npos )
        return false;

    return true;
}


const wxChar* MODULE::StringLibNameInvalidChars( bool aUserReadable )
{
    static const wxChar invalidChars[] = wxT("%$\t\n\r \"\\/:");
    static const wxChar invalidCharsReadable[] = wxT("% $ 'tab' 'return' 'line feed' 'space' \\ \" / :");

    if( aUserReadable )
        return invalidCharsReadable;
    else
        return invalidChars;
}


void MODULE::Move( const wxPoint& aMoveVector )
{
    wxPoint newpos = m_Pos + aMoveVector;
    SetPosition( newpos );
}


void MODULE::Rotate( const wxPoint& aRotCentre, double aAngle )
{
    double  orientation = GetOrientation();
    double  newOrientation = orientation + aAngle;
    wxPoint newpos = m_Pos;
    RotatePoint( &newpos, aRotCentre, aAngle );
    SetPosition( newpos );
    SetOrientation( newOrientation );

    m_Reference->KeepUpright( orientation, newOrientation );
    m_Value->KeepUpright( orientation, newOrientation );

    for( auto item : m_drawings )
    {
        if( item->Type() == PCB_MODULE_TEXT_T )
            static_cast<TEXTE_MODULE*>( item )->KeepUpright(  orientation, newOrientation  );
    }
}


void MODULE::Flip( const wxPoint& aCentre, bool aFlipLeftRight )
{
    // Move module to its final position:
    wxPoint finalPos = m_Pos;

    if( aFlipLeftRight )
        MIRROR( finalPos.x, aCentre.x );     /// Mirror the X position
    else
        MIRROR( finalPos.y, aCentre.y );     /// Mirror the Y position

    SetPosition( finalPos );

    // Flip layer
    SetLayer( FlipLayer( GetLayer() ) );

    // Reverse mirror orientation.
    m_Orient = -m_Orient;
    NORMALIZE_ANGLE_POS( m_Orient );

    // Mirror pads to other side of board.
    for( auto pad : m_pads )
        pad->Flip( m_Pos, aFlipLeftRight );

    // Mirror reference and value.
    m_Reference->Flip( m_Pos, aFlipLeftRight );
    m_Value->Flip( m_Pos, aFlipLeftRight );

    // Reverse mirror module graphics and texts.
    for( auto item : m_drawings )
    {
        switch( item->Type() )
        {
        case PCB_MODULE_EDGE_T:
            static_cast<EDGE_MODULE*>( item )->Flip( m_Pos, aFlipLeftRight );
            break;

        case PCB_MODULE_TEXT_T:
            static_cast<TEXTE_MODULE*>( item )->Flip( m_Pos, aFlipLeftRight );
            break;

        default:
            wxMessageBox( wxT( "MODULE::Flip() error: Unknown Draw Type" ) );
            break;
        }
    }

    CalculateBoundingBox();
}


void MODULE::SetPosition( const wxPoint& newpos )
{
    wxPoint delta = newpos - m_Pos;

    m_Pos += delta;

    m_Reference->EDA_TEXT::Offset( delta );
    m_Value->EDA_TEXT::Offset( delta );

    for( auto pad : m_pads )
    {
        pad->SetPosition( pad->GetPosition() + delta );
    }

    for( auto item : m_drawings )
    {
        switch( item->Type() )
        {
        case PCB_MODULE_EDGE_T:
        {
            EDGE_MODULE* pt_edgmod = (EDGE_MODULE*) item;
            pt_edgmod->SetDrawCoord();
            break;
        }

        case PCB_MODULE_TEXT_T:
        {
            TEXTE_MODULE* text = static_cast<TEXTE_MODULE*>( item );
            text->EDA_TEXT::Offset( delta );
            break;
        }

        default:
            wxMessageBox( wxT( "Draw type undefined." ) );
            break;
        }
    }

    CalculateBoundingBox();
}


void MODULE::MoveAnchorPosition( const wxPoint& aMoveVector )
{
    /* Move the reference point of the footprint
     * the footprints elements (pads, outlines, edges .. ) are moved
     * but:
     * - the footprint position is not modified.
     * - the relative (local) coordinates of these items are modified
     * - Draw coordinates are updated
     */


    // Update (move) the relative coordinates relative to the new anchor point.
    wxPoint moveVector = aMoveVector;
    RotatePoint( &moveVector, -GetOrientation() );

    // Update of the reference and value.
    m_Reference->SetPos0( m_Reference->GetPos0() + moveVector );
    m_Reference->SetDrawCoord();
    m_Value->SetPos0( m_Value->GetPos0() + moveVector );
    m_Value->SetDrawCoord();

    // Update the pad local coordinates.
    for( auto pad : m_pads )
    {
        pad->SetPos0( pad->GetPos0() + moveVector );
        pad->SetDrawCoord();
    }

    // Update the draw element coordinates.
    for( auto item : GraphicalItems() )
    {
        switch( item->Type() )
        {
        case PCB_MODULE_EDGE_T:
            {
            EDGE_MODULE* edge = static_cast<EDGE_MODULE*>( item );
            edge->Move( moveVector );
            }
            break;

        case PCB_MODULE_TEXT_T:
            {
            TEXTE_MODULE* text = static_cast<TEXTE_MODULE*>( item );
            text->SetPos0( text->GetPos0() + moveVector );
            text->SetDrawCoord();
            }
            break;

        default:
            break;
        }
    }

    CalculateBoundingBox();
}


void MODULE::SetOrientation( double newangle )
{
    double  angleChange = newangle - m_Orient;  // change in rotation

    NORMALIZE_ANGLE_POS( newangle );

    m_Orient = newangle;

    for( auto pad : m_pads )
    {
        pad->SetOrientation( pad->GetOrientation() + angleChange );
        pad->SetDrawCoord();
    }

    // Update of the reference and value.
    m_Reference->SetDrawCoord();
    m_Value->SetDrawCoord();

    // Displace contours and text of the footprint.
    for( auto item : m_drawings )
    {
        if( item->Type() == PCB_MODULE_EDGE_T )
        {
            static_cast<EDGE_MODULE*>( item )->SetDrawCoord();
        }
        else if( item->Type() == PCB_MODULE_TEXT_T )
        {
            static_cast<TEXTE_MODULE*>( item )->SetDrawCoord();
        }
    }

    CalculateBoundingBox();
}

BOARD_ITEM* MODULE::Duplicate( const BOARD_ITEM* aItem,
                               bool aIncrementPadNumbers,
                               bool aAddToModule )
{
    BOARD_ITEM* new_item = NULL;
    D_PAD* new_pad = NULL;

    switch( aItem->Type() )
    {
    case PCB_PAD_T:
    {
        new_pad = new D_PAD( *static_cast<const D_PAD*>( aItem ) );

        if( aAddToModule )
            m_pads.push_back( new_pad );

        new_item = new_pad;
        break;
    }

    case PCB_MODULE_TEXT_T:
    {
        const TEXTE_MODULE* old_text = static_cast<const TEXTE_MODULE*>( aItem );

        // do not duplicate value or reference fields
        // (there can only be one of each)
        if( old_text->GetType() == TEXTE_MODULE::TEXT_is_DIVERS )
        {
            TEXTE_MODULE* new_text = new TEXTE_MODULE( *old_text );

            if( aAddToModule )
                Add( new_text );

            new_item = new_text;
        }
        break;
    }

    case PCB_MODULE_EDGE_T:
    {
        EDGE_MODULE* new_edge = new EDGE_MODULE(
                *static_cast<const EDGE_MODULE*>(aItem) );

        if( aAddToModule )
            Add( new_edge );

        new_item = new_edge;
        break;
    }

    case PCB_MODULE_T:
        // Ignore the module itself
        break;

    default:
        // Un-handled item for duplication
        wxASSERT_MSG( false, "Duplication not supported for items of class "
                      + aItem->GetClass() );
        break;
    }

    if( aIncrementPadNumbers && new_pad && !new_pad->IsAperturePad() )
    {
        new_pad->IncrementPadName( true, true );
    }

    return new_item;
}


wxString MODULE::GetNextPadName( bool aFillSequenceGaps ) const
{
    std::set<int> usedNumbers;

    // Create a set of used pad numbers
    for( auto pad : m_pads )
    {
        int padNumber = GetTrailingInt( pad->GetName() );
        usedNumbers.insert( padNumber );
    }

    const int nextNum = getNextNumberInSequence( usedNumbers, aFillSequenceGaps );

    return wxString::Format( wxT( "%i" ), nextNum );
}


void MODULE::IncrementReference( int aDelta )
{
    const auto& refdes = GetReference();
    SetReference( wxString::Format( wxT( "%s%i" ), UTIL::GetReferencePrefix( refdes ),
            GetTrailingInt( refdes ) + aDelta ) );
}


// Calculate the area of aPolySet, after fracturation, because
// polygons with no hole are expected.
static double polygonArea( SHAPE_POLY_SET& aPolySet )
{
    double area = 0.0;
    for( int ii = 0; ii < aPolySet.OutlineCount(); ii++ )
    {
        SHAPE_LINE_CHAIN& outline = aPolySet.Outline( ii );
        // Ensure the curr outline is closed, to calculate area
        outline.SetClosed( true );

        area += outline.Area();
     }

    return area;
}

// a helper function to add a rectangular polygon aRect to aPolySet
static void addRect( SHAPE_POLY_SET& aPolySet, wxRect aRect )
{
    aPolySet.NewOutline();

    aPolySet.Append( aRect.GetX(), aRect.GetY() );
    aPolySet.Append( aRect.GetX()+aRect.width, aRect.GetY() );
    aPolySet.Append( aRect.GetX()+aRect.width, aRect.GetY()+aRect.height );
    aPolySet.Append( aRect.GetX(), aRect.GetY()+aRect.height );
}

double MODULE::CoverageRatio( const GENERAL_COLLECTOR& aCollector ) const
{
    double moduleArea = GetFootprintRect().GetArea();
    SHAPE_POLY_SET coveredRegion;
    addRect( coveredRegion, GetFootprintRect() );

    // build list of holes (covered areas not available for selection)
    SHAPE_POLY_SET holes;

    for( auto pad : m_pads )
        addRect( holes, pad->GetBoundingBox() );

    addRect( holes, m_Reference->GetBoundingBox() );
    addRect( holes, m_Value->GetBoundingBox() );

    for( int i = 0; i < aCollector.GetCount(); ++i )
    {
        BOARD_ITEM* item = aCollector[i];

        switch( item->Type() )
        {
        case PCB_TEXT_T:
        case PCB_MODULE_TEXT_T:
        case PCB_TRACE_T:
        case PCB_VIA_T:
            addRect( holes, item->GetBoundingBox() );
            break;
        default:
            break;
        }
    }

    SHAPE_POLY_SET uncoveredRegion;
    uncoveredRegion.BooleanSubtract( coveredRegion, holes, SHAPE_POLY_SET::PM_STRICTLY_SIMPLE );
    uncoveredRegion.Simplify( SHAPE_POLY_SET::PM_STRICTLY_SIMPLE );
    uncoveredRegion.Fracture( SHAPE_POLY_SET::PM_STRICTLY_SIMPLE );

    double uncoveredRegionArea = polygonArea( uncoveredRegion );
    double coveredArea = moduleArea - uncoveredRegionArea;
    double ratio = ( coveredArea / moduleArea );

    return std::min( ratio, 1.0 );
}


// see convert_drawsegment_list_to_polygon.cpp:
extern bool ConvertOutlineToPolygon( std::vector<DRAWSEGMENT*>& aSegList, SHAPE_POLY_SET& aPolygons,
        wxString* aErrorText, unsigned int aTolerance, wxPoint* aErrorLocation = nullptr );

bool MODULE::BuildPolyCourtyard()
{
    m_poly_courtyard_front.RemoveAllContours();
    m_poly_courtyard_back.RemoveAllContours();
    // Build the courtyard area from graphic items on the courtyard.
    // Only PCB_MODULE_EDGE_T have meaning, graphic texts are ignored.
    // Collect items:
    std::vector< DRAWSEGMENT* > list_front;
    std::vector< DRAWSEGMENT* > list_back;

    for( auto item : GraphicalItems() )
    {
        if( item->GetLayer() == B_CrtYd && item->Type() == PCB_MODULE_EDGE_T )
            list_back.push_back( static_cast< DRAWSEGMENT* > ( item ) );

        if( item->GetLayer() == F_CrtYd && item->Type() == PCB_MODULE_EDGE_T )
            list_front.push_back( static_cast< DRAWSEGMENT* > ( item ) );
    }

    // Note: if no item found on courtyard layers, return true.
    // false is returned only when the shape defined on courtyard layers
    // is not convertible to a polygon
    if( !list_front.size() && !list_back.size() )
        return true;

    wxString error_msg;

    bool success = ConvertOutlineToPolygon( list_front, m_poly_courtyard_front,
                                            &error_msg, (unsigned) Millimeter2iu( 0.05 ) );

    if( success )
    {
        success = ConvertOutlineToPolygon( list_back, m_poly_courtyard_back,
                                           &error_msg, (unsigned) Millimeter2iu( 0.05 ) );
    }

    if( !error_msg.IsEmpty() )
    {
        wxLogMessage( wxString::Format( _( "Processing courtyard of \"%s\": %s" ),
                                        GetChars( GetFPID().Format() ),
                                        error_msg) );
    }

    return success;
}

void MODULE::SwapData( BOARD_ITEM* aImage )
{
    assert( aImage->Type() == PCB_MODULE_T );

    std::swap( *((MODULE*) this), *((MODULE*) aImage) );
}
