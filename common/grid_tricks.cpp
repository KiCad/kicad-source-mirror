/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2012 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <grid_tricks.h>
#include <kiplatform/ui.h>
#include <wx/defs.h>
#include <wx/event.h>
#include <wx/tokenzr.h>
#include <wx/clipbrd.h>
#include <wx/combo.h>
#include <wx/log.h>
#include <wx/stc/stc.h>
#include <wx/textctrl.h>
#include <wx/toplevel.h>
#include <wx/weakref.h>
#include <widgets/grid_text_helpers.h>
#include <widgets/grid_icon_text_helpers.h>
#include <widgets/grid_combobox.h>
#include <search_stack.h>
#include <widgets/grid_text_button_helpers.h>

// It works for table data on clipboard for an Excel spreadsheet,
// why not us too for now.
#define COL_SEP     wxT( '\t' )
#define ROW_SEP     wxT( '\n' )
#define ROW_SEP_R   wxT( '\r' )


// wxBitmapComboBox is a native wxComboBox on GTK and MSW but a generic wxComboCtrl on macOS
static bool isDropdown( wxWindow* aControl )
{
    return dynamic_cast<wxComboBox*>( aControl ) || dynamic_cast<wxComboCtrl*>( aControl );
}


static void dismissDropdown( wxWindow* aControl )
{
    if( wxComboBox* combo = dynamic_cast<wxComboBox*>( aControl ) )
        combo->Dismiss();
    else if( wxComboCtrl* comboCtrl = dynamic_cast<wxComboCtrl*>( aControl ) )
        comboCtrl->Dismiss();
}


GRID_TRICKS::GRID_TRICKS( WX_GRID* aGrid ) :
        m_grid( aGrid ),
        m_addHandler( nullptr ),
        m_enableSingleClickEdit( true ),
        m_multiCellEditEnabled( true )
{
    init();
}


GRID_TRICKS::GRID_TRICKS( WX_GRID* aGrid, std::function<void( wxCommandEvent& )> aAddHandler ) :
        m_grid( aGrid ),
        m_addHandler( aAddHandler ),
        m_enableSingleClickEdit( true ),
        m_multiCellEditEnabled( true )
{
    init();
}


void GRID_TRICKS::init()
{
    m_sel_row_start = 0;
    m_sel_col_start = 0;
    m_sel_row_count = 0;
    m_sel_col_count = 0;

    Bind( wxEVT_GRID_CELL_CHANGING, &GRID_TRICKS::onGridCellChanging, this );
    Bind( wxEVT_GRID_CELL_CHANGED, &GRID_TRICKS::onGridCellChanged, this );
    Bind( wxEVT_GRID_EDITOR_CREATED, &GRID_TRICKS::onGridEditorCreated, this );
    Bind( wxEVT_GRID_EDITOR_SHOWN, &GRID_TRICKS::onGridEditorShown, this );
    Bind( wxEVT_GRID_EDITOR_HIDDEN, &GRID_TRICKS::onGridEditorHidden, this );

    m_grid->Bind( wxEVT_GRID_CELL_LEFT_CLICK, &GRID_TRICKS::onGridCellLeftClick, this );
    m_grid->Bind( wxEVT_GRID_CELL_LEFT_DCLICK, &GRID_TRICKS::onGridCellLeftDClick, this );
    m_grid->Bind( wxEVT_GRID_CELL_RIGHT_CLICK, &GRID_TRICKS::onGridCellRightClick, this );
    m_grid->Bind( wxEVT_GRID_LABEL_RIGHT_CLICK, &GRID_TRICKS::onGridLabelRightClick, this );
    m_grid->Bind( wxEVT_GRID_LABEL_LEFT_CLICK, &GRID_TRICKS::onGridLabelLeftClick, this );
    m_grid->Bind( wxEVT_MENU, &GRID_TRICKS::onPopupSelection, this, GRIDTRICKS_FIRST_ID, GRIDTRICKS_LAST_ID );
    m_grid->Bind( wxEVT_CHAR_HOOK, &GRID_TRICKS::onCharHook, this );
    m_grid->Bind( wxEVT_KEY_DOWN, &GRID_TRICKS::onKeyDown, this );
    m_grid->Bind( wxEVT_UPDATE_UI, &GRID_TRICKS::onUpdateUI, this );

    // The handlers that control the tooltips must be on the actual grid window, not the grid
    m_grid->GetGridWindow()->Bind( wxEVT_MOTION, &GRID_TRICKS::onGridMotion, this );
}


bool GRID_TRICKS::isTextEntry( int aRow, int aCol )
{
    wxGridCellEditor* editor = m_grid->GetCellEditor( aRow, aCol );
    bool              retval = ( dynamic_cast<wxGridCellTextEditor*>( editor )
                              || dynamic_cast<GRID_CELL_STC_EDITOR*>( editor )
                              || dynamic_cast<GRID_CELL_TEXT_BUTTON*>( editor ) );

    editor->DecRef();
    return retval;
}


bool GRID_TRICKS::isChoiceEditor( int aRow, int aCol )
{
    wxGridCellEditor* editor = m_grid->GetCellEditor( aRow, aCol );
    bool              retval = ( dynamic_cast<wxGridCellChoiceEditor*>( editor )
                              || dynamic_cast<GRID_CELL_ICON_TEXT_POPUP*>( editor )
                              || dynamic_cast<GRID_CELL_COMBOBOX*>( editor ) );

    editor->DecRef();
    return retval;
}


bool GRID_TRICKS::isCheckbox( int aRow, int aCol )
{
    wxGridCellRenderer* renderer = m_grid->GetCellRenderer( aRow, aCol );
    bool                retval = ( dynamic_cast<wxGridCellBoolRenderer*>( renderer ) );

    renderer->DecRef();
    return retval;
}


bool GRID_TRICKS::isReadOnly( int aRow, int aCol )
{
    return !m_grid->IsEditable() || m_grid->IsReadOnly( aRow, aCol );
}


bool GRID_TRICKS::toggleCell( int aRow, int aCol, bool aPreserveSelection )
{
    if( isCheckbox( aRow, aCol ) )
    {
        if( m_columnBulkEditEnabled )
        {
            if( isReadOnly( aRow, aCol ) )
                return false;

            const wxString oldValue = m_grid->GetCellValue( aRow, aCol );
            const wxString newValue = oldValue == wxT( "1" ) ? wxT( "0" ) : wxT( "1" );
            wxGridEvent    changing( m_grid->GetId(), wxEVT_GRID_CELL_CHANGING, m_grid, aRow, aCol );
            changing.SetString( newValue );
            m_grid->GetEventHandler()->ProcessEvent( changing );

            if( !changing.IsAllowed() )
                return true;

            m_grid->SetCellValue( aRow, aCol, newValue );

            wxGridEvent changed( m_grid->GetId(), wxEVT_GRID_CELL_CHANGED, m_grid, aRow, aCol );
            changed.SetString( oldValue );
            m_grid->GetEventHandler()->ProcessEvent( changed );

            if( !changed.IsAllowed() )
                m_grid->SetCellValue( aRow, aCol, oldValue );

            m_grid->ForceRefresh();
            return true;
        }

        if( !aPreserveSelection )
        {
            m_grid->ClearSelection();
            m_grid->SetGridCursor( aRow, aCol );
        }

        wxGridTableBase* model = m_grid->GetTable();

        if( model->CanGetValueAs( aRow, aCol, wxGRID_VALUE_BOOL )
                && model->CanSetValueAs( aRow, aCol, wxGRID_VALUE_BOOL ) )
        {
            model->SetValueAsBool( aRow, aCol, !model->GetValueAsBool( aRow, aCol ) );
        }
        else    // fall back to string processing
        {
            if( model->GetValue( aRow, aCol ) == wxT( "1" ) )
                model->SetValue( aRow, aCol, wxT( "0" ) );
            else
                model->SetValue( aRow, aCol, wxT( "1" ) );
        }

        // Mac needs this for the keyboard events; Linux appears to always need it.
        m_grid->ForceRefresh();

        // Let any clients know
        wxGridEvent event( m_grid->GetId(), wxEVT_GRID_CELL_CHANGED, m_grid, aRow, aCol );
        event.SetString( model->GetValue( aRow, aCol ) );
        m_grid->GetEventHandler()->ProcessEvent( event );

        return true;
    }

    return false;
}


bool GRID_TRICKS::showEditor( int aRow, int aCol )
{
    m_bulkEditor.Selection.clear();

    if( m_grid->GetGridCursorRow() != aRow || m_grid->GetGridCursorCol() != aCol )
        m_grid->SetGridCursor( aRow, aCol );

    if( !isReadOnly( aRow, aCol ) )
    {
        if( !m_columnBulkEditEnabled )
            m_grid->ClearSelection();
        else
        {
            if( !m_grid->IsInSelection( aRow, aCol ) )
                m_grid->SelectBlock( aRow, aCol, aRow, aCol );

            for( const wxGridBlockCoords& block : m_grid->GetSelectedBlocks() )
                m_bulkEditor.Selection.push_back( block );
        }

        m_sel_row_start = aRow;
        m_sel_col_start = aCol;
        m_sel_row_count = 1;
        m_sel_col_count = 1;

        if( !m_columnBulkEditEnabled && m_grid->GetSelectionMode() == wxGrid::wxGridSelectRows )
        {
            wxArrayInt rows = m_grid->GetSelectedRows();

            if( rows.size() != 1 || rows.Item( 0 ) != aRow )
                m_grid->SelectRow( aRow );
        }

        // For several reasons we can't enable the control here.  There's the whole
        // SetInSetFocus() issue/hack in wxWidgets, and there's also wxGrid's MouseUp
        // handler which doesn't notice it's processing a MouseUp until after it has
        // disabled the editor yet again.  So we re-use wxWidgets' slow-click hack,
        // which is processed later in the MouseUp handler.
        //
        // It should be pointed out that the fact that it's wxWidgets' hack doesn't
        // make it any less of a hack.  Be extra careful with any modifications here.
        // See, in particular, https://bugs.launchpad.net/kicad/+bug/1817965.
        m_grid->ShowEditorOnMouseUp();

        return true;
    }

    return false;
}


void GRID_TRICKS::onGridCellLeftClick( wxGridEvent& aEvent )
{
    int row = aEvent.GetRow();
    int col = aEvent.GetCol();

    // Don't make users click twice to toggle a checkbox or edit a text cell
    if( !aEvent.GetModifiers() )
    {
        // A plain click starts a new selection so an edit never fans out to cells the user forgot about
        if( m_columnBulkEditEnabled )
        {
            m_grid->SetGridCursor( row, col );
            m_grid->SelectBlock( row, col, row, col );
        }

        bool toggled = false;

        if( toggleCell( row, col, true ) )
        {
            if( m_columnBulkEditEnabled )
                return;

            toggled = true;
        }
        else if( m_enableSingleClickEdit && showEditor( row, col ) )
            return;

        // Apply checkbox changes to multi-selection.
        // Non-checkbox changes handled elsewhere
        if( toggled )
        {
            getSelectedArea();

            // We only want to apply this to whole rows.  If the grid allows selecting individual
            // cells, and the selection contains dijoint cells, skip this logic.
            if( !m_grid->GetSelectedCells().IsEmpty() || m_sel_row_count < 2 )
            {
                // We preserved the selection in toggleCell above; so clear it now that we know
                // we aren't doing a multi-select edit
                m_grid->ClearSelection();
                return;
            }

            wxString newVal = m_grid->GetCellValue( row, col );

            for( int otherRow = m_sel_row_start; otherRow < m_sel_row_start + m_sel_row_count; ++otherRow )
            {
                if( otherRow == row )
                    continue;

                m_grid->SetCellValue( otherRow, col, newVal );
            }

            return;
        }
    }

    aEvent.Skip();
}


void GRID_TRICKS::onGridCellChanging( wxGridEvent& aEvent )
{
    m_bulkEditRows.clear();

    if( !m_columnBulkEditEnabled )
    {
        aEvent.Skip();
        return;
    }

    const int        row = aEvent.GetRow();
    const int        col = aEvent.GetCol();
    std::vector<int> selectedRows;

    if( m_bulkEditor.Commit && m_bulkEditor.Source == wxGridCellCoords( row, col ) )
    {
        // Copied, not moved, so a vetoed commit can be retried against the same rows
        selectedRows = m_bulkEditor.Rows;
    }
    else if( m_grid->IsInSelection( row, col ) )
    {
        for( int otherRow = 0; otherRow < m_grid->GetNumberRows(); ++otherRow )
        {
            if( m_grid->IsInSelection( otherRow, col ) )
                selectedRows.push_back( otherRow );
        }
    }

    m_bulkEditor.Commit = false;
    GetNextHandler()->ProcessEvent( aEvent );
    aEvent.Skip( false );

    if( !aEvent.IsAllowed() )
        return;

    for( int otherRow : selectedRows )
    {
        if( otherRow == row || otherRow >= m_grid->GetNumberRows() || isReadOnly( otherRow, col ) )
        {
            continue;
        }

        wxGridEvent changing( m_grid->GetId(), wxEVT_GRID_CELL_CHANGING, m_grid, otherRow, col );
        changing.SetString( aEvent.GetString() );
        GetNextHandler()->ProcessEvent( changing );

        if( !changing.IsAllowed() )
        {
            m_bulkEditRows.clear();
            aEvent.Veto();
            return;
        }

        m_bulkEditRows.emplace_back( otherRow, m_grid->GetCellValue( otherRow, col ) );
    }

    m_bulkEditSource = wxGridCellCoords( row, col );
    m_bulkEditValue = aEvent.GetString();
}


void GRID_TRICKS::onGridEditorCreated( wxGridEditorCreatedEvent& aEvent )
{
    if( m_columnBulkEditEnabled )
    {
        wxWindow* control = aEvent.GetWindow();
        control->Bind( wxEVT_TEXT, &GRID_TRICKS::onEditorInput, this );
        control->Bind( wxEVT_COMBOBOX, &GRID_TRICKS::onEditorInput, this );

        if( isDropdown( control ) )
        {
            control->Bind( wxEVT_TEXT_ENTER, &GRID_TRICKS::onEditorTextEnter, this );
            control->GetEventHandler()->Bind( wxEVT_KEY_DOWN, &GRID_TRICKS::onEditorKeyDown, this );
            control->GetEventHandler()->Bind( wxEVT_COMBOBOX_DROPDOWN, &GRID_TRICKS::onEditorDropdownOpened, this );
            control->GetEventHandler()->Bind( wxEVT_COMBOBOX_CLOSEUP, &GRID_TRICKS::onEditorDropdownClosed, this );
        }
    }

    aEvent.Skip();
}


void GRID_TRICKS::onGridEditorShown( wxGridEvent& aEvent )
{
    std::vector<wxGridBlockCoords> selection = std::move( m_bulkEditor.Selection );
    m_bulkEditor.Begin( wxGridCellCoords( aEvent.GetRow(), aEvent.GetCol() ) );

    for( const wxGridBlockCoords& block : selection )
        m_grid->SelectBlock( block.GetTopRow(), block.GetLeftCol(), block.GetBottomRow(), block.GetRightCol(), true );

    const int row = aEvent.GetRow();
    const int col = aEvent.GetCol();

    if( m_columnBulkEditEnabled && m_grid->IsInSelection( row, col ) )
    {
        for( int otherRow = 0; otherRow < m_grid->GetNumberRows(); ++otherRow )
        {
            if( m_grid->IsInSelection( otherRow, col ) )
                m_bulkEditor.Rows.push_back( otherRow );
        }
    }

    aEvent.Skip();
}


void GRID_TRICKS::onEditorInput( wxCommandEvent& aEvent )
{
    if( m_bulkEditorOpening )
    {
        if( aEvent.GetEventType() == wxEVT_TEXT )
            m_bulkEditor.TextInitialized = true;

        aEvent.Skip();
        return;
    }

    const int row = m_grid->GetGridCursorRow();
    const int col = m_grid->GetGridCursorCol();

    if( m_columnBulkEditEnabled && row >= 0 && col >= 0 )
    {
        if( aEvent.GetEventType() == wxEVT_TEXT && dynamic_cast<wxTextCtrl*>( aEvent.GetEventObject() ) )
        {
            if( m_bulkEditor.TextInitialized || aEvent.GetString() != m_grid->GetCellValue( row, col ) )
                m_bulkEditor.Input = true;

            m_bulkEditor.TextInitialized = true;
        }
        else if( aEvent.GetEventType() == wxEVT_COMBOBOX || aEvent.GetString() != m_grid->GetCellValue( row, col ) )
        {
            m_bulkEditor.Input = true;
        }

        if( aEvent.GetEventType() == wxEVT_COMBOBOX
            && ( !m_bulkEditor.KeyboardNavigation || m_bulkEditor.PopupKey.GetKeyCode() != WXK_NONE ) )
        {
            finishDropdown();
        }
    }

    aEvent.Skip();
}


void GRID_TRICKS::onEditorKeyDown( wxKeyEvent& aEvent )
{
    if( m_columnBulkEditEnabled && ( aEvent.GetKeyCode() == WXK_RETURN || aEvent.GetKeyCode() == WXK_NUMPAD_ENTER ) )
    {
        m_bulkEditorEnterHandled = !aEvent.GetModifiers();
    }

    if( wxComboCtrl* comboCtrl = shownGenericPopup( aEvent ) )
    {
        // The grid editor handler would take Enter before the combo, so hand it to the popup directly
        m_bulkEditor.PopupKey = aEvent;
        comboCtrl->GetPopupControl()->GetControl()->GetEventHandler()->ProcessEvent( aEvent );
        return;
    }

    if( m_columnBulkEditEnabled && !aEvent.GetModifiers() )
    {
        switch( aEvent.GetKeyCode() )
        {
        case WXK_UP:
        case WXK_DOWN:
        case WXK_HOME:
        case WXK_END:
        case WXK_PAGEUP:
        case WXK_PAGEDOWN: m_bulkEditor.KeyboardNavigation = true; break;

        case WXK_RETURN:
        case WXK_NUMPAD_ENTER: finishEditor( false, true ); return;

        case WXK_ESCAPE: finishEditor( true ); return;
        }
    }

    aEvent.Skip();
}


void GRID_TRICKS::onEditorTextEnter( wxCommandEvent& aEvent )
{
    if( !m_columnBulkEditEnabled || !m_bulkEditorEnterHandled )
        aEvent.Skip();
}


void GRID_TRICKS::onEditorDropdownOpened( wxCommandEvent& aEvent )
{
    if( m_columnBulkEditEnabled )
    {
        wxWindow* control = dynamic_cast<wxWindow*>( aEvent.GetEventObject() );
        wxCHECK_RET( control, wxT( "Missing grid dropdown control" ) );

        if( !KIPLATFORM::UI::PrepareChoicePopup( control ) )
        {
            CallAfter(
                    [control = wxWeakRef<wxWindow>( control )]()
                    {
                        if( control )
                            KIPLATFORM::UI::PrepareChoicePopup( control );
                    } );
        }
    }

    if( !m_bulkEditorOpening )
        aEvent.Skip();
}


void GRID_TRICKS::onEditorDropdownClosed( wxCommandEvent& aEvent )
{
    if( !m_columnBulkEditEnabled )
    {
        aEvent.Skip();
        return;
    }

    if( m_bulkEditorClosing || !m_grid->IsCellEditControlEnabled() )
        return;

    // A generic popup closes before it reports the choice, so keep the Enter it was given
    if( m_bulkEditor.PopupKey.GetKeyCode() == WXK_NONE )
        m_bulkEditor.PopupKey = KIPLATFORM::UI::GetNativePopupDismissalEvent();

    wxWeakRef<WX_GRID> grid( m_grid );
    const unsigned int generation = m_bulkEditor.Generation;

    CallAfter(
            [this, grid, generation]()
            {
                if( grid && m_bulkEditor.Generation == generation && grid->IsCellEditControlEnabled() )
                    finishDropdown();
            } );
}


void GRID_TRICKS::finishDropdown()
{
    const wxKeyEvent key = m_bulkEditor.PopupKey;
    const bool       enter = key.GetKeyCode() == WXK_RETURN || key.GetKeyCode() == WXK_NUMPAD_ENTER;
    const bool       committed = finishEditor( key.GetKeyCode() == WXK_ESCAPE, enter && !key.GetModifiers() );

    if( committed && enter && key.GetModifiers() == wxMOD_SHIFT )
    {
        if( wxTopLevelWindow* window = dynamic_cast<wxTopLevelWindow*>( wxGetTopLevelParent( m_grid ) ) )
        {
            wxWindow* button = window->GetDefaultItem();

            if( button && button->IsEnabled() )
            {
                wxCommandEvent accept( wxEVT_BUTTON, button->GetId() );
                accept.SetEventObject( button );
                wxPostEvent( button, accept );
            }
        }
    }
}


wxComboCtrl* GRID_TRICKS::shownGenericPopup( const wxKeyEvent& aEnterKey )
{
    if( !m_columnBulkEditEnabled || aEnterKey.GetModifiers() || !m_grid->IsCellEditControlEnabled()
        || ( aEnterKey.GetKeyCode() != WXK_RETURN && aEnterKey.GetKeyCode() != WXK_NUMPAD_ENTER ) )
    {
        return nullptr;
    }

    wxGridCellEditor* editor = m_grid->GetCellEditor( m_grid->GetGridCursorRow(), m_grid->GetGridCursorCol() );
    wxComboCtrl*      comboCtrl = dynamic_cast<wxComboCtrl*>( editor->GetWindow() );
    editor->DecRef();

    // Native popups grab the keyboard, so only a generic one can be open while its control sees keys
    return comboCtrl && comboCtrl->IsPopupShown() ? comboCtrl : nullptr;
}


bool GRID_TRICKS::finishEditor( bool aCancel, bool aMoveDown )
{
    if( m_bulkEditorClosing || !m_grid->IsCellEditControlEnabled() )
        return false;

    m_bulkEditorClosing = true;

    const int row = m_grid->GetGridCursorRow();
    const int col = m_grid->GetGridCursorCol();
    int       nextRow = row + 1;

    if( aMoveDown )
    {
        while( nextRow < m_grid->GetNumberRows()
               && ( m_grid->IsInSelection( nextRow, col ) || !m_grid->IsRowShown( nextRow ) ) )
        {
            ++nextRow;
        }
    }

    wxGridCellEditor* editor = m_grid->GetCellEditor( row, col );

    dismissDropdown( editor->GetWindow() );

    bool committed = false;

    if( aCancel )
    {
        m_bulkEditor.Input = false;
        m_bulkEditor.Rows.clear();
        m_grid->CancelPendingChanges();
    }
    else
    {
        committed = m_grid->CommitPendingChanges();
    }

    editor->DecRef();

    if( committed && aMoveDown && nextRow < m_grid->GetNumberRows() )
    {
        m_grid->GoToCell( nextRow, col );

        if( m_grid->GetGridCursorRow() == nextRow && m_grid->GetGridCursorCol() == col )
            m_grid->SelectBlock( nextRow, col, nextRow, col );
    }

    m_bulkEditorClosing = false;
    return committed;
}


void GRID_TRICKS::onGridEditorHidden( wxGridEvent& aEvent )
{
    if( !m_columnBulkEditEnabled )
    {
        aEvent.Skip();
        return;
    }

    const int         row = aEvent.GetRow();
    const int         col = aEvent.GetCol();
    wxGridCellEditor* editor = m_grid->GetCellEditor( row, col );

    if( !editor->IsCreated() )
    {
        editor->DecRef();
        m_bulkEditor.Rows.clear();
        aEvent.Skip();
        return;
    }

    const wxString value = editor->GetValue();
    editor->DecRef();

    if( value == m_grid->GetCellValue( row, col ) && !m_bulkEditor.Input )
    {
        m_bulkEditor.Rows.clear();
        aEvent.Skip();
        return;
    }

    m_bulkEditor.Commit = true;
    GetNextHandler()->ProcessEvent( aEvent );
    aEvent.Skip( false );

    if( !aEvent.IsAllowed() )
    {
        m_bulkEditor.Commit = false;
        return;
    }

    if( value != m_grid->GetCellValue( row, col ) )
    {
        m_bulkEditor.Input = false;
        return;
    }

    wxGridEvent changing( m_grid->GetId(), wxEVT_GRID_CELL_CHANGING, m_grid, row, col );
    changing.SetString( value );
    m_grid->GetEventHandler()->ProcessEvent( changing );

    if( !changing.IsAllowed() )
    {
        aEvent.Veto();
        return;
    }

    m_bulkEditor.Input = false;

    if( !m_bulkEditRows.empty() )
    {
        wxGridEvent changed( m_grid->GetId(), wxEVT_GRID_CELL_CHANGED, m_grid, row, col );
        changed.SetString( value );
        m_grid->GetEventHandler()->ProcessEvent( changed );

        if( !changed.IsAllowed() )
            aEvent.Veto();
    }
}


void GRID_TRICKS::onGridCellChanged( wxGridEvent& aEvent )
{
    if( m_columnBulkEditEnabled && m_bulkEditSource == wxGridCellCoords( aEvent.GetRow(), aEvent.GetCol() )
        && !m_bulkEditRows.empty() )
    {
        const int                             col = aEvent.GetCol();
        const wxString                        sourceOldValue = aEvent.GetString();
        std::vector<std::pair<int, wxString>> rows;
        rows.swap( m_bulkEditRows );

        wxGridUpdateLocker lock( m_grid );

        for( const auto& [row, oldValue] : rows )
            m_grid->SetCellValue( row, col, m_bulkEditValue );

        bool allowed = true;

        for( const auto& [row, oldValue] : rows )
        {
            wxGridEvent changed( m_grid->GetId(), wxEVT_GRID_CELL_CHANGED, m_grid, row, col );
            changed.SetString( oldValue );
            GetNextHandler()->ProcessEvent( changed );

            if( !changed.IsAllowed() )
            {
                allowed = false;
                break;
            }
        }

        if( allowed )
        {
            GetNextHandler()->ProcessEvent( aEvent );
            allowed = aEvent.IsAllowed();
        }

        aEvent.Skip( false );

        if( !allowed )
        {
            for( const auto& [row, oldValue] : rows )
                m_grid->SetCellValue( row, col, oldValue );

            m_grid->SetCellValue( aEvent.GetRow(), col, sourceOldValue );
            aEvent.Veto();
        }
        else
        {
            m_grid->EvaluateCell( aEvent.GetRow(), col );

            for( const auto& [row, oldValue] : rows )
                m_grid->EvaluateCell( row, col );
        }

        m_grid->ForceRefresh();
        return;
    }

    aEvent.Skip();
}


void GRID_TRICKS::onGridCellLeftDClick( wxGridEvent& aEvent )
{
    if( !handleDoubleClick( aEvent ) )
        onGridCellLeftClick( aEvent );
}


void GRID_TRICKS::onGridMotion( wxMouseEvent& aEvent )
{
    // Always skip the event
    aEvent.Skip();

    wxPoint pt  = aEvent.GetPosition();
    wxPoint pos = m_grid->CalcUnscrolledPosition( wxPoint( pt.x, pt.y ) );

    int col = m_grid->XToCol( pos.x );
    int row = m_grid->YToRow( pos.y );

    // Empty tooltip if the cell doesn't exist or the column doesn't have tooltips
    if( col < 0 || row < 0 || static_cast<size_t>( col ) >= m_tooltipEnabled.size() || !m_tooltipEnabled[col] )
    {
        m_grid->GetGridWindow()->SetToolTip( wxS( "" ) );
        return;
    }

    // Set the tooltip to the string contained in the cell
    m_grid->GetGridWindow()->SetToolTip( m_grid->GetCellValue( row, col ) );
}


bool GRID_TRICKS::handleDoubleClick( wxGridEvent& aEvent )
{
    // Double-click processing must be handled by specific sub-classes
    return false;
}


void GRID_TRICKS::getSelectedArea()
{
    if( m_columnBulkEditEnabled && m_grid->IsSelection() )
    {
        int firstRow = m_grid->GetNumberRows();
        int firstCol = m_grid->GetNumberCols();
        int lastRow = -1;
        int lastCol = -1;

        for( const wxGridBlockCoords& block : m_grid->GetSelectedBlocks() )
        {
            firstRow = std::min( firstRow, block.GetTopRow() );
            firstCol = std::min( firstCol, block.GetLeftCol() );
            lastRow = std::max( lastRow, block.GetBottomRow() );
            lastCol = std::max( lastCol, block.GetRightCol() );
        }

        m_sel_row_start = firstRow;
        m_sel_col_start = firstCol;
        m_sel_row_count = lastRow - firstRow + 1;
        m_sel_col_count = lastCol - firstCol + 1;
        return;
    }

    wxGridCellCoordsArray topLeft  = m_grid->GetSelectionBlockTopLeft();
    wxGridCellCoordsArray botRight = m_grid->GetSelectionBlockBottomRight();

    wxArrayInt  cols = m_grid->GetSelectedCols();
    wxArrayInt  rows = m_grid->GetSelectedRows();

    if( topLeft.Count() && botRight.Count() )
    {
        m_sel_row_start = topLeft[0].GetRow();
        m_sel_col_start = topLeft[0].GetCol();

        m_sel_row_count = botRight[0].GetRow() - m_sel_row_start + 1;
        m_sel_col_count = botRight[0].GetCol() - m_sel_col_start + 1;
    }
    else if( cols.Count() )
    {
        m_sel_col_start = cols[0];
        m_sel_col_count = cols.Count();
        m_sel_row_start = 0;
        m_sel_row_count = m_grid->GetNumberRows();
    }
    else if( rows.Count() )
    {
        m_sel_col_start = 0;
        m_sel_col_count = m_grid->GetNumberCols();
        m_sel_row_start = rows[0];
        m_sel_row_count = rows.Count();
    }
    else
    {
        m_sel_row_start = m_grid->GetGridCursorRow();
        m_sel_col_start = m_grid->GetGridCursorCol();
        m_sel_row_count = m_sel_row_start >= 0 ? 1 : 0;
        m_sel_col_count = m_sel_col_start >= 0 ? 1 : 0;
    }
}


void GRID_TRICKS::onGridCellRightClick( wxGridEvent& aEvent  )
{
    if( !m_grid->CommitPendingChanges( !m_columnBulkEditEnabled ) )
        return;

    // Select the right-clicked row if it is not already part of the current selection
    int row = aEvent.GetRow();

    if( m_grid->GetSelectionMode() == wxGrid::wxGridSelectRows )
    {
        wxArrayInt selectedRows = m_grid->GetSelectedRows();

        if( selectedRows.Index( row ) == wxNOT_FOUND )
            m_grid->SelectRow( row );
    }
    else
    {
        if( !m_grid->IsInSelection( row, aEvent.GetCol() ) )
        {
            m_grid->SetGridCursor( row, aEvent.GetCol() );

            if( m_columnBulkEditEnabled )
                m_grid->SelectBlock( row, aEvent.GetCol(), row, aEvent.GetCol() );
        }
    }

    wxMenu menu;

    showPopupMenu( menu, aEvent );
}


void GRID_TRICKS::onGridLabelLeftClick( wxGridEvent& aEvent )
{
    m_grid->CommitPendingChanges();

    aEvent.Skip();
}


void GRID_TRICKS::onGridLabelRightClick( wxGridEvent&  )
{
    wxMenu menu;

    for( int i = 0; i < m_grid->GetNumberCols(); ++i )
    {
        int id = GRIDTRICKS_FIRST_SHOWHIDE + i;
        menu.AppendCheckItem( id, m_grid->GetColLabelValue( i ) );
        menu.Check( id, m_grid->IsColShown( i ) );
    }

    m_grid->PopupMenu( &menu );
}


void GRID_TRICKS::showPopupMenu( wxMenu& menu, wxGridEvent& aEvent )
{
    menu.Append( GRIDTRICKS_ID_CUT, _( "Cut" ) + "\tCtrl+X",
                 _( "Clear selected cells placing original contents on clipboard" ) );
    menu.Append( GRIDTRICKS_ID_COPY, _( "Copy" ) + "\tCtrl+C",
                 _( "Copy selected cells to clipboard" ) );

    if( m_multiCellEditEnabled )
    {
        menu.Append( GRIDTRICKS_ID_PASTE, _( "Paste" ) + "\tCtrl+V",
                     _( "Paste clipboard cells to matrix at current cell" ) );
        menu.Append( GRIDTRICKS_ID_DELETE, _( "Delete" ) + "\tDel",
                     _( "Clear contents of selected cells" ) );
    }

    menu.Append( GRIDTRICKS_ID_SELECT, _( "Select All" ) + "\tCtrl+A",
                 _( "Select all cells" ) );

    menu.Enable( GRIDTRICKS_ID_CUT,  false );
    menu.Enable( GRIDTRICKS_ID_DELETE, false );
    menu.Enable( GRIDTRICKS_ID_PASTE, false );

    getSelectedArea();

    auto anyCellsWritable =
            [&]()
            {
                for( int row = m_sel_row_start; row < m_sel_row_start + m_sel_row_count; ++row )
                {
                    for( int col = m_sel_col_start; col < m_sel_col_start + m_sel_col_count; ++col )
                    {
                        if( !isReadOnly( row, col )
                            && ( isTextEntry( row, col ) || isChoiceEditor( row, col ) ) )
                        {
                            return true;
                        }
                    }
                }

                return false;
            };

    if( anyCellsWritable() )
    {
        menu.Enable( GRIDTRICKS_ID_CUT,  true );
        menu.Enable( GRIDTRICKS_ID_DELETE, true );
    }

    // Paste can overflow the selection, so don't depend on the particular cell being writeable.

    wxLogNull doNotLog; // disable logging of failed clipboard actions

    if( wxTheClipboard->Open() )
    {
        if( wxTheClipboard->IsSupported( wxDF_TEXT )
            || wxTheClipboard->IsSupported( wxDF_UNICODETEXT ) )
        {
            if( m_grid->IsEditable() )
                menu.Enable( GRIDTRICKS_ID_PASTE, true );
        }

        wxTheClipboard->Close();
    }

    appendPopupMenuItems( menu );
    m_grid->PopupMenu( &menu );
}


void GRID_TRICKS::onPopupSelection( wxCommandEvent& event )
{
    doPopupSelection( event );
}


void GRID_TRICKS::doPopupSelection( wxCommandEvent& event )
{
    int     menu_id = event.GetId();

    // assume getSelectedArea() was called by rightClickPopupMenu() and there's
    // no way to have gotten here without that having been called.

    switch( menu_id )
    {
    case GRIDTRICKS_ID_CUT:
        cutcopy( true, true );
        break;

    case GRIDTRICKS_ID_COPY:
        cutcopy( true, false );
        break;

    case GRIDTRICKS_ID_DELETE:
        cutcopy( false, true );
        break;

    case GRIDTRICKS_ID_PASTE:
        paste_clipboard();
        break;

    case GRIDTRICKS_ID_SELECT:
        m_grid->SelectAll();
        break;

    default:
        if( menu_id >= GRIDTRICKS_FIRST_SHOWHIDE && m_grid->CommitPendingChanges( false ) )
        {
            int col = menu_id - GRIDTRICKS_FIRST_SHOWHIDE;

            if( m_grid->IsColShown( col ) )
                m_grid->HideCol( col );
            else
                m_grid->ShowCol( col );
        }
    }
}


void GRID_TRICKS::onCharHook( wxKeyEvent& ev )
{
    bool handled = false;

    if( m_columnBulkEditEnabled && ( ev.GetKeyCode() == WXK_RETURN || ev.GetKeyCode() == WXK_NUMPAD_ENTER ) )
    {
        m_bulkEditorEnterHandled = !ev.GetModifiers();
    }

    // Let Enter arrive as KEY_DOWN so onEditorKeyDown() can pass it to the open popup
    if( shownGenericPopup( ev ) )
    {
        ev.Skip();
        return;
    }

    if( m_columnBulkEditEnabled && !ev.GetModifiers()
        && ( ev.GetKeyCode() == WXK_RETURN || ev.GetKeyCode() == WXK_NUMPAD_ENTER ) )
    {
        const int row = m_grid->GetGridCursorRow();
        const int col = m_grid->GetGridCursorCol();

        if( m_grid->IsCellEditControlEnabled() )
        {
            finishEditor( false, true );
        }
        else if( m_addHandler && row == m_grid->GetNumberRows() - 1 && m_grid->GetSelectedRowIndices().size() <= 1 )
        {
            wxCommandEvent dummy;
            m_addHandler( dummy );
        }
        else if( row >= 0 && col >= 0 && !isReadOnly( row, col ) )
        {
            if( !m_grid->IsInSelection( row, col ) )
                m_grid->SelectBlock( row, col, row, col );

            m_bulkEditorOpening = true;
            m_grid->EnableCellEditControl();
            m_bulkEditorOpening = false;

            if( m_grid->IsCellEditControlEnabled() )
            {
                wxGridCellEditor* editor = m_grid->GetCellEditor( row, col );

#ifndef __WXOSX__
                // The macOS editors already open their popup in BeginEdit
                if( wxComboBox* combo = dynamic_cast<wxComboBox*>( editor->GetWindow() ) )
                {
                    auto handler = static_cast<wxGridCellEditorEvtHandler*>( combo->GetEventHandler() );
                    m_bulkEditorOpening = true;
                    handler->SetInSetFocus( true );
                    combo->Popup();
                    handler->SetInSetFocus( false );
                    m_bulkEditorOpening = false;
                }
#endif

                editor->DecRef();
            }
        }

        handled = true;
    }
    else if( ( ev.GetKeyCode() == WXK_RETURN || ev.GetKeyCode() == WXK_NUMPAD_ENTER ) && ev.GetModifiers() == wxMOD_NONE
             && m_grid->GetGridCursorRow() == m_grid->GetNumberRows() - 1 )
    {
        if( m_grid->IsCellEditControlShown() )
        {
            if( m_grid->CommitPendingChanges() )
                handled = true;
        }
        else
        {
            if( m_addHandler )
            {
                wxCommandEvent dummy;
                m_addHandler( dummy );
            }

            handled = true;
        }
    }
    else if( ev.GetModifiers() == wxMOD_CONTROL && ev.GetKeyCode() == 'C' )
    {
        if( m_grid->IsCellEditControlShown() )
        {
            wxTextEntry* te = dynamic_cast<wxTextEntry*>( ev.GetEventObject() );

            if( te )
            {
                wxString selectedText = te->GetStringSelection();

                if( !selectedText.IsEmpty() )
                {
                    wxLogNull doNotLog;

                    if( wxTheClipboard->Open() )
                    {
                        wxTheClipboard->SetData( new wxTextDataObject( selectedText ) );
                        wxTheClipboard->Flush();
                        wxTheClipboard->Close();
                        handled = true;
                    }
                }
            }

            if( !handled )
            {
                m_grid->CancelPendingChanges();
                getSelectedArea();
                cutcopy( true, false );
                handled = true;
            }
        }
    }
    else if( ev.GetModifiers() == wxMOD_CONTROL && ev.GetKeyCode() == 'V' )
    {
        if( m_grid->IsCellEditControlShown() )
        {
            wxLogNull doNotLog;

            if( wxTheClipboard->Open() )
            {
                if( wxTheClipboard->IsSupported( wxDF_TEXT )
                    || wxTheClipboard->IsSupported( wxDF_UNICODETEXT ) )
                {
                    wxTextDataObject data;
                    wxTheClipboard->GetData( data );
                    wxString         text = data.GetText();

                    bool hasMultipleCells = text.Contains( COL_SEP ) || text.Contains( ROW_SEP );

                    if( hasMultipleCells )
                    {
                        text.Replace( ROW_SEP, wxS( " " ) );
                        text.Replace( ROW_SEP_R, wxS( " " ) );
                        text.Replace( COL_SEP, wxS( " " ) );
                    }

                    wxTextEntry* te = dynamic_cast<wxTextEntry*>( ev.GetEventObject() );

                    if( te && te->IsEditable() )
                    {
                        te->WriteText( text );
                        handled = true;
                    }
                    else
                    {
                        m_grid->CancelPendingChanges();
                        getSelectedArea();
                        paste_text( text );
                        handled = true;
                    }
                }

                wxTheClipboard->Close();
                m_grid->ForceRefresh();
            }
        }
    }
    else if( ev.GetKeyCode() == WXK_ESCAPE )
    {
        if( m_grid->IsCellEditControlShown() )
        {
            if( m_columnBulkEditEnabled )
                finishEditor( true );
            else
                m_grid->CancelPendingChanges();

            handled = true;
        }
    }

    if( !handled )
        ev.Skip( true );
}


void GRID_TRICKS::onKeyDown( wxKeyEvent& ev )
{
    if( m_columnBulkEditEnabled && !ev.GetModifiers()
        && ( ev.GetKeyCode() == WXK_RETURN || ev.GetKeyCode() == WXK_NUMPAD_ENTER ) )
    {
        onCharHook( ev );
        return;
    }
    else if( ev.GetModifiers() == wxMOD_CONTROL && ev.GetKeyCode() == 'A' )
    {
        m_grid->SelectAll();
        return;
    }
    else if( ev.GetModifiers() == wxMOD_CONTROL && ev.GetKeyCode() == 'C' )
    {
        getSelectedArea();
        cutcopy( true, false );
        return;
    }
    else if( ev.GetModifiers() == wxMOD_CONTROL && ev.GetKeyCode() == 'V' )
    {
        getSelectedArea();
        paste_clipboard();
        return;
    }
    else if( ev.GetModifiers() == wxMOD_CONTROL && ev.GetKeyCode() == 'X' )
    {
        getSelectedArea();
        cutcopy( true, true );
        return;
    }
    else if( !ev.GetModifiers() && ev.GetKeyCode() == WXK_DELETE )
    {
        getSelectedArea();
        cutcopy( false, true );
        return;
    }

    // space-bar toggling of checkboxes
    if( m_grid->IsEditable() && ev.GetKeyCode() == ' ' )
    {
        if( m_columnBulkEditEnabled )
        {
            const int row = m_grid->GetGridCursorRow();
            const int col = m_grid->GetGridCursorCol();

            if( row >= 0 && col >= 0 && toggleCell( row, col, true ) )
                return;

            ev.Skip();
            return;
        }

        bool retVal = false;

        // If only rows can be selected, only toggle the first cell in a row
        if( m_grid->GetSelectionMode() == wxGrid::wxGridSelectRows )
        {
            wxArrayInt rowSel = m_grid->GetSelectedRows();

            for( unsigned int rowInd = 0; rowInd < rowSel.GetCount(); rowInd++ )
                retVal |= toggleCell( rowSel[rowInd], 0, true );
        }

        // If only columns can be selected, only toggle the first cell in a column
        else if( m_grid->GetSelectionMode() == wxGrid::wxGridSelectColumns )
        {
            wxArrayInt colSel = m_grid->GetSelectedCols();

            for( unsigned int colInd = 0; colInd < colSel.GetCount(); colInd++ )
                retVal |= toggleCell( 0, colSel[colInd], true );
        }

        // If the user can select the individual cells, toggle each cell selected
        else if( m_grid->GetSelectionMode() == wxGrid::wxGridSelectCells )
        {
            wxArrayInt            rowSel   = m_grid->GetSelectedRows();
            wxArrayInt            colSel   = m_grid->GetSelectedCols();
            wxGridCellCoordsArray cellSel  = m_grid->GetSelectedCells();
            wxGridCellCoordsArray topLeft  = m_grid->GetSelectionBlockTopLeft();
            wxGridCellCoordsArray botRight = m_grid->GetSelectionBlockBottomRight();

            // Iterate over every individually selected cell and try to toggle it
            for( unsigned int cellInd = 0; cellInd < cellSel.GetCount(); cellInd++ )
            {
                retVal |= toggleCell( cellSel[cellInd].GetRow(), cellSel[cellInd].GetCol(), true );
            }

            // Iterate over every column and try to toggle each cell in it
            for( unsigned int colInd = 0; colInd < colSel.GetCount(); colInd++ )
            {
                for( int row = 0; row < m_grid->GetNumberRows(); row++ )
                    retVal |= toggleCell( row, colSel[colInd], true );
            }

            // Iterate over every row and try to toggle each cell in it
            for( unsigned int rowInd = 0; rowInd < rowSel.GetCount(); rowInd++ )
            {
                for( int col = 0; col < m_grid->GetNumberCols(); col++ )
                    retVal |= toggleCell( rowSel[rowInd], col, true );
            }

            // Iterate over the selection blocks
            for( unsigned int blockInd = 0; blockInd < topLeft.GetCount(); blockInd++ )
            {
                wxGridCellCoords start = topLeft[blockInd];
                wxGridCellCoords end   = botRight[blockInd];

                for( int row = start.GetRow(); row <= end.GetRow(); row++ )
                {
                    for( int col = start.GetCol(); col <= end.GetCol(); col++ )
                        retVal |= toggleCell( row, col, true );
                }
            }
        }

        // Return if there were any cells toggled
        if( retVal )
            return;
    }

    // ctrl-tab for exit grid
#ifdef __APPLE__
    bool ctrl = ev.RawControlDown();
#else
    bool ctrl = ev.ControlDown();
#endif

    if( ctrl && ev.GetKeyCode() == WXK_TAB )
    {
        wxWindow* test = m_grid->GetNextSibling();

        if( !test )
            test = m_grid->GetParent()->GetNextSibling();

        while( test && !test->IsTopLevel() )
        {
            test->SetFocus();

            if( test->HasFocus() )
                break;

            if( !test->GetChildren().empty() )
            {
                test = test->GetChildren().front();
            }
            else if( test->GetNextSibling() )
            {
                test = test->GetNextSibling();
            }
            else
            {
                while( test )
                {
                    test = test->GetParent();

                    if( test && test->IsTopLevel() )
                    {
                        break;
                    }
                    else if( test && test->GetNextSibling() )
                    {
                        test = test->GetNextSibling();
                        break;
                    }
                }
            }
        }

        return;
    }

    ev.Skip( true );
}


void GRID_TRICKS::paste_clipboard()
{
    wxLogNull doNotLog; // disable logging of failed clipboard actions

    if( m_grid->IsEditable() && ( wxTheClipboard->IsOpened() || wxTheClipboard->Open() ) )
    {
        if( wxTheClipboard->IsSupported( wxDF_TEXT )
            || wxTheClipboard->IsSupported( wxDF_UNICODETEXT ) )
        {
            wxTextDataObject    data;

            wxTheClipboard->GetData( data );

            wxString text = data.GetText();

#ifdef __WXMAC__
            // Some editors use windows linefeeds (\r\n), which wx re-writes to \n\n
            text.Replace( "\n\n", "\n" );
#endif
            if( m_grid->CommitPendingChanges( !m_columnBulkEditEnabled ) )
                paste_text( text );
        }

        wxTheClipboard->Close();
        m_grid->ForceRefresh();
    }
}


void GRID_TRICKS::paste_text( const wxString& cb_text )
{
    if( !m_multiCellEditEnabled )
        return;

    wxGridTableBase*   tbl = m_grid->GetTable();

    const int cur_row = m_grid->GetGridCursorRow();
    const int cur_col = m_grid->GetGridCursorCol();
    int start_row;
    int end_row;
    int start_col;
    int end_col;
    bool is_selection = false;

    if( cur_row < 0 || cur_col < 0 )
    {
        wxBell();
        return;
    }

    if( m_grid->GetSelectionMode() == wxGrid::wxGridSelectRows )
    {
        if( m_sel_row_count > 1 )
            is_selection = true;
    }
    else if( m_sel_col_count > 1 || m_sel_row_count > 1 )
    {
        is_selection = true;
    }

    wxStringTokenizer rows( cb_text, ROW_SEP, wxTOKEN_RET_EMPTY );

    // If selection of cells is present
    // then a clipboard pastes to selected cells only.
    if( is_selection )
    {
        start_row = m_sel_row_start;
        end_row = m_sel_row_start + m_sel_row_count;
        start_col = m_sel_col_start;
        end_col = m_sel_col_start + m_sel_col_count;
    }
    // Otherwise, paste whole clipboard
    // starting from cell with cursor.
    else
    {
        start_row = cur_row;
        end_row = cur_row + rows.CountTokens();

        if( end_row > tbl->GetNumberRows() )
        {
            if( m_addHandler )
            {
                for( int ii = end_row - tbl->GetNumberRows(); ii > 0; --ii )
                {
                    wxCommandEvent dummy;
                    m_addHandler( dummy );
                }
            }

            end_row = tbl->GetNumberRows();
        }

        start_col = cur_col;
        end_col = start_col; // end_col actual value calculates later
    }

    for( int row = start_row;  row < end_row;  ++row )
    {
        // If number of selected rows is larger than the count of rows on the clipboard, paste
        // again and again until the end of the selection is reached.
        if( !rows.HasMoreTokens() )
            rows.SetString( cb_text, ROW_SEP, wxTOKEN_RET_EMPTY );

        wxString rowTxt = rows.GetNextToken();

        wxStringTokenizer cols( rowTxt, COL_SEP, wxTOKEN_RET_EMPTY );

        if( !is_selection )
            end_col = cur_col + cols.CountTokens();

        for( int col = start_col;  col < end_col && col < tbl->GetNumberCols();  ++col )
        {
            // Skip hidden columns
            if( !m_grid->IsColShown( col ) )
            {
                end_col++;
                continue;
            }

            // If number of selected cols is larger than the count of cols on the clipboard,
            // paste again and again until the end of the selection is reached.
            if( !cols.HasMoreTokens() )
                cols.SetString( rowTxt, COL_SEP, wxTOKEN_RET_EMPTY );

            wxString cellTxt = cols.GetNextToken();

            if( m_columnBulkEditEnabled && is_selection && !m_grid->IsInSelection( row, col ) )
                continue;

            // Allow paste to anything that can take a string, including things like color
            // swatches and checkboxes
            if( tbl->CanSetValueAs( row, col, wxGRID_VALUE_STRING ) && !isReadOnly( row, col ) )
            {
                tbl->SetValue( row, col, cellTxt );

                wxGridEvent evt( m_grid->GetId(), wxEVT_GRID_CELL_CHANGED, m_grid, row, col );
                m_grid->GetEventHandler()->ProcessEvent( evt );
            }
            // Allow paste to any cell that can accept a boolean value
            else if( tbl->CanSetValueAs( row, col, wxGRID_VALUE_BOOL ) )
            {
                tbl->SetValueAsBool( row, col, cellTxt == wxT( "1" ) );

                wxGridEvent evt( m_grid->GetId(), wxEVT_GRID_CELL_CHANGED, m_grid, row, col );
                m_grid->GetEventHandler()->ProcessEvent( evt );
            }
        }
    }
}


void GRID_TRICKS::cutcopy( bool doCopy, bool doDelete )
{
    wxLogNull doNotLog; // disable logging of failed clipboard actions

    if( doCopy && !wxTheClipboard->Open() )
        return;

    wxGridTableBase*    tbl = m_grid->GetTable();
    wxString            txt;

    // fill txt with a format that is compatible with most spreadsheets
    for( int row = m_sel_row_start;  row < m_sel_row_start + m_sel_row_count;  ++row )
    {
        if( row != m_sel_row_start )
            txt += ROW_SEP;

        for( int col = m_sel_col_start;  col < m_sel_col_start + m_sel_col_count; ++col )
        {
            if( !m_grid->IsColShown( col ) )
                continue;

            const bool selected =
                    !m_columnBulkEditEnabled || !m_grid->IsSelection() || m_grid->IsInSelection( row, col );

            if( selected )
                txt += tbl->GetValue( row, col );

            if( col < m_sel_col_start + m_sel_col_count - 1 )   // that was not last column
                txt += COL_SEP;

            if( doDelete && selected )
            {
                // Do NOT allow clear of things that can take strings but aren't textEntries
                // (ie: color swatches, textboxes, etc.).
                if( isTextEntry( row, col ) && !isReadOnly( row, col ) )
                {
                    bool hadContent = !tbl->GetValue( row, col ).IsEmpty();

                    tbl->SetValue( row, col, wxEmptyString );

                    if( hadContent )
                    {
                        wxGridEvent evt( m_grid->GetId(), wxEVT_GRID_CELL_CHANGED, m_grid, row, col );
                        m_grid->GetEventHandler()->ProcessEvent( evt );
                    }
                }
            }
        }
    }

    if( doCopy )
    {
        wxTheClipboard->SetData( new wxTextDataObject( txt ) );
        wxTheClipboard->Flush(); // Allow data to be available after closing KiCad
        wxTheClipboard->Close();
    }

    if( doDelete )
        m_grid->ForceRefresh();
}


void GRID_TRICKS::onUpdateUI( wxUpdateUIEvent& event )
{
    // Respect ROW selectionMode when moving cursor

    if( m_grid->GetSelectionMode() == wxGrid::wxGridSelectRows )
    {
        int cursorRow = m_grid->GetGridCursorRow();
        bool cursorInSelectedRow = false;

        for( int row : m_grid->GetSelectedRows() )
        {
            if( row == cursorRow )
            {
                cursorInSelectedRow = true;
                break;
            }
        }

        if( !cursorInSelectedRow && cursorRow >= 0 )
            m_grid->SelectRow( cursorRow );
    }

    event.Skip();
}
