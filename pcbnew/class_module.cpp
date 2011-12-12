/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2009 Jean-Pierre Charras, jaen-pierre.charras@gipsa-lab.inpg.com
 * Copyright (C) 2011 Wayne Stambaugh <stambaughw@verizon.net>
 * Copyright (C) 1992-2011 KiCad Developers, see AUTHORS.txt for contributors.
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

#include "fctsys.h"
#include "gr_basic.h"
#include "wxstruct.h"
#include "plot_common.h"
#include "class_drawpanel.h"
#include "trigo.h"
#include "confirm.h"
#include "kicad_string.h"
#include "pcbcommon.h"
#include "pcbnew.h"
#include "colors_selection.h"
#include "richio.h"
#include "filter_reader.h"
#include "macros.h"
#include "3d_struct.h"

#include "drag.h"

#include "protos.h"
#include "class_board.h"
#include "class_edge_mod.h"
#include "class_module.h"


MODULE::MODULE( BOARD* parent ) :
    BOARD_ITEM( (BOARD_ITEM*) parent, PCB_MODULE_T )
{
    m_Attributs    = MOD_DEFAULT;
    m_Layer        = LAYER_N_FRONT;
    m_Orient       = 0;
    m_ModuleStatus = 0;
    flag = 0;
    m_CntRot90 = m_CntRot180 = 0;
    m_Surface  = 0.0;
    m_Link     = 0;
    m_LastEdit_Time  = time( NULL );
    m_LocalClearance = 0;
    m_LocalSolderMaskMargin  = 0;
    m_LocalSolderPasteMargin = 0;
    m_LocalSolderPasteMarginRatio = 0.0;

    m_Reference = new TEXTE_MODULE( this, TEXT_is_REFERENCE );

    m_Value = new TEXTE_MODULE( this, TEXT_is_VALUE );

    // Reserve one void 3D entry, to avoid problems with void list
    m_3D_Drawings.PushBack( new S3D_MASTER( this ) );
}


MODULE::~MODULE()
{
    delete m_Reference;
    delete m_Value;
}


/* Draw the anchor cross (vertical)
 * Must be done after the pads, because drawing the hole will erase overwrite
 * every thing already drawn.
 */
void MODULE::DrawAncre( EDA_DRAW_PANEL* panel, wxDC* DC, const wxPoint& offset,
                        int dim_ancre, int draw_mode )
{
    int anchor_size = DC->DeviceToLogicalXRel( dim_ancre );

    GRSetDrawMode( DC, draw_mode );

    if( GetBoard()->IsElementVisible( ANCHOR_VISIBLE ) )
    {
        int color = g_ColorsSettings.GetItemColor( ANCHOR_VISIBLE );
        GRLine( &panel->m_ClipBox, DC,
                m_Pos.x - offset.x - anchor_size, m_Pos.y - offset.y,
                m_Pos.x - offset.x + anchor_size, m_Pos.y - offset.y,
                0, color );
        GRLine( &panel->m_ClipBox, DC,
                m_Pos.x - offset.x, m_Pos.y - offset.y - anchor_size,
                m_Pos.x - offset.x, m_Pos.y - offset.y + anchor_size,
                0, color );
    }
}


void MODULE::Copy( MODULE* aModule )
{
    m_Pos           = aModule->m_Pos;
    m_Layer         = aModule->m_Layer;
    m_LibRef        = aModule->m_LibRef;
    m_Attributs     = aModule->m_Attributs;
    m_Orient        = aModule->m_Orient;
    m_BoundaryBox   = aModule->m_BoundaryBox;
    m_PadNum        = aModule->m_PadNum;
    m_CntRot90      = aModule->m_CntRot90;
    m_CntRot180     = aModule->m_CntRot180;
    m_LastEdit_Time = aModule->m_LastEdit_Time;
    m_Link          = aModule->m_Link;
    m_Path          = aModule->m_Path; //is this correct behavior?
    SetTimeStamp( GetNewTimeStamp() );

    m_LocalClearance                = aModule->m_LocalClearance;
    m_LocalSolderMaskMargin         = aModule->m_LocalSolderMaskMargin;
    m_LocalSolderPasteMargin        = aModule->m_LocalSolderPasteMargin;
    m_LocalSolderPasteMarginRatio   = aModule->m_LocalSolderPasteMarginRatio;

    /* Copy reference and value. */
    m_Reference->Copy( aModule->m_Reference );
    m_Value->Copy( aModule->m_Value );

    /* Copy auxiliary data: Pads */
    m_Pads.DeleteAll();

    for( D_PAD* pad = aModule->m_Pads;  pad;  pad = pad->Next() )
    {
        D_PAD* newpad = new D_PAD( this );
        newpad->Copy( pad );
        m_Pads.PushBack( newpad );
    }

    /* Copy auxiliary data: Drawings */
    m_Drawings.DeleteAll();

    for( BOARD_ITEM* item = aModule->m_Drawings;  item;  item = item->Next() )
    {
        switch( item->Type() )
        {
        case PCB_MODULE_TEXT_T:
            TEXTE_MODULE * textm;
            textm = new TEXTE_MODULE( this );
            textm->Copy( (TEXTE_MODULE*) item );
            m_Drawings.PushBack( textm );
            break;

        case PCB_MODULE_EDGE_T:
            EDGE_MODULE * edge;
            edge = new EDGE_MODULE( this );
            edge->Copy( (EDGE_MODULE*) item );
            m_Drawings.PushBack( edge );
            break;

        default:
            wxMessageBox( wxT( "MODULE::Copy() Internal Err:  unknown type" ) );
            break;
        }
    }

    /* Copy auxiliary data: 3D_Drawings info */
    m_3D_Drawings.DeleteAll();

    // Ensure there is one (or more) item in m_3D_Drawings
    m_3D_Drawings.PushBack( new S3D_MASTER( this ) ); // push a void item

    for( S3D_MASTER* item = aModule->m_3D_Drawings;  item;  item = item->Next() )
    {
        if( item->m_Shape3DName.IsEmpty() )           // do not copy empty shapes.
            continue;

        S3D_MASTER* t3d = m_3D_Drawings;

        if( t3d && t3d->m_Shape3DName.IsEmpty() )       // The first entry can
        {                                               // exist, but is empty : use it.
            t3d->Copy( item );
        }
        else
        {
            t3d = new S3D_MASTER( this );
            t3d->Copy( item );
            m_3D_Drawings.PushBack( t3d );
        }
    }

    m_Doc     = aModule->m_Doc;
    m_KeyWord = aModule->m_KeyWord;
}


/**
 * Function Draw
 *  Draws the footprint to the current Device Context
 * @param aPanel = draw panel, Used to know the clip box
 * @param aDC = Current Device Context
 * @param aDrawMode = GR_OR, GR_XOR..
 * @param aOffset = draw offset (usually wxPoint(0,0)
 */
void MODULE::Draw( EDA_DRAW_PANEL* aPanel, wxDC* aDC, int aDrawMode, const wxPoint& aOffset )
{
    if( (m_Flags & DO_NOT_DRAW) || (m_Flags & IS_MOVED) )
        return;

    for( D_PAD* pad = m_Pads;  pad;  pad = pad->Next() )
    {
        if( pad->m_Flags & IS_MOVED )
            continue;

        pad->Draw( aPanel, aDC, aDrawMode, aOffset );
    }

    BOARD* brd = GetBoard();

    // Draws footprint anchor
    DrawAncre( aPanel, aDC, aOffset, DIM_ANCRE_MODULE, aDrawMode );

    /* Draw graphic items */
    if( brd->IsElementVisible( MOD_REFERENCES_VISIBLE ) )
    {
        if( !(m_Reference->m_Flags & IS_MOVED) )
            m_Reference->Draw( aPanel, aDC, aDrawMode, aOffset );
    }

    if( brd->IsElementVisible( MOD_VALUES_VISIBLE ) )
    {
        if( !(m_Value->m_Flags & IS_MOVED) )
            m_Value->Draw( aPanel, aDC, aDrawMode, aOffset );
    }

    for( BOARD_ITEM* item = m_Drawings;  item;  item = item->Next() )
    {
        if( item->m_Flags & IS_MOVED )
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
    GRRect( &aPanel->m_ClipBox, aDC, m_BoundaryBox, 0, BROWN );
#endif

}


/**
 * Function DrawEdgesOnly
 *  Draws the footprint edges only to the current Device Context
 *  @param panel = The active Draw Panel (used to know the clip box)
 *  @param DC = current Device Context
 *  @param offset = draw offset (usually wxPoint(0,0)
 *  @param draw_mode =  GR_OR, GR_XOR, GR_AND
 */
void MODULE::DrawEdgesOnly( EDA_DRAW_PANEL* panel, wxDC* DC, const wxPoint& offset, int draw_mode )
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
    m_BoundaryBox = GetFootPrintRect();
    m_Surface = ABS( (double) m_BoundaryBox.GetWidth() * m_BoundaryBox.GetHeight() );
}


EDA_RECT MODULE::GetFootPrintRect() const
{
    EDA_RECT area;

    area.m_Pos = m_Pos;
    area.SetEnd( m_Pos );
    area.Inflate( 50 );       // Give a min size

    for( EDGE_MODULE* edge = (EDGE_MODULE*) m_Drawings.GetFirst(); edge; edge = edge->Next() )
        if( edge->Type() == PCB_MODULE_EDGE_T )
            area.Merge( edge->GetBoundingBox() );

    for( D_PAD* pad = m_Pads;  pad;  pad = pad->Next() )
        area.Merge( pad->GetBoundingBox() );

    return area;
}


EDA_RECT MODULE::GetBoundingBox() const
{
    EDA_RECT area = GetFootPrintRect();

    // Calculate extended area including text fields
    area.Merge( m_Reference->GetBoundingBox() );
    area.Merge( m_Value->GetBoundingBox() );

    // Add the Clearance shape size: (shape around the pads when the
    // clearance is shown.  Not optimized, but the draw cost is small
    // (perhaps smaller than optimization).
    int biggest_clearance = GetBoard()->GetBiggestClearanceValue();
    area.Inflate( biggest_clearance );

    return area;
}


/* Virtual function, from EDA_ITEM.
 * display module info on MsgPanel
 */
void MODULE::DisplayInfo( EDA_DRAW_FRAME* frame )
{
    int      nbpad;
    char     bufcar[512], Line[512];
    bool     flag = false;
    wxString msg;
    BOARD*   board = GetBoard();

    frame->EraseMsgBox();

    if( frame->IsType( PCB_FRAME ) )
        flag = true;

    frame->AppendMsgPanel( m_Reference->m_Text, m_Value->m_Text, DARKCYAN );

    if( flag ) // Display last date the component was edited( useful in Module Editor)
    {
        time_t edit_time = m_LastEdit_Time;
        strcpy( Line, ctime( &edit_time ) );
        strtok( Line, " \n\r" );
        strcpy( bufcar, strtok( NULL, " \n\r" ) ); strcat( bufcar, " " );
        strcat( bufcar, strtok( NULL, " \n\r" ) ); strcat( bufcar, ", " );
        strtok( NULL, " \n\r" );
        strcat( bufcar, strtok( NULL, " \n\r" ) );
        msg = FROM_UTF8( bufcar );
        frame->AppendMsgPanel( _( "Last Change" ), msg, BROWN );
    }
    else    // display time stamp in schematic
    {
        msg.Printf( wxT( "%8.8lX" ), m_TimeStamp );
        frame->AppendMsgPanel( _( "Netlist path" ), m_Path, BROWN );
    }

    frame->AppendMsgPanel( _( "Layer" ), board->GetLayerName( m_Layer ), RED );

    EDA_ITEM* PtStruct = m_Pads;
    nbpad = 0;

    while( PtStruct )
    {
        nbpad++;
        PtStruct = PtStruct->Next();
    }

    msg.Printf( wxT( "%d" ), nbpad );
    frame->AppendMsgPanel( _( "Pads" ), msg, BLUE );

    msg = wxT( ".." );

    if( IsLocked() )
        msg[0] = 'L';

    if( m_ModuleStatus & MODULE_is_PLACED )
        msg[1] = 'P';

    frame->AppendMsgPanel( _( "Stat" ), msg, MAGENTA );

    msg.Printf( wxT( "%.1f" ), (float) m_Orient / 10 );
    frame->AppendMsgPanel( _( "Orient" ), msg, BROWN );

    frame->AppendMsgPanel( _( "Module" ), m_LibRef, BLUE );

    if(  m_3D_Drawings != NULL )
        msg = m_3D_Drawings->m_Shape3DName;
    else
        msg = _( "No 3D shape" );

    frame->AppendMsgPanel( _( "3D-Shape" ), msg, RED );

    wxString doc     = _( "Doc:  " ) + m_Doc;
    wxString keyword = _( "KeyW: " ) + m_KeyWord;
    frame->AppendMsgPanel( doc, keyword, BLACK );
}


bool MODULE::HitTest( const wxPoint& aRefPos )
{
    if( m_BoundaryBox.Contains( aRefPos ) )
        return true;

    return false;
}


bool MODULE::HitTest( EDA_RECT& aRefArea )
{
    if( m_BoundaryBox.m_Pos.x < aRefArea.GetX() )
        return false;

    if( m_BoundaryBox.m_Pos.y < aRefArea.GetY() )
        return false;

    if( m_BoundaryBox.GetRight() > aRefArea.GetRight() )
        return false;

    if( m_BoundaryBox.GetBottom() > aRefArea.GetBottom() )
        return false;

    return true;
}


D_PAD* MODULE::FindPadByName( const wxString& aPadName ) const
{
    wxString buf;

    for( D_PAD* pad = m_Pads;  pad;  pad = pad->Next() )
    {
        pad->ReturnStringPadName( buf );
#if 1
        if( buf.CmpNoCase( aPadName ) == 0 )    // why case insensitive?
#else
        if( buf == aPadName )
#endif
            return pad;
    }

    return NULL;
}


D_PAD* MODULE::GetPad( const wxPoint& aPosition, int aLayerMask )
{
    for( D_PAD* pad = m_Pads;   pad;   pad = pad->Next() )
    {
        /* ... and on the correct layer. */
        if( ( pad->m_layerMask & aLayerMask ) == 0 )
            continue;

        if( pad->HitTest( aPosition ) )
            return pad;
    }

    return NULL;
}


// see class_module.h
SEARCH_RESULT MODULE::Visit( INSPECTOR* inspector, const void* testData,
                             const KICAD_T scanTypes[] )
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
            result = inspector->Inspect( this, testData );  // inspect me
            ++p;
            break;

        case PCB_PAD_T:
            result = IterateForward( m_Pads, inspector, testData, p );
            ++p;
            break;

        case PCB_MODULE_TEXT_T:
            result = inspector->Inspect( m_Reference, testData );

            if( result == SEARCH_QUIT )
                break;

            result = inspector->Inspect( m_Value, testData );

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

    text << _( "Footprint" ) << wxT( " " ) << GetReference();
    text << wxT( " (" ) << GetLayerName() << wxT( ")" );

    return text;
}


#if defined(DEBUG)

/**
 * Function Show
 * is used to output the object tree, currently for debugging only.
 * @param nestLevel An aid to prettier tree indenting, and is the level
 *          of nesting of this object within the overall tree.
 * @param os The ostream& to output to.
 */
void MODULE::Show( int nestLevel, std::ostream& os )
{
    BOARD* board = GetBoard();

    // for now, make it look like XML, expand on this later.
    NestedSpace( nestLevel, os ) << '<' << GetClass().Lower().mb_str() <<
    " ref=\"" << m_Reference->m_Text.mb_str() << '"' <<
    " value=\"" << m_Value->m_Text.mb_str() << '"' <<
    " layer=\"" << board->GetLayerName( m_Layer ).mb_str() << '"' <<
    ">\n";

    NestedSpace( nestLevel + 1, os ) <<
    "<boundingBox" << m_BoundaryBox.m_Pos << m_BoundaryBox.m_Size << "/>\n";

    NestedSpace( nestLevel + 1, os ) << "<orientation tenths=\"" << m_Orient
                                     << "\"/>\n";

    EDA_ITEM* p;

    NestedSpace( nestLevel + 1, os ) << "<mpads>\n";
    p = m_Pads;

    for( ; p; p = p->Next() )
        p->Show( nestLevel + 2, os );

    NestedSpace( nestLevel + 1, os ) << "</mpads>\n";

    NestedSpace( nestLevel + 1, os ) << "<mdrawings>\n";
    p = m_Drawings;

    for( ; p; p = p->Next() )
        p->Show( nestLevel + 2, os );

    NestedSpace( nestLevel + 1, os ) << "</mdrawings>\n";

    p = m_Son;

    for( ; p;  p = p->Next() )
    {
        p->Show( nestLevel + 1, os );
    }

    NestedSpace( nestLevel, os ) << "</" << GetClass().Lower().mb_str()
                                 << ">\n";
}

#endif
