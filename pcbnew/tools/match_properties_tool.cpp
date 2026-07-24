/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <tools/match_properties_tool.h>

#include <board.h>
#include <board_commit.h>
#include <collectors.h>
#include <dialog_shim.h>
#include <pgm_base.h>
#include <pcbnew_settings.h>
#include <properties/property_mgr.h>
#include <settings/settings_manager.h>
#include <tool/conditional_menu.h>
#include <tool/tool_manager.h>
#include <kiplatform/ui.h>
#include <preview_items/selection_area.h>
#include <status_popup.h>
#include <tools/hover_picker.h>
#include <tools/match_properties.h>
#include <tools/pcb_actions.h>
#include <tools/pcb_picker_tool.h>
#include <tools/pcb_selection_tool.h>
#include <view/view_controls.h>

#include <algorithm>

#include <wx/checklst.h>
#include <wx/clntdata.h>
#include <wx/dialog.h>
#include <wx/menu.h>
#include <wx/sizer.h>
#include <wx/stattext.h>


/// The properties this kind of source can offer.  Other kinds keep their own settings, which
/// this dialog does not show and must not clear.
class MATCH_PROPERTIES_DIALOG : public DIALOG_SHIM
{
public:
    MATCH_PROPERTIES_DIALOG( wxWindow* aParent, std::set<wxString>& aEnabled, const wxString& aFamily ) :
            DIALOG_SHIM( aParent, wxID_ANY, _( "Match Properties Settings" ), wxDefaultPosition, wxDefaultSize,
                         wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER ),
            m_enabled( aEnabled )
    {
        wxBoxSizer* sizer = new wxBoxSizer( wxVERTICAL );

        sizer->Add( new wxStaticText( this, wxID_ANY,
                                      wxString::Format( _( "Properties copied from %s:" ),
                                                        MATCH_PROPERTIES_CATALOG::FamilyLabel( aFamily ) ) ),
                    0, wxALL, 10 );

        m_properties = new wxCheckListBox( this, wxID_ANY );
        m_properties->SetMinSize( FromDIP( wxSize( 380, 320 ) ) );

        std::vector<std::pair<wxString, wxString>> rows;

        for( const wxString& key : MATCH_PROPERTIES_CATALOG::AllSafeKeys() )
        {
            if( MATCH_PROPERTIES_CATALOG::FamiliesFor( key ).contains( aFamily ) )
                rows.emplace_back( MATCH_PROPERTIES_CATALOG::PropertyLabel( key ), key );
        }

        std::ranges::sort( rows );

        for( const auto& [label, key] : rows )
        {
            const unsigned int index = m_properties->Append( label, new wxStringClientData( key ) );

            m_properties->Check( index, m_enabled.contains( key ) );
        }

        sizer->Add( m_properties, 1, wxEXPAND | wxLEFT | wxRIGHT, 10 );
        sizer->Add( CreateStdDialogButtonSizer( wxOK | wxCANCEL ), 0, wxEXPAND | wxALL, 10 );
        SetSizer( sizer );

        m_properties->Bind( wxEVT_CONTEXT_MENU, &MATCH_PROPERTIES_DIALOG::onContextMenu, this );

        SetupStandardButtons();
        finishDialogSettings();
    }

    bool TransferDataFromWindow() override
    {
        // Only the rows on screen are answered for.  Another kind's properties were never shown,
        // so clearing the set wholesale would silently turn them all off.
        for( unsigned int ii = 0; ii < m_properties->GetCount(); ++ii )
        {
            const wxString key = keyAt( ii );

            if( m_properties->IsChecked( ii ) )
                m_enabled.insert( key );
            else
                m_enabled.erase( key );
        }

        return true;
    }

private:
    wxString keyAt( unsigned int aIndex ) const
    {
        auto* data = static_cast<wxStringClientData*>( m_properties->GetClientObject( aIndex ) );

        return data ? data->GetData() : wxString();
    }

    void checkAll( bool aChecked )
    {
        for( unsigned int ii = 0; ii < m_properties->GetCount(); ++ii )
            m_properties->Check( ii, aChecked );
    }

    void onContextMenu( wxContextMenuEvent& aEvent )
    {
        wxMenu menu;

        enum
        {
            ID_CHECK_ALL = wxID_HIGHEST + 1,
            ID_UNCHECK_ALL
        };

        menu.Append( ID_CHECK_ALL, _( "Check All" ) );
        menu.Append( ID_UNCHECK_ALL, _( "Uncheck All" ) );

        switch( GetPopupMenuSelectionFromUser( menu ) )
        {
        case ID_CHECK_ALL:   checkAll( true ); break;
        case ID_UNCHECK_ALL: checkAll( false ); break;
        default:             break;
        }
    }

    wxCheckListBox*     m_properties;
    std::set<wxString>& m_enabled;
};


/// One item, and one the catalog can read.  With several there is no saying which one leads.
static bool hasMatchableSource( const SELECTION& aSelection )
{
    if( aSelection.Size() != 1 )
        return false;

    return !MATCH_PROPERTIES_CATALOG::Family( *aSelection.Front() ).IsEmpty();
}


MATCH_PROPERTIES_TOOL::MATCH_PROPERTIES_TOOL() :
        PCB_TOOL_BASE( "pcbnew.MatchProperties" )
{
}


bool MATCH_PROPERTIES_TOOL::Init()
{
    m_selectionTool = m_toolMgr->GetTool<PCB_SELECTION_TOOL>();

    // Settings live in the Edit menu only, like every other tool.
    CONDITIONAL_MENU& menu = m_selectionTool->GetToolMenu().GetMenu();

    menu.AddItem( PCB_ACTIONS::matchProperties, hasMatchableSource, 950 );

    return true;
}


const std::set<wxString>& MATCH_PROPERTIES_TOOL::enabledKeys()
{
    PCB_VIEWERS_SETTINGS_BASE* settings = frame()->GetViewerSettingsBase();

    return settings ? settings->m_MatchProperties : MATCH_PROPERTIES_CATALOG::DefaultKeys();
}


bool MATCH_PROPERTIES_TOOL::applyToTargets( const EDA_ITEM& aSource, const std::vector<EDA_ITEM*>& aTargets )
{
    if( aTargets.empty() )
        return false;

    const std::set<wxString>& enabled = enabledKeys();
    BOARD_COMMIT              commit( this );
    PROPERTY_COMMIT_HANDLER   handler( &commit );
    wxString                  error;
    int                       changed = 0;

    // CompatibleTargets() already reduced this to board items of the source family.
    for( EDA_ITEM* target : aTargets )
    {
        wxCHECK2( target->IsBOARD_ITEM(), continue );

        commit.Modify( static_cast<BOARD_ITEM*>( target ) );

        // Copy() stages on a clone and writes only if every value validates.  A refusal leaves
        // this target untouched.  Revert() undoes the earlier ones.
        MATCH_PROPERTIES_RESULT result = MATCH_PROPERTIES_CATALOG::Copy( aSource, *target, enabled );

        if( !result )
        {
            error = result.m_Error;
            break;
        }

        changed += result.m_Changed;
    }

    if( !error.IsEmpty() )
        frame()->ShowInfoBarError( error );
    else if( changed == 0 )
        frame()->ShowInfoBarMsg( _( "Nothing to copy.  The target already matches the source." ) );

    if( !error.IsEmpty() || changed == 0 )
    {
        commit.Revert();
        return false;
    }

    commit.Push( _( "Match Properties" ) );
    return true;
}


int MATCH_PROPERTIES_TOOL::Match( const TOOL_EVENT& aEvent )
{
    PCB_SELECTION& selection = m_selectionTool->GetSelection();

    // The tool copies from one item to many, so the one it copies from has to be settled before
    // it starts.  The context menu says the same, but the hotkey can arrive any time.
    if( selection.Size() != 1 )
    {
        frame()->ShowInfoBarError( _( "Select one item to copy properties from." ) );
        return 0;
    }

    EDA_ITEM* source = selection.Front();

    if( MATCH_PROPERTIES_CATALOG::Family( *source ).IsEmpty() )
    {
        frame()->ShowInfoBarError( _( "The selected source item has no properties to match." ) );
        return 0;
    }

    // Not a reason to refuse.  The settings only open from inside the tool, so there would
    // be no way left to turn anything on.
    if( !MATCH_PROPERTIES_CATALOG::AnyEnabledFor( *source, enabledKeys() ) )
    {
        frame()->ShowInfoBarMsg( _( "No properties are enabled for this kind of item.  Press <ctrl>+<,> to "
                                    "choose some." ) );
    }

    return runInteractive( aEvent, source->m_Uuid );
}


int MATCH_PROPERTIES_TOOL::runInteractive( const TOOL_EVENT& aEvent, const KIID& aSourceId )
{
    BOARD*            board = frame()->GetBoard();
    PCB_PICKER_TOOL*  picker = m_toolMgr->GetTool<PCB_PICKER_TOOL>();
    HOVER_PICKER      hover( m_toolMgr );
    STATUS_TEXT_POPUP statusPopup( frame() );
    const KIID        sourceId = aSourceId;
    bool              done = false;

    // Settled before the run started, so it can only go missing under a commit or an undo.
    auto currentSource = [&]() -> BOARD_ITEM*
    {
        return board->ResolveItem( sourceId, true );
    };

    // How much of the enabled set this kind of source can actually offer.  Saying the number
    // beats making the user open the dialog to find out nothing is turned on.
    auto enabledCount = [&]( const EDA_ITEM& aItem )
    {
        const wxString family = MATCH_PROPERTIES_CATALOG::Family( aItem );
        int            count = 0;

        for( const wxString& key : enabledKeys() )
        {
            if( MATCH_PROPERTIES_CATALOG::FamiliesFor( key ).contains( family ) )
                count++;
        }

        return count;
    };

    auto prompt = [&]()
    {
        if( BOARD_ITEM* source = currentSource() )
        {
            statusPopup.SetText( wxString::Format( _( "Click or drag over the items to copy to.\n"
                                                      "%d properties enabled; <ctrl>+<,> for settings." ),
                                                   enabledCount( *source ) ) );
        }
    };

    /// A target is anything the source has something in common with, bar the source itself.
    auto accepts = [&]( BOARD_ITEM* aSource )
    {
        return [aSource]( BOARD_ITEM& aItem )
        {
            return &aItem != aSource && MATCH_PROPERTIES_CATALOG::Compatible( *aSource, aItem );
        };
    };

    // The source keeps a mark of its own for the whole run.  Without it there is nothing on
    // screen saying what the properties are being copied from.
    auto updateHover = [&]( const VECTOR2I& aPointer )
    {
        BOARD_ITEM* source = currentSource();

        hover.ClearBrightening();
        statusPopup.Move( KIPLATFORM::UI::GetMousePosition() + wxPoint( 20, 20 ) );

        // An undo can take the source out from under the run.  Nothing is a target without one
        // to compare against.
        if( !source )
            return;

        hover.Brighten( source );

        if( BOARD_ITEM* target = hover.Pick( aPointer, accepts( source ) ) )
            hover.Brighten( target );
    };

    Activate();

    // The selection tool arms its disambiguation on button-down unless a tool owns the stack.
    // Without this the applying click also selects, which cancels the picker.
    frame()->PushTool( aEvent );

    m_selectionTool->ClearSelection();
    prompt();
    statusPopup.Popup();
    statusPopup.Move( KIPLATFORM::UI::GetMousePosition() + wxPoint( 20, 20 ) );
    canvas()->SetStatusPopup( statusPopup.GetPanel() );

    picker->SetCursor( KICURSOR::BULLSEYE );

    // Snapping on is what makes the picker honour the modifiers that turn it off again, so
    // <shift> and <ctrl> give a finer aim among crowded items.
    picker->SetSnapping( true );

    // The pointer only names an item, so the lines the snap system draws are just noise.
    picker->SetConstructionGeometry( false );
    picker->ClearHandlers();
    picker->SetMotionHandler(
            [&]( const VECTOR2D& aPointer )
            {
                updateHover( aPointer );
            } );
    picker->SetClickHandler(
            [&]( const VECTOR2D& aPointer ) -> bool
            {
                BOARD_ITEM* source = currentSource();

                if( !source )
                {
                    frame()->ShowInfoBarError( _( "The Match Properties source no longer exists." ) );
                    return false;
                }

                // The click takes what the hover lit, so what was shown is what is changed.
                BOARD_ITEM* target = hover.Pick( aPointer, accepts( source ) );

                if( !target )
                    return true;

                // The commit may replace either item, so let go of every highlight first.
                hover.ClearBrightening();
                applyToTargets( *source, MATCH_PROPERTIES_CATALOG::CompatibleTargets( *source, { target } ) );
                updateHover( aPointer );
                return true;
            } );

    // Light the items the box has caught so far, so a drag shows its reach as it grows.
    picker->SetAreaPreviewHandler(
            [&]( KIGFX::PREVIEW::SELECTION_AREA& aArea )
            {
                BOARD_ITEM* source = currentSource();

                if( !source )
                    return;

                std::vector<BOARD_ITEM*> lit{ source };

                for( BOARD_ITEM* item : m_selectionTool->CollectMultiple( aArea ) )
                {
                    if( item != source && MATCH_PROPERTIES_CATALOG::Compatible( *source, *item ) )
                        lit.push_back( item );
                }

                // The box grows by a little on every motion event, so most of this set was
                // already lit a moment ago.
                hover.BrightenOnly( lit );
            } );
    picker->SetAreaHandler(
            [&]() -> bool
            {
                BOARD_ITEM* source = currentSource();

                if( !source )
                    return false;

                const PCB_SELECTION&   selected = m_selectionTool->GetSelection();
                std::vector<EDA_ITEM*> candidates( selected.begin(), selected.end() );

                hover.ClearBrightening();
                applyToTargets( *source, MATCH_PROPERTIES_CATALOG::CompatibleTargets( *source, candidates ) );
                m_selectionTool->ClearSelection();
                return true;
            } );
    picker->SetCancelHandler(
            [&]()
            {
                hover.ClearBrightening();
            } );
    picker->SetFinalizeHandler(
            [&]( const int& )
            {
                hover.ClearBrightening();
                done = true;
            } );
    m_toolMgr->RunAction( ACTIONS::pickerSubTool );

    while( !done )
    {
        TOOL_EVENT* event = Wait();

        if( !event )
            break;

        // Handled here rather than passed on.  The tool is already inside this coroutine,
        // and letting the action dispatch to it would tear the picker down.
        if( event->IsAction( &PCB_ACTIONS::matchPropertiesSettings ) )
        {
            if( BOARD_ITEM* source = currentSource() )
                showSettingsDialog( *source );

            prompt();

            // The enabled set decides what a click would copy, so the mark under the pointer
            // may mean something different now.
            updateHover( getViewControls()->GetMousePosition() );
            continue;
        }

        event->SetPassEvent();
    }

    picker->ClearHandlers();
    hover.ClearBrightening();
    statusPopup.Hide();
    canvas()->SetStatusPopup( nullptr );
    m_toolMgr->RunAction( ACTIONS::selectionClear );

    if( EDA_ITEM* remaining = board->ResolveItem( sourceId, true ) )
        m_selectionTool->AddItemToSel( remaining, true );

    frame()->PopTool( aEvent );
    return 0;
}


void MATCH_PROPERTIES_TOOL::showSettingsDialog( const EDA_ITEM& aSource )
{
    PCB_VIEWERS_SETTINGS_BASE* settings = frame()->GetViewerSettingsBase();

    if( !settings )
        return;

    std::set<wxString>      enabled = settings->m_MatchProperties;
    MATCH_PROPERTIES_DIALOG dialog( frame(), enabled, MATCH_PROPERTIES_CATALOG::Family( aSource ) );

    if( dialog.ShowModal() == wxID_OK )
    {
        settings->m_MatchProperties = std::move( enabled );
        Pgm().GetSettingsManager().Save( settings );
    }
}


void MATCH_PROPERTIES_TOOL::setTransitions()
{
    Go( &MATCH_PROPERTIES_TOOL::Match, PCB_ACTIONS::matchProperties.MakeEvent() );
}
