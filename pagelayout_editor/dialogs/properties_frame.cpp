/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2013 CERN
 * Copyright (C) 2021-2023 KiCad Developers, see AUTHORS.txt for contributors.
 * @author Jean-Pierre Charras, jp.charras at wanadoo.fr
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

#include <project.h>
#include <scintilla_tricks.h>
#include <widgets/bitmap_button.h>
#include <widgets/font_choice.h>
#include <widgets/color_swatch.h>
#include <tool/tool_manager.h>
#include <drawing_sheet/ds_draw_item.h>
#include <drawing_sheet/ds_data_item.h>
#include <drawing_sheet/ds_data_model.h>
#include <view/view.h>
#include <bitmaps.h>

#include "properties_frame.h"
#include "pl_editor_frame.h"
#include "tools/pl_selection_tool.h"

#include <dialogs/html_message_box.h>


/**
 * Minimum drawing sheet text default size in millmeters from #PROPERTIES_FRAME.
 *
 * @note 0.0 is allowed for a given text to use the default size.
 */
#define DLG_MIN_TEXTSIZE 0.01
#define DLG_MAX_TEXTSIZE 100.0   ///< Maximum drawing sheet text size in mm from PROPERTIES_FRAME.


PROPERTIES_FRAME::PROPERTIES_FRAME( PL_EDITOR_FRAME* aParent ) :
        PANEL_PROPERTIES_BASE( aParent ),
        m_scintillaTricks( nullptr ),
        m_textSizeX( aParent, m_staticTextTsizeX, m_textCtrlTextSizeX, m_textSizeXUnits ),
        m_textSizeY( aParent, m_staticTextTsizeY, m_textCtrlTextSizeY,  m_textSizeYUnits ),
        m_constraintX( aParent, m_constraintXLabel, m_constraintXCtrl, m_constraintXUnits ),
        m_constraintY( aParent, m_constraintYLabel, m_constraintYCtrl, m_constraintYUnits ),
        m_textPosX( aParent, m_staticTextPosX, m_textCtrlPosX, m_TextPosXUnits ),
        m_textPosY( aParent, m_staticTextPosY, m_textCtrlPosY, m_TextPosYUnits ),
        m_textEndX( aParent, m_staticTextEndX, m_textCtrlEndX, m_TextEndXUnits ),
        m_textEndY( aParent, m_staticTextEndY, m_textCtrlEndY, m_TextEndYUnits ),
        m_textStepX( aParent, m_staticTextStepX, m_textCtrlStepX, m_TextStepXUnits ),
        m_textStepY( aParent, m_staticTextStepY, m_textCtrlStepY, m_TextStepYUnits ),
        m_defaultTextSizeX( aParent, m_staticTextDefTsX, m_textCtrlDefaultTextSizeX,
                            m_defaultTextSizeXUnits ),
        m_defaultTextSizeY( aParent, m_staticTextDefTsY, m_textCtrlDefaultTextSizeY,
                            m_defaultTextSizeYUnits ),
        m_defaultLineWidth( aParent, m_defaultLineWidthLabel, m_defaultLineWidthCtrl,
                            m_defaultLineWidthUnits ),
        m_defaultTextThickness( aParent, m_defaultTextThicknessLabel, m_defaultTextThicknessCtrl,
                                m_defaultTextThicknessUnits ),
        m_textLeftMargin( aParent, m_leftMarginLabel, m_leftMarginCtrl, m_leftMarginUnits ),
        m_textRightMargin( aParent, m_rightMarginLabel, m_rightMarginCtrl, m_rightMarginUnits ),
        m_textTopMargin( aParent, m_topMarginLabel, m_topMarginCtrl, m_topMarginUnits ),
        m_textBottomMargin( aParent, m_bottomMarginLabel, m_bottomMarginCtrl, m_bottomMarginUnits ),
        m_lineWidth( aParent, m_lineWidthLabel, m_lineWidthCtrl, m_lineWidthUnits )
{
    m_parent = aParent;

    m_stcText->SetUseVerticalScrollBar( false );
    m_stcText->SetUseHorizontalScrollBar( false );
    m_stcText->SetEOLMode( wxSTC_EOL_LF );  // Always use LF as eol char, regardless the platform
    m_scintillaTricks = new SCINTILLA_TRICKS( m_stcText, wxT( "{}" ), false );

    m_staticTextSizeInfo->SetFont( KIUI::GetInfoFont( this ).Italic() );

    m_bold->SetIsCheckButton();
    m_bold->SetBitmap( KiBitmap( BITMAPS::text_bold ) );
    m_italic->SetIsCheckButton();
    m_italic->SetBitmap( KiBitmap( BITMAPS::text_italic ) );

    m_separator2->SetIsSeparator();

    m_alignLeft->SetIsRadioButton();
    m_alignLeft->SetBitmap( KiBitmap( BITMAPS::text_align_left ) );
    m_alignCenter->SetIsRadioButton();
    m_alignCenter->SetBitmap( KiBitmap( BITMAPS::text_align_center ) );
    m_alignRight->SetIsRadioButton();
    m_alignRight->SetBitmap( KiBitmap( BITMAPS::text_align_right ) );

    m_separator3->SetIsSeparator();

    m_vAlignTop->SetIsRadioButton();
    m_vAlignTop->SetBitmap( KiBitmap( BITMAPS::text_valign_top ) );
    m_vAlignMiddle->SetIsRadioButton();
    m_vAlignMiddle->SetBitmap( KiBitmap( BITMAPS::text_valign_center ) );
    m_vAlignBottom->SetIsRadioButton();
    m_vAlignBottom->SetBitmap( KiBitmap( BITMAPS::text_valign_bottom ) );

    m_separator4->SetIsSeparator();

    m_textColorSwatch->SetDefaultColor( COLOR4D::UNSPECIFIED );
    m_textColorSwatch->SetSwatchBackground( aParent->GetDrawBgColor() );

    m_buttonOK->SetDefault();

    // ensure sizers are up to date
    // (fix an issue on GTK but should not create issues on other platforms):
    m_swItemProperties->Fit();
    m_swGeneralOpts->Fit();

    m_stcText->Bind( wxEVT_STC_CHARADDED, &PROPERTIES_FRAME::onScintillaCharAdded, this );
    m_stcText->Bind( wxEVT_STC_AUTOCOMP_CHAR_DELETED, &PROPERTIES_FRAME::onScintillaCharAdded,
                     this );
    m_alignLeft->Bind( wxEVT_BUTTON, &PROPERTIES_FRAME::onHAlignButton, this );
    m_alignCenter->Bind( wxEVT_BUTTON, &PROPERTIES_FRAME::onHAlignButton, this );
    m_alignRight->Bind( wxEVT_BUTTON, &PROPERTIES_FRAME::onHAlignButton, this );
    m_vAlignTop->Bind( wxEVT_BUTTON, &PROPERTIES_FRAME::onVAlignButton, this );
    m_vAlignMiddle->Bind( wxEVT_BUTTON, &PROPERTIES_FRAME::onVAlignButton, this );
    m_vAlignBottom->Bind( wxEVT_BUTTON, &PROPERTIES_FRAME::onVAlignButton, this );
}


PROPERTIES_FRAME::~PROPERTIES_FRAME()
{
    delete m_scintillaTricks;
}


void PROPERTIES_FRAME::OnPageChanged( wxNotebookEvent& event )
{
    if( event.GetSelection() == 0 )
        m_buttonOK->SetDefault();
    else
        m_buttonGeneralOptsOK->SetDefault();

    event.Skip();
}


wxSize PROPERTIES_FRAME::GetMinSize() const
{
    return wxSize( 150, -1 );
}


void PROPERTIES_FRAME::CopyPrmsFromGeneralToPanel()
{
    static EDA_UNITS units = EDA_UNITS::MILLIMETRES;
    DS_DATA_MODEL&   model = DS_DATA_MODEL::GetTheInstance();

    // Set default parameters
    m_defaultLineWidth.SetDoubleValue(
            EDA_UNIT_UTILS::UI::FromUserUnit( drawSheetIUScale,units, model.m_DefaultLineWidth ) );

    m_defaultTextSizeX.SetDoubleValue(
            EDA_UNIT_UTILS::UI::FromUserUnit( drawSheetIUScale,units, model.m_DefaultTextSize.x ) );
    m_defaultTextSizeY.SetDoubleValue(
            EDA_UNIT_UTILS::UI::FromUserUnit( drawSheetIUScale,units, model.m_DefaultTextSize.y ) );
    m_defaultTextThickness.SetDoubleValue( EDA_UNIT_UTILS::UI::FromUserUnit(
            drawSheetIUScale,units, model.m_DefaultTextThickness ) );

    m_textLeftMargin.SetDoubleValue(
            EDA_UNIT_UTILS::UI::FromUserUnit( drawSheetIUScale,units, model.GetLeftMargin() ) );
    m_textRightMargin.SetDoubleValue(
            EDA_UNIT_UTILS::UI::FromUserUnit( drawSheetIUScale,units, model.GetRightMargin() ) );
    m_textTopMargin.SetDoubleValue(
            EDA_UNIT_UTILS::UI::FromUserUnit( drawSheetIUScale,units, model.GetTopMargin() ) );
    m_textBottomMargin.SetDoubleValue(
            EDA_UNIT_UTILS::UI::FromUserUnit( drawSheetIUScale,units, model.GetBottomMargin() ) );
}


bool PROPERTIES_FRAME::CopyPrmsFromPanelToGeneral()
{
    static EDA_UNITS units = EDA_UNITS::MILLIMETRES;
    DS_DATA_MODEL&   model = DS_DATA_MODEL::GetTheInstance();

    // Import default parameters from widgets
    bool is_valid = m_defaultLineWidth.Validate( 0.0, 10.0, EDA_UNITS::MILLIMETRES );

    if( is_valid )
        model.m_DefaultLineWidth = EDA_UNIT_UTILS::UI::ToUserUnit( drawSheetIUScale, units,
                                                                   m_defaultLineWidth.GetValue() );

    is_valid = m_defaultTextSizeX.Validate( DLG_MIN_TEXTSIZE, DLG_MAX_TEXTSIZE,
                                            EDA_UNITS::MILLIMETRES );
    if( is_valid )
        model.m_DefaultTextSize.x = EDA_UNIT_UTILS::UI::ToUserUnit( drawSheetIUScale, units,
                                                                    m_defaultTextSizeX.GetValue() );

    is_valid = m_defaultTextSizeY.Validate( DLG_MIN_TEXTSIZE, DLG_MAX_TEXTSIZE,
                                            EDA_UNITS::MILLIMETRES );

    if( is_valid )
        model.m_DefaultTextSize.y = EDA_UNIT_UTILS::UI::ToUserUnit( drawSheetIUScale, units,
                                                                    m_defaultTextSizeY.GetValue() );

    is_valid = m_defaultTextThickness.Validate( 0.0, 5.0, EDA_UNITS::MILLIMETRES );

    if( is_valid )
        model.m_DefaultTextThickness = EDA_UNIT_UTILS::UI::ToUserUnit( drawSheetIUScale, units,
                                                                       m_defaultTextThickness.GetValue() );

    // Get page margins values
    model.SetRightMargin( EDA_UNIT_UTILS::UI::ToUserUnit( drawSheetIUScale, units,
                                                          m_textRightMargin.GetValue() ) );
    model.SetBottomMargin( EDA_UNIT_UTILS::UI::ToUserUnit( drawSheetIUScale, units,
                                                           m_textBottomMargin.GetValue() ) );
    model.SetLeftMargin( EDA_UNIT_UTILS::UI::ToUserUnit( drawSheetIUScale, units,
                                                         m_textLeftMargin.GetValue() ) );
    model.SetTopMargin( EDA_UNIT_UTILS::UI::ToUserUnit( drawSheetIUScale, units,
                                                         m_textTopMargin.GetValue() ) );

    return true;
}


void PROPERTIES_FRAME::CopyPrmsFromItemToPanel( DS_DATA_ITEM* aItem )
{
    if( !aItem )
    {
        m_SizerItemProperties->Show( false );
        return;
    }

    static EDA_UNITS units = EDA_UNITS::MILLIMETRES;
    wxString         msg;

    // Set parameters common to all DS_DATA_ITEM types
    m_staticTextType->SetLabel( aItem->GetClassName() );
    m_textCtrlComment->SetValue( aItem->m_Info );

    switch( aItem->GetPage1Option() )
    {
    default:
    case ALL_PAGES:        m_choicePageOpt->SetSelection( 0 ); break;
    case FIRST_PAGE_ONLY:  m_choicePageOpt->SetSelection( 1 ); break;
    case SUBSEQUENT_PAGES: m_choicePageOpt->SetSelection( 2 ); break;
    }

    // Position/ start point
    m_textPosX.SetDoubleValue( EDA_UNIT_UTILS::UI::FromUserUnit( drawSheetIUScale, units,
                                                                 aItem->m_Pos.m_Pos.x ) );
    m_textPosY.SetDoubleValue( EDA_UNIT_UTILS::UI::FromUserUnit( drawSheetIUScale, units,
                                                                 aItem->m_Pos.m_Pos.y ) );

    switch(  aItem->m_Pos.m_Anchor )
    {
    case RB_CORNER: m_comboBoxCornerPos->SetSelection( 2 ); break;
    case RT_CORNER: m_comboBoxCornerPos->SetSelection( 0 ); break;
    case LB_CORNER: m_comboBoxCornerPos->SetSelection( 3 ); break;
    case LT_CORNER: m_comboBoxCornerPos->SetSelection( 1 ); break;
    }

    // End point
    m_textEndX.SetDoubleValue( EDA_UNIT_UTILS::UI::FromUserUnit( drawSheetIUScale, units,
                                                                 aItem->m_End.m_Pos.x ) );
    m_textEndY.SetDoubleValue( EDA_UNIT_UTILS::UI::FromUserUnit( drawSheetIUScale, units,
                                                                 aItem->m_End.m_Pos.y ) );

    switch( aItem->m_End.m_Anchor )
    {
    case RB_CORNER: m_comboBoxCornerEnd->SetSelection( 2 ); break;
    case RT_CORNER: m_comboBoxCornerEnd->SetSelection( 0 ); break;
    case LB_CORNER: m_comboBoxCornerEnd->SetSelection( 3 ); break;
    case LT_CORNER: m_comboBoxCornerEnd->SetSelection( 1 ); break;
    }

    m_lineWidth.SetDoubleValue( EDA_UNIT_UTILS::UI::FromUserUnit( drawSheetIUScale, units,
                                                                  aItem->m_LineWidth ) );

    // Now, set prms more specific to DS_DATA_ITEM types
    // For a given type, disable widgets which are not relevant,
    // and be sure widgets which are relevant are enabled
    if( aItem->GetType() == DS_DATA_ITEM::DS_TEXT )
    {
        DS_DATA_ITEM_TEXT* item = static_cast<DS_DATA_ITEM_TEXT*>( aItem );
        item->m_FullText = item->m_TextBase;

        // Replace our '\' 'n' sequence by the EOL char
        item->ReplaceAntiSlashSequence();
        m_stcText->SetValue( item->m_FullText );
        m_stcText->EmptyUndoBuffer();

        msg.Printf( wxT( "%d" ), item->m_IncrementLabel );
        m_textCtrlTextIncrement->SetValue( msg );

        // Rotation (poly and text)
        msg.Printf( wxT( "%.3f" ), item->m_Orient );
        m_textCtrlRotation->SetValue( msg );

        // Constraints:
        m_constraintX.SetDoubleValue( EDA_UNIT_UTILS::UI::FromUserUnit( drawSheetIUScale, units,
                                                                        item->m_BoundingBoxSize.x ) );
        m_constraintY.SetDoubleValue( EDA_UNIT_UTILS::UI::FromUserUnit( drawSheetIUScale, units,
                                                                        item->m_BoundingBoxSize.y ) );

        // Font Options
        m_fontCtrl->SetFontSelection( item->m_Font );

        m_bold->Check( item->m_Bold );
        m_italic->Check( item->m_Italic );

        m_textColorSwatch->SetSwatchColor( item->m_TextColor, false );

        for( BITMAP_BUTTON* btn : { m_alignLeft, m_alignCenter, m_alignRight } )
            btn->Check( false );

        switch( item->m_Hjustify )
        {
        case GR_TEXT_H_ALIGN_LEFT:   m_alignLeft->Check(); break;
        case GR_TEXT_H_ALIGN_CENTER: m_alignCenter->Check(); break;
        case GR_TEXT_H_ALIGN_RIGHT:  m_alignRight->Check(); break;
        }

        for( BITMAP_BUTTON* btn : { m_vAlignTop, m_vAlignMiddle, m_vAlignBottom } )
            btn->Check( false );

        switch( item->m_Vjustify )
        {
        case GR_TEXT_V_ALIGN_TOP:    m_vAlignTop->Check(); break;
        case GR_TEXT_V_ALIGN_CENTER: m_vAlignMiddle->Check(); break;
        case GR_TEXT_V_ALIGN_BOTTOM: m_vAlignBottom->Check(); break;
        }

        // Text size
        m_textSizeX.SetDoubleValue( EDA_UNIT_UTILS::UI::FromUserUnit( drawSheetIUScale, units,
                                                                      item->m_TextSize.x ) );
        m_textSizeY.SetDoubleValue( EDA_UNIT_UTILS::UI::FromUserUnit( drawSheetIUScale, units,
                                                                      item->m_TextSize.y ) );
    }

    if( aItem->GetType() == DS_DATA_ITEM::DS_POLYPOLYGON )
    {
        DS_DATA_ITEM_POLYGONS* item = static_cast<DS_DATA_ITEM_POLYGONS*>( aItem );

        // Rotation (poly and text)
        msg.Printf( wxT( "%.3f" ), item->m_Orient.AsDegrees() );
        m_textCtrlRotation->SetValue( msg );
    }

    if( aItem->GetType() == DS_DATA_ITEM::DS_BITMAP )
    {
        DS_DATA_ITEM_BITMAP* item = static_cast<DS_DATA_ITEM_BITMAP*>( aItem );

        // select definition in PPI
        msg.Printf( wxT( "%d" ), item->GetPPI() );
        m_textCtrlBitmapDPI->SetValue( msg );
    }

    m_SizerItemProperties->Show( true );

    m_SizerTextOptions->Show( aItem->GetType() == DS_DATA_ITEM::DS_TEXT );
    m_buttonHelp->Show( aItem->GetType() == DS_DATA_ITEM::DS_TEXT );

    m_sbSizerEndPosition->Show( aItem->GetType() == DS_DATA_ITEM::DS_SEGMENT
                                || aItem->GetType() == DS_DATA_ITEM::DS_RECT );

    m_lineWidth.Show( aItem->GetType() != DS_DATA_ITEM::DS_BITMAP );

    if( aItem->GetType() == DS_DATA_ITEM::DS_TEXT
            || aItem->GetType() == DS_DATA_ITEM::DS_POLYPOLYGON )
    {
        m_staticTextRot->Show( true );
        m_textCtrlRotation->Show( true );
    }
    else
    {
        m_staticTextRot->Show( false );
        m_textCtrlRotation->Show( false );
    }

    m_staticTextBitmapDPI->Show( aItem->GetType() == DS_DATA_ITEM::DS_BITMAP );
    m_textCtrlBitmapDPI->Show( aItem->GetType() == DS_DATA_ITEM::DS_BITMAP );

    m_staticTextInclabel->Show( aItem->GetType() == DS_DATA_ITEM::DS_TEXT );
    m_textCtrlTextIncrement->Show( aItem->GetType() == DS_DATA_ITEM::DS_TEXT );

    // Repeat parameters
    msg.Printf( wxT( "%d" ), aItem->m_RepeatCount );
    m_textCtrlRepeatCount->SetValue( msg );

    m_textStepX.SetDoubleValue( EDA_UNIT_UTILS::UI::FromUserUnit( drawSheetIUScale, units,
                                                                  aItem->m_IncrementVector.x ) );
    m_textStepY.SetDoubleValue( EDA_UNIT_UTILS::UI::FromUserUnit( drawSheetIUScale, units,
                                                                  aItem->m_IncrementVector.y ) );

    // The number of widgets was modified, so recalculate sizers
    m_swItemProperties->Layout();

#ifdef __WXGTK__
    // This call is mandatory on wxGTK to initialize the right virtual size and therefore
    // scrollbars, but for some reason, create issues on Windows (incorrect display
    // until the frame is resized). Joys of multiplatform dev.
    m_swItemProperties->Fit();
#endif

    // send a size event to be sure scrollbars will be added/removed as needed
    m_swItemProperties->PostSizeEvent();
    m_swItemProperties->Refresh();
}


void PROPERTIES_FRAME::onHAlignButton( wxCommandEvent& aEvent )
{
    for( BITMAP_BUTTON* btn : { m_alignLeft, m_alignCenter, m_alignRight } )
    {
        if( btn->IsChecked() && btn != aEvent.GetEventObject() )
            btn->Check( false );
    }
}


void PROPERTIES_FRAME::onVAlignButton( wxCommandEvent& aEvent )
{
    for( BITMAP_BUTTON* btn : { m_vAlignTop, m_vAlignMiddle, m_vAlignBottom } )
    {
        if( btn->IsChecked() && btn != aEvent.GetEventObject() )
            btn->Check( false );
    }
}


void PROPERTIES_FRAME::OnAcceptPrms( wxCommandEvent& event )
{
    PL_SELECTION_TOOL* selTool = m_parent->GetToolManager()->GetTool<PL_SELECTION_TOOL>();
    PL_SELECTION&      selection = selTool->GetSelection();

    m_parent->SaveCopyInUndoList();

    DS_DRAW_ITEM_BASE* drawItem = (DS_DRAW_ITEM_BASE*) selection.Front();

    if( drawItem )
    {
        DS_DATA_ITEM* dataItem = drawItem->GetPeer();
        CopyPrmsFromPanelToItem( dataItem );

        // Be sure what is displayed is what is set for item
        // (mainly, texts can be modified if they contain "\n")
        CopyPrmsFromItemToPanel( dataItem );
        m_parent->GetCanvas()->GetView()->Update( drawItem );
    }

    CopyPrmsFromPanelToGeneral();

    // Refresh values, exactly as they are converted, to avoid any mistake
    CopyPrmsFromGeneralToPanel();

    m_parent->OnModify();

    // Rebuild the draw list with the new parameters
    m_parent->GetCanvas()->DisplayDrawingSheet();
    m_parent->GetCanvas()->Refresh();
}


void PROPERTIES_FRAME::OnSetDefaultValues( wxCommandEvent& event )
{
    DS_DATA_MODEL& model = DS_DATA_MODEL::GetTheInstance();

    model.m_DefaultTextSize = VECTOR2D( TB_DEFAULT_TEXTSIZE, TB_DEFAULT_TEXTSIZE );
    model.m_DefaultLineWidth = 0.15;
    model.m_DefaultTextThickness = 0.15;

    CopyPrmsFromGeneralToPanel();

    // Rebuild the draw list with the new parameters
    m_parent->GetCanvas()->DisplayDrawingSheet();
    m_parent->GetCanvas()->Refresh();
}


bool PROPERTIES_FRAME::CopyPrmsFromPanelToItem( DS_DATA_ITEM* aItem )
{
    if( aItem == nullptr )
        return false;

    const EDA_UNITS units = EDA_UNITS::MILLIMETRES;
    wxString        msg;

    // Import common parameters:
    aItem->m_Info = m_textCtrlComment->GetValue();

    switch( m_choicePageOpt->GetSelection() )
    {
    default:
    case 0: aItem->SetPage1Option( ALL_PAGES );        break;
    case 1: aItem->SetPage1Option( FIRST_PAGE_ONLY );  break;
    case 2: aItem->SetPage1Option( SUBSEQUENT_PAGES ); break;
    }

    // Import thickness
    bool is_valid = m_lineWidth.Validate( 0.0, 10.0, EDA_UNITS::MILLIMETRES );

    if( is_valid )
        aItem->m_LineWidth =
            EDA_UNIT_UTILS::UI::ToUserUnit( drawSheetIUScale, units, m_lineWidth.GetValue() );

    // Import Start point
    aItem->m_Pos.m_Pos.x =
            EDA_UNIT_UTILS::UI::ToUserUnit( drawSheetIUScale, units, m_textPosX.GetValue() );
    aItem->m_Pos.m_Pos.y =
            EDA_UNIT_UTILS::UI::ToUserUnit( drawSheetIUScale, units, m_textPosY.GetValue() );

    switch( m_comboBoxCornerPos->GetSelection() )
    {
    case 2: aItem->m_Pos.m_Anchor = RB_CORNER; break;
    case 0: aItem->m_Pos.m_Anchor = RT_CORNER; break;
    case 3: aItem->m_Pos.m_Anchor = LB_CORNER; break;
    case 1: aItem->m_Pos.m_Anchor = LT_CORNER; break;
    }

    // Import End point
    aItem->m_End.m_Pos.x =
            EDA_UNIT_UTILS::UI::ToUserUnit( drawSheetIUScale, units, m_textEndX.GetValue() );
    aItem->m_End.m_Pos.y =
            EDA_UNIT_UTILS::UI::ToUserUnit( drawSheetIUScale, units, m_textEndY.GetValue() );

    switch( m_comboBoxCornerEnd->GetSelection() )
    {
    case 2: aItem->m_End.m_Anchor = RB_CORNER; break;
    case 0: aItem->m_End.m_Anchor = RT_CORNER; break;
    case 3: aItem->m_End.m_Anchor = LB_CORNER; break;
    case 1: aItem->m_End.m_Anchor = LT_CORNER; break;
    }

    // Import Repeat prms
    long itmp;
    msg = m_textCtrlRepeatCount->GetValue();
    msg.ToLong( &itmp );

    // Ensure m_RepeatCount is > 0. Otherwise it create issues because a repeat
    // count < 1 make no sense
    if( itmp < 1l )
    {
        itmp = 1;
        msg.Printf( wxT( "%ld" ), itmp );
        m_textCtrlRepeatCount->SetValue( msg );
    }

    aItem->m_RepeatCount = itmp;

    aItem->m_IncrementVector.x =
            EDA_UNIT_UTILS::UI::ToUserUnit( drawSheetIUScale, units, m_textStepX.GetValue() );
    aItem->m_IncrementVector.y =
            EDA_UNIT_UTILS::UI::ToUserUnit( drawSheetIUScale, units, m_textStepY.GetValue() );

    if( aItem->GetType() == DS_DATA_ITEM::DS_TEXT )
    {
        DS_DATA_ITEM_TEXT* item = static_cast<DS_DATA_ITEM_TEXT*>( aItem );

        item->m_TextBase = m_stcText->GetValue();

        msg = m_textCtrlTextIncrement->GetValue();
        msg.ToLong( &itmp );
        item->m_IncrementLabel = itmp;

        item->m_Bold = m_bold->IsChecked();
        item->m_Italic = m_italic->IsChecked();
        item->m_TextColor = m_textColorSwatch->GetSwatchColor();

        if( m_alignLeft->IsChecked() )
            item->m_Hjustify = GR_TEXT_H_ALIGN_LEFT;
        else if( m_alignCenter->IsChecked() )
            item->m_Hjustify = GR_TEXT_H_ALIGN_CENTER;
        else if( m_alignRight->IsChecked() )
            item->m_Hjustify = GR_TEXT_H_ALIGN_RIGHT;

        if( m_vAlignTop->IsChecked() )
            item->m_Vjustify = GR_TEXT_V_ALIGN_TOP;
        else if( m_vAlignMiddle->IsChecked() )
            item->m_Vjustify = GR_TEXT_V_ALIGN_CENTER;
        else if( m_vAlignBottom->IsChecked() )
            item->m_Vjustify = GR_TEXT_V_ALIGN_BOTTOM;

        if( m_fontCtrl->HaveFontSelection() )
            item->m_Font = m_fontCtrl->GetFontSelection( item->m_Bold, item->m_Italic );

        msg = m_textCtrlRotation->GetValue();
        item->m_Orient = EDA_UNIT_UTILS::UI::DoubleValueFromString( drawSheetIUScale,
                                                                    EDA_UNITS::UNSCALED, msg );

        // Import text size
        is_valid = m_textSizeX.Validate( 0.0, DLG_MAX_TEXTSIZE, EDA_UNITS::MILLIMETRES );

        if( is_valid )
            item->m_TextSize.x = EDA_UNIT_UTILS::UI::ToUserUnit( drawSheetIUScale, units,
                                                                 m_textSizeX.GetValue() );
        is_valid = m_textSizeY.Validate( 0.0, DLG_MAX_TEXTSIZE, EDA_UNITS::MILLIMETRES );

        if( is_valid )
            item->m_TextSize.y = EDA_UNIT_UTILS::UI::ToUserUnit( drawSheetIUScale, units,
                                                                 m_textSizeY.GetValue() );

        // Import constraints:
        item->m_BoundingBoxSize.x =
                EDA_UNIT_UTILS::UI::ToUserUnit( drawSheetIUScale, units, m_constraintX.GetValue() );
        item->m_BoundingBoxSize.y =
                EDA_UNIT_UTILS::UI::ToUserUnit( drawSheetIUScale, units, m_constraintY.GetValue() );
    }

    if( aItem->GetType() == DS_DATA_ITEM::DS_POLYPOLYGON )
    {
        DS_DATA_ITEM_POLYGONS* item = static_cast<DS_DATA_ITEM_POLYGONS*>( aItem );

        msg = m_textCtrlRotation->GetValue();
        item->m_Orient = EDA_ANGLE(
                EDA_UNIT_UTILS::UI::DoubleValueFromString( drawSheetIUScale,EDA_UNITS::UNSCALED,
                                                           msg ), DEGREES_T );
    }

    if( aItem->GetType() == DS_DATA_ITEM::DS_BITMAP )
    {
        DS_DATA_ITEM_BITMAP* item = static_cast<DS_DATA_ITEM_BITMAP*>( aItem );
        long                 value;

        msg = m_textCtrlBitmapDPI->GetValue();

        if( msg.ToLong( &value ) )
            item->SetPPI( (int)value );
    }

    return true;
}


void PROPERTIES_FRAME::onScintillaCharAdded( wxStyledTextEvent &aEvent )
{
    wxArrayString   autocompleteTokens;
    int             pos = m_stcText->GetCurrentPos();
    int             start = m_stcText->WordStartPosition( pos, true );
    wxString        partial;

    if( start >= 2
            && m_stcText->GetCharAt( start-2 ) == '$'
            && m_stcText->GetCharAt( start-1 ) == '{' )
    {
        DS_DRAW_ITEM_LIST::GetTextVars( &autocompleteTokens );

        partial = m_stcText->GetTextRange( start, pos );

        for( std::pair<wxString, wxString> entry : m_parent->Prj().GetTextVars() )
            autocompleteTokens.push_back( entry.first );
    }

    m_scintillaTricks->DoAutocomplete( partial, autocompleteTokens );
    m_stcText->SetFocus();
}


void PROPERTIES_FRAME::onHelp( wxCommandEvent& aEvent )
{
    // Show the system variables for worksheet text:
    HTML_MESSAGE_BOX dlg( this, _( "Predefined Keywords" ) );
    wxString message;

    message << _( "Texts can include keywords." ) << "<br>";
    message << _( "Keyword notation is ${keyword}" ) << "<br>";
    message << _( "Each keyword is replaced by its value" ) << "<br><br>";
    message << _( "These build-in keywords are always available:" ) << "<br><br>";
    dlg.AddHTML_Text( message );

    message = "KICAD_VERSION\n";
    message << "# " << _( "(sheet number)" ) << "\n";
    message << "## " << _( "(sheet count)" ) << "\n";
    message << "COMMENT1 thru COMMENT9\n";
    message << "COMPANY\n";
    message << "FILENAME\n";
    message << "ISSUE_DATE\n";
    message << "LAYER\n";
    message << "PAPER " << _( "(paper size)" ) << "\n";
    message << "REVISION\n";
    message << "SHEETNAME\n";
    message << "SHEETPATH\n";
    message << "TITLE\n";

    dlg.ListSet( message );
    dlg.ShowModal();
}
