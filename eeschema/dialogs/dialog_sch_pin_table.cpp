/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2020 KiCad Developers, see AUTHORS.txt for contributors.
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

#include "dialog_sch_pin_table.h"
#include <grid_tricks.h>
#include <pin_number.h>
#include <sch_edit_frame.h>
#include <confirm.h>
#include <lib_edit_frame.h>
#include <widgets/grid_icon_text_helpers.h>
#include <widgets/wx_grid.h>
#include <settings/settings_manager.h>
#include <widgets/grid_combobox.h>

class SCH_PIN_TABLE_DATA_MODEL : public wxGridTableBase, public std::vector<SCH_PIN>
{
protected:
    std::vector<wxGridCellAttr*> m_nameAttrs;
    wxGridCellAttr*              m_typeAttr;
    wxGridCellAttr*              m_shapeAttr;

public:
    SCH_PIN_TABLE_DATA_MODEL() :
            m_typeAttr( nullptr ),
            m_shapeAttr( nullptr )
    {
    }

    ~SCH_PIN_TABLE_DATA_MODEL()
    {
        for( wxGridCellAttr* attr : m_nameAttrs )
            attr->DecRef();

        m_typeAttr->DecRef();
        m_shapeAttr->DecRef();
    }

    void BuildAttrs()
    {
        for( const SCH_PIN& pin : *this )
        {
            wxArrayString choices;
            LIB_PIN*      lib_pin = pin.GetLibPin();

            choices.push_back( lib_pin->GetName() );

            for( const std::pair<const wxString, LIB_PIN::ALT>& alt : lib_pin->GetAlternates() )
                choices.push_back( alt.first );

            wxGridCellAttr* attr = new wxGridCellAttr();
            attr->SetEditor( new GRID_CELL_COMBOBOX( choices ) );

            m_nameAttrs.push_back( attr );
        }

        m_typeAttr = new wxGridCellAttr;
        m_typeAttr->SetRenderer( new GRID_CELL_ICON_TEXT_RENDERER( PinTypeIcons(), PinTypeNames() ) );
        m_typeAttr->SetReadOnly( true );

        m_shapeAttr = new wxGridCellAttr;
        m_shapeAttr->SetRenderer( new GRID_CELL_ICON_TEXT_RENDERER( PinShapeIcons(), PinShapeNames() ) );
        m_shapeAttr->SetReadOnly( true );
    }

    int GetNumberRows() override { return (int) size(); }
    int GetNumberCols() override { return COL_COUNT; }

    wxString GetColLabelValue( int aCol ) override
    {
        switch( aCol )
        {
        case COL_NUMBER: return _( "Number" );
        case COL_NAME:   return _( "Name" );
        case COL_TYPE:   return _( "Electrical Type" );
        case COL_SHAPE:  return _( "Graphic Style" );
        default:         wxFAIL; return wxEmptyString;
        }
    }

    bool IsEmptyCell( int row, int col ) override
    {
        return false;   // don't allow adjacent cell overflow, even if we are actually empty
    }

    wxString GetValue( int aRow, int aCol ) override
    {
        return GetValue( at( aRow ), aCol );
    }

    static wxString GetValue( const SCH_PIN& aPin, int aCol )
    {
        switch( aCol )
        {
        case COL_NUMBER: return aPin.GetNumber();
        case COL_NAME:   return aPin.GetName();
        case COL_TYPE:   return PinTypeNames()[static_cast<int>( aPin.GetType() )];
        case COL_SHAPE:  return PinShapeNames()[static_cast<int>( aPin.GetShape() )];
        default:         wxFAIL; return wxEmptyString;
        }
    }

    wxGridCellAttr* GetAttr( int aRow, int aCol, wxGridCellAttr::wxAttrKind  ) override
    {
        switch( aCol )
        {
        case COL_NAME:
            m_nameAttrs[ aRow ]->IncRef();
            return m_nameAttrs[ aRow ];

        case COL_NUMBER:
            return nullptr;

        case COL_TYPE:
            m_typeAttr->IncRef();
            return m_typeAttr;

        case COL_SHAPE:
            m_shapeAttr->IncRef();
            return m_shapeAttr;

        default:
            wxFAIL;
            return nullptr;
        }
    }

    void SetValue( int aRow, int aCol, const wxString &aValue ) override
    {
        switch( aCol )
        {
        case COL_NAME:
            if( aValue == at( aRow ).GetName() )
                at( aRow ).SetAlt( wxEmptyString );
            else
                at( aRow ).SetAlt( aValue );
            break;

        case COL_NUMBER:
        case COL_TYPE:
        case COL_SHAPE:
            // Read-only.
            break;

        default:
            wxFAIL;
            break;
        }
    }

    static bool compare( const SCH_PIN& lhs, const SCH_PIN& rhs, int sortCol, bool ascending )
    {
        wxString lhStr = GetValue( lhs, sortCol );
        wxString rhStr = GetValue( rhs, sortCol );

        if( lhStr == rhStr )
        {
            // Secondary sort key is always COL_NUMBER
            sortCol = COL_NUMBER;
            lhStr = GetValue( lhs, sortCol );
            rhStr = GetValue( rhs, sortCol );
        }

        bool res;

        // N.B. To meet the iterator sort conditions, we cannot simply invert the truth
        // to get the opposite sort.  i.e. ~(a<b) != (a>b)
        auto cmp = [ ascending ]( const auto a, const auto b )
                   {
                       if( ascending )
                           return a < b;
                       else
                           return b < a;
                   };

        switch( sortCol )
        {
        case COL_NUMBER:
        case COL_NAME:
            res = cmp( PinNumbers::Compare( lhStr, rhStr ), 0 );
            break;
        case COL_TYPE:
        case COL_SHAPE:
            res = cmp( lhStr.CmpNoCase( rhStr ), 0 );
            break;
        default:
            res = cmp( StrNumCmp( lhStr, rhStr ), 0 );
            break;
        }

        return res;
    }

    void SortRows( int aSortCol, bool ascending )
    {
        std::sort( begin(), end(),
                   [ aSortCol, ascending ]( const SCH_PIN& lhs, const SCH_PIN& rhs ) -> bool
                   {
                       return compare( lhs, rhs, aSortCol, ascending );
                   } );
    }
};


DIALOG_SCH_PIN_TABLE::DIALOG_SCH_PIN_TABLE( SCH_EDIT_FRAME* parent, SCH_COMPONENT* aComp ) :
        DIALOG_SCH_PIN_TABLE_BASE( parent ),
        m_editFrame( parent ),
        m_comp( aComp )
{
    m_dataModel = new SCH_PIN_TABLE_DATA_MODEL();

    // Make a copy of the pins for editing
    for( const std::unique_ptr<SCH_PIN>& pin : m_comp->GetRawPins() )
        m_dataModel->push_back( *pin );

    m_dataModel->SortRows( COL_NUMBER, true );
    m_dataModel->BuildAttrs();

    // Save original columns widths so we can do proportional sizing.
    for( int i = 0; i < COL_COUNT; ++i )
        m_originalColWidths[ i ] = m_grid->GetColSize( i );

    // Give a bit more room for combobox editors
    m_grid->SetDefaultRowSize( m_grid->GetDefaultRowSize() + 4 );

    m_grid->SetTable( m_dataModel );
    m_grid->PushEventHandler( new GRID_TRICKS( m_grid ) );

    GetSizer()->SetSizeHints(this);
    Centre();

    m_ButtonsOK->SetDefault();
    m_initialized = true;
    m_modified = false;
    m_width = 0;

    // Connect Events
    m_grid->Connect( wxEVT_GRID_COL_SORT, wxGridEventHandler( DIALOG_SCH_PIN_TABLE::OnColSort ), nullptr, this );
}


DIALOG_SCH_PIN_TABLE::~DIALOG_SCH_PIN_TABLE()
{
    // Disconnect Events
    m_grid->Disconnect( wxEVT_GRID_COL_SORT, wxGridEventHandler( DIALOG_SCH_PIN_TABLE::OnColSort ), nullptr, this );

    // Prevents crash bug in wxGrid's d'tor
    m_grid->DestroyTable( m_dataModel );

    // Delete the GRID_TRICKS.
    m_grid->PopEventHandler( true );
}


bool DIALOG_SCH_PIN_TABLE::TransferDataToWindow()
{
    return true;
}


bool DIALOG_SCH_PIN_TABLE::TransferDataFromWindow()
{
    if( !m_grid->CommitPendingChanges() )
        return false;

    // Update any assignments
    for( const SCH_PIN& model_pin : *m_dataModel )
    {
        // map from the edited copy back to the "real" pin in the component
        SCH_PIN* src_pin = m_comp->GetPin( model_pin.GetLibPin() );
        src_pin->SetAlt( model_pin.GetAlt() );
    }

    return true;
}


void DIALOG_SCH_PIN_TABLE::OnCellEdited( wxGridEvent& aEvent )
{
    int row = aEvent.GetRow();

    // These are just to get the cells refreshed
    m_dataModel->SetValue( row, COL_TYPE, m_dataModel->GetValue( row, COL_TYPE ) );
    m_dataModel->SetValue( row, COL_SHAPE, m_dataModel->GetValue( row, COL_SHAPE ) );

    m_modified = true;
}


void DIALOG_SCH_PIN_TABLE::OnColSort( wxGridEvent& aEvent )
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

    m_dataModel->SortRows( sortCol, ascending );
}


void DIALOG_SCH_PIN_TABLE::adjustGridColumns( int aWidth )
{
    m_width = aWidth;

    // Account for scroll bars
    aWidth -= ( m_grid->GetSize().x - m_grid->GetClientSize().x );

    wxGridUpdateLocker deferRepaintsTillLeavingScope;

    // The Number and Name columns must be at least wide enough to hold their contents, but
    // no less wide than their original widths.

    m_grid->AutoSizeColumn( COL_NUMBER );

    if( m_grid->GetColSize( COL_NUMBER ) < m_originalColWidths[ COL_NUMBER ] )
        m_grid->SetColSize( COL_NUMBER, m_originalColWidths[ COL_NUMBER ] );

    m_grid->AutoSizeColumn( COL_NAME );

    if( m_grid->GetColSize( COL_NAME ) < m_originalColWidths[ COL_NAME ] )
        m_grid->SetColSize( COL_NAME, m_originalColWidths[ COL_NAME ] );

    // If the grid is still wider than the columns, then stretch the Number and Name columns
    // to fit.

    for( int i = 0; i < COL_COUNT; ++i )
        aWidth -= m_grid->GetColSize( i );

    if( aWidth > 0 )
    {
        m_grid->SetColSize( COL_NUMBER, m_grid->GetColSize( COL_NUMBER ) + aWidth / 2 );
        m_grid->SetColSize( COL_NAME, m_grid->GetColSize( COL_NAME ) + aWidth / 2 );
    }
}


void DIALOG_SCH_PIN_TABLE::OnSize( wxSizeEvent& event )
{
    auto new_size = event.GetSize().GetX();

    if( m_initialized && m_width != new_size )
    {
        adjustGridColumns( new_size );
    }

    // Always propagate for a grid repaint (needed if the height changes, as well as width)
    event.Skip();
}


void DIALOG_SCH_PIN_TABLE::OnCancel( wxCommandEvent& event )
{
    Close();
}


void DIALOG_SCH_PIN_TABLE::OnClose( wxCloseEvent& event )
{
    // This is a cancel, so commit quietly as we're going to throw the results away anyway.
    m_grid->CommitPendingChanges( true );

    int retval = wxCANCEL;

    if( m_modified && !HandleUnsavedChanges( this, _( "Save changes?" ),
                                             [&]()->bool
                                             {
                                                 if( TransferDataFromWindow() )
                                                 {
                                                     retval = wxOK;
                                                     return true;
                                                 }

                                                 return false;
                                             } ) )
    {
        event.Veto();
        return;
    }

    if( IsQuasiModal() )
        EndQuasiModal( retval );
    else
        EndModal( retval );
}
