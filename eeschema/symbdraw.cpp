/*********************************************************************/
/*          EESchema - symbdraw.cpp                                  */
/* Create, move .. graphic shapes used to build and draw a component */
/* (lines, arcs ..                                                   */
/*********************************************************************/

#include "fctsys.h"
#include "gr_basic.h"
#include "class_drawpanel.h"
#include "confirm.h"

#include "eeschema_id.h"
#include "program.h"
#include "general.h"
#include "libeditframe.h"
#include "class_libentry.h"
#include "dialog_lib_edit_draw_item.h"
#include "lib_arc.h"
#include "lib_circle.h"
#include "lib_polyline.h"
#include "lib_rectangle.h"
#include "lib_text.h"


static void SymbolDisplayDraw( WinEDA_DrawPanel* panel, wxDC* DC, bool erase );
static void RedrawWhileMovingCursor( WinEDA_DrawPanel* panel, wxDC* DC, bool erase );


/*
 * Show the dialog box for editing a graphical item properties
 */
void WinEDA_LibeditFrame::EditGraphicSymbol( wxDC* DC, LIB_DRAW_ITEM* DrawItem )
{
    if( DrawItem == NULL )
        return;

    LIB_COMPONENT* component = DrawItem->GetParent();

    DIALOG_LIB_EDIT_DRAW_ITEM dialog( this, DrawItem->m_typeName );

    dialog.SetWidthUnits( ReturnUnitSymbol( g_UserUnit ) );

    wxString val = ReturnStringFromValue( g_UserUnit, m_drawLineWidth, m_InternalUnits );
    dialog.SetWidth( val );
    dialog.SetApplyToAllUnits( !m_drawSpecificUnit );
    dialog.EnableApplyToAllUnits( component && component->GetPartCount() > 1 );
    dialog.SetApplyToAllConversions( !m_drawSpecificConvert );
    dialog.EnableApplyToAllConversions( component && component->HasConversion() );
//    dialog.SetFillStyle( m_drawFillStyle );   // could better to show the current setting
    dialog.SetFillStyle( DrawItem->m_Fill);
    dialog.EnableFillStyle( DrawItem->IsFillable() );

    if( dialog.ShowModal() == wxID_CANCEL )
        return;

    val = dialog.GetWidth();
    m_drawLineWidth = ReturnValueFromString( g_UserUnit, val, m_InternalUnits );
    m_drawSpecificConvert = !dialog.GetApplyToAllConversions();
    m_drawSpecificUnit    = !dialog.GetApplyToAllUnits();

#if 0
    /* TODO: see if m_drawFillStyle must retain the last fill option or not.
     * if the last is Filled, having next new graphic items created
     * with filled body is often bad.
     * currently m_drawFillStyle is left with the default value (not filled)
     */
    if( DrawItem->IsFillable() )
        m_drawFillStyle = (FILL_T) dialog.GetFillStyle();
#endif

    // Save copy for undo if not in edit (edit command already handle the save copy)
    if( !DrawItem->InEditMode() )
        SaveCopyInUndoList( DrawItem->GetParent() );

    if( m_drawSpecificUnit )
        DrawItem->m_Unit = GetUnit();
    else
        DrawItem->m_Unit = 0;

    if( m_drawSpecificConvert )
        DrawItem->m_Convert = GetConvert();
    else
        DrawItem->m_Convert = 0;

    if( DrawItem->IsFillable() )
        DrawItem->m_Fill = (FILL_T) dialog.GetFillStyle();

    DrawItem->SetWidth( m_drawLineWidth );

    if( component )
        component->GetDrawItemList().sort();
    OnModify( );

    DrawItem->DisplayInfo( this );
    DrawPanel->Refresh();
}


static void AbortSymbolTraceOn( WinEDA_DrawPanel* Panel, wxDC* DC )
{
    WinEDA_LibeditFrame* parent = (WinEDA_LibeditFrame*) Panel->GetParent();
    LIB_DRAW_ITEM*       item   = parent->GetDrawItem();

    if( item == NULL )
        return;

    Panel->ManageCurseur  = NULL;
    Panel->ForceCloseManageCurseur = NULL;

    bool newItem = item->IsNew();
    item->EndEdit( parent->GetScreen()->GetCursorDrawPosition(), true );

    if( newItem )
    {
        delete item;
    }
    else
        parent->RestoreComponent();

    parent->SetDrawItem( NULL );
    Panel->Refresh();
}


LIB_DRAW_ITEM* WinEDA_LibeditFrame::CreateGraphicItem( LIB_COMPONENT* LibEntry, wxDC* DC )
{
    DrawPanel->ManageCurseur = SymbolDisplayDraw;
    DrawPanel->ForceCloseManageCurseur = AbortSymbolTraceOn;
    wxPoint drawPos = GetScreen()->GetCursorDrawPosition();

    // no temp copy -> the current version of component will be used for Undo
    // This is normal when adding new items to the current component
    ClearTempCopyComponent();

    switch( m_ID_current_state )
    {
    case ID_LIBEDIT_BODY_ARC_BUTT:
    {
        m_drawItem = new LIB_ARC( LibEntry );
        break;
    }
    case ID_LIBEDIT_BODY_CIRCLE_BUTT:
        m_drawItem = new LIB_CIRCLE( LibEntry );
        break;

    case ID_LIBEDIT_BODY_RECT_BUTT:
        m_drawItem = new LIB_RECTANGLE( LibEntry );
        break;

    case ID_LIBEDIT_BODY_LINE_BUTT:
        m_drawItem = new LIB_POLYLINE( LibEntry );
        break;

    case ID_LIBEDIT_BODY_TEXT_BUTT:
    {
        LIB_TEXT* Text = new LIB_TEXT( LibEntry );
        Text->m_Size.x = Text->m_Size.y = m_textSize;
        Text->m_Orient = m_textOrientation;

        // Enter the graphic text info
        DrawPanel->m_IgnoreMouseEvents = true;
        EditSymbolText( NULL, Text );
        DrawPanel->m_IgnoreMouseEvents = false;
        DrawPanel->MouseToCursorSchema();

        if( Text->m_Text.IsEmpty() )
        {
            delete Text;
            m_drawItem = NULL;
        }
        else
        {
            m_drawItem = Text;
        }
        break;
    }
    default:
        DisplayError( this, wxT( "WinEDA_LibeditFrame::CreateGraphicItem() error" ) );
        return NULL;
    }

    if( m_drawItem )
    {
        m_drawItem->BeginEdit( IS_NEW, drawPos );
        m_drawItem->SetWidth( m_drawLineWidth );
        m_drawItem->m_Fill  = m_drawFillStyle;

        if( m_drawSpecificUnit )
            m_drawItem->m_Unit = m_unit;
        if( m_drawSpecificConvert )
            m_drawItem->m_Convert = m_convert;

        // Draw initial symbol:
        DrawPanel->ManageCurseur( DrawPanel, DC, false );
    }
    else
    {
        DrawPanel->ManageCurseur = NULL;
        DrawPanel->ForceCloseManageCurseur = NULL;
        return NULL;
    }

    DrawPanel->MouseToCursorSchema();
    DrawPanel->m_IgnoreMouseEvents = FALSE;

    return m_drawItem;
}


/* Create new library component graphic object.
 */
void WinEDA_LibeditFrame::GraphicItemBeginDraw( wxDC* DC )
{
    if( m_drawItem == NULL )
        return;

    wxPoint pos = GetScreen()->GetCursorDrawPosition();

    if( m_drawItem->ContinueEdit( pos ) )
    {
        m_drawItem->Draw( DrawPanel, DC, pos, -1, g_XorMode, NULL, DefaultTransform );
        return;
    }

    EndDrawGraphicItem( DC );
}


/*
 * Redraw the graphic shape while moving
 */
static void RedrawWhileMovingCursor( WinEDA_DrawPanel* panel, wxDC* DC, bool erase )
{
    LIB_DRAW_ITEM* item;

    item = ( (WinEDA_LibeditFrame*) panel->GetParent() )->GetDrawItem();

    if( item == NULL )
        return;

    BASE_SCREEN* Screen = panel->GetScreen();

    item->SetEraseLastDrawItem( erase );
    item->Draw( panel, DC, Screen->GetCursorDrawPosition(), -1, g_XorMode, NULL,
                DefaultTransform );
}


void WinEDA_LibeditFrame::StartMoveDrawSymbol( wxDC* DC )
{
    if( m_drawItem == NULL )
        return;

    SetCursor( wxCURSOR_HAND );

    if( m_drawItem->m_Unit != m_unit )
        m_drawItem->m_Unit = m_unit;

    TempCopyComponent();
    m_drawItem->BeginEdit( IS_MOVED, GetScreen()->GetCursorDrawPosition() );
    DrawPanel->ManageCurseur = RedrawWhileMovingCursor;
    DrawPanel->ForceCloseManageCurseur = AbortSymbolTraceOn;
    DrawPanel->ManageCurseur( DrawPanel, DC, true );
}


// @brief Modify a graphic symbol (drag edges etc.)
void WinEDA_LibeditFrame::StartModifyDrawSymbol( wxDC* DC )
{
    if( m_drawItem == NULL )
        return;

    TempCopyComponent();
    m_drawItem->BeginEdit( IS_RESIZED, GetScreen()->GetCursorDrawPosition() );
    DrawPanel->ManageCurseur = SymbolDisplayDraw;
    DrawPanel->ForceCloseManageCurseur = AbortSymbolTraceOn;
    DrawPanel->ManageCurseur( DrawPanel, DC, true );
}


//! @brief Manage mouse events when creating new graphic object or modifying an graphic object.
static void SymbolDisplayDraw( WinEDA_DrawPanel* panel, wxDC* DC, bool erase )
{
    BASE_SCREEN*   Screen   = panel->GetScreen();
    LIB_DRAW_ITEM* item = ( (WinEDA_LibeditFrame*) panel->GetParent() )->GetDrawItem();

    if( item == NULL )
        return;

    item->SetEraseLastDrawItem( erase );
    item->Draw( panel, DC, Screen->GetCursorDrawPosition(), -1, g_XorMode, NULL,
                DefaultTransform );
}


/*
 * Place the new graphic object in the list of component drawing objects,
 * or terminate a draw item edition
 */
void WinEDA_LibeditFrame::EndDrawGraphicItem( wxDC* DC )
{
    if( m_component == NULL || m_drawItem == NULL )
        return;

    if( m_ID_current_state )
        SetCursor( wxCURSOR_PENCIL );
    else
        SetCursor( wxCURSOR_ARROW );

    if( GetTempCopyComponent() )    // used when editing an existing item
        SaveCopyInUndoList( GetTempCopyComponent() );
    else    // When creating a new item, there is still no change for the current component
            // So save it.
        SaveCopyInUndoList( m_component );

    if( m_drawItem->IsNew() )
        m_component->AddDrawItem( m_drawItem );

    m_drawItem->EndEdit( GetScreen()->GetCursorDrawPosition() );

    m_drawItem = NULL;

    OnModify();

    DrawPanel->ManageCurseur = NULL;
    DrawPanel->ForceCloseManageCurseur = NULL;
    DrawPanel->Refresh();
}
