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

#include <dialog_global_edit_text_and_graphics_base.h>
#include <string_utils.h>
#include <sch_symbol.h>
#include <sch_connection.h>
#include <sch_edit_frame.h>
#include <sch_shape.h>
#include <sch_line.h>
#include <sch_junction.h>
#include <sch_sheet.h>
#include <sch_sheet_pin.h>
#include <schematic.h>
#include <sch_commit.h>
#include <tool/tool_manager.h>
#include <tools/sch_selection_tool.h>
#include <tools/sch_edit_tool.h>
#include <widgets/unit_binder.h>
#include <widgets/font_choice.h>

static wxString   g_fieldnameFilter;
static wxString   g_referenceFilter;
static wxString   g_symbolFilter;
static wxString   g_netFilter;


class DIALOG_GLOBAL_EDIT_TEXT_AND_GRAPHICS : public DIALOG_GLOBAL_EDIT_TEXT_AND_GRAPHICS_BASE
{
public:
    DIALOG_GLOBAL_EDIT_TEXT_AND_GRAPHICS( SCH_EDIT_FRAME* parent );
    ~DIALOG_GLOBAL_EDIT_TEXT_AND_GRAPHICS() override;

protected:
    void OnReferenceFilterText( wxCommandEvent& event ) override
    {
        m_referenceFilterOpt->SetValue( true );
    }
    void OnSymbolFilterText( wxCommandEvent& event ) override
    {
        m_symbolFilterOpt->SetValue( true );
    }
    void OnFieldNameFilterText( wxCommandEvent& event ) override
    {
        m_fieldnameFilterOpt->SetValue( true );
    }
    void OnNetFilterText( wxCommandEvent& event ) override
    {
        m_netFilterOpt->SetValue( true );
    }

    bool TransferDataToWindow() override;
    bool TransferDataFromWindow() override;

    void visitItem( SCH_COMMIT* aCommit, const SCH_SHEET_PATH& aSheetPath, SCH_ITEM* aItem );
    void processItem( SCH_COMMIT* aCommit, const SCH_SHEET_PATH& aSheetPath, SCH_ITEM* aItem );

private:
    SCH_EDIT_FRAME*        m_parent;
    SCH_SELECTION          m_selection;

    UNIT_BINDER            m_textSize;
    UNIT_BINDER            m_lineWidth;
    UNIT_BINDER            m_junctionSize;
};


DIALOG_GLOBAL_EDIT_TEXT_AND_GRAPHICS::DIALOG_GLOBAL_EDIT_TEXT_AND_GRAPHICS( SCH_EDIT_FRAME* parent ) :
        DIALOG_GLOBAL_EDIT_TEXT_AND_GRAPHICS_BASE( parent ),
        m_textSize( parent, m_textSizeLabel, m_textSizeCtrl, m_textSizeUnits, true ),
        m_lineWidth( parent, m_lineWidthLabel, m_LineWidthCtrl, m_lineWidthUnits, true ),
        m_junctionSize( parent, m_dotSizeLabel, m_dotSizeCtrl, m_dotSizeUnits, true )
{
    m_parent = parent;

    m_lineStyle->Append( INDETERMINATE_ACTION );

    m_textColorSwatch->SetSwatchColor( COLOR4D::UNSPECIFIED, false );
    m_textColorSwatch->SetDefaultColor( COLOR4D::UNSPECIFIED );
    m_colorSwatch->SetSwatchColor( COLOR4D::UNSPECIFIED, false );
    m_colorSwatch->SetDefaultColor( COLOR4D::UNSPECIFIED );
    m_fillColorSwatch->SetSwatchColor( COLOR4D::UNSPECIFIED, false );
    m_fillColorSwatch->SetDefaultColor( COLOR4D::UNSPECIFIED );
    m_dotColorSwatch->SetSwatchColor( COLOR4D::UNSPECIFIED, false );
    m_dotColorSwatch->SetDefaultColor( COLOR4D::UNSPECIFIED );

    m_staticText12->SetFont( KIUI::GetSmallInfoFont( this ).Italic() );
    m_staticText13->SetFont( KIUI::GetSmallInfoFont( this ).Italic() );
    m_staticText14->SetFont( KIUI::GetSmallInfoFont( this ).Italic() );
    m_staticText15->SetFont( KIUI::GetSmallInfoFont( this ).Italic() );
    m_staticText16->SetFont( KIUI::GetSmallInfoFont( this ).Italic() );

    SetupStandardButtons();

    finishDialogSettings();
}


DIALOG_GLOBAL_EDIT_TEXT_AND_GRAPHICS::~DIALOG_GLOBAL_EDIT_TEXT_AND_GRAPHICS()
{
    g_fieldnameFilter = m_fieldnameFilter->GetValue();
    g_referenceFilter = m_referenceFilter->GetValue();
    g_symbolFilter = m_symbolFilter->GetValue();
    g_netFilter = m_netFilter->GetValue();
}


bool DIALOG_GLOBAL_EDIT_TEXT_AND_GRAPHICS::TransferDataToWindow()
{
    SCH_SELECTION_TOOL* selectionTool = m_parent->GetToolManager()->GetTool<SCH_SELECTION_TOOL>();
    m_selection = selectionTool->GetSelection();

    // SetValue() generates events, ChangeValue() does not
    m_fieldnameFilter->ChangeValue( g_fieldnameFilter );
    m_referenceFilter->ChangeValue( g_referenceFilter );
    m_symbolFilter->ChangeValue( g_symbolFilter );
    m_netFilter->ChangeValue( g_netFilter );

    if( g_netFilter.IsEmpty() && !m_parent->GetHighlightedConnection().IsEmpty() )
        m_netFilter->SetValue( m_parent->GetHighlightedConnection() );

    m_fontCtrl->Append( INDETERMINATE_ACTION );
    m_fontCtrl->SetStringSelection( INDETERMINATE_ACTION );

    m_textSize.SetValue( INDETERMINATE_ACTION );
    m_orientation->SetStringSelection( INDETERMINATE_ACTION );
    m_hAlign->SetStringSelection( INDETERMINATE_ACTION );
    m_vAlign->SetStringSelection( INDETERMINATE_ACTION );
    m_italic->Set3StateValue( wxCHK_UNDETERMINED );
    m_bold->Set3StateValue( wxCHK_UNDETERMINED );
    m_visible->Set3StateValue( wxCHK_UNDETERMINED );
    m_showFieldNames->Set3StateValue( wxCHK_UNDETERMINED );
    m_lineWidth.SetValue( INDETERMINATE_ACTION );
    m_lineStyle->SetStringSelection( INDETERMINATE_ACTION );
    m_junctionSize.SetValue( INDETERMINATE_ACTION );
    m_setColor->SetValue( false );
    m_setFillColor->SetValue( false );
    m_setDotColor->SetValue( false );

    return true;
}


void DIALOG_GLOBAL_EDIT_TEXT_AND_GRAPHICS::processItem( SCH_COMMIT* aCommit,
                                                        const SCH_SHEET_PATH& aSheetPath,
                                                        SCH_ITEM* aItem )
{
    if( m_selectedFilterOpt->GetValue() )
    {
        if( !aItem->IsSelected() && ( !aItem->GetParent() || !aItem->GetParent()->IsSelected() ) )
            return;
    }

    aCommit->Modify( aItem, aSheetPath.LastScreen() );

    if( EDA_TEXT* eda_text = dynamic_cast<EDA_TEXT*>( aItem ) )
    {
        if( !m_textSize.IsIndeterminate() )
            eda_text->SetTextSize( VECTOR2I( m_textSize.GetIntValue(), m_textSize.GetIntValue() ) );

        if( m_setTextColor->GetValue() )
            eda_text->SetTextColor( m_textColorSwatch->GetSwatchColor() );

        if( m_hAlign->GetStringSelection() != INDETERMINATE_ACTION )
        {
            GR_TEXT_H_ALIGN_T hAlign = EDA_TEXT::MapHorizJustify( m_hAlign->GetSelection() - 1 );
            SCH_SYMBOL*       parentSymbol = dynamic_cast<SCH_SYMBOL*>( aItem->GetParent() );

            if( parentSymbol && parentSymbol->GetTransform().x1 < 0 )
            {
                if( hAlign == GR_TEXT_H_ALIGN_LEFT )
                    hAlign = GR_TEXT_H_ALIGN_RIGHT;
                else if( hAlign == GR_TEXT_H_ALIGN_RIGHT )
                    hAlign = GR_TEXT_H_ALIGN_LEFT;
            }

            eda_text->SetHorizJustify( hAlign );
        }

        if( m_vAlign->GetStringSelection() != INDETERMINATE_ACTION )
        {
            GR_TEXT_V_ALIGN_T vAlign = EDA_TEXT::MapVertJustify( m_vAlign->GetSelection() - 1 );
            SCH_SYMBOL*       parentSymbol = dynamic_cast<SCH_SYMBOL*>( aItem->GetParent() );

            if( parentSymbol && parentSymbol->GetTransform().y1 < 0 )
            {
                if( vAlign == GR_TEXT_V_ALIGN_TOP )
                    vAlign = GR_TEXT_V_ALIGN_BOTTOM;
                else if( vAlign == GR_TEXT_V_ALIGN_BOTTOM )
                    vAlign = GR_TEXT_V_ALIGN_TOP;
            }

            eda_text->SetVertJustify( vAlign );
        }

        if( m_italic->Get3StateValue() != wxCHK_UNDETERMINED )
            eda_text->SetItalic( m_italic->GetValue() );

        // Must come after SetTextSize()
        if( m_bold->Get3StateValue() != wxCHK_UNDETERMINED )
            eda_text->SetBold( m_bold->GetValue() );

        // Must come after SetBold() & SetItalic()
        if( m_fontCtrl->GetStringSelection() != INDETERMINATE_ACTION )
        {
            eda_text->SetFont( m_fontCtrl->GetFontSelection( eda_text->IsBold(),
                                                             eda_text->IsItalic() ) );
        }
        else if( m_italic->Get3StateValue() != wxCHK_UNDETERMINED
                || m_bold->Get3StateValue() != wxCHK_UNDETERMINED )
        {
            if( !eda_text->GetFontName().IsEmpty() )
            {
                eda_text->SetFont( KIFONT::FONT::GetFont( eda_text->GetFontName(),
                                                          eda_text->IsBold(),
                                                          eda_text->IsItalic() ) );
            }
        }
    }

    if( SCH_LABEL_BASE* sch_label = dynamic_cast<SCH_LABEL_BASE*>( aItem ) )
    {
        if( m_orientation->GetStringSelection() != INDETERMINATE_ACTION )
            sch_label->SetSpinStyle( (SPIN_STYLE::SPIN) m_orientation->GetSelection() );
    }

    if( SCH_FIELD* sch_field = dynamic_cast<SCH_FIELD*>( aItem ) )
    {
        if( m_visible->Get3StateValue() != wxCHK_UNDETERMINED )
            sch_field->SetVisible( m_visible->GetValue() );

        if( m_showFieldNames->Get3StateValue() != wxCHK_UNDETERMINED )
            sch_field->SetNameShown( m_showFieldNames->GetValue() );
    }

    if( aItem->HasLineStroke() )
    {
        STROKE_PARAMS stroke = aItem->GetStroke();

        if( !m_lineWidth.IsIndeterminate() )
            stroke.SetWidth( m_lineWidth.GetIntValue() );

        if( m_lineStyle->GetStringSelection() != INDETERMINATE_ACTION )
            stroke.SetLineStyle( (LINE_STYLE) m_lineStyle->GetSelection() );

        if( m_setColor->GetValue() )
            stroke.SetColor( m_colorSwatch->GetSwatchColor() );

        aItem->SetStroke( stroke );
    }

    if( SCH_SHAPE* shape = dynamic_cast<SCH_SHAPE*>( aItem ) )
    {
        if( m_setFillColor->GetValue() )
        {
            shape->SetFillColor( m_fillColorSwatch->GetSwatchColor() );

            if( m_fillColorSwatch->GetSwatchColor() == COLOR4D::UNSPECIFIED )
                shape->SetFillMode( FILL_T::NO_FILL );
            else
                shape->SetFillMode( FILL_T::FILLED_WITH_COLOR );
        }
    }

    if( SCH_JUNCTION* junction = dynamic_cast<SCH_JUNCTION*>( aItem ) )
    {
        if( !m_junctionSize.IsIndeterminate() )
            junction->SetDiameter( m_junctionSize.GetIntValue() );

        if( m_setDotColor->GetValue() )
            junction->SetColor( m_dotColorSwatch->GetSwatchColor() );
    }
}

void DIALOG_GLOBAL_EDIT_TEXT_AND_GRAPHICS::visitItem( SCH_COMMIT* aCommit,
                                                      const SCH_SHEET_PATH& aSheetPath,
                                                      SCH_ITEM* aItem )
{
    if( m_netFilterOpt->GetValue() && !m_netFilter->GetValue().IsEmpty() )
    {
        SCH_CONNECTION* connection = aItem->Connection( &aSheetPath );

        if( !connection )
            return;

        if( !WildCompareString( m_netFilter->GetValue(), connection->Name(), false ) )
            return;
    }

    if( m_referenceFilterOpt->GetValue() && !m_referenceFilter->GetValue().IsEmpty() )
    {
        if( aItem->Type() == SCH_SYMBOL_T )
        {
            wxString ref = static_cast<SCH_SYMBOL*>( aItem )->GetRef( &aSheetPath );

            if( !WildCompareString( m_referenceFilter->GetValue(), ref, false ) )
                return;
        }
    }

    if( m_symbolFilterOpt->GetValue() && !m_symbolFilter->GetValue().IsEmpty() )
    {
        if( aItem->Type() == SCH_SYMBOL_T )
        {
            wxString id = UnescapeString( static_cast<SCH_SYMBOL*>( aItem )->GetLibId().Format() );

            if( !WildCompareString( m_symbolFilter->GetValue(), id, false ) )
                return;
        }
    }

    if( m_typeFilterOpt->GetValue() )
    {
        if( aItem->Type() == SCH_SYMBOL_T )
        {
            bool isPower = static_cast<SCH_SYMBOL*>( aItem )->GetLibSymbolRef()->IsPower();

            if( isPower != ( m_typeFilter->GetSelection() == 1 ) )
                return;
        }
    }

    static const std::vector<KICAD_T> wireLabelTypes = { SCH_LABEL_LOCATE_WIRE_T };
    static const std::vector<KICAD_T> busLabelTypes = { SCH_LABEL_LOCATE_BUS_T };

    switch( aItem->Type() )
    {
    case SCH_SYMBOL_T:
    {
        SCH_SYMBOL* symbol = (SCH_SYMBOL*) aItem;

        if( m_references->GetValue() )
            processItem( aCommit, aSheetPath, symbol->GetField( FIELD_T::REFERENCE ) );

        if( m_values->GetValue() )
            processItem( aCommit, aSheetPath, symbol->GetField( FIELD_T::VALUE ) );

        if( m_otherFields->GetValue() )
        {
            for( SCH_FIELD& field : symbol->GetFields() )
            {
                if( field.GetId() == FIELD_T::REFERENCE || field.GetId() == FIELD_T::VALUE )
                    continue;

                const wxString& fieldName = field.GetName();

                if( !m_fieldnameFilterOpt->GetValue() || m_fieldnameFilter->GetValue().IsEmpty()
                        || WildCompareString( m_fieldnameFilter->GetValue(), fieldName, false ) )
                {
                    processItem( aCommit, aSheetPath, &field );
                }
            }
        }

        break;
    }

    case SCH_SHEET_T:
    {
        SCH_SHEET* sheet = static_cast<SCH_SHEET*>( aItem );

        if( m_sheetTitles->GetValue() )
            processItem( aCommit, aSheetPath, sheet->GetField( FIELD_T::SHEET_NAME ) );

        if( m_sheetFields->GetValue() )
        {
            for( SCH_FIELD& field : sheet->GetFields() )
            {
                if( field.GetId() == FIELD_T::SHEET_NAME )
                    continue;

                const wxString& fieldName = field.GetName();

                if( !m_fieldnameFilterOpt->GetValue() || m_fieldnameFilter->GetValue().IsEmpty()
                        || WildCompareString( m_fieldnameFilter->GetValue(), fieldName, false ) )
                {
                    processItem( aCommit, aSheetPath, &field );
                }
            }
        }

        if( m_sheetBorders->GetValue() )
        {
            if( !m_lineWidth.IsIndeterminate() )
                sheet->SetBorderWidth( m_lineWidth.GetIntValue() );

            if( m_setColor->GetValue() )
                sheet->SetBorderColor( m_colorSwatch->GetSwatchColor() );

            if( m_setFillColor->GetValue() )
                sheet->SetBackgroundColor( m_fillColorSwatch->GetSwatchColor() );
        }

        if( m_sheetPins->GetValue() )
        {
            for( SCH_SHEET_PIN* pin : sheet->GetPins() )
                processItem( aCommit, aSheetPath, pin );
        }

        break;
    }

    case SCH_LINE_T:
        if( m_schTextAndGraphics->GetValue() && aItem->GetLayer() == LAYER_NOTES )
            processItem( aCommit, aSheetPath, aItem );
        else if( m_wires->GetValue() && aItem->GetLayer() == LAYER_WIRE )
            processItem( aCommit, aSheetPath, aItem );
        else if( m_buses->GetValue() && aItem->GetLayer() == LAYER_BUS )
            processItem( aCommit, aSheetPath, aItem );

        break;

    case SCH_LABEL_T:
    case SCH_GLOBAL_LABEL_T:
    case SCH_HIER_LABEL_T:
    case SCH_DIRECTIVE_LABEL_T:
        if( m_wires->GetValue() && aItem->IsType( wireLabelTypes ) )
            processItem( aCommit, aSheetPath, aItem );

        if( m_buses->GetValue() && aItem->IsType( busLabelTypes ) )
            processItem( aCommit, aSheetPath, aItem );

        if( m_globalLabels->GetValue() && aItem->Type() == SCH_GLOBAL_LABEL_T )
            processItem( aCommit, aSheetPath, aItem );

        if( m_hierLabels->GetValue() && aItem->Type() == SCH_HIER_LABEL_T )
            processItem( aCommit, aSheetPath, aItem );

        if( m_labelFields->GetValue() )
        {
            for( SCH_FIELD& field : static_cast<SCH_LABEL_BASE*>( aItem )->GetFields() )
            {
                const wxString& fieldName = field.GetName();

                if( !m_fieldnameFilterOpt->GetValue() || m_fieldnameFilter->GetValue().IsEmpty()
                        || WildCompareString( m_fieldnameFilter->GetValue(), fieldName, false ) )
                {
                    processItem( aCommit, aSheetPath, &field );
                }
            }
        }

        break;

    case SCH_JUNCTION_T:
    {
        SCH_JUNCTION* junction = static_cast<SCH_JUNCTION*>( aItem );

        for( SCH_ITEM* item : junction->ConnectedItems( aSheetPath ) )
        {
            if( item->GetLayer() == LAYER_BUS && m_buses->GetValue() )
            {
                processItem( aCommit, aSheetPath, aItem );
                break;
            }
            else if( item->GetLayer() == LAYER_WIRE && m_wires->GetValue() )
            {
                processItem( aCommit, aSheetPath, aItem );
                break;
            }
        }

        break;
    }

    case SCH_TEXT_T:
    case SCH_TEXTBOX_T:
    case SCH_SHAPE_T:
        if( m_schTextAndGraphics->GetValue() )
            processItem( aCommit, aSheetPath, aItem );

        break;

    default:
        break;
    }
}


bool DIALOG_GLOBAL_EDIT_TEXT_AND_GRAPHICS::TransferDataFromWindow()
{
    if( !m_textSize.Validate( 1.0, 10000.0, EDA_UNITS::MILS ) )  // 1 mil .. 10 inches
        return false;

    SCH_SHEET_PATH currentSheet = m_parent->GetCurrentSheet();
    SCH_COMMIT     commit( m_parent );

    // Go through sheets
    for( const SCH_SHEET_PATH& sheetPath : m_parent->Schematic().Hierarchy() )
    {
        SCH_SCREEN* screen = sheetPath.LastScreen();

        if( screen )
        {
            m_parent->SetCurrentSheet( sheetPath );

            for( SCH_ITEM* item : screen->Items() )
                visitItem( &commit, sheetPath, item );
        }
    }

    if( !commit.Empty() )
    {
        commit.Push( _( "Edit Text and Graphics" ) );
        m_parent->HardRedraw();
    }

    // Reset the view to where we left the user
    m_parent->SetCurrentSheet( currentSheet );
    m_parent->Refresh();

    return true;
}


int SCH_EDIT_TOOL::GlobalEdit( const TOOL_EVENT& aEvent )
{
    DIALOG_GLOBAL_EDIT_TEXT_AND_GRAPHICS dlg( m_frame );
    dlg.ShowModal();
    return 0;
}


