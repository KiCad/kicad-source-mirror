/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2017 Oliver Walters
 * Copyright (C) 2017-2021 KiCad Developers, see AUTHORS.txt for contributors.
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
#include <eeschema_settings.h>
#include <general.h>
#include <grid_tricks.h>
#include <kicad_string.h>
#include <kiface_i.h>
#include <refdes_utils.h>
#include <sch_edit_frame.h>
#include <sch_reference_list.h>
#include <schematic.h>
#include <tools/sch_editor_control.h>
#include <widgets/grid_text_button_helpers.h>
#include <widgets/wx_grid.h>
#include <wx/grid.h>
#include <wx/msgdlg.h>
#include <wx/textdlg.h>

#include "dialog_fields_editor_global.h"


#define DISPLAY_NAME_COLUMN   0
#define SHOW_FIELD_COLUMN     1
#define GROUP_BY_COLUMN       2
#define CANONICAL_NAME_COLUMN 3

#define QUANTITY_COLUMN   ( GetNumberCols() - 1 )

#ifdef __WXMAC__
#define COLUMN_MARGIN 5
#else
#define COLUMN_MARGIN 15
#endif

enum
{
    MYID_SELECT_FOOTPRINT = 991,         // must be within GRID_TRICKS' enum range
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
    void showPopupMenu( wxMenu& menu ) override
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

        GRID_TRICKS::showPopupMenu( menu );
    }

    void doPopupSelection( wxCommandEvent& event ) override
    {
        if( event.GetId() == MYID_SELECT_FOOTPRINT )
        {
            // pick a footprint using the footprint picker.
            wxString      fpid = m_grid->GetCellValue( m_grid->GetGridCursorRow(), FOOTPRINT_FIELD );
            KIWAY_PLAYER* frame = m_dlg->Kiway().Player( FRAME_FOOTPRINT_VIEWER_MODAL, true, m_dlg );

            if( frame->ShowModal( &fpid, m_dlg ) )
                m_grid->SetCellValue( m_grid->GetGridCursorRow(), FOOTPRINT_FIELD, fpid );

            frame->Destroy();
        }
        else if (event.GetId() == MYID_SHOW_DATASHEET )
        {
            wxString datasheet_uri = m_grid->GetCellValue( m_grid->GetGridCursorRow(), DATASHEET_FIELD );
            GetAssociatedDocument( m_dlg, datasheet_uri, &m_dlg->Prj() );
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


class FIELDS_EDITOR_GRID_DATA_MODEL : public wxGridTableBase
{
protected:
    // The data model is fundamentally m_componentRefs X m_fieldNames.

    SCH_EDIT_FRAME*       m_frame;
    SCH_REFERENCE_LIST    m_symbolsList;
    bool                  m_edited;
    std::vector<wxString> m_fieldNames;
    int                   m_sortColumn;
    bool                  m_sortAscending;

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

    void AddColumn( const wxString& aFieldName )
    {
        m_fieldNames.push_back( aFieldName );

        for( unsigned i = 0; i < m_symbolsList.GetCount(); ++i )
        {
            SCH_SYMBOL* symbol = m_symbolsList[ i ].GetSymbol();
            m_dataStore[ symbol->m_Uuid ][ aFieldName ] = symbol->GetFieldText( aFieldName,
                                                                                m_frame );
        }
    }

    int GetNumberRows() override { return m_rows.size(); }

    // Columns are fieldNames + quantity column
    int GetNumberCols() override { return (int) m_fieldNames.size() + 1; }

    wxString GetColLabelValue( int aCol ) override
    {
        if( aCol == QUANTITY_COLUMN )
            return _( "Qty" );
        else if( aCol < MANDATORY_FIELDS )
            return TEMPLATE_FIELDNAME::GetDefaultFieldName( aCol );
        else
            return m_fieldNames[ aCol ];
    }

    wxString GetCanonicalColLabel( int aCol )
    {
        if( aCol == QUANTITY_COLUMN )
            return _( "Qty" );
        else
            return m_fieldNames[ aCol ];
    }

    bool IsEmptyCell( int aRow, int aCol ) override
    {
        return false;   // don't allow adjacent cell overflow, even if we are actually empty
    }

    wxString GetValue( int aRow, int aCol ) override
    {
        if( aCol == REFERENCE_FIELD )
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

    std::vector<SCH_REFERENCE> GetRowReferences( int aRow ) const
    {
        wxCHECK( aRow < (int)m_rows.size(), std::vector<SCH_REFERENCE>() );
        return m_rows[ aRow ].m_Refs;
    }

    wxString GetValue( const DATA_MODEL_ROW& group, int aCol )
    {
        std::vector<SCH_REFERENCE> references;
        wxString                   fieldValue;

        for( const auto& ref : group.m_Refs )
        {
            if( aCol == REFERENCE_FIELD || aCol == QUANTITY_COLUMN )
            {
                references.push_back( ref );
            }
            else // Other columns are either a single value or ROW_MULTI_ITEMS
            {
                const KIID& symbolID = ref.GetSymbol()->m_Uuid;

                if( !m_dataStore.count( symbolID )
                        || !m_dataStore[ symbolID ].count( m_fieldNames[ aCol ] ) )
                {
                    return INDETERMINATE_STATE;
                }

                if( &ref == &group.m_Refs.front() )
                    fieldValue = m_dataStore[ symbolID ][ m_fieldNames[ aCol ] ];
                else if ( fieldValue != m_dataStore[ symbolID ][ m_fieldNames[ aCol ] ] )
                    return INDETERMINATE_STATE;
            }
        }

        if( aCol == REFERENCE_FIELD || aCol == QUANTITY_COLUMN )
        {
            // Remove duplicates (other units of multi-unit parts)
            std::sort( references.begin(), references.end(),
                    []( const SCH_REFERENCE& l, const SCH_REFERENCE& r ) -> bool
                    {
                        wxString l_ref( l.GetRef() << l.GetRefNumber() );
                        wxString r_ref( r.GetRef() << r.GetRefNumber() );
                        return UTIL::RefDesStringCompare( l_ref, r_ref ) < 0;
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

        if( aCol == REFERENCE_FIELD )
            fieldValue = SCH_REFERENCE_LIST::Shorthand( references );
        else if( aCol == QUANTITY_COLUMN )
            fieldValue = wxString::Format( wxT( "%d" ), ( int )references.size() );

        return fieldValue;
    }

    void SetValue( int aRow, int aCol, const wxString &aValue ) override
    {
        if( aCol == REFERENCE_FIELD || aCol == QUANTITY_COLUMN )
            return;             // Can't modify references or quantity

        DATA_MODEL_ROW& rowGroup = m_rows[ aRow ];
        wxString fieldName = m_fieldNames[ aCol ];

        for( const auto& ref : rowGroup.m_Refs )
            m_dataStore[ ref.GetSymbol()->m_Uuid ][ fieldName ] = aValue;

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
            return local_cmp( UTIL::RefDesStringCompare( lhRef, rhRef ), 0 );
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

        std::sort( m_rows.begin(), m_rows.end(),
               [ this ]( const DATA_MODEL_ROW& lhs, const DATA_MODEL_ROW& rhs ) -> bool
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

            wxString fieldName = fieldsCtrl->GetTextValue( i, CANONICAL_NAME_COLUMN );

            if( m_dataStore[ lhRefID ][ fieldName ] != m_dataStore[ rhRefID ][ fieldName ] )
                return false;

            matchFound = true;
        }

        return matchFound;
    }

    void RebuildRows( wxCheckBox* aGroupSymbolsBox, wxDataViewListCtrl* aFieldsCtrl )
    {
        if ( GetView() )
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
            bool          matchFound = false;

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
                const wxString& srcName = srcData.first;
                const wxString& srcValue = srcData.second;
                SCH_FIELD*      destField = symbol.FindField( srcName );

                if( !destField && !srcValue.IsEmpty() )
                {
                    const wxPoint symbolPos = symbol.GetPosition();
                    destField = symbol.AddField( SCH_FIELD( symbolPos, -1, &symbol, srcName ) );
                }

                if( !destField )
                {
                    symbol.RemoveField( srcName );
                }
                else if( destField->GetId() == REFERENCE_FIELD )
                {
                    // Reference is not editable
                }
                else if( destField->GetId() == VALUE_FIELD )
                {
                    // Value field cannot be empty
                    if( !srcValue.IsEmpty() )
                        symbol.SetValue( srcValue );
                }
                else if( destField->GetId() == FOOTPRINT_FIELD )
                {
                    symbol.SetFootprint( srcValue );
                }
                else
                {
                    destField->SetText( srcValue );
                }
            }
        }

        m_edited = false;
    }

    int GetDataWidth( int aCol )
    {
        int width = 0;

        if( aCol == REFERENCE_FIELD )
        {
            for( int row = 0; row < GetNumberRows(); ++row )
                width = std::max( width, KIUI::GetTextSize( GetValue( row, aCol ), GetView() ).x );
        }
        else
        {
            wxString column_label = GetColLabelValue( aCol );  // symbol fieldName or Qty string

            for( unsigned symbolRef = 0; symbolRef < m_symbolsList.GetCount(); ++ symbolRef )
            {
                const KIID& symbolID = m_symbolsList[ symbolRef ].GetSymbol()->m_Uuid;
                wxString    text = m_dataStore[ symbolID ][ column_label ];

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


DIALOG_FIELDS_EDITOR_GLOBAL::DIALOG_FIELDS_EDITOR_GLOBAL( SCH_EDIT_FRAME* parent ) :
        DIALOG_FIELDS_EDITOR_GLOBAL_BASE( parent ),
        m_parent( parent )
{
    wxSize defaultDlgSize = ConvertDialogToPixels( wxSize( 600, 300 ) );

    // Get all symbols from the list of schematic sheets
    m_parent->Schematic().GetSheets().GetSymbols( m_symbolsList, false );

    m_bRefresh->SetBitmap( KiBitmap( BITMAPS::small_refresh ) );

    m_fieldsCtrl->AppendTextColumn( _( "Field" ), wxDATAVIEW_CELL_INERT, 0, wxALIGN_LEFT, 0 );
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

    m_dataModel = new FIELDS_EDITOR_GRID_DATA_MODEL( m_parent, m_symbolsList );

    LoadFieldNames();   // loads rows into m_fieldsCtrl and columns into m_dataModel

    // Now that the fields are loaded we can set the initial location of the splitter
    // based on the list width.  Again, SetWidth( wxCOL_WIDTH_AUTOSIZE ) fails us on GTK.
    int nameColWidth = 0;

    for( int row = 0; row < m_fieldsCtrl->GetItemCount(); ++row )
    {
        const wxString& fieldName = m_fieldsCtrl->GetTextValue( row, DISPLAY_NAME_COLUMN );
        nameColWidth = std::max( nameColWidth, KIUI::GetTextSize( fieldName, m_fieldsCtrl ).x );
    }

    int fieldsMinWidth = nameColWidth + m_groupByColWidth + m_showColWidth;

    m_fieldsCtrl->GetColumn( DISPLAY_NAME_COLUMN )->SetWidth( nameColWidth );

    // This is used for data only.  Don't show it to the user.
    m_fieldsCtrl->GetColumn( CANONICAL_NAME_COLUMN )->SetWidth( 0 );

    m_splitterMainWindow->SetMinimumPaneSize( fieldsMinWidth );
    m_splitterMainWindow->SetSashPosition( fieldsMinWidth + 40 );

    m_dataModel->RebuildRows( m_groupSymbolsBox, m_fieldsCtrl );
    m_dataModel->Sort( 0, true );

    // wxGrid's column moving is buggy with native headers and this is one dialog where you'd
    // really like to be able to rearrange columns.
    m_grid->UseNativeColHeader( false );
    m_grid->SetTable( m_dataModel, true );

    // must be done after SetTable(), which appears to re-set it
    m_grid->SetSelectionMode( wxGrid::wxGridSelectCells );

    // sync m_grid's column visibilities to Show checkboxes in m_fieldsCtrl
    for( int i = 0; i < m_fieldsCtrl->GetItemCount(); ++i )
    {
        if( m_fieldsCtrl->GetToggleValue( i, 1 ) )
            m_grid->ShowCol( i );
        else
            m_grid->HideCol( i );
    }

    // add Cut, Copy, and Paste to wxGrid
    m_grid->PushEventHandler( new FIELDS_EDITOR_GRID_TRICKS( this, m_grid, m_fieldsCtrl ) );

    // give a bit more room for comboboxes
    m_grid->SetDefaultRowSize( m_grid->GetDefaultRowSize() + 4 );

    // set reference column attributes
    wxGridCellAttr* attr = new wxGridCellAttr;
    attr->SetReadOnly();
    m_grid->SetColAttr( REFERENCE_FIELD, attr );

    // set footprint column browse button
    attr = new wxGridCellAttr;
    attr->SetEditor( new GRID_CELL_FOOTPRINT_ID_EDITOR( this ) );
    m_grid->SetColAttr( FOOTPRINT_FIELD, attr );

    // set datasheet column viewer button
    attr = new wxGridCellAttr;
    attr->SetEditor( new GRID_CELL_URL_EDITOR( this ) );
    m_grid->SetColAttr( DATASHEET_FIELD, attr );

    // set quantities column attributes
    attr = new wxGridCellAttr;
    attr->SetReadOnly();
    m_grid->SetColAttr( m_dataModel->GetColsCount() - 1, attr );
    m_grid->SetColFormatNumber( m_dataModel->GetColsCount() - 1 );
    m_grid->AutoSizeColumns( false );

    for( int col = 0; col < m_grid->GetNumberCols(); ++col )
    {
        // Columns are hidden by setting their width to 0 so if we resize them they will
        // become unhidden.
        if( m_grid->IsColShown( col ) )
        {
            EESCHEMA_SETTINGS* cfg = static_cast<EESCHEMA_SETTINGS*>( Kiface().KifaceSettings() );
            std::string        key( m_dataModel->GetCanonicalColLabel( col ).ToUTF8() );

            if( cfg->m_FieldEditorPanel.column_widths.count( key ) )
            {
                int width = cfg->m_FieldEditorPanel.column_widths.at( key );
                m_grid->SetColSize( col, width );
            }
            else
            {
                int textWidth = m_dataModel->GetDataWidth( col ) + COLUMN_MARGIN;
                int maxWidth = defaultDlgSize.x / 3;

                if( col == m_grid->GetNumberCols() - 1 )
                    m_grid->SetColSize( col, std::min( std::max( 50, textWidth ), maxWidth ) );
                else
                    m_grid->SetColSize( col, std::min( std::max( 100, textWidth ), maxWidth ) );
            }
        }
    }

    m_grid->SelectRow( 0 );
    m_grid->SetGridCursor( 0, 1 );
    SetInitialFocus( m_grid );

    m_sdbSizerOK->SetDefault();

    finishDialogSettings();
    SetSize( defaultDlgSize );
    Center();

    // Connect Events
    m_grid->Connect( wxEVT_GRID_COL_SORT,
                     wxGridEventHandler( DIALOG_FIELDS_EDITOR_GLOBAL::OnColSort ), NULL, this );
}


DIALOG_FIELDS_EDITOR_GLOBAL::~DIALOG_FIELDS_EDITOR_GLOBAL()
{
    // Disconnect Events
    m_grid->Disconnect( wxEVT_GRID_COL_SORT,
                        wxGridEventHandler( DIALOG_FIELDS_EDITOR_GLOBAL::OnColSort ), NULL, this );

    // Delete the GRID_TRICKS.
    m_grid->PopEventHandler( true );

    // we gave ownership of m_dataModel to the wxGrid...
}


bool DIALOG_FIELDS_EDITOR_GLOBAL::TransferDataToWindow()
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


bool DIALOG_FIELDS_EDITOR_GLOBAL::TransferDataFromWindow()
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


void DIALOG_FIELDS_EDITOR_GLOBAL::AddField( const wxString& aDisplayName,
                                            const wxString& aCanonicalName,
                                            bool defaultShow, bool defaultSortBy )
{
    m_dataModel->AddColumn( aCanonicalName );

    wxVector<wxVariant> fieldsCtrlRow;

    EESCHEMA_SETTINGS* cfg     = static_cast<EESCHEMA_SETTINGS*>( Kiface().KifaceSettings() );
    bool               show    = defaultShow;
    bool               sort_by = defaultSortBy;

    std::string key( aCanonicalName.ToUTF8() );

    if( cfg->m_FieldEditorPanel.fields_show.count( key ) )
        show = cfg->m_FieldEditorPanel.fields_show.at( key );

    if( cfg->m_FieldEditorPanel.fields_group_by.count( key ) )
        sort_by = cfg->m_FieldEditorPanel.fields_group_by.at( key );

    // Don't change these to emplace_back: some versions of wxWidgets don't support it
    fieldsCtrlRow.push_back( wxVariant( aDisplayName ) );
    fieldsCtrlRow.push_back( wxVariant( show ) );
    fieldsCtrlRow.push_back( wxVariant( sort_by ) );
    fieldsCtrlRow.push_back( wxVariant( aCanonicalName ) );

    m_fieldsCtrl->AppendItem( fieldsCtrlRow );
}


/**
 * Constructs the rows of m_fieldsCtrl and the columns of m_dataModel from a union of all
 * field names in use.
 */
void DIALOG_FIELDS_EDITOR_GLOBAL::LoadFieldNames()
{
    std::set<wxString> userFieldNames;

    for( unsigned i = 0; i < m_symbolsList.GetCount(); ++i )
    {
        SCH_SYMBOL* symbol = m_symbolsList[ i ].GetSymbol();

        for( int j = MANDATORY_FIELDS; j < symbol->GetFieldCount(); ++j )
            userFieldNames.insert( symbol->GetFields()[j].GetName() );
    }

    // Force References to always be shown
    auto cfg = dynamic_cast<EESCHEMA_SETTINGS*>( Kiface().KifaceSettings() );
    wxCHECK( cfg, /*void*/ );

    cfg->m_FieldEditorPanel.fields_show["Reference"] = true;

    AddField( _( "Reference" ), wxT( "Reference" ), true, true  );
    AddField( _( "Value" ),     wxT( "Value" ),     true, true  );
    AddField( _( "Footprint" ), wxT( "Footprint" ), true, true  );
    AddField( _( "Datasheet" ), wxT( "Datasheet" ), true, false );

    for( const wxString& fieldName : userFieldNames )
        AddField( fieldName, fieldName, true, false );

    // Add any templateFieldNames which aren't already present in the userFieldNames
    for( const TEMPLATE_FIELDNAME& templateFieldname :
            m_parent->Schematic().Settings().m_TemplateFieldNames.GetTemplateFieldNames() )
    {
        if( userFieldNames.count( templateFieldname.m_Name ) == 0 )
            AddField( templateFieldname.m_Name, templateFieldname.m_Name, false, false );
    }
}


void DIALOG_FIELDS_EDITOR_GLOBAL::OnAddField( wxCommandEvent& event )
{
    // quantities column will become new field column, so it needs to be reset
    auto attr = new wxGridCellAttr;
    m_grid->SetColAttr( m_dataModel->GetColsCount() - 1, attr );
    m_grid->SetColFormatCustom( m_dataModel->GetColsCount() - 1, wxGRID_VALUE_STRING );

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
        if( fieldName == m_dataModel->GetColLabelValue( i ) )
        {
            DisplayError( this, wxString::Format( _( "Field name \"%s\" already in use." ),
                                                  fieldName ) );
            return;
        }
    }

    std::string key( fieldName.ToUTF8() );

    auto cfg = static_cast<EESCHEMA_SETTINGS*>( Kiface().KifaceSettings() );
    cfg->m_FieldEditorPanel.fields_show[key] = true;

    AddField( fieldName, fieldName, true, false );

    wxGridTableMessage msg( m_dataModel, wxGRIDTABLE_NOTIFY_COLS_INSERTED,
                            m_fieldsCtrl->GetItemCount(), 1 );
    m_grid->ProcessTableMessage( msg );

    // set up attributes on the new quantities column
    attr = new wxGridCellAttr;
    attr->SetReadOnly();
    m_grid->SetColAttr( m_dataModel->GetColsCount() - 1, attr );
    m_grid->SetColFormatNumber( m_dataModel->GetColsCount() - 1 );
    m_grid->SetColSize( m_dataModel->GetColsCount() - 1, 50 );
}


void DIALOG_FIELDS_EDITOR_GLOBAL::OnColumnItemToggled( wxDataViewEvent& event )
{
    EESCHEMA_SETTINGS* cfg = static_cast<EESCHEMA_SETTINGS*>( Kiface().KifaceSettings() );
    wxDataViewItem     item = event.GetItem();

    int row = m_fieldsCtrl->ItemToRow( item );
    int col = event.GetColumn();

    switch ( col )
    {
    case SHOW_FIELD_COLUMN:
    {
        bool value = m_fieldsCtrl->GetToggleValue( row, col );

        if( row == REFERENCE_FIELD && !value )
        {
            DisplayError( this, _( "The Reference column cannot be hidden." ) );

            value = true;
            m_fieldsCtrl->SetToggleValue( value, row, col );
        }

        std::string fieldName( m_fieldsCtrl->GetTextValue( row, CANONICAL_NAME_COLUMN ).ToUTF8() );
        cfg->m_FieldEditorPanel.fields_show[fieldName] = value;

        if( value )
            m_grid->ShowCol( row );
        else
            m_grid->HideCol( row );     // grid's columns map to fieldsCtrl's rows

        break;
    }

    case GROUP_BY_COLUMN:
    {
        bool value = m_fieldsCtrl->GetToggleValue( row, col );
        std::string fieldName( m_fieldsCtrl->GetTextValue( row, CANONICAL_NAME_COLUMN ).ToUTF8() );
        cfg->m_FieldEditorPanel.fields_group_by[fieldName] = value;

        m_dataModel->RebuildRows( m_groupSymbolsBox, m_fieldsCtrl );
        m_dataModel->Sort( m_grid->GetSortingColumn(), m_grid->IsSortOrderAscending() );
        m_grid->ForceRefresh();
        break;
    }

    default:
        break;
    }
}


void DIALOG_FIELDS_EDITOR_GLOBAL::OnGroupSymbolsToggled( wxCommandEvent& event )
{
    m_dataModel->RebuildRows( m_groupSymbolsBox, m_fieldsCtrl );
    m_dataModel->Sort( m_grid->GetSortingColumn(), m_grid->IsSortOrderAscending() );
    m_grid->ForceRefresh();
}


void DIALOG_FIELDS_EDITOR_GLOBAL::OnColSort( wxGridEvent& aEvent )
{
    int sortCol = aEvent.GetCol();
    bool ascending;

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

    m_dataModel->Sort( sortCol, ascending );
    m_grid->ForceRefresh();
}


void DIALOG_FIELDS_EDITOR_GLOBAL::OnTableValueChanged( wxGridEvent& aEvent )
{
    m_grid->ForceRefresh();
}


void DIALOG_FIELDS_EDITOR_GLOBAL::OnTableColSize( wxGridSizeEvent& aEvent )
{
    EESCHEMA_SETTINGS* cfg = static_cast<EESCHEMA_SETTINGS*>( Kiface().KifaceSettings() );
    int                col = aEvent.GetRowOrCol();
    std::string        key( m_dataModel->GetCanonicalColLabel( col ).ToUTF8() );

    if( m_grid->GetColSize( col ) )
        cfg->m_FieldEditorPanel.column_widths[ key ] = m_grid->GetColSize( col );

    aEvent.Skip();
}


void DIALOG_FIELDS_EDITOR_GLOBAL::OnRegroupSymbols( wxCommandEvent& aEvent )
{
    m_dataModel->RebuildRows( m_groupSymbolsBox, m_fieldsCtrl );
    m_dataModel->Sort( m_grid->GetSortingColumn(), m_grid->IsSortOrderAscending() );
    m_grid->ForceRefresh();
}


void DIALOG_FIELDS_EDITOR_GLOBAL::OnTableCellClick( wxGridEvent& event )
{
    if( event.GetCol() == REFERENCE_FIELD )
    {
        m_grid->ClearSelection();
        m_grid->SetGridCursor( event.GetRow(), event.GetCol() );

        m_dataModel->ExpandCollapseRow( event.GetRow() );
        std::vector<SCH_REFERENCE> refs = m_dataModel->GetRowReferences( event.GetRow() );

        // Focus Eeschema view on the symbol selected in the dialog
        if( refs.size() == 1 )
        {
            SCH_EDITOR_CONTROL* editor = m_parent->GetToolManager()->GetTool<SCH_EDITOR_CONTROL>();

            editor->FindSymbolAndItem( refs[ 0 ].GetRef() + refs[ 0 ].GetRefNumber(), true,
                                       HIGHLIGHT_SYMBOL, wxEmptyString );
        }
    }
    else
    {
        event.Skip();
    }
}


void DIALOG_FIELDS_EDITOR_GLOBAL::OnTableItemContextMenu( wxGridEvent& event )
{
    // TODO: Option to select footprint if FOOTPRINT column selected

    event.Skip();
}


void DIALOG_FIELDS_EDITOR_GLOBAL::OnSizeFieldList( wxSizeEvent& event )
{
    int nameColWidth = event.GetSize().GetX() - m_showColWidth - m_groupByColWidth - 8;

    // GTK loses its head and messes these up when resizing the splitter bar:
    m_fieldsCtrl->GetColumn( SHOW_FIELD_COLUMN )->SetWidth( m_showColWidth );
    m_fieldsCtrl->GetColumn( GROUP_BY_COLUMN )->SetWidth( m_groupByColWidth );

    m_fieldsCtrl->GetColumn( CANONICAL_NAME_COLUMN )->SetWidth( 0 );
    m_fieldsCtrl->GetColumn( DISPLAY_NAME_COLUMN )->SetWidth( nameColWidth );

    event.Skip();
}


void DIALOG_FIELDS_EDITOR_GLOBAL::OnSaveAndContinue( wxCommandEvent& aEvent )
{
    if( TransferDataFromWindow() )
        m_parent->SaveProject();
}


void DIALOG_FIELDS_EDITOR_GLOBAL::OnCancel( wxCommandEvent& event )
{
    Close();
}


void DIALOG_FIELDS_EDITOR_GLOBAL::OnClose( wxCloseEvent& event )
{
    // This is a cancel, so commit quietly as we're going to throw the results away anyway.
    m_grid->CommitPendingChanges( true );

    if( m_dataModel->IsEdited() )
    {
        if( !HandleUnsavedChanges( this, _( "Save changes?" ),
                                   [&]()->bool
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
