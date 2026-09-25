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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "dialog_footprint_users.h"

#include <algorithm>

#include <wx/clipbrd.h>
#include <wx/menu.h>
#include <wx/types.h>
#include <wx/wupdlock.h>

#include <kiway.h>
#include <kiway_mail.h>
#include <string_utils.h>


namespace
{
// A footprint is normally used by at most a few dozen symbols, which is all the list needs
// to show.
static constexpr int MAX_USER_RESULTS = 400;


/// The match categories of the m_cbShow* checkboxes, as the bits of a set.
enum FOOTPRINT_USERS_MATCH_CATEGORY
{
    MATCH_FILTERS = 1 << 0,
    MATCH_FOOTPRINT_FIELD = 1 << 1,
    MATCH_PIN_MAPS = 1 << 2
};


/// The columns of m_footprintUserList, in the order they are appended.
enum FOOTPRINT_USERS_COLUMN
{
    COL_FOOTPRINT_USERS_LIBRARY = 0,
    COL_FOOTPRINT_USERS_SYMBOL,
    COL_FOOTPRINT_USERS_PINS,
    COL_FOOTPRINT_USERS_FILTER,
    COL_FOOTPRINT_USERS_FOOTPRINT_FIELD_MATCH,
    COL_FOOTPRINT_USERS_PIN_MAP_MATCH
};


enum ROW_DATA_FLAGS
{
    ROW_DATA_FLAG_PIN_COUNT_MATCHES = 0x01,
    ROW_DATA_FLAG_DERIVED_SYMBOL = 0x02
};


/**
 * Store that marks up the rows the dialog highlights (e.g. bold pin count
 * for matching pin count and italics for derived symbols)
 */
class FOOTPRINT_USERS_LIST_STORE : public wxDataViewListStore
{
public:
    bool GetAttrByRow( unsigned int aRow, unsigned int aCol, wxDataViewItemAttr& aAttr ) const override
    {
        switch( aCol )
        {
        case COL_FOOTPRINT_USERS_SYMBOL:
            if( GetItemData( GetItem( aRow ) ) & ROW_DATA_FLAG_DERIVED_SYMBOL )
            {
                aAttr.SetItalic( true );
                return true;
            }
            break;
        case COL_FOOTPRINT_USERS_PINS:
            if( GetItemData( GetItem( aRow ) ) & ROW_DATA_FLAG_PIN_COUNT_MATCHES )
            {
                aAttr.SetBold( true );
                return true;
            }
            break;
        default:
            break;
        }

        return false;
    }
};

} // namespace


DIALOG_FOOTPRINT_USERS::DIALOG_FOOTPRINT_USERS( wxWindow* aParent, KIWAY& aKiway, const LIB_ID& aFootprint,
                                                int aFootprintPinCount ) :
        DIALOG_FOOTPRINT_USERS_BASE( aParent ),
        m_kiway( aKiway ),
        m_footprint( aFootprint ),
        m_footprintPinCount( aFootprintPinCount ),
        m_loadTimer( this ),
        m_librariesReady( false )
{
    // Replace the default store with the pin count bolding variant before adding columns
    FOOTPRINT_USERS_LIST_STORE* store = new FOOTPRINT_USERS_LIST_STORE();
    m_footprintUserList->AssociateModel( store );
    store->DecRef();

    // Each row is one matching symbol; the columns say where it comes from and what makes it
    // a match.
    m_footprintUserList->AppendTextColumn( _( "Library" ), wxDATAVIEW_CELL_INERT, 180 );
    m_footprintUserList->AppendTextColumn( _( "Symbol" ), wxDATAVIEW_CELL_INERT, 200 );
    m_footprintUserList->AppendTextColumn( _( "Pins" ), wxDATAVIEW_CELL_INERT, 60 );
    m_footprintUserList->AppendTextColumn( _( "Footprint filter" ), wxDATAVIEW_CELL_INERT, 160 );
    m_footprintUserList->AppendToggleColumn( _( "Footprint field match" ), wxDATAVIEW_CELL_INERT, 120 );
    m_footprintUserList->AppendTextColumn( _( "Pin map match" ), wxDATAVIEW_CELL_INERT, 160 );


    // Double-clicking a user opens that symbol in the symbol editor.
    m_footprintUserList->Bind( wxEVT_DATAVIEW_ITEM_ACTIVATED, &DIALOG_FOOTPRINT_USERS::onUserActivated, this );
    m_footprintUserList->Bind( wxEVT_DATAVIEW_ITEM_CONTEXT_MENU, &DIALOG_FOOTPRINT_USERS::onUserRightClick, this );
    m_footprintUserList->Bind( wxEVT_KEY_DOWN, &DIALOG_FOOTPRINT_USERS::onUserKeyDown, this );

    // Every way of using the footprint is shown by default, and switched independently.
    m_cbShowFilterMatches->SetValue( true );
    m_cbShowFootprintFieldMatches->SetValue( true );
    m_cbShowPinMapMatches->SetValue( true );

    m_cbShowFilterMatches->Bind( wxEVT_CHECKBOX, &DIALOG_FOOTPRINT_USERS::onMatchTypeChanged, this );
    m_cbShowFootprintFieldMatches->Bind( wxEVT_CHECKBOX, &DIALOG_FOOTPRINT_USERS::onMatchTypeChanged, this );
    m_cbShowPinMapMatches->Bind( wxEVT_CHECKBOX, &DIALOG_FOOTPRINT_USERS::onMatchTypeChanged, this );

    // The libraries the users come from are loaded in the background.  Rather than blocking
    // the dialog, the status text shows the load progress and the list is greyed out until
    // they arrive.
    Bind( wxEVT_TIMER, &DIALOG_FOOTPRINT_USERS::onLoadTimer, this, m_loadTimer.GetId() );

    m_footprintUserList->SetFocus();

    // We'll fill this status text as needed
    m_statusText->SetLabel( wxEmptyString );

    // Most likely the symbol libraries are already loaded/ing (eeschema preloads them when a
    // project is open) but this is basically free in that case.
    StartSymbolLibrariesLoad( m_kiway );

    // Wait until the dialog has been painted before showing the results or the loading
    // indication.
    CallAfter(
            [this]()
            {
                updateUsers();
            } );

    // Now all widgets have the size fixed, call FinishDialogSettings
    finishDialogSettings();
}


void DIALOG_FOOTPRINT_USERS::updateUsers()
{
    if( !symbolLibrariesReady() )
        return;

    const int categories = matchCategories();

    FOOTPRINT_USERS_QUERY query;
    query.m_Footprint = m_footprint;
    query.m_MatchFilters = ( categories & MATCH_FILTERS ) != 0;
    query.m_MatchFootprintField = ( categories & MATCH_FOOTPRINT_FIELD ) != 0;
    query.m_MatchPinMaps = ( categories & MATCH_PIN_MAPS ) != 0;
    query.m_MaxResults = MAX_USER_RESULTS;

    showUsers( QueryFootprintUsers( m_kiway, query ) );
}


int DIALOG_FOOTPRINT_USERS::matchCategories() const
{
    int categories = 0;

    if( m_cbShowFilterMatches->GetValue() )
        categories |= MATCH_FILTERS;

    if( m_cbShowFootprintFieldMatches->GetValue() )
        categories |= MATCH_FOOTPRINT_FIELD;

    if( m_cbShowPinMapMatches->GetValue() )
        categories |= MATCH_PIN_MAPS;

    return categories;
}


bool DIALOG_FOOTPRINT_USERS::symbolLibrariesReady()
{
    if( m_librariesReady )
        return true;

    float progress = GetSymbolLibrariesLoadProgress( m_kiway );

    if( progress >= 1.0f )
    {
        m_librariesReady = true;
        showLoadIndication( false );
        return true;
    }

    showLoadIndication( true, progress );
    // Go and poll in #onLoadTimer
    m_loadTimer.StartOnce( 250 );
    return false;
}


void DIALOG_FOOTPRINT_USERS::showUsers( const FOOTPRINT_USERS_RESULT& aResult )
{
    std::vector<FOOTPRINT_USER_MATCH> matches = aResult.m_Matches;

    const auto libThenSymbolNameCmp =
            []( const FOOTPRINT_USER_MATCH& aLeft, const FOOTPRINT_USER_MATCH& aRight )
            {
                const int libCmp = StrNumCmp( aLeft.m_Symbol.GetUniStringLibNickname(),
                                              aRight.m_Symbol.GetUniStringLibNickname(),
                                              true );

                if( libCmp != 0 )
                    return libCmp < 0;

                return StrNumCmp( aLeft.m_Symbol.GetUniStringLibItemName(),
                                  aRight.m_Symbol.GetUniStringLibItemName(),
                                  true ) < 0;
            };

    std::sort( matches.begin(), matches.end(), libThenSymbolNameCmp );

    wxWindowUpdateLocker lock( m_footprintUserList );
    m_footprintUserList->DeleteAllItems();

    for( const FOOTPRINT_USER_MATCH& match : matches )
    {
        wxVector<wxVariant> row;

        row.push_back( wxVariant( match.m_Symbol.GetUniStringLibNickname() ) );
        row.push_back( wxVariant( match.m_Symbol.GetUniStringLibItemName() ) );
        row.push_back( wxVariant( wxString::Format( wxS( "%d" ), match.m_PinCount ) ) );
        row.push_back( wxVariant( match.m_MatchedFilter ) );
        row.push_back( wxVariant( match.m_MatchesFootprintField ) );

        // The pin map column names the maps bound to the footprint rather than ticking a box.
        wxString pinMapNames;

        for( const wxString& mapName : match.m_MatchedPinMaps )
        {
            if( !pinMapNames.IsEmpty() )
                pinMapNames += wxS( ", " );

            pinMapNames += mapName;
        }

        row.push_back( wxVariant( pinMapNames ) );

        wxUIntPtr rowData = 0;

        if( match.m_PinCount == m_footprintPinCount )
            rowData |= ROW_DATA_FLAG_PIN_COUNT_MATCHES;

        if( match.m_IsDerived )
            rowData |= ROW_DATA_FLAG_DERIVED_SYMBOL;

        m_footprintUserList->AppendItem( row, rowData );
    }

    if( !aResult.m_Success )
    {
        m_statusText->SetLabel( _( "Failed to query matching symbols." ) );
    }
    else if( aResult.m_IsLimited )
    {
        m_statusText->SetLabel(
                wxString::Format( _( "This footprint is referenced by more than %zu symbols" ), matches.size() ) );
    }
    else
    {
        m_statusText->SetLabel(
                wxString::Format( _( "This footprint is referenced by %zu symbols" ), matches.size() ) );
    }

    Layout();
}


void DIALOG_FOOTPRINT_USERS::showLoadIndication( bool aShow, float aProgress )
{
    m_footprintUserList->Enable( !aShow );

    if( aShow )
    {
        m_statusText->SetLabel(
                wxString::Format( _( "Loading symbol libraries (%d%%)..." ), static_cast<int>( aProgress * 100.0f ) ) );
    }
    else
    {
        m_footprintUserList->DeleteAllItems();
        m_statusText->SetLabel( wxEmptyString );
    }
}


void DIALOG_FOOTPRINT_USERS::onMatchTypeChanged( wxCommandEvent& aEvent )
{
    updateUsers();
}


void DIALOG_FOOTPRINT_USERS::onLoadTimer( wxTimerEvent& aEvent )
{
    if( !symbolLibrariesReady() )
        return;

    // Inputs may have changed while the libraries were loading.
    updateUsers();
}


wxString DIALOG_FOOTPRINT_USERS::userSymbolName( int aRow ) const
{
    // Events can have wxNOT_FOUND as the row.
    if( aRow < 0 || aRow >= static_cast<int>( m_footprintUserList->GetItemCount() ) )
        return wxEmptyString;

    const wxString library = m_footprintUserList->GetTextValue( aRow, COL_FOOTPRINT_USERS_LIBRARY );
    const wxString symbol = m_footprintUserList->GetTextValue( aRow, COL_FOOTPRINT_USERS_SYMBOL );

    // A row without a library and symbol is not a match.
    if( library.IsEmpty() || symbol.IsEmpty() )
        return wxEmptyString;

    LIB_ID symbolId;

    if( symbolId.Parse( library + wxS( ":" ) + symbol ) != -1 || !symbolId.IsValid() )
        return wxEmptyString;

    return symbolId.Format().wx_str();
}


void DIALOG_FOOTPRINT_USERS::openSymbolInEditor( const wxString& aSymbolName )
{
    m_kiway.Player( FRAME_SCH_SYMBOL_EDITOR, true );

    std::string packet = aSymbolName.utf8_string();
    m_kiway.ExpressMail( FRAME_SCH_SYMBOL_EDITOR, MAIL_SCH_EDIT_LIBID, packet, this );

    // Close the dialog to avoid a modal focus fight
    EndModal( wxID_OK );
}


void DIALOG_FOOTPRINT_USERS::copySymbolName( const wxString& aSymbolName )
{
    if( wxTheClipboard->Open() )
    {
        wxTheClipboard->SetData( new wxTextDataObject( aSymbolName ) );
        wxTheClipboard->Close();
    }
}


void DIALOG_FOOTPRINT_USERS::onUserActivated( wxDataViewEvent& aEvent )
{
    wxString symbolName = userSymbolName( m_footprintUserList->ItemToRow( aEvent.GetItem() ) );

    if( !symbolName.IsEmpty() )
        openSymbolInEditor( symbolName );
}


void DIALOG_FOOTPRINT_USERS::onUserRightClick( wxDataViewEvent& aEvent )
{
    const wxDataViewItem item = aEvent.GetItem();

    // The context menu can be asked for from the empty area below the rows.
    if( !item.IsOk() )
        return;

    wxString symbolName = userSymbolName( m_footprintUserList->ItemToRow( item ) );

    if( symbolName.IsEmpty() )
        return;

    enum
    {
        OPEN_IN_SYMBOL_EDITOR = 1,
        COPY_SYMBOL_NAME
    };

    wxMenu menu;

    menu.Append( OPEN_IN_SYMBOL_EDITOR, _( "Open in Symbol Editor" ) );
    menu.Append( COPY_SYMBOL_NAME, _( "Copy Symbol Name" ) + "\tCtrl+C" );

    switch( GetPopupMenuSelectionFromUser( menu ) )
    {
    case OPEN_IN_SYMBOL_EDITOR:
        openSymbolInEditor( symbolName );
        break;
    case COPY_SYMBOL_NAME:
        copySymbolName( symbolName );
        break;
    default:
        break;
    }
}


void DIALOG_FOOTPRINT_USERS::onUserKeyDown( wxKeyEvent& aEvent )
{
    if( aEvent.ControlDown() && aEvent.GetKeyCode() == 'C' )
    {
        wxString symbolName = userSymbolName( m_footprintUserList->GetSelectedRow() );

        if( !symbolName.IsEmpty() )
        {
            copySymbolName( symbolName );
            return;
        }
    }

    aEvent.Skip();
}
