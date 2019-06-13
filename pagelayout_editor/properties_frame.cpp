/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2013 CERN
 * Copyright (C) 2019 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <fctsys.h>
#include <ws_draw_item.h>
#include <ws_data_model.h>
#include <properties_frame.h>
#include <tool/tool_manager.h>
#include <tools/pl_selection_tool.h>
#include <pl_draw_panel_gal.h>

PROPERTIES_FRAME::PROPERTIES_FRAME( PL_EDITOR_FRAME* aParent ):
    PANEL_PROPERTIES_BASE( aParent )
{
    m_parent = aParent;

    wxFont infoFont = wxSystemSettings::GetFont( wxSYS_DEFAULT_GUI_FONT );
    infoFont.SetSymbolicSize( wxFONTSIZE_SMALL );
    m_staticTextSizeInfo->SetFont( infoFont );
    infoFont.SetSymbolicSize( wxFONTSIZE_X_SMALL );
    m_staticTextInfoThickness->SetFont( infoFont );

    m_buttonOK->SetDefault();
}


PROPERTIES_FRAME::~PROPERTIES_FRAME()
{
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


// Data transfert from general properties to widgets
void PROPERTIES_FRAME::CopyPrmsFromGeneralToPanel()
{
    WS_DATA_MODEL& model = WS_DATA_MODEL::GetTheInstance();
    wxString       msg;

    // Set default parameters
    msg.Printf( wxT("%.3f"),  model.m_DefaultLineWidth );
    m_textCtrlDefaultLineWidth->SetValue( msg );

    msg.Printf( wxT("%.3f"), model.m_DefaultTextSize.x );
    m_textCtrlDefaultTextSizeX->SetValue( msg );
    msg.Printf( wxT("%.3f"),  model.m_DefaultTextSize.y );
    m_textCtrlDefaultTextSizeY->SetValue( msg );

    msg.Printf( wxT("%.3f"),  model.m_DefaultTextThickness );
    m_textCtrlDefaultTextThickness->SetValue( msg );

    // Set page margins values
    msg.Printf( wxT("%.3f"),  model.GetRightMargin() );
    m_textCtrlRightMargin->SetValue( msg );
    msg.Printf( wxT("%.3f"),  model.GetBottomMargin() );
    m_textCtrlDefaultBottomMargin->SetValue( msg );

    msg.Printf( wxT("%.3f"),  model.GetLeftMargin() );
    m_textCtrlLeftMargin->SetValue( msg );
    msg.Printf( wxT("%.3f"),  model.GetTopMargin() );
    m_textCtrlTopMargin->SetValue( msg );
}

// Data transfert from widgets to general properties
bool PROPERTIES_FRAME::CopyPrmsFromPanelToGeneral()
{
    WS_DATA_MODEL& model = WS_DATA_MODEL::GetTheInstance();
    wxString       msg;

    // Import default parameters from widgets
    msg = m_textCtrlDefaultLineWidth->GetValue();
    model.m_DefaultLineWidth = DoubleValueFromString( UNSCALED_UNITS, msg );

    msg = m_textCtrlDefaultTextSizeX->GetValue();
    model.m_DefaultTextSize.x = DoubleValueFromString( UNSCALED_UNITS, msg );
    msg = m_textCtrlDefaultTextSizeY->GetValue();
    model.m_DefaultTextSize.y = DoubleValueFromString( UNSCALED_UNITS, msg );

    msg = m_textCtrlDefaultTextThickness->GetValue();
    model.m_DefaultTextThickness = DoubleValueFromString( UNSCALED_UNITS, msg );

    // Get page margins values
    msg = m_textCtrlRightMargin->GetValue();
    model.SetRightMargin( DoubleValueFromString( UNSCALED_UNITS, msg ) );
    msg = m_textCtrlDefaultBottomMargin->GetValue();
    model.SetBottomMargin( DoubleValueFromString( UNSCALED_UNITS, msg ) );

    // cordinates of the left top corner are the left and top margins
    msg = m_textCtrlLeftMargin->GetValue();
    model.SetLeftMargin( DoubleValueFromString( UNSCALED_UNITS, msg ) );
    msg = m_textCtrlTopMargin->GetValue();
    model.SetTopMargin( DoubleValueFromString( UNSCALED_UNITS, msg ) );

    return true;
}


// Data transfert from item to widgets in properties frame
void PROPERTIES_FRAME::CopyPrmsFromItemToPanel( WS_DATA_ITEM* aItem )
{
    if( !aItem )
    {
        m_SizerItemProperties->Show( false );
        return;
    }

    wxString msg;

    // Set parameters common to all WS_DATA_ITEM types
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
    msg.Printf( wxT("%.3f"), aItem->m_Pos.m_Pos.x );
    m_textCtrlPosX->SetValue( msg );
    msg.Printf( wxT("%.3f"), aItem->m_Pos.m_Pos.y );
    m_textCtrlPosY->SetValue( msg );

    switch(  aItem->m_Pos.m_Anchor )
    {
    case RB_CORNER: m_comboBoxCornerPos->SetSelection( 2 ); break;
    case RT_CORNER: m_comboBoxCornerPos->SetSelection( 0 ); break;
    case LB_CORNER: m_comboBoxCornerPos->SetSelection( 3 ); break;
    case LT_CORNER: m_comboBoxCornerPos->SetSelection( 1 ); break;
    }

    // End point
    msg.Printf( wxT("%.3f"), aItem->m_End.m_Pos.x );
    m_textCtrlEndX->SetValue( msg );
    msg.Printf( wxT("%.3f"), aItem->m_End.m_Pos.y );
    m_textCtrlEndY->SetValue( msg );

    switch( aItem->m_End.m_Anchor )
    {
    case RB_CORNER: m_comboBoxCornerEnd->SetSelection( 2 ); break;
    case RT_CORNER: m_comboBoxCornerEnd->SetSelection( 0 ); break;
    case LB_CORNER: m_comboBoxCornerEnd->SetSelection( 3 ); break;
    case LT_CORNER: m_comboBoxCornerEnd->SetSelection( 1 ); break;
    }

    msg.Printf( wxT("%.3f"), aItem->m_LineWidth );
    m_textCtrlThickness->SetValue( msg );

    // Now, set prms more specific to WS_DATA_ITEM types
    // For a given type, disable widgets which are not relevant,
    // and be sure widgets which are relevant are enabled
    if( aItem->GetType() == WS_DATA_ITEM::WS_TEXT )
    {
        WS_DATA_ITEM_TEXT* item = (WS_DATA_ITEM_TEXT*) aItem;
        item->m_FullText = item->m_TextBase;
        // Replace our '\' 'n' sequence by the EOL char
        item->ReplaceAntiSlashSequence();
        m_textCtrlText->SetValue( item->m_FullText );

        msg.Printf( wxT("%d"), item->m_IncrementLabel );
        m_textCtrlTextIncrement->SetValue( msg );

        // Rotation (poly and text)
        msg.Printf( wxT("%.3f"), item->m_Orient );
        m_textCtrlRotation->SetValue( msg );

        // Constraints:
        msg.Printf( wxT("%.3f"), item->m_BoundingBoxSize.x );
        m_textCtrlConstraintX->SetValue( msg );
        msg.Printf( wxT("%.3f"), item->m_BoundingBoxSize.y );
        m_textCtrlConstraintY->SetValue( msg );

        // Font Options
		m_checkBoxBold->SetValue( item->m_Bold );
		m_checkBoxItalic->SetValue( item->m_Italic );

        switch( item->m_Hjustify )
        {
        case GR_TEXT_HJUSTIFY_LEFT:   m_choiceHjustify->SetSelection( 0 ); break;
        case GR_TEXT_HJUSTIFY_CENTER: m_choiceHjustify->SetSelection( 1 ); break;
        case GR_TEXT_HJUSTIFY_RIGHT:  m_choiceHjustify->SetSelection( 2 ); break;
        }

        switch( item->m_Vjustify )
        {
        case GR_TEXT_VJUSTIFY_TOP:    m_choiceVjustify->SetSelection( 0 ); break;
        case GR_TEXT_VJUSTIFY_CENTER: m_choiceVjustify->SetSelection( 1 ); break;
        case GR_TEXT_VJUSTIFY_BOTTOM: m_choiceVjustify->SetSelection( 2 ); break;
        }

        // Text size
        msg.Printf( wxT("%.3f"), item->m_TextSize.x );
        m_textCtrlTextSizeX->SetValue( msg );
        msg.Printf( wxT("%.3f"), item->m_TextSize.y );
        m_textCtrlTextSizeY->SetValue( msg );
    }

    if( aItem->GetType() == WS_DATA_ITEM::WS_POLYPOLYGON )
    {
        WS_DATA_ITEM_POLYGONS* item = (WS_DATA_ITEM_POLYGONS*) aItem;
        // Rotation (poly and text)
        msg.Printf( wxT("%.3f"), item->m_Orient );
        m_textCtrlRotation->SetValue( msg );
    }

    if( aItem->GetType() == WS_DATA_ITEM::WS_BITMAP )
    {
        WS_DATA_ITEM_BITMAP* item = (WS_DATA_ITEM_BITMAP*) aItem;
        // select definition in PPI
        msg.Printf( wxT("%d"), item->GetPPI() );
        m_textCtrlBitmapPPI->SetValue( msg );
    }

    m_SizerItemProperties->Show( true );

    m_SizerTextOptions->Show( aItem->GetType() == WS_DATA_ITEM::WS_TEXT );

    m_SizerEndPosition->Show( aItem->GetType() == WS_DATA_ITEM::WS_SEGMENT
                           || aItem->GetType() == WS_DATA_ITEM::WS_RECT );

    m_SizerLineThickness->Show( aItem->GetType() != WS_DATA_ITEM::WS_BITMAP );
    // Polygons have no defaut value for line width
    m_staticTextInfoThickness->Show( aItem->GetType() != WS_DATA_ITEM::WS_POLYPOLYGON );

    m_SizerRotation->Show( aItem->GetType() == WS_DATA_ITEM::WS_TEXT
                        || aItem->GetType() == WS_DATA_ITEM::WS_POLYPOLYGON );

    m_SizerPPI->Show( aItem->GetType() == WS_DATA_ITEM::WS_BITMAP );

    m_staticTextInclabel->Show( aItem->GetType() == WS_DATA_ITEM::WS_TEXT );
    m_textCtrlTextIncrement->Show( aItem->GetType() == WS_DATA_ITEM::WS_TEXT );

    // Repeat parameters
    msg.Printf( wxT("%d"), aItem->m_RepeatCount );
    m_textCtrlRepeatCount->SetValue( msg );
    msg.Printf( wxT("%.3f"), aItem->m_IncrementVector.x );
    m_textCtrlStepX->SetValue( msg );
    msg.Printf( wxT("%.3f"), aItem->m_IncrementVector.y );
    m_textCtrlStepY->SetValue( msg );

    // The number of widgets was modified
    m_swItemProperties->Layout();
    m_swItemProperties->Refresh();
}


// Event function called by clicking on the OK button
void PROPERTIES_FRAME::OnAcceptPrms( wxCommandEvent& event )
{
    PL_SELECTION_TOOL* selTool = m_parent->GetToolManager()->GetTool<PL_SELECTION_TOOL>();
    PL_SELECTION&      selection = selTool->GetSelection();

    m_parent->SaveCopyInUndoList();

    WS_DRAW_ITEM_BASE* drawItem = (WS_DRAW_ITEM_BASE*) selection.Front();

    if( drawItem )
    {
        WS_DATA_ITEM* dataItem = drawItem->GetPeer();
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
    m_parent->GetCanvas()->DisplayWorksheet();
    m_parent->GetCanvas()->Refresh();
}


void PROPERTIES_FRAME::OnSetDefaultValues( wxCommandEvent& event )
{
    WS_DATA_MODEL& model = WS_DATA_MODEL::GetTheInstance();

    model.m_DefaultTextSize = DSIZE( TB_DEFAULT_TEXTSIZE, TB_DEFAULT_TEXTSIZE );
    model.m_DefaultLineWidth = 0.15;
    model.m_DefaultTextThickness = 0.15;

    CopyPrmsFromGeneralToPanel();

    // Rebuild the draw list with the new parameters
    m_parent->GetCanvas()->DisplayWorksheet();
    m_parent->GetCanvas()->Refresh();
}


// Data transfert from  properties frame to item parameters
bool PROPERTIES_FRAME::CopyPrmsFromPanelToItem( WS_DATA_ITEM* aItem )
{
    if( aItem == NULL )
        return false;

    wxString msg;

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
    msg = m_textCtrlThickness->GetValue();
    aItem->m_LineWidth = DoubleValueFromString( UNSCALED_UNITS, msg );

    // Import Start point
    msg = m_textCtrlPosX->GetValue();
    aItem->m_Pos.m_Pos.x = DoubleValueFromString( UNSCALED_UNITS, msg );

    msg = m_textCtrlPosY->GetValue();
    aItem->m_Pos.m_Pos.y = DoubleValueFromString( UNSCALED_UNITS, msg );

    switch( m_comboBoxCornerPos->GetSelection() )
    {
    case 2: aItem->m_Pos.m_Anchor = RB_CORNER; break;
    case 0: aItem->m_Pos.m_Anchor = RT_CORNER; break;
    case 3: aItem->m_Pos.m_Anchor = LB_CORNER; break;
    case 1: aItem->m_Pos.m_Anchor = LT_CORNER; break;
    }

    // Import End point
    msg = m_textCtrlEndX->GetValue();
    aItem->m_End.m_Pos.x = DoubleValueFromString( UNSCALED_UNITS, msg );

    msg = m_textCtrlEndY->GetValue();
    aItem->m_End.m_Pos.y = DoubleValueFromString( UNSCALED_UNITS, msg );

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
    aItem->m_RepeatCount = itmp;

    msg = m_textCtrlStepX->GetValue();
    aItem->m_IncrementVector.x = DoubleValueFromString( UNSCALED_UNITS, msg );

    msg = m_textCtrlStepY->GetValue();
    aItem->m_IncrementVector.y = DoubleValueFromString( UNSCALED_UNITS, msg );

    if( aItem->GetType() == WS_DATA_ITEM::WS_TEXT )
    {
        WS_DATA_ITEM_TEXT* item = (WS_DATA_ITEM_TEXT*) aItem;

        item->m_TextBase = m_textCtrlText->GetValue();

        msg = m_textCtrlTextIncrement->GetValue();
        msg.ToLong( &itmp );
        item->m_IncrementLabel = itmp;

        item->m_Bold = m_checkBoxBold->IsChecked();
        item->m_Italic = m_checkBoxItalic->IsChecked();

        switch( m_choiceHjustify->GetSelection() )
        {
        case 0: item->m_Hjustify = GR_TEXT_HJUSTIFY_LEFT; break;
        case 1: item->m_Hjustify = GR_TEXT_HJUSTIFY_CENTER; break;
        case 2: item->m_Hjustify = GR_TEXT_HJUSTIFY_RIGHT; break;
        }

        switch( m_choiceVjustify->GetSelection() )
        {
        case 0: item->m_Vjustify = GR_TEXT_VJUSTIFY_TOP; break;
        case 1: item->m_Vjustify = GR_TEXT_VJUSTIFY_CENTER; break;
        case 2: item->m_Vjustify = GR_TEXT_VJUSTIFY_BOTTOM; break;
        }

        msg = m_textCtrlRotation->GetValue();
        item->m_Orient = DoubleValueFromString( UNSCALED_UNITS, msg );

        // Import text size
        msg = m_textCtrlTextSizeX->GetValue();
        item->m_TextSize.x = DoubleValueFromString( UNSCALED_UNITS, msg );

        msg = m_textCtrlTextSizeY->GetValue();
        item->m_TextSize.y = DoubleValueFromString( UNSCALED_UNITS, msg );

        // Import constraints:
        msg = m_textCtrlConstraintX->GetValue();
        item->m_BoundingBoxSize.x = DoubleValueFromString( UNSCALED_UNITS, msg );

        msg = m_textCtrlConstraintY->GetValue();
        item->m_BoundingBoxSize.y = DoubleValueFromString( UNSCALED_UNITS, msg );
    }

    if( aItem->GetType() == WS_DATA_ITEM::WS_POLYPOLYGON )
    {
        WS_DATA_ITEM_POLYGONS* item = (WS_DATA_ITEM_POLYGONS*) aItem;

        msg = m_textCtrlRotation->GetValue();
        item->m_Orient = DoubleValueFromString( UNSCALED_UNITS, msg );
    }

    if( aItem->GetType() == WS_DATA_ITEM::WS_BITMAP )
    {
        WS_DATA_ITEM_BITMAP* item = (WS_DATA_ITEM_BITMAP*) aItem;
        // Set definition in PPI
        long value;
        msg = m_textCtrlBitmapPPI->GetValue();
        if( msg.ToLong( &value ) )
            item->SetPPI( (int)value );
    }

    return true;
}

