/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2019-2021 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <connection_graph.h>
#include <dialog_global_edit_text_and_graphics_base.h>
#include <kicad_string.h>
#include <sch_symbol.h>
#include <sch_connection.h>
#include <sch_edit_frame.h>
#include <sch_line.h>
#include <sch_junction.h>
#include <sch_sheet.h>
#include <schematic.h>
#include <advanced_config.h>
#include <tool/tool_manager.h>
#include <tools/ee_selection_tool.h>
#include <tools/sch_edit_tool.h>
#include <widgets/unit_binder.h>

static bool       g_modifyReferences;
static bool       g_modifyValues;
static bool       g_modifyOtherFields;
static bool       g_modifyWires;
static bool       g_modifyBuses;
static bool       g_modifyGlobalLabels;
static bool       g_modifyHierLabels;
static bool       g_modifySheetTitles;
static bool       g_modifyOtherSheetFields;
static bool       g_modifySheetPins;
static bool       g_modifySheetBorders;
static bool       g_modifySchTextAndGraphics;

static bool       g_filterByFieldname;
static wxString   g_fieldnameFilter;
static bool       g_filterByReference;
static wxString   g_referenceFilter;
static bool       g_filterBySymbol;
static wxString   g_symbolFilter;
static bool       g_filterByType;
static bool       g_typeFilterIsPower;
static bool       g_filterByNet;
static wxString   g_netFilter;
static bool       g_filterSelected;


#define DEFAULT_STYLE _( "Default" )

class DIALOG_GLOBAL_EDIT_TEXT_AND_GRAPHICS : public DIALOG_GLOBAL_EDIT_TEXT_AND_GRAPHICS_BASE
{
    SCH_EDIT_FRAME*        m_parent;
    EE_SELECTION           m_selection;

    UNIT_BINDER            m_textSize;
    UNIT_BINDER            m_lineWidth;

    bool                   m_hasChange;

public:
    DIALOG_GLOBAL_EDIT_TEXT_AND_GRAPHICS( SCH_EDIT_FRAME* parent );
    ~DIALOG_GLOBAL_EDIT_TEXT_AND_GRAPHICS() override;

protected:
    void OnUpdateUI( wxUpdateUIEvent& event ) override;
    void OnReferenceFilterText( wxCommandEvent& event ) override
    {
        m_referenceFilterOpt->SetValue( true );
    }
    void OnSymbolFilterText( wxCommandEvent& event ) override
    {
        m_symbolFilterOpt->SetValue( true );
    }

    bool TransferDataToWindow() override;
    bool TransferDataFromWindow() override;

    void visitItem( const SCH_SHEET_PATH& aSheetPath, SCH_ITEM* aItem );
    void processItem( const SCH_SHEET_PATH& aSheetPath, SCH_ITEM* aItem );
};


DIALOG_GLOBAL_EDIT_TEXT_AND_GRAPHICS::DIALOG_GLOBAL_EDIT_TEXT_AND_GRAPHICS( SCH_EDIT_FRAME* parent ) :
        DIALOG_GLOBAL_EDIT_TEXT_AND_GRAPHICS_BASE( parent ),
        m_textSize( parent, m_textSizeLabel, m_textSizeCtrl, m_textSizeUnits, true ),
        m_lineWidth( parent, m_lineWidthLabel, m_LineWidthCtrl, m_lineWidthUnits, true )
{
    m_parent = parent;
    m_hasChange = false;

    // TODO(JE) remove once real-time connectivity is a given
    if( !ADVANCED_CFG::GetCfg().m_RealTimeConnectivity || !CONNECTION_GRAPH::m_allowRealTime )
        m_parent->RecalculateConnections( NO_CLEANUP );

    m_lineStyle->Append( DEFAULT_STYLE );
    m_lineStyle->Append( INDETERMINATE_ACTION );

    m_colorSwatch->SetSwatchColor( COLOR4D::UNSPECIFIED, false );
    m_colorSwatch->SetDefaultColor( COLOR4D::UNSPECIFIED );
    m_bgColorSwatch->SetSwatchColor( COLOR4D::UNSPECIFIED, false );
    m_bgColorSwatch->SetDefaultColor( COLOR4D::UNSPECIFIED );

    m_sdbSizerButtonsOK->SetDefault();

    finishDialogSettings();
}


DIALOG_GLOBAL_EDIT_TEXT_AND_GRAPHICS::~DIALOG_GLOBAL_EDIT_TEXT_AND_GRAPHICS()
{
    g_modifyReferences = m_references->GetValue();
    g_modifyValues = m_values->GetValue();
    g_modifyOtherFields = m_otherFields->GetValue();
    g_modifyWires = m_wires->GetValue();
    g_modifyBuses = m_buses->GetValue();
    g_modifyGlobalLabels = m_globalLabels->GetValue();
    g_modifyHierLabels = m_hierLabels->GetValue();
    g_modifySheetTitles = m_sheetTitles->GetValue();
    g_modifyOtherSheetFields = m_sheetFields->GetValue();
    g_modifySheetPins = m_sheetPins->GetValue();
    g_modifySheetBorders = m_sheetBorders->GetValue();
    g_modifySchTextAndGraphics = m_schTextAndGraphics->GetValue();

    g_filterByFieldname = m_fieldnameFilterOpt->GetValue();
    g_fieldnameFilter = m_fieldnameFilter->GetValue();
    g_filterByReference = m_referenceFilterOpt->GetValue();
    g_referenceFilter = m_referenceFilter->GetValue();
    g_filterBySymbol = m_symbolFilterOpt->GetValue();
    g_symbolFilter = m_symbolFilter->GetValue();
    g_filterByType = m_typeFilterOpt->GetValue();
    g_typeFilterIsPower = m_typeFilter->GetSelection() == 1;
    g_filterByNet = m_netFilterOpt->GetValue();
    g_netFilter = m_netFilter->GetValue();
    g_filterSelected = m_selectedFilterOpt->GetValue();
}


bool DIALOG_GLOBAL_EDIT_TEXT_AND_GRAPHICS::TransferDataToWindow()
{
    EE_SELECTION_TOOL* selectionTool = m_parent->GetToolManager()->GetTool<EE_SELECTION_TOOL>();
    m_selection = selectionTool->GetSelection();

    m_references->SetValue( g_modifyReferences );
    m_values->SetValue( g_modifyValues );
    m_otherFields->SetValue( g_modifyOtherFields );
    m_wires->SetValue( g_modifyWires );
    m_buses->SetValue( g_modifyBuses );
    m_globalLabels->SetValue( g_modifyGlobalLabels );
    m_hierLabels->SetValue( g_modifyHierLabels );
    m_sheetTitles->SetValue( g_modifySheetTitles );
    m_sheetFields->SetValue( g_modifyOtherSheetFields );
    m_sheetPins->SetValue( g_modifySheetPins );
    m_sheetBorders->SetValue( g_modifySheetBorders );
    m_schTextAndGraphics->SetValue( g_modifySchTextAndGraphics );

    // SetValue() generates events, ChangeValue() does not
    m_fieldnameFilter->ChangeValue( g_fieldnameFilter );
    m_fieldnameFilterOpt->SetValue( g_filterByFieldname );
    m_referenceFilter->ChangeValue( g_referenceFilter );
    m_referenceFilterOpt->SetValue( g_filterByReference );
    m_symbolFilter->ChangeValue( g_symbolFilter );
    m_symbolFilterOpt->SetValue( g_filterBySymbol );
    m_typeFilter->SetSelection( g_typeFilterIsPower ? 1 : 0 );
    m_typeFilterOpt->SetValue( g_filterByType );
    m_selectedFilterOpt->SetValue( g_filterSelected );

    if( g_filterByNet && !g_netFilter.IsEmpty() )
    {
        m_netFilter->SetValue( g_netFilter );
        m_netFilterOpt->SetValue( true );
    }
    else if( m_parent->GetHighlightedConnection() )
    {
        m_netFilter->SetValue( m_parent->GetHighlightedConnection()->Name() );
    }
    else if( m_selection.GetSize() )
    {
        SCH_ITEM* sch_item = (SCH_ITEM*) m_selection.Front();
        SCH_CONNECTION* connection = sch_item->Connection();

        if( connection )
            m_netFilter->SetValue( connection->Name() );
    }

    m_netFilterOpt->SetValue( g_filterByNet );

    m_textSize.SetValue( INDETERMINATE_ACTION );
    m_orientation->SetStringSelection( INDETERMINATE_ACTION );
    m_hAlign->SetStringSelection( INDETERMINATE_ACTION );
    m_vAlign->SetStringSelection( INDETERMINATE_ACTION );
    m_Italic->Set3StateValue( wxCHK_UNDETERMINED );
    m_Bold->Set3StateValue( wxCHK_UNDETERMINED );
    m_Visible->Set3StateValue( wxCHK_UNDETERMINED );
    m_lineWidth.SetValue( INDETERMINATE_ACTION );
    m_lineStyle->SetStringSelection( INDETERMINATE_ACTION );
    m_setColor->SetValue( false );
    m_setBgColor->SetValue( false );

    return true;
}


void DIALOG_GLOBAL_EDIT_TEXT_AND_GRAPHICS::OnUpdateUI( wxUpdateUIEvent&  )
{
}


void DIALOG_GLOBAL_EDIT_TEXT_AND_GRAPHICS::processItem( const SCH_SHEET_PATH& aSheetPath,
                                                        SCH_ITEM* aItem )
{
    EDA_TEXT* eda_text = dynamic_cast<EDA_TEXT*>( aItem );
    SCH_TEXT* sch_text = dynamic_cast<SCH_TEXT*>( aItem );
    SCH_LINE* lineItem = dynamic_cast<SCH_LINE*>( aItem );

    m_parent->SaveCopyInUndoList( aSheetPath.LastScreen(), aItem, UNDO_REDO::CHANGED, m_hasChange );

    if( eda_text )
    {
        if( !m_textSize.IsIndeterminate() )
        {
            eda_text->SetTextSize( wxSize( m_textSize.GetValue(), m_textSize.GetValue() ) );
            m_hasChange = true;
        }

        if( m_hAlign->GetStringSelection() != INDETERMINATE_ACTION )
        {
            eda_text->SetHorizJustify( EDA_TEXT::MapHorizJustify( m_hAlign->GetSelection() - 1 ) );
            m_hasChange = true;
        }

        if( m_hAlign->GetStringSelection() != INDETERMINATE_ACTION )
        {
            eda_text->SetVertJustify( EDA_TEXT::MapVertJustify( m_vAlign->GetSelection() - 1 ) );
            m_hasChange = true;
        }

        if( m_Visible->Get3StateValue() != wxCHK_UNDETERMINED )
        {
            eda_text->SetVisible( m_Visible->GetValue() );
            m_hasChange = true;
        }

        if( m_Italic->Get3StateValue() != wxCHK_UNDETERMINED )
        {
            eda_text->SetItalic( m_Italic->GetValue() );
            m_hasChange = true;
        }

        if( m_Bold->Get3StateValue() != wxCHK_UNDETERMINED )
        {
            eda_text->SetBold( m_Bold->GetValue() );
            m_hasChange = true;
        }
    }

    // No else!  Labels are both.
    if( sch_text )
    {
        if( m_orientation->GetStringSelection() != INDETERMINATE_ACTION )
        {
            sch_text->SetLabelSpinStyle( (LABEL_SPIN_STYLE::SPIN) m_orientation->GetSelection() );
            m_hasChange = true;
        }
    }

    if( lineItem )
    {
        if( !m_lineWidth.IsIndeterminate() )
        {
            lineItem->SetLineWidth( m_lineWidth.GetValue() );
            m_hasChange = true;
        }

        if( m_lineStyle->GetStringSelection() != INDETERMINATE_ACTION )
        {
            if( m_lineStyle->GetStringSelection() == DEFAULT_STYLE )
                lineItem->SetLineStyle( PLOT_DASH_TYPE::DEFAULT );
            else
                lineItem->SetLineStyle( m_lineStyle->GetSelection() );

            m_hasChange = true;
        }

        if( m_setColor->GetValue() )
        {
            lineItem->SetLineColor( m_colorSwatch->GetSwatchColor() );
            m_hasChange = true;
        }
    }
}

void DIALOG_GLOBAL_EDIT_TEXT_AND_GRAPHICS::visitItem( const SCH_SHEET_PATH& aSheetPath,
                                                      SCH_ITEM* aItem )
{
    if( m_selectedFilterOpt->GetValue() && !m_selection.Contains( aItem ) )
    {
        return;
    }

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
        if( aItem->Type() == SCH_COMPONENT_T )
        {
            wxString ref = static_cast<SCH_COMPONENT*>( aItem )->GetRef( &aSheetPath );

            if( !WildCompareString( m_referenceFilter->GetValue(), ref, false ) )
                return;
        }
    }

    if( m_symbolFilterOpt->GetValue() && !m_symbolFilter->GetValue().IsEmpty() )
    {
        if( aItem->Type() == SCH_COMPONENT_T )
        {
            wxString id = static_cast<SCH_COMPONENT*>( aItem )->GetLibId().Format();

            if( !WildCompareString( m_symbolFilter->GetValue(), id, false ) )
                return;
        }
    }

    if( m_typeFilterOpt->GetValue() )
    {
        if( aItem->Type() == SCH_COMPONENT_T )
        {
            bool isPower = static_cast<SCH_COMPONENT*>( aItem )->GetPartRef()->IsPower();

            if( isPower != ( m_typeFilter->GetSelection() == 1 ) )
                return;
        }
    }

    static KICAD_T wireTypes[] = { SCH_LINE_LOCATE_WIRE_T, SCH_LABEL_LOCATE_WIRE_T, EOT };
    static KICAD_T busTypes[] = { SCH_LINE_LOCATE_BUS_T, SCH_LABEL_LOCATE_BUS_T, EOT };
    static KICAD_T schTextAndGraphics[] = { SCH_TEXT_T, SCH_LINE_LOCATE_GRAPHIC_LINE_T, EOT };

    if( aItem->Type() == SCH_COMPONENT_T )
    {
        SCH_COMPONENT* component = (SCH_COMPONENT*) aItem;

        if( m_references->GetValue() )
            processItem( aSheetPath, component->GetField( REFERENCE_FIELD ) );

        if( m_values->GetValue() )
            processItem( aSheetPath, component->GetField( VALUE_FIELD ) );

        if( m_otherFields->GetValue() )
        {
            for( int i = 2; i < component->GetFieldCount(); ++i )
            {
                SCH_FIELD* field = component->GetField( i );
                const wxString& fieldName = field->GetName();

                if( !m_fieldnameFilterOpt->GetValue() || m_fieldnameFilter->GetValue().IsEmpty()
                        || WildCompareString( m_fieldnameFilter->GetValue(), fieldName, false ) )
                {
                    processItem( aSheetPath, field );
                }
            }
        }
    }
    else if( aItem->Type() == SCH_SHEET_T )
    {
        SCH_SHEET* sheet = static_cast<SCH_SHEET*>( aItem );

        if( m_sheetTitles->GetValue() )
            processItem( aSheetPath, &sheet->GetFields()[ SHEETNAME ] );

        if( m_sheetFields->GetValue() )
        {
            for( SCH_FIELD& field : sheet->GetFields() )
            {
                if( field.GetId() == SHEETNAME )
                    continue;

                const wxString& fieldName = field.GetName();

                if( !m_fieldnameFilterOpt->GetValue() || m_fieldnameFilter->GetValue().IsEmpty()
                        || WildCompareString( m_fieldnameFilter->GetValue(), fieldName, false ) )
                {
                    processItem( aSheetPath, &field );
                }
            }
        }

        if( m_sheetBorders->GetValue() )
        {
            if( !m_lineWidth.IsIndeterminate() )
            {
                sheet->SetBorderWidth( m_lineWidth.GetValue() );
                m_hasChange = true;
            }

            if( m_setColor->GetValue() )
            {
                sheet->SetBorderColor( m_colorSwatch->GetSwatchColor() );
                m_hasChange = true;
            }

            if( m_setBgColor->GetValue() )
            {
                sheet->SetBackgroundColor( m_bgColorSwatch->GetSwatchColor() );
                m_hasChange = true;
            }
        }
    }
    else if( aItem->Type() == SCH_JUNCTION_T )
    {
        SCH_JUNCTION* junction = static_cast<SCH_JUNCTION*>( aItem );

        for( SCH_ITEM* item : junction->ConnectedItems( aSheetPath ) )
        {
            if( item->GetLayer() == LAYER_BUS )
            {
                if( m_buses->GetValue() && m_setColor->GetValue() )
                {
                    junction->SetColor( m_colorSwatch->GetSwatchColor() );
                    m_hasChange = true;
                }
                break;
            }
            else if( item->GetLayer() == LAYER_WIRE )
            {
                if( m_wires->GetValue() && m_setColor->GetValue() )
                {
                    junction->SetColor( m_colorSwatch->GetSwatchColor() );
                    m_hasChange = true;
                }
                break;
            }
        }
    }
    else if( m_wires->GetValue() && aItem->IsType( wireTypes ) )
        processItem( aSheetPath, aItem );
    else if( m_buses->GetValue() && aItem->IsType( busTypes ) )
        processItem( aSheetPath, aItem );
    else if( m_globalLabels->GetValue() && aItem->Type() == SCH_GLOBAL_LABEL_T )
        processItem( aSheetPath, aItem );
    else if( m_hierLabels->GetValue() && aItem->Type() == SCH_HIER_LABEL_T )
        processItem( aSheetPath, aItem );
    else if( m_sheetPins->GetValue() && aItem->Type() == SCH_SHEET_PIN_T )
        processItem( aSheetPath, aItem );
    else if( m_schTextAndGraphics->GetValue() && aItem->IsType( schTextAndGraphics ) )
        processItem( aSheetPath, aItem );
}


bool DIALOG_GLOBAL_EDIT_TEXT_AND_GRAPHICS::TransferDataFromWindow()
{
    if( !m_textSize.Validate( 1.0, 10000.0, EDA_UNITS::MILS ) )  // 1 mil .. 10 inches
        return false;

    SCH_SHEET_PATH currentSheet = m_parent->GetCurrentSheet();

    // Go through sheets
    for( const SCH_SHEET_PATH& sheetPath : m_parent->Schematic().GetSheets() )
    {
        SCH_SCREEN* screen = sheetPath.LastScreen();

        if( screen )
        {
            m_parent->SetCurrentSheet( sheetPath );
            m_hasChange = false;

            for( auto item : screen->Items() )
                visitItem( sheetPath, item );

            if( m_hasChange )
            {
                m_parent->OnModify();
                m_parent->HardRedraw();
            }
        }
    }

    // Reset the view to where we left the user
    m_parent->SetCurrentSheet( currentSheet );

    return true;
}


int SCH_EDIT_TOOL::GlobalEdit( const TOOL_EVENT& aEvent )
{
    DIALOG_GLOBAL_EDIT_TEXT_AND_GRAPHICS dlg( m_frame );
    dlg.ShowModal();
    return 0;
}


