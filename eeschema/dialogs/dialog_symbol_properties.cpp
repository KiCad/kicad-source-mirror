/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2004-2023 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <memory>

#include <bitmaps.h>
#include <wx/tooltip.h>
#include <grid_tricks.h>
#include <confirm.h>
#include <kiface_base.h>
#include <pin_numbers.h>
#include <string_utils.h>
#include <kiplatform/ui.h>
#include <widgets/grid_icon_text_helpers.h>
#include <widgets/grid_combobox.h>
#include <widgets/std_bitmap_button.h>
#include <settings/settings_manager.h>
#include <ee_collectors.h>
#include <symbol_library.h>
#include <fields_grid_table.h>
#include <sch_edit_frame.h>
#include <sch_reference_list.h>
#include <schematic.h>
#include <sch_commit.h>
#include <tool/tool_manager.h>
#include <tool/actions.h>

#include <dialog_sim_model.h>


wxDEFINE_EVENT( SYMBOL_DELAY_FOCUS, wxCommandEvent );
wxDEFINE_EVENT( SYMBOL_DELAY_SELECTION, wxCommandEvent );

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
        for( wxGridCellAttr* attr : m_nameAttrs )
            attr->DecRef();

        m_nameAttrs.clear();

        if( m_readOnlyAttr )
            m_readOnlyAttr->DecRef();

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

        if( m_typeAttr )
            m_typeAttr->DecRef();

        m_typeAttr = new wxGridCellAttr;
        m_typeAttr->SetRenderer( new GRID_CELL_ICON_TEXT_RENDERER( PinTypeIcons(),
                                                                   PinTypeNames() ) );
        m_typeAttr->SetReadOnly( true );

        if( m_shapeAttr )
            m_shapeAttr->DecRef();

        m_shapeAttr = new wxGridCellAttr;
        m_shapeAttr->SetRenderer( new GRID_CELL_ICON_TEXT_RENDERER( PinShapeIcons(),
                                                                    PinShapeNames() ) );
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
            res = cmp( PIN_NUMBERS::Compare( lhStr, rhStr ), 0 );
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
                                                    SCH_SYMBOL* aSymbol ) :
        DIALOG_SYMBOL_PROPERTIES_BASE( aParent ),
        m_symbol( nullptr ),
        m_part( nullptr ),
        m_fieldsSize( 0, 0 ),
        m_lastRequestedSize( 0, 0 ),
        m_editorShown( false ),
        m_fields( nullptr ),
        m_dataModel( nullptr )
{
    m_symbol = aSymbol;
    m_part = m_symbol->GetLibSymbolRef().get();

    // GetLibSymbolRef() now points to the cached part in the schematic, which should always be
    // there for usual cases, but can be null when opening old schematics not storing the part
    // so we need to handle m_part == nullptr
    // wxASSERT( m_part );

    m_fields = new FIELDS_GRID_TABLE<SCH_FIELD>( this, aParent, m_fieldsGrid, m_symbol );

    // Give a bit more room for combobox editors
    m_fieldsGrid->SetDefaultRowSize( m_fieldsGrid->GetDefaultRowSize() + 4 );
    m_pinGrid->SetDefaultRowSize( m_pinGrid->GetDefaultRowSize() + 4 );

    m_fieldsGrid->SetTable( m_fields );
    m_fieldsGrid->PushEventHandler( new FIELDS_GRID_TRICKS( m_fieldsGrid, this,
                                                            [&]( wxCommandEvent& aEvent )
                                                            {
                                                                OnAddField( aEvent );
                                                            } ) );
    m_fieldsGrid->SetSelectionMode( wxGrid::wxGridSelectRows );

    // Show/hide columns according to user's preference
    if( EESCHEMA_SETTINGS* cfg = dynamic_cast<EESCHEMA_SETTINGS*>( Kiface().KifaceSettings() ) )
    {
        m_fieldsGrid->ShowHideColumns( cfg->m_Appearance.edit_symbol_visible_columns );
        m_shownColumns = m_fieldsGrid->GetShownColumns();
    }

    if( m_part && m_part->HasConversion() )
    {
        // DeMorgan conversions are a subclass of alternate pin assignments, so don't allow
        // free-form alternate assignments as well.  (We won't know how to map the alternates
        // back and forth when the conversion is changed.)
        m_pinTablePage->Disable();
        m_pinTablePage->SetToolTip( _( "Alternate pin assignments are not available for De Morgan "
                                       "symbols." ) );
    }
    else
    {
        m_dataModel = new SCH_PIN_TABLE_DATA_MODEL();

        // Make a copy of the pins for editing
        for( const std::unique_ptr<SCH_PIN>& pin : m_symbol->GetRawPins() )
            m_dataModel->push_back( *pin );

        m_dataModel->SortRows( COL_NUMBER, true );
        m_dataModel->BuildAttrs();

        m_pinGrid->SetTable( m_dataModel );
    }

    if( m_part && m_part->IsPower() )
        m_spiceFieldsButton->Hide();

    m_pinGrid->PushEventHandler( new GRID_TRICKS( m_pinGrid ) );
    m_pinGrid->SetSelectionMode( wxGrid::wxGridSelectRows );

    m_tcLibraryID->SetBackgroundColour( KIPLATFORM::UI::GetDialogBGColour() );

    wxToolTip::Enable( true );
    SetupStandardButtons();

    // Configure button logos
    m_bpAdd->SetBitmap( KiBitmap( BITMAPS::small_plus ) );
    m_bpDelete->SetBitmap( KiBitmap( BITMAPS::small_trash ) );
    m_bpMoveUp->SetBitmap( KiBitmap( BITMAPS::small_up ) );
    m_bpMoveDown->SetBitmap( KiBitmap( BITMAPS::small_down ) );

    // wxFormBuilder doesn't include this event...
    m_fieldsGrid->Bind( wxEVT_GRID_CELL_CHANGING, &DIALOG_SYMBOL_PROPERTIES::OnGridCellChanging,
                        this );
    m_pinGrid->Bind( wxEVT_GRID_COL_SORT, &DIALOG_SYMBOL_PROPERTIES::OnPinTableColSort, this );
    Bind( SYMBOL_DELAY_FOCUS, &DIALOG_SYMBOL_PROPERTIES::HandleDelayedFocus, this );
    Bind( SYMBOL_DELAY_SELECTION, &DIALOG_SYMBOL_PROPERTIES::HandleDelayedSelection, this );

    QueueEvent( new wxCommandEvent( SYMBOL_DELAY_SELECTION ) );
    wxCommandEvent *evt = new wxCommandEvent( SYMBOL_DELAY_FOCUS );
    evt->SetClientData( new VECTOR2I( REFERENCE_FIELD, FDC_VALUE ) );
    QueueEvent( evt );

    finishDialogSettings();
}


DIALOG_SYMBOL_PROPERTIES::~DIALOG_SYMBOL_PROPERTIES()
{
    if( EESCHEMA_SETTINGS* cfg = dynamic_cast<EESCHEMA_SETTINGS*>( Kiface().KifaceSettings() ) )
    {
        cfg->m_Appearance.edit_symbol_visible_columns = m_fieldsGrid->GetShownColumnsAsString();
        cfg->m_Appearance.edit_symbol_width = GetSize().x;
        cfg->m_Appearance.edit_symbol_height = GetSize().y;
    }

    // Prevents crash bug in wxGrid's d'tor
    m_fieldsGrid->DestroyTable( m_fields );

    if( m_dataModel )
        m_pinGrid->DestroyTable( m_dataModel );

    m_fieldsGrid->Unbind( wxEVT_GRID_CELL_CHANGING, &DIALOG_SYMBOL_PROPERTIES::OnGridCellChanging,
                          this );
    m_pinGrid->Unbind( wxEVT_GRID_COL_SORT, &DIALOG_SYMBOL_PROPERTIES::OnPinTableColSort, this );
    Unbind( SYMBOL_DELAY_FOCUS, &DIALOG_SYMBOL_PROPERTIES::HandleDelayedFocus, this );
    Unbind( SYMBOL_DELAY_SELECTION, &DIALOG_SYMBOL_PROPERTIES::HandleDelayedSelection, this );

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
    for( int i = 0; i < m_symbol->GetFieldCount(); ++i )
    {
        SCH_FIELD field( m_symbol->GetFields()[i] );

        // change offset to be symbol-relative
        field.Offset( -m_symbol->GetPosition() );

        defined.insert( field.GetName() );
        m_fields->push_back( field );
    }

    // Add in any template fieldnames not yet defined:
    for( const TEMPLATE_FIELDNAME& templateFieldname :
            GetParent()->Schematic().Settings().m_TemplateFieldNames.GetTemplateFieldNames() )
    {
        if( defined.count( templateFieldname.m_Name ) <= 0 )
        {
            SCH_FIELD field( VECTOR2I( 0, 0 ), -1, m_symbol, templateFieldname.m_Name );
            field.SetVisible( templateFieldname.m_Visible );
            m_fields->push_back( field );
        }
    }

    // notify the grid
    wxGridTableMessage msg( m_fields, wxGRIDTABLE_NOTIFY_ROWS_APPENDED, m_fields->size() );
    m_fieldsGrid->ProcessTableMessage( msg );
    AdjustFieldsGridColumns();

    // If a multi-unit symbol, set up the unit selector and interchangeable checkbox.
    if( m_symbol->GetUnitCount() > 1 )
    {
        // Ensure symbol unit is the currently selected unit (mandatory in complex hierarchies)
        // from the current sheet path, because it can be modified by previous calculations
        m_symbol->UpdateUnit( m_symbol->GetUnitSelection( &GetParent()->GetCurrentSheet() ) );

        for( int ii = 1; ii <= m_symbol->GetUnitCount(); ii++ )
        {
            if( m_symbol->HasUnitDisplayName( ii ) )
                m_unitChoice->Append( m_symbol->GetUnitDisplayName( ii ) );
            else
                m_unitChoice->Append( LIB_SYMBOL::SubReference( ii, false ) );
        }

        if( m_symbol->GetUnit() <= ( int )m_unitChoice->GetCount() )
            m_unitChoice->SetSelection( m_symbol->GetUnit() - 1 );
    }
    else
    {
        m_unitLabel->Enable( false );
        m_unitChoice->Enable( false );
    }

    if( m_part && m_part->HasConversion() )
    {
        if( m_symbol->GetConvert() > LIB_ITEM::LIB_CONVERT::BASE )
            m_cbAlternateSymbol->SetValue( true );
    }
    else
    {
        m_cbAlternateSymbol->Enable( false );
    }

    // Set the symbol orientation and mirroring.
    int orientation = m_symbol->GetOrientation() & ~( SYM_MIRROR_X | SYM_MIRROR_Y );

    switch( orientation )
    {
    default:
    case SYM_ORIENT_0:   m_orientationCtrl->SetSelection( 0 ); break;
    case SYM_ORIENT_90:  m_orientationCtrl->SetSelection( 1 ); break;
    case SYM_ORIENT_270: m_orientationCtrl->SetSelection( 2 ); break;
    case SYM_ORIENT_180: m_orientationCtrl->SetSelection( 3 ); break;
    }

    int mirror = m_symbol->GetOrientation() & ( SYM_MIRROR_X | SYM_MIRROR_Y );

    switch( mirror )
    {
    default:           m_mirrorCtrl->SetSelection( 0 ) ; break;
    case SYM_MIRROR_X: m_mirrorCtrl->SetSelection( 1 ); break;
    case SYM_MIRROR_Y: m_mirrorCtrl->SetSelection( 2 ); break;
    }

    m_cbExcludeFromSim->SetValue( m_symbol->GetFieldText( SIM_ENABLE_FIELD ) == wxS( "0" ) );
    m_cbExcludeFromBom->SetValue( m_symbol->GetExcludedFromBOM() );
    m_cbExcludeFromBoard->SetValue( m_symbol->GetExcludedFromBoard() );
    m_cbDNP->SetValue( m_symbol->GetDNP() );

    if( m_part )
    {
        m_ShowPinNumButt->SetValue( m_part->ShowPinNumbers() );
        m_ShowPinNameButt->SetValue( m_part->ShowPinNames() );
    }

    // Set the symbol's library name.
    m_tcLibraryID->SetValue( UnescapeString( m_symbol->GetLibId().Format() ) );

    Layout();
    m_fieldsGrid->Layout();
    wxSafeYield();

    return true;
}


void DIALOG_SYMBOL_PROPERTIES::OnExcludeFromSimulation( wxCommandEvent& event )
{
    int simEnableFieldRow = -1;

    for( int ii = MANDATORY_FIELDS; ii < m_fieldsGrid->GetNumberRows(); ++ii )
    {
        if( m_fieldsGrid->GetCellValue( ii, FDC_NAME ) == SIM_ENABLE_FIELD )
            simEnableFieldRow = ii;
    }

    if( event.IsChecked() )
    {
        if( simEnableFieldRow == -1 )
        {
            simEnableFieldRow = (int) m_fields->size();
            m_fields->emplace_back( VECTOR2I( 0, 0 ), simEnableFieldRow, m_symbol, SIM_ENABLE_FIELD );

            // notify the grid
            wxGridTableMessage msg( m_fields, wxGRIDTABLE_NOTIFY_ROWS_APPENDED, 1 );
            m_fieldsGrid->ProcessTableMessage( msg );
        }

        m_fieldsGrid->SetCellValue( simEnableFieldRow, FDC_VALUE, wxT( "0" ) );
        m_fieldsGrid->SetCellValue( simEnableFieldRow, FDC_SHOWN, wxT( "0" ) );
        m_fieldsGrid->SetCellValue( simEnableFieldRow, FDC_SHOW_NAME, wxT( "0" ) );
    }
    else if( simEnableFieldRow >= 0 )
    {
        m_fields->erase( m_fields->begin() + simEnableFieldRow );

        // notify the grid
        wxGridTableMessage msg( m_fields, wxGRIDTABLE_NOTIFY_ROWS_DELETED, simEnableFieldRow, 1 );
        m_fieldsGrid->ProcessTableMessage( msg );
    }

    OnModify();
}


void DIALOG_SYMBOL_PROPERTIES::OnEditSpiceModel( wxCommandEvent& event )
{
    if( !m_fieldsGrid->CommitPendingChanges() )
        return;

    std::vector<SCH_FIELD> fields;

    for( const SCH_FIELD& field : *m_fields )
        fields.emplace_back( field );

    DIALOG_SIM_MODEL dialog( this, *m_symbol, fields );

    if( dialog.ShowModal() != wxID_OK )
        return;

    // Add in any new fields
    for( const SCH_FIELD& editedField : fields )
    {
        bool found = false;

        for( SCH_FIELD& existingField : *m_fields )
        {
            if( existingField.GetName() == editedField.GetName() )
            {
                found = true;
                existingField.SetText( editedField.GetText() );
                break;
            }
        }

        if( !found )
        {
            m_fields->emplace_back( editedField );
            wxGridTableMessage msg( m_fields, wxGRIDTABLE_NOTIFY_ROWS_APPENDED, 1 );
            m_fieldsGrid->ProcessTableMessage( msg );
        }
    }

    // Remove any deleted fields
    for( int ii = (int) m_fields->size() - 1; ii >= 0; --ii )
    {
        SCH_FIELD& existingField = m_fields->at( ii );
        bool       found = false;

        for( SCH_FIELD& editedField : fields )
        {
            if( editedField.GetName() == existingField.GetName() )
            {
                found = true;
                break;
            }
        }

        if( !found )
        {
            m_fields->erase( m_fields->begin() + ii );
            wxGridTableMessage msg( m_fields, wxGRIDTABLE_NOTIFY_ROWS_DELETED, ii, 1 );
            m_fieldsGrid->ClearSelection();
            m_fieldsGrid->ProcessTableMessage( msg );
        }
    }

    OnModify();
    m_fieldsGrid->ForceRefresh();
}


void DIALOG_SYMBOL_PROPERTIES::OnCancelButtonClick( wxCommandEvent& event )
{
    // Running the Footprint Browser gums up the works and causes the automatic cancel
    // stuff to no longer work.  So we do it here ourselves.
    EndQuasiModal( wxID_CANCEL );
}


bool DIALOG_SYMBOL_PROPERTIES::Validate()
{
    LIB_ID   id;

    if( !m_fieldsGrid->CommitPendingChanges() || !m_fieldsGrid->Validate() )
        return false;

    if( !SCH_SYMBOL::IsReferenceStringValid( m_fields->at( REFERENCE_FIELD ).GetText() ) )
    {
        DisplayErrorMessage( this, _( "References must start with a letter." ) );

        wxCommandEvent *evt = new wxCommandEvent( SYMBOL_DELAY_FOCUS );
        evt->SetClientData( new VECTOR2I( REFERENCE_FIELD, FDC_VALUE ) );
        QueueEvent( evt );

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

            wxCommandEvent *evt = new wxCommandEvent( SYMBOL_DELAY_FOCUS );
            evt->SetClientData( new VECTOR2I( i, FDC_VALUE ) );
            QueueEvent( evt );

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

    SCH_COMMIT  commit( GetParent() );
    SCH_SCREEN* currentScreen = GetParent()->GetScreen();
    wxCHECK( currentScreen, false );

    // This needs to be done before the LIB_ID is changed to prevent stale library symbols in
    // the schematic file.
    currentScreen->Remove( m_symbol );

    // save old cmp in undo list if not already in edit, or moving ...
    if( m_symbol->GetEditFlags() == 0 )
        commit.Modify( m_symbol, currentScreen );

    // Save current flags which could be modified by next change settings
    EDA_ITEM_FLAGS flags = m_symbol->GetFlags();

    // For symbols with multiple shapes (De Morgan representation) Set the selected shape:
    if( m_cbAlternateSymbol->IsEnabled() && m_cbAlternateSymbol->GetValue() )
        m_symbol->SetConvert( LIB_ITEM::LIB_CONVERT::DEMORGAN );
    else
        m_symbol->SetConvert( LIB_ITEM::LIB_CONVERT::BASE );

    //Set the part selection in multiple part per package
    int unit_selection = m_unitChoice->IsEnabled() ? m_unitChoice->GetSelection() + 1 : 1;
    m_symbol->SetUnitSelection( &GetParent()->GetCurrentSheet(), unit_selection );
    m_symbol->SetUnit( unit_selection );

    switch( m_orientationCtrl->GetSelection() )
    {
    case 0: m_symbol->SetOrientation( SYM_ORIENT_0 );   break;
    case 1: m_symbol->SetOrientation( SYM_ORIENT_90 );  break;
    case 2: m_symbol->SetOrientation( SYM_ORIENT_270 ); break;
    case 3: m_symbol->SetOrientation( SYM_ORIENT_180 ); break;
    }

    switch( m_mirrorCtrl->GetSelection() )
    {
    case 0:                                           break;
    case 1: m_symbol->SetOrientation( SYM_MIRROR_X ); break;
    case 2: m_symbol->SetOrientation( SYM_MIRROR_Y ); break;
    }

    if( m_part )
    {
        m_part->SetShowPinNames( m_ShowPinNameButt->GetValue() );
        m_part->SetShowPinNumbers( m_ShowPinNumButt->GetValue() );
    }

    // Restore m_Flag modified by SetUnit() and other change settings from the dialog
    m_symbol->ClearFlags();
    m_symbol->SetFlags( flags );

    // change all field positions from relative to absolute
    for( unsigned i = 0;  i < m_fields->size();  ++i )
        m_fields->at( i ).Offset( m_symbol->GetPosition() );

    SCH_FIELDS& fields = m_symbol->GetFields();

    fields.clear();

    for( size_t ii = 0; ii < m_fields->size(); ++ii )
    {
        SCH_FIELD&      field = m_fields->at( ii );
        const wxString& fieldName = field.GetCanonicalName();

        if( fieldName.IsEmpty() && field.GetText().IsEmpty() )
            continue;
        else if( fieldName.IsEmpty() )
            field.SetName( _( "untitled" ) );

        fields.push_back( field );
    }

    // Reference has a specific initialization, depending on the current active sheet
    // because for a given symbol, in a complex hierarchy, there are more than one
    // reference.
    m_symbol->SetRef( &GetParent()->GetCurrentSheet(), m_fields->at( REFERENCE_FIELD ).GetText() );

    // Similar for Value and Footprint, except that the GUI behaviour is that they are kept
    // in sync between multiple instances.
    m_symbol->SetValueFieldText( m_fields->at( VALUE_FIELD ).GetText() );
    m_symbol->SetFootprintFieldText(  m_fields->at( FOOTPRINT_FIELD ).GetText() );

    m_symbol->SetExcludedFromBOM( m_cbExcludeFromBom->IsChecked() );
    m_symbol->SetExcludedFromBoard( m_cbExcludeFromBoard->IsChecked() );
    m_symbol->SetDNP( m_cbDNP->IsChecked() );

    // Update any assignments
    if( m_dataModel )
    {
        for( const SCH_PIN& model_pin : *m_dataModel )
        {
            // map from the edited copy back to the "real" pin in the symbol.
            SCH_PIN* src_pin = m_symbol->GetPin( model_pin.GetNumber() );

            if( src_pin )
                src_pin->SetAlt( model_pin.GetAlt() );
        }
    }

    // Keep fields other than the reference, include/exclude flags, and alternate pin assignements
    // in sync in multi-unit parts.
    if( m_symbol->GetUnitCount() > 1 && m_symbol->IsAnnotated( &GetParent()->GetCurrentSheet() ) )
    {
        wxString ref = m_symbol->GetRef( &GetParent()->GetCurrentSheet() );
        int      unit = m_symbol->GetUnit();
        LIB_ID   libId = m_symbol->GetLibId();

        for( SCH_SHEET_PATH& sheet : GetParent()->Schematic().GetSheets() )
        {
            SCH_SCREEN*              screen = sheet.LastScreen();
            std::vector<SCH_SYMBOL*> otherUnits;

            CollectOtherUnits( ref, unit, libId, sheet, &otherUnits );

            for( SCH_SYMBOL* otherUnit : otherUnits )
            {
                commit.Modify( otherUnit, screen );
                otherUnit->SetValueFieldText( m_fields->at( VALUE_FIELD ).GetText() );
                otherUnit->SetFootprintFieldText( m_fields->at( FOOTPRINT_FIELD ).GetText() );

                for( size_t ii = DATASHEET_FIELD; ii < m_fields->size(); ++ii )
                {
                    SCH_FIELD* otherField = otherUnit->FindField( m_fields->at( ii ).GetName() );

                    if( otherField )
                    {
                        otherField->SetText( m_fields->at( ii ).GetText() );
                    }
                    else
                    {
                        SCH_FIELD newField( m_fields->at( ii ) );
                        const_cast<KIID&>( newField.m_Uuid ) = KIID();

                        newField.Offset( -m_symbol->GetPosition() );
                        newField.Offset( otherUnit->GetPosition() );

                        newField.SetParent( otherUnit );
                        otherUnit->AddField( newField );
                    }
                }

                for( size_t ii = otherUnit->GetFields().size() - 1; ii > DATASHEET_FIELD; ii-- )
                {
                    SCH_FIELD& otherField = otherUnit->GetFields().at( ii );

                    if( !m_symbol->FindField( otherField.GetName() ) )
                        otherUnit->GetFields().erase( otherUnit->GetFields().begin() + ii );
                }

                otherUnit->SetExcludedFromBOM( m_cbExcludeFromBom->IsChecked() );
                otherUnit->SetExcludedFromBoard( m_cbExcludeFromBoard->IsChecked() );
                otherUnit->SetDNP( m_cbDNP->IsChecked() );

                if( m_dataModel )
                {
                    for( const SCH_PIN& model_pin : *m_dataModel )
                    {
                        SCH_PIN* src_pin = otherUnit->GetPin( model_pin.GetNumber() );

                        if( src_pin )
                            src_pin->SetAlt( model_pin.GetAlt() );
                    }
                }
            }
        }
    }

    currentScreen->Append( m_symbol );

    if( !commit.Empty() )
        commit.Push( _( "Edit Symbol Properties" ) );

    return true;
}


void DIALOG_SYMBOL_PROPERTIES::OnGridCellChanging( wxGridEvent& event )
{
    wxGridCellEditor* editor = m_fieldsGrid->GetCellEditor( event.GetRow(), event.GetCol() );
    wxControl* control = editor->GetControl();

    if( control && control->GetValidator() && !control->GetValidator()->Validate( control ) )
    {
        event.Veto();
        wxCommandEvent *evt = new wxCommandEvent( SYMBOL_DELAY_FOCUS );
        evt->SetClientData( new VECTOR2I( event.GetRow(), event.GetCol() ) );
        QueueEvent( evt );
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
                DisplayError( this, wxString::Format( _( "Field name '%s' already in use." ),
                                                      newName ) );
                event.Veto();
                wxCommandEvent *evt = new wxCommandEvent( SYMBOL_DELAY_FOCUS );
                evt->SetClientData( new VECTOR2I( event.GetRow(), event.GetCol() ) );
                QueueEvent( evt );
            }
        }
    }

    editor->DecRef();
}


void DIALOG_SYMBOL_PROPERTIES::OnGridEditorShown( wxGridEvent& aEvent )
{
    if( aEvent.GetRow() == REFERENCE_FIELD && aEvent.GetCol() == FDC_VALUE )
        QueueEvent( new wxCommandEvent( SYMBOL_DELAY_SELECTION ) );

    m_editorShown = true;
}


void DIALOG_SYMBOL_PROPERTIES::OnGridEditorHidden( wxGridEvent& aEvent )
{
    m_editorShown = false;
}


void DIALOG_SYMBOL_PROPERTIES::OnAddField( wxCommandEvent& event )
{
    if( !m_fieldsGrid->CommitPendingChanges() )
        return;

    SCHEMATIC_SETTINGS& settings = m_symbol->Schematic()->Settings();
    int                 fieldID = (int) m_fields->size();
    SCH_FIELD           newField( VECTOR2I( 0, 0 ), fieldID, m_symbol,
                                  TEMPLATE_FIELDNAME::GetDefaultFieldName( fieldID, DO_TRANSLATE ) );

    newField.SetTextAngle( m_fields->at( REFERENCE_FIELD ).GetTextAngle() );
    newField.SetTextSize( VECTOR2I( settings.m_DefaultTextSize, settings.m_DefaultTextSize ) );

    m_fields->push_back( newField );

    // notify the grid
    wxGridTableMessage msg( m_fields, wxGRIDTABLE_NOTIFY_ROWS_APPENDED, 1 );
    m_fieldsGrid->ProcessTableMessage( msg );

    m_fieldsGrid->MakeCellVisible( (int) m_fields->size() - 1, 0 );
    m_fieldsGrid->SetGridCursor( (int) m_fields->size() - 1, 0 );

    m_fieldsGrid->EnableCellEditControl();
    m_fieldsGrid->ShowCellEditControl();

    OnModify();
}


void DIALOG_SYMBOL_PROPERTIES::OnDeleteField( wxCommandEvent& event )
{
    wxArrayInt selectedRows = m_fieldsGrid->GetSelectedRows();

    if( selectedRows.empty() && m_fieldsGrid->GetGridCursorRow() >= 0 )
        selectedRows.push_back( m_fieldsGrid->GetGridCursorRow() );

    if( selectedRows.empty() )
        return;

    for( int row : selectedRows )
    {
        if( row < MANDATORY_FIELDS )
        {
            DisplayError( this, wxString::Format( _( "The first %d fields are mandatory." ),
                                                  MANDATORY_FIELDS ) );
            return;
        }
    }

    m_fieldsGrid->CommitPendingChanges( true /* quiet mode */ );
    m_fieldsGrid->ClearSelection();

    // Reverse sort so deleting a row doesn't change the indexes of the other rows.
    selectedRows.Sort( []( int* first, int* second ) { return *second - *first; } );

    for( int row : selectedRows )
    {
        m_fields->erase( m_fields->begin() + row );

        // notify the grid
        wxGridTableMessage msg( m_fields, wxGRIDTABLE_NOTIFY_ROWS_DELETED, row, 1 );
        m_fieldsGrid->ProcessTableMessage( msg );

        if( m_fieldsGrid->GetNumberRows() > 0 )
        {
            m_fieldsGrid->MakeCellVisible( std::max( 0, row-1 ), m_fieldsGrid->GetGridCursorCol() );
            m_fieldsGrid->SetGridCursor( std::max( 0, row-1 ), m_fieldsGrid->GetGridCursorCol() );
        }
    }

    OnModify();
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
        m_fieldsGrid->MakeCellVisible( m_fieldsGrid->GetGridCursorRow(),
                                       m_fieldsGrid->GetGridCursorCol() );

        OnModify();
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
        m_fieldsGrid->MakeCellVisible( m_fieldsGrid->GetGridCursorRow(),
                                       m_fieldsGrid->GetGridCursorCol() );

        OnModify();
    }
    else
    {
        wxBell();
    }
}


void DIALOG_SYMBOL_PROPERTIES::OnEditSymbol( wxCommandEvent&  )
{
    if( TransferDataFromWindow() )
        EndQuasiModal( SYMBOL_PROPS_EDIT_SCHEMATIC_SYMBOL );
}


void DIALOG_SYMBOL_PROPERTIES::OnEditLibrarySymbol( wxCommandEvent&  )
{
    if( TransferDataFromWindow() )
        EndQuasiModal( SYMBOL_PROPS_EDIT_LIBRARY_SYMBOL );
}


void DIALOG_SYMBOL_PROPERTIES::OnUpdateSymbol( wxCommandEvent&  )
{
    if( TransferDataFromWindow() )
        EndQuasiModal( SYMBOL_PROPS_WANT_UPDATE_SYMBOL );
}


void DIALOG_SYMBOL_PROPERTIES::OnExchangeSymbol( wxCommandEvent&  )
{
    if( TransferDataFromWindow() )
        EndQuasiModal( SYMBOL_PROPS_WANT_EXCHANGE_SYMBOL );
}


void DIALOG_SYMBOL_PROPERTIES::OnPinTableCellEdited( wxGridEvent& aEvent )
{
    int row = aEvent.GetRow();

    if( m_pinGrid->GetCellValue( row, COL_ALT_NAME )
            == m_dataModel->GetValue( row, COL_BASE_NAME ) )
    {
        m_dataModel->SetValue( row, COL_ALT_NAME, wxEmptyString );
    }

    // These are just to get the cells refreshed
    m_dataModel->SetValue( row, COL_TYPE, m_dataModel->GetValue( row, COL_TYPE ) );
    m_dataModel->SetValue( row, COL_SHAPE, m_dataModel->GetValue( row, COL_SHAPE ) );

    OnModify();
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
    m_dataModel->BuildAttrs();
}


void DIALOG_SYMBOL_PROPERTIES::AdjustFieldsGridColumns()
{
    wxGridUpdateLocker deferRepaintsTillLeavingScope( m_fieldsGrid );

    // Account for scroll bars
    int fieldsWidth = KIPLATFORM::UI::GetUnobscuredSize( m_fieldsGrid ).x;

    m_fieldsGrid->AutoSizeColumn( 0 );
    m_fieldsGrid->SetColSize( 0, std::max( 72, m_fieldsGrid->GetColSize( 0 ) ) );

    int fixedColsWidth = m_fieldsGrid->GetColSize( 0 );

    for( int i = 2; i < m_fieldsGrid->GetNumberCols(); i++ )
        fixedColsWidth += m_fieldsGrid->GetColSize( i );

    m_fieldsGrid->SetColSize( 1, std::max( 120, fieldsWidth - fixedColsWidth ) );
}


void DIALOG_SYMBOL_PROPERTIES::AdjustPinsGridColumns()
{
    wxGridUpdateLocker deferRepaintsTillLeavingScope( m_pinGrid );

    // Account for scroll bars
    int pinTblWidth = KIPLATFORM::UI::GetUnobscuredSize( m_pinGrid ).x;

    // Stretch the Base Name and Alternate Assignment columns to fit.
    for( int i = 0; i < COL_COUNT; ++i )
    {
        if( i != COL_BASE_NAME && i != COL_ALT_NAME )
            pinTblWidth -= m_pinGrid->GetColSize( i );
    }

    m_pinGrid->SetColSize( COL_BASE_NAME, pinTblWidth / 2 );
    m_pinGrid->SetColSize( COL_ALT_NAME, pinTblWidth / 2 );
}


void DIALOG_SYMBOL_PROPERTIES::OnUpdateUI( wxUpdateUIEvent& event )
{
    std::bitset<64> shownColumns = m_fieldsGrid->GetShownColumns();

    if( shownColumns != m_shownColumns )
    {
        m_shownColumns = shownColumns;

        if( !m_fieldsGrid->IsCellEditControlShown() )
            AdjustFieldsGridColumns();
    }

    wxString simEnable;

    for( int ii = MANDATORY_FIELDS; ii < m_fieldsGrid->GetNumberRows(); ++ii )
    {
        if( m_fieldsGrid->GetCellValue( ii, FDC_NAME ) == SIM_ENABLE_FIELD )
        {
            simEnable = m_fieldsGrid->GetCellValue( ii, FDC_VALUE );
            break;
        }
    }

    m_cbExcludeFromSim->SetValue( simEnable == wxS( "0" ) );
}


void DIALOG_SYMBOL_PROPERTIES::HandleDelayedFocus( wxCommandEvent& event )
{
    VECTOR2I *loc = static_cast<VECTOR2I*>( event.GetClientData() );

    wxCHECK_RET( loc, wxT( "Missing focus cell location" ) );

    // Handle a delayed focus

    m_fieldsGrid->SetFocus();
    m_fieldsGrid->MakeCellVisible( loc->x, loc->y );
    m_fieldsGrid->SetGridCursor( loc->x, loc->y );

    m_fieldsGrid->EnableCellEditControl( true );
    m_fieldsGrid->ShowCellEditControl();

    delete loc;
}


void DIALOG_SYMBOL_PROPERTIES::HandleDelayedSelection( wxCommandEvent& event )
{
    // Handle a delayed selection
    wxGridCellEditor* cellEditor = m_fieldsGrid->GetCellEditor( REFERENCE_FIELD, FDC_VALUE );

    if( wxTextEntry* txt = dynamic_cast<wxTextEntry*>( cellEditor->GetControl() ) )
        KIUI::SelectReferenceNumber( txt );

    cellEditor->DecRef();   // we're done; must release
}

void DIALOG_SYMBOL_PROPERTIES::OnSizeFieldsGrid( wxSizeEvent& event )
{
    wxSize new_size = event.GetSize();

    if( ( !m_editorShown || m_lastRequestedSize != new_size ) && m_fieldsSize != new_size )
    {
        m_fieldsSize = new_size;

        AdjustFieldsGridColumns();
    }

    // We store this value to check whether the dialog is changing size.  This might indicate
    // that the user is scaling the dialog with an editor shown.  Some editors do not close
    // (at least on GTK) when the user drags a dialog corner
    m_lastRequestedSize = new_size;

    // Always propagate for a grid repaint (needed if the height changes, as well as width)
    event.Skip();
}


void DIALOG_SYMBOL_PROPERTIES::OnSizePinsGrid( wxSizeEvent& event )
{
    wxSize new_size = event.GetSize();

    if( m_pinsSize != new_size )
    {
        m_pinsSize = new_size;

        AdjustPinsGridColumns();
    }

    // Always propagate for a grid repaint (needed if the height changes, as well as width)
    event.Skip();
}


void DIALOG_SYMBOL_PROPERTIES::OnInitDlg( wxInitDialogEvent& event )
{
    TransferDataToWindow();

    // Now all widgets have the size fixed, call FinishDialogSettings
    finishDialogSettings();

    EESCHEMA_SETTINGS* cfg = dynamic_cast<EESCHEMA_SETTINGS*>( Kiface().KifaceSettings() );

    if( cfg && cfg->m_Appearance.edit_symbol_width > 0 && cfg->m_Appearance.edit_symbol_height > 0 )
        SetSize( cfg->m_Appearance.edit_symbol_width, cfg->m_Appearance.edit_symbol_height );
}


void DIALOG_SYMBOL_PROPERTIES::OnCheckBox( wxCommandEvent& event )
{
    OnModify();
}


void DIALOG_SYMBOL_PROPERTIES::OnUnitChoice( wxCommandEvent& event )
{
    if( m_dataModel )
    {
        EDA_ITEM_FLAGS flags = m_symbol->GetFlags();

        int unit_selection = m_unitChoice->GetSelection() + 1;

        // We need to select a new unit to build the new unit pin list
        // but we should not change the symbol, so the initial unit will be selected
        // after rebuilding the pin list
        int old_unit = m_symbol->GetUnit();
        m_symbol->SetUnit( unit_selection );

        // Rebuild a copy of the pins of the new unit for editing
        m_dataModel->clear();

        for( const std::unique_ptr<SCH_PIN>& pin : m_symbol->GetRawPins() )
            m_dataModel->push_back( *pin );

        m_dataModel->SortRows( COL_NUMBER, true );
        m_dataModel->BuildAttrs();

        m_symbol->SetUnit( old_unit );

        // Restore m_Flag modified by SetUnit()
        m_symbol->ClearFlags();
        m_symbol->SetFlags( flags );
    }

    OnModify();
}


void DIALOG_SYMBOL_PROPERTIES::onUpdateEditSymbol( wxUpdateUIEvent& event )
{
    event.Enable( m_symbol && m_symbol->GetLibSymbolRef() );
}


void DIALOG_SYMBOL_PROPERTIES::onUpdateEditLibrarySymbol( wxUpdateUIEvent& event )
{
    event.Enable( m_symbol && m_symbol->GetLibSymbolRef() );
}

