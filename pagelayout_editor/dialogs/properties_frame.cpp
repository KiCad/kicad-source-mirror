/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2013 CERN
 * Copyright (C) 2021 KiCad Developers, see AUTHORS.txt for contributors.
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
#include <tool/tool_manager.h>
#include <drawing_sheet/ds_draw_item.h>
#include <drawing_sheet/ds_data_item.h>
#include <drawing_sheet/ds_data_model.h>
#include <view/view.h>

#include "properties_frame.h"
#include "pl_editor_frame.h"
#include "tools/pl_selection_tool.h"

#include <dialogs/html_messagebox.h>


PROPERTIES_FRAME::PROPERTIES_FRAME( PL_EDITOR_FRAME* aParent ) :
        PANEL_PROPERTIES_BASE( aParent ),
        m_scintillaTricks( nullptr ),
        m_textCtrlTextSizeXBinder( aParent, m_staticTextTsizeX, m_textCtrlTextSizeX,
                                   m_TextTextSizeXUnits ),
        m_textCtrlTextSizeYBinder( aParent, m_staticTextTsizeY, m_textCtrlTextSizeY,
                                   m_TextTextSizeYUnits ),
        m_textCtrlConstraintXBinder( aParent, m_staticTextConstraintX, m_textCtrlConstraintX,
                                     m_TextConstraintXUnits ),
        m_textCtrlConstraintYBinder( aParent, m_staticTextConstraintY, m_textCtrlConstraintY,
                                     m_TextConstraintYUnits ),
        m_textCtrlPosXBinder( aParent, m_staticTextPosX, m_textCtrlPosX, m_TextPosXUnits ),
        m_textCtrlPosYBinder( aParent, m_staticTextPosY, m_textCtrlPosY, m_TextPosYUnits ),
        m_textCtrlEndXBinder( aParent, m_staticTextEndX, m_textCtrlEndX, m_TextEndXUnits ),
        m_textCtrlEndYBinder( aParent, m_staticTextEndY, m_textCtrlEndY, m_TextEndYUnits ),
        m_textCtrlStepXBinder( aParent, m_staticTextStepX, m_textCtrlStepX, m_TextStepXUnits ),
        m_textCtrlStepYBinder( aParent, m_staticTextStepY, m_textCtrlStepY, m_TextStepYUnits ),
        m_textCtrlDefaultTextSizeXBinder( aParent, m_staticTextDefTsX, m_textCtrlDefaultTextSizeX,
                                          m_TextDefaultTextSizeXUnits ),
        m_textCtrlDefaultTextSizeYBinder( aParent, m_staticTextDefTsY, m_textCtrlDefaultTextSizeY,
                                          m_TextDefaultTextSizeYUnits ),
        m_textCtrlDefaultLineWidthBinder( aParent, m_staticTextDefLineW,
                                          m_textCtrlDefaultLineWidth, m_TextDefaultLineWidthUnits ),
        m_textCtrlDefaultTextThicknessBinder( aParent, m_staticTextDefTextThickness,
                                              m_textCtrlDefaultTextThickness,
                                              m_TextDefaultTextThicknessUnits ),
        m_textCtrlLeftMarginBinder( aParent, m_staticTextLeftMargin, m_textCtrlLeftMargin,
                                    m_TextLeftMarginUnits ),
        m_textCtrlRightMarginBinder( aParent, m_staticTextDefRightMargin, m_textCtrlRightMargin,
                                     m_TextRightMarginUnits ),
        m_textCtrlTopMarginBinder( aParent, m_staticTextTopMargin, m_textCtrlTopMargin,
                                   m_TextTopMarginUnits ),
        m_textCtrlBottomMarginBinder( aParent, m_staticTextBottomMargin, m_textCtrlBottomMargin,
                                      m_TextBottomMarginUnits ),
        m_textCtrlThicknessBinder( aParent, m_staticTextThickness, m_textCtrlThickness,
                                   m_TextLineThicknessUnits )
{
    m_parent = aParent;

    m_stcText->SetUseVerticalScrollBar( false );
    m_stcText->SetUseHorizontalScrollBar( false );
    m_scintillaTricks = new SCINTILLA_TRICKS( m_stcText, wxT( "{}" ), false );

    m_staticTextSizeInfo->SetFont( KIUI::GetInfoFont( this ) );

    m_buttonOK->SetDefault();

    // ensure sizers are up to date
    // (fix an issue on GTK but should not create issues on other platforms):
    m_swItemProperties->Fit();
    m_swGeneralOpts->Fit();

    m_stcText->Bind( wxEVT_STC_CHARADDED, &PROPERTIES_FRAME::onScintillaCharAdded, this );
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
    DS_DATA_MODEL& model = DS_DATA_MODEL::GetTheInstance();

    // Set default parameters
    m_textCtrlDefaultLineWidthBinder.SetDoubleValue(
            From_User_Unit( EDA_UNITS::MILLIMETRES, model.m_DefaultLineWidth ) );

    m_textCtrlDefaultTextSizeXBinder.SetDoubleValue(
            From_User_Unit( EDA_UNITS::MILLIMETRES, model.m_DefaultTextSize.x ) );

    m_textCtrlDefaultTextSizeYBinder.SetDoubleValue(
            From_User_Unit( EDA_UNITS::MILLIMETRES, model.m_DefaultTextSize.y ) );

    m_textCtrlDefaultTextThicknessBinder.SetDoubleValue(
            From_User_Unit( EDA_UNITS::MILLIMETRES, model.m_DefaultTextThickness ) );

    m_textCtrlLeftMarginBinder.SetDoubleValue( From_User_Unit( EDA_UNITS::MILLIMETRES,
                                                               model.GetLeftMargin() ) );
    m_textCtrlRightMarginBinder.SetDoubleValue( From_User_Unit(  EDA_UNITS::MILLIMETRES,
                                                                 model.GetRightMargin() ) );
    m_textCtrlTopMarginBinder.SetDoubleValue( From_User_Unit( EDA_UNITS::MILLIMETRES,
                                                              model.GetTopMargin() ) );
    m_textCtrlBottomMarginBinder.SetDoubleValue( From_User_Unit( EDA_UNITS::MILLIMETRES,
                                                                 model.GetBottomMargin() ) );
}


bool PROPERTIES_FRAME::CopyPrmsFromPanelToGeneral()
{
    DS_DATA_MODEL& model = DS_DATA_MODEL::GetTheInstance();

    // Import default parameters from widgets
    model.m_DefaultLineWidth =
            To_User_Unit( EDA_UNITS::MILLIMETRES, m_textCtrlDefaultLineWidthBinder.GetValue() );

    model.m_DefaultTextSize.x =
            To_User_Unit( EDA_UNITS::MILLIMETRES, m_textCtrlDefaultTextSizeXBinder.GetValue() );
    model.m_DefaultTextSize.y =
            To_User_Unit( EDA_UNITS::MILLIMETRES, m_textCtrlDefaultTextSizeYBinder.GetValue() );

    model.m_DefaultTextThickness =
            To_User_Unit( EDA_UNITS::MILLIMETRES, m_textCtrlDefaultTextThicknessBinder.GetValue() );

    // Get page margins values
    model.SetRightMargin(
            To_User_Unit( EDA_UNITS::MILLIMETRES, m_textCtrlRightMarginBinder.GetValue() ) );
    model.SetBottomMargin(
            To_User_Unit( EDA_UNITS::MILLIMETRES, m_textCtrlBottomMarginBinder.GetValue() ) );

    // coordinates of the left top corner are the left and top margins
    model.SetLeftMargin(
            To_User_Unit( EDA_UNITS::MILLIMETRES, m_textCtrlLeftMarginBinder.GetValue() ) );
    model.SetTopMargin(
            To_User_Unit( EDA_UNITS::MILLIMETRES, m_textCtrlTopMarginBinder.GetValue() ) );

    return true;
}


void PROPERTIES_FRAME::CopyPrmsFromItemToPanel( DS_DATA_ITEM* aItem )
{
    if( !aItem )
    {
        m_SizerItemProperties->Show( false );
        return;
    }

    wxString msg;

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
    m_textCtrlPosXBinder.SetDoubleValue(
            From_User_Unit( EDA_UNITS::MILLIMETRES, aItem->m_Pos.m_Pos.x ) );

    m_textCtrlPosYBinder.SetDoubleValue(
            From_User_Unit( EDA_UNITS::MILLIMETRES, aItem->m_Pos.m_Pos.y ) );

    switch(  aItem->m_Pos.m_Anchor )
    {
    case RB_CORNER: m_comboBoxCornerPos->SetSelection( 2 ); break;
    case RT_CORNER: m_comboBoxCornerPos->SetSelection( 0 ); break;
    case LB_CORNER: m_comboBoxCornerPos->SetSelection( 3 ); break;
    case LT_CORNER: m_comboBoxCornerPos->SetSelection( 1 ); break;
    }

    // End point
    m_textCtrlEndXBinder.SetDoubleValue(
            From_User_Unit( EDA_UNITS::MILLIMETRES, aItem->m_End.m_Pos.x ) );
    m_textCtrlEndYBinder.SetDoubleValue(
            From_User_Unit( EDA_UNITS::MILLIMETRES, aItem->m_End.m_Pos.y ) );

    switch( aItem->m_End.m_Anchor )
    {
    case RB_CORNER: m_comboBoxCornerEnd->SetSelection( 2 ); break;
    case RT_CORNER: m_comboBoxCornerEnd->SetSelection( 0 ); break;
    case LB_CORNER: m_comboBoxCornerEnd->SetSelection( 3 ); break;
    case LT_CORNER: m_comboBoxCornerEnd->SetSelection( 1 ); break;
    }

    m_textCtrlThicknessBinder.SetDoubleValue(
            From_User_Unit( EDA_UNITS::MILLIMETRES, aItem->m_LineWidth ) );

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

        msg.Printf( wxT("%d"), item->m_IncrementLabel );
        m_textCtrlTextIncrement->SetValue( msg );

        // Rotation (poly and text)
        msg.Printf( wxT("%.3f"), item->m_Orient );
        m_textCtrlRotation->SetValue( msg );

        // Constraints:
        m_textCtrlConstraintXBinder.SetDoubleValue(
                From_User_Unit( EDA_UNITS::MILLIMETRES, item->m_BoundingBoxSize.x ) );
        m_textCtrlConstraintYBinder.SetDoubleValue(
                From_User_Unit( EDA_UNITS::MILLIMETRES, item->m_BoundingBoxSize.y ) );

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
        m_textCtrlTextSizeXBinder.SetDoubleValue(
                From_User_Unit( EDA_UNITS::MILLIMETRES, item->m_TextSize.x ) );
        m_textCtrlTextSizeYBinder.SetDoubleValue(
                From_User_Unit( EDA_UNITS::MILLIMETRES, item->m_TextSize.y ) );
    }

    if( aItem->GetType() == DS_DATA_ITEM::DS_POLYPOLYGON )
    {
        DS_DATA_ITEM_POLYGONS* item = static_cast<DS_DATA_ITEM_POLYGONS*>( aItem );

        // Rotation (poly and text)
        msg.Printf( wxT("%.3f"), item->m_Orient );
        m_textCtrlRotation->SetValue( msg );
    }

    if( aItem->GetType() == DS_DATA_ITEM::DS_BITMAP )
    {
        DS_DATA_ITEM_BITMAP* item = static_cast<DS_DATA_ITEM_BITMAP*>( aItem );

        // select definition in PPI
        msg.Printf( wxT("%d"), item->GetPPI() );
        m_textCtrlBitmapDPI->SetValue( msg );
    }

    m_SizerItemProperties->Show( true );

    m_SizerTextOptions->Show( aItem->GetType() == DS_DATA_ITEM::DS_TEXT );
    m_buttonHelp->Show( aItem->GetType() == DS_DATA_ITEM::DS_TEXT );

    m_sbSizerEndPosition->Show( aItem->GetType() == DS_DATA_ITEM::DS_SEGMENT
                                || aItem->GetType() == DS_DATA_ITEM::DS_RECT );

    m_textCtrlThicknessBinder.Show( aItem->GetType() != DS_DATA_ITEM::DS_BITMAP );

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
    msg.Printf( wxT("%d"), aItem->m_RepeatCount );
    m_textCtrlRepeatCount->SetValue( msg );

    m_textCtrlStepXBinder.SetDoubleValue(
            From_User_Unit( EDA_UNITS::MILLIMETRES, aItem->m_IncrementVector.x ) );

    m_textCtrlStepYBinder.SetDoubleValue(
            From_User_Unit( EDA_UNITS::MILLIMETRES, aItem->m_IncrementVector.y ) );

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

    model.m_DefaultTextSize = DSIZE( TB_DEFAULT_TEXTSIZE, TB_DEFAULT_TEXTSIZE );
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
    aItem->m_LineWidth = To_User_Unit( EDA_UNITS::MILLIMETRES,
                                       m_textCtrlThicknessBinder.GetValue() );

    // Import Start point
    aItem->m_Pos.m_Pos.x = To_User_Unit( EDA_UNITS::MILLIMETRES, m_textCtrlPosXBinder.GetValue() );
    aItem->m_Pos.m_Pos.y = To_User_Unit( EDA_UNITS::MILLIMETRES, m_textCtrlPosYBinder.GetValue() );

    switch( m_comboBoxCornerPos->GetSelection() )
    {
    case 2: aItem->m_Pos.m_Anchor = RB_CORNER; break;
    case 0: aItem->m_Pos.m_Anchor = RT_CORNER; break;
    case 3: aItem->m_Pos.m_Anchor = LB_CORNER; break;
    case 1: aItem->m_Pos.m_Anchor = LT_CORNER; break;
    }

    // Import End point
    aItem->m_End.m_Pos.x = To_User_Unit( EDA_UNITS::MILLIMETRES, m_textCtrlEndXBinder.GetValue() );
    aItem->m_End.m_Pos.y = To_User_Unit( EDA_UNITS::MILLIMETRES, m_textCtrlEndYBinder.GetValue() );

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

    aItem->m_IncrementVector.x = To_User_Unit( EDA_UNITS::MILLIMETRES,
                                               m_textCtrlStepXBinder.GetValue() );
    aItem->m_IncrementVector.y = To_User_Unit( EDA_UNITS::MILLIMETRES,
                                               m_textCtrlStepYBinder.GetValue() );

    if( aItem->GetType() == DS_DATA_ITEM::DS_TEXT )
    {
        DS_DATA_ITEM_TEXT* item = static_cast<DS_DATA_ITEM_TEXT*>( aItem );

        item->m_TextBase = m_stcText->GetValue();

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
        case 0: item->m_Vjustify = GR_TEXT_VJUSTIFY_TOP;    break;
        case 1: item->m_Vjustify = GR_TEXT_VJUSTIFY_CENTER; break;
        case 2: item->m_Vjustify = GR_TEXT_VJUSTIFY_BOTTOM; break;
        }

        msg = m_textCtrlRotation->GetValue();
        item->m_Orient = DoubleValueFromString( EDA_UNITS::UNSCALED, msg );

        // Import text size
        item->m_TextSize.x =
                To_User_Unit( EDA_UNITS::MILLIMETRES, m_textCtrlTextSizeXBinder.GetValue() );
        item->m_TextSize.y =
                To_User_Unit( EDA_UNITS::MILLIMETRES, m_textCtrlTextSizeYBinder.GetValue() );

        // Import constraints:
        item->m_BoundingBoxSize.x =
                To_User_Unit( EDA_UNITS::MILLIMETRES, m_textCtrlConstraintXBinder.GetValue() );
        item->m_BoundingBoxSize.y =
                To_User_Unit( EDA_UNITS::MILLIMETRES, m_textCtrlConstraintYBinder.GetValue() );
    }

    if( aItem->GetType() == DS_DATA_ITEM::DS_POLYPOLYGON )
    {
        DS_DATA_ITEM_POLYGONS* item = static_cast<DS_DATA_ITEM_POLYGONS*>( aItem );

        msg = m_textCtrlRotation->GetValue();
        item->m_Orient = DoubleValueFromString( EDA_UNITS::UNSCALED, msg );
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
    message << _( "Keywords are replaced by they actual value in strings" ) << "<br><br>";
    message << _( "These build-in keywords are always available:" ) << "<br><br>";
    dlg.AddHTML_Text( message );

    message = "KICAD_VERSION\n";
    message << "# " << _( "(sheet number)" ) << "\n";
    message << "## " << _( "(sheet count)" ) << "\n";
    message << "COMMENT1 … COMMENT9\n";
    message << "COMPANY\n";
    message << "FILENAME\n";
    message << "ISSUE_DATE\n";
    message << "LAYER\n";
    message << "PAPER " << _( "(paper size)" ) << "\n";
    message << "REVISION\n";
    message << "SHEETNAME\n";
    message << "TITLE\n";

    dlg.ListSet( message );
    dlg.ShowModal();
}
