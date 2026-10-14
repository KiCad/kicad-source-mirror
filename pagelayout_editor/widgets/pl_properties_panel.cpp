/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
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

#include "pl_properties_panel.h"

#include <dialogs/html_message_box.h>
#include <dialog_shim.h>
#include <drawing_sheet/ds_draw_item.h>
#include <project.h>
#include <properties/pg_editors.h>
#include <properties/pg_properties.h>
#include <properties/property.h>
#include <properties/property_mgr.h>
#include <scintilla_tricks.h>
#include <tool/tool_manager.h>
#include <tool/actions.h>
#include <wx/hyperlink.h>
#include <wx/propgrid/advprops.h>
#include <wx/sizer.h>
#include <wx/stc/stc.h>

#include "pl_editor_frame.h"
#include "pl_properties.h"
#include "tools/pl_selection_tool.h"

#include <set>


namespace
{
/** Retains multiline editing and drawing-sheet/project variable completion. */
class PL_TEXT_PROPERTY : public wxLongStringProperty
{
public:
    explicit PL_TEXT_PROPERTY( PL_EDITOR_FRAME* aFrame ) :
            wxLongStringProperty( _( "Text" ), wxS( "Text" ) ),
            m_frame( aFrame )
    {
    }

protected:
    bool DisplayEditorDialog( wxPropertyGrid* aGrid, wxVariant& aValue ) override
    {
        DIALOG_SHIM       dlg( m_frame, wxID_ANY, _( "Edit Text" ), wxDefaultPosition, wxDefaultSize,
                               wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER );
        wxBoxSizer*       sizer = new wxBoxSizer( wxVERTICAL );
        wxStyledTextCtrl* editor = new wxStyledTextCtrl( &dlg, wxID_ANY );
        editor->SetMinSize( dlg.FromDIP( wxSize( 450, 200 ) ) );
        editor->SetEOLMode( wxSTC_EOL_LF );
#ifdef _WIN32
        editor->SetTechnology( wxSTC_TECHNOLOGY_DIRECTWRITE );
#endif
        DS_DATA_ITEM_TEXT text( wxEmptyString );
        text.m_FullText = aValue.IsNull() ? wxString() : aValue.GetString();
        text.ReplaceAntiSlashSequence();
        editor->SetText( text.m_FullText );
        editor->EmptyUndoBuffer();
        SCINTILLA_TRICKS tricks( editor, wxT( "{}" ), false );

        auto autocomplete = [&]( wxStyledTextEvent& )
        {
            wxArrayString tokens;
            int           pos = editor->GetCurrentPos();
            int           start = editor->WordStartPosition( pos, true );
            wxString      partial;

            if( start >= 2 && editor->GetCharAt( start - 2 ) == '$' && editor->GetCharAt( start - 1 ) == '{' )
            {
                DS_DRAW_ITEM_LIST::GetTextVars( &tokens );
                partial = editor->GetTextRange( start, pos );

                for( const auto& [key, value] : m_frame->Prj().GetTextVars() )
                    tokens.push_back( key );
            }

            tricks.DoAutocomplete( partial, tokens );
        };

        editor->Bind( wxEVT_STC_CHARADDED, autocomplete );
        editor->Bind( wxEVT_STC_AUTOCOMP_CHAR_DELETED, autocomplete );
        sizer->Add( editor, 1, wxEXPAND | wxALL, 10 );

        wxHyperlinkCtrl* help = new wxHyperlinkCtrl( &dlg, wxID_ANY, _( "Text Variables" ), wxEmptyString );
        help->Bind( wxEVT_HYPERLINK,
                    [&]( wxHyperlinkEvent& )
                    {
                        HTML_MESSAGE_BOX helpDlg( &dlg, _( "Predefined Keywords" ) );
                        helpDlg.AddHTML_Text( _( "Text can include variables using ${keyword} notation." ) );
                        wxArrayString tokens;
                        DS_DRAW_ITEM_LIST::GetTextVars( &tokens );
                        wxString list;

                        for( const wxString& token : tokens )
                            list += token + wxS( "\n" );

                        helpDlg.ListSet( list );
                        helpDlg.ShowModal();
                    } );
        sizer->Add( help, 0, wxLEFT | wxRIGHT | wxBOTTOM, 10 );
        sizer->Add( dlg.CreateStdDialogButtonSizer( wxOK | wxCANCEL ), 0, wxEXPAND | wxALL, 10 );
        dlg.SetSizerAndFit( sizer );
        dlg.Centre();
        editor->SetFocus();

        if( dlg.ShowModal() != wxID_OK )
            return false;

        aValue = editor->GetText();
        return true;
    }

private:
    PL_EDITOR_FRAME* m_frame;
};
} // namespace


PL_PROPERTIES_PANEL::PL_PROPERTIES_PANEL( PL_EDITOR_FRAME* aFrame ) :
        PROPERTIES_PANEL( aFrame, aFrame ),
        m_editor( aFrame ),
        m_unitEditor( nullptr ),
        m_pageProperties( std::make_unique<PL_PAGE_PROPERTIES>( DS_DATA_MODEL::GetTheInstance() ) )
{
    SetMinSize( FromDIP( wxSize( 150, -1 ) ) );
    PROPERTY_MANAGER::Instance().Rebuild();

    auto& editors = wxPGGlobalVars->m_mapEditorClasses;
    auto  it = editors.find( PG_UNIT_EDITOR::BuildEditorName( aFrame ) );

    if( it != editors.end() )
    {
        m_unitEditor = static_cast<PG_UNIT_EDITOR*>( it->second );
        m_unitEditor->UpdateFrame( aFrame );
    }
    else
    {
        m_unitEditor =
                static_cast<PG_UNIT_EDITOR*>( wxPropertyGrid::RegisterEditorClass( new PG_UNIT_EDITOR( aFrame ) ) );
    }

    if( editors.find( PG_CHECKBOX_EDITOR::EDITOR_NAME ) == editors.end() )
        wxPropertyGrid::RegisterEditorClass( new PG_CHECKBOX_EDITOR() );

    if( editors.find( PG_COLOR_EDITOR::EDITOR_NAME ) == editors.end() )
        wxPropertyGrid::RegisterEditorClass( new PG_COLOR_EDITOR() );

    addCategoryButton(
            _HKI( "Defaults" ), _( "Reset to Defaults" ), BITMAPS::small_refresh,
            [this]()
            {
                resetDefaults();
            },
            [this]()
            {
                return m_itemProperties.empty();
            } );
}


PL_PROPERTIES_PANEL::~PL_PROPERTIES_PANEL()
{
    m_deferredRebuildTimer.Stop();
    m_deferredSelection.Clear();
    m_unitEditor->UpdateFrame( nullptr );
}


void PL_PROPERTIES_PANEL::UpdateData()
{
    if( !m_editor->GetToolManager() )
        return;

    // A deferred grid rebuild must never retain adapters for deleted or reloaded definitions.
    m_deferredRebuildTimer.Stop();
    m_deferredSelection.Clear();
    m_selection.Clear();
    m_itemProperties.clear();

    std::set<DS_DATA_ITEM*> seen;

    for( EDA_ITEM* item : m_editor->GetCurrentSelection() )
    {
        DS_DATA_ITEM* peer = static_cast<DS_DRAW_ITEM_BASE*>( item )->GetPeer();

        if( peer && seen.insert( peer ).second )
        {
            m_itemProperties.push_back( std::make_unique<PL_ITEM_PROPERTIES>( peer ) );
            m_selection.Add( m_itemProperties.back().get() );
        }
    }

    if( m_selection.Empty() )
        m_selection.Add( m_pageProperties.get() );

    PROPERTIES_PANEL::rebuildProperties( m_selection );
}


void PL_PROPERTIES_PANEL::rebuildProperties( const SELECTION& aSelection )
{
    // The base class may defer a rebuild until a property event has finished.  Resolve the
    // current selection again; rendered drawing-sheet items may have been replaced meanwhile.
    UpdateData();
}


wxPGProperty* PL_PROPERTIES_PANEL::createPGProperty( const PROPERTY_BASE* aProperty ) const
{
    wxPGProperty* property;

    if( aProperty->Name() == wxS( "Text" ) )
    {
        property = new PL_TEXT_PROPERTY( m_editor );
        property->SetClientData( const_cast<PROPERTY_BASE*>( aProperty ) );
        property->SetHelpString( wxGetTranslation( aProperty->HelpString() ) );
    }
    else if( aProperty->Name() == wxS( "Comment" ) )
    {
        // Drawing-sheet strings are stored literally, without the schematic escape encoding.
        property = new wxStringProperty( wxGetTranslation( aProperty->Name() ), aProperty->Name() );
        property->SetClientData( const_cast<PROPERTY_BASE*>( aProperty ) );
    }
    else
    {
        property = PGPropertyFactory( aProperty, m_editor );
    }

    if( auto* color = dynamic_cast<PGPROPERTY_COLOR4D*>( property ) )
        color->SetBackgroundColor( m_editor->GetDrawBgColor() );

    return property;
}


void PL_PROPERTIES_PANEL::valueChanging( wxPropertyGridEvent& aEvent )
{
    if( m_SuppressGridChangeEvents )
        return;

    PROPERTY_MANAGER& mgr = PROPERTY_MANAGER::Instance();

    for( EDA_ITEM* item : m_selection )
    {
        PROPERTY_BASE* property = mgr.GetProperty( item, aEvent.GetPropertyName() );

        if( !property || !property->Available( item ) || !property->Writeable( item ) )
        {
            aEvent.Veto();
            return;
        }

        if( auto error = property->Validate( aEvent.GetPropertyValue().GetAny(), item ) )
        {
            m_editor->ShowInfoBarError( wxString::Format( wxS( "%s: %s" ), wxGetTranslation( property->Name() ),
                                                          error->get()->Format( m_editor ) ) );
            aEvent.Veto();
            return;
        }
    }

    aEvent.Skip();
}


void PL_PROPERTIES_PANEL::valueChanged( wxPropertyGridEvent& aEvent )
{
    if( m_SuppressGridChangeEvents || m_selection.Empty() )
        return;

    m_editor->SaveCopyInUndoList();
    PROPERTY_MANAGER& mgr = PROPERTY_MANAGER::Instance();

    for( EDA_ITEM* item : m_selection )
    {
        if( PROPERTY_BASE* property = mgr.GetProperty( item, aEvent.GetPropertyName() ) )
            item->Set( property, aEvent.GetPropertyValue() );
    }

    refreshDrawing();
}


void PL_PROPERTIES_PANEL::resetDefaults()
{
    m_editor->SaveCopyInUndoList();
    DS_DATA_MODEL& model = DS_DATA_MODEL::GetTheInstance();
    model.m_DefaultTextSize = VECTOR2D( TB_DEFAULT_TEXTSIZE, TB_DEFAULT_TEXTSIZE );
    model.m_DefaultLineWidth = 0.15;
    model.m_DefaultTextThickness = 0.15;
    refreshDrawing();
}


void PL_PROPERTIES_PANEL::refreshDrawing()
{
    m_editor->OnModify();

    // End point editing before rebuilding the rendered items.  Its handles otherwise retain
    // a pointer to a draw item deleted by DisplayDrawingSheet().
    m_editor->GetToolManager()->ProcessEvent( EVENTS::UnselectedEvent );
    m_editor->GetCanvas()->DisplayDrawingSheet();

    // Reducing a repeat count can remove the selected copy.  Keep its definition selected
    // through the first surviving copy so the user can continue editing it.
    PL_SELECTION_TOOL* selectionTool = m_editor->GetToolManager()->GetTool<PL_SELECTION_TOOL>();

    for( const auto& properties : m_itemProperties )
    {
        const auto& items = properties->GetItem()->GetDrawItems();

        if( !items.empty()
            && std::ranges::none_of( items,
                                     []( DS_DRAW_ITEM_BASE* aItem )
                                     {
                                         return aItem->IsSelected();
                                     } ) )
        {
            selectionTool->AddItemToSel( items.front(), true );
        }
    }

    m_editor->GetCanvas()->Refresh();
    UpdateData();
    m_editor->GetToolManager()->PostEvent( EVENTS::SelectedEvent );
}
