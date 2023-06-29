/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2020-2021 CERN
 * Copyright (C) 2021-2023 KiCad Developers, see AUTHORS.txt for contributors.
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
#include <string_utils.h>   // WildCompareString
#include <kiway.h>
#include <refdes_utils.h>
#include <core/kicad_algo.h>
#include <dialog_change_symbols.h>
#include <sch_symbol.h>
#include <sch_edit_frame.h>
#include <sch_screen.h>
#include <schematic.h>
#include <template_fieldnames.h>
#include <widgets/wx_html_report_panel.h>
#include <sch_commit.h>

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


DIALOG_CHANGE_SYMBOLS::DIALOG_CHANGE_SYMBOLS( SCH_EDIT_FRAME* aParent, SCH_SYMBOL* aSymbol,
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
            m_matchBySelection->SetLabel( _( "Change selected symbol(s)" ) );

        m_newId->ChangeValue( UnescapeString( m_symbol->GetLibId().Format() ) );
        m_specifiedReference->ChangeValue( m_symbol->GetRef( currentSheet ) );
        m_specifiedValue->ChangeValue( UnescapeString( m_symbol->GetField( VALUE_FIELD )->GetText() ) );
        m_specifiedId->ChangeValue( UnescapeString( m_symbol->GetLibId().Format() ) );
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
        m_fieldsBox->Append( TEMPLATE_FIELDNAME::GetDefaultFieldName( i, DO_TRANSLATE ) );

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

    wxString okLabel = m_mode == MODE::CHANGE ? _( "Change" ) : _( "Update" );

    SetupStandardButtons( { { wxID_OK,     okLabel      },
                            { wxID_CANCEL, _( "Close" ) } } );

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
    event.Skip();       // Mandatory in wxFocusEvent
}


void DIALOG_CHANGE_SYMBOLS::onMatchIDKillFocus( wxFocusEvent& event )
{
    updateFieldsList();
    event.Skip();       // Mandatory in wxFocusEvent
}


void DIALOG_CHANGE_SYMBOLS::onNewLibIDKillFocus( wxFocusEvent& event )
{
    updateFieldsList();
    event.Skip();       // Mandatory in wxFocusEvent
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


wxString getLibIdValue( const wxTextCtrl* aCtrl )
{
    wxString rawValue = aCtrl->GetValue();
    wxString itemName;
    wxString libName = rawValue.BeforeFirst( ':', &itemName );

    return EscapeString( libName, CTX_LIBID ) + ':' + EscapeString( itemName, CTX_LIBID );
}


void DIALOG_CHANGE_SYMBOLS::launchMatchIdSymbolBrowser( wxCommandEvent& aEvent )
{
    wxString newName = getLibIdValue( m_specifiedId );

    KIWAY_PLAYER* frame = Kiway().Player( FRAME_SCH_VIEWER_MODAL, true, this );

    if( frame->ShowModal( &newName, this ) )
    {
        m_specifiedId->SetValue( UnescapeString( newName ) );
        updateFieldsList();
    }

    frame->Destroy();
}


void DIALOG_CHANGE_SYMBOLS::launchNewIdSymbolBrowser( wxCommandEvent& aEvent )
{
    wxString newName = getLibIdValue( m_newId );

    KIWAY_PLAYER* frame = Kiway().Player( FRAME_SCH_VIEWER_MODAL, true, this );

    if( frame->ShowModal( &newName, this ) )
    {
        m_newId->SetValue( UnescapeString( newName ) );
        updateFieldsList();
    }

    frame->Destroy();
}


void DIALOG_CHANGE_SYMBOLS::updateFieldsList()
{
    SCH_EDIT_FRAME* frame = dynamic_cast<SCH_EDIT_FRAME*>( GetParent() );

    wxCHECK( frame, /* void */ );

    SCH_SHEET_LIST  hierarchy = frame->Schematic().GetSheets();

    // Load non-mandatory fields from all matching symbols and their library symbols
    std::vector<SCH_FIELD*> fields;
    std::vector<LIB_FIELD*> libFields;
    std::set<wxString>      fieldNames;

    for( SCH_SHEET_PATH& instance : hierarchy )
    {
        SCH_SCREEN* screen = instance.LastScreen();

        wxCHECK2( screen, continue );

        for( SCH_ITEM* item : screen->Items().OfType( SCH_SYMBOL_T ) )
        {
            SCH_SYMBOL* symbol = dynamic_cast<SCH_SYMBOL*>( item );

            wxCHECK2( symbol, continue );

            if( !isMatch( symbol, &instance ) )
                continue;

            fields.clear();
            symbol->GetFields( fields, false );

            for( unsigned i = MANDATORY_FIELDS; i < fields.size(); ++i )
                fieldNames.insert( fields[i]->GetName() );

            if( m_mode == MODE::UPDATE && symbol->GetLibId().IsValid() )
            {
                LIB_SYMBOL* libSymbol = frame->GetLibSymbol( symbol->GetLibId() );

                if( libSymbol )
                {
                    std::unique_ptr<LIB_SYMBOL> flattenedSymbol = libSymbol->Flatten();

                    flattenedSymbol->GetFields( libFields );

                    for( unsigned i = MANDATORY_FIELDS; i < libFields.size(); ++i )
                        fieldNames.insert( libFields[i]->GetName() );

                    libFields.clear();  // flattenedSymbol is about to go out of scope...
                }
            }
        }
    }

    // Load non-mandatory fields from the change-to library symbol
    if( m_mode == MODE::CHANGE )
    {
        LIB_ID newId;

        newId.Parse( getLibIdValue( m_newId ) );

        if( newId.IsValid() )
        {
            LIB_SYMBOL* libSymbol = frame->GetLibSymbol( newId );

            if( libSymbol )
            {
                std::unique_ptr<LIB_SYMBOL> flattenedSymbol = libSymbol->Flatten();

                flattenedSymbol->GetFields( libFields );

                for( unsigned i = MANDATORY_FIELDS; i < libFields.size(); ++i )
                    fieldNames.insert( libFields[i]->GetName() );

                libFields.clear();  // flattenedSymbol is about to go out of scope...
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
    wxBusyCursor    dummy;
    SCH_EDIT_FRAME* parent = dynamic_cast<SCH_EDIT_FRAME*>( GetParent() );
    SCH_COMMIT      commit( parent );

    wxCHECK( parent, /* void */ );

    m_messagePanel->Clear();
    m_messagePanel->Flush( false );

    // Create the set of fields to be updated. Use non translated (canonical) names
    // for mandatory fields
    m_updateFields.clear();

    for( unsigned i = 0; i < m_fieldsBox->GetCount(); ++i )
    {
        if( m_fieldsBox->IsChecked( i ) )
        {
            if( i < MANDATORY_FIELDS )
            {
                LIB_FIELD dummy_field( i );
                m_updateFields.insert( dummy_field.GetCanonicalName() );
            }
            else
            {
                m_updateFields.insert( m_fieldsBox->GetString( i ) );
            }
        }
    }

    if( processMatchingSymbols( &commit) )
        commit.Push( m_mode == MODE::CHANGE ? _( "Change Symbols" ) : _( "Update Symbols" ) );

    m_messagePanel->Flush( false );
}


bool DIALOG_CHANGE_SYMBOLS::isMatch( SCH_SYMBOL* aSymbol, SCH_SHEET_PATH* aInstance )
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
        return aSymbol == m_symbol || aSymbol->IsSelected();
    }
    else if( m_matchByReference->GetValue() )
    {
        return WildCompareString( m_specifiedReference->GetValue(),
                                  UnescapeString( aSymbol->GetRef( aInstance, false ) ),
                                  false );
    }
    else if( m_matchByValue->GetValue() )
    {
        return WildCompareString( m_specifiedValue->GetValue(),
                                  UnescapeString( aSymbol->GetField( VALUE_FIELD )->GetText() ),
                                  false );
    }
    else if( m_matchById )
    {
        id.Parse( getLibIdValue( m_specifiedId ) );
        return aSymbol->GetLibId() == id;
    }

    return false;
}




int DIALOG_CHANGE_SYMBOLS::processMatchingSymbols( SCH_COMMIT* aCommit )
{
    SCH_EDIT_FRAME* frame = dynamic_cast<SCH_EDIT_FRAME*>( GetParent() );

    wxCHECK( frame, false );

    LIB_ID newId;
    wxString msg;
    int matchesProcessed = 0;
    SCH_SYMBOL* symbol = nullptr;
    SCH_SHEET_LIST hierarchy = frame->Schematic().GetSheets();

    if( m_mode == MODE::CHANGE )
    {
        newId.Parse( getLibIdValue( m_newId ) );

        if( !newId.IsValid() )
            return false;
    }

    std::map<SCH_SYMBOL*, SYMBOL_CHANGE_INFO> symbols;

    for( SCH_SHEET_PATH& instance : hierarchy )
    {
        SCH_SCREEN* screen = instance.LastScreen();

        wxCHECK2( screen, continue );

        // Fetch all the symbols that meet the change criteria.
        for( SCH_ITEM* item : screen->Items().OfType( SCH_SYMBOL_T ) )
        {
            symbol = static_cast<SCH_SYMBOL*>( item );

            wxCHECK2( symbol, continue );

            if( !isMatch( symbol, &instance ) )
                continue;

            if( m_mode == MODE::UPDATE )
                newId = symbol->GetLibId();

            auto it = symbols.find( symbol );

            if( it == symbols.end() )
            {
                SYMBOL_CHANGE_INFO info;

                info.m_Instances.emplace_back( instance );
                info.m_LibId = newId;
                symbols.insert( { symbol, info } );
            }
            else
            {
                it->second.m_Instances.emplace_back( instance );
            }
        }
    }

    if( symbols.size() > 0 )
        matchesProcessed += processSymbols( aCommit, symbols );
    else
        m_messagePanel->Report( _( "*** No symbols matching criteria found ***" ), RPT_SEVERITY_ERROR );

    frame->GetCurrentSheet().UpdateAllScreenReferences();

    return matchesProcessed;
}


int DIALOG_CHANGE_SYMBOLS::processSymbols( SCH_COMMIT* aCommit,
                                           const std::map<SCH_SYMBOL*, SYMBOL_CHANGE_INFO>& aSymbols )
{
    wxCHECK( !aSymbols.empty(), 0 );

    int             matchesProcessed = 0;
    SCH_EDIT_FRAME* frame = dynamic_cast<SCH_EDIT_FRAME*>( GetParent() );
    wxString        msg;

    wxCHECK( frame, 0 );

    std::map<SCH_SYMBOL*, SYMBOL_CHANGE_INFO>           symbols = aSymbols;
    std::map<SCH_SYMBOL*, SYMBOL_CHANGE_INFO>::iterator it = symbols.begin();

    // Remove all symbols that don't have a valid library symbol link or enough units to
    // satify the library symbol update.
    while( it != symbols.end() )
    {
        SCH_SYMBOL* symbol = it->first;

        wxCHECK2( symbol && it->second.m_LibId.IsValid(), continue );

        LIB_SYMBOL* libSymbol = frame->GetLibSymbol( it->second.m_LibId );

        if( !libSymbol )
        {
            msg = getSymbolReferences( *symbol, it->second.m_LibId );
            msg << wxT( ": " ) << _( "*** symbol not found ***" );
            m_messagePanel->Report( msg, RPT_SEVERITY_ERROR );
            it = symbols.erase( it );
            continue;
        }

        std::unique_ptr<LIB_SYMBOL> flattenedSymbol = libSymbol->Flatten();

        if( flattenedSymbol->GetUnitCount() < symbol->GetUnit() )
        {
            msg = getSymbolReferences( *symbol, it->second.m_LibId );
            msg << wxT( ": " ) << _( "*** new symbol has too few units ***" );
            m_messagePanel->Report( msg, RPT_SEVERITY_ERROR );
            it = symbols.erase( it );
        }
        else
        {
            ++it;
        }
    }

    // Removing the symbol needs to be done before the LIB_SYMBOL is changed to prevent stale
    // library symbols in the schematic file.
    for( const auto& [ symbol, symbol_change_info ] : symbols )
    {
        wxCHECK( symbol && !symbol_change_info.m_Instances.empty(), 0 );

        SCH_SCREEN* screen = symbol_change_info.m_Instances[0].LastScreen();

        wxCHECK( screen, 0 );

        screen->Remove( symbol );
        aCommit->Modify( symbol, screen );
    }

    for( const auto& [ symbol, symbol_change_info ] : symbols )
    {
        if( symbol_change_info.m_LibId != symbol->GetLibId() )
            symbol->SetLibId( symbol_change_info.m_LibId );

        LIB_SYMBOL*                 libSymbol = frame->GetLibSymbol( symbol_change_info.m_LibId );
        std::unique_ptr<LIB_SYMBOL> flattenedSymbol = libSymbol->Flatten();
        SCH_SCREEN*                 screen = symbol_change_info.m_Instances[0].LastScreen();

        symbol->SetLibSymbol( flattenedSymbol.release() );

        if( m_resetAttributes->GetValue() )
        {
            // Fetch the attributes from the *flattened* library symbol.  They are not supported
            // in derived symbols.
            symbol->SetExcludedFromBOM( symbol->GetLibSymbolRef()->GetExcludedFromBOM() );
            symbol->SetExcludedFromBoard( symbol->GetLibSymbolRef()->GetExcludedFromBoard() );
        }

        bool removeExtras = m_removeExtraBox->GetValue();
        bool resetVis = m_resetFieldVisibilities->GetValue();
        bool resetEffects = m_resetFieldEffects->GetValue();
        bool resetPositions = m_resetFieldPositions->GetValue();

        for( unsigned i = 0; i < symbol->GetFields().size(); ++i )
        {
            SCH_FIELD& field = symbol->GetFields()[i];
            LIB_FIELD* libField = nullptr;

            // Mandatory fields always exist in m_updateFields, but these names can be translated.
            // so use GetCanonicalName().
            if( !alg::contains( m_updateFields, field.GetCanonicalName() ) )
                continue;

            if( i < MANDATORY_FIELDS )
                libField = symbol->GetLibSymbolRef()->GetFieldById( (int) i );
            else
                libField = symbol->GetLibSymbolRef()->FindField( field.GetName() );

            if( libField )
            {
                bool resetText = libField->GetText().IsEmpty() ? m_resetEmptyFields->GetValue()
                                                               : m_resetFieldText->GetValue();

                if( resetText )
                {
                    if( i == REFERENCE_FIELD )
                    {
                        for( const SCH_SHEET_PATH& instance : symbol_change_info.m_Instances )
                        {
                            symbol->SetRef( &instance,
                                            UTIL::GetRefDesUnannotated( libField->GetText() ) );
                        }
                    }
                    else if( i == VALUE_FIELD )
                    {
                        symbol->SetValueFieldText( UnescapeString( libField->GetText() ) );
                    }
                    else if( i == FOOTPRINT_FIELD )
                    {
                        symbol->SetFootprintFieldText( libField->GetText() );
                    }
                    else
                    {
                        field.SetText( libField->GetText() );
                    }
                }

                if( resetVis )
                    field.SetVisible( libField->IsVisible() );

                if( resetEffects )
                {
                    // Careful: the visible bit and position are also set by SetAttributes()
                    bool     visible = field.IsVisible();
                    VECTOR2I pos = field.GetPosition();

                    field.SetAttributes( *libField );

                    field.SetVisible( visible );
                    field.SetPosition( pos );
                    field.SetNameShown( libField->IsNameShown() );
                    field.SetCanAutoplace( libField->CanAutoplace() );
                }

                if( resetPositions )
                    field.SetTextPos( symbol->GetPosition() + libField->GetTextPos() );
            }
            else if( i >= MANDATORY_FIELDS && removeExtras )
            {
                symbol->RemoveField( field.GetName() );
                i--;
            }
        }

        std::vector<LIB_FIELD*> libFields;
        symbol->GetLibSymbolRef()->GetFields( libFields );

        for( unsigned i = MANDATORY_FIELDS; i < libFields.size(); ++i )
        {
            const LIB_FIELD& libField = *libFields[i];

            if( !alg::contains( m_updateFields, libField.GetCanonicalName() ) )
                continue;

            if( !symbol->FindField( libField.GetName(), false ) )
            {
                wxString   fieldName = libField.GetCanonicalName();
                SCH_FIELD  newField( VECTOR2I( 0, 0 ), symbol->GetFieldCount(), symbol,
                                     fieldName );
                SCH_FIELD* schField = symbol->AddField( newField );

                // Careful: the visible bit and position are also set by SetAttributes()
                schField->SetAttributes( libField );
                schField->SetText( libField.GetText() );
                schField->SetTextPos( symbol->GetPosition() + libField.GetTextPos() );
            }

            if( resetPositions && frame->eeconfig()->m_AutoplaceFields.enable )
                symbol->AutoAutoplaceFields( screen );
        }

        symbol->SetSchSymbolLibraryName( wxEmptyString );
        screen->Append( symbol );
        frame->GetCanvas()->GetView()->Update( symbol );

        msg = getSymbolReferences( *symbol, symbol_change_info.m_LibId );
        msg += wxS( ": OK" );
        m_messagePanel->Report( msg, RPT_SEVERITY_ACTION );
        matchesProcessed +=1;
    }

    return matchesProcessed;
}


wxString DIALOG_CHANGE_SYMBOLS::getSymbolReferences( SCH_SYMBOL& aSymbol, const LIB_ID& aNewId )
{
    wxString msg;
    wxString references;
    LIB_ID oldId = aSymbol.GetLibId();

    for( const SCH_SYMBOL_INSTANCE& instance : aSymbol.GetInstanceReferences() )
    {
        if( references.IsEmpty() )
            references = instance.m_Reference;
        else
            references += wxT( " " ) + instance.m_Reference;
    }

    if( m_mode == MODE::UPDATE )
    {
        if( aSymbol.GetInstanceReferences().size() == 1 )
        {
            msg.Printf( _( "Update symbol %s from '%s' to '%s'" ),
                        references,
                        UnescapeString( oldId.Format() ),
                        UnescapeString( aNewId.Format() ) );
        }
        else
        {
            msg.Printf( _( "Update symbols %s from '%s' to '%s'" ),
                        references,
                        UnescapeString( oldId.Format() ),
                        UnescapeString( aNewId.Format() ) );
        }
    }
    else
    {
        if( aSymbol.GetInstanceReferences().size() == 1 )
        {
            msg.Printf( _( "Change symbol %s from '%s' to '%s'" ),
                        references,
                        UnescapeString( oldId.Format() ),
                        UnescapeString( aNewId.Format() ) );
        }
        else
        {
            msg.Printf( _( "Change symbols %s from '%s' to '%s'" ),
                        references,
                        UnescapeString( oldId.Format() ),
                        UnescapeString( aNewId.Format() ) );
        }
    }

    return msg;
}
