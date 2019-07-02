/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2019 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <fctsys.h>
#include <kicad_string.h>
#include <widgets/unit_binder.h>
#include <tool/tool_manager.h>
#include <tools/ee_selection_tool.h>
#include <tools/sch_edit_tool.h>
#include <sch_edit_frame.h>
#include <sch_component.h>
#include <sch_line.h>
#include <sch_connection.h>
#include <sch_sheet.h>
#include <connection_graph.h>
#include <dialog_global_edit_text_and_graphics_base.h>
#include <advanced_config.h>

static bool       g_modifyReferences;
static bool       g_modifyValues;
static bool       g_modifyOtherFields;
static bool       g_modifyWires;
static bool       g_modifyBusses;
static bool       g_modifyGlobalLabels;
static bool       g_modifyHierLabels;
static bool       g_modifySheetTitles;
static bool       g_modifySheetPins;
static bool       g_modifySchTextAndGraphics;

static bool       g_filterByFieldname;
static wxString   g_fieldnameFilter;
static bool       g_filterByReference;
static wxString   g_referenceFilter;
static bool       g_filterBySymbol;
static wxString   g_symbolFilter;
static bool       g_filterByNet;
static wxString   g_netFilter;


class DIALOG_GLOBAL_EDIT_TEXT_AND_GRAPHICS : public DIALOG_GLOBAL_EDIT_TEXT_AND_GRAPHICS_BASE
{
    SCH_EDIT_FRAME*        m_parent;

    UNIT_BINDER            m_textSize;
    UNIT_BINDER            m_lineWidth;

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

    // TODO(JE) remove once real-time connectivity is a given
    if( !ADVANCED_CFG::GetCfg().m_realTimeConnectivity || !CONNECTION_GRAPH::m_allowRealTime )
        m_parent->RecalculateConnections();

    m_sdbSizerButtonsOK->SetDefault();

    FinishDialogSettings();
}


DIALOG_GLOBAL_EDIT_TEXT_AND_GRAPHICS::~DIALOG_GLOBAL_EDIT_TEXT_AND_GRAPHICS()
{
    g_modifyReferences = m_references->GetValue();
    g_modifyValues = m_values->GetValue();
    g_modifyOtherFields = m_otherFields->GetValue();
    g_modifyWires = m_wires->GetValue();
    g_modifyBusses = m_busses->GetValue();
    g_modifyGlobalLabels = m_globalLabels->GetValue();
    g_modifyHierLabels = m_hierLabels->GetValue();
    g_modifySheetTitles = m_sheetTitles->GetValue();
    g_modifySheetPins = m_sheetPins->GetValue();
    g_modifySchTextAndGraphics = m_schTextAndGraphics->GetValue();

    g_filterByFieldname = m_fieldnameFilterOpt->GetValue();
    g_fieldnameFilter = m_fieldnameFilter->GetValue();
    g_filterByReference = m_referenceFilterOpt->GetValue();
    g_referenceFilter = m_referenceFilter->GetValue();
    g_filterBySymbol = m_symbolFilterOpt->GetValue();
    g_symbolFilter = m_symbolFilter->GetValue();
    g_filterByNet = m_netFilterOpt->GetValue();
    g_netFilter = m_netFilter->GetValue();
}


bool DIALOG_GLOBAL_EDIT_TEXT_AND_GRAPHICS::TransferDataToWindow()
{
    EE_SELECTION_TOOL* selectionTool = m_parent->GetToolManager()->GetTool<EE_SELECTION_TOOL>();
    SELECTION&         selection = selectionTool->GetSelection();

    m_references->SetValue( g_modifyReferences );
    m_values->SetValue( g_modifyValues );
    m_otherFields->SetValue( g_modifyOtherFields );
    m_wires->SetValue( g_modifyWires );
    m_busses->SetValue( g_modifyBusses );
    m_globalLabels->SetValue( g_modifyGlobalLabels );
    m_hierLabels->SetValue( g_modifyHierLabels );
    m_sheetTitles->SetValue( g_modifySheetTitles );
    m_sheetPins->SetValue( g_modifySheetPins );
    m_schTextAndGraphics->SetValue( g_modifySchTextAndGraphics );

    // SetValue() generates events, ChangeValue() does not
    m_fieldnameFilter->ChangeValue( g_fieldnameFilter );
    m_fieldnameFilterOpt->SetValue( g_filterByFieldname );
    m_referenceFilter->ChangeValue( g_referenceFilter );
    m_referenceFilterOpt->SetValue( g_filterByReference );
    m_symbolFilter->ChangeValue( g_symbolFilter );
    m_symbolFilterOpt->SetValue( g_filterBySymbol );

    if( g_filterByNet && !g_netFilter.IsEmpty() )
    {
        m_netFilter->SetValue( g_netFilter );
        m_netFilterOpt->SetValue( true );
    }
    else if( !m_parent->GetSelectedNetName().IsEmpty() )
    {
        m_netFilter->SetValue( m_parent->GetSelectedNetName() );
    }
    else if( selection.GetSize() )
    {
        SCH_ITEM* sch_item = (SCH_ITEM*) selection.Front();
        SCH_CONNECTION* connection = sch_item->Connection( *g_CurrentSheet );

        if( connection )
            m_netFilter->SetValue( connection->Name() );
    }

    m_netFilterOpt->SetValue( g_filterByNet );

    m_textSize.SetValue( INDETERMINATE );
    m_orientation->SetStringSelection( INDETERMINATE );
    m_hAlign->SetStringSelection( INDETERMINATE );
    m_vAlign->SetStringSelection( INDETERMINATE );
    m_Italic->Set3StateValue( wxCHK_UNDETERMINED );
    m_Bold->Set3StateValue( wxCHK_UNDETERMINED );
    m_Visible->Set3StateValue( wxCHK_UNDETERMINED );
    m_lineWidth.SetValue( INDETERMINATE );
    m_lineStyle->SetStringSelection( INDETERMINATE );
    m_setColor->SetValue( false );

    return true;
}


void DIALOG_GLOBAL_EDIT_TEXT_AND_GRAPHICS::OnUpdateUI( wxUpdateUIEvent&  )
{
}


void DIALOG_GLOBAL_EDIT_TEXT_AND_GRAPHICS::processItem( const SCH_SHEET_PATH& aSheetPath,
                                                        SCH_ITEM* aItem )
{
    auto eda_text = dynamic_cast<EDA_TEXT*>( aItem );
    auto sch_text = dynamic_cast<SCH_TEXT*>( aItem );
    auto lineItem = dynamic_cast<SCH_LINE*>( aItem );

    if( eda_text )
    {
        if( !m_textSize.IsIndeterminate() )
            eda_text->SetTextSize( wxSize( m_textSize.GetValue(), m_textSize.GetValue() ) );

        if( m_hAlign->GetStringSelection() != INDETERMINATE )
            eda_text->SetHorizJustify( EDA_TEXT::MapHorizJustify( m_hAlign->GetSelection() - 1 ) );

        if( m_hAlign->GetStringSelection() != INDETERMINATE )
            eda_text->SetVertJustify( EDA_TEXT::MapVertJustify( m_vAlign->GetSelection() - 1 ) );

        if( m_Visible->Get3StateValue() != wxCHK_UNDETERMINED )
            eda_text->SetVisible( m_Visible->GetValue() );

        if( m_Italic->Get3StateValue() != wxCHK_UNDETERMINED )
            eda_text->SetItalic( m_Visible->GetValue() );

        if( m_Bold->Get3StateValue() != wxCHK_UNDETERMINED )
            eda_text->SetBold( m_Visible->GetValue() );
    }

    // No else!  Labels are both.
    if( sch_text )
    {
        if( m_orientation->GetStringSelection() != INDETERMINATE )
        {
            int orient = m_orientation->GetSelection();
            sch_text->SetLabelSpinStyle( EDA_TEXT::MapOrientation( sch_text->Type(), orient ) );
        }
    }

    if( lineItem )
    {
        if( !m_lineWidth.IsIndeterminate() )
            lineItem->SetLineWidth( m_lineWidth.GetValue() );

        if( lineItem->GetLayer() == LAYER_NOTES )
        {
            if( m_lineStyle->GetStringSelection() != INDETERMINATE )
                lineItem->SetLineStyle( m_lineStyle->GetSelection() );

            if( m_setColor->GetValue() )
                lineItem->SetLineColor( m_color->GetColour() );
        }
    }

    m_parent->OnModify();
}

void DIALOG_GLOBAL_EDIT_TEXT_AND_GRAPHICS::visitItem( const SCH_SHEET_PATH& aSheetPath,
                                                      SCH_ITEM* aItem )
{
    if( m_netFilterOpt->GetValue() && !m_netFilter->GetValue().IsEmpty() )
    {
        SCH_CONNECTION* connection = aItem->Connection( aSheetPath );

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

    static KICAD_T wireTypes[] = { SCH_LINE_LOCATE_WIRE_T, SCH_LABEL_LOCATE_WIRE_T, EOT };
    static KICAD_T busTypes[] = { SCH_LINE_LOCATE_BUS_T, SCH_LABEL_LOCATE_BUS_T, EOT };
    static KICAD_T schTextAndGraphics[] = { SCH_TEXT_T, SCH_LINE_LOCATE_GRAPHIC_LINE_T, EOT };

    if( aItem->Type() == SCH_COMPONENT_T )
    {
        SCH_COMPONENT* component = (SCH_COMPONENT*) aItem;

        if( m_references->GetValue() )
            processItem( aSheetPath, component->GetField( REFERENCE ) );

        if( m_values->GetValue() )
            processItem( aSheetPath, component->GetField( VALUE ) );

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
    else if( m_wires->GetValue() && aItem->IsType( wireTypes ) )
        processItem( aSheetPath, aItem );
    else if( m_busses->GetValue() && aItem->IsType( busTypes ) )
        processItem( aSheetPath, aItem );
    else if( m_globalLabels->GetValue() && aItem->Type() == SCH_GLOBAL_LABEL_T )
        processItem( aSheetPath, aItem );
    else if( m_hierLabels->GetValue() && aItem->Type() == SCH_HIER_LABEL_T )
        processItem( aSheetPath, aItem );
    else if( m_sheetTitles->GetValue() && aItem->Type() == SCH_SHEET_T )
    {
        if( !m_textSize.IsIndeterminate() )
            static_cast<SCH_SHEET*>( aItem )->SetSheetNameSize( m_textSize.GetValue() );
    }
    else if( m_sheetPins->GetValue() && aItem->Type() == SCH_SHEET_PIN_T )
        processItem( aSheetPath, aItem );
    else if( m_schTextAndGraphics->GetValue() && aItem->IsType( schTextAndGraphics ) )
        processItem( aSheetPath, aItem );
}


bool DIALOG_GLOBAL_EDIT_TEXT_AND_GRAPHICS::TransferDataFromWindow()
{
    if( !m_textSize.Validate( 1, 10000 ) )  // 1 mill .. 10 inches
        return false;

    SCH_SHEET_LIST aSheets( g_RootSheet );

    // Go through sheets
    for( const SCH_SHEET_PATH& sheetPath : aSheets )
    {
        SCH_SCREEN* screen = sheetPath.LastScreen();

        if( screen )
        {
            for( SCH_ITEM* item = screen->GetDrawItems(); item; item = item->Next() )
                visitItem( sheetPath, item );
        }
    }

    m_parent->HardRedraw();

    return true;
}


int SCH_EDIT_TOOL::GlobalEdit( const TOOL_EVENT& aEvent )
{
    DIALOG_GLOBAL_EDIT_TEXT_AND_GRAPHICS dlg( m_frame );
    dlg.ShowModal();
    return 0;
}


