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
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <lib_table_grid_data_model.h>
#include <string_utils.h>
#include <libraries/library_manager.h>
#include <widgets/grid_button.h>
#include <bitmaps.h>
#include <widgets/grid_text_button_helpers.h>


LIB_TABLE_GRID_DATA_MODEL::LIB_TABLE_GRID_DATA_MODEL( DIALOG_SHIM* aDialog, WX_GRID* aGrid,
                                                      const LIBRARY_TABLE& aTableToEdit,
                                                      LIBRARY_MANAGER_ADAPTER* aAdapter,
                                                      const wxArrayString& aPluginChoices,
                                                      wxString* aMRUDirectory, const wxString& aProjectPath ) :
        m_table( aTableToEdit ),
        m_adapter( aAdapter )
{
    m_uriEditor = new wxGridCellAttr;
    m_uriEditor->SetEditor( new GRID_CELL_PATH_EDITOR( aDialog, aGrid, aMRUDirectory, !aProjectPath.IsEmpty(),
                                                       aProjectPath,
                                                       [this]( WX_GRID* aCurrGrid, int aRow ) -> wxString
                                                       {
                                                           return getFileTypes( aCurrGrid, aRow );
                                                       } ) );

    m_typesEditor = new wxGridCellAttr;
    m_typesEditor->SetEditor( new wxGridCellChoiceEditor( aPluginChoices ) );

    m_boolAttr = new wxGridCellAttr;
    m_boolAttr->SetRenderer( new wxGridCellBoolRenderer() );
    m_boolAttr->SetReadOnly(); // not really; we delegate interactivity to GRID_TRICKS
    m_boolAttr->SetAlignment( wxALIGN_CENTER, wxALIGN_CENTER );

    m_warningAttr = new wxGridCellAttr;
    m_warningAttr->SetRenderer( new GRID_BITMAP_BUTTON_RENDERER( KiBitmapBundle( BITMAPS::small_warning ) ) );
    m_warningAttr->SetReadOnly();
    m_warningAttr->SetAlignment( wxALIGN_CENTER, wxALIGN_CENTER );

    m_noStatusAttr = new wxGridCellAttr;
    m_noStatusAttr->SetReadOnly();
    m_noStatusAttr->SetAlignment( wxALIGN_CENTER, wxALIGN_CENTER );

    m_editSettingsAttr = new wxGridCellAttr;
    m_editSettingsAttr->SetRenderer( new GRID_BITMAP_BUTTON_RENDERER( KiBitmapBundle( BITMAPS::config ) ) );
    m_editSettingsAttr->SetReadOnly(); // not really; we delegate interactivity to GRID_TRICKS
    m_editSettingsAttr->SetAlignment( wxALIGN_CENTER, wxALIGN_CENTER );

    m_openTableAttr = new wxGridCellAttr;
    m_openTableAttr->SetRenderer( new GRID_BITMAP_BUTTON_RENDERER( KiBitmapBundle( BITMAPS::small_new_window ) ) );
    m_openTableAttr->SetReadOnly(); // not really; we delegate interactivity to GRID_TRICKS
    m_openTableAttr->SetAlignment( wxALIGN_CENTER, wxALIGN_CENTER );
}


LIB_TABLE_GRID_DATA_MODEL::~LIB_TABLE_GRID_DATA_MODEL()
{
    m_uriEditor->DecRef();
    m_typesEditor->DecRef();
    m_boolAttr->DecRef();
    m_warningAttr->DecRef();
    m_noStatusAttr->DecRef();
    m_editSettingsAttr->DecRef();
    m_openTableAttr->DecRef();
}


bool LIB_TABLE_GRID_DATA_MODEL::badCoords( int aRow, int aCol )
{
    if( aRow < 0 || aRow >= (int) size() )
        return true;

    if( aCol < 0 || aCol >= GetNumberCols() )
        return true;

    return false;
}


wxString LIB_TABLE_GRID_DATA_MODEL::GetValue( int aRow, int aCol )
{
    if( badCoords( aRow, aCol ) )
        return wxEmptyString;

    const LIBRARY_TABLE_ROW& r = at( aRow );

    switch( aCol )
    {
    case COL_NICKNAME: return UnescapeString( r.Nickname() );
    case COL_URI:      return r.URI();
    case COL_TYPE:     return r.Type();
    case COL_OPTIONS:  return r.Options();
    case COL_DESCR:    return r.Description();
    case COL_ENABLED:  return r.Disabled() ? wxT( "0" ) : wxT( "1" );
    case COL_VISIBLE:  return r.Hidden() ? wxT( "0" ) : wxT( "1" );

    case COL_STATUS:
        if( !r.IsOk() )
            return r.ErrorDescription();

        if( std::optional<LIBRARY_ERROR> error = m_adapter->LibraryError( r.Nickname() ) )
            return error->message;

        if( m_adapter->SupportsConfigurationDialog( r.Nickname() ) )
            return _( "Edit settings" );
        else if( r.Type() == LIBRARY_TABLE_ROW::TABLE_TYPE_NAME )
            return _( "Open library table" );

        return wxEmptyString;

    default:
        return wxEmptyString;
    }
}


wxGridCellAttr* LIB_TABLE_GRID_DATA_MODEL::GetAttr( int aRow, int aCol, wxGridCellAttr::wxAttrKind aKind )
{
    if( badCoords( aRow, aCol ) )
        return enhanceAttr( nullptr, aRow, aCol, aKind );

    LIBRARY_TABLE_ROW& tableRow = at( aRow );

    switch( aCol )
    {
    case COL_URI:
        m_uriEditor->IncRef();
        return enhanceAttr( m_uriEditor, aRow, aCol, aKind );

    case COL_TYPE:
        m_typesEditor->IncRef();
        return enhanceAttr( m_typesEditor, aRow, aCol, aKind );

    case COL_ENABLED:
    case COL_VISIBLE:
        m_boolAttr->IncRef();
        return enhanceAttr( m_boolAttr, aRow, aCol, aKind );

    case COL_STATUS:
        if( !tableRow.IsOk() )
        {
            m_warningAttr->IncRef();
            return enhanceAttr( m_warningAttr, aRow, aCol, aKind );
        }

        if( std::optional<LIBRARY_ERROR> error = m_adapter->LibraryError( tableRow.Nickname() ) )
        {
            m_warningAttr->IncRef();
            return enhanceAttr( m_warningAttr, aRow, aCol, aKind );
        }

        if( m_adapter->SupportsConfigurationDialog( tableRow.Nickname() ) )
        {
            m_editSettingsAttr->IncRef();
            return enhanceAttr( m_editSettingsAttr, aRow, aCol, aKind );
        }
        else if( tableRow.Type() == LIBRARY_TABLE_ROW::TABLE_TYPE_NAME )
        {
            m_openTableAttr->IncRef();
            return enhanceAttr( m_openTableAttr, aRow, aCol, aKind );
        }

        m_noStatusAttr->IncRef();
        return enhanceAttr( m_noStatusAttr, aRow, aCol, aKind );

    case COL_NICKNAME:
    case COL_OPTIONS:
    case COL_DESCR:
    default:
        return enhanceAttr( nullptr, aRow, aCol, aKind );
    }
}


bool LIB_TABLE_GRID_DATA_MODEL::CanGetValueAs( int aRow, int aCol, const wxString& aTypeName )
{
    if( badCoords( aRow, aCol ) )
        return false;

    switch( aCol )
    {
    case COL_ENABLED:
    case COL_VISIBLE:
        return aTypeName == wxGRID_VALUE_BOOL;

    default:
        return aTypeName == wxGRID_VALUE_STRING;
    }
}


bool LIB_TABLE_GRID_DATA_MODEL::GetValueAsBool( int aRow, int aCol )
{
    if( badCoords( aRow, aCol ) )
        return false;

    if( aCol == COL_ENABLED )
        return !at( aRow ).Disabled();
    else if( aCol == COL_VISIBLE )
        return !at( aRow ).Hidden();
    else
        return false;
}


void LIB_TABLE_GRID_DATA_MODEL::SetValue( int aRow, int aCol, const wxString& aValue )
{
    if( badCoords( aRow, aCol ) )
        return;

    LIBRARY_TABLE_ROW& lrow = at( aRow );

    switch( aCol )
    {
    case COL_NICKNAME: lrow.SetNickname( EscapeString( aValue, CTX_LIBID ) );  break;
    case COL_URI:      lrow.SetURI( aValue );                                  break;
    case COL_TYPE:     lrow.SetType( aValue );                                 break;
    case COL_OPTIONS:  lrow.SetOptions( aValue );                              break;
    case COL_DESCR:    lrow.SetDescription( aValue );                          break;
    case COL_ENABLED:  lrow.SetDisabled( aValue == wxT( "0" ) );               break;
    case COL_VISIBLE:  lrow.SetHidden( aValue == wxT( "0" ) );                 break;
    case COL_STATUS:                                                           break;
    }

    if( aCol == COL_URI || aCol == COL_TYPE || aCol == COL_OPTIONS )
    {
        GetView()->CallAfter(
                [this, aRow, aCol]()
                {
                    if( badCoords( aRow, aCol ) )
                        return;

                    LIBRARY_TABLE_ROW& r = at( aRow );

                    m_adapter->CheckTableRow( r );

                    GetView()->RefreshBlock( aRow, COL_STATUS, aRow, COL_STATUS );
                } );
    }
}


void LIB_TABLE_GRID_DATA_MODEL::SetValueAsBool( int aRow, int aCol, bool aValue )
{
    if( badCoords( aRow, aCol ) )
        return;

    if( aCol == COL_ENABLED )
        at( aRow ).SetDisabled( !aValue );
    else if( aCol == COL_VISIBLE )
        at( aRow ).SetHidden( !aValue );
}


bool LIB_TABLE_GRID_DATA_MODEL::InsertRows( size_t aPos, size_t aNumRows  )
{
    if( aPos < size() )
    {
        for( size_t i = 0; i < aNumRows; i++ )
            insert( begin() + i, makeNewRow() );

        // use the (wxGridStringTable) source Luke.
        if( GetView() )
        {
            wxGridTableMessage msg( this, wxGRIDTABLE_NOTIFY_ROWS_INSERTED, aPos, aNumRows );
            GetView()->ProcessTableMessage( msg );
        }

        return true;
    }

    return false;
}


bool LIB_TABLE_GRID_DATA_MODEL::AppendRows( size_t aNumRows )
{
    // do not modify aNumRows, original value needed for wxGridTableMessage below
    for( int i = aNumRows; i; --i )
        push_back( makeNewRow() );

    if( GetView() )
    {
        wxGridTableMessage msg( this, wxGRIDTABLE_NOTIFY_ROWS_APPENDED, aNumRows );
        GetView()->ProcessTableMessage( msg );
    }

    return true;
}


bool LIB_TABLE_GRID_DATA_MODEL::DeleteRows( size_t aPos, size_t aNumRows )
{
    // aPos may be a large positive, e.g. size_t(-1), and the sum of
    // aPos+aNumRows may wrap here, so both ends of the range are tested.
    if( aPos < size() && aPos + aNumRows <= size() )
    {
        LIBRARY_TABLE_ROWS_ITER start = begin() + aPos;
        erase( start, start + aNumRows );

        if( GetView() )
        {
            wxGridTableMessage msg( this, wxGRIDTABLE_NOTIFY_ROWS_DELETED, aPos, aNumRows );
            GetView()->ProcessTableMessage( msg );
        }

        return true;
    }

    return false;
}


wxString LIB_TABLE_GRID_DATA_MODEL::GetColLabelValue( int aCol )
{
    switch( aCol )
    {
    case COL_NICKNAME:  return _( "Nickname" );
    case COL_URI:       return _( "Library Path" );

    // keep this "Library Format" text fairly long so column is sized wide enough
    case COL_TYPE:      return _( "Library Format" );
    case COL_OPTIONS:   return _( "Options" );
    case COL_DESCR:     return _( "Description" );
    case COL_ENABLED:   return _( "Enable" );
    case COL_VISIBLE:   return _( "Show" );
    case COL_STATUS:    return wxEmptyString;

    default:            return wxEmptyString;
    }
}


bool LIB_TABLE_GRID_DATA_MODEL::ContainsNickname( const wxString& aNickname )
{
    for( size_t i = 0; i < size(); ++i )
    {
        LIBRARY_TABLE_ROW& row = at( i );

        if( row.Nickname() == aNickname )
            return true;
    }
    return false;
}


LIBRARY_TABLE_ROW& LIB_TABLE_GRID_DATA_MODEL::at( size_t aIndex )
{
    return m_table.Rows().at( aIndex );
}


size_t LIB_TABLE_GRID_DATA_MODEL::size() const
{
    return m_table.Rows().size();
}


LIBRARY_TABLE_ROW LIB_TABLE_GRID_DATA_MODEL::makeNewRow()
{
    return m_table.MakeRow();
}


LIBRARY_TABLE_ROWS_ITER LIB_TABLE_GRID_DATA_MODEL::begin()
{
    return m_table.Rows().begin();
}


LIBRARY_TABLE_ROWS_ITER LIB_TABLE_GRID_DATA_MODEL::insert( LIBRARY_TABLE_ROWS_ITER aIterator,
                                                           const LIBRARY_TABLE_ROW& aRow )
{
    return m_table.Rows().insert( aIterator, aRow );
}


void LIB_TABLE_GRID_DATA_MODEL::push_back( const LIBRARY_TABLE_ROW& aRow )
{
    m_table.Rows().push_back( aRow );
}


LIBRARY_TABLE_ROWS_ITER LIB_TABLE_GRID_DATA_MODEL::erase( LIBRARY_TABLE_ROWS_ITER aFirst,
                                                          LIBRARY_TABLE_ROWS_ITER aLast )
{
    return m_table.Rows().erase( aFirst, aLast );
}
