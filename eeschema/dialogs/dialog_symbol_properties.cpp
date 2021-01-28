/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2004-2021 KiCad Developers, see AUTHORS.txt for contributors.
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

#include "dialog_symbol_properties.h"

#include <wx/tooltip.h>
#include <grid_tricks.h>
#include <confirm.h>
#include <kiface_i.h>
#include <pin_number.h>
#include <kicad_string.h>
#include <menus_helpers.h>
#include <widgets/grid_icon_text_helpers.h>
#include <widgets/grid_combobox.h>
#include <widgets/wx_grid.h>
#include <settings/settings_manager.h>
#include <ee_collectors.h>
#include <class_library.h>
#include <fields_grid_table.h>
#include <sch_edit_frame.h>
#include <sch_reference_list.h>
#include <schematic.h>
#include <tool/tool_manager.h>
#include <tool/actions.h>

#ifdef KICAD_SPICE
#include <dialog_spice_model.h>
#endif /* KICAD_SPICE */


enum PIN_TABLE_COL_ORDER
{
    COL_NUMBER,
    COL_BASE_NAME,
    COL_ALT_NAME,
    COL_TYPE,
    COL_SHAPE,

    COL_COUNT       // keep as last
};


class SCH_PIN_TABLE_DATA_MODEL : public wxGridTableBase, public std::vector<SCH_PIN>
{
protected:
    std::vector<wxGridCellAttr*> m_nameAttrs;
    wxGridCellAttr*              m_readOnlyAttr;
    wxGridCellAttr*              m_typeAttr;
    wxGridCellAttr*              m_shapeAttr;

public:
    SCH_PIN_TABLE_DATA_MODEL() :
            m_readOnlyAttr( nullptr ),
            m_typeAttr( nullptr ),
            m_shapeAttr( nullptr )
    {
    }

    ~SCH_PIN_TABLE_DATA_MODEL()
    {
        for( wxGridCellAttr* attr : m_nameAttrs )
            attr->DecRef();

        m_readOnlyAttr->DecRef();
        m_typeAttr->DecRef();
        m_shapeAttr->DecRef();
    }

    void BuildAttrs()
    {
        m_readOnlyAttr = new wxGridCellAttr;
        m_readOnlyAttr->SetReadOnly( true );

        for( const SCH_PIN& pin : *this )
        {
            LIB_PIN*        lib_pin = pin.GetLibPin();
            wxGridCellAttr* attr = nullptr;

            if( lib_pin->GetAlternates().empty() )
            {
                attr = new wxGridCellAttr;
                attr->SetReadOnly( true );
            }
            else
            {
                wxArrayString choices;
                choices.push_back( lib_pin->GetName() );

                for( const std::pair<const wxString, LIB_PIN::ALT>& alt : lib_pin->GetAlternates() )
                    choices.push_back( alt.first );

                attr = new wxGridCellAttr();
                attr->SetEditor( new GRID_CELL_COMBOBOX( choices ) );
            }

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
        case COL_NUMBER:    return _( "Number" );
        case COL_BASE_NAME: return _( "Base Name" );
        case COL_ALT_NAME:  return _( "Alternate Assignment" );
        case COL_TYPE:      return _( "Electrical Type" );
        case COL_SHAPE:     return _( "Graphic Style" );
        default:   wxFAIL;  return wxEmptyString;
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
        case COL_NUMBER:    return aPin.GetNumber();
        case COL_BASE_NAME: return aPin.GetLibPin()->GetName();
        case COL_ALT_NAME:  return aPin.GetAlt();
        case COL_TYPE:      return PinTypeNames()[static_cast<int>( aPin.GetType() )];
        case COL_SHAPE:     return PinShapeNames()[static_cast<int>( aPin.GetShape() )];
        default:   wxFAIL;  return wxEmptyString;
        }
    }

    wxGridCellAttr* GetAttr( int aRow, int aCol, wxGridCellAttr::wxAttrKind  ) override
    {
        switch( aCol )
        {
        case COL_NUMBER:
        case COL_BASE_NAME:
            m_readOnlyAttr->IncRef();
            return m_readOnlyAttr;

        case COL_ALT_NAME:
            m_nameAttrs[ aRow ]->IncRef();
            return m_nameAttrs[ aRow ];

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
        case COL_ALT_NAME:
            if( aValue == at( aRow ).GetLibPin()->GetName() )
                at( aRow ).SetAlt( wxEmptyString );
            else
                at( aRow ).SetAlt( aValue );
            break;

        case COL_NUMBER:
        case COL_BASE_NAME:
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
        case COL_BASE_NAME:
        case COL_ALT_NAME:
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


DIALOG_SYMBOL_PROPERTIES::DIALOG_SYMBOL_PROPERTIES( SCH_EDIT_FRAME* aParent,
                                                    SCH_COMPONENT* aComponent ) :
        DIALOG_SYMBOL_PROPERTIES_BASE( aParent ),
        m_comp( nullptr ),
        m_part( nullptr ),
        m_fields( nullptr ),
        m_dataModel( nullptr )
{
    m_comp = aComponent;
    m_part = m_comp->GetPartRef().get();

    // GetPartRef() now points to the cached part in the schematic, which should always be
    // there for usual cases, but can be null when opening old schematics not storing the part
    // so we need to handle m_part == nullptr
    wxASSERT( m_part );

    m_fields = new FIELDS_GRID_TABLE<SCH_FIELD>( this, aParent, m_part );

    m_width = 0;
    m_delayedFocusRow = REFERENCE_FIELD;
    m_delayedFocusColumn = FDC_VALUE;
    m_delayedSelection = true;

#ifndef KICAD_SPICE
    m_spiceFieldsButton->Hide();
#endif /* not KICAD_SPICE */

    // disable some options inside the edit dialog which can cause problems while dragging
    if( m_comp->IsDragging() )
    {
        m_orientationLabel->Disable();
        m_orientationCtrl->Disable();
        m_mirrorLabel->Disable();
        m_mirrorCtrl->Disable();
    }

    // Give a bit more room for combobox editors
    m_fieldsGrid->SetDefaultRowSize( m_fieldsGrid->GetDefaultRowSize() + 4 );
    m_pinGrid->SetDefaultRowSize( m_pinGrid->GetDefaultRowSize() + 4 );

    m_fieldsGrid->SetTable( m_fields );
    m_fieldsGrid->PushEventHandler( new FIELDS_GRID_TRICKS( m_fieldsGrid, this ) );

    // Show/hide columns according to user's preference
    auto cfg = dynamic_cast<EESCHEMA_SETTINGS*>( Kiface().KifaceSettings() );

    if( cfg )
    {
        m_shownColumns = cfg->m_Appearance.edit_component_visible_columns;
        m_fieldsGrid->ShowHideColumns( m_shownColumns );
    }

    if( m_part && m_part->HasConversion() )
    {
        // DeMorgan conversions are a subclass of alternate pin assignments, so don't allow
        // free-form alternate assignments as well.  (We won't know how to map the alternates
        // back and forth when the conversion is changed.)
        m_pinTablePage->Disable();
        m_pinTablePage->SetToolTip(
                _( "Alternate pin assignments are not available for DeMorgan components." ) );
    }
    else
    {
        m_dataModel = new SCH_PIN_TABLE_DATA_MODEL();

        // Make a copy of the pins for editing
        for( const std::unique_ptr<SCH_PIN>& pin : m_comp->GetRawPins() )
            m_dataModel->push_back( *pin );

        m_dataModel->SortRows( COL_NUMBER, true );
        m_dataModel->BuildAttrs();

        m_pinGrid->SetTable( m_dataModel );
    }

    m_pinGrid->PushEventHandler( new GRID_TRICKS( m_pinGrid ) );

    wxToolTip::Enable( true );
    m_stdDialogButtonSizerOK->SetDefault();

    // Configure button logos
    m_bpAdd->SetBitmap( KiBitmap( small_plus_xpm ) );
    m_bpDelete->SetBitmap( KiBitmap( small_trash_xpm ) );
    m_bpMoveUp->SetBitmap( KiBitmap( small_up_xpm ) );
    m_bpMoveDown->SetBitmap( KiBitmap( small_down_xpm ) );

    // wxFormBuilder doesn't include this event...
    m_fieldsGrid->Connect( wxEVT_GRID_CELL_CHANGING,
                           wxGridEventHandler( DIALOG_SYMBOL_PROPERTIES::OnGridCellChanging ),
                           nullptr, this );

    m_pinGrid->Connect( wxEVT_GRID_COL_SORT,
                        wxGridEventHandler( DIALOG_SYMBOL_PROPERTIES::OnPinTableColSort ),
                        nullptr, this );

    finishDialogSettings();
}


DIALOG_SYMBOL_PROPERTIES::~DIALOG_SYMBOL_PROPERTIES()
{
    auto cfg = dynamic_cast<EESCHEMA_SETTINGS*>( Kiface().KifaceSettings() );

    if( cfg )
        cfg->m_Appearance.edit_component_visible_columns = m_fieldsGrid->GetShownColumns();

    // Prevents crash bug in wxGrid's d'tor
    m_fieldsGrid->DestroyTable( m_fields );

    if( m_dataModel )
        m_pinGrid->DestroyTable( m_dataModel );

    m_fieldsGrid->Disconnect( wxEVT_GRID_CELL_CHANGING,
                              wxGridEventHandler( DIALOG_SYMBOL_PROPERTIES::OnGridCellChanging ),
                              nullptr, this );

    m_pinGrid->Disconnect( wxEVT_GRID_COL_SORT,
                           wxGridEventHandler( DIALOG_SYMBOL_PROPERTIES::OnPinTableColSort ),
                           nullptr, this );

    // Delete the GRID_TRICKS.
    m_fieldsGrid->PopEventHandler( true );
    m_pinGrid->PopEventHandler( true );
}


SCH_EDIT_FRAME* DIALOG_SYMBOL_PROPERTIES::GetParent()
{
    return dynamic_cast<SCH_EDIT_FRAME*>( wxDialog::GetParent() );
}


bool DIALOG_SYMBOL_PROPERTIES::TransferDataToWindow()
{
    if( !wxDialog::TransferDataToWindow() )
        return false;

    std::set<wxString> defined;

    // Push a copy of each field into m_updateFields
    for( int i = 0; i < m_comp->GetFieldCount(); ++i )
    {
        SCH_FIELD field( *m_comp->GetField( i ) );

        // change offset to be symbol-relative
        field.Offset( -m_comp->GetPosition() );

        defined.insert( field.GetName() );
        m_fields->push_back( field );
    }

    // Add in any template fieldnames not yet defined:
    for( const TEMPLATE_FIELDNAME& templateFieldname :
            GetParent()->Schematic().Settings().m_TemplateFieldNames.GetTemplateFieldNames() )
    {
        if( defined.count( templateFieldname.m_Name ) <= 0 )
        {
            SCH_FIELD field( wxPoint( 0, 0 ), -1, m_comp, templateFieldname.m_Name );
            field.SetVisible( templateFieldname.m_Visible );
            m_fields->push_back( field );
        }
    }

    // notify the grid
    wxGridTableMessage msg( m_fields, wxGRIDTABLE_NOTIFY_ROWS_APPENDED, m_fields->size() );
    m_fieldsGrid->ProcessTableMessage( msg );
    AdjustGridColumns( m_fieldsGrid->GetRect().GetWidth() );

    // If a multi-unit component, set up the unit selector and interchangeable checkbox.
    if( m_comp->GetUnitCount() > 1 )
    {
        for( int ii = 1; ii <= m_comp->GetUnitCount(); ii++ )
            m_unitChoice->Append( LIB_PART::SubReference( ii, false ) );

        if( m_comp->GetUnit() <= ( int )m_unitChoice->GetCount() )
            m_unitChoice->SetSelection( m_comp->GetUnit() - 1 );
    }
    else
    {
        m_unitLabel->Enable( false );
        m_unitChoice->Enable( false );
    }

    if( m_part && m_part->HasConversion() )
    {
        if( m_comp->GetConvert() > LIB_ITEM::LIB_CONVERT::BASE )
            m_cbAlternateSymbol->SetValue( true );
    }
    else
    {
        m_cbAlternateSymbol->Enable( false );
    }

    // Set the symbol orientation and mirroring.
    int orientation = m_comp->GetOrientation() & ~( CMP_MIRROR_X | CMP_MIRROR_Y );

    switch( orientation )
    {
    default:
    case CMP_ORIENT_0:   m_orientationCtrl->SetSelection( 0 ); break;
    case CMP_ORIENT_90:  m_orientationCtrl->SetSelection( 1 ); break;
    case CMP_ORIENT_270: m_orientationCtrl->SetSelection( 2 ); break;
    case CMP_ORIENT_180: m_orientationCtrl->SetSelection( 3 ); break;
    }

    int mirror = m_comp->GetOrientation() & ( CMP_MIRROR_X | CMP_MIRROR_Y );

    switch( mirror )
    {
    default:           m_mirrorCtrl->SetSelection( 0 ) ; break;
    case CMP_MIRROR_X: m_mirrorCtrl->SetSelection( 1 ); break;
    case CMP_MIRROR_Y: m_mirrorCtrl->SetSelection( 2 ); break;
    }

    m_cbExcludeFromBom->SetValue( !m_comp->GetIncludeInBom() );
    m_cbExcludeFromBoard->SetValue( !m_comp->GetIncludeOnBoard() );

    if( m_part )
    {
        m_ShowPinNumButt->SetValue( m_part->ShowPinNumbers() );
        m_ShowPinNameButt->SetValue( m_part->ShowPinNames() );
    }

    // Set the component's library name.
    m_tcLibraryID->SetLabelText( m_comp->GetLibId().Format() );

    Layout();

    return true;
}


void DIALOG_SYMBOL_PROPERTIES::OnEditSpiceModel( wxCommandEvent& event )
{
#ifdef KICAD_SPICE
    int diff = m_fields->size();

    DIALOG_SPICE_MODEL dialog( this, *m_comp, m_fields );

    if( dialog.ShowModal() != wxID_OK )
        return;

    diff = (int) m_fields->size() - diff;

    if( diff > 0 )
    {
        wxGridTableMessage msg( m_fields, wxGRIDTABLE_NOTIFY_ROWS_APPENDED, diff );
        m_fieldsGrid->ProcessTableMessage( msg );
    }
    else if( diff < 0 )
    {
        wxGridTableMessage msg( m_fields, wxGRIDTABLE_NOTIFY_ROWS_DELETED, 0, -diff );
        m_fieldsGrid->ProcessTableMessage( msg );
    }

    m_fieldsGrid->ForceRefresh();
#endif /* KICAD_SPICE */
}


void DIALOG_SYMBOL_PROPERTIES::OnCancelButtonClick( wxCommandEvent& event )
{
    // Running the Footprint Browser gums up the works and causes the automatic cancel
    // stuff to no longer work.  So we do it here ourselves.
    EndQuasiModal( wxID_CANCEL );
}


bool DIALOG_SYMBOL_PROPERTIES::Validate()
{
    wxString msg;
    LIB_ID   id;

    if( !m_fieldsGrid->CommitPendingChanges() || !m_fieldsGrid->Validate() )
        return false;

    if( !SCH_COMPONENT::IsReferenceStringValid( m_fields->at( REFERENCE_FIELD ).GetText() ) )
    {
        DisplayErrorMessage( this, _( "References must start with a letter." ) );

        m_delayedFocusColumn = FDC_VALUE;
        m_delayedFocusRow = REFERENCE_FIELD;
        m_delayedSelection = false;

        return false;
    }

    // Check for missing field names.
    for( size_t i = MANDATORY_FIELDS;  i < m_fields->size(); ++i )
    {
        SCH_FIELD& field = m_fields->at( i );
        wxString   fieldName = field.GetName( false );

        if( fieldName.IsEmpty() )
        {
            DisplayErrorMessage( this, _( "Fields must have a name." ) );

            m_delayedFocusColumn = FDC_NAME;
            m_delayedFocusRow = i;
            m_delayedSelection = false;

            return false;
        }
    }

    return true;
}


bool DIALOG_SYMBOL_PROPERTIES::TransferDataFromWindow()
{
    if( !wxDialog::TransferDataFromWindow() )  // Calls our Validate() method.
        return false;

    if( !m_fieldsGrid->CommitPendingChanges() )
        return false;

    if( !m_pinGrid->CommitPendingChanges() )
        return false;

    SCH_SCREEN* currentScreen = GetParent()->GetScreen();
    SCHEMATIC&  schematic = GetParent()->Schematic();

    wxCHECK( currentScreen, false );

    // This needs to be done before the LIB_ID is changed to prevent stale library symbols in
    // the schematic file.
    currentScreen->Remove( m_comp );

    wxString msg;

    // save old cmp in undo list if not already in edit, or moving ...
    if( m_comp->GetEditFlags() == 0 )
        GetParent()->SaveCopyInUndoList( currentScreen, m_comp, UNDO_REDO::CHANGED, false );

    // Save current flags which could be modified by next change settings
    STATUS_FLAGS flags = m_comp->GetFlags();

    // For symbols with multiple shapes (De Morgan representation) Set the selected shape:
    if( m_cbAlternateSymbol->IsEnabled() && m_cbAlternateSymbol->GetValue() )
        m_comp->SetConvert( LIB_ITEM::LIB_CONVERT::DEMORGAN );
    else
        m_comp->SetConvert( LIB_ITEM::LIB_CONVERT::BASE );

    //Set the part selection in multiple part per package
    int unit_selection = m_unitChoice->IsEnabled() ? m_unitChoice->GetSelection() + 1 : 1;
    m_comp->SetUnitSelection( &GetParent()->GetCurrentSheet(), unit_selection );
    m_comp->SetUnit( unit_selection );

    switch( m_orientationCtrl->GetSelection() )
    {
    case 0: m_comp->SetOrientation( CMP_ORIENT_0 );   break;
    case 1: m_comp->SetOrientation( CMP_ORIENT_90 );  break;
    case 2: m_comp->SetOrientation( CMP_ORIENT_270 ); break;
    case 3: m_comp->SetOrientation( CMP_ORIENT_180 ); break;
    }

    switch( m_mirrorCtrl->GetSelection() )
    {
    case 0:                                        break;
    case 1: m_comp->SetOrientation( CMP_MIRROR_X ); break;
    case 2: m_comp->SetOrientation( CMP_MIRROR_Y ); break;
    }

    if( m_part )
    {
        m_part->SetShowPinNames( m_ShowPinNameButt->GetValue() );
        m_part->SetShowPinNumbers( m_ShowPinNumButt->GetValue() );
    }

    // Restore m_Flag modified by SetUnit() and other change settings
    m_comp->ClearFlags();
    m_comp->SetFlags( flags );

    // change all field positions from relative to absolute
    for( unsigned i = 0;  i < m_fields->size();  ++i )
        m_fields->at( i ).Offset( m_comp->GetPosition() );

    LIB_PART* entry = GetParent()->GetLibPart( m_comp->GetLibId() );

    if( entry && entry->IsPower() )
        m_fields->at( VALUE_FIELD ).SetText( m_comp->GetLibId().GetLibItemName() );

    // Push all fields to the component -except- for those which are TEMPLATE_FIELDNAMES
    // with empty values.
    SCH_FIELDS& fields = m_comp->GetFields();

    fields.clear();

    for( size_t i = 0; i < m_fields->size(); ++i )
    {
        SCH_FIELD& field = m_fields->at( i );
        bool       emptyTemplateField = false;

        if( i >= MANDATORY_FIELDS )
        {
            for( const TEMPLATE_FIELDNAME& fieldname :
                    schematic.Settings().m_TemplateFieldNames.GetTemplateFieldNames() )
            {
                if( field.GetName() == fieldname.m_Name && field.GetText().IsEmpty() )
                {
                    emptyTemplateField = true;
                    break;
                }
            }
        }

        if( !emptyTemplateField )
            fields.push_back( field );
    }

    // Reference has a specific initialization, depending on the current active sheet
    // because for a given component, in a complex hierarchy, there are more than one
    // reference.
    m_comp->SetRef( &GetParent()->GetCurrentSheet(), m_fields->at( REFERENCE_FIELD ).GetText() );

    // Similar for Value and Footprint, except that the GUI behaviour is that they are kept
    // in sync between multiple instances.
    m_comp->SetValue( m_fields->at( VALUE_FIELD ).GetText() );
    m_comp->SetFootprint( m_fields->at( FOOTPRINT_FIELD ).GetText() );

    m_comp->SetIncludeInBom( !m_cbExcludeFromBom->IsChecked() );
    m_comp->SetIncludeOnBoard( !m_cbExcludeFromBoard->IsChecked() );

    // The value, footprint and datasheet fields and exclude from bill of materials setting
    // should be kept in sync in multi-unit parts.
    if( m_comp->GetUnitCount() > 1 && m_comp->IsAnnotated( &GetParent()->GetCurrentSheet() ) )
    {
        wxString ref = m_comp->GetRef( &GetParent()->GetCurrentSheet() );
        int      unit = m_comp->GetUnit();

        for( SCH_SHEET_PATH& sheet : GetParent()->Schematic().GetSheets() )
        {
            SCH_SCREEN*                 screen = sheet.LastScreen();
            std::vector<SCH_COMPONENT*> otherUnits;
            constexpr bool              appendUndo = true;

            CollectOtherUnits( ref, unit, sheet, &otherUnits );

            for( SCH_COMPONENT* otherUnit : otherUnits )
            {
                GetParent()->SaveCopyInUndoList( screen, otherUnit, UNDO_REDO::CHANGED,
                                                 appendUndo );
                otherUnit->SetValue( m_fields->at( VALUE_FIELD ).GetText() );
                otherUnit->SetFootprint( m_fields->at( FOOTPRINT_FIELD ).GetText() );
                otherUnit->GetField( DATASHEET_FIELD )->SetText( m_fields->at( DATASHEET_FIELD ).GetText() );
                otherUnit->SetIncludeInBom( !m_cbExcludeFromBom->IsChecked() );
                otherUnit->SetIncludeOnBoard( !m_cbExcludeFromBoard->IsChecked() );
                GetParent()->UpdateItem( otherUnit );
            }
        }
    }

    // Update any assignments
    if( m_dataModel )
    {
        for( const SCH_PIN& model_pin : *m_dataModel )
        {
            // map from the edited copy back to the "real" pin in the component
            SCH_PIN* src_pin = m_comp->GetPin( model_pin.GetLibPin() );
            src_pin->SetAlt( model_pin.GetAlt() );
        }
    }

    currentScreen->Append( m_comp );
    GetParent()->TestDanglingEnds();
    GetParent()->UpdateItem( m_comp );
    GetParent()->OnModify();

    // This must go after OnModify() so that the connectivity graph will have been updated.
    GetParent()->GetToolManager()->PostEvent( EVENTS::SelectedItemsModified );

    return true;
}


void DIALOG_SYMBOL_PROPERTIES::OnGridCellChanging( wxGridEvent& event )
{
    wxGridCellEditor* editor = m_fieldsGrid->GetCellEditor( event.GetRow(), event.GetCol() );
    wxControl* control = editor->GetControl();

    if( control && control->GetValidator() && !control->GetValidator()->Validate( control ) )
    {
        event.Veto();
        m_delayedFocusRow = event.GetRow();
        m_delayedFocusColumn = event.GetCol();
        m_delayedSelection = false;
    }
    else if( event.GetCol() == FDC_NAME )
    {
        wxString newName = event.GetString();

        for( int i = 0; i < m_fieldsGrid->GetNumberRows(); ++i )
        {
            if( i == event.GetRow() )
                continue;

            if( newName.CmpNoCase( m_fieldsGrid->GetCellValue( i, FDC_NAME ) ) == 0 )
            {
                DisplayError( this, wxString::Format( _( "The name '%s' is already in use." ),
                                                      newName ) );
                event.Veto();
                m_delayedFocusRow = event.GetRow();
                m_delayedFocusColumn = event.GetCol();
                m_delayedSelection = false;
            }
        }
    }

    editor->DecRef();
}


void DIALOG_SYMBOL_PROPERTIES::OnGridEditorShown( wxGridEvent& aEvent )
{
    if( aEvent.GetRow() == REFERENCE_FIELD && aEvent.GetCol() == FDC_VALUE )
        m_delayedSelection= true;
}


void DIALOG_SYMBOL_PROPERTIES::OnAddField( wxCommandEvent& event )
{
    if( !m_fieldsGrid->CommitPendingChanges() )
        return;

    SCHEMATIC_SETTINGS& settings = m_comp->Schematic()->Settings();
    int                 fieldID = m_fields->size();
    SCH_FIELD           newField( wxPoint( 0, 0 ), fieldID, m_comp,
                                  TEMPLATE_FIELDNAME::GetDefaultFieldName( fieldID ) );

    newField.SetTextAngle( m_fields->at( REFERENCE_FIELD ).GetTextAngle() );
    newField.SetTextSize( wxSize( settings.m_DefaultTextSize, settings.m_DefaultTextSize ) );

    m_fields->push_back( newField );

    // notify the grid
    wxGridTableMessage msg( m_fields, wxGRIDTABLE_NOTIFY_ROWS_APPENDED, 1 );
    m_fieldsGrid->ProcessTableMessage( msg );

    m_fieldsGrid->MakeCellVisible( (int) m_fields->size() - 1, 0 );
    m_fieldsGrid->SetGridCursor( (int) m_fields->size() - 1, 0 );

    m_fieldsGrid->EnableCellEditControl();
    m_fieldsGrid->ShowCellEditControl();
}


void DIALOG_SYMBOL_PROPERTIES::OnDeleteField( wxCommandEvent& event )
{
    int curRow = m_fieldsGrid->GetGridCursorRow();

    if( curRow < 0 )
    {
        return;
    }
    else if( curRow < MANDATORY_FIELDS )
    {
        DisplayError( this, wxString::Format( _( "The first %d fields are mandatory." ),
                                              MANDATORY_FIELDS ) );
        return;
    }

    m_fieldsGrid->CommitPendingChanges( true /* quiet mode */ );

    m_fields->erase( m_fields->begin() + curRow );

    // notify the grid
    wxGridTableMessage msg( m_fields, wxGRIDTABLE_NOTIFY_ROWS_DELETED, curRow, 1 );
    m_fieldsGrid->ProcessTableMessage( msg );

    if( m_fieldsGrid->GetNumberRows() > 0 )
    {
        m_fieldsGrid->MakeCellVisible( std::max( 0, curRow-1 ), m_fieldsGrid->GetGridCursorCol() );
        m_fieldsGrid->SetGridCursor( std::max( 0, curRow-1 ), m_fieldsGrid->GetGridCursorCol() );
    }
}


void DIALOG_SYMBOL_PROPERTIES::OnMoveUp( wxCommandEvent& event )
{
    if( !m_fieldsGrid->CommitPendingChanges() )
        return;

    int i = m_fieldsGrid->GetGridCursorRow();

    if( i > MANDATORY_FIELDS )
    {
        SCH_FIELD tmp = m_fields->at( (unsigned) i );
        m_fields->erase( m_fields->begin() + i, m_fields->begin() + i + 1 );
        m_fields->insert( m_fields->begin() + i - 1, tmp );
        m_fieldsGrid->ForceRefresh();

        m_fieldsGrid->SetGridCursor( i - 1, m_fieldsGrid->GetGridCursorCol() );
        m_fieldsGrid->MakeCellVisible( m_fieldsGrid->GetGridCursorRow(), m_fieldsGrid->GetGridCursorCol() );
    }
    else
    {
        wxBell();
    }
}


void DIALOG_SYMBOL_PROPERTIES::OnMoveDown( wxCommandEvent& event )
{
    if( !m_fieldsGrid->CommitPendingChanges() )
        return;

    int i = m_fieldsGrid->GetGridCursorRow();

    if( i >= MANDATORY_FIELDS && i < m_fieldsGrid->GetNumberRows() - 1 )
    {
        SCH_FIELD tmp = m_fields->at( (unsigned) i );
        m_fields->erase( m_fields->begin() + i, m_fields->begin() + i + 1 );
        m_fields->insert( m_fields->begin() + i + 1, tmp );
        m_fieldsGrid->ForceRefresh();

        m_fieldsGrid->SetGridCursor( i + 1, m_fieldsGrid->GetGridCursorCol() );
        m_fieldsGrid->MakeCellVisible( m_fieldsGrid->GetGridCursorRow(), m_fieldsGrid->GetGridCursorCol() );
    }
    else
    {
        wxBell();
    }
}


void DIALOG_SYMBOL_PROPERTIES::OnEditSymbol( wxCommandEvent&  )
{
    EndQuasiModal( SYMBOL_PROPS_EDIT_SCHEMATIC_SYMBOL );
}


void DIALOG_SYMBOL_PROPERTIES::OnEditLibrarySymbol( wxCommandEvent&  )
{
    EndQuasiModal( SYMBOL_PROPS_EDIT_LIBRARY_SYMBOL );
}


void DIALOG_SYMBOL_PROPERTIES::OnUpdateSymbol( wxCommandEvent&  )
{
    EndQuasiModal( SYMBOL_PROPS_WANT_UPDATE_SYMBOL );
}


void DIALOG_SYMBOL_PROPERTIES::OnExchangeSymbol( wxCommandEvent&  )
{
    EndQuasiModal( SYMBOL_PROPS_WANT_EXCHANGE_SYMBOL );
}


void DIALOG_SYMBOL_PROPERTIES::OnPinTableCellEdited( wxGridEvent& aEvent )
{
    int row = aEvent.GetRow();

    if( m_pinGrid->GetCellValue( row, COL_ALT_NAME ) == m_dataModel->GetValue( row, COL_BASE_NAME ) )
        m_dataModel->SetValue( row, COL_ALT_NAME, wxEmptyString );

    // These are just to get the cells refreshed
    m_dataModel->SetValue( row, COL_TYPE, m_dataModel->GetValue( row, COL_TYPE ) );
    m_dataModel->SetValue( row, COL_SHAPE, m_dataModel->GetValue( row, COL_SHAPE ) );

    m_modified = true;
}


void DIALOG_SYMBOL_PROPERTIES::OnPinTableColSort( wxGridEvent& aEvent )
{
    int sortCol = aEvent.GetCol();
    bool ascending;

    // This is bonkers, but wxWidgets doesn't tell us ascending/descending in the
    // event, and if we ask it will give us pre-event info.
    if( m_pinGrid->IsSortingBy( sortCol ) )
        // same column; invert ascending
        ascending = !m_pinGrid->IsSortOrderAscending();
    else
        // different column; start with ascending
        ascending = true;

    m_dataModel->SortRows( sortCol, ascending );
}


void DIALOG_SYMBOL_PROPERTIES::AdjustGridColumns( int aWidth )
{
    wxGridUpdateLocker deferRepaintsTillLeavingScope;

    m_width = aWidth;

    // Account for scroll bars
    int fieldsWidth = aWidth - ( m_fieldsGrid->GetSize().x - m_fieldsGrid->GetClientSize().x );
    int pinTblWidth = aWidth - ( m_pinGrid->GetSize().x - m_pinGrid->GetClientSize().x );

    m_fieldsGrid->AutoSizeColumn( 0 );

    int fixedColsWidth = m_fieldsGrid->GetColSize( 0 );

    for( int i = 2; i < m_fieldsGrid->GetNumberCols(); i++ )
        fixedColsWidth += m_fieldsGrid->GetColSize( i );

    m_fieldsGrid->SetColSize( 1, fieldsWidth - fixedColsWidth );

    // Stretch the Base Name and Alternate Assignment columns to fit.
    for( int i = 0; i < COL_COUNT; ++i )
    {
        if( i != COL_BASE_NAME && i != COL_ALT_NAME )
            pinTblWidth -= m_pinGrid->GetColSize( i );
    }

    // Why?  I haven't a clue....
    pinTblWidth += 22;

    m_pinGrid->SetColSize( COL_BASE_NAME, pinTblWidth / 2 );
    m_pinGrid->SetColSize( COL_ALT_NAME, pinTblWidth / 2 );
}


void DIALOG_SYMBOL_PROPERTIES::OnUpdateUI( wxUpdateUIEvent& event )
{
    wxString shownColumns = m_fieldsGrid->GetShownColumns();

    if( shownColumns != m_shownColumns )
    {
        m_shownColumns = shownColumns;

        if( !m_fieldsGrid->IsCellEditControlShown() )
            AdjustGridColumns( m_fieldsGrid->GetRect().GetWidth() );
    }

    // Handle a delayed focus
    if( m_delayedFocusRow >= 0 )
    {
        m_fieldsGrid->SetFocus();
        m_fieldsGrid->MakeCellVisible( m_delayedFocusRow, m_delayedFocusColumn );
        m_fieldsGrid->SetGridCursor( m_delayedFocusRow, m_delayedFocusColumn );

        m_fieldsGrid->EnableCellEditControl( true );
        m_fieldsGrid->ShowCellEditControl();

        m_delayedFocusRow = -1;
        m_delayedFocusColumn = -1;
    }

    // Handle a delayed selection
    if( m_delayedSelection )
    {
        wxGridCellEditor* cellEditor = m_fieldsGrid->GetCellEditor( REFERENCE_FIELD, FDC_VALUE );

        if( wxTextEntry* txt = dynamic_cast<wxTextEntry*>( cellEditor->GetControl() ) )
            KIUI::SelectReferenceNumber( txt );

        cellEditor->DecRef();   // we're done; must release

        m_delayedSelection = false;
    }
}


void DIALOG_SYMBOL_PROPERTIES::OnSizeGrid( wxSizeEvent& event )
{
    auto new_size = event.GetSize().GetX();

    if( m_width != new_size )
    {
        AdjustGridColumns( new_size );
    }

    // Always propagate for a grid repaint (needed if the height changes, as well as width)
    event.Skip();
}


void DIALOG_SYMBOL_PROPERTIES::OnInitDlg( wxInitDialogEvent& event )
{
    TransferDataToWindow();

    // Now all widgets have the size fixed, call FinishDialogSettings
    finishDialogSettings();
}
