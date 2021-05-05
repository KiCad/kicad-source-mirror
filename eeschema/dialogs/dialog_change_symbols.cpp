/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2020-2021 CERN
 * Copyright (C) 2021 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <bitmaps.h>
#include <kicad_string.h>   // WildCompareString
#include <kiway.h>
#include <lib_id.h>
#include <refdes_utils.h>
#include <core/kicad_algo.h>
#include <dialog_change_symbols.h>
#include <sch_symbol.h>
#include <sch_edit_frame.h>
#include <sch_screen.h>
#include <sch_sheet_path.h>
#include <schematic.h>
#include <template_fieldnames.h>
#include <wx_html_report_panel.h>

bool g_selectRefDes = false;
bool g_selectValue  = false;
                                // { change, update }
bool g_removeExtraFields[2]      = { false,  false  };
bool g_resetEmptyFields[2]       = { false,  false  };
bool g_resetFieldText[2]         = { true,   true   };
bool g_resetFieldVisibilities[2] = { true,   false  };
bool g_resetFieldEffects[2]      = { true,   false  };
bool g_resetFieldPositions[2]    = { true,   false  };
bool g_resetAttributes[2]        = { true,   false  };


DIALOG_CHANGE_SYMBOLS::DIALOG_CHANGE_SYMBOLS( SCH_EDIT_FRAME* aParent, SCH_COMPONENT* aSymbol,
                                              MODE aMode ) :
    DIALOG_CHANGE_SYMBOLS_BASE( aParent ),
    m_symbol( aSymbol),
    m_mode( aMode )
{
    wxASSERT( aParent );

    if( m_mode == MODE::UPDATE )
    {
        m_newIdSizer->Show( false );
    }
    else
    {
        m_matchAll->SetLabel( _( "Change all symbols in schematic" ) );
        SetTitle( _( "Change Symbols" ) );
        m_matchSizer->FindItem( m_matchAll )->Show( false );
    }

    if( m_symbol )
    {
        SCH_SHEET_PATH* currentSheet = &aParent->Schematic().CurrentSheet();

        if( m_mode == MODE::CHANGE )
            m_matchBySelection->SetLabel( _( "Change selected Symbol" ) );

        m_newId->AppendText( FROM_UTF8( m_symbol->GetLibId().Format().c_str() ) );
        m_specifiedReference->ChangeValue( m_symbol->GetRef( currentSheet ) );
        m_specifiedValue->ChangeValue( m_symbol->GetValue( currentSheet, false ) );
        m_specifiedId->ChangeValue( FROM_UTF8( m_symbol->GetLibId().Format().c_str() ) );
    }
    else
    {
        m_matchSizer->FindItem( m_matchBySelection )->Show( false );
    }

    m_matchIdBrowserButton->SetBitmap( KiBitmap( BITMAPS::small_library ) );
    m_newIdBrowserButton->SetBitmap( KiBitmap( BITMAPS::small_library ) );

    if( m_mode == MODE::CHANGE )
    {
        m_matchByReference->SetLabel( _( "Change symbols matching reference designator:" ) );
        m_matchByValue->SetLabel( _( "Change symbols matching value:" ) );
        m_matchById->SetLabel( _( "Change symbols matching library identifier:" ) );
    }

    m_matchSizer->SetEmptyCellSize( wxSize( 0, 0 ) );
    m_matchSizer->Layout();

    for( int i = 0; i < MANDATORY_FIELDS; ++i )
    {
        m_fieldsBox->Append( TEMPLATE_FIELDNAME::GetDefaultFieldName( i ) );

        if( i == REFERENCE_FIELD )
            m_fieldsBox->Check( i, g_selectRefDes );
        else if( i == VALUE_FIELD )
            m_fieldsBox->Check( i, g_selectValue );
        else
            m_fieldsBox->Check( i, true );
    }

    m_messagePanel->SetLazyUpdate( true );
    m_messagePanel->SetFileName( Prj().GetProjectPath() + wxT( "report.txt" ) );

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

    updateFieldsList();

    if( m_mode == MODE::CHANGE )
    {
        m_updateFieldsSizer->GetStaticBox()->SetLabel( _( "Update Fields" ) );
        m_removeExtraBox->SetLabel( _( "Remove fields if not in new symbol" ) );
        m_resetEmptyFields->SetLabel( _( "Reset fields if empty in new symbol" ) );
        m_resetFieldText->SetLabel( _( "Update field text" ) );
        m_resetFieldVisibilities->SetLabel( _( "Update field visibilities" ) );
        m_resetFieldEffects->SetLabel( _( "Update field sizes and styles" ) );
        m_resetFieldPositions->SetLabel( _( "Update field positions" ) );
        m_resetAttributes->SetLabel( _( "Update symbol attributes" ) );
    }

    m_removeExtraBox->SetValue( g_removeExtraFields[ (int) m_mode ] );
    m_resetEmptyFields->SetValue( g_resetEmptyFields[ (int) m_mode ] );
    m_resetFieldText->SetValue( g_resetFieldText[ (int) m_mode ] );
    m_resetFieldVisibilities->SetValue( g_resetFieldVisibilities[ (int) m_mode ] );
    m_resetFieldEffects->SetValue( g_resetFieldEffects[ (int) m_mode ] );
    m_resetFieldPositions->SetValue( g_resetFieldPositions[ (int) m_mode ] );
    m_resetAttributes->SetValue( g_resetAttributes[ (int) m_mode ] );

    // DIALOG_SHIM needs a unique hash_key because classname is not sufficient
    // because the update and change versions of this dialog have different controls.
    m_hash_key = TO_UTF8( GetTitle() );

    // Ensure m_closeButton (with id = wxID_CANCEL) has the right label
    // (to fix automatic renaming of button label )
    m_sdbSizerCancel->SetLabel( _( "Close" ) );

    if( m_mode == MODE::CHANGE )
        m_sdbSizerOK->SetLabel( _( "Change" ) );
    else
        m_sdbSizerOK->SetLabel( _( "Update" ) );

    m_sdbSizerOK->SetDefault();

    // Now all widgets have the size fixed, call FinishDialogSettings
    finishDialogSettings();
}


void DIALOG_CHANGE_SYMBOLS::onMatchByAll( wxCommandEvent& aEvent )
{
    updateFieldsList();
}


void DIALOG_CHANGE_SYMBOLS::onMatchBySelected( wxCommandEvent& aEvent )
{
    updateFieldsList();
}


void DIALOG_CHANGE_SYMBOLS::onMatchByReference( wxCommandEvent& aEvent )
{
    updateFieldsList();
    m_specifiedReference->SetFocus();
}


void DIALOG_CHANGE_SYMBOLS::onMatchByValue( wxCommandEvent& aEvent )
{
    updateFieldsList();
    m_specifiedValue->SetFocus();
}


void DIALOG_CHANGE_SYMBOLS::onMatchById( wxCommandEvent& aEvent )
{
    updateFieldsList();
    m_specifiedId->SetFocus();
}


void DIALOG_CHANGE_SYMBOLS::onMatchTextKillFocus( wxFocusEvent& event )
{
    updateFieldsList();
}


DIALOG_CHANGE_SYMBOLS::~DIALOG_CHANGE_SYMBOLS()
{
    g_selectRefDes = m_fieldsBox->IsChecked( REFERENCE_FIELD );
    g_selectValue = m_fieldsBox->IsChecked( VALUE_FIELD );

    g_removeExtraFields[ (int) m_mode ] = m_removeExtraBox->GetValue();
    g_resetEmptyFields[ (int) m_mode ] = m_resetEmptyFields->GetValue();
    g_resetFieldText[ (int) m_mode ] = m_resetFieldText->GetValue();
    g_resetFieldVisibilities[ (int) m_mode ] = m_resetFieldVisibilities->GetValue();
    g_resetFieldEffects[ (int) m_mode ] = m_resetFieldEffects->GetValue();
    g_resetFieldPositions[ (int) m_mode ] = m_resetFieldPositions->GetValue();
    g_resetAttributes[ (int) m_mode ] = m_resetAttributes->GetValue();
}


void DIALOG_CHANGE_SYMBOLS::launchMatchIdSymbolBrowser( wxCommandEvent& aEvent )
{
    wxString newName = m_specifiedId->GetValue();

    KIWAY_PLAYER* frame = Kiway().Player( FRAME_SCH_VIEWER_MODAL, true );

    if( frame->ShowModal( &newName, this ) )
    {
        m_specifiedId->SetValue( newName );
        updateFieldsList();
    }

    frame->Destroy();
}


void DIALOG_CHANGE_SYMBOLS::launchNewIdSymbolBrowser( wxCommandEvent& aEvent )
{
    wxString newName = m_newId->GetValue();

    KIWAY_PLAYER* frame = Kiway().Player( FRAME_SCH_VIEWER_MODAL, true );

    if( frame->ShowModal( &newName, this ) )
    {
        m_newId->SetValue( newName );
        updateFieldsList();
    }

    frame->Destroy();
}


void DIALOG_CHANGE_SYMBOLS::updateFieldsList()
{
    SCH_EDIT_FRAME* frame = dynamic_cast<SCH_EDIT_FRAME*>( GetParent() );

    wxCHECK( frame, /* void */ );

    SCH_SHEET_LIST  hierarchy = frame->Schematic().GetSheets();

    // Load non-mandatory fields from all matching symbols and their library parts
    std::vector<SCH_FIELD*> fields;
    std::vector<LIB_FIELD*> libFields;
    std::set<wxString>      fieldNames;

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

            fields.clear();
            symbol->GetFields( fields, false );

            for( unsigned i = MANDATORY_FIELDS; i < fields.size(); ++i )
                fieldNames.insert( fields[i]->GetName() );

            if( m_mode == MODE::UPDATE && symbol->GetLibId().IsValid() )
            {
                LIB_PART* libSymbol = frame->GetLibPart( symbol->GetLibId() );

                if( libSymbol )
                {
                    std::unique_ptr<LIB_PART> flattenedPart = libSymbol->Flatten();

                    flattenedPart->GetFields( libFields );

                    for( unsigned i = MANDATORY_FIELDS; i < libFields.size(); ++i )
                        fieldNames.insert( libFields[i]->GetName() );

                    libFields.clear();  // flattenedPart is about to go out of scope...
                }
            }
        }
    }

    // Load non-mandatory fields from the change-to library part
    if( m_mode == MODE::CHANGE )
    {
        LIB_ID newId;

        newId.Parse( m_newId->GetValue() );

        if( newId.IsValid() )
        {
            LIB_PART* libSymbol = frame->GetLibPart( newId );

            if( libSymbol )
            {
                std::unique_ptr<LIB_PART> flattenedPart = libSymbol->Flatten();

                flattenedPart->GetFields( libFields );

                for( unsigned i = MANDATORY_FIELDS; i < libFields.size(); ++i )
                    fieldNames.insert( libFields[i]->GetName() );

                libFields.clear();  // flattenedPart is about to go out of scope...
            }
        }
    }

    // Update the listbox widget
    for( unsigned i = m_fieldsBox->GetCount() - 1; i >= MANDATORY_FIELDS; --i )
        m_fieldsBox->Delete( i );

    for( const wxString& fieldName : fieldNames )
        m_fieldsBox->Append( fieldName );

    for( unsigned i = MANDATORY_FIELDS; i < m_fieldsBox->GetCount(); ++i )
        m_fieldsBox->Check( i, true );
}


void DIALOG_CHANGE_SYMBOLS::checkAll( bool aCheck )
{
    for( unsigned i = 0; i < m_fieldsBox->GetCount(); ++i )
        m_fieldsBox->Check( i, aCheck );
}


void DIALOG_CHANGE_SYMBOLS::onOkButtonClicked( wxCommandEvent& aEvent )
{
    wxBusyCursor dummy;
    SCH_EDIT_FRAME* parent = dynamic_cast<SCH_EDIT_FRAME*>( GetParent() );

    wxCHECK( parent, /* void */ );

    m_messagePanel->Clear();
    m_messagePanel->Flush( false );

    // Create the set of fields to be updated
    m_updateFields.clear();

    for( unsigned i = 0; i < m_fieldsBox->GetCount(); ++i )
    {
        if( m_fieldsBox->IsChecked( i ) )
            m_updateFields.insert( m_fieldsBox->GetString( i ) );
    }

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
    {
        return true;
    }
    else if( m_matchBySelection->GetValue() )
    {
        return aSymbol == m_symbol;
    }
    else if( m_matchByReference->GetValue() )
    {
        return WildCompareString( m_specifiedReference->GetValue(),
                                  aSymbol->GetRef( aInstance, false ), false );
    }
    else if( m_matchByValue->GetValue() )
    {
        return WildCompareString( m_specifiedValue->GetValue(),
                                  aSymbol->GetValue( aInstance, false ), false );
    }
    else if( m_matchById )
    {
        id.Parse( m_specifiedId->GetValue() );
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
        newId.Parse( m_newId->GetValue() );

        if( !newId.IsValid() )
            return false;
    }

    for( SCH_SHEET_PATH& instance : hierarchy )
    {
        SCH_SCREEN* screen = instance.LastScreen();

        wxCHECK2( screen, continue );

        std::vector<SCH_COMPONENT*> symbols;

        for( SCH_ITEM* item : screen->Items().OfType( SCH_COMPONENT_T ) )
            symbols.push_back( static_cast<SCH_COMPONENT*>( item ) );

        for( SCH_COMPONENT* symbol : symbols )
        {
            if( !isMatch( symbol, &instance ) )
                continue;

            if( m_mode == MODE::UPDATE )
            {
                if( processSymbol( symbol, &instance, symbol->GetLibId(), appendToUndo ) )
                    changed = true;
            }
            else
            {
                if( processSymbol( symbol, &instance, newId, appendToUndo ) )
                    changed = true;
            }

            if( changed )
                appendToUndo = true;
        }
    }

    frame->GetCurrentSheet().UpdateAllScreenReferences();

    return changed;
}


bool DIALOG_CHANGE_SYMBOLS::processSymbol( SCH_COMPONENT* aSymbol, const SCH_SHEET_PATH* aInstance,
                                           const LIB_ID& aNewId, bool aAppendToUndo )
{
    wxCHECK( aSymbol, false );
    wxCHECK( aNewId.IsValid(), false );

    SCH_EDIT_FRAME* frame = dynamic_cast<SCH_EDIT_FRAME*>( GetParent() );
    SCH_SCREEN*     screen = aInstance->LastScreen();

    wxCHECK( frame, false );

    LIB_ID    oldId = aSymbol->GetLibId();
    wxString  msg;
    wxString  references;

    for( SYMBOL_INSTANCE_REFERENCE instance : aSymbol->GetInstanceReferences() )
    {
        if( references.IsEmpty() )
            references = instance.m_Reference;
        else
            references += " " + instance.m_Reference;
    }

    if( m_mode == MODE::UPDATE )
    {
        if( aSymbol->GetInstanceReferences().size() == 1 )
        {
            msg.Printf( _( "Update symbol %s from '%s' to '%s'" ),
                        references,
                        oldId.Format().c_str(),
                        aNewId.Format().c_str() );
        }
        else
        {
            msg.Printf( _( "Update symbols %s from '%s' to '%s'" ),
                        references,
                        oldId.Format().c_str(),
                        aNewId.Format().c_str() );
        }
    }
    else
    {
        if( aSymbol->GetInstanceReferences().size() == 1 )
        {
            msg.Printf( _( "Change symbol %s from '%s' to '%s'" ),
                        references,
                        oldId.Format().c_str(),
                        aNewId.Format().c_str() );
        }
        else
        {
            msg.Printf( _( "Change symbols %s from '%s' to '%s'" ),
                        references,
                        oldId.Format().c_str(),
                        aNewId.Format().c_str() );
        }
    }

    LIB_PART* libSymbol = frame->GetLibPart( aNewId );

    if( !libSymbol )
    {
        msg << ": " << _( "*** symbol not found ***" );
        m_messagePanel->Report( msg, RPT_SEVERITY_ERROR );
        return false;
    }

    std::unique_ptr<LIB_PART> flattenedSymbol = libSymbol->Flatten();

    if( flattenedSymbol->GetUnitCount() < aSymbol->GetUnit() )
    {
        msg << ": " << _( "*** new symbol has too few units ***" );
        m_messagePanel->Report( msg, RPT_SEVERITY_ERROR );
        return false;
    }

    // Removing the symbol needs to be done before the LIB_PART is changed to prevent stale
    // library symbols in the schematic file.
    screen->Remove( aSymbol );
    frame->SaveCopyInUndoList( screen, aSymbol, UNDO_REDO::CHANGED, aAppendToUndo );

    if( aNewId != aSymbol->GetLibId() )
        aSymbol->SetLibId( aNewId );

    aSymbol->SetLibSymbol( flattenedSymbol.release() );

    if( m_resetAttributes->GetValue() )
    {
        aSymbol->SetIncludeInBom( libSymbol->GetIncludeInBom() );
        aSymbol->SetIncludeOnBoard( libSymbol->GetIncludeOnBoard() );
    }

    bool removeExtras = m_removeExtraBox->GetValue();
    bool resetVis = m_resetFieldVisibilities->GetValue();
    bool resetEffects = m_resetFieldEffects->GetValue();
    bool resetPositions = m_resetFieldPositions->GetValue();

    for( unsigned i = 0; i < aSymbol->GetFields().size(); ++i )
    {
        SCH_FIELD& field = aSymbol->GetFields()[i];
        LIB_FIELD* libField = nullptr;

        if( !alg::contains( m_updateFields, field.GetName() ) )
            continue;

        if( i < MANDATORY_FIELDS )
            libField = libSymbol->GetFieldById( (int) i );
        else
            libField = libSymbol->FindField( field.GetName() );

        if( libField )
        {
            bool resetText = libField->GetText().IsEmpty() ? m_resetEmptyFields->GetValue()
                                                           : m_resetFieldText->GetValue();

            if( resetText )
            {
                if( i == REFERENCE_FIELD )
                    aSymbol->SetRef( aInstance, UTIL::GetRefDesUnannotated( libField->GetText() ) );
                else if( i == VALUE_FIELD )
                    aSymbol->SetValue( aInstance, libField->GetText() );
                else if( i == FOOTPRINT_FIELD )
                    aSymbol->SetFootprint( aInstance, libField->GetText() );
                else
                    field.SetText( libField->GetText() );
            }

            if( resetVis )
                field.SetVisible( libField->IsVisible() );

            if( resetEffects )
            {
                // Careful: the visible bit and position are also in Effects
                bool    visible = field.IsVisible();
                wxPoint pos = field.GetPosition();

                field.SetEffects( *libField );

                field.SetVisible( visible );
                field.SetPosition( pos );
            }

            if( resetPositions )
                field.SetTextPos( aSymbol->GetPosition() + libField->GetTextPos() );
        }
        else if( i >= MANDATORY_FIELDS && removeExtras )
        {
            aSymbol->RemoveField( field.GetName() );
            i--;
        }
    }

    std::vector<LIB_FIELD*> libFields;
    libSymbol->GetFields( libFields );

    for( unsigned i = MANDATORY_FIELDS; i < libFields.size(); ++i )
    {
        const LIB_FIELD& libField = *libFields[i];

        if( !alg::contains( m_updateFields, libField.GetName() ) )
            continue;

        if( !aSymbol->FindField( libField.GetName(), false ) )
        {
            wxString   fieldName = libField.GetCanonicalName();
            SCH_FIELD  newField( wxPoint( 0, 0), aSymbol->GetFieldCount(), aSymbol, fieldName );
            SCH_FIELD* schField = aSymbol->AddField( newField );

            schField->SetEffects( libField );
            schField->SetText( libField.GetText() );
            schField->SetTextPos( aSymbol->GetPosition() + libField.GetTextPos() );
        }
    }

    aSymbol->SetSchSymbolLibraryName( wxEmptyString );
    screen->Append( aSymbol );
    frame->GetCanvas()->GetView()->Update( aSymbol );

    msg += ": OK";
    m_messagePanel->Report( msg, RPT_SEVERITY_ACTION );

    return true;
}


