/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2017 Oliver Walters
 * Copyright (C) 2017-2023 KiCad Developers, see AUTHORS.txt for contributors.
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


#include <base_units.h>
#include <bitmaps.h>
#include <symbol_library.h>
#include <confirm.h>
#include <eda_doc.h>
#include <wildcards_and_files_ext.h>
#include <schematic_settings.h>
#include <general.h>
#include <grid_tricks.h>
#include <string_utils.h>
#include <kiface_base.h>
#include <sch_edit_frame.h>
#include <sch_reference_list.h>
#include <schematic.h>
#include <tools/sch_editor_control.h>
#include <kiplatform/ui.h>
#include <widgets/grid_text_button_helpers.h>
#include <widgets/bitmap_button.h>
#include <widgets/wx_grid.h>
#include <wx/debug.h>
#include <wx/ffile.h>
#include <wx/grid.h>
#include <wx/textdlg.h>
#include <wx/filedlg.h>
#include <dialogs/eda_view_switcher.h>
#include "dialog_symbol_fields_table.h"
#include "eda_list_dialog.h"

// The field name in the data model (translated)
#define DISPLAY_NAME_COLUMN   0
// The field name's label for exporting (CSV, etc.)
#define LABEL_COLUMN          1
#define SHOW_FIELD_COLUMN     2
#define GROUP_BY_COLUMN       3
// The internal field name (untranslated)
#define FIELD_NAME_COLUMN     4

#ifdef __WXMAC__
#define COLUMN_MARGIN 5
#else
#define COLUMN_MARGIN 15
#endif

enum
{
    MYID_SELECT_FOOTPRINT = GRIDTRICKS_FIRST_CLIENT_ID,
    MYID_SHOW_DATASHEET
};


class FIELDS_EDITOR_GRID_TRICKS : public GRID_TRICKS
{
public:
    FIELDS_EDITOR_GRID_TRICKS( DIALOG_SHIM* aParent, WX_GRID* aGrid,
                               wxDataViewListCtrl* aFieldsCtrl ) :
            GRID_TRICKS( aGrid ),
            m_dlg( aParent ),
            m_fieldsCtrl( aFieldsCtrl )
    {}

protected:
    void showPopupMenu( wxMenu& menu, wxGridEvent& aEvent ) override
    {
        if( m_grid->GetGridCursorCol() == FOOTPRINT_FIELD )
        {
            menu.Append( MYID_SELECT_FOOTPRINT, _( "Select Footprint..." ),
                         _( "Browse for footprint" ) );
            menu.AppendSeparator();
        }
        else if( m_grid->GetGridCursorCol() == DATASHEET_FIELD )
        {
            menu.Append( MYID_SHOW_DATASHEET, _( "Show Datasheet" ),
                         _( "Show datasheet in browser" ) );
            menu.AppendSeparator();
        }

        GRID_TRICKS::showPopupMenu( menu, aEvent );
    }

    void doPopupSelection( wxCommandEvent& event ) override
    {
        if( event.GetId() == MYID_SELECT_FOOTPRINT )
        {
            // pick a footprint using the footprint picker.
            wxString      fpid = m_grid->GetCellValue( m_grid->GetGridCursorRow(),
                                                       FOOTPRINT_FIELD );
            KIWAY_PLAYER* frame = m_dlg->Kiway().Player( FRAME_FOOTPRINT_VIEWER_MODAL, true,
                                                         m_dlg );

            if( frame->ShowModal( &fpid, m_dlg ) )
                m_grid->SetCellValue( m_grid->GetGridCursorRow(), FOOTPRINT_FIELD, fpid );

            frame->Destroy();
        }
        else if (event.GetId() == MYID_SHOW_DATASHEET )
        {
            wxString datasheet_uri = m_grid->GetCellValue( m_grid->GetGridCursorRow(),
                                                           DATASHEET_FIELD );
            GetAssociatedDocument( m_dlg, datasheet_uri, &m_dlg->Prj(), m_dlg->Prj().SchSearchS() );
        }
        else
        {
            GRID_TRICKS::doPopupSelection( event );
        }

        if( event.GetId() >= GRIDTRICKS_FIRST_SHOWHIDE && event.GetId() < GRIDTRICKS_LAST_ID )
        {
            if( !m_grid->IsColShown( REFERENCE_FIELD ) )
            {
                DisplayError( m_dlg, _( "The Reference column cannot be hidden." ) );

                m_grid->ShowCol( REFERENCE_FIELD );
            }

            // Refresh Show checkboxes from grid columns
            for( int i = 0; i < m_fieldsCtrl->GetItemCount(); ++i )
                m_fieldsCtrl->SetToggleValue( m_grid->IsColShown( i ), i, 1 );
        }
    }

    DIALOG_SHIM*        m_dlg;
    wxDataViewListCtrl* m_fieldsCtrl;
};


enum GROUP_TYPE
{
    GROUP_SINGLETON,
    GROUP_COLLAPSED,
    GROUP_COLLAPSED_DURING_SORT,
    GROUP_EXPANDED,
    CHILD_ITEM
};


BOM_PRESET DIALOG_SYMBOL_FIELDS_TABLE::bomPresetGroupedByValue(
        _HKI( "Grouped By Value" ),
        std::map<std::string, bool>( {
                std::pair<std::string, bool>( "Reference", true ),
                std::pair<std::string, bool>( "Value", true ),
                std::pair<std::string, bool>( "Datasheet", true ),
                std::pair<std::string, bool>( "Footprint", true ),
                std::pair<std::string, bool>( "Quantity", true ),
        } ),
        std::map<std::string, bool>( {
                std::pair<std::string, bool>( "Reference", false ),
                std::pair<std::string, bool>( "Value", true ),
                std::pair<std::string, bool>( "Datasheet", false ),
                std::pair<std::string, bool>( "Footprint", false ),
                std::pair<std::string, bool>( "Quantity", false ),
        } ),
        std::map<std::string, int>(), std::map<std::string, bool>(), std::vector<wxString>(),
        _HKI( "" ), true );


BOM_PRESET DIALOG_SYMBOL_FIELDS_TABLE::bomPresetGroupedByValueFootprint(
        _HKI( "Grouped By Value and Footprint" ),
        std::map<std::string, bool>( {
                std::pair<std::string, bool>( "Reference", true ),
                std::pair<std::string, bool>( "Value", true ),
                std::pair<std::string, bool>( "Datasheet", true ),
                std::pair<std::string, bool>( "Footprint", true ),
                std::pair<std::string, bool>( "Quantity", true ),
        } ),
        std::map<std::string, bool>( {
                std::pair<std::string, bool>( "Reference", false ),
                std::pair<std::string, bool>( "Value", true ),
                std::pair<std::string, bool>( "Datasheet", false ),
                std::pair<std::string, bool>( "Footprint", true ),
                std::pair<std::string, bool>( "Quantity", false ),
        } ),
        std::map<std::string, int>(), std::map<std::string, bool>(), std::vector<wxString>(),
        _HKI( "" ), true );


struct DATA_MODEL_ROW
{
    DATA_MODEL_ROW( const SCH_REFERENCE& aFirstReference, GROUP_TYPE aType )
    {
        m_Refs.push_back( aFirstReference );
        m_Flag = aType;
    }

    GROUP_TYPE                 m_Flag;
    std::vector<SCH_REFERENCE> m_Refs;
};


struct DATA_MODEL_COL
{
    wxString m_fieldName;
    wxString m_label;
    bool     m_userAdded;
};


class FIELDS_EDITOR_GRID_DATA_MODEL : public wxGridTableBase
{
protected:
    // The data model is fundamentally m_componentRefs X m_fieldNames.

    SCH_EDIT_FRAME*       m_frame;
    SCH_REFERENCE_LIST    m_symbolsList;
    bool                  m_edited;
    int                   m_sortColumn;
    bool                  m_sortAscending;
    std::vector<DATA_MODEL_COL> m_cols;

    // However, the grid view can vary in two ways:
    //   1) the componentRefs can be grouped into fewer rows
    //   2) some columns can be hidden
    //
    // We handle (1) here (ie: a table row maps to a group, and the table is rebuilt
    // when the groupings change), and we let the wxGrid handle (2) (ie: the number
    // of columns is constant but are hidden/shown by the wxGrid control).

    std::vector< DATA_MODEL_ROW > m_rows;

    // Data store
    // A map of compID : fieldSet, where fieldSet is a map of fieldName : fieldValue
    std::map< KIID, std::map<wxString, wxString> > m_dataStore;

public:
    FIELDS_EDITOR_GRID_DATA_MODEL( SCH_EDIT_FRAME* aFrame, SCH_REFERENCE_LIST& aSymbolsList ) :
            m_frame( aFrame ),
            m_symbolsList( aSymbolsList ),
            m_edited( false ),
            m_sortColumn( 0 ),
            m_sortAscending( false )
    {
        m_symbolsList.SplitReferences();
    }

    void AddColumn( const wxString& aFieldName, const wxString& aLabel, bool aAddedByUser )
    {
        m_cols.push_back((struct DATA_MODEL_COL) {
                .m_fieldName = aFieldName,
                .m_label = aLabel,
                .m_userAdded = aAddedByUser });

        for( unsigned i = 0; i < m_symbolsList.GetCount(); ++i )
        {
            SCH_SYMBOL* symbol = m_symbolsList[ i ].GetSymbol();

            wxCHECK( symbol && ( symbol->GetInstanceReferences().size() != 0 ), /* void */ );

            wxString val = symbol->GetFieldText( aFieldName );

            if( aFieldName == wxT( "Value" ) )
                val = symbol->GetValueFieldText( true );
            else if( aFieldName == wxT( "Footprint" ) )
                val = symbol->GetFootprintFieldText( true );

            m_dataStore[ symbol->m_Uuid ][ aFieldName ] = val;
        }
    }

    void RemoveColumn( int aCol )
    {
        for( unsigned i = 0; i < m_symbolsList.GetCount(); ++i )
        {
            SCH_SYMBOL* symbol = m_symbolsList[ i ].GetSymbol();
            m_dataStore[symbol->m_Uuid].erase( m_cols[aCol].m_fieldName );
        }

        m_cols.erase( m_cols.begin() + aCol );
    }

    void RenameColumn( int aCol, const wxString& newName )
    {
        for( unsigned i = 0; i < m_symbolsList.GetCount(); ++i )
        {
            SCH_SYMBOL* symbol = m_symbolsList[i].GetSymbol();

            auto node = m_dataStore[symbol->m_Uuid].extract( m_cols[aCol].m_fieldName );
            node.key() = newName;
            m_dataStore[symbol->m_Uuid].insert( std::move( node ) );
        }

        m_cols[aCol].m_fieldName = newName;
    }

    void MoveColumn( int aCol, int aNewPos ) { std::swap( m_cols[aCol], m_cols[aNewPos] ); }

    int GetNumberRows() override { return (int) m_rows.size(); }

    int GetNumberCols() override { return (int) m_cols.size(); }

    void SetColLabelValue( int aCol, const wxString& aLabel ) override
    {
        m_cols[aCol].m_label = aLabel;
    }

    wxString GetColLabelValue( int aCol ) override { return m_cols[aCol].m_label; }

    wxString GetColFieldName( int aCol ) { return m_cols[aCol].m_fieldName; }

    int GetFieldNameCol( wxString aFieldName )
    {
        for( size_t i = 0; i < m_cols.size(); i++ )
        {
            if( m_cols[i].m_fieldName == aFieldName )
                return (int) i;
        }

        return -1;
    }

    const std::vector<wxString> GetFieldsOrder()
    {
        std::vector<wxString> fields;

        for( auto col : m_cols )
        {
            fields.emplace_back( col.m_fieldName );
        }

        return fields;
    }

    void SetFieldsOrder( const std::vector<wxString>& aNewOrder )
    {
        size_t foundCount = 0;

        for( const wxString& newField : aNewOrder )
        {
            for( size_t i = 0; i < m_cols.size(); i++ )
            {
                if( m_cols[i].m_fieldName == newField )
                {
                    std::swap( m_cols[foundCount], m_cols[i] );
                    foundCount++;
                }
            }
        }
    }

    bool IsEmptyCell( int aRow, int aCol ) override
    {
        return false;   // don't allow adjacent cell overflow, even if we are actually empty
    }

    wxString GetValue( int aRow, int aCol ) override
    {
        if( ColIsReference( aCol ) )
        {
            // Poor-man's tree controls
            if( m_rows[ aRow ].m_Flag == GROUP_COLLAPSED )
                return wxT( ">  " ) + GetValue( m_rows[ aRow ], aCol );
            else if (m_rows[ aRow ].m_Flag == GROUP_EXPANDED )
                return wxT( "v  " ) + GetValue( m_rows[ aRow ], aCol );
            else if( m_rows[ aRow ].m_Flag == CHILD_ITEM )
                return wxT( "        " ) + GetValue( m_rows[ aRow ], aCol );
            else
                return wxT( "    " ) + GetValue( m_rows[ aRow ], aCol );
        }
        else
        {
            return GetValue( m_rows[ aRow ], aCol );
        }
    }

    wxString GetRawValue( int aRow, int aCol )
    {
        return GetValue( m_rows[ aRow ], aCol );
    }

    GROUP_TYPE GetRowFlags( int aRow )
    {
        return m_rows[ aRow ].m_Flag;
    }

    std::vector<SCH_REFERENCE> GetRowReferences( int aRow ) const
    {
        wxCHECK( aRow < (int)m_rows.size(), std::vector<SCH_REFERENCE>() );
        return m_rows[ aRow ].m_Refs;
    }

    bool ColIsReference( int aCol )
    {
        return ( aCol < (int) m_cols.size() ) && m_cols[aCol].m_fieldName == _( "Reference" );
    }

    bool ColIsQuantity( int aCol )
    {
        return ( aCol < (int) m_cols.size() ) && m_cols[aCol].m_fieldName == _( "Qty" );
    }

    wxString GetValue( const DATA_MODEL_ROW& group, int aCol )
    {
        std::vector<SCH_REFERENCE> references;
        wxString                   fieldValue;

        for( const SCH_REFERENCE& ref : group.m_Refs )
        {
            if( ColIsReference( aCol ) || ColIsQuantity( aCol ) )
            {
                references.push_back( ref );
            }
            else // Other columns are either a single value or ROW_MULTI_ITEMS
            {
                const KIID& symbolID = ref.GetSymbol()->m_Uuid;

                if( !m_dataStore.count( symbolID )
                    || !m_dataStore[symbolID].count( m_cols[aCol].m_fieldName ) )
                {
                    return INDETERMINATE_STATE;
                }

                if( &ref == &group.m_Refs.front() )
                    fieldValue = m_dataStore[symbolID][m_cols[aCol].m_fieldName];
                else if( fieldValue != m_dataStore[symbolID][m_cols[aCol].m_fieldName] )
                    return INDETERMINATE_STATE;
            }
        }

        if( ColIsReference( aCol ) || ColIsQuantity( aCol ) )
        {
            // Remove duplicates (other units of multi-unit parts)
            std::sort( references.begin(), references.end(),
                    []( const SCH_REFERENCE& l, const SCH_REFERENCE& r ) -> bool
                    {
                        wxString l_ref( l.GetRef() << l.GetRefNumber() );
                        wxString r_ref( r.GetRef() << r.GetRefNumber() );
                        return StrNumCmp( l_ref, r_ref, true ) < 0;
                    } );

            auto logicalEnd = std::unique( references.begin(), references.end(),
                    []( const SCH_REFERENCE& l, const SCH_REFERENCE& r ) -> bool
                    {
                        // If unannotated then we can't tell what units belong together
                        // so we have to leave them all
                        if( l.GetRefNumber() == wxT( "?" ) )
                            return false;

                        wxString l_ref( l.GetRef() << l.GetRefNumber() );
                        wxString r_ref( r.GetRef() << r.GetRefNumber() );
                        return l_ref == r_ref;
                    } );

            references.erase( logicalEnd, references.end() );
        }

        if( ColIsReference( aCol ) )
            fieldValue = SCH_REFERENCE_LIST::Shorthand( references );
        else if( ColIsQuantity( aCol ) )
            fieldValue = wxString::Format( wxT( "%d" ), ( int )references.size() );

        return fieldValue;
    }

    void SetValue( int aRow, int aCol, const wxString &aValue ) override
    {
        if( ColIsReference( aCol ) || ColIsQuantity( aCol ) )
            return;             // Can't modify references or quantity

        DATA_MODEL_ROW& rowGroup = m_rows[aRow];

        for( const SCH_REFERENCE& ref : rowGroup.m_Refs )
            m_dataStore[ref.GetSymbol()->m_Uuid][m_cols[aCol].m_fieldName] = aValue;

        m_edited = true;
    }

    static bool cmp( const DATA_MODEL_ROW& lhGroup, const DATA_MODEL_ROW& rhGroup,
                     FIELDS_EDITOR_GRID_DATA_MODEL* dataModel, int sortCol, bool ascending )
    {
        // Empty rows always go to the bottom, whether ascending or descending
        if( lhGroup.m_Refs.size() == 0 )
            return true;
        else if( rhGroup.m_Refs.size() == 0 )
            return false;

        // N.B. To meet the iterator sort conditions, we cannot simply invert the truth
        // to get the opposite sort.  i.e. ~(a<b) != (a>b)
        auto local_cmp =
                [ ascending ]( const auto a, const auto b )
                {
                    if( ascending )
                        return a < b;
                    else
                        return a > b;
                };

        // Primary sort key is sortCol; secondary is always REFERENCE (column 0)

        wxString lhs = dataModel->GetValue( (DATA_MODEL_ROW&) lhGroup, sortCol );
        wxString rhs = dataModel->GetValue( (DATA_MODEL_ROW&) rhGroup, sortCol );

        if( lhs == rhs || sortCol == REFERENCE_FIELD )
        {
            wxString lhRef = lhGroup.m_Refs[ 0 ].GetRef() + lhGroup.m_Refs[ 0 ].GetRefNumber();
            wxString rhRef = rhGroup.m_Refs[ 0 ].GetRef() + rhGroup.m_Refs[ 0 ].GetRefNumber();
            return local_cmp( StrNumCmp( lhRef, rhRef, true ), 0 );
        }
        else
        {
            return local_cmp( ValueStringCompare( lhs, rhs ), 0 );
        }
    }

    void Sort( int aColumn, bool ascending )
    {
        if( aColumn < 0 )
            aColumn = 0;

        m_sortColumn = aColumn;
        m_sortAscending = ascending;

        CollapseForSort();

        // We're going to sort the rows based on their first reference, so the first reference
        // had better be the lowest one.
        for( DATA_MODEL_ROW& row : m_rows )
        {
            std::sort( row.m_Refs.begin(), row.m_Refs.end(),
                    []( const SCH_REFERENCE& lhs, const SCH_REFERENCE& rhs )
                    {
                        wxString lhs_ref( lhs.GetRef() << lhs.GetRefNumber() );
                        wxString rhs_ref( rhs.GetRef() << rhs.GetRefNumber() );
                        return StrNumCmp( lhs_ref, rhs_ref, true ) < 0;
                    } );
        }

        std::sort( m_rows.begin(), m_rows.end(),
               [this]( const DATA_MODEL_ROW& lhs, const DATA_MODEL_ROW& rhs ) -> bool
               {
                   return cmp( lhs, rhs, this, m_sortColumn, m_sortAscending );
               } );

        ExpandAfterSort();
    }

    bool unitMatch( const SCH_REFERENCE& lhRef, const SCH_REFERENCE& rhRef )
    {
        // If items are unannotated then we can't tell if they're units of the same symbol or not
        if( lhRef.GetRefNumber() == wxT( "?" ) )
            return false;

        return ( lhRef.GetRef() == rhRef.GetRef() && lhRef.GetRefNumber() == rhRef.GetRefNumber() );
    }

    bool groupMatch( const SCH_REFERENCE& lhRef, const SCH_REFERENCE& rhRef,
                     wxDataViewListCtrl* fieldsCtrl )
    {
        bool matchFound = false;

        // First check the reference column.  This can be done directly out of the
        // SCH_REFERENCEs as the references can't be edited in the grid.
        if( fieldsCtrl->GetToggleValue( REFERENCE_FIELD, GROUP_BY_COLUMN ) )
        {
            // if we're grouping by reference, then only the prefix must match
            if( lhRef.GetRef() != rhRef.GetRef() )
                return false;

            matchFound = true;
        }

        const KIID& lhRefID = lhRef.GetSymbol()->m_Uuid;
        const KIID& rhRefID = rhRef.GetSymbol()->m_Uuid;

        // Now check all the other columns.  This must be done out of the dataStore
        // for the refresh button to work after editing.
        for( int i = REFERENCE_FIELD + 1; i < fieldsCtrl->GetItemCount(); ++i )
        {
            if( !fieldsCtrl->GetToggleValue( i, GROUP_BY_COLUMN ) )
                continue;

            wxString fieldName = fieldsCtrl->GetTextValue( i, FIELD_NAME_COLUMN );

            if( m_dataStore[ lhRefID ][ fieldName ] != m_dataStore[ rhRefID ][ fieldName ] )
                return false;

            matchFound = true;
        }

        return matchFound;
    }

    void RebuildRows( wxSearchCtrl* aFilter, wxCheckBox* aGroupSymbolsBox,
                      wxDataViewListCtrl* aFieldsCtrl )
    {
        if( GetView() )
        {
            // Commit any pending in-place edits before the row gets moved out from under
            // the editor.
            static_cast<WX_GRID*>( GetView() )->CommitPendingChanges( true );

            wxGridTableMessage msg( this, wxGRIDTABLE_NOTIFY_ROWS_DELETED, 0, m_rows.size() );
            GetView()->ProcessTableMessage( msg );
        }

        m_rows.clear();

        for( unsigned i = 0; i < m_symbolsList.GetCount(); ++i )
        {
            SCH_REFERENCE ref = m_symbolsList[ i ];

            if( !aFilter->GetValue().IsEmpty()
                    && !WildCompareString( aFilter->GetValue(), ref.GetFullRef(), false ) )
            {
                continue;
            }

            bool matchFound = false;

            // See if we already have a row which this symbol fits into
            for( DATA_MODEL_ROW& row : m_rows )
            {
                // all group members must have identical refs so just use the first one
                SCH_REFERENCE rowRef = row.m_Refs[ 0 ];

                if( unitMatch( ref, rowRef ) )
                {
                    matchFound = true;
                    row.m_Refs.push_back( ref );
                    break;
                }
                else if ( aGroupSymbolsBox->GetValue() && groupMatch( ref, rowRef, aFieldsCtrl ) )
                {
                    matchFound = true;
                    row.m_Refs.push_back( ref );
                    row.m_Flag = GROUP_COLLAPSED;
                    break;
                }
            }

            if( !matchFound )
                m_rows.emplace_back( DATA_MODEL_ROW( ref, GROUP_SINGLETON ) );
        }

        if ( GetView() )
        {
            wxGridTableMessage msg( this, wxGRIDTABLE_NOTIFY_ROWS_APPENDED, m_rows.size() );
            GetView()->ProcessTableMessage( msg );
        }
    }

    void ExpandRow( int aRow )
    {
        std::vector<DATA_MODEL_ROW> children;

        for( SCH_REFERENCE& ref : m_rows[ aRow ].m_Refs )
        {
            bool matchFound = false;

            // See if we already have a child group which this symbol fits into
            for( DATA_MODEL_ROW& child : children )
            {
                // group members are by definition all matching, so just check
                // against the first member
                if( unitMatch( ref, child.m_Refs[ 0 ] ) )
                {
                    matchFound = true;
                    child.m_Refs.push_back( ref );
                    break;
                }
            }

            if( !matchFound )
                children.emplace_back( DATA_MODEL_ROW( ref, CHILD_ITEM ) );
        }

        if( children.size() < 2 )
            return;

        std::sort( children.begin(), children.end(),
                   [ this ] ( const DATA_MODEL_ROW& lhs, const DATA_MODEL_ROW& rhs ) -> bool
                   {
                       return cmp( lhs, rhs, this, m_sortColumn, m_sortAscending );
                   } );

        m_rows[ aRow ].m_Flag = GROUP_EXPANDED;
        m_rows.insert( m_rows.begin() + aRow + 1, children.begin(), children.end() );

        wxGridTableMessage msg( this, wxGRIDTABLE_NOTIFY_ROWS_INSERTED, aRow, children.size() );
        GetView()->ProcessTableMessage( msg );
    }

    void CollapseRow( int aRow )
    {
        auto firstChild = m_rows.begin() + aRow + 1;
        auto afterLastChild = firstChild;
        int  deleted = 0;

        while( afterLastChild != m_rows.end() && afterLastChild->m_Flag == CHILD_ITEM )
        {
            deleted++;
            afterLastChild++;
        }

        m_rows[ aRow ].m_Flag = GROUP_COLLAPSED;
        m_rows.erase( firstChild, afterLastChild );

        wxGridTableMessage msg( this, wxGRIDTABLE_NOTIFY_ROWS_DELETED, aRow + 1, deleted );
        GetView()->ProcessTableMessage( msg );
    }

    void ExpandCollapseRow( int aRow )
    {
        DATA_MODEL_ROW& group = m_rows[ aRow ];

        if( group.m_Flag == GROUP_COLLAPSED )
            ExpandRow( aRow );
        else if( group.m_Flag == GROUP_EXPANDED )
            CollapseRow( aRow );
    }

    void CollapseForSort()
    {
        for( size_t i = 0; i < m_rows.size(); ++i )
        {
            if( m_rows[ i ].m_Flag == GROUP_EXPANDED )
            {
                CollapseRow( i );
                m_rows[ i ].m_Flag = GROUP_COLLAPSED_DURING_SORT;
            }
        }
    }

    void ExpandAfterSort()
    {
        for( size_t i = 0; i < m_rows.size(); ++i )
        {
            if( m_rows[ i ].m_Flag == GROUP_COLLAPSED_DURING_SORT )
                ExpandRow( i );
        }
    }

    void ApplyData()
    {
        for( unsigned i = 0; i < m_symbolsList.GetCount(); ++i )
        {
            SCH_SYMBOL& symbol = *m_symbolsList[ i ].GetSymbol();
            SCH_SCREEN* screen = m_symbolsList[i].GetSheetPath().LastScreen();

            m_frame->SaveCopyInUndoList( screen, &symbol, UNDO_REDO::CHANGED, true );

            const std::map<wxString, wxString>& fieldStore = m_dataStore[symbol.m_Uuid];

            for( const std::pair<wxString, wxString> srcData : fieldStore )
            {
                if( srcData.first == _( "Qty" ) )
                    continue;

                const wxString& srcName = srcData.first;
                const wxString& srcValue = srcData.second;
                SCH_FIELD*      destField = symbol.FindField( srcName );
                int             col = GetFieldNameCol( srcName );
                bool            userAdded = ( col != -1 && m_cols[col].m_userAdded );

                // Add a not existing field if it has a value for this symbol
                bool createField = !destField && ( !srcValue.IsEmpty() || userAdded );

                if( createField )
                {
                    const VECTOR2I symbolPos = symbol.GetPosition();
                    destField = symbol.AddField( SCH_FIELD( symbolPos, -1, &symbol, srcName ) );
                }

                if( !destField )
                    continue;

                if( destField->GetId() == REFERENCE_FIELD )
                {
                    // Reference is not editable from this dialog
                }
                else if( destField->GetId() == VALUE_FIELD )
                {
                    // Value field cannot be empty
                    if( !srcValue.IsEmpty() )
                        symbol.SetValueFieldText( srcValue );
                }
                else if( destField->GetId() == FOOTPRINT_FIELD )
                {
                    symbol.SetFootprintFieldText( srcValue );
                }
                else
                {
                    destField->SetText( srcValue );
                }
            }

            for( int ii = symbol.GetFields().size() - 1; ii >= MANDATORY_FIELDS; ii-- )
            {
                if( fieldStore.count( symbol.GetFields()[ii].GetName() ) == 0 )
                    symbol.GetFields().erase( symbol.GetFields().begin() + ii );
            }
        }

        m_edited = false;
    }

    int GetDataWidth( int aCol )
    {
        int width = 0;

        if( ColIsReference( aCol ) )
        {
            for( int row = 0; row < GetNumberRows(); ++row )
                width = std::max( width, KIUI::GetTextSize( GetValue( row, aCol ), GetView() ).x );
        }
        else
        {
            wxString fieldName = GetColFieldName( aCol ); // symbol fieldName or Qty string

            for( unsigned symbolRef = 0; symbolRef < m_symbolsList.GetCount(); ++ symbolRef )
            {
                const KIID& symbolID = m_symbolsList[ symbolRef ].GetSymbol()->m_Uuid;
                wxString    text = m_dataStore[symbolID][fieldName];

                width = std::max( width, KIUI::GetTextSize( text, GetView() ).x );
            }
        }

        return width;
    }


    bool IsEdited()
    {
        return m_edited;
    }
};


DIALOG_SYMBOL_FIELDS_TABLE::DIALOG_SYMBOL_FIELDS_TABLE( SCH_EDIT_FRAME* parent ) :
        DIALOG_SYMBOL_FIELDS_TABLE_BASE( parent ), m_currentBomPreset( nullptr ),
        m_lastSelectedBomPreset( nullptr ), m_parent( parent ),
        m_schSettings( parent->Schematic().Settings() )
{
    wxSize defaultDlgSize = ConvertDialogToPixels( wxSize( 600, 300 ) );
    int    nameColWidthMargin = 44;

    // Get all symbols from the list of schematic sheets
    m_parent->Schematic().GetSheets().GetSymbols( m_symbolsList, false );

    m_separator1->SetIsSeparator();
    m_separator2->SetIsSeparator();
    m_bRefresh->SetBitmap( KiBitmap( BITMAPS::small_refresh ) );

    m_fieldsCtrl->AppendTextColumn( _( "Field" ), wxDATAVIEW_CELL_INERT, 0, wxALIGN_LEFT, 0 );
    m_fieldsCtrl->AppendTextColumn( _( "Label" ), wxDATAVIEW_CELL_EDITABLE, 0, wxALIGN_LEFT, 0 );
    m_fieldsCtrl->AppendToggleColumn( _( "Show" ), wxDATAVIEW_CELL_ACTIVATABLE, 0,
                                      wxALIGN_CENTER, 0 );
    m_fieldsCtrl->AppendToggleColumn( _( "Group By" ), wxDATAVIEW_CELL_ACTIVATABLE, 0,
                                      wxALIGN_CENTER, 0 );

    // GTK asserts if the number of columns doesn't match the data, but we still don't want
    // to display the canonical names.  So we'll insert a column for them, but keep it 0 width.
    m_fieldsCtrl->AppendTextColumn( _( "Name" ), wxDATAVIEW_CELL_INERT, 0, wxALIGN_LEFT, 0 );

    // SetWidth( wxCOL_WIDTH_AUTOSIZE ) fails here on GTK, so we calculate the title sizes and
    // set the column widths ourselves.
    wxDataViewColumn* column = m_fieldsCtrl->GetColumn( SHOW_FIELD_COLUMN );
    m_showColWidth = KIUI::GetTextSize( column->GetTitle(), m_fieldsCtrl ).x + COLUMN_MARGIN;
    column->SetMinWidth( m_showColWidth );

    column = m_fieldsCtrl->GetColumn( GROUP_BY_COLUMN );
    m_groupByColWidth = KIUI::GetTextSize( column->GetTitle(), m_fieldsCtrl ).x + COLUMN_MARGIN;
    column->SetMinWidth( m_groupByColWidth );

    // The fact that we're a list should keep the control from reserving space for the
    // expander buttons... but it doesn't.  Fix by forcing the indent to 0.
    m_fieldsCtrl->SetIndent( 0 );

    m_filter->SetDescriptiveText( _( "Filter" ) );
    m_dataModel = new FIELDS_EDITOR_GRID_DATA_MODEL( m_parent, m_symbolsList );

    LoadFieldNames();   // loads rows into m_fieldsCtrl and columns into m_dataModel

    // Now that the fields are loaded we can set the initial location of the splitter
    // based on the list width.  Again, SetWidth( wxCOL_WIDTH_AUTOSIZE ) fails us on GTK.
    m_fieldNameColWidth = 0;
    m_labelColWidth = 0;

    for( int row = 0; row < m_fieldsCtrl->GetItemCount(); ++row )
    {
        const wxString& displayName = m_fieldsCtrl->GetTextValue( row, DISPLAY_NAME_COLUMN );
        m_fieldNameColWidth =
                std::max( m_fieldNameColWidth, KIUI::GetTextSize( displayName, m_fieldsCtrl ).x );

        const wxString& label = m_fieldsCtrl->GetTextValue( row, LABEL_COLUMN );
        m_labelColWidth = std::max( m_labelColWidth, KIUI::GetTextSize( label, m_fieldsCtrl ).x );
    }

    m_fieldNameColWidth += nameColWidthMargin;
    m_labelColWidth += nameColWidthMargin;

    int fieldsMinWidth = m_fieldNameColWidth + m_labelColWidth + m_groupByColWidth + m_showColWidth;

    m_fieldsCtrl->GetColumn( DISPLAY_NAME_COLUMN )->SetWidth( m_fieldNameColWidth );
    m_fieldsCtrl->GetColumn( LABEL_COLUMN )->SetWidth( m_labelColWidth );

    // This is used for data only.  Don't show it to the user.
    m_fieldsCtrl->GetColumn( FIELD_NAME_COLUMN )->SetHidden( true );

    m_splitterMainWindow->SetMinimumPaneSize( fieldsMinWidth );
    m_splitterMainWindow->SetSashPosition( fieldsMinWidth + 40 );

    m_cbBomPresets->SetToolTip( wxString::Format(
            _( "Save and restore layer visibility combinations.\n"
               "Use %s+Tab to activate selector.\n"
               "Successive Tabs while holding %s down will "
               "cycle through presets in the popup." ),
            KeyNameFromKeyCode( PRESET_SWITCH_KEY ), KeyNameFromKeyCode( PRESET_SWITCH_KEY ) ) );

    m_dataModel->RebuildRows( m_filter, m_groupSymbolsBox, m_fieldsCtrl );

    m_grid->UseNativeColHeader( true );
    m_grid->SetTable( m_dataModel, true );

    // must be done after SetTable(), which appears to re-set it
    m_grid->SetSelectionMode( wxGrid::wxGridSelectCells );

    // add Cut, Copy, and Paste to wxGrid
    m_grid->PushEventHandler( new FIELDS_EDITOR_GRID_TRICKS( this, m_grid, m_fieldsCtrl ) );

    // give a bit more room for comboboxes
    m_grid->SetDefaultRowSize( m_grid->GetDefaultRowSize() + 4 );

    // Load our BOM view presets
    SetUserBomPresets( m_schSettings.m_BomPresets );
    ApplyBomPreset( m_schSettings.m_BomSettings );
    syncBomPresetSelection();

    SetupColumnProperties();

    m_grid->SelectRow( 0 );
    m_grid->SetGridCursor( 0, 1 );
    SetInitialFocus( m_grid );

    SetupStandardButtons();

    finishDialogSettings();
    SetSize( defaultDlgSize );
    Center();

    // Connect Events
    m_grid->Connect( wxEVT_GRID_COL_SORT,
                     wxGridEventHandler( DIALOG_SYMBOL_FIELDS_TABLE::OnColSort ), nullptr, this );
    m_grid->Connect( wxEVT_GRID_COL_MOVE,
                     wxGridEventHandler( DIALOG_SYMBOL_FIELDS_TABLE::OnColMove ), nullptr, this );
    m_cbBomPresets->Bind( wxEVT_CHOICE, &DIALOG_SYMBOL_FIELDS_TABLE::onBomPresetChanged, this );
    m_fieldsCtrl->Bind( wxEVT_DATAVIEW_ITEM_VALUE_CHANGED,
                        &DIALOG_SYMBOL_FIELDS_TABLE::OnColLabelChange, this );
}


void DIALOG_SYMBOL_FIELDS_TABLE::SetupColumnProperties()
{
    wxSize defaultDlgSize = ConvertDialogToPixels( wxSize( 600, 300 ) );

    // Restore column sorting order and widths
    m_grid->AutoSizeColumns( false );
    int  sortCol = 0;
    bool sortAscending = true;


    for( int col = 0; col < m_grid->GetNumberCols(); ++col )
    {
        wxGridCellAttr* attr = new wxGridCellAttr;
        attr->SetReadOnly( false );

        // Set some column types to specific editors
        if( m_dataModel->GetColFieldName( col ) == _( "Reference" ) )
        {
            attr->SetReadOnly();
            m_grid->SetColAttr( col, attr );
        }
        else if( m_dataModel->GetColFieldName( col ) == _( "Footprint" ) )
        {
            attr->SetEditor( new GRID_CELL_FPID_EDITOR( this, wxEmptyString ) );
            m_grid->SetColAttr( col, attr );
        }
        else if( m_dataModel->GetColFieldName( col ) == _( "Datasheet" ) )
        {
            // set datasheet column viewer button
            attr->SetEditor( new GRID_CELL_URL_EDITOR( this, Prj().SchSearchS() ) );
            m_grid->SetColAttr( col, attr );
        }
        else if( m_dataModel->GetColFieldName( col ) == _( "Qty" ) )
        {
            attr->SetReadOnly();
            m_grid->SetColAttr( col, attr );
            m_grid->SetColFormatNumber( col );
        }
        else
        {
            attr->SetEditor( m_grid->GetDefaultEditor() );
            m_grid->SetColAttr( col, attr );
            m_grid->SetColFormatCustom( col, wxGRID_VALUE_STRING );
        }

        // Columns are hidden by setting their width to 0 so if we resize them they will
        // become unhidden.
        if( m_grid->IsColShown( col ) )
        {
            std::string key( m_dataModel->GetColFieldName( col ).ToUTF8() );

            if( m_schSettings.m_BomSettings.column_widths.count( key ) )
            {
                int width = m_schSettings.m_BomSettings.column_widths.at( key );
                m_grid->SetColSize( col, width );
            }
            else
            {
                int textWidth = m_dataModel->GetDataWidth( col ) + COLUMN_MARGIN;
                int maxWidth = defaultDlgSize.x / 3;

                if( col == m_grid->GetNumberCols() - 1 )
                    m_grid->SetColSize( col, Clamp( 50, textWidth, maxWidth ) );
                else
                    m_grid->SetColSize( col, Clamp( 100, textWidth, maxWidth ) );
            }

            if( m_schSettings.m_BomSettings.column_sorts.count( key ) )
            {
                sortCol = col;
                sortAscending = m_schSettings.m_BomSettings.column_sorts[key];
            }
        }
    }

    // sync m_grid's column visibilities to Show checkboxes in m_fieldsCtrl
    for( int i = 0; i < m_fieldsCtrl->GetItemCount(); ++i )
    {
        int col = m_dataModel->GetFieldNameCol( m_fieldsCtrl->GetTextValue( i, FIELD_NAME_COLUMN ) );

        if( col == -1 )
            continue;

        if( m_fieldsCtrl->GetToggleValue( i, SHOW_FIELD_COLUMN ) )
            m_grid->ShowCol( col );
        else
            m_grid->HideCol( col );
    }

    m_dataModel->Sort( sortCol, sortAscending );
    m_grid->SetSortingColumn( sortCol, sortAscending );
}


DIALOG_SYMBOL_FIELDS_TABLE::~DIALOG_SYMBOL_FIELDS_TABLE()
{
    // Disconnect Events
    m_grid->Disconnect( wxEVT_GRID_COL_SORT,
                        wxGridEventHandler( DIALOG_SYMBOL_FIELDS_TABLE::OnColSort ), nullptr,
                        this );
    m_grid->Disconnect( wxEVT_GRID_COL_SORT,
                        wxGridEventHandler( DIALOG_SYMBOL_FIELDS_TABLE::OnColMove ), nullptr,
                        this );

    // Delete the GRID_TRICKS.
    m_grid->PopEventHandler( true );

    // we gave ownership of m_dataModel to the wxGrid...
}


bool DIALOG_SYMBOL_FIELDS_TABLE::TransferDataToWindow()
{
    if( !wxDialog::TransferDataFromWindow() )
        return false;

    TOOL_MANAGER*      toolMgr = m_parent->GetToolManager();
    EE_SELECTION_TOOL* selectionTool = toolMgr->GetTool<EE_SELECTION_TOOL>();
    EE_SELECTION&      selection = selectionTool->GetSelection();
    SCH_SYMBOL*        symbol = nullptr;

    if( selection.GetSize() == 1 )
    {
        EDA_ITEM*      item = selection.Front();

        if( item->Type() == SCH_SYMBOL_T )
            symbol = (SCH_SYMBOL*) item;
        else if( item->GetParent() && item->GetParent()->Type() == SCH_SYMBOL_T )
            symbol = (SCH_SYMBOL*) item->GetParent();
    }

    if( symbol )
    {
        for( int row = 0; row < m_dataModel->GetNumberRows(); ++row )
        {
            std::vector<SCH_REFERENCE> references = m_dataModel->GetRowReferences( row );
            bool                       found = false;

            for( const SCH_REFERENCE& ref : references )
            {
                if( ref.GetSymbol() == symbol )
                {
                    found = true;
                    break;
                }
            }

            if( found )
            {
                m_grid->GoToCell( row, 1 );
                break;
            }
        }
    }

    return true;
}


bool DIALOG_SYMBOL_FIELDS_TABLE::TransferDataFromWindow()
{
    if( !m_grid->CommitPendingChanges() )
        return false;

    if( !wxDialog::TransferDataFromWindow() )
        return false;

    SCH_SHEET_PATH currentSheet = m_parent->GetCurrentSheet();

    m_dataModel->ApplyData();

    // Reset the view to where we left the user
    m_parent->SetCurrentSheet( currentSheet );
    m_parent->SyncView();
    m_parent->Refresh();

    m_parent->OnModify();

    return true;
}


void DIALOG_SYMBOL_FIELDS_TABLE::AddField( const wxString& aFieldName,
                                           const wxString& aLabelValue,
                                           bool defaultShow, bool defaultSortBy, bool addedByUser )
{
    m_dataModel->AddColumn( aFieldName, aLabelValue, addedByUser );

    wxVector<wxVariant> fieldsCtrlRow;
    bool                show    = defaultShow;
    bool                sort_by = defaultSortBy;

    std::string key( aFieldName.ToUTF8() );

    if( m_schSettings.m_BomSettings.fields_show.count( key ) )
        show = m_schSettings.m_BomSettings.fields_show.at( key );

    if( m_schSettings.m_BomSettings.fields_group_by.count( key ) )
        sort_by = m_schSettings.m_BomSettings.fields_group_by.at( key );

    // Don't change these to emplace_back: some versions of wxWidgets don't support it
    fieldsCtrlRow.push_back( wxVariant( aFieldName ) );
    fieldsCtrlRow.push_back( wxVariant( aLabelValue ) );
    fieldsCtrlRow.push_back( wxVariant( show ) );
    fieldsCtrlRow.push_back( wxVariant( sort_by ) );
    fieldsCtrlRow.push_back( wxVariant( aFieldName ) );

    m_fieldsCtrl->AppendItem( fieldsCtrlRow );
}


void DIALOG_SYMBOL_FIELDS_TABLE::LoadFieldNames()
{
    std::set<wxString> userFieldNames;

    for( unsigned i = 0; i < m_symbolsList.GetCount(); ++i )
    {
        SCH_SYMBOL* symbol = m_symbolsList[ i ].GetSymbol();

        for( int j = MANDATORY_FIELDS; j < symbol->GetFieldCount(); ++j )
            userFieldNames.insert( symbol->GetFields()[j].GetName() );
    }


    AddField( _( "Reference" ), wxT( "Reference" ), true, true  );
    AddField( _( "Value" ),     wxT( "Value" ),     true, true  );
    AddField( _( "Footprint" ), wxT( "Footprint" ), true, true  );
    AddField( _( "Datasheet" ), wxT( "Datasheet" ), true, false );
    AddField( _( "Quantity" ),  wxT( "Qty" ),  true, false );

    for( const wxString& fieldName : userFieldNames )
        AddField( fieldName, fieldName, true, false );

    // Add any templateFieldNames which aren't already present in the userFieldNames
    for( const TEMPLATE_FIELDNAME& templateFieldname :
         m_schSettings.m_TemplateFieldNames.GetTemplateFieldNames() )
    {
        if( userFieldNames.count( templateFieldname.m_Name ) == 0 )
            AddField( templateFieldname.m_Name, templateFieldname.m_Name, false, false );
    }

    m_dataModel->SetFieldsOrder( m_schSettings.m_BomSettings.column_order );
}


void DIALOG_SYMBOL_FIELDS_TABLE::OnAddField( wxCommandEvent& event )
{
    wxTextEntryDialog dlg( this, _( "New field name:" ), _( "Add Field" ) );

    if( dlg.ShowModal() != wxID_OK )
        return;

    wxString fieldName = dlg.GetValue();

    if( fieldName.IsEmpty() )
    {
        DisplayError( this, _( "Field must have a name." ) );
        return;
    }

    for( int i = 0; i < m_dataModel->GetNumberCols(); ++i )
    {
        if( fieldName == m_dataModel->GetColFieldName( i ) )
        {
            DisplayError( this, wxString::Format( _( "Field name '%s' already in use." ),
                                                  fieldName ) );
            return;
        }
    }

    std::string key( fieldName.ToUTF8() );

    m_parent->Schematic().Settings().m_BomSettings.fields_show[key] = true;
    AddField( fieldName, fieldName, true, false, true );

    wxGridTableMessage msg( m_dataModel, wxGRIDTABLE_NOTIFY_COLS_APPENDED, 1 );
    m_grid->ProcessTableMessage( msg );

    wxGridCellAttr* attr = new wxGridCellAttr;
    m_grid->SetColAttr( m_dataModel->GetColsCount() - 1, attr );
    m_grid->SetColFormatCustom( m_dataModel->GetColsCount() - 1, wxGRID_VALUE_STRING );

    syncBomPresetSelection();
}


void DIALOG_SYMBOL_FIELDS_TABLE::OnRemoveField( wxCommandEvent& event )
{
    int col = -1;
    int row = m_fieldsCtrl->GetSelectedRow();

   // Should never occur: "Remove Field..." button should be disabled if invalid selection
   // via OnFieldsCtrlSelectionChanged()
    wxCHECK_RET( row != -1, wxS( "Some user defined field must be selected first" ) );
    wxCHECK_RET( row >= MANDATORY_FIELDS, wxS( "Mandatory fields cannot be removed" ) );

    wxString fieldName = m_fieldsCtrl->GetTextValue( row, FIELD_NAME_COLUMN );
    wxString displayName = m_fieldsCtrl->GetTextValue( row, DISPLAY_NAME_COLUMN );

    wxString confirm_msg =
            wxString::Format( _( "Are you sure you want to remove the field '%s'?" ), displayName );

    if( !IsOK( this, confirm_msg ) )
        return;

    for( int i = 0; i < m_dataModel->GetNumberCols(); ++i )
    {
        if( fieldName == m_dataModel->GetColFieldName( i ) )
            col = i;
    }

    m_fieldsCtrl->DeleteItem( row );
    m_dataModel->RemoveColumn( col );

    // Make selection and update the state of "Remove field..." button via OnFieldsCtrlSelectionChanged()
    // Safe to decrement row index because we always have mandatory fields
    m_fieldsCtrl->SelectRow( --row );

    if( row < MANDATORY_FIELDS )
    {
         m_removeFieldButton->Enable( false );
         m_renameFieldButton->Enable( false );
    }

    wxGridTableMessage msg( m_dataModel, wxGRIDTABLE_NOTIFY_COLS_DELETED,
                                m_fieldsCtrl->GetItemCount(), 1 );

    m_grid->ProcessTableMessage( msg );

    // set up attributes on the new quantities column
    wxGridCellAttr* attr = new wxGridCellAttr;
    attr->SetReadOnly();

    m_grid->SetColAttr( m_dataModel->GetColsCount() - 1, attr );
    m_grid->SetColFormatNumber( m_dataModel->GetColsCount() - 1 );
    m_grid->SetColSize( m_dataModel->GetColsCount() - 1, 50 );

    syncBomPresetSelection();
}


void DIALOG_SYMBOL_FIELDS_TABLE::OnRenameField( wxCommandEvent& event )
{
    int col = -1;
    int row = m_fieldsCtrl->GetSelectedRow();

    // Should never occur: "Rename Field..." button should be disabled if invalid selection
    // via OnFieldsCtrlSelectionChanged()
    wxCHECK_RET( row != -1, wxS( "Some user defined field must be selected first" ) );
    wxCHECK_RET( row >= MANDATORY_FIELDS, wxS( "Mandatory fields cannot be renamed" ) );

    wxString fieldName = m_fieldsCtrl->GetTextValue( row, 0 );


    wxTextEntryDialog dlg( this, _( "New field name:" ), _( "Rename Field" ) );

    if( dlg.ShowModal() != wxID_OK )
         return;

    wxString newFieldName = dlg.GetValue();

    if( fieldName.IsEmpty() )
    {
         DisplayError( this, _( "Field must have a name." ) );
         return;
    }

    for( int i = 0; i < m_dataModel->GetNumberCols(); ++i )
    {
         if( fieldName == m_dataModel->GetColFieldName( i ) )
         {
            if( col == -1 )
            {
                col = i;
            }
            else
            {
                wxString confirm_msg = wxString::Format(
                        _( "Field name %s already exists. Cannot rename over existing field." ),
                        fieldName );
                DisplayError( this, confirm_msg );
                return;
            }
         }
    }


    m_dataModel->RenameColumn( col, newFieldName );
    m_fieldsCtrl->SetTextValue( newFieldName, col, 0 );

    std::string oldKey( fieldName.ToUTF8() );
    std::string newKey( newFieldName.ToUTF8() );

    //In-place rename map key
    auto node = m_schSettings.m_BomSettings.fields_show.extract( oldKey );
    node.key() = newKey;
    m_schSettings.m_BomSettings.fields_show.insert( std::move( node ) );

    syncBomPresetSelection();
}


void DIALOG_SYMBOL_FIELDS_TABLE::OnFilterText( wxCommandEvent& aEvent )
{
    m_schSettings.m_BomSettings.filter_string = m_filter->GetValue();
    m_dataModel->RebuildRows( m_filter, m_groupSymbolsBox, m_fieldsCtrl );
    m_dataModel->Sort( m_grid->GetSortingColumn(), m_grid->IsSortOrderAscending() );
    m_grid->ForceRefresh();

    syncBomPresetSelection();
}


void DIALOG_SYMBOL_FIELDS_TABLE::OnFilterMouseMoved( wxMouseEvent& aEvent )
{
    wxPoint pos = aEvent.GetPosition();
    wxRect  ctrlRect = m_filter->GetScreenRect();
    int     buttonWidth = ctrlRect.GetHeight();         // Presume buttons are square

    if( m_filter->IsSearchButtonVisible() && pos.x < buttonWidth )
        SetCursor( wxCURSOR_ARROW );
    else if( m_filter->IsCancelButtonVisible() && pos.x > ctrlRect.GetWidth() - buttonWidth )
        SetCursor( wxCURSOR_ARROW );
    else
        SetCursor( wxCURSOR_IBEAM );
}

void DIALOG_SYMBOL_FIELDS_TABLE::OnFieldsCtrlSelectionChanged( wxDataViewEvent& event )
{
    int row = m_fieldsCtrl->GetSelectedRow();

    if( row >= MANDATORY_FIELDS )
    {
        m_removeFieldButton->Enable( true );
        m_renameFieldButton->Enable( true );
    }
    else
    {
        m_removeFieldButton->Enable( false );
        m_renameFieldButton->Enable( false );
    }
}

void DIALOG_SYMBOL_FIELDS_TABLE::OnColumnItemToggled( wxDataViewEvent& event )
{
    wxDataViewItem item = event.GetItem();
    int            row = m_fieldsCtrl->ItemToRow( item );
    int            col = event.GetColumn();

    switch ( col )
    {
    case SHOW_FIELD_COLUMN:
    {
        bool value = m_fieldsCtrl->GetToggleValue( row, col );

        std::string fieldName( m_fieldsCtrl->GetTextValue( row, FIELD_NAME_COLUMN ).ToUTF8() );
        m_schSettings.m_BomSettings.fields_show[fieldName] = value;

        int dataCol = m_dataModel->GetFieldNameCol( fieldName );

        if( dataCol != -1 )
        {
            if( value )
                m_grid->ShowCol( dataCol );
            else
                m_grid->HideCol( dataCol );
        }

        break;
    }

    case GROUP_BY_COLUMN:
    {
        bool value = m_fieldsCtrl->GetToggleValue( row, col );

        if( m_dataModel->ColIsQuantity( row ) && value )
        {
            DisplayError( this, _( "The Quantity column cannot be grouped by." ) );

            value = false;
            m_fieldsCtrl->SetToggleValue( value, row, col );
        }

        std::string fieldName( m_fieldsCtrl->GetTextValue( row, FIELD_NAME_COLUMN ).ToUTF8() );
        m_schSettings.m_BomSettings.fields_group_by[fieldName] = value;

        m_dataModel->RebuildRows( m_filter, m_groupSymbolsBox, m_fieldsCtrl );
        m_dataModel->Sort( m_grid->GetSortingColumn(), m_grid->IsSortOrderAscending() );
        m_grid->ForceRefresh();
        break;
    }

    default:
        break;
    }

    syncBomPresetSelection();
}


void DIALOG_SYMBOL_FIELDS_TABLE::OnGroupSymbolsToggled( wxCommandEvent& event )
{
    m_schSettings.m_BomSettings.group_symbols = m_groupSymbolsBox->GetValue();
    m_dataModel->RebuildRows( m_filter, m_groupSymbolsBox, m_fieldsCtrl );
    m_dataModel->Sort( m_grid->GetSortingColumn(), m_grid->IsSortOrderAscending() );
    m_grid->ForceRefresh();

    syncBomPresetSelection();
}


void DIALOG_SYMBOL_FIELDS_TABLE::OnColSort( wxGridEvent& aEvent )
{
    int         sortCol = aEvent.GetCol();
    std::string key( m_dataModel->GetColFieldName( sortCol ).ToUTF8() );
    bool        ascending;

    // This is bonkers, but wxWidgets doesn't tell us ascending/descending in the event, and
    // if we ask it will give us pre-event info.
    if( m_grid->IsSortingBy( sortCol ) )
    {
        // same column; invert ascending
        ascending = !m_grid->IsSortOrderAscending();
    }
    else
    {
        // different column; start with ascending
        ascending = true;
    }

    // We only support sorting on one column at this time
    m_schSettings.m_BomSettings.column_sorts.clear();
    m_schSettings.m_BomSettings.column_sorts[key] = ascending;

    m_dataModel->Sort( sortCol, ascending );
    m_grid->ForceRefresh();

    syncBomPresetSelection();
}


void DIALOG_SYMBOL_FIELDS_TABLE::OnColMove( wxGridEvent& aEvent )
{
    int origPos = aEvent.GetCol();

    CallAfter(
            [origPos, this]()
            {
                int newPos = m_grid->GetColPos( origPos );

                m_dataModel->MoveColumn( origPos, newPos );

                m_schSettings.m_BomSettings.column_order = m_dataModel->GetFieldsOrder();

                // "Unmove" the column since we've moved the column internally
                m_grid->ResetColPos();

                // We need to reset all the column attr's to the correct column order
                SetupColumnProperties();

                m_grid->ForceRefresh();
            } );

    syncBomPresetSelection();
}


void DIALOG_SYMBOL_FIELDS_TABLE::OnColLabelChange( wxDataViewEvent& aEvent )
{
    wxDataViewItem item = aEvent.GetItem();
    int            row = m_fieldsCtrl->ItemToRow( item );
    wxString       label = m_fieldsCtrl->GetTextValue( row, LABEL_COLUMN );
    wxString       fieldName = m_fieldsCtrl->GetTextValue( row, FIELD_NAME_COLUMN );
    int            col = m_dataModel->GetFieldNameCol( fieldName );

    if( col != -1 )
        m_dataModel->SetColLabelValue( col, label );

    syncBomPresetSelection();

    aEvent.Skip();

    m_grid->ForceRefresh();
}

void DIALOG_SYMBOL_FIELDS_TABLE::OnTableValueChanged( wxGridEvent& aEvent )
{
    m_grid->ForceRefresh();
}


void DIALOG_SYMBOL_FIELDS_TABLE::OnTableColSize( wxGridSizeEvent& aEvent )
{
    int         col = aEvent.GetRowOrCol();
    std::string key( m_dataModel->GetColFieldName( col ).ToUTF8() );

    if( m_grid->GetColSize( col ) )
        m_schSettings.m_BomSettings.column_widths[key] = m_grid->GetColSize( col );

    aEvent.Skip();

    m_grid->ForceRefresh();
}


void DIALOG_SYMBOL_FIELDS_TABLE::OnRegroupSymbols( wxCommandEvent& aEvent )
{
    m_dataModel->RebuildRows( m_filter, m_groupSymbolsBox, m_fieldsCtrl );
    m_dataModel->Sort( m_grid->GetSortingColumn(), m_grid->IsSortOrderAscending() );
    m_grid->ForceRefresh();
}


void DIALOG_SYMBOL_FIELDS_TABLE::OnTableCellClick( wxGridEvent& event )
{
    if( m_dataModel->ColIsReference( event.GetCol() ) )
    {
        m_grid->ClearSelection();
        m_grid->SetGridCursor( event.GetRow(), event.GetCol() );

        m_dataModel->ExpandCollapseRow( event.GetRow() );
    }
    else
    {
        event.Skip();
    }
}

void DIALOG_SYMBOL_FIELDS_TABLE::OnTableRangeSelected( wxGridRangeSelectEvent& event )
{
    wxGridCellCoordsArray selectedCells = m_grid->GetSelectedCells();

    if( selectedCells.GetCount() == 1 )
    {
        int row = selectedCells[0].GetRow();
        int flag = m_dataModel->GetRowFlags( row );
        std::vector<SCH_REFERENCE> refs = m_dataModel->GetRowReferences( row );

        // Focus Eeschema view on the symbol selected in the dialog
        // TODO: Highlight or select more than one unit
        if( ( flag == GROUP_SINGLETON || flag == CHILD_ITEM ) && refs.size() >= 1 )
        {
            SCH_EDITOR_CONTROL* editor = m_parent->GetToolManager()->GetTool<SCH_EDITOR_CONTROL>();

            std::sort( refs.begin(), refs.end(),
                    []( const SCH_REFERENCE& a, const SCH_REFERENCE& b )
                    {
                        return a.GetUnit() < b.GetUnit();
                    } );

            // search and highlight the symbol found by its full path.
            // It allows select of not yet annotated or duplicaded symbols
            wxString symbol_path = refs[0].GetFullPath();
            // wxString reference = refs[0].GetRef() + refs[0].GetRefNumber();  // Not used
            editor->FindSymbolAndItem( &symbol_path, nullptr, true, HIGHLIGHT_SYMBOL, wxEmptyString );
        }

        return;
    }

    event.Skip();
}

void DIALOG_SYMBOL_FIELDS_TABLE::OnTableItemContextMenu( wxGridEvent& event )
{
    // TODO: Option to select footprint if FOOTPRINT column selected

    event.Skip();
}


void DIALOG_SYMBOL_FIELDS_TABLE::OnSizeFieldList( wxSizeEvent& event )
{
    m_labelColWidth = KIPLATFORM::UI::GetUnobscuredSize( m_fieldsCtrl ).x;
    m_labelColWidth -= m_fieldNameColWidth + m_showColWidth + m_groupByColWidth;
#ifdef __WXMAC__
    // TODO: something in wxWidgets 3.1.x pads checkbox columns with extra space.  (It used to
    // also be that the width of the column would get set too wide (to 30), but that's patched in
    // our local wxWidgets fork.)
    m_labelColWidth -= 30;
#endif

    // GTK loses its head and messes these up when resizing the splitter bar:
    m_fieldsCtrl->GetColumn( SHOW_FIELD_COLUMN )->SetWidth( m_showColWidth );
    m_fieldsCtrl->GetColumn( GROUP_BY_COLUMN )->SetWidth( m_groupByColWidth );

    m_fieldsCtrl->GetColumn( FIELD_NAME_COLUMN )->SetHidden( true );
    m_fieldsCtrl->GetColumn( DISPLAY_NAME_COLUMN )->SetWidth( m_fieldNameColWidth );
    m_fieldsCtrl->GetColumn( LABEL_COLUMN )->SetWidth( m_labelColWidth );

    m_fieldsCtrl->Refresh(); // To refresh checkboxes on Windows.

    event.Skip();
}


void DIALOG_SYMBOL_FIELDS_TABLE::OnSaveAndContinue( wxCommandEvent& aEvent )
{
    if( TransferDataFromWindow() )
        m_parent->SaveProject();
}


void DIALOG_SYMBOL_FIELDS_TABLE::OnExport( wxCommandEvent& aEvent )
{
    int last_col = m_grid->GetNumberCols() - 1;

    if( m_dataModel->IsEdited() )
        if( OKOrCancelDialog( nullptr, _( "Unsaved data" ),
                              _( "Changes are unsaved. Export unsaved data?" ), "", _( "OK" ),
                              _( "Cancel" ) )
            == wxID_CANCEL )
            return;


    // Calculate the netlist filename
    wxFileName fn = m_parent->Schematic().GetFileName();
    fn.SetExt( CsvFileExtension );

    wxFileDialog saveDlg( this, _( "Save as CSV" ), wxPathOnly( Prj().GetProjectFullName() ),
                          fn.GetFullName(), CsvFileWildcard(), wxFD_SAVE | wxFD_OVERWRITE_PROMPT );

    if( saveDlg.ShowModal() == wxID_CANCEL )
        return;

    wxFFile out( saveDlg.GetPath(), "wb" );

    if( !out.IsOpened() )
        return;

    // Find the location for the line terminator
    for( int col = m_grid->GetNumberCols() - 1; col >=0 ; --col )
    {
        if( m_grid->IsColShown( col ) )
        {
            last_col = col;
            break;
        }
    }

    // Column names
    for( int col = 0; col < m_grid->GetNumberCols(); col++ )
    {
        if( !m_grid->IsColShown( col ) )
            continue;

        wxString escapedValue = m_grid->GetColLabelValue( col );
        escapedValue.Replace( wxS( "\"" ), wxS( "\"\"" ) );

        wxString format = col == last_col ? wxS( "\"%s\"\r\n" ) : wxS( "\"%s\"," );

        out.Write( wxString::Format( format, escapedValue ) );
    }

    // Data rows
    for( int row = 0; row < m_grid->GetNumberRows(); row++ )
    {
        // Don't output child rows
        if( m_dataModel->GetRowFlags( row ) == CHILD_ITEM )
            continue;

        for( int col = 0; col < m_grid->GetNumberCols(); col++ )
        {
            if( !m_grid->IsColShown( col ) )
                continue;

            // Get the unanottated version of the field, e.g. no ">   " or "v   " by
            wxString escapedValue = m_dataModel->GetRawValue( row, col );
            escapedValue.Replace( wxS( "\"" ), wxS( "\"\"" ) );

            wxString format = col == last_col ? wxS( "\"%s\"\r\n" ) : wxS( "\"%s\"," );

            out.Write( wxString::Format( format, escapedValue ) );
        }
    }
}


void DIALOG_SYMBOL_FIELDS_TABLE::OnCancel( wxCommandEvent& event )
{
    Close();
}


void DIALOG_SYMBOL_FIELDS_TABLE::OnClose( wxCloseEvent& event )
{
    // This is a cancel, so commit quietly as we're going to throw the results away anyway.
    m_grid->CommitPendingChanges( true );

    if( m_dataModel->IsEdited() )
    {
        if( !HandleUnsavedChanges( this, _( "Save changes?" ),
                                   [&]() -> bool
                                   {
                                       return TransferDataFromWindow();
                                   } ) )
        {
            event.Veto();
            return;
        }
    }

    event.Skip();
}


std::vector<BOM_PRESET> DIALOG_SYMBOL_FIELDS_TABLE::GetUserBomPresets() const
{
    std::vector<BOM_PRESET> ret;

    for( const std::pair<const wxString, BOM_PRESET>& pair : m_bomPresets )
    {
        if( !pair.second.readOnly )
            ret.emplace_back( pair.second );
    }

    return ret;
}


void DIALOG_SYMBOL_FIELDS_TABLE::SetUserBomPresets( std::vector<BOM_PRESET>& aPresetList )
{
    // Reset to defaults
    loadDefaultBomPresets();

    for( const BOM_PRESET& preset : aPresetList )
    {
        if( m_bomPresets.count( preset.name ) )
            continue;

        m_bomPresets[preset.name] = preset;

        m_bomPresetMRU.Add( preset.name );
    }

    rebuildBomPresetsWidget();
}


void DIALOG_SYMBOL_FIELDS_TABLE::ApplyBomPreset( const wxString& aPresetName )
{
    updateBomPresetSelection( aPresetName );

    wxCommandEvent dummy;
    onBomPresetChanged( dummy );
}


void DIALOG_SYMBOL_FIELDS_TABLE::ApplyBomPreset( const BOM_PRESET& aPreset )
{
    if( m_bomPresets.count( aPreset.name ) )
        m_currentBomPreset = &m_bomPresets[aPreset.name];
    else
        m_currentBomPreset = nullptr;

    m_lastSelectedBomPreset =
            ( m_currentBomPreset && !m_currentBomPreset->readOnly ) ? m_currentBomPreset : nullptr;

    updateBomPresetSelection( aPreset.name );
    doApplyBomPreset( aPreset );
}


void DIALOG_SYMBOL_FIELDS_TABLE::loadDefaultBomPresets()
{
    m_bomPresets.clear();
    m_bomPresetMRU.clear();

    // Load the read-only defaults
    for( const BOM_PRESET& preset : { bomPresetGroupedByValue, bomPresetGroupedByValueFootprint } )
    {
        m_bomPresets[preset.name] = preset;
        m_bomPresets[preset.name].readOnly = true;

        m_bomPresetMRU.Add( preset.name );
    }
}


void DIALOG_SYMBOL_FIELDS_TABLE::rebuildBomPresetsWidget()
{
    m_bomPresetsLabel->SetLabel(
            wxString::Format( _( "Presets (%s+Tab):" ), KeyNameFromKeyCode( PRESET_SWITCH_KEY ) ) );
    m_cbBomPresets->Clear();

    // Build the layers preset list.
    // By default, the presetAllLayers will be selected
    int idx = 0;
    int default_idx = 0;

    for( std::pair<const wxString, BOM_PRESET>& pair : m_bomPresets )
    {
        m_cbBomPresets->Append( wxGetTranslation( pair.first ),
                                static_cast<void*>( &pair.second ) );

        if( pair.first == bomPresetGroupedByValueFootprint.name )
            default_idx = idx;

        idx++;
    }

    m_cbBomPresets->Append( wxT( "---" ) );
    m_cbBomPresets->Append( _( "Save preset..." ) );
    m_cbBomPresets->Append( _( "Delete preset..." ) );

    // At least the built-in presets should always be present
    wxASSERT( !m_bomPresets.empty() );

    // Default preset: all Boms
    m_cbBomPresets->SetSelection( default_idx );
    m_currentBomPreset = static_cast<BOM_PRESET*>( m_cbBomPresets->GetClientData( default_idx ) );
}


void DIALOG_SYMBOL_FIELDS_TABLE::syncBomPresetSelection()
{
    BOM_PRESET& current = m_parent->Schematic().Settings().m_BomSettings;

    auto it = std::find_if( m_bomPresets.begin(), m_bomPresets.end(),
                            [&]( const std::pair<const wxString, BOM_PRESET>& aPair )
                            {
                                return ( aPair.second.fields_show == current.fields_show
                                         && aPair.second.fields_group_by == current.fields_group_by
                                         && aPair.second.column_sorts == current.column_sorts
                                         && aPair.second.column_order == current.column_order
                                         && aPair.second.filter_string == current.filter_string
                                         && aPair.second.group_symbols == current.group_symbols );
                            } );

    if( it != m_bomPresets.end() )
    {
        // Select the right m_cbBomPresets item.
        // but these items are translated if they are predefined items.
        bool     do_translate = it->second.readOnly;
        wxString text = do_translate ? wxGetTranslation( it->first ) : it->first;

        m_cbBomPresets->SetStringSelection( text );
    }
    else
    {
        m_cbBomPresets->SetSelection( m_cbBomPresets->GetCount() - 3 ); // separator
    }

    m_currentBomPreset = static_cast<BOM_PRESET*>(
            m_cbBomPresets->GetClientData( m_cbBomPresets->GetSelection() ) );
}


void DIALOG_SYMBOL_FIELDS_TABLE::updateBomPresetSelection( const wxString& aName )
{
    // look at m_userBomPresets to know if aName is a read only preset, or a user preset.
    // Read only presets have translated names in UI, so we have to use
    // a translated name in UI selection.
    // But for a user preset name we should search for aName (not translated)
    wxString ui_label = aName;

    for( std::pair<const wxString, BOM_PRESET>& pair : m_bomPresets )
    {
        if( pair.first != aName )
            continue;

        if( pair.second.readOnly == true )
            ui_label = wxGetTranslation( aName );

        break;
    }

    int idx = m_cbBomPresets->FindString( ui_label );

    if( idx >= 0 && m_cbBomPresets->GetSelection() != idx )
    {
        m_cbBomPresets->SetSelection( idx );
        m_currentBomPreset = static_cast<BOM_PRESET*>( m_cbBomPresets->GetClientData( idx ) );
    }
    else if( idx < 0 )
    {
        m_cbBomPresets->SetSelection( m_cbBomPresets->GetCount() - 3 ); // separator
    }
}


void DIALOG_SYMBOL_FIELDS_TABLE::onBomPresetChanged( wxCommandEvent& aEvent )
{
    int count = m_cbBomPresets->GetCount();
    int index = m_cbBomPresets->GetSelection();

    auto resetSelection =
            [&]()
            {
                if( m_currentBomPreset )
                    m_cbBomPresets->SetStringSelection( m_currentBomPreset->name );
                else
                    m_cbBomPresets->SetSelection( m_cbBomPresets->GetCount() - 3 );
            };

    if( index == count - 3 )
    {
        // Separator: reject the selection
        resetSelection();
        return;
    }
    else if( index == count - 2 )
    {
        // Save current state to new preset
        wxString name;

        if( m_lastSelectedBomPreset )
            name = m_lastSelectedBomPreset->name;

        wxTextEntryDialog dlg( this, _( "BOM preset name:" ), _( "Save BOM Preset" ), name );

        if( dlg.ShowModal() != wxID_OK )
        {
            resetSelection();
            return;
        }

        name = dlg.GetValue();
        bool exists = m_bomPresets.count( name );

        if( !exists )
        {
            m_bomPresets[name] = BOM_PRESET( name, m_schSettings.m_BomSettings.fields_show,
                                             m_schSettings.m_BomSettings.fields_group_by,
                                             m_schSettings.m_BomSettings.column_widths,
                                             m_schSettings.m_BomSettings.column_sorts,
                                             m_schSettings.m_BomSettings.column_order,
                                             m_schSettings.m_BomSettings.filter_string,
                                             m_schSettings.m_BomSettings.group_symbols );
        }

        BOM_PRESET* preset = &m_bomPresets[name];
        m_currentBomPreset = preset;

        if( !exists )
        {
            index = m_cbBomPresets->Insert( name, index - 1, static_cast<void*>( preset ) );
        }
        else
        {
            preset->fields_show = m_schSettings.m_BomSettings.fields_show;
            preset->fields_group_by = m_schSettings.m_BomSettings.fields_group_by;
            preset->column_widths = m_schSettings.m_BomSettings.column_widths;
            preset->column_sorts = m_schSettings.m_BomSettings.column_sorts;
            preset->column_order = m_schSettings.m_BomSettings.column_order;
            preset->filter_string = m_schSettings.m_BomSettings.filter_string;
            preset->group_symbols = m_schSettings.m_BomSettings.group_symbols;

            index = m_cbBomPresets->FindString( name );
            m_bomPresetMRU.Remove( name );
        }

        m_cbBomPresets->SetSelection( index );
        m_bomPresetMRU.Insert( name, 0 );

        return;
    }
    else if( index == count - 1 )
    {
        // Delete a preset
        wxArrayString              headers;
        std::vector<wxArrayString> items;

        headers.Add( _( "Presets" ) );

        for( std::pair<const wxString, BOM_PRESET>& pair : m_bomPresets )
        {
            if( !pair.second.readOnly )
            {
                wxArrayString item;
                item.Add( pair.first );
                items.emplace_back( item );
            }
        }

        EDA_LIST_DIALOG dlg( this, _( "Delete Preset" ), headers, items );
        dlg.SetListLabel( _( "Select preset:" ) );

        if( dlg.ShowModal() == wxID_OK )
        {
            wxString presetName = dlg.GetTextSelection();
            int      idx = m_cbBomPresets->FindString( presetName );

            if( idx != wxNOT_FOUND )
            {
                m_bomPresets.erase( presetName );

                m_cbBomPresets->Delete( idx );
                m_currentBomPreset = nullptr;

                m_bomPresetMRU.Remove( presetName );
            }
        }

        resetSelection();
        return;
    }

    BOM_PRESET* preset = static_cast<BOM_PRESET*>( m_cbBomPresets->GetClientData( index ) );
    m_currentBomPreset = preset;

    m_lastSelectedBomPreset = ( !preset || preset->readOnly ) ? nullptr : preset;

    if( preset )
    {
        doApplyBomPreset( *preset );
        syncBomPresetSelection();
        m_currentBomPreset = preset;

        if( !m_currentBomPreset->name.IsEmpty() )
        {
            m_bomPresetMRU.Remove( preset->name );
            m_bomPresetMRU.Insert( preset->name, 0 );
        }
    }
}


void DIALOG_SYMBOL_FIELDS_TABLE::doApplyBomPreset( const BOM_PRESET& aPreset )
{
    // Set a good default sort
    m_dataModel->Sort( m_dataModel->GetFieldNameCol( _( "Reference" ) ), false );

    for( int i = 0; i < m_fieldsCtrl->GetItemCount(); i++ )
    {
        const std::string fieldName( m_fieldsCtrl->GetTextValue( i, FIELD_NAME_COLUMN ).ToUTF8() );
        int               col = m_dataModel->GetFieldNameCol( fieldName );

        if( col == -1 )
            continue;

        bool show = aPreset.fields_show.count( fieldName ) && aPreset.fields_show.at( fieldName );
        bool groupBy = aPreset.fields_group_by.count( fieldName )
                       && aPreset.fields_group_by.at( fieldName );
        int width = aPreset.column_widths.count( fieldName ) ? aPreset.column_widths.at( fieldName )
                                                             : -1;

        m_fieldsCtrl->SetToggleValue( show, i, SHOW_FIELD_COLUMN );

        if( show )
            m_grid->ShowCol( col );
        else
            m_grid->HideCol( col );

        m_fieldsCtrl->SetToggleValue( groupBy, i, GROUP_BY_COLUMN );

        if( aPreset.column_sorts.count( fieldName ) )
            m_dataModel->Sort( col, false );

        if( width != -1 )
            m_grid->SetColSize( col, width );
    }

    m_dataModel->SetFieldsOrder( aPreset.column_order );
    SetupColumnProperties();

    m_filter->ChangeValue( aPreset.filter_string );
    m_groupSymbolsBox->SetValue( aPreset.group_symbols );

    m_schSettings.m_BomSettings.fields_show = aPreset.fields_show;
    m_schSettings.m_BomSettings.fields_group_by = aPreset.fields_group_by;
    m_schSettings.m_BomSettings.column_widths = aPreset.column_widths;
    m_schSettings.m_BomSettings.column_sorts = aPreset.column_sorts;
    m_schSettings.m_BomSettings.column_order = aPreset.column_order;
    m_schSettings.m_BomSettings.filter_string = aPreset.filter_string;
    m_schSettings.m_BomSettings.group_symbols = aPreset.group_symbols;
}


bool DIALOG_SYMBOL_FIELDS_TABLE::TryBefore( wxEvent& aEvent )
{
    static bool s_presetSwitcherShown = false;

    // wxWidgets generates no key events for the tab key when the ctrl key is held down.  One
    // way around this is to look at all events and inspect the keyboard state of the tab key.
    // However, this runs into issues on some linux VMs where querying the keyboard state is
    // very slow.  Fortunately we only use ctrl-tab on Mac, so we implement this lovely hack:
#ifdef __WXMAC__
    if( wxGetKeyState( WXK_TAB ) )
#else
    if( ( aEvent.GetEventType() == wxEVT_CHAR || aEvent.GetEventType() == wxEVT_CHAR_HOOK )
        && static_cast<wxKeyEvent&>( aEvent ).GetKeyCode() == WXK_TAB )
#endif
    {
        if( !s_presetSwitcherShown && wxGetKeyState( PRESET_SWITCH_KEY ) )
        {
            if( this->IsActive() )
            {
                if( m_bomPresetMRU.size() > 0 )
                {
                    EDA_VIEW_SWITCHER switcher( this, m_bomPresetMRU, PRESET_SWITCH_KEY );

                    s_presetSwitcherShown = true;
                    switcher.ShowModal();
                    s_presetSwitcherShown = false;

                    int idx = switcher.GetSelection();

                    if( idx >= 0 && idx < (int) m_bomPresetMRU.size() )
                        ApplyBomPreset( m_bomPresetMRU[idx] );

                    return true;
                }
            }
        }
    }

    return DIALOG_SYMBOL_FIELDS_TABLE_BASE::TryBefore( aEvent );
}
