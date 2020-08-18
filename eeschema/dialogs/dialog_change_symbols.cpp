/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2020 CERN
 *
 * @author Wayne Stambaugh <stambaughw@gmail.com>
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

#include <algorithm>

#include <fctsys.h>
#include <bitmaps.h>
#include <kicad_string.h>   // WildCompareString
#include <kiway.h>
#include <lib_id.h>

#include <dialog_change_symbols.h>
#include <sch_component.h>
#include <sch_edit_frame.h>
#include <sch_screen.h>
#include <sch_sheet_path.h>
#include <schematic.h>
#include <template_fieldnames.h>
#include <wx_html_report_panel.h>


DIALOG_CHANGE_SYMBOLS::DIALOG_CHANGE_SYMBOLS( SCH_EDIT_FRAME* aParent, SCH_COMPONENT* aSymbol,
        MODE aMode ) :
    DIALOG_CHANGE_SYMBOLS_BASE( aParent ),
    m_symbol( aSymbol),
    m_mode( aMode )
{
    wxASSERT( aParent );
    wxString label;
    wxString verb  = ( m_mode == MODE::UPDATE ) ? _( "Update" ) : _( "Change" );

    label.Printf( m_matchAll->GetLabel(), verb );

    if( m_mode == MODE::UPDATE )
    {
        m_matchAll->SetLabel( label );
        SetTitle( _( "Update Symbol(s) from Library" ) );
        m_newIdSizer->Show( false );
    }
    else
    {
        SetTitle( _( "Change Symbol(s)" ) );
        m_matchSizer->FindItem( m_matchAll )->Show( false );
    }

    if( m_symbol )
    {
        label.Printf( m_matchBySelection->GetLabel(), verb );
        m_matchBySelection->SetLabel( label );
        m_newId->AppendText( FROM_UTF8( m_symbol->GetLibId().Format().c_str() ) );
        m_specifiedReference->ChangeValue(
                m_symbol->GetRef( &aParent->Schematic().CurrentSheet() ) );
        m_specifiedValue->ChangeValue( m_symbol->GetField( VALUE )->GetText() );
        m_specifiedId->ChangeValue( FROM_UTF8( m_symbol->GetLibId().Format().c_str() ) );
    }
    else
    {
        m_matchSizer->FindItem( m_matchBySelection )->Show( false );
    }

    m_matchIdBrowserButton->SetBitmap( KiBitmap( small_library_xpm ) );
    m_newIdBrowserButton->SetBitmap( KiBitmap( small_library_xpm ) );

    label.Printf( m_matchByReference->GetLabel(), verb );
    m_matchByReference->SetLabel( label );
    label.Printf( m_matchByValue->GetLabel(), verb );
    m_matchByValue->SetLabel( label );
    label.Printf( m_matchById->GetLabel(), verb );
    m_matchById->SetLabel( label );

    m_matchSizer->SetEmptyCellSize( wxSize( 0, 0 ) );
    m_matchSizer->Layout();

    m_messagePanel->SetLazyUpdate( true );

    if( aSymbol && aSymbol->IsSelected() )
    {
        m_matchBySelection->SetValue( true );
    }
    else
    {
        if( aMode == MODE::UPDATE )
            m_matchAll->SetValue( true );
        else
            m_matchByReference->SetValue( true );
    }

    // DIALOG_SHIM needs a unique hash_key because classname is not sufficient
    // because the update and change versions of this dialog have different controls.
    m_hash_key = TO_UTF8( GetTitle() );

    // Ensure m_closeButton (with id = wxID_CANCEL) has the right label
    // (to fix automatic renaming of button label )
    m_sdbSizerCancel->SetLabel( _( "Close" ) );
    m_sdbSizerOK->SetLabel( verb );
    m_sdbSizerOK->SetDefault();

    // Now all widgets have the size fixed, call FinishDialogSettings
    FinishDialogSettings();
}


void DIALOG_CHANGE_SYMBOLS::onMatchByReference( wxCommandEvent& aEvent )
{
    m_specifiedReference->SetFocus();
}


void DIALOG_CHANGE_SYMBOLS::onMatchByValue( wxCommandEvent& aEvent )
{
    m_specifiedValue->SetFocus();
}


void DIALOG_CHANGE_SYMBOLS::onMatchById( wxCommandEvent& aEvent )
{
    m_specifiedId->SetFocus();
}


DIALOG_CHANGE_SYMBOLS::~DIALOG_CHANGE_SYMBOLS()
{
}


void DIALOG_CHANGE_SYMBOLS::launchMatchIdSymbolBrowser( wxCommandEvent& aEvent )
{
    wxString newName = m_specifiedId->GetValue();

    KIWAY_PLAYER* frame = Kiway().Player( FRAME_SCH_VIEWER_MODAL, true );

    if( frame->ShowModal( &newName, this ) )
        m_specifiedId->SetValue( newName );

    frame->Destroy();
}


void DIALOG_CHANGE_SYMBOLS::launchNewIdSymbolBrowser( wxCommandEvent& aEvent )
{
    wxString newName = m_newId->GetValue();

    KIWAY_PLAYER* frame = Kiway().Player( FRAME_SCH_VIEWER_MODAL, true );

    if( frame->ShowModal( &newName, this ) )
        m_newId->SetValue( newName );

    frame->Destroy();
}


void DIALOG_CHANGE_SYMBOLS::onOkButtonClicked( wxCommandEvent& aEvent )
{
    wxBusyCursor dummy;
    SCH_EDIT_FRAME* parent = dynamic_cast<SCH_EDIT_FRAME*>( GetParent() );

    wxCHECK( parent, /* void */ );

    m_messagePanel->Clear();
    m_messagePanel->Flush( false );

    if( processMatchingSymbols() )
    {
        parent->TestDanglingEnds();   // This will also redraw the changed symbols.
        parent->OnModify();
    }

    m_messagePanel->Flush( false );
}


bool DIALOG_CHANGE_SYMBOLS::isMatch( SCH_COMPONENT* aSymbol, SCH_SHEET_PATH* aInstance )
{
    LIB_ID id;

    wxCHECK( aSymbol, false );

    SCH_EDIT_FRAME* frame = dynamic_cast<SCH_EDIT_FRAME*>( GetParent() );

    wxCHECK( frame, false );

    if( m_matchAll->GetValue() )
        return true;
    else if( m_matchBySelection->GetValue() )
        return aSymbol == m_symbol;
    else if( m_matchByReference->GetValue() )
        return WildCompareString( m_specifiedReference->GetValue(), aSymbol->GetRef( aInstance ),
                false );
    else if( m_matchByValue->GetValue() )
        return WildCompareString( m_specifiedValue->GetValue(),
                aSymbol->GetField( VALUE )->GetText(), false );
    else if( m_matchById )
    {
        id.Parse( m_specifiedId->GetValue(), LIB_ID::ID_SCH );
        return aSymbol->GetLibId() == id;
    }

    return false;
}


bool DIALOG_CHANGE_SYMBOLS::processMatchingSymbols()
{
    SCH_EDIT_FRAME* frame = dynamic_cast<SCH_EDIT_FRAME*>( GetParent() );

    wxCHECK( frame, false );

    LIB_ID newId;
    bool appendToUndo = false;
    bool changed = false;
    SCH_SHEET_LIST hierarchy = frame->Schematic().GetSheets();

    if( m_mode == MODE::CHANGE )
    {
        newId.Parse( m_newId->GetValue(), LIB_ID::ID_SCH );

        if( !newId.IsValid() )
            return false;
    }

    // Use map as a cheap and dirty way to ensure library symbols are not updated multiple
    // times in complex hierachies.
    std::map<SCH_COMPONENT*, SCH_SCREEN*> symbolsToProcess;

    for( SCH_SHEET_PATH& instance : hierarchy )
    {
        SCH_SCREEN* screen = instance.LastScreen();

        wxCHECK2( screen, continue );

        for( SCH_ITEM* item : screen->Items().OfType( SCH_COMPONENT_T ) )
        {
            SCH_COMPONENT* symbol = dynamic_cast<SCH_COMPONENT*>( item );

            wxCHECK2( symbol, continue );

            if( !isMatch( symbol, &instance ) )
                continue;

            // Shared symbols always have identical library symbols so don't add duplicates.
            symbolsToProcess[symbol] = screen;
        }
    }

    for( auto i : symbolsToProcess )
    {
        SCH_COMPONENT* symbol = i.first;

        if( m_mode == MODE::UPDATE )
        {
            if( processSymbol( symbol, i.second, symbol->GetLibId(), appendToUndo ) )
                changed = true;
        }
        else
        {
            if( processSymbol( symbol, i.second, newId, appendToUndo ) )
                changed = true;
        }

        if( changed )
            appendToUndo = true;
    }

    return changed;
}


bool DIALOG_CHANGE_SYMBOLS::processSymbol( SCH_COMPONENT* aSymbol, SCH_SCREEN* aScreen,
        const LIB_ID& aNewId, bool aAppendToUndo )
{
    wxCHECK( aSymbol, false );
    wxCHECK( aScreen, false );
    wxCHECK( aNewId.IsValid(), false );

    SCH_EDIT_FRAME* frame = dynamic_cast<SCH_EDIT_FRAME*>( GetParent() );

    wxCHECK( frame, false );

    LIB_ID    oldId = aSymbol->GetLibId();
    wxString  msg;
    wxString  references;

    for( COMPONENT_INSTANCE_REFERENCE instance : aSymbol->GetInstanceReferences() )
    {
        if( references.IsEmpty() )
            references = instance.m_Reference;
        else
            references += " " + instance.m_Reference;
    }

    msg.Printf( _( "%s %s \"%s\" from \"%s\" to \"%s\"" ),
            ( m_mode == MODE::UPDATE ) ? _( "Update" ) : _( "Change" ),
            ( aSymbol->GetInstanceReferences().size() == 1 ) ? _( "symbol" ) : _( "symbols" ),
            references,
            oldId.Format().c_str(),
            aNewId.Format().c_str() );

    LIB_PART* libSymbol = frame->GetLibPart( aNewId );

    if( !libSymbol )
    {
        msg << ": " << _( "*** symbol not found ***" );
        m_messagePanel->Report( msg, RPT_SEVERITY_ERROR );
        return false;
    }

    std::unique_ptr< LIB_PART > flattenedSymbol = libSymbol->Flatten();

    if( flattenedSymbol->GetUnitCount() < aSymbol->GetUnit() )
    {
        msg << ": " << _( "*** new symbol has too few units ***" );
        m_messagePanel->Report( msg, RPT_SEVERITY_ERROR );
        return false;
    }

    // Removing the symbol needs to be done before the LIB_PART is changed to prevent stale
    // library symbols in the schematic file.
    aScreen->Remove( aSymbol );
    frame->SaveCopyInUndoList( aScreen, aSymbol, UR_CHANGED, aAppendToUndo );

    if( aNewId != aSymbol->GetLibId() )
        aSymbol->SetLibId( aNewId );

    aSymbol->SetLibSymbol( flattenedSymbol.release() );
    aScreen->Append( aSymbol );
    frame->GetCanvas()->GetView()->Update( aSymbol );

    msg += ": OK";
    m_messagePanel->Report( msg, RPT_SEVERITY_ACTION );

    return true;
}


