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
#include <wx/uiaction.h>
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
#include <sch_collectors.h>
#include <fields_grid_table.h>
#include <sch_edit_frame.h>
#include <sch_reference_list.h>
#include <schematic.h>
#include <sch_commit.h>
#include <tool/tool_manager.h>
#include <tool/actions.h>

#include <dialog_sim_model.h>
#include <panel_embedded_files.h>


wxDEFINE_EVENT( SYMBOL_DELAY_FOCUS, wxCommandEvent );
wxDEFINE_EVENT( SYMBOL_DELAY_SELECTION, wxCommandEvent );


/**
 * Debug helper to trap all SetFocus events on a window and its children.
 * This creates a custom event handler that logs whenever focus is set on any control.
 */
static void EnableFocusDebugging( wxWindow* aWindow, const wxString& aWindowName = wxEmptyString )
{
    // Lambda-based focus event handler
    auto onFocus = [aWindowName]( wxFocusEvent& aEvent ) -> void
    {
        wxWindow* window = static_cast<wxWindow*>( aEvent.GetEventObject() );
        if( !window )
            return;

        wxString controlName = window->GetName();
        wxString controlLabel;

        // Try to get a human-readable label for the control
        wxControl* ctrl = dynamic_cast<wxControl*>( window );
        if( ctrl && !ctrl->GetLabel().empty() )
            controlLabel = ctrl->GetLabel();

        wxString windowInfo = aWindowName.empty() ? wxString( "" ) : aWindowName + wxString( ": " );
        // fix a conflict that happens with a Windows header on MINGW. so undefine GetClassName
        #if defined( GetClassName ) && defined( __MINGW32__ )
        #undef GetClassName
        #endif

        if( aEvent.GetEventType() == wxEVT_SET_FOCUS )
        {
            wxLogTrace( wxS( "FOCUS_DEBUG" ), wxS( "%sFocus SET on: %s (name=%s, label=%s)" ),
                        windowInfo,
                        window->GetClassInfo()->GetClassName(),
                        controlName,
                        controlLabel );
        }
        else if( aEvent.GetEventType() == wxEVT_KILL_FOCUS )
        {
            wxLogTrace( wxS( "FOCUS_DEBUG" ), wxS( "%sFocus LOST from: %s (name=%s, label=%s)" ),
                        windowInfo,
                        window->GetClassInfo()->GetClassName(),
                        controlName,
                        controlLabel );
        }

        aEvent.Skip();  // Allow event to propagate
    };

    // Recursively attach handler to this window and all children
    std::function<void( wxWindow* )> attachToTree = [&]( wxWindow* w ) -> void
    {
        if( !w )
            return;

        w->Bind( wxEVT_SET_FOCUS, onFocus );
        w->Bind( wxEVT_KILL_FOCUS, onFocus );

        // Recursively attach to all child windows
        for( wxWindow* child : w->GetChildren() )
            attachToTree( child );
    };

    attachToTree( aWindow );

    wxLogTrace( wxS( "FOCUS_DEBUG" ), wxS( "Focus debugging enabled for: %s" ), aWindowName );
}


enum PIN_TABLE_COL_ORDER
{
    COL_NUMBER,
    COL_BASE_NAME,
    COL_ALT_NAME,
    COL_TYPE,
    COL_SHAPE,

    COL_COUNT       // keep as last
};


class SCH_PIN_TABLE_DATA_MODEL : public WX_GRID_TABLE_BASE, public std::vector<SCH_PIN>
{
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
            SCH_PIN*        lib_pin = pin.GetLibPin();
            wxGridCellAttr* attr = nullptr;

            if( !lib_pin || lib_pin->GetAlternates().empty() )
            {
                attr = new wxGridCellAttr;
                attr->SetReadOnly( true );
                attr->SetBackgroundColour( KIPLATFORM::UI::GetDialogBGColour() );
            }
            else
            {
                wxArrayString choices;
                choices.push_back( lib_pin->GetName() );

                for( const std::pair<const wxString, SCH_PIN::ALT>& alt : lib_pin->GetAlternates() )
                    choices.push_back( alt.first );

                attr = new wxGridCellAttr();
                attr->SetEditor( new GRID_CELL_COMBOBOX( choices ) );
            }

            m_nameAttrs.push_back( attr );
        }

        if( m_typeAttr )
            m_typeAttr->DecRef();

        m_typeAttr = new wxGridCellAttr;
        m_typeAttr->SetRenderer( new GRID_CELL_ICON_TEXT_RENDERER( PinTypeIcons(), PinTypeNames() ) );
        m_typeAttr->SetReadOnly( true );

        if( m_shapeAttr )
            m_shapeAttr->DecRef();

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

    bool CanSetValueAs( int aRow, int aCol, const wxString& aTypeName ) override
    {
        // Don't accept random values; must use the popup to change to a known alternate
        return false;
    }

    wxString GetValue( int aRow, int aCol ) override
    {
        return GetValue( at( aRow ), aCol );
    }

    static wxString GetValue( const SCH_PIN& aPin, int aCol )
    {
        if( aCol == COL_ALT_NAME )
        {
            if( !aPin.GetLibPin() || aPin.GetLibPin()->GetAlternates().empty() )
                return wxEmptyString;
            else if( aPin.GetAlt().IsEmpty() )
                return aPin.GetName();
            else
                return aPin.GetAlt();
        }

        switch( aCol )
        {
        case COL_NUMBER:    return aPin.GetNumber();
        case COL_BASE_NAME: return aPin.GetBaseName();
        case COL_TYPE:      return PinTypeNames()[static_cast<int>( aPin.GetType() )];
        case COL_SHAPE:     return PinShapeNames()[static_cast<int>( aPin.GetShape() )];
        default:   wxFAIL;  return wxEmptyString;
        }
    }

    wxGridCellAttr* GetAttr( int aRow, int aCol, wxGridCellAttr::wxAttrKind aKind ) override
    {
        switch( aCol )
        {
        case COL_NUMBER:
        case COL_BASE_NAME:
            m_readOnlyAttr->IncRef();
            return enhanceAttr( m_readOnlyAttr, aRow, aCol, aKind );

        case COL_ALT_NAME:
            m_nameAttrs[ aRow ]->IncRef();
            return enhanceAttr( m_nameAttrs[ aRow ], aRow, aCol, aKind );

        case COL_TYPE:
            m_typeAttr->IncRef();
            return enhanceAttr( m_typeAttr, aRow, aCol, aKind );

        case COL_SHAPE:
            m_shapeAttr->IncRef();
            return enhanceAttr( m_shapeAttr, aRow, aCol, aKind );

        default:
            wxFAIL;
            return nullptr;
        }
    }

    void SetValue( int aRow, int aCol, const wxString &aValue ) override
    {
        SCH_PIN& pin = at( aRow );

        switch( aCol )
        {
        case COL_ALT_NAME:
            if( pin.GetLibPin() && aValue == pin.GetLibPin()->GetName() )
                pin.SetAlt( wxEmptyString );
            else
                pin.SetAlt( aValue );
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

protected:
    std::vector<wxGridCellAttr*> m_nameAttrs;
    wxGridCellAttr*              m_readOnlyAttr;
    wxGridCellAttr*              m_typeAttr;
    wxGridCellAttr*              m_shapeAttr;
};


DIALOG_SYMBOL_PROPERTIES::DIALOG_SYMBOL_PROPERTIES( SCH_EDIT_FRAME* aParent, SCH_SYMBOL* aSymbol ) :
        DIALOG_SYMBOL_PROPERTIES_BASE( aParent ),
        m_symbol( nullptr ),
        m_part( nullptr ),
        m_lastRequestedPinsSize( 0, 0 ),
        m_editorShown( false ),
        m_fields( nullptr ),
        m_dataModel( nullptr ),
        m_embeddedFiles( nullptr )
{
    m_symbol = aSymbol;
    m_part = m_symbol->GetLibSymbolRef().get();

    // GetLibSymbolRef() now points to the cached part in the schematic, which should always be
    // there for usual cases, but can be null when opening old schematics not storing the part
    // so we need to handle m_part == nullptr
    // wxASSERT( m_part );

    m_fields = new FIELDS_GRID_TABLE( this, aParent, m_fieldsGrid, m_symbol );

    m_fieldsGrid->SetTable( m_fields );
    m_fieldsGrid->PushEventHandler( new FIELDS_GRID_TRICKS( m_fieldsGrid, this,
                                                            { &aParent->Schematic(), m_part },
                                                            [&]( wxCommandEvent& aEvent )
                                                            {
                                                                OnAddField( aEvent );
                                                            } ) );
    m_fieldsGrid->SetSelectionMode( wxGrid::wxGridSelectRows );
    m_fieldsGrid->ShowHideColumns( "0 1 2 3 4 5 6 7" );
    m_shownColumns = m_fieldsGrid->GetShownColumns();

    if( m_symbol->GetEmbeddedFiles() )
    {
        m_embeddedFiles = new PANEL_EMBEDDED_FILES( m_notebook1, m_symbol->GetEmbeddedFiles() );
        m_notebook1->AddPage( m_embeddedFiles, _( "Embedded Files" ) );
    }

    if( m_part && m_part->IsMultiBodyStyle() )
    {
        // Multiple body styles are a superclass of alternate pin assignments, so don't allow
        // free-form alternate assignments as well.  (We won't know how to map the alternates
        // back and forth when the body style is changed.)
        m_pinTablePage->Disable();
        m_pinTablePage->SetToolTip( _( "Alternate pin assignments are not available for symbols with multiple "
                                       "body styles." ) );
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

    wxFont infoFont = KIUI::GetSmallInfoFont( this );
    m_libraryIDLabel->SetFont( infoFont );
    m_tcLibraryID->SetFont( infoFont );
    m_tcLibraryID->SetBackgroundColour( KIPLATFORM::UI::GetDialogBGColour() );

    wxToolTip::Enable( true );
    SetupStandardButtons();

    // Configure button logos
    m_bpAdd->SetBitmap( KiBitmapBundle( BITMAPS::small_plus ) );
    m_bpDelete->SetBitmap( KiBitmapBundle( BITMAPS::small_trash ) );
    m_bpMoveUp->SetBitmap( KiBitmapBundle( BITMAPS::small_up ) );
    m_bpMoveDown->SetBitmap( KiBitmapBundle( BITMAPS::small_down ) );

    // Enable focus debugging to track which element has focus
    EnableFocusDebugging( this, wxS( "DIALOG_SYMBOL_PROPERTIES" ) );

    // wxFormBuilder doesn't include this event...
    m_fieldsGrid->Bind( wxEVT_GRID_CELL_CHANGING, &DIALOG_SYMBOL_PROPERTIES::OnGridCellChanging, this );
    m_pinGrid->Bind( wxEVT_GRID_COL_SORT, &DIALOG_SYMBOL_PROPERTIES::OnPinTableColSort, this );
    Bind( SYMBOL_DELAY_FOCUS, &DIALOG_SYMBOL_PROPERTIES::HandleDelayedFocus, this );
    Bind( SYMBOL_DELAY_SELECTION, &DIALOG_SYMBOL_PROPERTIES::HandleDelayedSelection, this );

    wxCommandEvent* evt = new wxCommandEvent( SYMBOL_DELAY_SELECTION );
    evt->SetClientData( new VECTOR2I( 0, FDC_VALUE ) );
    QueueEvent( evt );

    evt = new wxCommandEvent( SYMBOL_DELAY_FOCUS );
    evt->SetClientData( new VECTOR2I( 0, FDC_VALUE ) );
    QueueEvent( evt );

    // Remind user that they are editing the current variant.
    if( !aParent->Schematic().GetCurrentVariant().IsEmpty() )
        SetTitle( GetTitle() + wxS( " - " ) + aParent->Schematic().GetCurrentVariant() + _( " Variant" ) );

    Layout();
    m_fieldsGrid->Layout();

    if( GetSizer() )
        GetSizer()->Fit( this );

    finishDialogSettings();
}


DIALOG_SYMBOL_PROPERTIES::~DIALOG_SYMBOL_PROPERTIES()
{
    // Prevents crash bug in wxGrid's d'tor
    m_fieldsGrid->DestroyTable( m_fields );

    if( m_dataModel )
        m_pinGrid->DestroyTable( m_dataModel );

    m_fieldsGrid->Unbind( wxEVT_GRID_CELL_CHANGING, &DIALOG_SYMBOL_PROPERTIES::OnGridCellChanging, this );
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

    const SCHEMATIC& schematic = GetParent()->Schematic();
    SCH_SHEET_PATH& sheetPath = schematic.CurrentSheet();
    wxString variantName = schematic.GetCurrentVariant();
    std::optional<SCH_SYMBOL_VARIANT> variant = m_symbol->GetVariant( sheetPath, variantName );
    std::set<wxString> defined;

    // Push a copy of each field into m_updateFields
    for( SCH_FIELD& srcField : m_symbol->GetFields() )
    {
        SCH_FIELD field( srcField );

        // change offset to be symbol-relative
        field.Offset( -m_symbol->GetPosition() );
        field.SetText( schematic.ConvertKIIDsToRefs( m_symbol->GetFieldText( field.GetName(), &sheetPath,
                                                                             variantName ) ) );

        defined.insert( field.GetName() );
        m_fields->push_back( field );
    }

    // Add in any template fieldnames not yet defined:
    for( const TEMPLATE_FIELDNAME& templateFieldname :
         schematic.Settings().m_TemplateFieldNames.GetTemplateFieldNames() )
    {
        if( defined.count( templateFieldname.m_Name ) <= 0 )
        {
            SCH_FIELD field( m_symbol, FIELD_T::USER, templateFieldname.m_Name );
            field.SetVisible( templateFieldname.m_Visible );
            m_fields->push_back( field );
        }
    }

    // notify the grid
    wxGridTableMessage msg( m_fields, wxGRIDTABLE_NOTIFY_ROWS_APPENDED, m_fields->GetNumberRows() );
    m_fieldsGrid->ProcessTableMessage( msg );

    // If a multi-unit symbol, set up the unit selector and interchangeable checkbox.
    if( m_symbol->IsMultiUnit() )
    {
        // Ensure symbol unit is the currently selected unit (mandatory in complex hierarchies)
        // from the current sheet path, because it can be modified by previous calculations
        m_symbol->SetUnit( m_symbol->GetUnitSelection( &sheetPath ) );

        for( int ii = 1; ii <= m_symbol->GetUnitCount(); ii++ )
            m_unitChoice->Append( m_symbol->GetUnitDisplayName( ii, false ) );

        if( m_symbol->GetUnit() <= ( int )m_unitChoice->GetCount() )
            m_unitChoice->SetSelection( m_symbol->GetUnit() - 1 );
    }
    else
    {
        m_unitLabel->Enable( false );
        m_unitChoice->Enable( false );
    }

    if( m_part && m_part->IsMultiBodyStyle() )
    {
        if( m_part->HasDeMorganBodyStyles() )
        {
            m_bodyStyleChoice->Append( _( "Standard" ) );
            m_bodyStyleChoice->Append( _( "Alternate" ) );
        }
        else
        {
            wxASSERT( (int)m_part->GetBodyStyleNames().size() == m_part->GetBodyStyleCount() );

            for( int ii = 0; ii < m_part->GetBodyStyleCount(); ii++ )
            {
                try
                {
                    m_bodyStyleChoice->Append( m_part->GetBodyStyleNames().at( ii ) );
                }
                catch( ... )
                {
                    m_bodyStyleChoice->Append( wxT( "???" ) );
                }
            }
        }

        if( m_symbol->GetBodyStyle() <= (int) m_bodyStyleChoice->GetCount() )
            m_bodyStyleChoice->SetSelection( m_symbol->GetBodyStyle() - 1 );
    }
    else
    {
        m_bodyStyle->Enable( false );
        m_bodyStyleChoice->Enable( false );
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
    case SYM_MIRROR_X: m_mirrorCtrl->SetSelection( 1 );  break;
    case SYM_MIRROR_Y: m_mirrorCtrl->SetSelection( 2 );  break;
    }

    m_cbExcludeFromSim->SetValue( m_symbol->GetExcludedFromSim( &sheetPath, variantName ) );
    m_cbExcludeFromBom->SetValue( m_symbol->GetExcludedFromBOM( &sheetPath, variantName ) );
    m_cbExcludeFromBoard->SetValue( m_symbol->GetExcludedFromBoard() );
    m_cbExcludeFromPosFiles->SetValue( m_symbol->GetExcludedFromPosFiles() );
    m_cbDNP->SetValue( m_symbol->GetDNP( &sheetPath, variantName ) );

    if( m_part )
    {
        m_ShowPinNumButt->SetValue( m_part->GetShowPinNumbers() );
        m_ShowPinNameButt->SetValue( m_part->GetShowPinNames() );
    }

    // Set the symbol's library name.
    m_tcLibraryID->SetValue( UnescapeString( m_symbol->GetLibId().Format() ) );

    if( m_embeddedFiles && !m_embeddedFiles->TransferDataToWindow() )
        return false;

    return true;
}


void DIALOG_SYMBOL_PROPERTIES::OnEditSpiceModel( wxCommandEvent& event )
{
    if( !m_fieldsGrid->CommitPendingChanges() )
        return;

    m_fieldsGrid->ClearSelection();

    std::vector<SCH_FIELD> fields;

    for( const SCH_FIELD& field : *m_fields )
        fields.emplace_back( field );

    DIALOG_SIM_MODEL dialog( this, m_parentFrame, *m_symbol, fields );

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
            m_fieldsGrid->ClearSelection();
            m_fields->erase( m_fields->begin() + ii );

            wxGridTableMessage msg( m_fields, wxGRIDTABLE_NOTIFY_ROWS_DELETED, ii, 1 );
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

    // Check for missing field names.
    for( size_t i = 0; i < m_fields->size(); ++i )
    {
        SCH_FIELD& field = m_fields->at( i );

        if( field.IsMandatory() )
            continue;

        wxString fieldName = field.GetName( false );

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

    if( m_embeddedFiles && !m_embeddedFiles->TransferDataFromWindow() )
        return false;

    if( !m_fieldsGrid->CommitPendingChanges() )
        return false;

    if( !m_pinGrid->CommitPendingChanges() )
        return false;

    SCH_COMMIT  commit( GetParent() );
    SCH_SCREEN* currentScreen = GetParent()->GetScreen();
    SCH_SHEET_PATH currentSheet = GetParent()->Schematic().CurrentSheet();
    wxString currentVariant = GetParent()->Schematic().GetCurrentVariant();
    bool        replaceOnCurrentScreen;

    wxCHECK( currentScreen, false );

    // This needs to be done before the LIB_ID is changed to prevent stale library symbols in
    // the schematic file.
    replaceOnCurrentScreen = currentScreen->Remove( m_symbol );

    // save old cmp in undo list if not already in edit, or moving ...
    if( m_symbol->GetEditFlags() == 0 )
        commit.Modify( m_symbol, currentScreen );

    // Save current flags which could be modified by next change settings
    EDA_ITEM_FLAGS flags = m_symbol->GetFlags();

    //Set the part selection in multiple part per package
    int unit_selection = m_unitChoice->IsEnabled() ? m_unitChoice->GetSelection() + 1 : 1;
    m_symbol->SetUnitSelection( &GetParent()->GetCurrentSheet(), unit_selection );
    m_symbol->SetUnit( unit_selection );

    int bodyStyle_selection = m_bodyStyleChoice->IsEnabled() ? m_bodyStyleChoice->GetSelection() + 1 : 1;
    m_symbol->SetBodyStyle( bodyStyle_selection );

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

    m_symbol->SetShowPinNames( m_ShowPinNameButt->GetValue() );
    m_symbol->SetShowPinNumbers( m_ShowPinNumButt->GetValue() );

    // Restore m_Flag modified by SetUnit() and other change settings from the dialog
    m_symbol->ClearFlags();
    m_symbol->SetFlags( flags );

    // change all field positions from relative to absolute
    for( SCH_FIELD& field : *m_fields )
        field.Offset( m_symbol->GetPosition() );

    int ordinal = 42;   // Arbitrarily larger than any mandatory FIELD_T ids.

    for( SCH_FIELD& field : *m_fields )
    {
        const wxString& fieldName = field.GetCanonicalName();

        if( fieldName.IsEmpty() && field.GetText().IsEmpty() )
            continue;
        else if( fieldName.IsEmpty() )
            field.SetName( _( "untitled" ) );

        const SCH_FIELD* existingField = m_symbol->GetField( fieldName );
        SCH_FIELD* tmp;

        if( !existingField )
        {
            tmp = m_symbol->AddField( field );
            tmp->SetParent( m_symbol );
        }
        else
        {
            wxString defaultText = m_symbol->Schematic()->ConvertRefsToKIIDs( existingField->GetText() );
            tmp = const_cast<SCH_FIELD*>( existingField );

            *tmp = field;

            if( !currentVariant.IsEmpty() )
            {
                // Restore the default field text for existing fields.
                tmp->SetText( defaultText, &currentSheet );

                wxString variantText = m_symbol->Schematic()->ConvertRefsToKIIDs( field.GetText() );
                tmp->SetText( variantText, &currentSheet, currentVariant );
            }
        }

        if( !field.IsMandatory() )
            field.SetOrdinal( ordinal++ );
    }

    m_symbol->SetExcludedFromSim( m_cbExcludeFromSim->IsChecked(), &currentSheet, currentVariant );
    m_symbol->SetExcludedFromBOM( m_cbExcludeFromBom->IsChecked(), &currentSheet, currentVariant );
    m_symbol->SetExcludedFromBoard( m_cbExcludeFromBoard->IsChecked() );
    m_symbol->SetExcludedFromPosFiles( m_cbExcludeFromPosFiles->IsChecked() );
    m_symbol->SetDNP( m_cbDNP->IsChecked(), &currentSheet, currentVariant );

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
    m_symbol->SyncOtherUnits( currentSheet, commit, nullptr );

    if( replaceOnCurrentScreen )
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
    if( m_fields->at( aEvent.GetRow() ).GetId() == FIELD_T::REFERENCE
            && aEvent.GetCol() == FDC_VALUE )
    {
        wxCommandEvent* evt = new wxCommandEvent( SYMBOL_DELAY_SELECTION );
        evt->SetClientData( new VECTOR2I( aEvent.GetRow(), aEvent.GetCol() ) );
        QueueEvent( evt );
    }

    m_editorShown = true;
}


void DIALOG_SYMBOL_PROPERTIES::OnGridEditorHidden( wxGridEvent& aEvent )
{
    m_editorShown = false;
}


void DIALOG_SYMBOL_PROPERTIES::OnAddField( wxCommandEvent& event )
{
    m_fieldsGrid->OnAddRow(
            [&]() -> std::pair<int, int>
            {
                SCH_FIELD newField( m_symbol, FIELD_T::USER, GetUserFieldName( m_fields->size(), DO_TRANSLATE ) );

                newField.SetTextAngle( m_fields->GetField( FIELD_T::REFERENCE )->GetTextAngle() );
                newField.SetVisible( false );

                m_fields->push_back( newField );

                // notify the grid
                wxGridTableMessage msg( m_fields, wxGRIDTABLE_NOTIFY_ROWS_APPENDED, 1 );
                m_fieldsGrid->ProcessTableMessage( msg );
                OnModify();

                return { m_fields->size() - 1, FDC_NAME };
            } );
}


void DIALOG_SYMBOL_PROPERTIES::OnDeleteField( wxCommandEvent& event )
{
    m_fieldsGrid->OnDeleteRows(
            [&]( int row )
            {
                if( row < m_fields->GetMandatoryRowCount() )
                {
                    DisplayError( this, wxString::Format( _( "The first %d fields are mandatory." ),
                                                          m_fields->GetMandatoryRowCount() ) );
                    return false;
                }

                return true;
            },
            [&]( int row )
            {
                m_fields->erase( m_fields->begin() + row );

                // notify the grid
                wxGridTableMessage msg( m_fields, wxGRIDTABLE_NOTIFY_ROWS_DELETED, row, 1 );
                m_fieldsGrid->ProcessTableMessage( msg );
            } );

    OnModify();
}


void DIALOG_SYMBOL_PROPERTIES::OnMoveUp( wxCommandEvent& event )
{
    m_fieldsGrid->OnMoveRowUp(
            [&]( int row )
            {
                return row > m_fields->GetMandatoryRowCount();
            },
            [&]( int row )
            {
                std::swap( *( m_fields->begin() + row ), *( m_fields->begin() + row - 1 ) );
                m_fieldsGrid->ForceRefresh();
                OnModify();
            } );
}


void DIALOG_SYMBOL_PROPERTIES::OnMoveDown( wxCommandEvent& event )
{
    m_fieldsGrid->OnMoveRowDown(
            [&]( int row )
            {
                return row >= m_fields->GetMandatoryRowCount();
            },
            [&]( int row )
            {
                    std::swap( *( m_fields->begin() + row ), *( m_fields->begin() + row + 1 ) );
                    m_fieldsGrid->ForceRefresh();
                    OnModify();
            } );
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

    if( m_pinGrid->GetCellValue( row, COL_ALT_NAME ) == m_dataModel->GetValue( row, COL_BASE_NAME ) )
        m_dataModel->SetValue( row, COL_ALT_NAME, wxEmptyString );

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

    if( pinTblWidth > 2 )
    {
        m_pinGrid->SetColSize( COL_BASE_NAME, pinTblWidth / 2 );
        m_pinGrid->SetColSize( COL_ALT_NAME, pinTblWidth / 2 );
    }
}


void DIALOG_SYMBOL_PROPERTIES::OnUpdateUI( wxUpdateUIEvent& event )
{
    std::bitset<64> shownColumns = m_fieldsGrid->GetShownColumns();

    if( shownColumns != m_shownColumns )
    {
        m_shownColumns = shownColumns;

        if( !m_fieldsGrid->IsCellEditControlShown() )
            m_fieldsGrid->SetGridWidthsDirty();
    }
}


void DIALOG_SYMBOL_PROPERTIES::HandleDelayedFocus( wxCommandEvent& event )
{
    VECTOR2I *loc = static_cast<VECTOR2I*>( event.GetClientData() );

    wxCHECK_RET( loc, wxT( "Missing focus cell location" ) );

    // Run the AutoColumnSizer before setting focus (as it will clear any shown cell edit control
    // if it has to resize that column).
    m_fieldsGrid->RecomputeGridWidths();

    // Handle a delayed focus

    m_fieldsGrid->SetFocus();
    m_fieldsGrid->MakeCellVisible( loc->x, loc->y );
    m_fieldsGrid->SetGridCursor( loc->x, loc->y );

    delete loc;

    CallAfter(
            [this]()
            {
                m_fieldsGrid->EnableCellEditControl( true );
            } );
}


void DIALOG_SYMBOL_PROPERTIES::HandleDelayedSelection( wxCommandEvent& event )
{
    VECTOR2I *loc = static_cast<VECTOR2I*>( event.GetClientData() );

    wxCHECK_RET( loc, wxT( "Missing focus cell location" ) );

    // Handle a delayed selection
    wxGridCellEditor* cellEditor = m_fieldsGrid->GetCellEditor( loc->x, loc->y );

    if( wxTextEntry* txt = dynamic_cast<wxTextEntry*>( cellEditor->GetControl() ) )
        KIUI::SelectReferenceNumber( txt );

    cellEditor->DecRef();   // we're done; must release
    delete loc;
}


void DIALOG_SYMBOL_PROPERTIES::OnSizePinsGrid( wxSizeEvent& event )
{
    wxSize new_size = event.GetSize();

    if( ( !m_editorShown || m_lastRequestedPinsSize != new_size ) && m_pinsSize != new_size )
    {
        m_pinsSize = new_size;

        AdjustPinsGridColumns();
    }

    // We store this value to check whether the dialog is changing size.  This might indicate
    // that the user is scaling the dialog with a grid-cell-editor shown.  Some editors do not
    // close (at least on GTK) when the user drags a dialog corner
    m_lastRequestedPinsSize = new_size;

    // Always propagate for a grid repaint (needed if the height changes, as well as width)
    event.Skip();
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


void DIALOG_SYMBOL_PROPERTIES::OnPageChanging( wxBookCtrlEvent& aEvent )
{
    if( !m_fieldsGrid->CommitPendingChanges() )
        aEvent.Veto();

    if( !m_pinGrid->CommitPendingChanges() )
        aEvent.Veto();
}
