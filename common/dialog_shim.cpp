/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2012 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright (C) 2023 CERN
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
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#include <app_monitor.h>
#include <dialog_shim.h>
#include <core/ignore.h>
#include <kiway_player.h>
#include <kiway.h>
#include <pgm_base.h>
#include <project/project_local_settings.h>
#include <property_holder.h>
#include <settings/settings_manager.h>
#include <tool/tool_manager.h>
#include <kiplatform/ui.h>
#include <widgets/unit_binder.h>

#include <wx/display.h>
#include <wx/evtloop.h>
#include <wx/app.h>
#include <wx/event.h>
#include <wx/grid.h>
#include <wx/propgrid/propgrid.h>
#include <wx/checklst.h>
#include <wx/dataview.h>
#include <wx/bmpbuttn.h>
#include <wx/textctrl.h>
#include <wx/stc/stc.h>
#include <wx/combobox.h>
#include <wx/odcombo.h>
#include <wx/choice.h>
#include <wx/checkbox.h>
#include <wx/spinctrl.h>
#include <wx/splitter.h>
#include <wx/radiobox.h>
#include <wx/radiobut.h>
#include <wx/variant.h>

#include <algorithm>
#include <functional>
#include <nlohmann/json.hpp>
#include <typeinfo>

BEGIN_EVENT_TABLE( DIALOG_SHIM, wxDialog )
    EVT_CHAR_HOOK( DIALOG_SHIM::OnCharHook )
END_EVENT_TABLE()


DIALOG_SHIM::DIALOG_SHIM( wxWindow* aParent, wxWindowID id, const wxString& title,
                          const wxPoint& pos, const wxSize& size, long style, const wxString& name ) :
        wxDialog( aParent, id, title, pos, size, style, name ),
        KIWAY_HOLDER( nullptr, KIWAY_HOLDER::DIALOG ),
        m_units( EDA_UNITS::MM ),
        m_useCalculatedSize( false ),
        m_firstPaintEvent( true ),
        m_initialFocusTarget( nullptr ),
        m_isClosing( false ),
        m_qmodal_loop( nullptr ),
        m_qmodal_showing( false ),
        m_qmodal_parent_disabler( nullptr ),
        m_parentFrame( nullptr ),
        m_userPositioned( false ),
        m_userResized( false ),
        m_handlingUndoRedo( false )
{
    KIWAY_HOLDER* kiwayHolder = nullptr;
    m_initialSize = size;

    if( aParent )
    {
        kiwayHolder = dynamic_cast<KIWAY_HOLDER*>( aParent );

        while( !kiwayHolder && aParent->GetParent() )
        {
            aParent = aParent->GetParent();
            kiwayHolder = dynamic_cast<KIWAY_HOLDER*>( aParent );
        }
    }

    // Inherit units from parent
    if( kiwayHolder && kiwayHolder->GetType() == KIWAY_HOLDER::FRAME )
        m_units = static_cast<EDA_BASE_FRAME*>( kiwayHolder )->GetUserUnits();
    else if( kiwayHolder && kiwayHolder->GetType() == KIWAY_HOLDER::DIALOG )
        m_units = static_cast<DIALOG_SHIM*>( kiwayHolder )->GetUserUnits();

    // Don't mouse-warp after a dialog run from the context menu
    if( kiwayHolder && kiwayHolder->GetType() == KIWAY_HOLDER::FRAME )
    {
        m_parentFrame = static_cast<EDA_BASE_FRAME*>( kiwayHolder );
        TOOL_MANAGER* toolMgr = m_parentFrame->GetToolManager();

        if( toolMgr && toolMgr->IsContextMenuActive() )
            toolMgr->VetoContextMenuMouseWarp();
    }

    // Set up the message bus
    if( kiwayHolder )
        SetKiway( this, &kiwayHolder->Kiway() );

    if( HasKiway() )
        Kiway().SetBlockingDialog( this );

    Bind( wxEVT_CLOSE_WINDOW, &DIALOG_SHIM::OnCloseWindow, this );
    Bind( wxEVT_BUTTON, &DIALOG_SHIM::OnButton, this );
    Bind( wxEVT_SIZE, &DIALOG_SHIM::OnSize, this );
    Bind( wxEVT_MOVE, &DIALOG_SHIM::OnMove, this );
    Bind( wxEVT_INIT_DIALOG, &DIALOG_SHIM::onInitDialog, this );

#ifdef __WINDOWS__
    // On Windows, the app top windows can be brought to the foreground (at least temporarily)
    // in certain circumstances such as when calling an external tool in Eeschema BOM generation.
    // So set the parent frame (if exists) to top window to avoid this annoying behavior.
    if( kiwayHolder && kiwayHolder->GetType() == KIWAY_HOLDER::FRAME )
        Pgm().App().SetTopWindow( (EDA_BASE_FRAME*) kiwayHolder );
#endif

    Bind( wxEVT_PAINT, &DIALOG_SHIM::OnPaint, this );

    wxString msg = wxString::Format( "Opening dialog %s", GetTitle() );
    APP_MONITOR::AddNavigationBreadcrumb( msg, "dialog.open" );
}


DIALOG_SHIM::~DIALOG_SHIM()
{
    m_isClosing = true;

    Unbind( wxEVT_CLOSE_WINDOW, &DIALOG_SHIM::OnCloseWindow, this );
    Unbind( wxEVT_BUTTON, &DIALOG_SHIM::OnButton, this );
    Unbind( wxEVT_PAINT, &DIALOG_SHIM::OnPaint, this );
    Unbind( wxEVT_SIZE, &DIALOG_SHIM::OnSize, this );
    Unbind( wxEVT_MOVE, &DIALOG_SHIM::OnMove, this );
    Unbind( wxEVT_INIT_DIALOG, &DIALOG_SHIM::onInitDialog, this );

    std::function<void( wxWindowList& )> disconnectFocusHandlers =
            [&]( wxWindowList& children )
            {
                for( wxWindow* child : children )
                {
                    if( wxTextCtrl* textCtrl = dynamic_cast<wxTextCtrl*>( child ) )
                    {
                        textCtrl->Disconnect( wxEVT_SET_FOCUS, wxFocusEventHandler( DIALOG_SHIM::onChildSetFocus ),
                                              nullptr, this );
                    }
                    else if( wxStyledTextCtrl* scintilla = dynamic_cast<wxStyledTextCtrl*>( child ) )
                    {
                        scintilla->Disconnect( wxEVT_SET_FOCUS, wxFocusEventHandler( DIALOG_SHIM::onChildSetFocus ),
                                               nullptr, this );
                    }
                    else
                    {
                        disconnectFocusHandlers( child->GetChildren() );
                    }
                }
            };

    disconnectFocusHandlers( GetChildren() );

    std::function<void( wxWindowList& )> disconnectUndoRedoHandlers =
            [&]( wxWindowList& children )
            {
                for( wxWindow* child : children )
                {
                    if( wxTextCtrl* textCtrl = dynamic_cast<wxTextCtrl*>( child ) )
                    {
                        textCtrl->Unbind( wxEVT_TEXT, &DIALOG_SHIM::onCommandEvent, this );
                    }
                    else if( wxStyledTextCtrl* scintilla = dynamic_cast<wxStyledTextCtrl*>( child ) )
                    {
                        scintilla->Unbind( wxEVT_STC_CHANGE, &DIALOG_SHIM::onStyledTextChanged, this );
                    }
                    else if( wxComboBox* combo = dynamic_cast<wxComboBox*>( child ) )
                    {
                        combo->Unbind( wxEVT_TEXT, &DIALOG_SHIM::onCommandEvent, this );
                        combo->Unbind( wxEVT_COMBOBOX, &DIALOG_SHIM::onCommandEvent, this );
                    }
                    else if( wxChoice* choice = dynamic_cast<wxChoice*>( child ) )
                    {
                        choice->Unbind( wxEVT_CHOICE, &DIALOG_SHIM::onCommandEvent, this );
                    }
                    else if( wxCheckBox* check = dynamic_cast<wxCheckBox*>( child ) )
                    {
                        check->Unbind( wxEVT_CHECKBOX, &DIALOG_SHIM::onCommandEvent, this );
                    }
                    else if( wxSpinCtrl* spin = dynamic_cast<wxSpinCtrl*>( child ) )
                    {
                        spin->Unbind( wxEVT_SPINCTRL, &DIALOG_SHIM::onSpinEvent, this );
                        spin->Unbind( wxEVT_TEXT, &DIALOG_SHIM::onCommandEvent, this );
                    }
                    else if( wxSpinCtrlDouble* spinD = dynamic_cast<wxSpinCtrlDouble*>( child ) )
                    {
                        spinD->Unbind( wxEVT_SPINCTRLDOUBLE, &DIALOG_SHIM::onSpinDoubleEvent, this );
                        spinD->Unbind( wxEVT_TEXT, &DIALOG_SHIM::onCommandEvent, this );
                    }
                    else if( wxRadioButton* radio = dynamic_cast<wxRadioButton*>( child ) )
                    {
                        radio->Unbind( wxEVT_RADIOBUTTON, &DIALOG_SHIM::onCommandEvent, this );
                    }
                    else if( wxRadioBox* radioBox = dynamic_cast<wxRadioBox*>( child ) )
                    {
                        radioBox->Unbind( wxEVT_RADIOBOX, &DIALOG_SHIM::onCommandEvent, this );
                    }
                    else if( wxGrid* grid = dynamic_cast<wxGrid*>( child ) )
                    {
                        grid->Unbind( wxEVT_GRID_CELL_CHANGED, &DIALOG_SHIM::onGridCellChanged, this );
                    }
                    else if( wxPropertyGrid* propGrid = dynamic_cast<wxPropertyGrid*>( child ) )
                    {
                        propGrid->Unbind( wxEVT_PG_CHANGED, &DIALOG_SHIM::onPropertyGridChanged, this );
                    }
                    else if( wxCheckListBox* checkList = dynamic_cast<wxCheckListBox*>( child ) )
                    {
                        checkList->Unbind( wxEVT_CHECKLISTBOX, &DIALOG_SHIM::onCommandEvent, this );
                    }
                    else if( wxDataViewListCtrl* dataList = dynamic_cast<wxDataViewListCtrl*>( child ) )
                    {
                        dataList->Unbind( wxEVT_DATAVIEW_ITEM_VALUE_CHANGED, &DIALOG_SHIM::onDataViewListChanged, this );
                    }
                    else
                    {
                        disconnectUndoRedoHandlers( child->GetChildren() );
                    }
                }
            };

    disconnectUndoRedoHandlers( GetChildren() );

    // if the dialog is quasi-modal, this will end its event loop
    if( IsQuasiModal() )
        EndQuasiModal( wxID_CANCEL );

    if( HasKiway() )
        Kiway().SetBlockingDialog( nullptr );

    delete m_qmodal_parent_disabler;
}


void DIALOG_SHIM::onInitDialog( wxInitDialogEvent& aEvent )
{
    LoadControlState();
    aEvent.Skip();
}


void DIALOG_SHIM::finishDialogSettings()
{
    // must be called from the constructor of derived classes,
    // when all widgets are initialized, and therefore their size fixed

    // SetSizeHints fixes the minimal size of sizers in the dialog
    // (SetSizeHints calls Fit(), so no need to call it)
    GetSizer()->SetSizeHints( this );
}


void DIALOG_SHIM::setSizeInDU( int x, int y )
{
    wxSize sz( x, y );
    SetSize( ConvertDialogToPixels( sz ) );
}


int DIALOG_SHIM::horizPixelsFromDU( int x ) const
{
    wxSize sz( x, 0 );
    return ConvertDialogToPixels( sz ).x;
}


int DIALOG_SHIM::vertPixelsFromDU( int y ) const
{
    wxSize sz( 0, y );
    return ConvertDialogToPixels( sz ).y;
}


// our hashtable is an implementation secret, don't need or want it in a header file
#include <hashtables.h>
#include <typeinfo>
#include <grid_tricks.h>


void DIALOG_SHIM::SetPosition( const wxPoint& aNewPosition )
{
    wxDialog::SetPosition( aNewPosition );
}


bool DIALOG_SHIM::Show( bool show )
{
    bool ret;

    if( show )
    {
#ifndef __WINDOWS__
        wxDialog::Raise();  // Needed on OS X and some other window managers (i.e. Unity)
#endif
        ret = wxDialog::Show( show );

        wxRect      savedDialogRect;
        std::string key = m_hash_key.empty() ? GetTitle().ToStdString() : m_hash_key;

        if( COMMON_SETTINGS* settings = Pgm().GetCommonSettings() )
        {
            auto dlgIt = settings->m_dialogControlValues.find( key );

            if( dlgIt != settings->m_dialogControlValues.end() )
            {
                auto geoIt = dlgIt->second.find( "__geometry" );

                if( geoIt != dlgIt->second.end() && geoIt->second.is_object() )
                {
                    const nlohmann::json& g = geoIt->second;
                    savedDialogRect.SetPosition( wxPoint( g.value( "x", 0 ), g.value( "y", 0 ) ) );
                    savedDialogRect.SetSize( wxSize( g.value( "w", 500 ), g.value( "h", 300 ) ) );
                }
            }
        }

        if( savedDialogRect.GetSize().x != 0 && savedDialogRect.GetSize().y != 0 )
        {
            if( m_useCalculatedSize )
            {
                SetSize( savedDialogRect.GetPosition().x, savedDialogRect.GetPosition().y,
                         wxDialog::GetSize().x, wxDialog::GetSize().y, 0 );
            }
            else
            {
                SetSize( savedDialogRect.GetPosition().x, savedDialogRect.GetPosition().y,
                         std::max( wxDialog::GetSize().x, savedDialogRect.GetSize().x ),
                         std::max( wxDialog::GetSize().y, savedDialogRect.GetSize().y ), 0 );
            }

#ifdef __WXMAC__
            if( m_parent != nullptr )
            {
                if( wxDisplay::GetFromPoint( m_parent->GetPosition() )
                    != wxDisplay::GetFromPoint( savedDialogRect.GetPosition() ) )
                {
                    Centre();
                }
            }
#endif

        }
        else if( m_initialSize != wxDefaultSize )
        {
            SetSize( m_initialSize );
            Centre();
        }

        if( wxDisplay::GetFromWindow( this ) == wxNOT_FOUND )
            Centre();

        m_userPositioned = false;
        m_userResized = false;

        KIPLATFORM::UI::EnsureVisible( this );
    }
    else
    {

#ifdef __WXMAC__
        if ( m_eventLoop )
            m_eventLoop->Exit( GetReturnCode() );   // Needed for APP-MODAL dlgs on OSX
#endif

        ret = wxDialog::Show( show );

        SaveControlState();

        if( m_parent )
            m_parent->SetFocus();
    }

    return ret;
}


void DIALOG_SHIM::resetSize()
{
    if( COMMON_SETTINGS* settings = Pgm().GetCommonSettings() )
    {
        std::string key = m_hash_key.empty() ? GetTitle().ToStdString() : m_hash_key;

        auto dlgIt = settings->m_dialogControlValues.find( key );

        if( dlgIt == settings->m_dialogControlValues.end() )
            return;

        dlgIt->second.erase( "__geometry" );
    }
}


void DIALOG_SHIM::OnSize( wxSizeEvent& aEvent )
{
    m_userResized = true;
    aEvent.Skip();
}


void DIALOG_SHIM::OnMove( wxMoveEvent& aEvent )
{
    m_userPositioned = true;
    aEvent.Skip();
}


bool DIALOG_SHIM::Enable( bool enable )
{
    // so we can do logging of this state change:
    return wxDialog::Enable( enable );
}


std::string DIALOG_SHIM::generateKey( const wxWindow* aWin ) const
{
    auto getSiblingIndex =
            []( const wxWindow* parent, const wxWindow* child )
            {
                wxString childClass = child->GetClassInfo()->GetClassName();
                int      index = 0;

                for( const wxWindow* sibling : parent->GetChildren() )
                {
                    if( sibling->GetClassInfo()->GetClassName() != childClass )
                        continue;

                    if( sibling == child )
                        break;

                    index++;
                }

                return index;
            };

    auto makeKey =
            [&]( const wxWindow* window )
            {
                std::string key = wxString( window->GetClassInfo()->GetClassName() ).ToStdString();

                if( window->GetParent() )
                    key += "_" + std::to_string( getSiblingIndex( window->GetParent(), window ) );

                return key;
            };

    std::string key = makeKey( aWin );

    for( const wxWindow* parent = aWin->GetParent(); parent && parent != this; parent = parent->GetParent() )
        key = makeKey( parent ) + key;

    return key;
}


void DIALOG_SHIM::SaveControlState()
{
    COMMON_SETTINGS* settings = Pgm().GetCommonSettings();

    if( !settings )
        return;

    std::string dialogKey = m_hash_key.empty() ? GetTitle().ToStdString() : m_hash_key;
    std::map<std::string, nlohmann::json>& dlgMap = settings->m_dialogControlValues[ dialogKey ];

    wxRect rect( GetPosition(), GetSize() );
    nlohmann::json geom;
    geom[ "x" ] = rect.GetX();
    geom[ "y" ] = rect.GetY();
    geom[ "w" ] = rect.GetWidth();
    geom[ "h" ] = rect.GetHeight();
    dlgMap[ "__geometry" ] = geom;

    std::function<void( wxWindow* )> saveFn =
            [&]( wxWindow* win )
            {
                if( PROPERTY_HOLDER* props = PROPERTY_HOLDER::SafeCast( win->GetClientData() ) )
                {
                    if( !props->GetPropertyOr( "persist", false ) )
                        return;
                }

                std::string key = generateKey( win );

                if( !key.empty() )
                {
                    if( m_unitBinders.contains( win ) && !m_unitBinders[ win ]->UnitsInvariant() )
                    {
                        dlgMap[ key ] = m_unitBinders[ win ]->GetValue();
                    }
                    else if( wxComboBox* combo = dynamic_cast<wxComboBox*>( win ) )
                    {
                        dlgMap[ key ] = combo->GetValue();
                    }
                    else if( wxOwnerDrawnComboBox* od_combo = dynamic_cast<wxOwnerDrawnComboBox*>( win ) )
                    {
                        dlgMap[ key ] = od_combo->GetSelection();
                    }
                    else if( wxTextEntry* textEntry = dynamic_cast<wxTextEntry*>( win ) )
                    {
                        dlgMap[ key ] = textEntry->GetValue();
                    }
                    else if( wxChoice* choice = dynamic_cast<wxChoice*>( win ) )
                    {
                        dlgMap[ key ] = choice->GetSelection();
                    }
                    else if( wxCheckBox* check = dynamic_cast<wxCheckBox*>( win ) )
                    {
                        dlgMap[ key ] = check->GetValue();
                    }
                    else if( wxSpinCtrl* spin = dynamic_cast<wxSpinCtrl*>( win ) )
                    {
                        dlgMap[ key ] = spin->GetValue();
                    }
                    else if( wxRadioButton* radio = dynamic_cast<wxRadioButton*>( win ) )
                    {
                        dlgMap[ key ] = radio->GetValue();
                    }
                    else if( wxRadioBox* radioBox = dynamic_cast<wxRadioBox*>( win ) )
                    {
                        dlgMap[ key ] = radioBox->GetSelection();
                    }
                    else if( wxSplitterWindow* splitter = dynamic_cast<wxSplitterWindow*>( win ) )
                    {
                        dlgMap[ key ] = splitter->GetSashPosition();
                    }
                    else if( wxScrolledWindow* scrolled = dynamic_cast<wxScrolledWindow*>( win ) )
                    {
                        dlgMap[ key ] = scrolled->GetScrollPos( wxVERTICAL );
                    }
                    else if( wxNotebook* notebook = dynamic_cast<wxNotebook*>( win ) )
                    {
                        int index = notebook->GetSelection();

                        if( index >= 0 && index < (int) notebook->GetPageCount() )
                            dlgMap[ key ] = notebook->GetPageText( notebook->GetSelection() );
                    }
                    else if( WX_GRID* grid = dynamic_cast<WX_GRID*>( win ) )
                    {
                        dlgMap[ key ] = grid->GetShownColumnsAsString();
                    }
                }

                for( wxWindow* child : win->GetChildren() )
                    saveFn( child );
            };

    if( PROPERTY_HOLDER* props = PROPERTY_HOLDER::SafeCast( GetClientData() ) )
    {
        if( !props->GetPropertyOr( "persist", false ) )
            return;
    }

    for( wxWindow* child : GetChildren() )
        saveFn( child );
}


void DIALOG_SHIM::LoadControlState()
{
    COMMON_SETTINGS* settings = Pgm().GetCommonSettings();

    if( !settings )
        return;

    std::string dialogKey = m_hash_key.empty() ? GetTitle().ToStdString() : m_hash_key;
    auto        dlgIt = settings->m_dialogControlValues.find( dialogKey );

    if( dlgIt == settings->m_dialogControlValues.end() )
        return;

    const std::map<std::string, nlohmann::json>& dlgMap = dlgIt->second;

    std::function<void( wxWindow* )> loadFn =
            [&]( wxWindow* win )
            {
                if( PROPERTY_HOLDER* props = PROPERTY_HOLDER::SafeCast( win->GetClientData() ) )
                {
                    if( !props->GetPropertyOr( "persist", false ) )
                        return;
                }

                std::string key = generateKey( win );

                if( !key.empty() )
                {
                    auto it = dlgMap.find( key );

                    if( it != dlgMap.end() )
                    {
                        const nlohmann::json& j = it->second;

                        if( m_unitBinders.contains( win ) && !m_unitBinders[ win ]->UnitsInvariant() )
                        {
                            if( j.is_number_integer() )
                                m_unitBinders[ win ]->ChangeValue( j.get<int>() );
                        }
                        else if( wxComboBox* combo = dynamic_cast<wxComboBox*>( win ) )
                        {
                            if( j.is_string() )
                                combo->SetValue( wxString::FromUTF8( j.get<std::string>().c_str() ) );
                        }
                        else if( wxOwnerDrawnComboBox* od_combo = dynamic_cast<wxOwnerDrawnComboBox*>( win ) )
                        {
                            if( j.is_number_integer() )
                            {
                                int index = j.get<int>();

                                if( index >= 0 && index < (int) od_combo->GetCount() )
                                    od_combo->SetSelection( index );
                            }
                        }
                        else if( wxTextEntry* textEntry = dynamic_cast<wxTextEntry*>( win ) )
                        {
                            if( j.is_string() )
                                textEntry->ChangeValue( wxString::FromUTF8( j.get<std::string>().c_str() ) );
                        }
                        else if( wxChoice* choice = dynamic_cast<wxChoice*>( win ) )
                        {
                            if( j.is_number_integer() )
                            {
                                int index = j.get<int>();

                                if( index >= 0 && index < (int) choice->GetCount() )
                                    choice->SetSelection( index );
                            }
                        }
                        else if( wxCheckBox* check = dynamic_cast<wxCheckBox*>( win ) )
                        {
                            if( j.is_boolean() )
                                check->SetValue( j.get<bool>() );
                        }
                        else if( wxSpinCtrl* spin = dynamic_cast<wxSpinCtrl*>( win ) )
                        {
                            if( j.is_number_integer() )
                                spin->SetValue( j.get<int>() );
                        }
                        else if( wxRadioButton* radio = dynamic_cast<wxRadioButton*>( win ) )
                        {
                            if( j.is_boolean() )
                            {
                                // Only set active radio buttons.  Let wxWidgets handle unsetting the inactive
                                // ones.  This prevents all from being unset, which trips up wxWidgets in some
                                // cases.
                                if( j.get<bool>() )
                                    radio->SetValue( true );
                            }
                        }
                        else if( wxRadioBox* radioBox = dynamic_cast<wxRadioBox*>( win ) )
                        {
                            if( j.is_number_integer() )
                            {
                                int index = j.get<int>();

                                if( index >= 0 && index < (int) radioBox->GetCount() )
                                    radioBox->SetSelection( index );
                            }
                        }
                        else if( wxSplitterWindow* splitter = dynamic_cast<wxSplitterWindow*>( win ) )
                        {
                            if( j.is_number_integer() )
                                splitter->SetSashPosition( j.get<int>() );
                        }
                        else if( wxScrolledWindow* scrolled = dynamic_cast<wxScrolledWindow*>( win ) )
                        {
                            if( j.is_number_integer() )
                                scrolled->SetScrollPos( wxVERTICAL, j.get<int>() );
                        }
                        else if( wxNotebook* notebook = dynamic_cast<wxNotebook*>( win ) )
                        {
                            if( j.is_string() )
                            {
                                wxString pageTitle = wxString::FromUTF8( j.get<std::string>().c_str() );

                                for( int page = 0; page < (int) notebook->GetPageCount(); ++page )
                                {
                                    if( notebook->GetPageText( page ) == pageTitle )
                                        notebook->SetSelection( page );
                                }
                            }
                        }
                        else if( WX_GRID* grid = dynamic_cast<WX_GRID*>( win ) )
                        {
                            if( j.is_string() )
                                grid->ShowHideColumns( wxString::FromUTF8( j.get<std::string>().c_str() ) );
                        }
                    }
                }

                for( wxWindow* child : win->GetChildren() )
                    loadFn( child );
            };

    if( PROPERTY_HOLDER* props = PROPERTY_HOLDER::SafeCast( GetClientData() ) )
    {
        if( !props->GetPropertyOr( "persist", false ) )
            return;
    }

    for( wxWindow* child : GetChildren() )
        loadFn( child );
}


void DIALOG_SHIM::OptOut( wxWindow* aWindow )
{
    PROPERTY_HOLDER* props = new PROPERTY_HOLDER();
    props->SetProperty( "persist", false );
    aWindow->SetClientData( props );
}


void DIALOG_SHIM::RegisterUnitBinder( UNIT_BINDER* aUnitBinder, wxWindow* aWindow )
{
    m_unitBinders[ aWindow ] = aUnitBinder;
}


// Recursive descent doing a SelectAll() in wxTextCtrls.
// MacOS User Interface Guidelines state that when tabbing to a text control all its
// text should be selected.  Since wxWidgets fails to implement this, we do it here.
void DIALOG_SHIM::SelectAllInTextCtrls( wxWindowList& children )
{
    for( wxWindow* child : children )
    {
        if( wxTextCtrl* textCtrl = dynamic_cast<wxTextCtrl*>( child ) )
        {
            m_beforeEditValues[ textCtrl ] = textCtrl->GetValue();
            textCtrl->Connect( wxEVT_SET_FOCUS, wxFocusEventHandler( DIALOG_SHIM::onChildSetFocus ),
                               nullptr, this );

            // We don't currently run this on GTK because some window managers don't hide the
            // selection in non-active controls, and other window managers do the selection
            // automatically anyway.
#if defined( __WXMAC__ ) || defined( __WXMSW__ )
            if( !textCtrl->GetStringSelection().IsEmpty() )
            {
                // Respect an existing selection
            }
            else if( textCtrl->IsEditable() )
            {
                textCtrl->SelectAll();
            }
#else
            ignore_unused( textCtrl );
#endif
        }
        else if( wxStyledTextCtrl* scintilla = dynamic_cast<wxStyledTextCtrl*>( child ) )
        {
            m_beforeEditValues[ scintilla ] = scintilla->GetText();
            scintilla->Connect( wxEVT_SET_FOCUS,
                                wxFocusEventHandler( DIALOG_SHIM::onChildSetFocus ),
                                nullptr, this );

            if( !scintilla->GetSelectedText().IsEmpty() )
            {
                // Respect an existing selection
            }
            else if( scintilla->GetMarginWidth( 0 ) > 0 )
            {
                // Don't select-all in Custom Rules, etc.
            }
            else if( scintilla->IsEditable() )
            {
                scintilla->SelectAll();
            }
        }
#ifdef __WXMAC__
        // Temp hack for square (looking) buttons on OSX.  Will likely be made redundant
        // by the image store....
        else if( dynamic_cast<wxBitmapButton*>( child ) != nullptr )
        {
            wxSize minSize( 29, 27 );
            wxRect rect = child->GetRect();

            child->ConvertDialogToPixels( minSize );

            rect.Inflate( std::max( 0, minSize.x - rect.GetWidth() ),
                          std::max( 0, minSize.y - rect.GetHeight() ) );

            child->SetMinSize( rect.GetSize() );
            child->SetSize( rect );
        }
#endif
        else
        {
            SelectAllInTextCtrls( child->GetChildren() );
        }
    }
}


void DIALOG_SHIM::registerUndoRedoHandlers( wxWindowList& children )
{
    for( wxWindow* child : children )
    {
        if( wxTextCtrl* textCtrl = dynamic_cast<wxTextCtrl*>( child ) )
        {
            textCtrl->Bind( wxEVT_TEXT, &DIALOG_SHIM::onCommandEvent, this );
            m_currentValues[ textCtrl ] = textCtrl->GetValue();
        }
        else if( wxStyledTextCtrl* scintilla = dynamic_cast<wxStyledTextCtrl*>( child ) )
        {
            scintilla->Bind( wxEVT_STC_CHANGE, &DIALOG_SHIM::onStyledTextChanged, this );
            m_currentValues[ scintilla ] = scintilla->GetText();
        }
        else if( wxComboBox* combo = dynamic_cast<wxComboBox*>( child ) )
        {
            combo->Bind( wxEVT_TEXT, &DIALOG_SHIM::onCommandEvent, this );
            combo->Bind( wxEVT_COMBOBOX, &DIALOG_SHIM::onCommandEvent, this );
            m_currentValues[ combo ] = combo->GetValue();
        }
        else if( wxChoice* choice = dynamic_cast<wxChoice*>( child ) )
        {
            choice->Bind( wxEVT_CHOICE, &DIALOG_SHIM::onCommandEvent, this );
            m_currentValues[ choice ] = static_cast<long>( choice->GetSelection() );
        }
        else if( wxCheckBox* check = dynamic_cast<wxCheckBox*>( child ) )
        {
            check->Bind( wxEVT_CHECKBOX, &DIALOG_SHIM::onCommandEvent, this );
            m_currentValues[ check ] = check->GetValue();
        }
        else if( wxSpinCtrl* spin = dynamic_cast<wxSpinCtrl*>( child ) )
        {
            spin->Bind( wxEVT_SPINCTRL, &DIALOG_SHIM::onSpinEvent, this );
            spin->Bind( wxEVT_TEXT, &DIALOG_SHIM::onCommandEvent, this );
            m_currentValues[ spin ] = static_cast<long>( spin->GetValue() );
        }
        else if( wxSpinCtrlDouble* spinD = dynamic_cast<wxSpinCtrlDouble*>( child ) )
        {
            spinD->Bind( wxEVT_SPINCTRLDOUBLE, &DIALOG_SHIM::onSpinDoubleEvent, this );
            spinD->Bind( wxEVT_TEXT, &DIALOG_SHIM::onCommandEvent, this );
            m_currentValues[ spinD ] = spinD->GetValue();
        }
        else if( wxRadioButton* radio = dynamic_cast<wxRadioButton*>( child ) )
        {
            radio->Bind( wxEVT_RADIOBUTTON, &DIALOG_SHIM::onCommandEvent, this );
            m_currentValues[ radio ] = radio->GetValue();
        }
        else if( wxRadioBox* radioBox = dynamic_cast<wxRadioBox*>( child ) )
        {
            radioBox->Bind( wxEVT_RADIOBOX, &DIALOG_SHIM::onCommandEvent, this );
            m_currentValues[ radioBox ] = static_cast<long>( radioBox->GetSelection() );
        }
        else if( wxGrid* grid = dynamic_cast<wxGrid*>( child ) )
        {
            grid->Bind( wxEVT_GRID_CELL_CHANGED, &DIALOG_SHIM::onGridCellChanged, this );
            m_currentValues[ grid ] = getControlValue( grid );
        }
        else if( wxPropertyGrid* propGrid = dynamic_cast<wxPropertyGrid*>( child ) )
        {
            propGrid->Bind( wxEVT_PG_CHANGED, &DIALOG_SHIM::onPropertyGridChanged, this );
            m_currentValues[ propGrid ] = getControlValue( propGrid );
        }
        else if( wxCheckListBox* checkList = dynamic_cast<wxCheckListBox*>( child ) )
        {
            checkList->Bind( wxEVT_CHECKLISTBOX, &DIALOG_SHIM::onCommandEvent, this );
            m_currentValues[ checkList ] = getControlValue( checkList );
        }
        else if( wxDataViewListCtrl* dataList = dynamic_cast<wxDataViewListCtrl*>( child ) )
        {
            dataList->Bind( wxEVT_DATAVIEW_ITEM_VALUE_CHANGED, &DIALOG_SHIM::onDataViewListChanged, this );
            m_currentValues[ dataList ] = getControlValue( dataList );
        }
        else
        {
            registerUndoRedoHandlers( child->GetChildren() );
        }
    }
}


void DIALOG_SHIM::recordControlChange( wxWindow* aCtrl )
{
    wxVariant before = m_currentValues[ aCtrl ];
    wxVariant after = getControlValue( aCtrl );

    if( before != after )
    {
        m_undoStack.push_back( { aCtrl, before, after } );
        m_redoStack.clear();
        m_currentValues[ aCtrl ] = after;
    }
}


void DIALOG_SHIM::onCommandEvent( wxCommandEvent& aEvent )
{
    if( !m_handlingUndoRedo )
        recordControlChange( static_cast<wxWindow*>( aEvent.GetEventObject() ) );

    aEvent.Skip();
}


void DIALOG_SHIM::onSpinEvent( wxSpinEvent& aEvent )
{
    if( !m_handlingUndoRedo )
        recordControlChange( static_cast<wxWindow*>( aEvent.GetEventObject() ) );

    aEvent.Skip();
}


void DIALOG_SHIM::onSpinDoubleEvent( wxSpinDoubleEvent& aEvent )
{
    if( !m_handlingUndoRedo )
        recordControlChange( static_cast<wxWindow*>( aEvent.GetEventObject() ) );

    aEvent.Skip();
}


void DIALOG_SHIM::onStyledTextChanged( wxStyledTextEvent& aEvent )
{
    if( !m_handlingUndoRedo )
        recordControlChange( static_cast<wxWindow*>( aEvent.GetEventObject() ) );

    aEvent.Skip();
}


void DIALOG_SHIM::onGridCellChanged( wxGridEvent& aEvent )
{
    if( !m_handlingUndoRedo )
        recordControlChange( static_cast<wxWindow*>( aEvent.GetEventObject() ) );

    aEvent.Skip();
}

void DIALOG_SHIM::onPropertyGridChanged( wxPropertyGridEvent& aEvent )
{
    if( !m_handlingUndoRedo )
        recordControlChange( static_cast<wxWindow*>( aEvent.GetEventObject() ) );

    aEvent.Skip();
}

void DIALOG_SHIM::onDataViewListChanged( wxDataViewEvent& aEvent )
{
    if( !m_handlingUndoRedo )
        recordControlChange( static_cast<wxWindow*>( aEvent.GetEventObject() ) );

    aEvent.Skip();
}

wxVariant DIALOG_SHIM::getControlValue( wxWindow* aCtrl )
{
    if( wxTextCtrl* textCtrl = dynamic_cast<wxTextCtrl*>( aCtrl ) )
        return wxVariant( textCtrl->GetValue() );
    else if( wxStyledTextCtrl* scintilla = dynamic_cast<wxStyledTextCtrl*>( aCtrl ) )
        return wxVariant( scintilla->GetText() );
    else if( wxComboBox* combo = dynamic_cast<wxComboBox*>( aCtrl ) )
        return wxVariant( combo->GetValue() );
    else if( wxChoice* choice = dynamic_cast<wxChoice*>( aCtrl ) )
        return wxVariant( (long) choice->GetSelection() );
    else if( wxCheckBox* check = dynamic_cast<wxCheckBox*>( aCtrl ) )
        return wxVariant( check->GetValue() );
    else if( wxSpinCtrl* spin = dynamic_cast<wxSpinCtrl*>( aCtrl ) )
        return wxVariant( (long) spin->GetValue() );
    else if( wxSpinCtrlDouble* spinD = dynamic_cast<wxSpinCtrlDouble*>( aCtrl ) )
        return wxVariant( spinD->GetValue() );
    else if( wxRadioButton* radio = dynamic_cast<wxRadioButton*>( aCtrl ) )
        return wxVariant( radio->GetValue() );
    else if( wxRadioBox* radioBox = dynamic_cast<wxRadioBox*>( aCtrl ) )
        return wxVariant( (long) radioBox->GetSelection() );
    else if( wxGrid* grid = dynamic_cast<wxGrid*>( aCtrl ) )
    {
        nlohmann::json j = nlohmann::json::array();
        int rows = grid->GetNumberRows();
        int cols = grid->GetNumberCols();

        for( int r = 0; r < rows; ++r )
        {
            nlohmann::json row = nlohmann::json::array();

            for( int c = 0; c < cols; ++c )
                row.push_back( std::string( grid->GetCellValue( r, c ).ToUTF8() ) );

            j.push_back( row );
        }

        return wxVariant( wxString( j.dump() ) );
    }
    else if( wxPropertyGrid* propGrid = dynamic_cast<wxPropertyGrid*>( aCtrl ) )
    {
        nlohmann::json j;

        for( wxPropertyGridIterator it = propGrid->GetIterator(); !it.AtEnd(); ++it )
        {
            wxPGProperty* prop = *it;
            j[ prop->GetName().ToStdString() ] = prop->GetValueAsString().ToStdString();
        }

        return wxVariant( wxString( j.dump() ) );
    }
    else if( wxCheckListBox* checkList = dynamic_cast<wxCheckListBox*>( aCtrl ) )
    {
        nlohmann::json j = nlohmann::json::array();
        unsigned int count = checkList->GetCount();

        for( unsigned int i = 0; i < count; ++i )
        {
            if( checkList->IsChecked( i ) )
                j.push_back( i );
        }

        return wxVariant( wxString( j.dump() ) );
    }
    else if( wxDataViewListCtrl* dataList = dynamic_cast<wxDataViewListCtrl*>( aCtrl ) )
    {
        nlohmann::json j = nlohmann::json::array();
        unsigned int rows = dataList->GetItemCount();
        unsigned int cols = dataList->GetColumnCount();

        for( unsigned int r = 0; r < rows; ++r )
        {
            nlohmann::json row = nlohmann::json::array();

            for( unsigned int c = 0; c < cols; ++c )
            {
                wxVariant val;
                dataList->GetValue( val, r, c );
                row.push_back( std::string( val.GetString().ToUTF8() ) );
            }

            j.push_back( row );
        }

        return wxVariant( wxString( j.dump() ) );
    }
    else
        return wxVariant();
}


void DIALOG_SHIM::setControlValue( wxWindow* aCtrl, const wxVariant& aValue )
{
    if( wxTextCtrl* textCtrl = dynamic_cast<wxTextCtrl*>( aCtrl ) )
        textCtrl->SetValue( aValue.GetString() );
    else if( wxStyledTextCtrl* scintilla = dynamic_cast<wxStyledTextCtrl*>( aCtrl ) )
        scintilla->SetText( aValue.GetString() );
    else if( wxComboBox* combo = dynamic_cast<wxComboBox*>( aCtrl ) )
        combo->SetValue( aValue.GetString() );
    else if( wxChoice* choice = dynamic_cast<wxChoice*>( aCtrl ) )
        choice->SetSelection( (int) aValue.GetLong() );
    else if( wxCheckBox* check = dynamic_cast<wxCheckBox*>( aCtrl ) )
        check->SetValue( aValue.GetBool() );
    else if( wxSpinCtrl* spin = dynamic_cast<wxSpinCtrl*>( aCtrl ) )
        spin->SetValue( (int) aValue.GetLong() );
    else if( wxSpinCtrlDouble* spinD = dynamic_cast<wxSpinCtrlDouble*>( aCtrl ) )
        spinD->SetValue( aValue.GetDouble() );
    else if( wxRadioButton* radio = dynamic_cast<wxRadioButton*>( aCtrl ) )
        radio->SetValue( aValue.GetBool() );
    else if( wxRadioBox* radioBox = dynamic_cast<wxRadioBox*>( aCtrl ) )
        radioBox->SetSelection( (int) aValue.GetLong() );
    else if( wxGrid* grid = dynamic_cast<wxGrid*>( aCtrl ) )
    {
        nlohmann::json j = nlohmann::json::parse( aValue.GetString().ToStdString(), nullptr, false );

        if( j.is_array() )
        {
            int rows = std::min( (int) j.size(), grid->GetNumberRows() );

            for( int r = 0; r < rows; ++r )
            {
                nlohmann::json row = j[r];
                int cols = std::min( (int) row.size(), grid->GetNumberCols() );

                for( int c = 0; c < cols; ++c )
                    grid->SetCellValue( r, c, wxString( row[c].get<std::string>() ) );
            }
        }
    }
    else if( wxPropertyGrid* propGrid = dynamic_cast<wxPropertyGrid*>( aCtrl ) )
    {
        nlohmann::json j = nlohmann::json::parse( aValue.GetString().ToStdString(), nullptr, false );

        if( j.is_object() )
        {
            for( auto it = j.begin(); it != j.end(); ++it )
                propGrid->SetPropertyValue( wxString( it.key() ), wxString( it.value().get<std::string>() ) );
        }
    }
    else if( wxCheckListBox* checkList = dynamic_cast<wxCheckListBox*>( aCtrl ) )
    {
        nlohmann::json j = nlohmann::json::parse( aValue.GetString().ToStdString(), nullptr, false );

        if( j.is_array() )
        {
            unsigned int count = checkList->GetCount();

            for( unsigned int i = 0; i < count; ++i )
                checkList->Check( i, false );

            for( auto& idx : j )
            {
                unsigned int i = idx.get<unsigned int>();

                if( i < count )
                    checkList->Check( i, true );
            }
        }
    }
    else if( wxDataViewListCtrl* dataList = dynamic_cast<wxDataViewListCtrl*>( aCtrl ) )
    {
        nlohmann::json j = nlohmann::json::parse( aValue.GetString().ToStdString(), nullptr, false );

        if( j.is_array() )
        {
            unsigned int rows = std::min( static_cast<unsigned int>( j.size() ),
                                          static_cast<unsigned int>( dataList->GetItemCount() ) );

            for( unsigned int r = 0; r < rows; ++r )
            {
                nlohmann::json row = j[r];
                unsigned int cols = std::min( (unsigned int) row.size(), dataList->GetColumnCount() );

                for( unsigned int c = 0; c < cols; ++c )
                {
                    wxVariant val( wxString( row[c].get<std::string>() ) );
                    dataList->SetValue( val, r, c );
                }
            }
        }
    }
}


void DIALOG_SHIM::doUndo()
{
    if( m_undoStack.empty() )
        return;

    m_handlingUndoRedo = true;
    UNDO_STEP step = m_undoStack.back();
    m_undoStack.pop_back();
    setControlValue( step.ctrl, step.before );
    m_currentValues[ step.ctrl ] = step.before;
    m_redoStack.push_back( step );
    m_handlingUndoRedo = false;
}


void DIALOG_SHIM::doRedo()
{
    if( m_redoStack.empty() )
        return;

    m_handlingUndoRedo = true;
    UNDO_STEP step = m_redoStack.back();
    m_redoStack.pop_back();
    setControlValue( step.ctrl, step.after );
    m_currentValues[ step.ctrl ] = step.after;
    m_undoStack.push_back( step );
    m_handlingUndoRedo = false;
}


void DIALOG_SHIM::OnPaint( wxPaintEvent &event )
{
    if( m_firstPaintEvent )
    {
        KIPLATFORM::UI::FixupCancelButtonCmdKeyCollision( this );

        SelectAllInTextCtrls( GetChildren() );
        registerUndoRedoHandlers( GetChildren() );

        if( m_initialFocusTarget )
            KIPLATFORM::UI::ForceFocus( m_initialFocusTarget );
        else
            KIPLATFORM::UI::ForceFocus( this );     // Focus the dialog itself

        m_firstPaintEvent = false;
    }

    event.Skip();
}


void DIALOG_SHIM::OnModify()
{
    if( !GetTitle().StartsWith( wxS( "*" ) ) )
        SetTitle( wxS( "*" ) + GetTitle() );
}


void DIALOG_SHIM::ClearModify()
{
    if( GetTitle().StartsWith( wxS( "*" ) ) )
        SetTitle( GetTitle().AfterFirst( '*' ) );
}

int DIALOG_SHIM::ShowModal()
{
    // Apple in its infinite wisdom will raise a disabled window before even passing
    // us the event, so we have no way to stop it.  Instead, we must set an order on
    // the windows so that the modal will be pushed in front of the disabled
    // window when it is raised.
    KIPLATFORM::UI::ReparentModal( this );

    // Call the base class ShowModal() method
    return wxDialog::ShowModal();
}

/*
    QuasiModal Mode Explained:

    The gtk calls in wxDialog::ShowModal() cause event routing problems if that
    modal dialog then tries to use KIWAY_PLAYER::ShowModal().  The latter shows up
    and mostly works but does not respond to the window decoration close button.
    There is no way to get around this without reversing the gtk calls temporarily.

    There are also issues with the Scintilla text editor putting up autocomplete
    popups, which appear behind the dialog window if QuasiModal is not used.

    QuasiModal mode is our own almost modal mode which disables only the parent
    of the DIALOG_SHIM, leaving other frames operable and while staying captured in the
    nested event loop.  This avoids the gtk calls and leaves event routing pure
    and sufficient to operate the KIWAY_PLAYER::ShowModal() properly.  When using
    ShowQuasiModal() you have to use EndQuasiModal() in your dialogs and not
    EndModal().  There is also IsQuasiModal() but its value can only be true
    when the nested event loop is active.  Do not mix the modal and quasi-modal
    functions.  Use one set or the other.

    You might find this behavior preferable over a pure modal mode, and it was said
    that only the Mac has this natively, but now other platforms have something
    similar.  You CAN use it anywhere for any dialog.  But you MUST use it when
    you want to use KIWAY_PLAYER::ShowModal() from a dialog event.
*/

int DIALOG_SHIM::ShowQuasiModal()
{
    NULLER raii_nuller( (void*&) m_qmodal_loop );

    // release the mouse if it's currently captured as the window having it
    // will be disabled when this dialog is shown -- but will still keep the
    // capture making it impossible to do anything in the modal dialog itself
    if( wxWindow* win = wxWindow::GetCapture() )
        win->ReleaseMouse();

    // Get the optimal parent
    wxWindow* parent = GetParentForModalDialog( GetParent(), GetWindowStyle() );

    wxASSERT_MSG( !m_qmodal_parent_disabler, wxT( "Caller using ShowQuasiModal() twice on same window?" ) );

    // quasi-modal: disable only my "optimal" parent
    m_qmodal_parent_disabler = new WINDOW_DISABLER( parent );

    // Apple in its infinite wisdom will raise a disabled window before even passing
    // us the event, so we have no way to stop it.  Instead, we must set an order on
    // the windows so that the quasi-modal will be pushed in front of the disabled
    // window when it is raised.
    KIPLATFORM::UI::ReparentModal( this );

    Show( true );

    m_qmodal_showing = true;

    wxGUIEventLoop event_loop;

    m_qmodal_loop = &event_loop;

    event_loop.Run();

    m_qmodal_showing = false;

    if( parent )
        parent->SetFocus();

    return GetReturnCode();
}


void DIALOG_SHIM::PrepareForModalSubDialog()
{
    if( m_qmodal_parent_disabler )
        m_qmodal_parent_disabler->SuspendForTrueModal();
}


void DIALOG_SHIM::CleanupAfterModalSubDialog()
{
    if( m_qmodal_parent_disabler )
        m_qmodal_parent_disabler->ResumeAfterTrueModal();
}


void DIALOG_SHIM::EndQuasiModal( int retCode )
{
    // Hook up validator and transfer data from controls handling so quasi-modal dialogs
    // handle validation in the same way as other dialogs.
    if( ( retCode == wxID_OK ) && ( !Validate() || !TransferDataFromWindow() ) )
        return;

    SetReturnCode( retCode );

    if( !IsQuasiModal() )
    {
        wxFAIL_MSG( wxT( "Either DIALOG_SHIM::EndQuasiModal was called twice, or ShowQuasiModal wasn't called" ) );
        return;
    }

    TearDownQuasiModal();

    if( m_qmodal_loop )
    {
        if( m_qmodal_loop->IsRunning() )
            m_qmodal_loop->Exit( 0 );
        else
            m_qmodal_loop->ScheduleExit( 0 );
    }

    delete m_qmodal_parent_disabler;
    m_qmodal_parent_disabler = nullptr;

    Show( false );
}


void DIALOG_SHIM::OnCloseWindow( wxCloseEvent& aEvent )
{
    wxString msg = wxString::Format( "Closing dialog %s", GetTitle() );
    APP_MONITOR::AddNavigationBreadcrumb( msg, "dialog.close" );

    SaveControlState();

    if( IsQuasiModal() )
    {
        EndQuasiModal( wxID_CANCEL );
        return;
    }

    // This is mandatory to allow wxDialogBase::OnCloseWindow() to be called.
    aEvent.Skip();
}


void DIALOG_SHIM::OnButton( wxCommandEvent& aEvent )
{
    const int id = aEvent.GetId();

    if( IsQuasiModal() )
    {
        if( id == GetAffirmativeId() )
        {
            EndQuasiModal( id );
        }
        else if( id == wxID_APPLY )
        {
            // Dialogs that provide Apply buttons should make sure data is valid before
            // allowing a transfer, as there is no other way to indicate failure
            // (i.e. the dialog can't refuse to close as it might with OK, because it
            // isn't closing anyway)
            if( Validate() )
                ignore_unused( TransferDataFromWindow() );
        }
        else if( id == wxID_CANCEL )
        {
            EndQuasiModal( wxID_CANCEL );
        }
        else // not a standard button
        {
            aEvent.Skip();
        }

        return;
    }

    // This is mandatory to allow wxDialogBase::OnButton() to be called.
    aEvent.Skip();
}


void DIALOG_SHIM::onChildSetFocus( wxFocusEvent& aEvent )
{
    // When setting focus to a text control reset the before-edit value.

    if( !m_isClosing )
    {
        if( wxTextCtrl* textCtrl = dynamic_cast<wxTextCtrl*>( aEvent.GetEventObject() ) )
            m_beforeEditValues[ textCtrl ] = textCtrl->GetValue();
        else if( wxStyledTextCtrl* scintilla = dynamic_cast<wxStyledTextCtrl*>( aEvent.GetEventObject() ) )
            m_beforeEditValues[ scintilla ] = scintilla->GetText();
    }

    aEvent.Skip();
}


void DIALOG_SHIM::OnCharHook( wxKeyEvent& aEvt )
{
    int key = aEvt.GetKeyCode();
    int mods = 0;

    if( aEvt.ControlDown() )
        mods |= MD_CTRL;
    if( aEvt.ShiftDown() )
        mods |= MD_SHIFT;
    if( aEvt.AltDown() )
        mods |= MD_ALT;

    int hotkey = key | mods;

    // Check for standard undo/redo hotkeys
    if( hotkey == (MD_CTRL + 'Z') )
    {
        doUndo();
        return;
    }
    else if( hotkey == (MD_CTRL + MD_SHIFT + 'Z') || hotkey == (MD_CTRL + 'Y') )
    {
        doRedo();
        return;
    }

    if( aEvt.GetKeyCode() == 'U' && aEvt.GetModifiers() == wxMOD_CONTROL )
    {
        if( m_parentFrame )
        {
            m_parentFrame->ToggleUserUnits();
            return;
        }
    }
    // shift-return (Mac default) or Ctrl-Return (GTK) for new line input
    else if( ( aEvt.GetKeyCode() == WXK_RETURN || aEvt.GetKeyCode() == WXK_NUMPAD_ENTER ) && aEvt.ShiftDown() )
    {
        wxObject* eventSource = aEvt.GetEventObject();

        if( wxTextCtrl* textCtrl = dynamic_cast<wxTextCtrl*>( eventSource ) )
        {
            // If the text control is not multi-line, we want to close the dialog
            if( !textCtrl->IsMultiLine() )
            {
                wxPostEvent( this, wxCommandEvent( wxEVT_COMMAND_BUTTON_CLICKED, wxID_OK ) );
                return;
            }

#if defined( __WXMAC__ ) || defined( __WXMSW__ )
            wxString eol = "\r\n";
#else
            wxString eol = "\n";
#endif

            long pos = textCtrl->GetInsertionPoint();
            textCtrl->WriteText( eol );
            textCtrl->SetInsertionPoint( pos + eol.length() );
            return;
        }
        else if( wxStyledTextCtrl* scintilla = dynamic_cast<wxStyledTextCtrl*>( eventSource ) )
        {
            wxString eol = "\n";

            switch( scintilla->GetEOLMode() )
            {
            case wxSTC_EOL_CRLF: eol = "\r\n"; break;
            case wxSTC_EOL_CR:   eol = "\r";   break;
            case wxSTC_EOL_LF:   eol = "\n";   break;
            }

            long pos = scintilla->GetCurrentPos();
            scintilla->InsertText( pos, eol );
            scintilla->GotoPos( pos + eol.length() );
            return;
        }
        return;
    }
    // command-return (Mac default) or Ctrl-Return (GTK) for OK
    else if( ( aEvt.GetKeyCode() == WXK_RETURN || aEvt.GetKeyCode() == WXK_NUMPAD_ENTER ) && aEvt.ControlDown() )
    {
        wxPostEvent( this, wxCommandEvent( wxEVT_COMMAND_BUTTON_CLICKED, wxID_OK ) );
        return;
    }
    else if( aEvt.GetKeyCode() == WXK_TAB && !aEvt.ControlDown() )
    {
        wxWindow* currentWindow = wxWindow::FindFocus();
        int       currentIdx = -1;
        int       delta = aEvt.ShiftDown() ? -1 : 1;

        auto advance =
                [&]( int& idx )
                {
                    // Wrap-around modulus
                    int size = (int) m_tabOrder.size();
                    idx = ( ( idx + delta ) % size + size ) % size;
                };

        for( size_t i = 0; i < m_tabOrder.size(); ++i )
        {
            // Check for exact match or if currentWindow is a child of the control
            // (e.g., the text entry inside a wxComboBox)
            if( m_tabOrder[i] == currentWindow
                || ( currentWindow && m_tabOrder[i]->IsDescendant( currentWindow ) ) )
            {
                currentIdx = (int) i;
                break;
            }
        }

        if( currentIdx >= 0 )
        {
            advance( currentIdx );

            // Skip hidden or disabled controls
            int startIdx = currentIdx;

            while( !m_tabOrder[currentIdx]->IsShown() || !m_tabOrder[currentIdx]->IsEnabled() )
            {
                advance( currentIdx );

                if( currentIdx == startIdx )
                    break;  // Avoid infinite loop if all controls are hidden
            }

            //todo: We don't currently have non-textentry dialog boxes but this will break if
            // we add them.
#ifdef __APPLE__
            while( dynamic_cast<wxTextEntry*>( m_tabOrder[ currentIdx ] ) == nullptr )
                advance( currentIdx );
#endif

            m_tabOrder[ currentIdx ]->SetFocus();
            return;
        }
    }
    else if( aEvt.GetKeyCode() == WXK_ESCAPE )
    {
        wxObject* eventSource = aEvt.GetEventObject();

        if( wxTextCtrl* textCtrl = dynamic_cast<wxTextCtrl*>( eventSource ) )
        {
            // First escape after an edit cancels edit
            if( textCtrl->GetValue() != m_beforeEditValues[ textCtrl ] )
            {
                textCtrl->SetValue( m_beforeEditValues[ textCtrl ] );
                textCtrl->SelectAll();
                return;
            }
        }
        else if( wxStyledTextCtrl* scintilla = dynamic_cast<wxStyledTextCtrl*>( eventSource ) )
        {
            // First escape after an edit cancels edit
            if( scintilla->GetText() != m_beforeEditValues[ scintilla ] )
            {
                scintilla->SetText( m_beforeEditValues[ scintilla ] );
                scintilla->SelectAll();
                return;
            }
        }
    }

    aEvt.Skip();
}


static void recursiveDescent( wxSizer* aSizer, std::map<int, wxString>& aLabels )
{
    wxStdDialogButtonSizer* sdbSizer = dynamic_cast<wxStdDialogButtonSizer*>( aSizer );

    auto setupButton =
            [&]( wxButton* aButton )
            {
                if( aLabels.count( aButton->GetId() ) > 0 )
                {
                    aButton->SetLabel( aLabels[ aButton->GetId() ] );
                }
                else
                {
                    // wxWidgets has an uneven track record when the language is changed on
                    // the fly so we set them even when they don't appear in the label map
                    switch( aButton->GetId() )
                    {
                    case wxID_OK:           aButton->SetLabel( _( "&OK" ) );     break;
                    case wxID_CANCEL:       aButton->SetLabel( _( "&Cancel" ) ); break;
                    case wxID_YES:          aButton->SetLabel( _( "&Yes" ) );    break;
                    case wxID_NO:           aButton->SetLabel( _( "&No" ) );     break;
                    case wxID_APPLY:        aButton->SetLabel( _( "&Apply" ) );  break;
                    case wxID_SAVE:         aButton->SetLabel( _( "&Save" ) );   break;
                    case wxID_HELP:         aButton->SetLabel( _( "&Help" ) );   break;
                    case wxID_CONTEXT_HELP: aButton->SetLabel( _( "&Help" ) );   break;
                    }
                }
            };

    if( sdbSizer )
    {
        if( sdbSizer->GetAffirmativeButton() )
            setupButton( sdbSizer->GetAffirmativeButton() );

        if( sdbSizer->GetApplyButton() )
            setupButton( sdbSizer->GetApplyButton() );

        if( sdbSizer->GetNegativeButton() )
            setupButton( sdbSizer->GetNegativeButton() );

        if( sdbSizer->GetCancelButton() )
            setupButton( sdbSizer->GetCancelButton() );

        if( sdbSizer->GetHelpButton() )
            setupButton( sdbSizer->GetHelpButton() );

        sdbSizer->Layout();

        if( sdbSizer->GetAffirmativeButton() )
            sdbSizer->GetAffirmativeButton()->SetDefault();
    }

    for( wxSizerItem* item : aSizer->GetChildren() )
    {
        if( item->GetSizer() )
            recursiveDescent( item->GetSizer(), aLabels );
    }
}


void DIALOG_SHIM::SetupStandardButtons( std::map<int, wxString> aLabels )
{
    recursiveDescent( GetSizer(), aLabels );
}
