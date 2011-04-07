/*
 * This program source code file is part of KICAD, a free EDA CAD application.
 *
 * Copyright (C) 2004-2010 Jean-Pierre Charras, jean-pierre.charras@gpisa-lab.inpg.fr
 * Copyright (C) 2010 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright (C) 2010 Kicad Developers, see change_log.txt for contributors.
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


/******************************************************/
/* class_pcb_layer_widget.cpp - Pcbnew layers manager */
/******************************************************/

#include "fctsys.h"
#include "appl_wxstruct.h"
#include "common.h"
#include "class_drawpanel.h"
#include "confirm.h"
#include "pcbnew.h"
#include "wxPcbStruct.h"
#include "pcbstruct.h"      // enum PCB_VISIBLE
#include "collectors.h"
#include "bitmaps.h"
#include "pcbnew_id.h"
#include "layer_widget.h"
#include "class_pcb_layer_widget.h"


/**
 * Class PCB_LAYER_WIDGET
 * is here to implement the abtract functions of LAYER_WIDGET so they
 * may be tied into the PCB_EDIT_FRAME's data and so we can add a popup
 * menu which is specific to PCBNEW's needs.
 */


PCB_LAYER_WIDGET::PCB_LAYER_WIDGET( PCB_EDIT_FRAME* aParent, wxWindow* aFocusOwner, int aPointSize ) :
    LAYER_WIDGET( aParent, aFocusOwner, aPointSize ),
    myframe( aParent )
{
    ReFillRender();

    // Update default tabs labels for gerbview
    SetLayersManagerTabsText( );

    //-----<Popup menu>-------------------------------------------------
    // handle the popup menu over the layer window.
    m_LayerScrolledWindow->Connect( wxEVT_RIGHT_DOWN,
        wxMouseEventHandler( PCB_LAYER_WIDGET::onRightDownLayers ), NULL, this );

    // since Popupmenu() calls this->ProcessEvent() we must call this->Connect()
    // and not m_LayerScrolledWindow->Connect()
    Connect( ID_SHOW_ALL_COPPERS, ID_SHOW_NO_COPPERS, wxEVT_COMMAND_MENU_SELECTED,
        wxCommandEventHandler( PCB_LAYER_WIDGET::onPopupSelection ), NULL, this );

    // install the right click handler into each control at end of ReFill()
    // using installRightLayerClickHandler
}


void PCB_LAYER_WIDGET::installRightLayerClickHandler()
{
    int rowCount = GetLayerRowCount();
    for( int row=0;  row<rowCount;  ++row )
    {
        for( int col=0; col<LYR_COLUMN_COUNT;  ++col )
        {
            wxWindow* w = getLayerComp( row, col );

            w->Connect( wxEVT_RIGHT_DOWN, wxMouseEventHandler(
                PCB_LAYER_WIDGET::onRightDownLayers ), NULL, this );
        }
    }
}


void PCB_LAYER_WIDGET::onRightDownLayers( wxMouseEvent& event )
{
    wxMenu          menu;

    // menu text is capitalized:
    // http://library.gnome.org/devel/hig-book/2.20/design-text-labels.html.en#layout-capitalization
    menu.Append( new wxMenuItem( &menu, ID_SHOW_ALL_COPPERS,
        _("Show All Copper Layers") ) );

    menu.Append( new wxMenuItem( &menu, ID_SHOW_NO_COPPERS,
        _( "Hide All Copper Layers" ) ) );

    PopupMenu( &menu );

    passOnFocus();
}

void PCB_LAYER_WIDGET::onPopupSelection( wxCommandEvent& event )
{
    int     rowCount;
    int     menuId = event.GetId();
    bool    visible;

    switch( menuId )
    {
    case ID_SHOW_ALL_COPPERS:
        visible = true;
        goto L_change_coppers;

    case ID_SHOW_NO_COPPERS:
        visible = false;
    L_change_coppers:
        int lastCu = -1;
        rowCount = GetLayerRowCount();
        for( int row=rowCount-1;  row>=0;  --row )
        {
            wxCheckBox* cb = (wxCheckBox*) getLayerComp( row, 3 );
            int layer = getDecodedId( cb->GetId() );
            if( IsValidCopperLayerIndex( layer ) )
            {
                lastCu = row;
                break;
            }
        }

        for( int row=0;  row<rowCount;  ++row )
        {
            wxCheckBox* cb = (wxCheckBox*) getLayerComp( row, 3 );
            int layer = getDecodedId( cb->GetId() );

            if( IsValidCopperLayerIndex( layer ) )
            {
                cb->SetValue( visible );

                bool isLastCopperLayer = (row==lastCu);

                OnLayerVisible( layer, visible, isLastCopperLayer );

                if( isLastCopperLayer )
                    break;
            }
        }
        break;
    }
}

/**
 * Function SetLayersManagerTabsText
 * Update the layer manager tabs labels
 * Useful when changing Language or to set labels to a non default value
 */
void PCB_LAYER_WIDGET::SetLayersManagerTabsText( )
{
    m_notebook->SetPageText(0, _("Layer") );
    m_notebook->SetPageText(1, _("Render") );
}

/**
 * Function ReFillRender
 * Rebuild Render for instance after the config is read
 */
void PCB_LAYER_WIDGET::ReFillRender()
{
    BOARD*  board = myframe->GetBoard();
    ClearRenderRows();
    // Fixed "Rendering" tab rows within the LAYER_WIDGET, only the initial color
    // is changed before appending to the LAYER_WIDGET.  This is an automatic variable
    // not a static variable, change the color & state after copying from code to renderRows
    // on the stack.
    LAYER_WIDGET::ROW renderRows[16] = {

#define RR  LAYER_WIDGET::ROW   // Render Row abreviation to reduce source width

             // text                id                      color       tooltip                 checked
        RR( _( "Through Via" ),     VIA_THROUGH_VISIBLE,    WHITE,      _( "Show through vias" ) ),
        RR( _( "Bl/Buried Via" ),   VIA_BBLIND_VISIBLE,     WHITE,      _( "Show blind or buried vias" )  ),
        RR( _( "Micro Via" ),       VIA_MICROVIA_VISIBLE,   WHITE,      _( "Show micro vias") ),
        RR( _( "Ratsnest" ),        RATSNEST_VISIBLE,       WHITE,      _( "Show unconnected nets as a ratsnest") ),

        RR( _( "Pads Front" ),      PAD_FR_VISIBLE,         WHITE,      _( "Show footprint pads on board's front" ) ),
        RR( _( "Pads Back" ),       PAD_BK_VISIBLE,         WHITE,      _( "Show footprint pads on board's back" ) ),

        RR( _( "Text Front" ),      MOD_TEXT_FR_VISIBLE,    WHITE,      _( "Show footprint text on board's back" ) ),
        RR( _( "Text Back" ),       MOD_TEXT_BK_VISIBLE,    WHITE,      _( "Show footprint text on board's back" ) ),
        RR( _( "Hidden Text" ),     MOD_TEXT_INVISIBLE,     WHITE,      _( "Show footprint text marked as invisible" ) ),

        RR( _( "Anchors" ),         ANCHOR_VISIBLE,         WHITE,      _( "Show footprint and text origins as a cross" ) ),
        RR( _( "Grid" ),            GRID_VISIBLE,           WHITE,      _( "Show the (x,y) grid dots" ) ),
        RR( _( "No-Connects" ),     NO_CONNECTS_VISIBLE,    -1,         _( "Show a marker on pads which have no net connected" ) ),
        RR( _( "Modules Front" ),   MOD_FR_VISIBLE,         -1,         _( "Show footprints that are on board's front") ),
        RR( _( "Modules Back" ),    MOD_BK_VISIBLE,         -1,         _( "Show footprints that are on board's back") ),
        RR( _( "Values" ),          MOD_VALUES_VISIBLE,     -1,         _( "Show footprint's values") ),
        RR( _( "References" ),      MOD_REFERENCES_VISIBLE, -1,         _( "Show footprint's references") ),
    };

    for( unsigned row=0;  row<DIM(renderRows);  ++row )
    {
        if( renderRows[row].color != -1 )       // does this row show a color?
        {
            // this window frame must have an established BOARD, i.e. after SetBoard()
            renderRows[row].color = board->GetVisibleElementColor( renderRows[row].id );
        }
        renderRows[row].state = board->IsElementVisible( renderRows[row].id );
    }

    AppendRenderRows( renderRows, DIM(renderRows) );
}

void PCB_LAYER_WIDGET::ReFill()
{
    BOARD*  brd = myframe->GetBoard();
    int     layer;

    int enabledLayers = brd->GetEnabledLayers();

//    m_Layers->Freeze();     // no screen updates until done modifying

    ClearLayerRows();

    // show all coppers first, with front on top, back on bottom, then technical layers

    layer = LAYER_N_FRONT;
    if( enabledLayers & (1 << layer) )
    {
        AppendLayerRow( LAYER_WIDGET::ROW(
            brd->GetLayerName( layer ), layer, brd->GetLayerColor( layer ), _("Front copper layer"), true ) );
    }

    for( layer = LAYER_N_FRONT-1;  layer >= 1;  --layer )
    {
        if( enabledLayers & (1 << layer) )
        {
            AppendLayerRow( LAYER_WIDGET::ROW(
                brd->GetLayerName( layer ), layer, brd->GetLayerColor( layer ), _("An innner copper layer"), true ) );
        }
    }

    layer = LAYER_N_BACK;
    if( enabledLayers & (1 << layer) )
    {
        AppendLayerRow( LAYER_WIDGET::ROW(
            brd->GetLayerName( layer ), layer, brd->GetLayerColor( layer ), _("Back copper layer"), true ) );
    }

    // technical layers are shown in this order:
    static const struct {
        int         layerId;
        wxString    tooltip;
    } techLayerSeq[] = {
        { ADHESIVE_N_FRONT,     _( "Adhesive on board's front" )    },
        { ADHESIVE_N_BACK,      _( "Adhesive on board's back" )     },
        { SOLDERPASTE_N_FRONT,  _( "Solder paste on board's front" )},
        { SOLDERPASTE_N_BACK,   _( "Solder paste on board's back" ) },
        { SILKSCREEN_N_FRONT,   _( "Silkscreen on board's front" )  },
        { SILKSCREEN_N_BACK,    _( "Silkscreen on board's back" )   },
        { SOLDERMASK_N_FRONT,   _( "Solder mask on board's front" ) },
        { SOLDERMASK_N_BACK,    _( "Solder mask on board's back" )  },
        { DRAW_N,               _( "Explanatory drawings" )         },
        { COMMENT_N,            _( "Explanatory comments" )         },
        { ECO1_N,               _( "TDB" )                          },
        { ECO2_N,               _( "TBD" )                          },
        { EDGE_N,               _( "Board's perimeter definition" ) },
    };

    for( unsigned i=0;  i<DIM(techLayerSeq);  ++i )
    {
        layer = techLayerSeq[i].layerId;

        if( !(enabledLayers & (1 << layer)) )
            continue;

        AppendLayerRow( LAYER_WIDGET::ROW(
            brd->GetLayerName( layer ), layer, brd->GetLayerColor( layer ),
            techLayerSeq[i].tooltip, true ) );
    }

    installRightLayerClickHandler();

//    m_Layers->Thaw();
}

//-----<LAYER_WIDGET callbacks>-------------------------------------------

void PCB_LAYER_WIDGET::OnLayerColorChange( int aLayer, int aColor )
{
    myframe->GetBoard()->SetLayerColor( aLayer, aColor );
    myframe->ReCreateLayerBox( NULL );
    myframe->DrawPanel->Refresh();
}

bool PCB_LAYER_WIDGET::OnLayerSelect( int aLayer )
{
    // the layer change from the PCB_LAYER_WIDGET can be denied by returning
    // false from this function.
    myframe->setActiveLayer( aLayer, false );

    if(DisplayOpt.ContrastModeDisplay)
        myframe->DrawPanel->Refresh();

    return true;
}

void PCB_LAYER_WIDGET::OnLayerVisible( int aLayer, bool isVisible, bool isFinal )
{
    BOARD* brd = myframe->GetBoard();

    int visibleLayers = brd->GetVisibleLayers();

    if( isVisible )
        visibleLayers |= (1 << aLayer);
    else
        visibleLayers &= ~(1 << aLayer);

    brd->SetVisibleLayers( visibleLayers );

    if( isFinal )
        myframe->DrawPanel->Refresh();
}

void PCB_LAYER_WIDGET::OnRenderColorChange( int aId, int aColor )
{
    myframe->GetBoard()->SetVisibleElementColor( aId, aColor );
    myframe->DrawPanel->Refresh();
}

void PCB_LAYER_WIDGET::OnRenderEnable( int aId, bool isEnabled )
{
    BOARD*  brd = myframe->GetBoard();

    /* @todo:

        move:

        GRID_VISIBLE,   ? maybe not this one
        into m_VisibleElements and get rid of globals.
   */

    switch( aId )
    {
        // see todo above, don't really want anything except IsElementVisible() here.

    case GRID_VISIBLE:
        // @todo, make read/write accessors for grid control so the write accessor can fire updates to
        // grid state listeners.  I think the grid state should be kept in the BOARD.
        brd->SetElementVisibility( aId, isEnabled );    // set visibilty flag also in list, and myframe->m_Draw_Grid
        break;

    default:
        brd->SetElementVisibility( aId, isEnabled );
    }

    myframe->DrawPanel->Refresh();
}

//-----</LAYER_WIDGET callbacks>------------------------------------------


