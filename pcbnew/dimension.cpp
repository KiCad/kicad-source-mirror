/**
 * @file dimension.cpp
 * @brief Dialog and code for editing a dimension object.
 */

/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2016 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2012 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
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

#include <fctsys.h>
#include <confirm.h>
#include <gr_basic.h>
#include <class_drawpanel.h>
#include <pcb_edit_frame.h>
#include <draw_graphic_text.h>
#include <dialog_helpers.h>
#include <macros.h>
#include <base_units.h>
#include <board_commit.h>

#include <class_board.h>
#include <class_pcb_text.h>
#include <class_dimension.h>

#include <pcbnew.h>
#include <pcb_layer_box_selector.h>
#include <dialogs/dialog_text_properties.h>

/* Local functions */
static void BuildDimension( EDA_DRAW_PANEL* aPanel, wxDC* aDC,
                            const wxPoint& aPosition, bool aErase );

static void MoveDimensionText( EDA_DRAW_PANEL* aPanel, wxDC* aDC,
                               const wxPoint& aPosition, bool aErase );
static void AbortMoveDimensionText( EDA_DRAW_PANEL* aPanel, wxDC* aDC );


/* Local variables : */
static int status_dimension; /* Used in dimension creation:
                              * = 0 : initial value: no dimension in progress
                              * = 1 : First point created
                              * = 2 : Second point created, the text must be placed */

/*
 *  A dimension has this shape:
 *  It has 2 reference points, and a text
 * |            |
 * |    dist    |
 * |<---------->|
 * |            |
 *
 */

static void AbortBuildDimension( EDA_DRAW_PANEL* Panel, wxDC* aDC )
{
    DIMENSION* dimension = (DIMENSION*) Panel->GetScreen()->GetCurItem();

    if( dimension )
    {
        if( dimension->IsNew() )
        {
            dimension->Draw( Panel, aDC, GR_XOR );
            dimension->DeleteStructure();
        }
        else
        {
            dimension->Draw( Panel, aDC, GR_OR );
        }
    }

    status_dimension = 0;
    ((PCB_EDIT_FRAME*)Panel->GetParent())->SetCurItem( NULL );
}


DIMENSION* PCB_EDIT_FRAME::EditDimension( DIMENSION* aDimension, wxDC* aDC )
{
    wxPoint pos;

    if( aDimension == NULL )
    {
        const BOARD_DESIGN_SETTINGS& boardSettings = GetBoard()->GetDesignSettings();

        status_dimension = 1;
        pos = GetCrossHairPosition();

        aDimension = new DIMENSION( GetBoard() );
        aDimension->SetFlags( IS_NEW );
        aDimension->SetLayer( GetActiveLayer() );
        aDimension->SetOrigin( pos );
        aDimension->SetEnd( pos );

        aDimension->Text().SetTextSize( boardSettings.GetTextSize( GetActiveLayer() ) );
        aDimension->Text().SetThickness( boardSettings.GetTextThickness( GetActiveLayer() ) );
        aDimension->Text().SetItalic( boardSettings.GetTextItalic( GetActiveLayer() ) );
        aDimension->SetWidth( boardSettings.GetLineThickness( GetActiveLayer() ) );
        aDimension->AdjustDimensionDetails();
        aDimension->Draw( m_canvas, aDC, GR_XOR );

        m_canvas->SetMouseCapture( BuildDimension, AbortBuildDimension );
        return aDimension;
    }

    // Dimension != NULL
    if( status_dimension == 1 )
    {
        status_dimension = 2;
        return aDimension;
    }

    aDimension->Draw( m_canvas, aDC, GR_OR );
    aDimension->ClearFlags();

    /* ADD this new item in list */
    GetBoard()->Add( aDimension );

    // Add store it in undo/redo list
    SaveCopyInUndoList( aDimension, UR_NEW );

    OnModify();
    m_canvas->SetMouseCapture( NULL, NULL );

    return NULL;
}


static void BuildDimension( EDA_DRAW_PANEL* aPanel, wxDC* aDC,
                            const wxPoint& aPosition, bool aErase )
{
    PCB_SCREEN* screen   = (PCB_SCREEN*) aPanel->GetScreen();
    DIMENSION*  Dimension = (DIMENSION*) screen->GetCurItem();
    wxPoint     pos = aPanel->GetParent()->GetCrossHairPosition();

    if( Dimension == NULL )
        return;

    // Erase previous dimension.
    if( aErase )
    {
        Dimension->Draw( aPanel, aDC, GR_XOR );
    }

    Dimension->SetLayer( screen->m_Active_Layer );

    if( status_dimension == 1 )
    {
        Dimension->m_featureLineDO = pos;
        Dimension->m_crossBarF  = Dimension->m_featureLineDO;
        Dimension->AdjustDimensionDetails();
    }
    else
    {
        /* Calculating the direction of travel perpendicular to the selected axis. */
        double angle = Dimension->GetAngle() + (M_PI / 2);

        wxPoint delta = pos - Dimension->m_featureLineDO;
        double depl   = ( delta.x * cos( angle ) ) + ( delta.y * sin( angle ) );
        Dimension->SetHeight( depl );
    }

    Dimension->Draw( aPanel, aDC, GR_XOR );
}


void PCB_EDIT_FRAME::ShowDimensionPropertyDialog( DIMENSION* aDimension, wxDC* aDC )
{
    if( aDimension == NULL )
        return;

    DIALOG_TEXT_PROPERTIES dlg( this, aDimension, aDC );
    dlg.ShowModal();
}


void PCB_EDIT_FRAME::DeleteDimension( DIMENSION* aDimension, wxDC* aDC )
{
    if( aDimension == NULL )
        return;

    if( aDC )
        aDimension->Draw( m_canvas, aDC, GR_XOR );

    SaveCopyInUndoList( aDimension, UR_DELETED );
    aDimension->UnLink();
    OnModify();
}


/* Initialize parameters to move a pcb text
 */
static wxPoint initialTextPosition;

void PCB_EDIT_FRAME::BeginMoveDimensionText( DIMENSION* aItem, wxDC* DC )
{
    if( aItem == NULL )
        return;

    // Store the initial position for undo/abort command
    initialTextPosition = aItem->Text().GetTextPos();

    aItem->Draw( m_canvas, DC, GR_XOR );
    aItem->SetFlags( IS_MOVED );
    SetMsgPanel( aItem );

    SetCrossHairPosition( aItem->Text().GetTextPos() );
    m_canvas->MoveCursorToCrossHair();

    m_canvas->SetMouseCapture( MoveDimensionText, AbortMoveDimensionText );
    SetCurItem( aItem );
    m_canvas->CallMouseCapture( DC, wxDefaultPosition, false );
}


/* Move dimension text following the cursor. */
static void MoveDimensionText( EDA_DRAW_PANEL* aPanel, wxDC* aDC, const wxPoint& aPosition,
                               bool aErase )
{
    DIMENSION* dimension = (DIMENSION*) aPanel->GetScreen()->GetCurItem();

    if( dimension == NULL )
        return;

    if( aErase )
        dimension->Draw( aPanel, aDC, GR_XOR );

    dimension->Text().SetTextPos( aPanel->GetParent()->GetCrossHairPosition() );

    dimension->Draw( aPanel, aDC, GR_XOR );
}


/*
 * Abort current text edit progress.
 *
 * If a text is selected, its initial coord are regenerated
 */
void AbortMoveDimensionText( EDA_DRAW_PANEL* aPanel, wxDC* aDC )
{
    DIMENSION* dimension = (DIMENSION*) aPanel->GetScreen()->GetCurItem();
    ( (PCB_EDIT_FRAME*) aPanel->GetParent() )->SetCurItem( NULL );

    aPanel->SetMouseCapture( NULL, NULL );

    if( dimension == NULL )  // Should not occur
        return;

    dimension->Draw( aPanel, aDC, GR_XOR );
    dimension->Text().SetTextPos( initialTextPosition );
    dimension->ClearFlags();
    dimension->Draw( aPanel, aDC, GR_OR );
}


/*
 *  Place the current dimension text being moving
 */
void PCB_EDIT_FRAME::PlaceDimensionText( DIMENSION* aItem, wxDC* DC )
{
    m_canvas->SetMouseCapture( NULL, NULL );
    SetCurItem( NULL );

    if( aItem == NULL )
        return;

    aItem->Draw( m_canvas, DC, GR_OR );
    OnModify();

    wxPoint tmp = aItem->Text().GetTextPos();
    aItem->Text().SetTextPos( initialTextPosition );
    SaveCopyInUndoList( aItem, UR_CHANGED );
    aItem->Text().SetTextPos( tmp );
    aItem->ClearFlags();
}
