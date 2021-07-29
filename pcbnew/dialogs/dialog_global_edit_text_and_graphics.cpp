/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2012 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 1992-2018 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <string_utils.h>
#include <board_commit.h>
#include <pcb_edit_frame.h>
#include <pcb_layer_box_selector.h>
#include <pcbnew.h>
#include <board.h>
#include <board_design_settings.h>
#include <footprint.h>
#include <pcb_dimension.h>
#include <fp_shape.h>
#include <pcb_text.h>
#include <widgets/unit_binder.h>
#include <tool/tool_manager.h>
#include <tools/global_edit_tool.h>
#include <dialog_global_edit_text_and_graphics_base.h>


// Columns of layer classes grid
enum
{
    COL_CLASS_NAME = 0,
    COL_LINE_THICKNESS,
    COL_TEXT_WIDTH,
    COL_TEXT_HEIGHT,
    COL_TEXT_THICKNESS,
    COL_TEXT_ITALIC,
    COL_TEXT_UPRIGHT
};

enum
{
    ROW_HEADER = 0,
    ROW_SILK,
    ROW_COPPER,
    ROW_EDGES,
    ROW_COURTYARD,
    ROW_FAB,
    ROW_OTHERS
};


static bool       g_modifyReferences;
static bool       g_modifyValues;
static bool       g_modifyOtherFields;
static bool       g_modifyFootprintGraphics;
static bool       g_modifyBoardText;
static bool       g_modifyBoardGraphics;
static bool       g_filterByLayer;
static LAYER_NUM  g_layerFilter;
static bool       g_filterByReference;
static wxString   g_referenceFilter;
static bool       g_filterByFootprint;
static wxString   g_footprintFilter;
static bool       g_filterSelected = false;


class DIALOG_GLOBAL_EDIT_TEXT_AND_GRAPHICS : public DIALOG_GLOBAL_EDIT_TEXT_AND_GRAPHICS_BASE
{
    PCB_EDIT_FRAME*        m_parent;
    BOARD_DESIGN_SETTINGS* m_brdSettings;
    PCB_SELECTION          m_selection;

    UNIT_BINDER            m_lineWidth;
    UNIT_BINDER            m_textWidth;
    UNIT_BINDER            m_textHeight;
    UNIT_BINDER            m_thickness;

public:
    DIALOG_GLOBAL_EDIT_TEXT_AND_GRAPHICS( PCB_EDIT_FRAME* parent );
    ~DIALOG_GLOBAL_EDIT_TEXT_AND_GRAPHICS() override;

protected:
    void onActionButtonChange( wxCommandEvent& event ) override;
    void onSpecifiedValueUpdateUI( wxUpdateUIEvent& event ) override;

    void OnLayerFilterSelect( wxCommandEvent& event ) override
    {
        m_layerFilterOpt->SetValue( true );
    }
    void OnReferenceFilterText( wxCommandEvent& event ) override
    {
        m_referenceFilterOpt->SetValue( true );
    }
    void OnFootprintFilterText( wxCommandEvent& event ) override
    {
        m_footprintFilterOpt->SetValue( true );
    }

    bool TransferDataToWindow() override;
    bool TransferDataFromWindow() override;

    void visitItem( BOARD_COMMIT& aCommit, BOARD_ITEM* aItem, FOOTPRINT* aParentItem );
    void processItem( BOARD_COMMIT& aCommit, BOARD_ITEM* aItem );
};


DIALOG_GLOBAL_EDIT_TEXT_AND_GRAPHICS::DIALOG_GLOBAL_EDIT_TEXT_AND_GRAPHICS( PCB_EDIT_FRAME* parent ) :
        DIALOG_GLOBAL_EDIT_TEXT_AND_GRAPHICS_BASE( parent ),
        m_lineWidth( parent, m_lineWidthLabel, m_LineWidthCtrl, m_lineWidthUnits ),
        m_textWidth( parent, m_SizeXlabel, m_SizeXCtrl, m_SizeXunit ),
        m_textHeight( parent, m_SizeYlabel, m_SizeYCtrl, m_SizeYunit ),
        m_thickness( parent, m_ThicknessLabel, m_ThicknessCtrl, m_ThicknessUnit )
{
    m_parent = parent;
    m_brdSettings = &m_parent->GetDesignSettings();

    m_layerFilter->SetBoardFrame( m_parent );
    m_layerFilter->SetLayersHotkeys( false );
    m_layerFilter->Resync();

    m_LayerCtrl->SetBoardFrame( m_parent );
    m_LayerCtrl->SetLayersHotkeys( false );
    m_LayerCtrl->SetUndefinedLayerName( INDETERMINATE_ACTION );
    m_LayerCtrl->Resync();

    m_grid->SetCellHighlightPenWidth( 0 );
    m_grid->SetDefaultCellFont( KIUI::GetInfoFont() );

    m_sdbSizerButtonsOK->SetDefault();

    finishDialogSettings();
}


DIALOG_GLOBAL_EDIT_TEXT_AND_GRAPHICS::~DIALOG_GLOBAL_EDIT_TEXT_AND_GRAPHICS()
{
    g_modifyReferences = m_references->GetValue();
    g_modifyValues = m_values->GetValue();
    g_modifyOtherFields = m_otherFields->GetValue();
    g_modifyFootprintGraphics = m_footprintGraphics->GetValue();
    g_modifyBoardText = m_boardText->GetValue();
    g_modifyBoardGraphics = m_boardGraphics->GetValue();

    g_filterByLayer = m_layerFilterOpt->GetValue();
    g_layerFilter = m_layerFilter->GetLayerSelection();
    g_filterByReference = m_referenceFilterOpt->GetValue();
    g_referenceFilter = m_referenceFilter->GetValue();
    g_filterByFootprint = m_footprintFilterOpt->GetValue();
    g_footprintFilter = m_footprintFilter->GetValue();
    g_filterSelected = m_selectedItemsFilter->GetValue();
}


bool DIALOG_GLOBAL_EDIT_TEXT_AND_GRAPHICS::TransferDataToWindow()
{
    PCB_SELECTION_TOOL* selTool = m_parent->GetToolManager()->GetTool<PCB_SELECTION_TOOL>();
    m_selection                 = selTool->GetSelection();

    m_references->SetValue( g_modifyReferences );
    m_values->SetValue( g_modifyValues );
    m_otherFields->SetValue( g_modifyOtherFields );
    m_footprintGraphics->SetValue( g_modifyFootprintGraphics );
    m_boardText->SetValue( g_modifyBoardText );
    m_boardGraphics->SetValue( g_modifyBoardGraphics );

    if( m_layerFilter->SetLayerSelection( g_layerFilter ) != wxNOT_FOUND )
        m_layerFilterOpt->SetValue( g_filterByLayer );

    // SetValue() generates events, ChangeValue() does not
    m_referenceFilter->ChangeValue( g_referenceFilter );
    m_referenceFilterOpt->SetValue( g_filterByReference );
    m_footprintFilter->ChangeValue( g_footprintFilter );
    m_footprintFilterOpt->SetValue( g_filterByFootprint );
    m_selectedItemsFilter->SetValue( g_filterSelected );

    m_lineWidth.SetValue( INDETERMINATE_ACTION );
    m_textWidth.SetValue( INDETERMINATE_ACTION );
    m_textHeight.SetValue( INDETERMINATE_ACTION );
    m_thickness.SetValue( INDETERMINATE_ACTION );
    m_Italic->Set3StateValue( wxCHK_UNDETERMINED );
    m_keepUpright->Set3StateValue( wxCHK_UNDETERMINED );
    m_Visible->Set3StateValue( wxCHK_UNDETERMINED );
    m_LayerCtrl->SetLayerSelection( UNDEFINED_LAYER );

#define SET_INT_VALUE( aRow, aCol, aValue ) \
        m_grid->SetCellValue( aRow, aCol, StringFromValue( GetUserUnits(), aValue, true ) )

#define SET_BOOL_VALUE( aRow, aCol, aValue ) \
        attr = new wxGridCellAttr; \
        attr->SetRenderer( new wxGridCellBoolRenderer() ); \
        attr->SetAlignment( wxALIGN_CENTER, wxALIGN_CENTER ); \
        attr->SetReadOnly(); \
        m_grid->SetAttr( aRow, aCol, attr ); \
        m_grid->SetCellValue( aRow, aCol, ( aValue ) ? "1" : "" )

    const BOARD_DESIGN_SETTINGS& bds = m_parent->GetBoard()->GetDesignSettings();
    wxGridCellAttr* attr;

    m_grid->SetCellValue( ROW_SILK,      COL_CLASS_NAME, _( "Silk Layers" ) );
    m_grid->SetCellValue( ROW_COPPER,    COL_CLASS_NAME, _( "Copper Layers" ) );
    m_grid->SetCellValue( ROW_EDGES,     COL_CLASS_NAME, _( "Edge Cuts" ) );
    m_grid->SetCellValue( ROW_COURTYARD, COL_CLASS_NAME, _( "Courtyards" ) );
    m_grid->SetCellValue( ROW_FAB,       COL_CLASS_NAME, _( "Fab Layers" ) );
    m_grid->SetCellValue( ROW_OTHERS,    COL_CLASS_NAME, _( "Other Layers" ) );

    m_grid->SetCellValue( ROW_HEADER, COL_LINE_THICKNESS, _( "Line Thickness" ) );
    SET_INT_VALUE( ROW_SILK,      COL_LINE_THICKNESS, bds.m_LineThickness[ LAYER_CLASS_SILK ] );
    SET_INT_VALUE( ROW_COPPER,    COL_LINE_THICKNESS, bds.m_LineThickness[ LAYER_CLASS_COPPER ] );
    SET_INT_VALUE( ROW_EDGES,     COL_LINE_THICKNESS, bds.m_LineThickness[ LAYER_CLASS_EDGES ] );
    SET_INT_VALUE( ROW_COURTYARD, COL_LINE_THICKNESS, bds.m_LineThickness[ LAYER_CLASS_COURTYARD ] );
    SET_INT_VALUE( ROW_FAB,       COL_LINE_THICKNESS, bds.m_LineThickness[ LAYER_CLASS_FAB ] );
    SET_INT_VALUE( ROW_OTHERS,    COL_LINE_THICKNESS, bds.m_LineThickness[ LAYER_CLASS_OTHERS ] );

    m_grid->SetCellValue( ROW_HEADER, COL_TEXT_WIDTH, _( "Text Width" ) );
    SET_INT_VALUE( ROW_SILK,   COL_TEXT_WIDTH, bds.m_TextSize[ LAYER_CLASS_SILK ].x );
    SET_INT_VALUE( ROW_COPPER, COL_TEXT_WIDTH, bds.m_TextSize[ LAYER_CLASS_COPPER ].x );
    SET_INT_VALUE( ROW_FAB,    COL_TEXT_WIDTH, bds.m_TextSize[ LAYER_CLASS_FAB ].x );
    SET_INT_VALUE( ROW_OTHERS, COL_TEXT_WIDTH, bds.m_TextSize[ LAYER_CLASS_OTHERS ].x );

    m_grid->SetCellValue( ROW_HEADER, COL_TEXT_HEIGHT, _( "Text Height" ) );
    SET_INT_VALUE( ROW_SILK,   COL_TEXT_HEIGHT, bds.m_TextSize[ LAYER_CLASS_SILK ].y );
    SET_INT_VALUE( ROW_COPPER, COL_TEXT_HEIGHT, bds.m_TextSize[ LAYER_CLASS_COPPER ].y );
    SET_INT_VALUE( ROW_FAB,    COL_TEXT_HEIGHT, bds.m_TextSize[ LAYER_CLASS_FAB ].y );
    SET_INT_VALUE( ROW_OTHERS, COL_TEXT_HEIGHT, bds.m_TextSize[ LAYER_CLASS_OTHERS ].y );

    m_grid->SetCellValue( ROW_HEADER, COL_TEXT_THICKNESS, _( "Text Thickness" ) );
    SET_INT_VALUE( ROW_SILK,   COL_TEXT_THICKNESS, bds.m_TextThickness[ LAYER_CLASS_SILK ] );
    SET_INT_VALUE( ROW_COPPER, COL_TEXT_THICKNESS, bds.m_TextThickness[ LAYER_CLASS_COPPER ] );
    SET_INT_VALUE( ROW_FAB,    COL_TEXT_THICKNESS, bds.m_TextThickness[ LAYER_CLASS_FAB ] );
    SET_INT_VALUE( ROW_OTHERS, COL_TEXT_THICKNESS, bds.m_TextThickness[ LAYER_CLASS_OTHERS ] );

    m_grid->SetCellValue(  ROW_HEADER, COL_TEXT_ITALIC, _( "Italic" ) );
    SET_BOOL_VALUE(  ROW_SILK,   COL_TEXT_ITALIC, bds.m_TextItalic[ LAYER_CLASS_SILK ] );
    SET_BOOL_VALUE(  ROW_COPPER, COL_TEXT_ITALIC, bds.m_TextItalic[ LAYER_CLASS_COPPER ] );
    SET_BOOL_VALUE(  ROW_FAB,    COL_TEXT_ITALIC, bds.m_TextItalic[ LAYER_CLASS_FAB ] );
    SET_BOOL_VALUE(  ROW_OTHERS, COL_TEXT_ITALIC, bds.m_TextItalic[ LAYER_CLASS_OTHERS ] );

    m_grid->SetCellValue(  ROW_HEADER, COL_TEXT_UPRIGHT, _( "Upright" ) );
    SET_BOOL_VALUE(  ROW_SILK,   COL_TEXT_UPRIGHT, bds.m_TextUpright[ LAYER_CLASS_SILK ] );
    SET_BOOL_VALUE(  ROW_COPPER, COL_TEXT_UPRIGHT, bds.m_TextUpright[ LAYER_CLASS_COPPER ] );
    SET_BOOL_VALUE(  ROW_FAB,    COL_TEXT_UPRIGHT, bds.m_TextUpright[ LAYER_CLASS_FAB ] );
    SET_BOOL_VALUE(  ROW_OTHERS, COL_TEXT_UPRIGHT, bds.m_TextUpright[ LAYER_CLASS_OTHERS ] );

    return true;

#undef SET_INT_VALUE
#undef SET_BOOL_VALUE
}


void DIALOG_GLOBAL_EDIT_TEXT_AND_GRAPHICS::onActionButtonChange( wxCommandEvent& event )
{
    // Update the UNIT_BINDER controls if the action to take is changed
    bool enable = m_setToSpecifiedValues->GetValue();

    m_lineWidth.Enable( enable );
    m_textWidth.Enable( enable );
    m_textHeight.Enable( enable );
    m_thickness.Enable( enable );
}


void DIALOG_GLOBAL_EDIT_TEXT_AND_GRAPHICS::onSpecifiedValueUpdateUI( wxUpdateUIEvent& event )
{
    // Update the UI for the elements inside the use specified values sizer
    event.Enable( m_setToSpecifiedValues->GetValue() );
}


void DIALOG_GLOBAL_EDIT_TEXT_AND_GRAPHICS::processItem( BOARD_COMMIT& aCommit, BOARD_ITEM* aItem )
{
    aCommit.Modify( aItem );

    EDA_TEXT*           textItem = dynamic_cast<EDA_TEXT*>( aItem );
    FP_TEXT*            fpTextItem = dynamic_cast<FP_TEXT*>( aItem );
    PCB_SHAPE*          drawItem = dynamic_cast<PCB_SHAPE*>( aItem );
    PCB_DIMENSION_BASE* dimension = dynamic_cast<PCB_DIMENSION_BASE*>( aItem );

    if( dimension )
        textItem = &dimension->Text();

    if( m_setToSpecifiedValues->GetValue() )
    {
        if( m_LayerCtrl->GetLayerSelection() != UNDEFINED_LAYER )
            aItem->SetLayer( ToLAYER_ID( m_LayerCtrl->GetLayerSelection() ) );

        if( !m_textWidth.IsIndeterminate() && textItem )
            textItem->SetTextSize( wxSize( m_textWidth.GetValue(), textItem->GetTextSize().y ) );

        if( !m_textHeight.IsIndeterminate() && textItem )
            textItem->SetTextSize( wxSize( textItem->GetTextSize().x, m_textHeight.GetValue() ) );

        if( !m_thickness.IsIndeterminate() && textItem )
            textItem->SetTextThickness( m_thickness.GetValue() );

        if( m_Italic->Get3StateValue() != wxCHK_UNDETERMINED && textItem )
            textItem->SetItalic( m_Italic->GetValue() );

        if( m_Visible->Get3StateValue() != wxCHK_UNDETERMINED && textItem )
            textItem->SetVisible( m_Visible->GetValue() );

        if( m_keepUpright->Get3StateValue() != wxCHK_UNDETERMINED && fpTextItem )
            fpTextItem->SetKeepUpright( m_keepUpright->GetValue() );

        if( !m_lineWidth.IsIndeterminate() )
        {
            if( drawItem )
                drawItem->SetWidth( m_lineWidth.GetValue() );

            if( dimension )
                dimension->SetLineThickness( m_lineWidth.GetValue() );
        }
    }
    else
    {
        PCB_LAYER_ID layer = aItem->GetLayer();

        if( textItem )
        {
            textItem->SetTextSize( m_brdSettings->GetTextSize( layer ) );
            textItem->SetTextThickness( m_brdSettings->GetTextThickness( layer ) );
            textItem->SetItalic( m_brdSettings->GetTextItalic( layer ) );
        }

        if( fpTextItem )
            fpTextItem->SetKeepUpright( m_brdSettings->GetTextUpright( layer ) );

        if( drawItem )
            drawItem->SetWidth( m_brdSettings->GetLineThickness( layer ) );

        if( dimension )
            dimension->SetLineThickness( m_brdSettings->GetLineThickness( layer ) );
    }
}


void DIALOG_GLOBAL_EDIT_TEXT_AND_GRAPHICS::visitItem( BOARD_COMMIT& aCommit, BOARD_ITEM* aItem,
                                                      FOOTPRINT* aParentItem = nullptr )
{
    if( m_selectedItemsFilter->GetValue() && !m_selection.Contains( aItem )
        && ( aParentItem == nullptr || !m_selection.Contains( aParentItem ) ) )
        return;

    if( m_layerFilterOpt->GetValue() && m_layerFilter->GetLayerSelection() != UNDEFINED_LAYER )
    {
        if( aItem->GetLayer() != m_layerFilter->GetLayerSelection() )
            return;
    }

    if( m_referenceFilterOpt->GetValue() && !m_referenceFilter->GetValue().IsEmpty() )
    {
        FOOTPRINT* fp = dynamic_cast<FOOTPRINT*>( aItem->GetParent() );

        if( fp )
        {
            if( !WildCompareString( m_referenceFilter->GetValue(), fp->GetReference(), false ) )
                return;
        }
    }

    if( m_footprintFilterOpt->GetValue() && !m_footprintFilter->GetValue().IsEmpty() )
    {
        FOOTPRINT* fp = dynamic_cast<FOOTPRINT*>( aItem->GetParent() );

        if( fp )
        {
            if( !WildCompareString( m_footprintFilter->GetValue(), fp->GetFPID().Format(), false ) )
                return;
        }
    }

    processItem( aCommit, aItem );
}


bool DIALOG_GLOBAL_EDIT_TEXT_AND_GRAPHICS::TransferDataFromWindow()
{
    if( !m_textWidth.Validate( TEXTS_MIN_SIZE, TEXTS_MAX_SIZE )
        || !m_textHeight.Validate( TEXTS_MIN_SIZE, TEXTS_MAX_SIZE ) )
    {
        return false;
    }

    BOARD_COMMIT commit( m_parent );

    // Go through the footprints
    for( FOOTPRINT* fp : m_parent->GetBoard()->Footprints() )
    {
        if( m_references->GetValue() )
            visitItem( commit, &fp->Reference(), fp );

        if( m_values->GetValue() )
            visitItem( commit, &fp->Value(), fp );

        // Go through all other footprint items
        for( BOARD_ITEM* boardItem : fp->GraphicalItems() )
        {
            if( boardItem->Type() == PCB_FP_TEXT_T )
            {
                // We are guaranteed to always get an EDA_TEXT in this statement, but we must
                // use the dynamic_cast to move through the type tree anyway.
                const wxString text = dynamic_cast<EDA_TEXT*>( boardItem )->GetText();

                if( m_references->GetValue() && text == wxT( "${REFERENCE}" ) )
                    visitItem( commit, boardItem, fp );
                else if( m_values->GetValue() && text == wxT( "${VALUE}" ) )
                    visitItem( commit, boardItem, fp );
                else if( m_otherFields->GetValue() )
                    visitItem( commit, boardItem, fp );
            }
            else if( boardItem->Type() == PCB_FP_SHAPE_T )
            {
                if( m_footprintGraphics->GetValue() )
                    visitItem( commit, boardItem, fp );
            }
        }
    }

    // Go through the PCB text & graphic items
    for( BOARD_ITEM* boardItem : m_parent->GetBoard()->Drawings() )
    {
        KICAD_T itemType = boardItem->Type();

        if( itemType == PCB_TEXT_T )
        {
            if( m_boardText->GetValue() )
                visitItem( commit, boardItem );
        }
        else if( itemType == PCB_SHAPE_T || BaseType( itemType ) == PCB_DIMENSION_T )
        {
            if( m_boardGraphics->GetValue() )
                visitItem( commit, boardItem );
        }
    }

    commit.Push( "Edit text and graphics properties" );
    m_parent->GetCanvas()->Refresh();

    return true;
}


int GLOBAL_EDIT_TOOL::EditTextAndGraphics( const TOOL_EVENT& aEvent )
{
    PCB_EDIT_FRAME* editFrame = getEditFrame<PCB_EDIT_FRAME>();
    DIALOG_GLOBAL_EDIT_TEXT_AND_GRAPHICS dlg( editFrame );

    dlg.ShowModal();
    return 0;
}


