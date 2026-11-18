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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */


#include <pcb_edit_frame.h>
#include <board_design_settings.h>
#include <dialogs/dialog_text_entry.h>
#include <dialogs/panel_setup_constraint_rules.h>
#include <panel_setup_mask_and_paste.h>
#include <widgets/paged_dialog.h>


PANEL_SETUP_MASK_AND_PASTE::PANEL_SETUP_MASK_AND_PASTE( wxWindow* aParentWindow, PCB_EDIT_FRAME* aFrame,
                                                        BOARD_DESIGN_SETTINGS& aSettings ) :
        PANEL_SETUP_MASK_AND_PASTE_BASE( aParentWindow ),
        m_maskExpansion( aFrame, m_maskMarginLabel, m_maskMarginCtrl, m_maskMarginUnits ),
        m_maskMinWidth( aFrame, m_maskMinWidthLabel, m_maskMinWidthCtrl, m_maskMinWidthUnits ),
        m_maskToCopperClearance( aFrame, m_maskToCopperClearanceLabel, m_maskToCopperClearanceCtrl,
                                 m_maskToCopperClearanceUnits ),
        m_pasteMargin( aFrame, m_pasteMarginLabel, m_pasteMarginCtrl, m_pasteMarginUnits )
{
    m_Frame = aFrame;
    m_BrdSettings = &aSettings;

    m_staticTextInfoPaste->SetFont( KIUI::GetInfoFont( this ).Italic() );

    // Update label text and tooltip for combined offset + ratio field
    m_pasteMarginLabel->SetLabel( _( "Solder paste clearance:" ) );
    m_pasteMarginLabel->SetToolTip( _( "Solder paste clearance relative to pad size.\n"
                                       "Enter an absolute value (e.g., -0.1mm), a percentage "
                                       "(e.g., -5%), or both (e.g., -0.1mm - 5%).\n"
                                       "This value can be superseded by local values for a "
                                       "footprint or a pad." ) );

    // Hide the old ratio controls - they're no longer needed
    m_pasteMarginRatioLabel->Show( false );
    m_pasteMarginRatioCtrl->Show( false );
    m_pasteMarginRatioUnits->Show( false );

    // These two are the values the mask rules are read against, so they are set with those rules.
    // The paste clearance stays here, because one control holds it and its ratio together.
    m_maskExpansionPage = PANEL_SETUP_CONSTRAINT_RULES::PageEditingDefault( wxS( "solder_mask_expansion" ) );
    m_maskMinWidthPage = PANEL_SETUP_CONSTRAINT_RULES::PageEditingDefault( wxS( "solder_mask_min_width" ) );

    if( !m_maskExpansionPage.IsEmpty() )
        PANEL_SETUP_CONSTRAINT_RULES::LinkToPage( m_maskMarginCtrl, m_maskExpansionPage, 0 );

    if( !m_maskMinWidthPage.IsEmpty() )
        PANEL_SETUP_CONSTRAINT_RULES::LinkToPage( m_maskMinWidthCtrl, m_maskMinWidthPage, 0 );

    // Only the absolute part of this control is a constraint a rule page holds; the ratio is a
    // constraint of its own with no page, so it is only ever set here
    m_pasteMarginPage = PANEL_SETUP_CONSTRAINT_RULES::PageEditingDefault( wxS( "solder_paste_margin" ) );

    if( !m_pasteMarginPage.IsEmpty() )
        PANEL_SETUP_CONSTRAINT_RULES::LinkToPage( m_pasteMarginCtrl, m_pasteMarginPage, 0 );
}


bool PANEL_SETUP_MASK_AND_PASTE::TransferDataToWindow()
{
    // Also records the baselines for the values a rule page edits
    RefreshDefaults();

    m_maskToCopperClearance.SetValue( m_BrdSettings->m_SolderMaskToCopperClearance );
    m_pasteMargin.SetRatioValue( m_BrdSettings->m_SolderPasteMarginRatio );

    m_allowBridges->SetValue( m_BrdSettings->m_AllowSoldermaskBridgesInFPs );

    return true;
}


bool PANEL_SETUP_MASK_AND_PASTE::TransferDataFromWindow()
{
    // These are all stored in project file, not board, so no need for OnModify()

    wxString message;

    if( wxWindow* field = firstInvalidField( &message ) )
    {
        PAGED_DIALOG::GetDialog( this )->SetError( message, this, field );
        return false;
    }

    writeEditedDefaults();

    m_BrdSettings->m_SolderMaskToCopperClearance = m_maskToCopperClearance.GetIntValue();
    m_BrdSettings->m_SolderPasteMarginRatio = m_pasteMargin.GetRatioValue().value_or( 0.0 );
    m_BrdSettings->m_AllowSoldermaskBridgesInFPs = m_allowBridges->GetValue();

    return true;
}


bool PANEL_SETUP_MASK_AND_PASTE::StageEditedDefaults()
{
    return !firstInvalidField( nullptr ) && writeEditedDefaults();
}


std::vector<std::pair<wxString, wxString>> PANEL_SETUP_MASK_AND_PASTE::StaleSearchValues() const
{
    std::vector<std::pair<wxString, wxString>> stale;

    auto check = [&]( wxTextCtrl* aCtrl, int aStaged, int aLoaded, int aShown, const wxString& aCurrent )
    {
        if( aStaged != aLoaded && aShown == aLoaded )
            stale.emplace_back( aCtrl->GetValue(), aCurrent );
    };

    const int expansion = m_BrdSettings->m_SolderMaskExpansion;
    const int minWidth = m_BrdSettings->m_SolderMaskMinWidth;
    const int paste = m_BrdSettings->m_SolderPasteMargin;

    check( m_maskMarginCtrl, expansion, m_maskExpansionLoaded, m_maskExpansion.GetIntValue(),
           m_Frame->StringFromValue( expansion, true ) );
    check( m_maskMinWidthCtrl, minWidth, m_maskMinWidthLoaded, m_maskMinWidth.GetIntValue(),
           m_Frame->StringFromValue( minWidth, true ) );
    check( m_pasteMarginCtrl, paste, m_pasteMarginLoaded, m_pasteMargin.GetOffsetValue().value_or( 0 ),
           m_pasteMargin.formatValue( paste, m_pasteMargin.GetRatioValue() ) );

    return stale;
}


wxWindow* PANEL_SETUP_MASK_AND_PASTE::firstInvalidField( wxString* aMessage ) const
{
    BOARD_DESIGN_SETTINGS temp( *m_BrdSettings );

    temp.m_SolderMaskExpansion = m_maskExpansion.GetIntValue();
    temp.m_SolderMaskMinWidth = m_maskMinWidth.GetIntValue();
    temp.m_SolderPasteMargin = m_pasteMargin.GetOffsetValue().value_or( 0 );

    // Only values a rule page shares are checked, since staging hands them to a page that rejects them
    const std::map<wxString, wxWindow*> fieldToControl = { { wxS( "solder_mask_expansion" ), m_maskMarginCtrl },
                                                           { wxS( "solder_mask_min_width" ), m_maskMinWidthCtrl },
                                                           { wxS( "solder_paste_margin" ), m_pasteMarginCtrl } };

    for( const BOARD_DESIGN_SETTINGS::VALIDATION_ERROR& error : temp.ValidateDesignRules( m_Frame->GetUserUnits() ) )
    {
        auto it = fieldToControl.find( error.setting_name );

        if( it == fieldToControl.end() )
            continue;

        if( aMessage )
            *aMessage = error.error_message;

        return it->second;
    }

    return nullptr;
}


bool PANEL_SETUP_MASK_AND_PASTE::writeEditedDefaults()
{
    bool written = false;

    // These can also be set with the rules read against them, so a field the user did not touch
    // here must not write back over an edit made on that page
    if( m_maskExpansion.GetIntValue() != m_maskExpansionLoaded )
    {
        m_maskExpansionLoaded = m_maskExpansion.GetIntValue();
        m_BrdSettings->m_SolderMaskExpansion = m_maskExpansionLoaded;
        written = true;
    }

    if( m_maskMinWidth.GetIntValue() != m_maskMinWidthLoaded )
    {
        m_maskMinWidthLoaded = m_maskMinWidth.GetIntValue();
        m_BrdSettings->m_SolderMaskMinWidth = m_maskMinWidthLoaded;
        written = true;
    }

    if( m_pasteMargin.GetOffsetValue().value_or( 0 ) != m_pasteMarginLoaded )
    {
        m_pasteMarginLoaded = m_pasteMargin.GetOffsetValue().value_or( 0 );
        m_BrdSettings->m_SolderPasteMargin = m_pasteMarginLoaded;
        written = true;
    }

    return written;
}


void PANEL_SETUP_MASK_AND_PASTE::RefreshDefaults()
{
    m_maskExpansionLoaded = m_BrdSettings->m_SolderMaskExpansion;
    m_maskExpansion.SetValue( m_maskExpansionLoaded );

    m_maskMinWidthLoaded = m_BrdSettings->m_SolderMaskMinWidth;
    m_maskMinWidth.SetValue( m_maskMinWidthLoaded );

    m_pasteMarginLoaded = m_BrdSettings->m_SolderPasteMargin;
    m_pasteMargin.SetOffsetValue( m_pasteMarginLoaded );
}


void PANEL_SETUP_MASK_AND_PASTE::CopySettings( const BOARD_DESIGN_SETTINGS& aFrom, BOARD_DESIGN_SETTINGS& aTo )
{
    aTo.m_SolderMaskExpansion = aFrom.m_SolderMaskExpansion;
    aTo.m_SolderMaskMinWidth = aFrom.m_SolderMaskMinWidth;
    aTo.m_SolderMaskToCopperClearance = aFrom.m_SolderMaskToCopperClearance;
    aTo.m_SolderPasteMargin = aFrom.m_SolderPasteMargin;
    aTo.m_SolderPasteMarginRatio = aFrom.m_SolderPasteMarginRatio;
    aTo.m_AllowSoldermaskBridgesInFPs = aFrom.m_AllowSoldermaskBridgesInFPs;
}
