/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2017 Oliver Walters
 * Copyright (C) 2017-2019 KiCad Developers, see AUTHORS.txt for contributors.
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


#include <wx/msgdlg.h>
#include <wx/grid.h>
#include <widgets/wx_grid.h>
#include <base_units.h>
#include <confirm.h>
#include <bitmaps.h>
#include <grid_tricks.h>
#include <kicad_string.h>
#include <refdes_utils.h>
#include <general.h>
#include <class_library.h>
#include <sch_edit_frame.h>
#include <sch_reference_list.h>
#include <tools/sch_editor_control.h>
#include <kiface_i.h>
#include <eda_doc.h>
#include <widgets/grid_text_button_helpers.h>

#include "dialog_fields_editor_global.h"


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
        if( m_grid->GetGridCursorCol() == FOOTPRINT )
        {
            menu.Append( MYID_SELECT_FOOTPRINT, _( "Select Footprint..." ),
                         _( "Browse for footprint" ) );
            menu.AppendSeparator();
        }
        else if( m_grid->GetGridCursorCol() == DATASHEET )
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
            wxString      fpid = m_grid->GetCellValue( m_grid->GetGridCursorRow(), FOOTPRINT );
            KIWAY_PLAYER* frame = m_dlg->Kiway().Player( FRAME_PCB_MODULE_VIEWER_MODAL, true,
                                                         m_dlg );

            if( frame->ShowModal( &fpid, m_dlg ) )
                m_grid->SetCellValue( m_grid->GetGridCursorRow(), FOOTPRINT, fpid );

            frame->Destroy();
        }
        else if (event.GetId() == MYID_SHOW_DATASHEET )
        {
            wxString datasheet_uri = m_grid->GetCellValue( m_grid->GetGridCursorRow(), DATASHEET );
            GetAssociatedDocument( m_dlg, datasheet_uri );
        }
        else
        {
            GRID_TRICKS::doPopupSelection( event );
        }

        if( event.GetId() >= GRIDTRICKS_FIRST_SHOWHIDE && event.GetId() < GRIDTRICKS_LAST_ID )
        {
            if( !m_grid->IsColShown( REFERENCE ) )
            {
                DisplayError( m_dlg, _( "The Reference column cannot be hidden." ) );

                m_grid->ShowCol( REFERENCE );
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
    DATA_MODEL_ROW( SCH_REFERENCE aFirstReference, GROUP_TYPE aType )
    {
        m_Refs.push_back( aFirstReference );
        m_Flag = aType;
    }

    GROUP_TYPE                 m_Flag;
    std::vector<SCH_REFERENCE> m_Refs;
};


#define FIELD_NAME_COLUMN 0
#define SHOW_FIELD_COLUMN 1
#define GROUP_BY_COLUMN   2

#define QUANTITY_COLUMN   ( GetNumberCols() - 1 )

#ifdef __WXMAC__
#define COLUMN_MARGIN 5
#else
#define COLUMN_MARGIN 15
#endif


class FIELDS_EDITOR_GRID_DATA_MODEL : public wxGridTableBase
{
protected:
    // The data model is fundamentally m_componentRefs X m_fieldNames.

    SCH_EDIT_FRAME*       m_frame;
    SCH_REFERENCE_LIST    m_componentRefs;
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
    std::map< timestamp_t, std::map<wxString, wxString> > m_dataStore;


public:
    FIELDS_EDITOR_GRID_DATA_MODEL( SCH_EDIT_FRAME* aFrame, SCH_REFERENCE_LIST& aComponentList ) :
            m_frame( aFrame ),
            m_componentRefs( aComponentList ),
            m_edited( false ),
            m_sortColumn( 0 ),
            m_sortAscending( false )
    {
        m_componentRefs.SplitReferences();
    }


    void AddColumn( const wxString& aFieldName )
    {
        m_fieldNames.push_back( aFieldName );

        for( unsigned i = 0; i < m_componentRefs.GetCount(); ++i )
        {
            SCH_COMPONENT* comp = m_componentRefs[ i ].GetComp();
            timestamp_t compID = comp->GetTimeStamp();

            m_dataStore[ compID ][ aFieldName ] = comp->GetFieldText( aFieldName, m_frame );
        }
    }


    int GetNumberRows() override { return m_rows.size(); }

    // Columns are fieldNames + quantity column
    int GetNumberCols() override { return m_fieldNames.size() + 1; }


    wxString GetColLabelValue( int aCol ) override
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
        if( aCol == REFERENCE )
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
            return GetValue( m_rows[ aRow ], aCol );
    }

    std::vector<SCH_REFERENCE> GetRowReferences( int aRow )
    {
        wxCHECK( aRow < (int)m_rows.size(), std::vector<SCH_REFERENCE>() );
        return m_rows[ aRow ].m_Refs;
    }

    wxString GetValue( DATA_MODEL_ROW& group, int aCol )
    {
        std::vector<SCH_REFERENCE> references;
        wxString                   fieldValue;

        for( const auto& ref : group.m_Refs )
        {
            if( aCol == REFERENCE || aCol == QUANTITY_COLUMN )
            {
                references.push_back( ref );
            }
            else // Other columns are either a single value or ROW_MULTI_ITEMS
            {
                timestamp_t compID = ref.GetComp()->GetTimeStamp();

                if( !m_dataStore.count( compID ) ||
                        !m_dataStore[ compID ].count( m_fieldNames[ aCol ] ) )
                    return INDETERMINATE;

                if( &ref == &group.m_Refs.front() )
                    fieldValue = m_dataStore[ compID ][ m_fieldNames[ aCol ] ];
                else if ( fieldValue != m_dataStore[ compID ][ m_fieldNames[ aCol ] ] )
                    return INDETERMINATE;
            }
        }

        if( aCol == REFERENCE || aCol == QUANTITY_COLUMN )
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

        if( aCol == REFERENCE )
        {
            fieldValue = SCH_REFERENCE_LIST::Shorthand( references );
        }
        else if( aCol == QUANTITY_COLUMN )
        {
            fieldValue = wxString::Format( wxT( "%d" ), ( int )references.size() );
        }

        return fieldValue;
    }


    void SetValue( int aRow, int aCol, const wxString &aValue ) override
    {
        if( aCol == REFERENCE || aCol == QUANTITY_COLUMN )
            return;             // Can't modify references or quantity

        DATA_MODEL_ROW& rowGroup = m_rows[ aRow ];
        wxString fieldName = m_fieldNames[ aCol ];

        for( const auto& ref : rowGroup.m_Refs )
            m_dataStore[ ref.GetComp()->GetTimeStamp() ][ fieldName ] = aValue;

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

        bool retVal;

        // Primary sort key is sortCol; secondary is always REFERENCE (column 0)

        wxString lhs = dataModel->GetValue( (DATA_MODEL_ROW&) lhGroup, sortCol );
        wxString rhs = dataModel->GetValue( (DATA_MODEL_ROW&) rhGroup, sortCol );

        if( lhs == rhs || sortCol == REFERENCE )
        {
            wxString lhRef = lhGroup.m_Refs[ 0 ].GetRef() + lhGroup.m_Refs[ 0 ].GetRefNumber();
            wxString rhRef = rhGroup.m_Refs[ 0 ].GetRef() + rhGroup.m_Refs[ 0 ].GetRefNumber();
            retVal = UTIL::RefDesStringCompare( lhRef, rhRef ) < 0;
        }
        else
            retVal = ValueStringCompare( lhs, rhs ) < 0;

        if( ascending )
            return retVal;
        else
            return !retVal;
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
        // If items are unannotated then we can't tell if they're units of the same
        // component or not
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
        if( fieldsCtrl->GetToggleValue( REFERENCE, GROUP_BY_COLUMN ) )
        {
            // if we're grouping by reference, then only the prefix must match
            if( lhRef.GetRef() != rhRef.GetRef() )
                return false;

            matchFound = true;
        }

        timestamp_t lhRefID = lhRef.GetComp()->GetTimeStamp();
        timestamp_t rhRefID = rhRef.GetComp()->GetTimeStamp();

        // Now check all the other columns.  This must be done out of the dataStore
        // for the refresh button to work after editing.
        for( int i = REFERENCE + 1; i < fieldsCtrl->GetItemCount(); ++i )
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


    void RebuildRows( wxCheckBox* groupComponentsBox, wxDataViewListCtrl* fieldsCtrl )
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

        for( unsigned i = 0; i < m_componentRefs.GetCount(); ++i )
        {
            SCH_REFERENCE ref = m_componentRefs[ i ];
            bool matchFound = false;

            // See if we already have a row which this component fits into
            for( auto& row : m_rows )
            {
                // all group members must have identical refs so just use the first one
                SCH_REFERENCE rowRef = row.m_Refs[ 0 ];

                if( unitMatch( ref, rowRef ) )
                {
                    matchFound = true;
                    row.m_Refs.push_back( ref );
                    break;
                }
                else if (groupComponentsBox->GetValue() && groupMatch( ref, rowRef, fieldsCtrl ) )
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

        for( auto& ref : m_rows[ aRow ].m_Refs )
        {
            bool matchFound = false;

            // See if we already have a child group which this component fits into
            for( auto& child : children )
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
        for( unsigned i = 0; i < m_componentRefs.GetCount(); ++i )
        {
            SCH_COMPONENT& comp = *m_componentRefs[i].GetComp();

            m_frame->SetCurrentSheet( m_componentRefs[i].GetSheetPath() );
            m_frame->SaveCopyInUndoList( &comp, UR_CHANGED, true );

            const std::map<wxString, wxString>& fieldStore = m_dataStore[comp.GetTimeStamp()];

            for( const std::pair<wxString, wxString> srcData : fieldStore )
            {
                const wxString& srcName = srcData.first;
                const wxString& srcValue = srcData.second;
                SCH_FIELD*      destField = comp.FindField( srcName );

                if( !destField && !srcValue.IsEmpty() )
                {
                    const auto compOrigin = comp.GetPosition();
                    destField = comp.AddField( SCH_FIELD( compOrigin, -1, &comp, srcName ) );
                }

                if( !destField )
                {
                    comp.RemoveField( srcName );
                    continue;
                }

                // Reference and value fields cannot be empty.  All other fields can.
                if( srcValue.IsEmpty()
                  && (destField->GetId() == REFERENCE || destField->GetId() == VALUE))
                    continue;

                destField->SetText( srcValue );
            }
        }

        m_edited = false;
    }


    int GetDataWidth( int aCol )
    {
        int width = 0;

        if( aCol == REFERENCE )
        {
            for( int row = 0; row < GetNumberRows(); ++row )
            {
                width = std::max( width, GetTextSize( GetValue( row, aCol ), GetView() ).x );
            }
        }
        else
        {
            wxString column_label = GetColLabelValue( aCol );  // component fieldName or Qty string

            for( unsigned compRef = 0; compRef < m_componentRefs.GetCount(); ++ compRef )
            {
                timestamp_t compId = m_componentRefs[ compRef ].GetComp()->GetTimeStamp();
                wxString text = m_dataStore[ compId ][ column_label ];
                width = std::max( width, GetTextSize( text, GetView() ).x );
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
        m_config( Kiface().KifaceSettings() ),
        m_parent( parent )
{
    wxSize defaultDlgSize = ConvertDialogToPixels( wxSize( 600, 300 ) );

    // Get all components from the list of schematic sheets
    SCH_SHEET_LIST sheets( g_RootSheet );
    sheets.GetComponents( m_componentRefs, false );

    m_bRefresh->SetBitmap( KiBitmap( refresh_xpm ) );

    m_fieldsCtrl->AppendTextColumn(   _( "Field" ), wxDATAVIEW_CELL_INERT, 0, wxALIGN_LEFT, 0 );
    m_fieldsCtrl->AppendToggleColumn( _( "Show" ), wxDATAVIEW_CELL_ACTIVATABLE, 0, wxALIGN_CENTER,
                                      0 );
    m_fieldsCtrl->AppendToggleColumn( _( "Group By" ), wxDATAVIEW_CELL_ACTIVATABLE, 0,
                                      wxALIGN_CENTER, 0 );

    // SetWidth( wxCOL_WIDTH_AUTOSIZE ) fails here on GTK, so we calculate the title sizes and
    // set the column widths ourselves.
    auto column = m_fieldsCtrl->GetColumn( SHOW_FIELD_COLUMN );
    m_showColWidth = GetTextSize( column->GetTitle(), m_fieldsCtrl ).x + COLUMN_MARGIN;
    column->SetWidth( m_showColWidth );

    column = m_fieldsCtrl->GetColumn( GROUP_BY_COLUMN );
    m_groupByColWidth = GetTextSize( column->GetTitle(), m_fieldsCtrl ).x + COLUMN_MARGIN;
    column->SetWidth( m_groupByColWidth );

    // The fact that we're a list should keep the control from reserving space for the
    // expander buttons... but it doesn't.  Fix by forcing the indent to 0.
    m_fieldsCtrl->SetIndent( 0 );

    m_dataModel = new FIELDS_EDITOR_GRID_DATA_MODEL( m_parent, m_componentRefs );

    LoadFieldNames();   // loads rows into m_fieldsCtrl and columns into m_dataModel

    // Now that the fields are loaded we can set the initial location of the splitter
    // based on the list width.  Again, SetWidth( wxCOL_WIDTH_AUTOSIZE ) fails us on GTK.
    int nameColWidth = 0;

    for( int row = 0; row < m_fieldsCtrl->GetItemCount(); ++row )
    {
        const wxString& fieldName = m_fieldsCtrl->GetTextValue( row, FIELD_NAME_COLUMN );
        nameColWidth = std::max( nameColWidth, GetTextSize( fieldName, m_fieldsCtrl ).x );
    }

    m_fieldsCtrl->GetColumn( FIELD_NAME_COLUMN )->SetWidth( nameColWidth );
    m_splitter1->SetSashPosition( nameColWidth + m_showColWidth + m_groupByColWidth + 40 );

    m_dataModel->RebuildRows( m_groupComponentsBox, m_fieldsCtrl );
    m_dataModel->Sort( 0, true );

    // wxGrid's column moving is buggy with native headers and this is one dialog where you'd
    // really like to be able to rearrange columns.
    m_grid->UseNativeColHeader( false );
    m_grid->SetTable( m_dataModel, true );

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
    m_grid->SetColAttr( REFERENCE, attr );

    // set footprint column browse button
    attr = new wxGridCellAttr;
    attr->SetEditor( new GRID_CELL_FOOTPRINT_ID_EDITOR( this ) );
    m_grid->SetColAttr( FOOTPRINT, attr );

    // set datasheet column viewer button
    attr = new wxGridCellAttr;
    attr->SetEditor( new GRID_CELL_URL_EDITOR( this ) );
    m_grid->SetColAttr( DATASHEET, attr );

    // set quantities column attributes
    attr = new wxGridCellAttr;
    attr->SetReadOnly();
    m_grid->SetColAttr( m_dataModel->GetColsCount() - 1, attr );
    m_grid->SetColFormatNumber( m_dataModel->GetColsCount() - 1 );
    m_grid->AutoSizeColumns( false );

    for( int col = 0; col < m_grid->GetNumberCols(); ++ col )
    {
        // Columns are hidden by setting their width to 0 so if we resize them they will
        // become unhidden.
        if( m_grid->IsColShown( col ) )
        {
            int textWidth = m_dataModel->GetDataWidth( col ) + COLUMN_MARGIN;
            int maxWidth = defaultDlgSize.x / 3;

            if( col == m_grid->GetNumberCols() - 1 )
                m_grid->SetColSize( col, std::min( std::max( 50, textWidth ), maxWidth ) );
            else
                m_grid->SetColSize( col, std::min( std::max( 100, textWidth ), maxWidth ) );
        }
    }

    m_grid->SetGridCursor( 0, 1 );
    SetInitialFocus( m_grid );

    m_sdbSizer1OK->SetDefault();

    FinishDialogSettings();
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

    // Clear highlighted symbols, if any
    m_parent->GetCanvas()->GetView()->HighlightItem( nullptr, nullptr );
    m_parent->GetCanvas()->Refresh();
}


bool DIALOG_FIELDS_EDITOR_GLOBAL::TransferDataFromWindow()
{
    if( !m_grid->CommitPendingChanges() )
        return false;

    if( !wxDialog::TransferDataFromWindow() )
        return false;

    SCH_SHEET_PATH currentSheet = m_parent->GetCurrentSheet();

    m_dataModel->ApplyData();
    m_parent->SyncView();
    m_parent->OnModify();

    // Reset the view to where we left the user
    m_parent->SetCurrentSheet( currentSheet );
    m_parent->Refresh();

    return true;
}


void DIALOG_FIELDS_EDITOR_GLOBAL::AddField( const wxString& aName,
                                            bool defaultShow, bool defaultSortBy )
{
    m_dataModel->AddColumn( aName );

    wxVector<wxVariant> fieldsCtrlRow;

    m_config->Read( "SymbolFieldEditor/Show/" + aName, &defaultShow );
    m_config->Read( "SymbolFieldEditor/GroupBy/" + aName, &defaultSortBy );

    fieldsCtrlRow.push_back( wxVariant( aName ) );
    fieldsCtrlRow.push_back( wxVariant( defaultShow ) );
    fieldsCtrlRow.push_back( wxVariant( defaultSortBy ) );

    m_fieldsCtrl->AppendItem( fieldsCtrlRow );
}


/**
 * Constructs the rows of m_fieldsCtrl and the columns of m_dataModel from a union of all
 * field names in use.
 */
void DIALOG_FIELDS_EDITOR_GLOBAL::LoadFieldNames()
{
    std::set<wxString> userFieldNames;

    for( unsigned i = 0; i < m_componentRefs.GetCount(); ++i )
    {
        SCH_COMPONENT* comp = m_componentRefs[ i ].GetComp();

        for( int j = MANDATORY_FIELDS; j < comp->GetFieldCount(); ++j )
            userFieldNames.insert( comp->GetField( j )->GetName() );
    }

    // Force References to always be shown
    m_config->Write( "SymbolFieldEditor/Show/Reference", true );

    AddField( _( "Reference" ), true, true  );
    AddField( _( "Value" ),     true, true  );
    AddField( _( "Footprint" ), true, true  );
    AddField( _( "Datasheet" ), true, false );

    for( const wxString& fieldName : userFieldNames )
        AddField( fieldName, true, false );

    // Add any templateFieldNames which aren't already present in the userFieldNames
    for( const TEMPLATE_FIELDNAME& templateFieldName : m_parent->GetTemplateFieldNames() )
        if( userFieldNames.count( templateFieldName.m_Name ) == 0 )
            AddField( templateFieldName.m_Name, false, false );
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

    m_config->Write( "SymbolFieldEditor/Show/" + fieldName, true );

    AddField( fieldName, true, false );

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
    wxDataViewItem item = event.GetItem();

    int row = m_fieldsCtrl->ItemToRow( item );
    int col = event.GetColumn();

    switch ( col )
    {
    default:
        break;

    case SHOW_FIELD_COLUMN:
    {
        bool value = m_fieldsCtrl->GetToggleValue( row, col );

        if( row == REFERENCE && !value )
        {
            DisplayError( this, _( "The Reference column cannot be hidden." ) );

            value = true;
            m_fieldsCtrl->SetToggleValue( value, row, col );
        }

        wxString fieldName = m_fieldsCtrl->GetTextValue( row, FIELD_NAME_COLUMN );
        m_config->Write( "SymbolFieldEditor/Show/" + fieldName, value );

        if( value )
            m_grid->ShowCol( row );
        else
            m_grid->HideCol( row );     // grid's columns map to fieldsCtrl's rows
        break;
    }

    case GROUP_BY_COLUMN:
    {
        bool value = m_fieldsCtrl->GetToggleValue( row, col );
        wxString fieldName = m_fieldsCtrl->GetTextValue( row, FIELD_NAME_COLUMN );
        m_config->Write( "SymbolFieldEditor/GroupBy/" + fieldName, value );
        m_dataModel->RebuildRows( m_groupComponentsBox, m_fieldsCtrl );
        m_dataModel->Sort( m_grid->GetSortingColumn(), m_grid->IsSortOrderAscending() );
        m_grid->ForceRefresh();
        break;
    }
    }
}


void DIALOG_FIELDS_EDITOR_GLOBAL::OnGroupComponentsToggled( wxCommandEvent& event )
{
    m_dataModel->RebuildRows( m_groupComponentsBox, m_fieldsCtrl );
    m_dataModel->Sort( m_grid->GetSortingColumn(), m_grid->IsSortOrderAscending() );
    m_grid->ForceRefresh();
}


void DIALOG_FIELDS_EDITOR_GLOBAL::OnColSort( wxGridEvent& aEvent )
{
    int sortCol = aEvent.GetCol();
    bool ascending;

    // This is bonkers, but wxWidgets doesn't tell us ascending/descending in the
    // event, and if we ask it will give us pre-event info.
    if( m_grid->IsSortingBy( sortCol ) )
        // same column; invert ascending
        ascending = !m_grid->IsSortOrderAscending();
    else
        // different column; start with ascending
        ascending = true;

    m_dataModel->Sort( sortCol, ascending );
}


void DIALOG_FIELDS_EDITOR_GLOBAL::OnTableValueChanged( wxGridEvent& event )
{
    m_grid->ForceRefresh();
}


void DIALOG_FIELDS_EDITOR_GLOBAL::OnRegroupComponents( wxCommandEvent& event )
{
    m_dataModel->RebuildRows( m_groupComponentsBox, m_fieldsCtrl );
    m_dataModel->Sort( m_grid->GetSortingColumn(), m_grid->IsSortOrderAscending() );
    m_grid->ForceRefresh();
}


void DIALOG_FIELDS_EDITOR_GLOBAL::OnTableCellClick( wxGridEvent& event )
{
    if( event.GetCol() == REFERENCE )
    {
        m_grid->ClearSelection();
        m_grid->SetGridCursor( event.GetRow(), event.GetCol() );

        // Clear highlighted symbols, if any
        m_parent->GetCanvas()->GetView()->HighlightItem( nullptr, nullptr );
        m_parent->GetCanvas()->Refresh();

        m_dataModel->ExpandCollapseRow( event.GetRow() );
        std::vector<SCH_REFERENCE> refs = m_dataModel->GetRowReferences( event.GetRow() );

        // Focus Eeschema view on the component selected in the dialog
        if( refs.size() == 1 )
        {
            SCH_EDITOR_CONTROL* editor = m_parent->GetToolManager()->GetTool<SCH_EDITOR_CONTROL>();

            editor->FindComponentAndItem( refs[0].GetRef() + refs[0].GetRefNumber(), true,
                                          HIGHLIGHT_COMPONENT, wxEmptyString );
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
    m_fieldsCtrl->GetColumn( 1 )->SetWidth( m_showColWidth );
    m_fieldsCtrl->GetColumn( 2 )->SetWidth( m_groupByColWidth );

    m_fieldsCtrl->GetColumn( 0 )->SetWidth( nameColWidth );

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
        if( !HandleUnsavedChanges( this, wxEmptyString,
                                   [&]()->bool { return TransferDataFromWindow(); } ) )
        {
            event.Veto();
            return;
        }
    }

    event.Skip();
}
