/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2017 Oliver Walters
 * Copyright (C) 2017-2018 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <confirm.h>
#include <bitmaps.h>
#include <grid_tricks.h>
#include <kicad_string.h>

#include <build_version.h>
#include <general.h>
#include <class_library.h>

#include <sch_edit_frame.h>
#include <sch_reference_list.h>

#include "dialog_fields_editor_global.h"


#define FIELD_NAME_COLUMN 0
#define SHOW_FIELD_COLUMN 1
#define GROUP_BY_COLUMN   2

#ifdef __WXMAC__
#define CHECKBOX_COLUMN_MARGIN 5
#else
#define CHECKBOX_COLUMN_MARGIN 15
#endif

#define QUANTITY_COLUMN   ( GetNumberCols() - 1 )


// Indicator that multiple values exist in child rows
#define ROW_MULT_ITEMS wxString( "< ... >" )


class FIELDS_EDITOR_GRID_DATA_MODEL : public wxGridTableBase
{
protected:
    // The data model is fundamentally m_componentRefs X m_fieldNames.

    SCH_REFERENCE_LIST    m_componentRefs;
    std::vector<wxString> m_fieldNames;

    // However, the grid view can vary in two ways:
    //   1) the componentRefs can be grouped into fewer rows
    //   2) some columns can be hidden
    //
    // We handle (1) here (ie: a table row maps to a group, and the table is rebuilt
    // when the groupings change), and we let the wxGrid handle (2) (ie: the number
    // of columns is constant but are hidden/shown by the wxGrid control).

    std::vector< std::vector<SCH_REFERENCE> > m_rows;

    // Data store
    // A map of compID : fieldSet, where fieldSet is a map of fieldName : fieldValue
    std::map< timestamp_t, std::map<wxString, wxString> > m_dataStore;

public:
    FIELDS_EDITOR_GRID_DATA_MODEL( SCH_REFERENCE_LIST& aComponentList )
    {
        m_componentRefs = aComponentList;
        m_componentRefs.SplitReferences();
    }


    void AddColumn( const wxString& aFieldName )
    {
        m_fieldNames.push_back( aFieldName );

        for( unsigned i = 0; i < m_componentRefs.GetCount(); ++i )
        {
            SCH_COMPONENT* comp = m_componentRefs[ i ].GetComp();
            timestamp_t compID = comp->GetTimeStamp();

            m_dataStore[ compID ][ aFieldName ] = comp->GetFieldText( aFieldName );
        }
    }


    int GetNumberRows() override { return m_rows.size(); }

    // Columns are fieldNames + quantity column
    int GetNumberCols() override { return m_fieldNames.size() + 1; }


    wxString GetColLabelValue( int aCol ) override
    {
        if( aCol == QUANTITY_COLUMN )
            return _T( "Qty" );
        else
            return m_fieldNames[ aCol ];
    }


    bool IsEmptyCell( int aRow, int aCol ) override
    {
        return false;   // don't allow adjacent cell overflow, even if we are actually empty
    }


    wxString GetValue( int aRow, int aCol ) override
    {
        return GetValue( m_rows[ aRow ], aCol );
    }


    wxString GetValue( std::vector<SCH_REFERENCE>& group, int aCol )
    {
        std::vector<wxString>  rootReferences;
        wxString               fieldValue;

        for( const auto& ref : group )
        {
            if( aCol == REFERENCE || aCol == QUANTITY_COLUMN )
            {
                rootReferences.push_back( ref.GetRef() << ref.GetRefNumber() );
            }
            else // Other columns are either a single value or ROW_MULTI_ITEMS
            {
                timestamp_t compID = ref.GetComp()->GetTimeStamp();

                if( &ref == &group.front() )
                    fieldValue = m_dataStore[ compID ][ m_fieldNames[ aCol ] ];
                else if ( fieldValue != m_dataStore[ compID ][ m_fieldNames[ aCol ] ] )
                    return ROW_MULT_ITEMS;
            }
        }

        if( aCol == REFERENCE || aCol == QUANTITY_COLUMN )
        {
            // Remove duplicates (other units of multi-unit parts)
            auto logicalEnd = std::unique( rootReferences.begin(), rootReferences.end(),
                    []( const wxString& l, const wxString& r )
                    {
                        // If not annotated then we don't really know if it's a duplicate
                        if( l.EndsWith( wxT( "?" ) ) )
                            return false;

                        return l == r;
                    } );
            rootReferences.erase( logicalEnd, rootReferences.end() );
        }

        if( aCol == REFERENCE )
        {
            for( const auto& ref : rootReferences )
            {
                if( fieldValue.length() )
                    fieldValue += wxT( ", " );
                fieldValue += ref;
            }
        }
        else if( aCol == QUANTITY_COLUMN )
        {
            fieldValue = wxString::Format( wxT( "%d" ), ( int )rootReferences.size() );
        }

        return fieldValue;
    }


    void SetValue( int aRow, int aCol, const wxString &aValue ) override
    {
        if( aCol == REFERENCE || aCol == QUANTITY_COLUMN )
            return;             // Can't modify references or quantity

        std::vector<SCH_REFERENCE>& rowGroup = m_rows[ aRow ];
        wxString fieldName = m_fieldNames[ aCol ];

        for( const auto& ref : rowGroup )
            m_dataStore[ ref.GetComp()->GetTimeStamp() ][ fieldName ] = aValue;
    }


    static bool cmp( const std::vector<SCH_REFERENCE>& lhGroup, const std::vector<SCH_REFERENCE>& rhGroup,
                     FIELDS_EDITOR_GRID_DATA_MODEL* dataModel, int sortCol, bool ascending )
    {
        // Empty rows always go to the bottom, whether ascending or descending
        if( lhGroup.size() == 0 )
            return true;
        else if( rhGroup.size() == 0 )
            return false;

        bool retVal;

        // Primary sort key is sortCol; secondary is always REFERENCE (column 0)

        wxString lhs = dataModel->GetValue( (std::vector<SCH_REFERENCE>&)lhGroup, sortCol );
        wxString rhs = dataModel->GetValue( (std::vector<SCH_REFERENCE>&)rhGroup, sortCol );

        if( lhs == rhs || sortCol == REFERENCE )
        {
            wxString lhRef = lhGroup[ 0 ].GetRef() + lhGroup[ 0 ].GetRefNumber();
            wxString rhRef = rhGroup[ 0 ].GetRef() + rhGroup[ 0 ].GetRefNumber();
            retVal = RefDesStringCompare( lhRef, rhRef ) < 0;
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

        std::sort( m_rows.begin(), m_rows.end(),
                   [ this, aColumn, ascending ]( const std::vector<SCH_REFERENCE>& lhs,
                                                 const std::vector<SCH_REFERENCE>& rhs ) -> bool
                   {
                       return cmp( lhs, rhs, this, aColumn, ascending );
                   } );
    }


    bool match( const SCH_REFERENCE& lhRef, const SCH_REFERENCE& rhRef,
                wxCheckBox* groupComponentsBox, wxDataViewListCtrl* fieldsCtrl )
    {
        // Units of same component always match
        if( lhRef.GetRef() == rhRef.GetRef() && lhRef.GetRefNumber() != wxT( "?" )
            && lhRef.GetRefNumber() == rhRef.GetRefNumber() )
            return true;

        // If we're not grouping, then nothing else matches
        if( !groupComponentsBox->GetValue() )
            return false;

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
            GetView()->DisableCellEditControl();

            wxGridTableMessage msg( this, wxGRIDTABLE_NOTIFY_ROWS_DELETED, 0, m_rows.size() );
            GetView()->ProcessTableMessage( msg );
        }

        m_rows.clear();

        for( unsigned i = 0; i < m_componentRefs.GetCount(); ++i )
        {
            SCH_REFERENCE compRef = m_componentRefs[ i ];
            bool matchFound = false;

            // See if we already have a group which this component fits into
            for( auto& rowGroup : m_rows )
            {
                // group members are by definition all matching, so just check
                // against the first member
                if( match( rowGroup[ 0 ], compRef, groupComponentsBox, fieldsCtrl ) )
                {
                    matchFound = true;
                    rowGroup.push_back( compRef );
                    break;
                }
            }

            if( !matchFound )
            {
                std::vector<SCH_REFERENCE> newGroup;
                newGroup.push_back( compRef );
                m_rows.push_back( newGroup );
            }
        }

        for( auto& rowGroup : m_rows )
        {
            std::sort( rowGroup.begin(), rowGroup.end(),
                       []( const SCH_REFERENCE& lhs, const SCH_REFERENCE& rhs ) -> bool
                       {
                           wxString lhRef( lhs.GetRef() << lhs.GetRefNumber() );
                           wxString rhRef( rhs.GetRef() << rhs.GetRefNumber() );
                           return RefDesStringCompare( lhRef, rhRef ) < 0;
                       } );
        }

        if ( GetView() )
        {
            wxGridTableMessage msg( this, wxGRIDTABLE_NOTIFY_ROWS_APPENDED, m_rows.size() );
            GetView()->ProcessTableMessage( msg );
        }
    }


    void ApplyData( SCH_EDIT_FRAME* aParent )
    {
        for( unsigned i = 0; i < m_componentRefs.GetCount(); ++i )
        {
            SCH_COMPONENT* comp = m_componentRefs[ i ].GetComp();

            aParent->SetCurrentSheet( m_componentRefs[ i ].GetSheetPath() );
            aParent->SaveCopyInUndoList( comp, UR_CHANGED, true );

            std::map<wxString, wxString>& fieldStore = m_dataStore[ comp->GetTimeStamp() ];

            for( std::pair<wxString, wxString> fieldData : fieldStore )
            {
                wxString   fieldName = fieldData.first;
                SCH_FIELD* field = comp->FindField( fieldName );

                if( !field )
                    field = comp->AddField( SCH_FIELD( wxPoint( 0, 0 ), -1, comp, fieldName ) );

                field->SetText( fieldData.second );
            }
        }
    }
};


DIALOG_FIELDS_EDITOR_GLOBAL::DIALOG_FIELDS_EDITOR_GLOBAL( SCH_EDIT_FRAME* parent ) :
        DIALOG_FIELDS_EDITOR_GLOBAL_BASE( parent ),
        m_parent( parent )
{
    // Get all components from the list of schematic sheets
    SCH_SHEET_LIST sheets( g_RootSheet );
    sheets.GetComponents( m_componentRefs, false );

    m_bRefresh->SetBitmap( KiBitmap( refresh_xpm ) );

    m_fieldsCtrl->AppendTextColumn(   _( "Field" ),    wxDATAVIEW_CELL_INERT,       0, wxALIGN_LEFT,   0 );
    m_fieldsCtrl->AppendToggleColumn( _( "Show" ),     wxDATAVIEW_CELL_ACTIVATABLE, 0, wxALIGN_CENTER, 0 );
    m_fieldsCtrl->AppendToggleColumn( _( "Group By" ), wxDATAVIEW_CELL_ACTIVATABLE, 0, wxALIGN_CENTER, 0 );

    // SetWidth( wxCOL_WIDTH_AUTOSIZE ) fails here on GTK, so we calculate the title sizes and
    // set the column widths ourselves.
    auto column = m_fieldsCtrl->GetColumn( SHOW_FIELD_COLUMN );
    m_showColWidth = GetTextSize( column->GetTitle(), m_fieldsCtrl ).x + CHECKBOX_COLUMN_MARGIN;
    column->SetWidth( m_showColWidth );

    column = m_fieldsCtrl->GetColumn( GROUP_BY_COLUMN );
    m_groupByColWidth = GetTextSize( column->GetTitle(), m_fieldsCtrl ).x + CHECKBOX_COLUMN_MARGIN;
    column->SetWidth( m_groupByColWidth );

    // The fact that we're a list should keep the control from reserving space for the
    // expander buttons... but it doesn't.  Fix by forcing the indent to 0.
    m_fieldsCtrl->SetIndent( 0 );

    m_dataModel = new FIELDS_EDITOR_GRID_DATA_MODEL( m_componentRefs );

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

    m_grid->UseNativeColHeader( true );
    m_grid->SetTable( m_dataModel, true );

    // add Cut, Copy, and Paste to wxGrid
    m_grid->PushEventHandler( new GRID_TRICKS( m_grid ) );

    // give a bit more room for editing
    m_grid->SetDefaultRowSize( m_grid->GetDefaultRowSize() + 2 );

    // set reference column attributes
    wxGridCellAttr* attr = new wxGridCellAttr;
    attr->SetReadOnly();
    m_grid->SetColAttr( 0, attr );
    m_grid->SetColMinimalWidth( 0, 100 );

    // set quantities column attributes
    attr = new wxGridCellAttr;
    attr->SetReadOnly();
    m_grid->SetColAttr( m_dataModel->GetColsCount() - 1, attr );
    m_grid->SetColFormatNumber( m_dataModel->GetColsCount() - 1 );
    m_grid->SetColMinimalWidth( m_dataModel->GetColsCount() - 1, 50 );

    m_grid->AutoSizeColumns( false );

    m_grid->SetGridCursor( 0, 1 );
    m_grid->SetFocus();

    m_sdbSizer1OK->SetDefault();

    FinishDialogSettings();
    SetSizeInDU( 600, 300 );
    Center();

    // Connect Events
    m_grid->Connect( wxEVT_GRID_COL_SORT, wxGridEventHandler( DIALOG_FIELDS_EDITOR_GLOBAL::OnColSort ), NULL, this );
}


DIALOG_FIELDS_EDITOR_GLOBAL::~DIALOG_FIELDS_EDITOR_GLOBAL()
{
    // Disconnect Events
    m_grid->Disconnect( wxEVT_GRID_COL_SORT, wxGridEventHandler( DIALOG_FIELDS_EDITOR_GLOBAL::OnColSort ), NULL, this );

    // Delete the GRID_TRICKS.
    m_grid->PopEventHandler( true );

    // we gave ownership of m_dataModel to the wxGrid...
}


bool DIALOG_FIELDS_EDITOR_GLOBAL::TransferDataFromWindow()
{
    // Commit any pending in-place edits and close the editor
    m_grid->DisableCellEditControl();

    if( !wxDialog::TransferDataFromWindow() )
        return false;

    SCH_SHEET_PATH currentSheet = m_parent->GetCurrentSheet();

    m_dataModel->ApplyData( m_parent );
    m_parent->OnModify();

    // Reset the view to where we left the user
    m_parent->SetCurrentSheet( currentSheet );
    m_parent->Refresh();

    return true;
}


void DIALOG_FIELDS_EDITOR_GLOBAL::AddField( const wxString& aFieldName,
                                            bool defaultShow, bool defaultSortBy )
{
    m_dataModel->AddColumn( aFieldName );

    wxVector<wxVariant> fieldsCtrlDataLine;

    fieldsCtrlDataLine.push_back( wxVariant( aFieldName ) );
    fieldsCtrlDataLine.push_back( wxVariant( defaultShow ) );
    fieldsCtrlDataLine.push_back( wxVariant( defaultSortBy ) );

    m_fieldsCtrl->AppendItem( fieldsCtrlDataLine );
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

    AddField( _( "Reference" ),   true,  true  );
    AddField( _( "Value" ),       true,  true  );
    AddField( _( "Footprint" ),   true,  true  );
    AddField( _( "Datasheet" ),   true,  false );

    for( auto fieldName : userFieldNames )
        AddField( fieldName,      true,  false );
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
        if( m_fieldsCtrl->GetToggleValue( row, col ) )
            m_grid->ShowCol( row );
        else
            m_grid->HideCol( row );     // grid's columns map to fieldsCtrl's rows
        break;

    case GROUP_BY_COLUMN:
        m_dataModel->RebuildRows( m_groupComponentsBox, m_fieldsCtrl );
        m_dataModel->Sort( m_grid->GetSortingColumn(), m_grid->IsSortOrderAscending() );
        m_grid->ForceRefresh();
        break;
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


void DIALOG_FIELDS_EDITOR_GLOBAL::OnTableItemContextMenu( wxGridEvent& event )
{
    /* TODO
     * - Option to select footprint if FOOTPRINT column selected
     */

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
    {
        wxCommandEvent dummyEvent;
        m_parent->OnSaveProject( dummyEvent );
    }
}
