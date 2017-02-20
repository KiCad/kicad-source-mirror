/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2016 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2015 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright (C) 2015 Wayne Stambaugh <stambaughw@verizon.net>
 * Copyright (C) 1992-2016 KiCad Developers, see AUTHORS.txt for contributors.
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

/**
 * @file class_module.cpp
 * @brief MODULE class implementation.
 */

#include <fctsys.h>
#include <gr_basic.h>
#include <wxstruct.h>
#include <plot_common.h>
#include <class_drawpanel.h>
#include <trigo.h>
#include <confirm.h>
#include <kicad_string.h>
#include <pcbnew.h>
#include <colors_selection.h>
#include <richio.h>
#include <filter_reader.h>
#include <macros.h>
#include <msgpanel.h>
#include <bitmaps.h>

#include <class_board.h>
#include <class_edge_mod.h>
#include <class_module.h>

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
    m_Surface  = 0.0;
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
    m_Path = aModule.m_Path;              //is this correct behavior?

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
    for( D_PAD* pad = aModule.m_Pads;  pad;  pad = pad->Next() )
    {
        Add( new D_PAD( *pad ) );
    }

    // Copy auxiliary data: Drawings
    for( BOARD_ITEM* item = aModule.m_Drawings;  item;  item = item->Next() )
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
    delete m_Reference;
    delete m_Value;
    delete m_initial_comments;
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
    m_Pads.DeleteAll();

    for( D_PAD* pad = aOther.m_Pads;  pad;  pad = pad->Next() )
    {
        Add( new D_PAD( *pad ) );
    }

    // Copy auxiliary data: Drawings
    m_Drawings.DeleteAll();

    for( BOARD_ITEM* item = aOther.m_Drawings;  item;  item = item->Next() )
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
    for( D_PAD* pad = Pads(); pad; pad = pad->Next() )
        pad->SetNetCode( NETINFO_LIST::ORPHANED );
}


void MODULE::DrawAncre( EDA_DRAW_PANEL* panel, wxDC* DC, const wxPoint& offset,
                        int dim_ancre, GR_DRAWMODE draw_mode )
{
    GRSetDrawMode( DC, draw_mode );

    if( GetBoard()->IsElementVisible( ANCHOR_VISIBLE ) )
    {
        GRDrawAnchor( panel->GetClipBox(), DC, m_Pos.x, m_Pos.y,
                      dim_ancre,
                      g_ColorsSettings.GetItemColor( ANCHOR_VISIBLE ) );
    }
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
            m_Drawings.PushBack( aBoardItem );
        else
            m_Drawings.PushFront( aBoardItem );
        break;

    case PCB_PAD_T:
        if( aMode == ADD_APPEND )
            m_Pads.PushBack( static_cast<D_PAD*>( aBoardItem ) );
        else
            m_Pads.PushFront( static_cast<D_PAD*>( aBoardItem ) );
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

    aBoardItem->SetParent( this );

    // Update relative coordinates, it can be done only after there is a parent object assigned
    switch( aBoardItem->Type() )
    {
    case PCB_MODULE_TEXT_T:
        static_cast<TEXTE_MODULE*>( aBoardItem )->SetLocalCoord();
        break;

    case PCB_MODULE_EDGE_T:
        static_cast<EDGE_MODULE*>( aBoardItem )->SetLocalCoord();
        break;

    case PCB_PAD_T:
        static_cast<D_PAD*>( aBoardItem )->SetLocalCoord();
        break;

    default:
        // Huh? It should have been filtered out by the previous switch
        assert(false);
        break;
    }
}


void MODULE::Remove( BOARD_ITEM* aBoardItem )
{
    switch( aBoardItem->Type() )
    {
    case PCB_MODULE_TEXT_T:
        // Only user texts can be removed this way. Reference and value are not hold in the DLIST.
        assert( static_cast<TEXTE_MODULE*>( aBoardItem )->GetType() == TEXTE_MODULE::TEXT_is_DIVERS );

        // no break

    case PCB_MODULE_EDGE_T:
        m_Drawings.Remove( aBoardItem );
        break;

    case PCB_PAD_T:
        m_Pads.Remove( static_cast<D_PAD*>( aBoardItem ) );
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


void MODULE::CopyNetlistSettings( MODULE* aModule, bool aCopyLocalSettings )
{
    // Don't do anything foolish like trying to copy to yourself.
    wxCHECK_RET( aModule != NULL && aModule != this, wxT( "Cannot copy to NULL or yourself." ) );

    // Not sure what to do with the value field.  Use netlist for now.
    aModule->SetPosition( GetPosition() );

    if( aModule->GetLayer() != GetLayer() )
        aModule->Flip( aModule->GetPosition() );

    if( aModule->GetOrientation() != GetOrientation() )
        aModule->Rotate( aModule->GetPosition(), GetOrientation() );

    aModule->SetLocked( IsLocked() );

    if( aCopyLocalSettings )
    {
        aModule->SetLocalSolderMaskMargin( GetLocalSolderMaskMargin() );
        aModule->SetLocalClearance( GetLocalClearance() );
        aModule->SetLocalSolderPasteMargin( GetLocalSolderPasteMargin() );
        aModule->SetLocalSolderPasteMarginRatio( GetLocalSolderPasteMarginRatio() );
        aModule->SetZoneConnection( GetZoneConnection() );
        aModule->SetThermalWidth( GetThermalWidth() );
        aModule->SetThermalGap( GetThermalGap() );
    }

    for( D_PAD* pad = aModule->Pads();  pad;  pad = pad->Next() )
    {
        // Fix me: if aCopyLocalSettings == true, for "multiple" pads
        // (set of pads having the same name/number) this is broken
        // because we copy settings from the first pad found.
        // When old and new footprints have very few differences, a better
        // algo can be used.
        D_PAD* oldPad = FindPadByName( pad->GetPadName() );

        if( oldPad )
            oldPad->CopyNetlistSettings( pad, aCopyLocalSettings );
    }

    // Not sure about copying description, keywords, 3D models or any other
    // local user changes to footprint.  Stick with the new footprint settings
    // called out in the footprint loaded in the netlist.
    aModule->CalculateBoundingBox();
}


void MODULE::Draw( EDA_DRAW_PANEL* aPanel, wxDC* aDC, GR_DRAWMODE aDrawMode,
                   const wxPoint& aOffset )
{
    if( (m_Flags & DO_NOT_DRAW) || (IsMoving()) )
        return;

    for( D_PAD* pad = m_Pads;  pad;  pad = pad->Next() )
    {
        if( pad->IsMoving() )
            continue;

        pad->Draw( aPanel, aDC, aDrawMode, aOffset );
    }

    BOARD* brd = GetBoard();

    // Draws footprint anchor
    DrawAncre( aPanel, aDC, aOffset, DIM_ANCRE_MODULE, aDrawMode );

    // Draw graphic items
    if( brd->IsElementVisible( MOD_REFERENCES_VISIBLE ) )
    {
        if( !(m_Reference->IsMoving()) )
            m_Reference->Draw( aPanel, aDC, aDrawMode, aOffset );
    }

    if( brd->IsElementVisible( MOD_VALUES_VISIBLE ) )
    {
        if( !(m_Value->IsMoving()) )
            m_Value->Draw( aPanel, aDC, aDrawMode, aOffset );
    }

    for( BOARD_ITEM* item = m_Drawings;  item;  item = item->Next() )
    {
        if( item->IsMoving() )
            continue;

        switch( item->Type() )
        {
        case PCB_MODULE_TEXT_T:
        case PCB_MODULE_EDGE_T:
            item->Draw( aPanel, aDC, aDrawMode, aOffset );
            break;

        default:
            break;
        }
    }

    // Enable these line to draw m_BoundaryBox (debug tests purposes only)
#if 0
    GRRect( aPanel->GetClipBox(), aDC, m_BoundaryBox, 0, BROWN );
#endif

}


void MODULE::DrawEdgesOnly( EDA_DRAW_PANEL* panel, wxDC* DC, const wxPoint& offset,
                            GR_DRAWMODE draw_mode )
{
    for( BOARD_ITEM* item = m_Drawings;  item;  item = item->Next() )
    {
        switch( item->Type() )
        {
        case PCB_MODULE_EDGE_T:
            item->Draw( panel, DC, draw_mode, offset );
            break;

        default:
            break;
        }
    }
}


void MODULE::CalculateBoundingBox()
{
    m_BoundaryBox = GetFootprintRect();
    m_Surface = std::abs( (double) m_BoundaryBox.GetWidth() * m_BoundaryBox.GetHeight() );
}


EDA_RECT MODULE::GetFootprintRect() const
{
    EDA_RECT area;

    area.SetOrigin( m_Pos );
    area.SetEnd( m_Pos );
    area.Inflate( Millimeter2iu( 0.25 ) );   // Give a min size to the area

    for( const BOARD_ITEM* item = m_Drawings.GetFirst(); item; item = item->Next() )
    {
        const EDGE_MODULE* edge = dyn_cast<const EDGE_MODULE*>( item );

        if( edge )
            area.Merge( edge->GetBoundingBox() );
    }

    for( D_PAD* pad = m_Pads;  pad;  pad = pad->Next() )
        area.Merge( pad->GetBoundingBox() );

    return area;
}


const EDA_RECT MODULE::GetBoundingBox() const
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


void MODULE::GetMsgPanelInfo( std::vector< MSG_PANEL_ITEM >& aList )
{
    int      nbpad;
    wxString msg;

    aList.push_back( MSG_PANEL_ITEM( m_Reference->GetShownText(), m_Value->GetShownText(), DARKCYAN ) );

    // Display last date the component was edited (useful in Module Editor).
    wxDateTime date( m_LastEditTime );

    if( m_LastEditTime && date.IsValid() )
    // Date format: see http://www.cplusplus.com/reference/ctime/strftime
        msg = date.Format( wxT( "%b %d, %Y" ) ); // Abbreviated_month_name Day, Year
    else
        msg = _( "Unknown" );

    aList.push_back( MSG_PANEL_ITEM( _( "Last Change" ), msg, BROWN ) );

    // display schematic path
    aList.push_back( MSG_PANEL_ITEM( _( "Netlist Path" ), m_Path, BROWN ) );

    // display the board side placement
    aList.push_back( MSG_PANEL_ITEM( _( "Board Side" ),
                     IsFlipped()? _( "Back (Flipped)" ) : _( "Front" ), RED ) );

    EDA_ITEM* PtStruct = m_Pads;
    nbpad = 0;

    while( PtStruct )
    {
        nbpad++;
        PtStruct = PtStruct->Next();
    }

    msg.Printf( wxT( "%d" ), nbpad );
    aList.push_back( MSG_PANEL_ITEM( _( "Pads" ), msg, BLUE ) );

    msg = wxT( ".." );

    if( IsLocked() )
        msg[0] = 'L';

    if( m_ModuleStatus & MODULE_is_PLACED )
        msg[1] = 'P';

    aList.push_back( MSG_PANEL_ITEM( _( "Status" ), msg, MAGENTA ) );

    msg.Printf( wxT( "%.1f" ), GetOrientationDegrees() );
    aList.push_back( MSG_PANEL_ITEM( _( "Rotation" ), msg, BROWN ) );

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

    aList.push_back( MSG_PANEL_ITEM( _( "Attributes" ), msg, BROWN ) );
    aList.push_back( MSG_PANEL_ITEM( _( "Footprint" ), FROM_UTF8( m_fpid.Format().c_str() ), BLUE ) );

    if( m_3D_Drawings.empty() )
        msg = _( "No 3D shape" );
    else
        msg = m_3D_Drawings.front().m_Filename;

    // Search the first active 3D shape in list

    aList.push_back( MSG_PANEL_ITEM( _( "3D-Shape" ), msg, RED ) );

    wxString doc, keyword;
    doc.Printf( _( "Doc: %s" ), GetChars( m_Doc ) );
    keyword.Printf( _( "Key Words: %s" ), GetChars( m_KeyWord ) );
    aList.push_back( MSG_PANEL_ITEM( doc, keyword, BLACK ) );
}


bool MODULE::HitTest( const wxPoint& aPosition ) const
{
    return m_BoundaryBox.Contains( aPosition );
}


bool MODULE::HitTest( const EDA_RECT& aRect, bool aContained, int aAccuracy ) const
{
    EDA_RECT arect = aRect;
    arect.Inflate( aAccuracy );

    if( aContained )
        return arect.Contains( m_BoundaryBox );
    else
        return m_BoundaryBox.Intersects( arect );
}


D_PAD* MODULE::FindPadByName( const wxString& aPadName ) const
{
    wxString buf;

    for( D_PAD* pad = m_Pads;  pad;  pad = pad->Next() )
    {
        pad->StringPadName( buf );
#if 1
        if( buf.CmpNoCase( aPadName ) == 0 )    // why case insensitive?
#else
        if( buf == aPadName )
#endif
            return pad;
    }

    return NULL;
}


D_PAD* MODULE::GetPad( const wxPoint& aPosition, LSET aLayerMask )
{
    for( D_PAD* pad = m_Pads;  pad;  pad = pad->Next() )
    {
        // ... and on the correct layer.
        if( !( pad->GetLayerSet() & aLayerMask ).any() )
            continue;

        if( pad->HitTest( aPosition ) )
            return pad;
    }

    return NULL;
}


unsigned MODULE::GetPadCount( INCLUDE_NPTH_T aIncludeNPTH ) const
{
    if( aIncludeNPTH )
        return m_Pads.GetCount();

    unsigned cnt = 0;

    for( D_PAD* pad = m_Pads; pad; pad = pad->Next() )
    {
        if( pad->GetAttribute() == PAD_ATTRIB_HOLE_NOT_PLATED )
            continue;

        cnt++;
    }

    return cnt;
}


unsigned MODULE::GetUniquePadCount( INCLUDE_NPTH_T aIncludeNPTH ) const
{
    std::set<wxUint32> usedNames;

    // Create a set of used pad numbers
    for( D_PAD* pad = Pads(); pad; pad = pad->Next() )
    {
        // Skip pads not on copper layers (used to build complex
        // solder paste shapes for instance)
        if( ( pad->GetLayerSet() & LSET::AllCuMask() ).none() )
            continue;

        // Skip pads with no name, because they are usually "mechanical"
        // pads, not "electrical" pads
        if( pad->GetPadName().IsEmpty() )
            continue;

        if( !aIncludeNPTH )
        {
            // skip NPTH
            if( pad->GetAttribute() == PAD_ATTRIB_HOLE_NOT_PLATED )
            {
                continue;
            }
        }

        usedNames.insert( pad->GetPackedPadName() );
    }

    return usedNames.size();
}


void MODULE::Add3DModel( S3D_INFO* a3DModel )
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
            result = IterateForward( m_Pads, inspector, testData, p );
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
            result = IterateForward( m_Drawings, inspector, testData, p );

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


wxString MODULE::GetSelectMenuText() const
{
    wxString text;
    text.Printf( _( "Footprint %s on %s" ),
                 GetChars ( GetReference() ),
                 GetChars ( GetLayerName() ) );

    return text;
}


BITMAP_DEF MODULE::GetMenuImage() const
{
    return module_xpm;
}


EDA_ITEM* MODULE::Clone() const
{
    return new MODULE( *this );
}


void MODULE::RunOnChildren( std::function<void (BOARD_ITEM*)> aFunction )
{
    try
    {
        for( D_PAD* pad = m_Pads; pad; pad = pad->Next() )
            aFunction( static_cast<BOARD_ITEM*>( pad ) );

        for( BOARD_ITEM* drawing = m_Drawings; drawing; drawing = drawing->Next() )
            aFunction( drawing );

        aFunction( static_cast<BOARD_ITEM*>( m_Reference ) );
        aFunction( static_cast<BOARD_ITEM*>( m_Value ) );
    }
    catch( std::bad_function_call& )
    {
        DisplayError( NULL, wxT( "Error running MODULE::RunOnChildren" ) );
    }
}

void MODULE::ViewGetLayers( int aLayers[], int& aCount ) const
{
    aCount = 2;
    aLayers[0] = ITEM_GAL_LAYER( ANCHOR_VISIBLE );

    switch( m_Layer )
    {

    default:
        wxASSERT_MSG( false, "Illegal layer" );    // do you really have modules placed on other layers?
        // pass through
    case F_Cu:
        aLayers[1] = ITEM_GAL_LAYER( MOD_FR_VISIBLE );
        break;

    case B_Cu:
        aLayers[1] = ITEM_GAL_LAYER( MOD_BK_VISIBLE );
        break;
    }
}


unsigned int MODULE::ViewGetLOD( int aLayer, KIGFX::VIEW* aView ) const
{
    int layer = ( m_Layer == F_Cu ) ? MOD_FR_VISIBLE :
                ( m_Layer == B_Cu ) ? MOD_BK_VISIBLE : ANCHOR_VISIBLE;

    // Currently it is only for anchor layer
    if( aView->IsLayerVisible( ITEM_GAL_LAYER( layer ) ) )
        return 30;

    return std::numeric_limits<unsigned int>::max();
}


const BOX2I MODULE::ViewBBox() const
{
    EDA_RECT fpRect = GetFootprintRect();

    return BOX2I( VECTOR2I( fpRect.GetOrigin() ), VECTOR2I( fpRect.GetSize() ) );
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
    static const wxChar invalidChars[] = wxT("%$\t \"\\/");
    static const wxChar invalidCharsReadable[] = wxT("% $ 'tab' 'space' \\ \" /");

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
    wxPoint newpos = m_Pos;
    RotatePoint( &newpos, aRotCentre, aAngle );
    SetPosition( newpos );
    SetOrientation( GetOrientation() + aAngle );
}


void MODULE::Flip( const wxPoint& aCentre )
{
    // Move module to its final position:
    wxPoint finalPos = m_Pos;
    MIRROR( finalPos.y, aCentre.y );     /// Mirror the Y position
    SetPosition( finalPos );

    // Flip layer
    SetLayer( FlipLayer( GetLayer() ) );

    // Reverse mirror orientation.
    m_Orient = -m_Orient;
    NORMALIZE_ANGLE_POS( m_Orient );

    // Mirror pads to other side of board about the x axis, i.e. vertically.
    for( D_PAD* pad = m_Pads; pad; pad = pad->Next() )
        pad->Flip( m_Pos );

    // Mirror reference and value.
    m_Reference->Flip( m_Pos );
    m_Value->Flip( m_Pos );

    // Reverse mirror module graphics and texts.
    for( EDA_ITEM* item = m_Drawings; item; item = item->Next() )
    {
        switch( item->Type() )
        {
        case PCB_MODULE_EDGE_T:
            ( (EDGE_MODULE*) item )->Flip( m_Pos );
            break;

        case PCB_MODULE_TEXT_T:
            static_cast<TEXTE_MODULE*>( item )->Flip( m_Pos );
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

    for( D_PAD* pad = m_Pads;  pad;  pad = pad->Next() )
    {
        pad->SetPosition( pad->GetPosition() + delta );
    }

    for( EDA_ITEM* item = m_Drawings;  item;  item = item->Next() )
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
    for( D_PAD* pad = Pads(); pad; pad = pad->Next() )
    {
        pad->SetPos0( pad->GetPos0() + moveVector );
        pad->SetDrawCoord();
    }

    // Update the draw element coordinates.
    for( EDA_ITEM* item = GraphicalItems(); item; item = item->Next() )
    {
        switch( item->Type() )
        {
        case PCB_MODULE_EDGE_T:
        {
            EDGE_MODULE* edge = static_cast<EDGE_MODULE*>( item );
            edge->m_Start0 += moveVector;
            edge->m_End0   += moveVector;
            edge->SetDrawCoord();
            break;
        }

        case PCB_MODULE_TEXT_T:
        {
            TEXTE_MODULE* text = static_cast<TEXTE_MODULE*>( item );
            text->SetPos0( text->GetPos0() + moveVector );
            text->SetDrawCoord();
            break;
        }

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

    for( D_PAD* pad = m_Pads;  pad;  pad = pad->Next() )
    {
        pad->SetOrientation( pad->GetOrientation() + angleChange );
        pad->SetDrawCoord();
    }

    // Update of the reference and value.
    m_Reference->SetDrawCoord();
    m_Value->SetDrawCoord();

    // Displace contours and text of the footprint.
    for( BOARD_ITEM* item = m_Drawings;  item;  item = item->Next() )
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
            Pads().PushBack( new_pad );

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
                GraphicalItems().PushBack( new_text );

            new_item = new_text;
        }
        break;
    }

    case PCB_MODULE_EDGE_T:
    {
        EDGE_MODULE* new_edge = new EDGE_MODULE(
                *static_cast<const EDGE_MODULE*>(aItem) );

        if( aAddToModule )
            GraphicalItems().PushBack( new_edge );

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

    if( aIncrementPadNumbers && new_pad )
    {
        new_pad->IncrementPadName( true, true );
    }

    return new_item;
}


wxString MODULE::GetNextPadName( bool aFillSequenceGaps ) const
{
    std::set<int> usedNumbers;

    // Create a set of used pad numbers
    for( D_PAD* pad = Pads(); pad; pad = pad->Next() )
    {
        int padNumber = getTrailingInt( pad->GetPadName() );
        usedNumbers.insert( padNumber );
    }

    const int nextNum = getNextNumberInSequence( usedNumbers, aFillSequenceGaps );

    return wxString::Format( wxT( "%i" ), nextNum );
}


wxString MODULE::GetReferencePrefix() const
{
    wxString prefix = GetReference();

    int strIndex = prefix.length() - 1;
    while( strIndex >= 0 )
    {
        const wxUniChar chr = prefix.GetChar( strIndex );

        // numeric suffix
        if( chr >= '0' && chr <= '9' )
            break;

        strIndex--;
    }

    prefix = prefix.Mid( 0, strIndex );

    return prefix;
}


double MODULE::PadCoverageRatio() const
{
    double padArea = 0.0;
    double moduleArea = GetFootprintRect().GetArea();

    for( D_PAD* pad = m_Pads; pad; pad = pad->Next() )
        padArea += pad->GetBoundingBox().GetArea();

    if( moduleArea == 0.0 )
        return 1.0;

    double ratio = padArea / moduleArea;

    return std::min( ratio, 1.0 );
}
