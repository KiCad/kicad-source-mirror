/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2023 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <gr_text.h>
#include <ee_actions.h>
#include <sch_edit_frame.h>
#include <widgets/bitmap_button.h>
#include <widgets/color_swatch.h>
#include <widgets/font_choice.h>
#include <settings/color_settings.h>
#include <sch_table.h>
#include <sch_commit.h>
#include <tool/tool_manager.h>
#include <dialog_tablecell_properties.h>


class TABLECELL_SCINTILLA_TRICKS : public SCINTILLA_TRICKS
{
public:
    TABLECELL_SCINTILLA_TRICKS( wxStyledTextCtrl* aScintilla,
                                std::function<void( wxKeyEvent& )> onAcceptHandler,
                                std::function<void()> onNextHandler ) :
            SCINTILLA_TRICKS( aScintilla, wxT( "{}" ), false, std::move( onAcceptHandler ) ),
            m_onNextHandler( std::move( onNextHandler ) )
    { }

protected:
    void onCharHook( wxKeyEvent& aEvent ) override
    {
        if( aEvent.GetKeyCode() == WXK_TAB && aEvent.AltDown() && !aEvent.ControlDown() )
            m_onNextHandler();
        else
            SCINTILLA_TRICKS::onCharHook( aEvent );
    }

private:
    std::function<void()> m_onNextHandler;
};


DIALOG_TABLECELL_PROPERTIES::DIALOG_TABLECELL_PROPERTIES( SCH_EDIT_FRAME* aFrame,
                                                          SCH_TABLECELL* aCell ) :
        DIALOG_TABLECELL_PROPERTIES_BASE( aFrame ),
        m_frame( aFrame ),
        m_table( nullptr ),
        m_cell( aCell ),
        m_borderWidth( aFrame, m_borderWidthLabel, m_borderWidthCtrl, m_borderWidthUnits ),
        m_separatorsWidth( aFrame, m_separatorsWidthLabel, m_separatorsWidthCtrl, m_separatorsWidthUnits ),
        m_textSize( aFrame, m_textSizeLabel, m_textSizeCtrl, m_textSizeUnits ),
        m_scintillaTricks( nullptr )
{
    m_table = static_cast<SCH_TABLE*>( m_cell->GetParent() );

#ifdef _WIN32
    // Without this setting, on Windows, some esoteric unicode chars create display issue
    // in a wxStyledTextCtrl.
    // for SetTechnology() info, see https://www.scintilla.org/ScintillaDoc.html#SCI_SETTECHNOLOGY
    m_textCtrl->SetTechnology(wxSTC_TECHNOLOGY_DIRECTWRITE);
#endif

    m_scintillaTricks = new TABLECELL_SCINTILLA_TRICKS( m_textCtrl,
            // onAccept handler
            [this]( wxKeyEvent& aEvent )
            {
                wxPostEvent( this, wxCommandEvent( wxEVT_COMMAND_BUTTON_CLICKED, wxID_OK ) );
            },
            // onNext handler
            [this]()
            {
                wxCommandEvent dummy;
                OnApply( dummy );
            } );

    // A hack which causes Scintilla to auto-size the text editor canvas
    // See: https://github.com/jacobslusser/ScintillaNET/issues/216
    m_textCtrl->SetScrollWidth( 1 );
    m_textCtrl->SetScrollWidthTracking( true );

    SetInitialFocus( m_textCtrl );

    for( const auto& [lineStyle, lineStyleDesc] : lineTypeNames )
    {
        m_borderStyleCombo->Append( lineStyleDesc.name, KiBitmap( lineStyleDesc.bitmap ) );
        m_separatorsStyleCombo->Append( lineStyleDesc.name, KiBitmap( lineStyleDesc.bitmap ) );
    }

    m_borderStyleCombo->Append( DEFAULT_STYLE );
    m_separatorsStyleCombo->Append( DEFAULT_STYLE );

    if( m_frame->GetColorSettings()->GetOverrideSchItemColors() )
        m_infoBar->ShowMessage( _( "Note: individual item colors overridden in Preferences." ) );

    m_separator1->SetIsSeparator();

    m_bold->SetIsCheckButton();
    m_bold->SetBitmap( KiBitmapBundle( BITMAPS::text_bold ) );
    m_italic->SetIsCheckButton();
    m_italic->SetBitmap( KiBitmapBundle( BITMAPS::text_italic ) );

    m_separator2->SetIsSeparator();

    m_hAlignLeft->SetIsRadioButton();
    m_hAlignLeft->SetBitmap( KiBitmapBundle( BITMAPS::text_align_left ) );
    m_hAlignCenter->SetIsRadioButton();
    m_hAlignCenter->SetBitmap( KiBitmapBundle( BITMAPS::text_align_center ) );
    m_hAlignRight->SetIsRadioButton();
    m_hAlignRight->SetBitmap( KiBitmapBundle( BITMAPS::text_align_right ) );

    m_separator3->SetIsSeparator();

    m_vAlignTop->SetIsRadioButton();
    m_vAlignTop->SetBitmap( KiBitmapBundle( BITMAPS::text_valign_top ) );
    m_vAlignCenter->SetIsRadioButton();
    m_vAlignCenter->SetBitmap( KiBitmapBundle( BITMAPS::text_valign_center ) );
    m_vAlignBottom->SetIsRadioButton();
    m_vAlignBottom->SetBitmap( KiBitmapBundle( BITMAPS::text_valign_bottom ) );

    m_separator4->SetIsSeparator();

    m_hotkeyHint->SetFont( KIUI::GetInfoFont( this ) );
    m_hotkeyHint->SetLabel( wxString::Format( wxT( "(%s+%s)" ),
                                              KeyNameFromKeyCode( WXK_ALT ),
                                              KeyNameFromKeyCode( WXK_TAB ) ) );

    SetupStandardButtons();
    Layout();

    m_hAlignLeft->Bind( wxEVT_BUTTON, &DIALOG_TABLECELL_PROPERTIES::onHAlignButton, this );
    m_hAlignCenter->Bind( wxEVT_BUTTON, &DIALOG_TABLECELL_PROPERTIES::onHAlignButton, this );
    m_hAlignRight->Bind( wxEVT_BUTTON, &DIALOG_TABLECELL_PROPERTIES::onHAlignButton, this );
    m_vAlignTop->Bind( wxEVT_BUTTON, &DIALOG_TABLECELL_PROPERTIES::onVAlignButton, this );
    m_vAlignCenter->Bind( wxEVT_BUTTON, &DIALOG_TABLECELL_PROPERTIES::onVAlignButton, this );
    m_vAlignBottom->Bind( wxEVT_BUTTON, &DIALOG_TABLECELL_PROPERTIES::onVAlignButton, this );

    // Now all widgets have the size fixed, call FinishDialogSettings
    finishDialogSettings();
}


DIALOG_TABLECELL_PROPERTIES::~DIALOG_TABLECELL_PROPERTIES()
{
    delete m_scintillaTricks;
}


bool DIALOG_TABLECELL_PROPERTIES::TransferDataToWindow()
{
    if( !wxDialog::TransferDataToWindow() )
        return false;

    m_borderCheckbox->SetValue( m_table->StrokeExternal() );
    m_headerBorder->SetValue( m_table->StrokeHeader() );

    if( m_table->GetBorderStroke().GetWidth() >= 0 )
        m_borderWidth.SetValue( m_table->GetBorderStroke().GetWidth() );

    m_borderColorSwatch->SetSwatchColor( m_table->GetBorderStroke().GetColor(), false );

    int style = static_cast<int>( m_table->GetBorderStroke().GetLineStyle() );

    if( style == -1 )
        m_borderStyleCombo->SetStringSelection( DEFAULT_STYLE );
    else if( style < (int) lineTypeNames.size() )
        m_borderStyleCombo->SetSelection( style );
    else
        wxFAIL_MSG( "Line type not found in the type lookup map" );

    m_borderWidth.Enable( m_table->StrokeExternal() || m_table->StrokeHeader() );
    m_borderColorLabel->Enable( m_table->StrokeExternal() || m_table->StrokeHeader() );
    m_borderColorSwatch->Enable( m_table->StrokeExternal() || m_table->StrokeHeader() );
    m_borderStyleLabel->Enable( m_table->StrokeExternal() || m_table->StrokeHeader() );
    m_borderStyleCombo->Enable( m_table->StrokeExternal() || m_table->StrokeHeader() );

    bool rows = m_table->StrokeRows() && m_table->GetSeparatorsStroke().GetWidth() >= 0;
    bool cols = m_table->StrokeColumns() && m_table->GetSeparatorsStroke().GetWidth() >= 0;

    m_rowSeparators->SetValue( rows );
    m_colSeparators->SetValue( cols );

    if( m_table->GetSeparatorsStroke().GetWidth() >= 0 )
        m_separatorsWidth.SetValue( m_table->GetSeparatorsStroke().GetWidth() );

    m_separatorsColorSwatch->SetSwatchColor( m_table->GetSeparatorsStroke().GetColor(), false );

    style = static_cast<int>( m_table->GetSeparatorsStroke().GetLineStyle() );

    if( style == -1 )
        m_separatorsStyleCombo->SetStringSelection( DEFAULT_STYLE );
    else if( style < (int) lineTypeNames.size() )
        m_separatorsStyleCombo->SetSelection( style );
    else
        wxFAIL_MSG( "Line type not found in the type lookup map" );

    m_separatorsWidth.Enable( rows || cols );
    m_separatorsColorLabel->Enable( rows || cols );
    m_separatorsColorSwatch->Enable( rows || cols );
    m_separatorsStyleLabel->Enable( rows || cols );
    m_separatorsStyleCombo->Enable( rows || cols );

    m_textCtrl->SetValue( m_cell->GetText() );
    m_fontCtrl->SetFontSelection( m_cell->GetFont() );
    m_textSize.SetValue( m_cell->GetTextWidth() );

    m_bold->Check( m_cell->IsBold() );
    m_italic->Check( m_cell->IsItalic() );

    switch( m_cell->GetHorizJustify() )
    {
    case GR_TEXT_H_ALIGN_LEFT:   m_hAlignLeft->Check();   break;
    case GR_TEXT_H_ALIGN_CENTER: m_hAlignCenter->Check(); break;
    case GR_TEXT_H_ALIGN_RIGHT:  m_hAlignRight->Check();  break;
    }

    switch( m_cell->GetVertJustify() )
    {
    case GR_TEXT_V_ALIGN_TOP:    m_vAlignTop->Check();    break;
    case GR_TEXT_V_ALIGN_CENTER: m_vAlignCenter->Check(); break;
    case GR_TEXT_V_ALIGN_BOTTOM: m_vAlignBottom->Check(); break;
    }

    m_textColorSwatch->SetSwatchColor( m_cell->GetTextColor(), false );

    if( m_cell->IsFilled() )
        m_fillColorSwatch->SetSwatchColor( m_cell->GetFillColor(), false );
    else
        m_fillColorSwatch->SetSwatchColor( COLOR4D::UNSPECIFIED, false );

    return true;
}


void DIALOG_TABLECELL_PROPERTIES::onHAlignButton( wxCommandEvent& aEvent )
{
    for( BITMAP_BUTTON* btn : { m_hAlignLeft, m_hAlignCenter, m_hAlignRight } )
    {
        if( btn->IsChecked() && btn != aEvent.GetEventObject() )
            btn->Check( false );
    }
}


void DIALOG_TABLECELL_PROPERTIES::onVAlignButton( wxCommandEvent& aEvent )
{
    for( BITMAP_BUTTON* btn : { m_vAlignTop, m_vAlignCenter, m_vAlignBottom } )
    {
        if( btn->IsChecked() && btn != aEvent.GetEventObject() )
            btn->Check( false );
    }
}


void DIALOG_TABLECELL_PROPERTIES::onBorderChecked( wxCommandEvent& aEvent )
{
    bool border = m_borderCheckbox->GetValue();

    if( border && m_borderWidth.GetValue() < 0 )
        m_borderWidth.SetValue( m_frame->eeconfig()->m_Drawing.default_line_thickness );

    m_borderWidth.Enable( border );
    m_borderColorLabel->Enable( border );
    m_borderColorSwatch->Enable( border );
    m_borderStyleLabel->Enable( border );
    m_borderStyleCombo->Enable( border );

    bool row = m_rowSeparators->GetValue();
    bool col = m_colSeparators->GetValue();

    if( ( row || col ) && m_separatorsWidth.GetValue() < 0 )
        m_separatorsWidth.SetValue( m_frame->eeconfig()->m_Drawing.default_line_thickness );

    m_separatorsWidth.Enable( row || col );
    m_separatorsColorLabel->Enable( row || col );
    m_separatorsColorSwatch->Enable( row || col );
    m_separatorsStyleLabel->Enable( row || col );
    m_separatorsStyleCombo->Enable( row || col );
}


void DIALOG_TABLECELL_PROPERTIES::OnCharHook( wxKeyEvent& aEvt )
{
    if( aEvt.GetKeyCode() == WXK_TAB && aEvt.AltDown() && !aEvt.ControlDown() )
    {
        wxCommandEvent dummy;
        OnApply( dummy );
    }
    else
    {
        DIALOG_SHIM::OnCharHook( aEvt );
    }
}


void DIALOG_TABLECELL_PROPERTIES::OnApply( wxCommandEvent& aEvent )
{
    TransferDataFromWindow();

    for( size_t ii = 0; ii < m_table->GetCells().size(); ++ii )
    {
        if( m_table->GetCells()[ii] == m_cell )
        {
            ii++;

            if( ii >= m_table->GetCells().size() )
                ii = 0;

            m_cell = m_table->GetCells()[ii];

            m_frame->GetToolManager()->RunAction( EE_ACTIONS::clearSelection );
            m_frame->GetToolManager()->RunAction<EDA_ITEM*>( EE_ACTIONS::addItemToSel, m_cell );
            break;
        }
    }

    TransferDataToWindow();
    m_textCtrl->SelectAll();
}


bool DIALOG_TABLECELL_PROPERTIES::TransferDataFromWindow()
{
    if( !wxDialog::TransferDataFromWindow() )
        return false;

    SCH_COMMIT commit( m_frame );

    /* save table in undo list if not already in edit */
    if( m_table->GetEditFlags() == 0 )
        commit.Modify( m_table, m_frame->GetScreen() );

    m_table->SetStrokeExternal( m_borderCheckbox->GetValue() );
    m_table->SetStrokeHeader( m_headerBorder->GetValue() );
    {
        STROKE_PARAMS stroke = m_table->GetBorderStroke();

        if( m_borderCheckbox->GetValue() )
            stroke.SetWidth( std::max( 0, m_borderWidth.GetIntValue() ) );
        else
            stroke.SetWidth( -1 );

        auto it = lineTypeNames.begin();
        std::advance( it, m_borderStyleCombo->GetSelection() );

        if( it == lineTypeNames.end() )
            stroke.SetLineStyle( LINE_STYLE::DEFAULT );
        else
            stroke.SetLineStyle( it->first );

        stroke.SetColor( m_borderColorSwatch->GetSwatchColor() );

        m_table->SetBorderStroke( stroke );
    }

    m_table->SetStrokeRows( m_rowSeparators->GetValue() );
    m_table->SetStrokeColumns( m_colSeparators->GetValue() );
    {
        STROKE_PARAMS stroke = m_table->GetSeparatorsStroke();

        if( m_rowSeparators->GetValue() || m_colSeparators->GetValue() )
            stroke.SetWidth( std::max( 0, m_separatorsWidth.GetIntValue() ) );
        else
            stroke.SetWidth( -1 );

        auto it = lineTypeNames.begin();
        std::advance( it, m_separatorsStyleCombo->GetSelection() );

        if( it == lineTypeNames.end() )
            stroke.SetLineStyle( LINE_STYLE::DEFAULT );
        else
            stroke.SetLineStyle( it->first );

        stroke.SetColor( m_separatorsColorSwatch->GetSwatchColor() );

        m_table->SetSeparatorsStroke( stroke );
    }

    m_cell->SetText( m_textCtrl->GetValue() );

    if( m_fontCtrl->HaveFontSelection() )
    {
        m_cell->SetFont( m_fontCtrl->GetFontSelection( m_bold->IsChecked(),
                                                       m_italic->IsChecked() ) );
    }

    if( m_cell->GetTextWidth() != m_textSize.GetValue() )
        m_cell->SetTextSize( VECTOR2I( m_textSize.GetValue(), m_textSize.GetValue() ) );

    m_cell->SetTextColor( m_textColorSwatch->GetSwatchColor() );

    if( m_bold->IsChecked() != m_cell->IsBold() )
    {
        if( m_bold->IsChecked() )
        {
            m_cell->SetBold( true );
            m_cell->SetTextThickness( GetPenSizeForBold( m_cell->GetTextWidth() ) );
        }
        else
        {
            m_cell->SetBold( false );
            m_cell->SetTextThickness( 0 ); // Use default pen width
        }
    }

    if( m_hAlignRight->IsChecked() )
        m_cell->SetHorizJustify( GR_TEXT_H_ALIGN_RIGHT );
    else if( m_hAlignCenter->IsChecked() )
        m_cell->SetHorizJustify( GR_TEXT_H_ALIGN_CENTER );
    else
        m_cell->SetHorizJustify( GR_TEXT_H_ALIGN_LEFT );

    if( m_vAlignBottom->IsChecked() )
        m_cell->SetVertJustify( GR_TEXT_V_ALIGN_BOTTOM );
    else if( m_vAlignCenter->IsChecked() )
        m_cell->SetVertJustify( GR_TEXT_V_ALIGN_CENTER );
    else
        m_cell->SetVertJustify( GR_TEXT_V_ALIGN_TOP );

    COLOR4D fillColor = m_fillColorSwatch->GetSwatchColor();

    if( fillColor == COLOR4D::UNSPECIFIED )
    {
        m_cell->SetFillMode( FILL_T::NO_FILL );
    }
    else
    {
        m_cell->SetFillMode( FILL_T::FILLED_WITH_COLOR );
        m_cell->SetFillColor( fillColor );
    }

    if( !commit.Empty() )
        commit.Push( _( "Edit Table Cell" ), SKIP_CONNECTIVITY );

    return true;
}
