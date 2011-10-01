/**
 * @file edtxtmod.cpp
 * @brief Edit module text.
 */

#include "fctsys.h"
#include "gr_basic.h"
#include "common.h"
#include "class_drawpanel.h"
#include "drawtxt.h"
#include "trigo.h"
#include "wxBasePcbFrame.h"
#include "macros.h"

#include "pcbnew.h"
#include "protos.h"

#include "class_board.h"
#include "class_module.h"
#include "class_text_mod.h"
#include "class_pcb_text.h"


static void Show_MoveTexte_Module( EDA_DRAW_PANEL* aPanel, wxDC* aDC, const wxPoint& aPosition,
                                   bool aErase );
static void AbortMoveTextModule( EDA_DRAW_PANEL* Panel, wxDC* DC );


wxPoint        MoveVector;              // Move vector for move edge, exported
                                        // to dialog_edit mod_text.cpp
static wxPoint TextInitialPosition;     // Mouse cursor initial position for
                                        // undo/abort move command
static int     TextInitialOrientation;  // module text initial orientation for
                                        // undo/abort move+rot command+rot


/* Add a new graphical text to the active module (footprint)
 *  Note there always are 2 texts: reference and value.
 *  New texts have the member TEXTE_MODULE.m_Type set to TEXT_is_DIVERS
 */
TEXTE_MODULE* PCB_BASE_FRAME::CreateTextModule( MODULE* Module, wxDC* DC )
{
    TEXTE_MODULE* Text;

    Text = new TEXTE_MODULE( Module );

    /* Add the new text object to the beginning of the draw item list. */
    if( Module )
        Module->m_Drawings.PushFront( Text );

    Text->m_Flags = IS_NEW;

    Text->m_Text = wxT( "text" );

    g_ModuleTextWidth = Clamp_Text_PenSize( g_ModuleTextWidth,
                                            MIN( g_ModuleTextSize.x, g_ModuleTextSize.y ), true );
    Text->m_Size  = g_ModuleTextSize;
    Text->m_Thickness = g_ModuleTextWidth;
    Text->m_Pos   = GetScreen()->GetCrossHairPosition();
    Text->SetLocalCoord();

    InstallTextModOptionsFrame( Text, NULL );
    DrawPanel->MoveCursorToCrossHair();

    Text->m_Flags = 0;

    if( DC )
        Text->Draw( DrawPanel, DC, GR_OR );

    Text->DisplayInfo( this );

    return Text;
}


/* Rotate text 90 degrees.
 */
void PCB_BASE_FRAME::RotateTextModule( TEXTE_MODULE* Text, wxDC* DC )
{
    if( Text == NULL )
        return;

    MODULE* module = (MODULE*) Text->GetParent();

    if( module && module->m_Flags == 0 && Text->m_Flags == 0 ) // prepare undo command
    {
        if( this->m_Ident == PCB_FRAME )
            SaveCopyInUndoList( module, UR_CHANGED );
    }

    // we expect MoveVector to be (0,0) if there is no move in progress
    Text->Draw( DrawPanel, DC, GR_XOR, MoveVector );

    Text->m_Orient += 900;

    while( Text->m_Orient >= 1800 )
        Text->m_Orient -= 1800;

    Text->Draw( DrawPanel, DC, GR_XOR, MoveVector );
    Text->DisplayInfo( this );

    if( module )
        module->m_LastEdit_Time = time( NULL );

    OnModify();
}


/*
 * Deletes text in module (if not the reference or value)
 */
void PCB_BASE_FRAME::DeleteTextModule( TEXTE_MODULE* Text )
{
    MODULE* Module;

    if( Text == NULL )
        return;

    Module = (MODULE*) Text->GetParent();

    if( Text->m_Type == TEXT_is_DIVERS )
    {
        DrawPanel->RefreshDrawingRect( Text->GetBoundingBox() );
        Text->DeleteStructure();
        OnModify();
        Module->m_LastEdit_Time = time( NULL );
    }
}


/*
 * Abort text move in progress.
 *
 * If a text is selected, its initial coordinates are regenerated.
 */
static void AbortMoveTextModule( EDA_DRAW_PANEL* Panel, wxDC* DC )
{
    BASE_SCREEN*  screen = Panel->GetScreen();
    TEXTE_MODULE* Text   = (TEXTE_MODULE*) screen->GetCurItem();
    MODULE*       Module;

    Panel->SetMouseCapture( NULL, NULL );

    if( Text == NULL )
        return;

    Module = (MODULE*) Text->GetParent();

    Text->DrawUmbilical( Panel, DC, GR_XOR, -MoveVector );
    Text->Draw( Panel, DC, GR_XOR, MoveVector );

    // If the text was moved (the move does not change internal data)
    // it could be rotated while moving. So set old value for orientation
    if( (Text->m_Flags & IS_MOVED) )
        Text->m_Orient = TextInitialOrientation;

    /* Redraw the text */
    Panel->RefreshDrawingRect( Text->GetBoundingBox() );

    // leave it at (0,0) so we can use it Rotate when not moving.
    MoveVector.x = MoveVector.y = 0;

    Text->m_Flags   = 0;
    Module->m_Flags = 0;

    screen->SetCurItem( NULL );
}


/* Start a text move.
 */
void PCB_BASE_FRAME::StartMoveTexteModule( TEXTE_MODULE* Text, wxDC* DC )
{
    MODULE* Module;

    if( Text == NULL )
        return;

    Module = (MODULE*) Text->GetParent();

    Text->m_Flags   |= IS_MOVED;
    Module->m_Flags |= IN_EDIT;

    MoveVector.x = MoveVector.y = 0;

    TextInitialPosition    = Text->m_Pos;
    TextInitialOrientation = Text->m_Orient;

    // Center cursor on initial position of text
    GetScreen()->SetCrossHairPosition( TextInitialPosition );
    DrawPanel->MoveCursorToCrossHair();

    Text->DisplayInfo( this );

    SetCurItem( Text );
    DrawPanel->SetMouseCapture( Show_MoveTexte_Module, AbortMoveTextModule );
    DrawPanel->m_mouseCaptureCallback( DrawPanel, DC, wxDefaultPosition, true );
}


/* Place the text a the cursor position when the left mouse button is clicked.
 */
void PCB_BASE_FRAME::PlaceTexteModule( TEXTE_MODULE* Text, wxDC* DC )
{
    if( Text != NULL )
    {
        DrawPanel->RefreshDrawingRect( Text->GetBoundingBox() );
        Text->DrawUmbilical( DrawPanel, DC, GR_XOR, -MoveVector );

        /* Update the coordinates for anchor. */
        MODULE* Module = (MODULE*) Text->GetParent();

        if( Module )
        {
            // Prepare undo command (a rotation can be made while moving)
            EXCHG( Text->m_Orient, TextInitialOrientation );

            if( m_Ident == PCB_FRAME )
                SaveCopyInUndoList( Module, UR_CHANGED );
            else
                SaveCopyInUndoList( Module, UR_MODEDIT );

            EXCHG( Text->m_Orient, TextInitialOrientation );

            // Set the new position for text.
            Text->m_Pos = GetScreen()->GetCrossHairPosition();
            wxPoint textRelPos = Text->m_Pos - Module->m_Pos;
            RotatePoint( &textRelPos, -Module->m_Orient );
            Text->m_Pos0    = textRelPos;
            Text->m_Flags   = 0;
            Module->m_Flags = 0;
            Module->m_LastEdit_Time = time( NULL );
            OnModify();

            /* Redraw text. */
            DrawPanel->RefreshDrawingRect( Text->GetBoundingBox() );
        }
        else
        {
            Text->m_Pos = GetScreen()->GetCrossHairPosition();
        }
    }

    // leave it at (0,0) so we can use it Rotate when not moving.
    MoveVector.x = MoveVector.y = 0;

    DrawPanel->SetMouseCapture( NULL, NULL );
}


static void Show_MoveTexte_Module( EDA_DRAW_PANEL* aPanel, wxDC* aDC, const wxPoint& aPosition,
                                   bool aErase )
{
    BASE_SCREEN*  screen = aPanel->GetScreen();
    TEXTE_MODULE* Text   = (TEXTE_MODULE*) screen->GetCurItem();

    if( Text == NULL )
        return;

    // Erase umbilical and text if necessary
    if( aErase )
    {
        Text->DrawUmbilical( aPanel, aDC, GR_XOR, -MoveVector );
        Text->Draw( aPanel, aDC, GR_XOR, MoveVector );
    }

    MoveVector = TextInitialPosition - screen->GetCrossHairPosition();

    // Draw umbilical if text moved
    if( MoveVector.x || MoveVector.y )
        Text->DrawUmbilical( aPanel, aDC, GR_XOR, -MoveVector );

    // Redraw text
    Text->Draw( aPanel, aDC, GR_XOR, MoveVector );
}

void PCB_BASE_FRAME::ResetTextSize( BOARD_ITEM* aItem, wxDC* aDC )
{
    wxSize newSize;
    int newThickness;
    TEXTE_PCB* pcbText = NULL;
    TEXTE_MODULE* moduleText = NULL;
    EDA_TEXT* text;

    switch( aItem->Type() )
    {
    case PCB_TEXT_T:
        newSize = GetBoard()->GetBoardDesignSettings()->m_PcbTextSize;
        newThickness = GetBoard()->GetBoardDesignSettings()->m_PcbTextWidth;
        pcbText = (TEXTE_PCB*) aItem;
        text = (EDA_TEXT*) pcbText;
        break;

    case PCB_MODULE_TEXT_T:
        newSize = g_ModuleTextSize;
        newThickness = g_ModuleTextWidth;
        moduleText = (TEXTE_MODULE*) aItem;
        text = (EDA_TEXT*) moduleText;
        break;

    default:
        // Exit if aItem is not a text field
        return;
        break;
    }

    // Exit if there's nothing to do
    if( text->GetSize() == newSize && text->GetThickness() == newThickness )
        return;

    // Push item to undo list
    switch( aItem->Type() )
    {
    case PCB_TEXT_T:
        SaveCopyInUndoList( pcbText, UR_CHANGED );
        break;

    case PCB_MODULE_TEXT_T:
        SaveCopyInUndoList( moduleText->GetParent(), UR_CHANGED );
        break;

    default:
        break;
    }

    // Apply changes
    text->SetSize( newSize );
    text->SetThickness( newThickness );

    if( aDC )
        DrawPanel->Refresh();

    OnModify();
}

void PCB_BASE_FRAME::ResetModuleTextSizes( int aType, wxDC* aDC )
{
    MODULE* module;
    BOARD_ITEM* boardItem;
    TEXTE_MODULE* item;
    ITEM_PICKER itemWrapper( NULL, UR_CHANGED );
    PICKED_ITEMS_LIST undoItemList;
    unsigned int ii;

    itemWrapper.m_PickedItemType = PCB_MODULE_T;

    module = GetBoard()->m_Modules;

    // Prepare undo list
    while( module )
    {
        itemWrapper.m_PickedItem = module;

        switch( aType )
        {
        case TEXT_is_REFERENCE:
            item = module->m_Reference;

            if( item->GetSize() != g_ModuleTextSize || item->GetThickness() != g_ModuleTextWidth )
                undoItemList.PushItem( itemWrapper );

            break;

        case TEXT_is_VALUE:
            item = module->m_Value;

            if( item->GetSize() != g_ModuleTextSize || item->GetThickness() != g_ModuleTextWidth )
                undoItemList.PushItem( itemWrapper );

            break;

        case TEXT_is_DIVERS:
            // Go through all other module text fields
            for( boardItem = module->m_Drawings; boardItem; boardItem = boardItem->Next() )
            {
                if( boardItem->Type() == PCB_MODULE_TEXT_T )
                {
                    item = (TEXTE_MODULE*) boardItem;

                    if( item->GetSize() != g_ModuleTextSize
                        || item->GetThickness() != g_ModuleTextWidth )
                    {
                        undoItemList.PushItem( itemWrapper );
                        break;
                    }
                }
            }

            break;

        default:
            break;
        }
        module = module->Next();
    }

    // Exit if there's nothing to do
    if( !undoItemList.GetCount() )
        return;

    SaveCopyInUndoList( undoItemList, UR_CHANGED );

    // Apply changes to modules in the undo list
    for( ii = 0; ii < undoItemList.GetCount(); ii++ )
    {
        module = (MODULE*) undoItemList.GetPickedItem( ii );

        switch( aType )
        {
        case TEXT_is_REFERENCE:
            module->m_Reference->SetThickness( g_ModuleTextWidth );
            module->m_Reference->SetSize( g_ModuleTextSize );
            break;

        case TEXT_is_VALUE:
            module->m_Value->SetThickness( g_ModuleTextWidth );
            module->m_Value->SetSize( g_ModuleTextSize );
            break;

        case TEXT_is_DIVERS:
            for( boardItem = module->m_Drawings; boardItem; boardItem = boardItem->Next() )
            {
                if( boardItem->Type() == PCB_MODULE_TEXT_T )
                {
                    item = (TEXTE_MODULE*) boardItem;
                    item->SetThickness( g_ModuleTextWidth );
                    item->SetSize( g_ModuleTextSize );
                }
            }

            break;
        }
    }

    if( aDC )
        DrawPanel->Refresh();

    OnModify();
}
