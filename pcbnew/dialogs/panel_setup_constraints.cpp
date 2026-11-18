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

#include <algorithm>
#include <functional>
#include <map>

#include <board_design_settings.h>
#include <board.h>
#include <math/util.h>
#include <panel_setup_constraints.h>
#include <panel_setup_constraints_base.h>
#include <pcb_edit_frame.h>
#include <widgets/field_warning_icon.h>
#include <dialogs/constraint_rules_list.h>
#include <dialogs/panel_setup_constraint_rules.h>
#include <drc/rule_editor/drc_re_panel_matcher.h>
#include <drc/rule_editor/drc_rule_document.h>
#include <widgets/paged_dialog.h>
#include <wx/hyperlink.h>
#include <wx/popupwin.h>
#include <wx/scrolwin.h>
#include <wx/settings.h>
#include <wx/textctrl.h>
#include <wx/tooltip.h>
#include <wx/treebook.h>
#include <bitmaps.h>
#include <advanced_config.h>
#include <netclass.h>
#include <project.h>
#include <project/net_settings.h>
#include <project/project_file.h>


namespace
{

/// The height a popup grows to before its list starts scrolling, before DPI scaling
constexpr int MAX_POPUP_HEIGHT = 360;

/// The width a popup grows to before its lines are shortened to fit, before DPI scaling
constexpr int MAX_POPUP_WIDTH = 720;

/// The longest rule name shown in full. A hyperlink cannot shorten itself.
constexpr size_t MAX_NAME_LENGTH = 60;

/// The gap between a rule's name and the constraint it sets, before DPI scaling
constexpr int NAME_GAP = 8;


/**
 * The rules overriding one board constraint, each one a link that opens it in its editor.
 */
class OVERRIDING_RULES_POPUP : public wxPopupTransientWindow
{
public:
    explicit OVERRIDING_RULES_POPUP( wxWindow* aParent ) :
            wxPopupTransientWindow( aParent, wxBORDER_SIMPLE )
    {
        m_panel = new wxPanel( this, wxID_ANY );
        m_panel->SetBackgroundColour( wxSystemSettings::GetColour( wxSYS_COLOUR_WINDOW ) );

        m_scrolled = new wxScrolledWindow( m_panel, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxVSCROLL );
        m_scrolled->SetScrollRate( 0, 10 );
        m_scrolled->SetBackgroundColour( m_panel->GetBackgroundColour() );

        m_rows = new wxBoxSizer( wxVERTICAL );
        m_scrolled->SetSizer( m_rows );

        m_outer = new wxBoxSizer( wxVERTICAL );
        m_outer->Add( m_scrolled, 1, wxEXPAND | wxALL, FromDIP( 8 ) );
        m_panel->SetSizer( m_outer );
    }

    void AddHeading( const wxString& aText )
    {
        wxStaticText* heading = new wxStaticText( m_scrolled, wxID_ANY, aText );

        heading->SetFont( heading->GetFont().Bold() );
        m_naturalWidth = std::max( m_naturalWidth, heading->GetTextExtent( aText ).x );
        m_rows->Add( heading, 0, wxTOP, m_rows->GetItemCount() ? FromDIP( 10 ) : 0 );
    }

    /// A named row that opens where it is set, with what it sets written beside the name
    void AddEntry( const wxString& aName, const wxString& aDetail, std::function<void()> aOpen )
    {
        wxBoxSizer* row = new wxBoxSizer( wxHORIZONTAL );
        wxString    name = aName;

        if( name.length() > MAX_NAME_LENGTH )
            name = name.Left( MAX_NAME_LENGTH ) + wxS( "..." );

        wxHyperlinkCtrl* link = new wxHyperlinkCtrl( m_scrolled, wxID_ANY, name, wxEmptyString );

        if( name != aName )
            link->SetToolTip( aName );

        link->Bind( wxEVT_HYPERLINK,
                    [this, aOpen]( wxHyperlinkEvent& )
                    {
                        // Dismiss alone skips OnDismiss and would leave the popup alive
                        std::function<void()> open = aOpen;
                        DismissAndNotify();
                        open();
                    } );

        row->Add( link, 0, wxALIGN_CENTER_VERTICAL );
        row->Add( shortenable( aDetail, link->GetBestSize().x + FromDIP( NAME_GAP ) ), 1,
                  wxALIGN_CENTER_VERTICAL | wxLEFT, FromDIP( NAME_GAP ) );

        m_rows->Add( row, 0, wxEXPAND | wxTOP, FromDIP( 3 ) );
    }

    /// A link on a row of its own, under a rule from the line above it
    void AddLink( const wxString& aText, std::function<void()> aOpen )
    {
        wxHyperlinkCtrl* link = new wxHyperlinkCtrl( m_scrolled, wxID_ANY, aText, wxEmptyString );

        link->Bind( wxEVT_HYPERLINK,
                    [this, aOpen]( wxHyperlinkEvent& )
                    {
                        std::function<void()> open = aOpen;
                        DismissAndNotify();
                        open();
                    } );

        m_naturalWidth = std::max( m_naturalWidth, link->GetBestSize().x );
        m_rows->Add( link, 0, wxTOP, m_rows->GetItemCount() ? FromDIP( 10 ) : 0 );
    }

    /// Told to the page that opened this, so it stops tracking a popup that is going away
    void SetOnDismiss( std::function<void()> aFn ) { m_onDismiss = std::move( aFn ); }

    /// Closes the popup the way a click outside it would, which wx only exposes to itself
    void CloseNow() { DismissAndNotify(); }

    /// Destroyed on dismissal so repeated clicks do not leave hidden popups behind
    void OnDismiss() override
    {
        if( m_onDismiss )
            m_onDismiss();

        Destroy();
    }

    void PopupUnder( wxWindow* aAnchor )
    {
        int contentHeight = m_rows->GetMinSize().y;
        int width = std::min( m_naturalWidth, FromDIP( MAX_POPUP_WIDTH ) );
        int height = contentHeight;

        if( height > FromDIP( MAX_POPUP_HEIGHT ) )
        {
            height = FromDIP( MAX_POPUP_HEIGHT );
            width += wxSystemSettings::GetMetric( wxSYS_VSCROLL_X, m_scrolled );
        }

        m_scrolled->SetMinSize( wxSize( width, height ) );

        m_outer->Fit( m_panel );
        SetClientSize( m_panel->GetSize() );

        // Only the list scrolls, and only downwards, so the rows are laid out at the width they
        // were given and shorten themselves to it.
        m_scrolled->SetVirtualSize( wxSize( m_scrolled->GetClientSize().x, contentHeight ) );
        m_scrolled->Layout();

        Position( aAnchor->GetScreenPosition(), wxSize( 0, aAnchor->GetSize().y ) );
        Popup();
    }

private:
    /// A line that shortens itself rather than widening the popup, keeping its text on hover
    wxStaticText* shortenable( const wxString& aText, int aLeading = 0 )
    {
        wxStaticText* line =
                new wxStaticText( m_scrolled, wxID_ANY, aText, wxDefaultPosition, wxDefaultSize, wxST_ELLIPSIZE_END );

        // A line that may shorten reports almost no width of its own, so the width it would like
        // is remembered here and the popup is sized from that instead.
        m_naturalWidth = std::max( m_naturalWidth, aLeading + line->GetTextExtent( aText ).x );

        line->SetMinSize( wxSize( 1, -1 ) );
        line->SetToolTip( aText );

        return line;
    }

    wxPanel*          m_panel;
    wxScrolledWindow* m_scrolled;
    wxBoxSizer*       m_rows;
    wxBoxSizer*       m_outer;
    int               m_naturalWidth = 0;

    std::function<void()> m_onDismiss;
};


/**
 * Selects the tree page holding a constraint's rules and opens one of them in its editor.
 */
void openRuleInEditor( wxWindow* aContext, const wxString& aTitle, DRC_RULE_EDITOR_CONSTRAINT_NAME aType, int aIndex )
{
    PAGED_DIALOG* dialog = PAGED_DIALOG::GetDialog( aContext );

    if( !dialog )
        return;

    if( auto* page = dynamic_cast<PANEL_SETUP_CONSTRAINT_RULES*>( dialog->ShowPage( aTitle ) ) )
        page->ShowRule( aType, aIndex );
}

} // namespace


PANEL_SETUP_CONSTRAINTS::PANEL_SETUP_CONSTRAINTS( wxWindow* aParentWindow, PCB_EDIT_FRAME* aFrame,
                                                  DRC_RULE_DOCUMENT* aDocument, BOARD_DESIGN_SETTINGS& aSettings ) :
        PANEL_SETUP_CONSTRAINTS_BASE( aParentWindow ),
        m_minClearance( aFrame, m_clearanceTitle, m_clearanceCtrl, m_clearanceUnits ),
        m_minConn( aFrame, m_MinConnTitle, m_MinConnCtrl, m_MinConnUnits ),
        m_trackMinWidth( aFrame, m_TrackMinWidthTitle, m_TrackMinWidthCtrl, m_TrackMinWidthUnits ),
        m_viaMinAnnulus( aFrame, m_ViaMinAnnulusTitle, m_ViaMinAnnulusCtrl, m_ViaMinAnnulusUnits ),
        m_viaMinSize( aFrame, m_ViaMinTitle, m_SetViasMinSizeCtrl, m_ViaMinUnits ),
        m_throughHoleMin( aFrame, m_MinDrillTitle, m_MinDrillCtrl, m_MinDrillUnits ),
        m_uviaMinSize( aFrame, m_uviaMinSizeLabel, m_uviaMinSizeCtrl, m_uviaMinSizeUnits ),
        m_uviaMinDrill( aFrame, m_uviaMinDrillLabel, m_uviaMinDrillCtrl, m_uviaMinDrillUnits ),
        m_holeToHoleMin( aFrame, m_HoleToHoleTitle, m_SetHoleToHoleCtrl, m_HoleToHoleUnits ),
        m_holeClearance( aFrame, m_HoleClearanceLabel, m_HoleClearanceCtrl, m_HoleClearanceUnits ),
        m_edgeClearance( aFrame, m_EdgeClearanceLabel, m_EdgeClearanceCtrl, m_EdgeClearanceUnits ),
        m_silkClearance( aFrame, m_silkClearanceLabel, m_silkClearanceCtrl, m_silkClearanceUnits ),
        m_minGrooveWidth( aFrame, m_minGrooveWidthLabel, m_minGrooveWidthCtrl, m_minGrooveWidthUnits ),
        m_minTextHeight( aFrame, m_textHeightLabel, m_textHeightCtrl, m_textHeightUnits ),
        m_minLineWidth( aFrame, m_lineWidthLabel, m_lineWidthCtrl, m_lineWidthUnits ),
        m_maxError( aFrame, m_maxErrorTitle, m_maxErrorCtrl, m_maxErrorUnits ),
        m_document( aDocument ),
        m_validationTimer( this ),
        m_hoverTimer( this ),
        m_closeTimer( this )
{
    m_Frame = aFrame;
    m_BrdSettings = &aSettings;

    m_filletBitmap->SetBitmap( KiBitmapBundle( BITMAPS::zone_fillet, 24 ) );
    m_spokeBitmap->SetBitmap( KiBitmapBundle( BITMAPS::thermal_spokes, 24 ) );
    m_bitmapClearance->SetBitmap( KiBitmapBundle( BITMAPS::ps_diff_pair_gap, 24 ) );
    m_bitmapMinTrackWidth->SetBitmap( KiBitmapBundle( BITMAPS::width_track, 24 ) );
    m_bitmapMinConn->SetBitmap( KiBitmapBundle( BITMAPS::width_conn, 24 ) );
    m_bitmapMinViaAnnulus->SetBitmap( KiBitmapBundle( BITMAPS::via_annulus, 24 ) );
    m_bitmapMinViaDiameter->SetBitmap( KiBitmapBundle( BITMAPS::via_diameter, 24 ) );
    m_bitmapMinViaDrill->SetBitmap( KiBitmapBundle( BITMAPS::via_hole_diameter, 24 ) );
    m_bitmapMinuViaDiameter->SetBitmap( KiBitmapBundle( BITMAPS::via_diameter, 24 ) );
    m_bitmapMinuViaDrill->SetBitmap( KiBitmapBundle( BITMAPS::via_hole_diameter, 24 ) );
    m_bitmapHoleClearance->SetBitmap( KiBitmapBundle( BITMAPS::hole_to_copper_clearance, 24 ) );
    m_bitmapMinHoleClearance->SetBitmap( KiBitmapBundle( BITMAPS::hole_to_hole_clearance, 24 ) );
    m_bitmapEdgeClearance->SetBitmap( KiBitmapBundle( BITMAPS::edge_to_copper_clearance, 24 ) );

    m_stCircleToPolyWarning->SetFont( KIUI::GetSmallInfoFont( this ).Italic() );

    // The spoke count is a spin control, so it is not one of the bound fields below
    m_spokeCountPage = PANEL_SETUP_CONSTRAINT_RULES::PageEditingDefault( wxS( "min_resolved_spokes" ) );

    wxSize ctrlSize = m_minResolvedSpokeCountCtrl->GetSize();
    ctrlSize.x = KIUI::GetTextSize( wxT( "XXX" ), m_minResolvedSpokeCountCtrl ).x;

    // The link sits inside the field, and this one is otherwise only as wide as its count
    if( !m_spokeCountPage.IsEmpty() )
        ctrlSize.x = m_minResolvedSpokeCountCtrl->GetBestSize().x + FromDIP( 24 );

    m_minResolvedSpokeCountCtrl->SetMinSize( ctrlSize );
    m_minResolvedSpokeCountCtrl->SetSize( ctrlSize );

    if( !ADVANCED_CFG::GetCfg().m_EnableCreepageSlot )
    {
        m_bitmapMinGrooveWidth->Show( false );
        m_minGrooveWidthLabel->Show( false );
        m_minGrooveWidthCtrl->Show( false );
        m_minGrooveWidthUnits->Show( false );
    }

    using BDS = BOARD_DESIGN_SETTINGS;

    m_fields = {
        { wxS( "min_clearance" ), m_clearanceCtrl, &m_minClearance, &BDS::m_MinClearance },
        { wxS( "min_connection" ), m_MinConnCtrl, &m_minConn, &BDS::m_MinConn },
        { wxS( "min_track_width" ), m_TrackMinWidthCtrl, &m_trackMinWidth, &BDS::m_TrackMinWidth },
        { wxS( "min_via_annular_width" ), m_ViaMinAnnulusCtrl, &m_viaMinAnnulus, &BDS::m_ViasMinAnnularWidth },
        { wxS( "min_via_diameter" ), m_SetViasMinSizeCtrl, &m_viaMinSize, &BDS::m_ViasMinSize },
        { wxS( "min_through_hole_diameter" ), m_MinDrillCtrl, &m_throughHoleMin, &BDS::m_MinThroughDrill },
        { wxS( "min_microvia_diameter" ), m_uviaMinSizeCtrl, &m_uviaMinSize, &BDS::m_MicroViasMinSize },
        { wxS( "min_microvia_drill" ), m_uviaMinDrillCtrl, &m_uviaMinDrill, &BDS::m_MicroViasMinDrill },
        { wxS( "min_hole_to_hole" ), m_SetHoleToHoleCtrl, &m_holeToHoleMin, &BDS::m_HoleToHoleMin },
        { wxS( "min_hole_clearance" ), m_HoleClearanceCtrl, &m_holeClearance, &BDS::m_HoleClearance },
        { wxS( "min_silk_clearance" ), m_silkClearanceCtrl, &m_silkClearance, &BDS::m_SilkClearance },
        { wxS( "min_groove_width" ), m_minGrooveWidthCtrl, &m_minGrooveWidth, &BDS::m_MinGrooveWidth },
        { wxS( "min_text_height" ), m_textHeightCtrl, &m_minTextHeight, &BDS::m_MinSilkTextHeight },
        { wxS( "min_silk_line_width" ), m_lineWidthCtrl, &m_minLineWidth, &BDS::m_MinSilkLineWidth },
        { wxS( "min_copper_edge_clearance" ), m_EdgeClearanceCtrl, &m_edgeClearance, &BDS::m_CopperEdgeClearance },
        { wxS( "max_error" ), m_maxErrorCtrl, &m_maxError, &BDS::m_MaxError }
    };

    for( CONSTRAINT_FIELD& field : m_fields )
    {
        // Debounced so a half-typed value does not flash a warning
        field.m_Ctrl->Bind( wxEVT_TEXT,
                            [this]( wxCommandEvent& aEvent )
                            {
                                m_validationTimer.StartOnce( 250 );
                                aEvent.Skip();
                            } );

        // The value stays editable here, so one icon gathers everywhere else it is read from:
        // the rules written against it, the net classes setting their own, and the rule page
        bool readElsewhere = !PANEL_SETUP_CONSTRAINT_RULES::PageEditingDefault( field.m_Setting ).IsEmpty()
                             || PANEL_SETUP_CONSTRAINT_RULES::RulesCanOverride( field.m_Setting, nullptr );

        // The icon that is always there holds the field's edge, so it sits clear of the value, and
        // a warning, which comes and goes, takes the slot beside it
        field.m_Warning = new FIELD_WARNING_ICON( field.m_Ctrl, BITMAPS::small_warning, readElsewhere ? 1 : 0 );

        if( !readElsewhere )
            continue;

        field.m_Override = new FIELD_WARNING_ICON( field.m_Ctrl, BITMAPS::small_new_window, 0 );
        field.m_Override->ShowAsBadge();
        field.m_Override->SetCursor( wxCURSOR_HAND );

        field.m_Override->Bind( wxEVT_LEFT_DOWN,
                                [this, setting = field.m_Setting, icon = field.m_Override]( wxMouseEvent& )
                                {
                                    m_hoverTimer.Stop();
                                    showOverridingRules( setting, icon, false );
                                } );

        // Opening on hover as well makes the icon read as a hint rather than a button
        field.m_Override->Bind( wxEVT_ENTER_WINDOW,
                                [this, setting = field.m_Setting, icon = field.m_Override]( wxMouseEvent& aEvent )
                                {
                                    aEvent.Skip();

                                    if( m_popup && m_popupAnchor == icon )
                                        return;

                                    // The field's own help would otherwise appear over the popup
                                    wxToolTip::Enable( false );

                                    m_hoverSetting = setting;
                                    m_hoverAnchor = icon;
                                    m_hoverTimer.StartOnce( 400 );
                                } );

        field.m_Override->Bind( wxEVT_LEAVE_WINDOW,
                                [this]( wxMouseEvent& aEvent )
                                {
                                    aEvent.Skip();
                                    m_hoverTimer.Stop();

                                    if( !m_popup )
                                        wxToolTip::Enable( true );
                                } );
    }

    if( !m_spokeCountPage.IsEmpty() )
        PANEL_SETUP_CONSTRAINT_RULES::LinkToPage( m_minResolvedSpokeCountCtrl, m_spokeCountPage, 0 );

    Bind( wxEVT_TIMER,
          [this]( wxTimerEvent& )
          {
              updateWarnings();
          },
          m_validationTimer.GetId() );

    Bind(
            wxEVT_TIMER,
            [this]( wxTimerEvent& )
            {
                if( m_hoverAnchor )
                    showOverridingRules( m_hoverSetting, m_hoverAnchor, true );
            },
            m_hoverTimer.GetId() );

    Bind(
            wxEVT_TIMER,
            [this]( wxTimerEvent& )
            {
                closeHoveredPopup();
            },
            m_closeTimer.GetId() );
}


void PANEL_SETUP_CONSTRAINTS::closeHoveredPopup()
{
    if( !m_popup || !m_popupFromHover )
        return;

    // Entering a link inside the popup leaves the popup itself, so ask what is under the pointer
    // rather than tracking enter and leave across its children
    wxWindow* under = wxFindWindowAtPoint( wxGetMousePosition() );

    for( wxWindow* w = under; w; w = w->GetParent() )
    {
        if( w == m_popup || w == m_popupAnchor )
            return;
    }

    static_cast<OVERRIDING_RULES_POPUP*>( m_popup )->CloseNow();
}


bool PANEL_SETUP_CONSTRAINTS::TransferDataToWindow()
{
    wxString msg;
    msg.Printf( m_stCircleToPolyWarning->GetLabel(), m_Frame->StringFromValue( ARC_HIGH_DEF, true ) );
    m_stCircleToPolyWarning->SetLabel( msg );

    m_useHeightForLengthCalcs->SetValue( m_BrdSettings->m_UseHeightForLengthCalcs );
    m_allowExternalFilletsOpt->SetValue( m_BrdSettings->m_ZoneKeepExternalFillets );

    // Every bound field is read there, which also records what was read
    RefreshDefaults();

    return true;
}


bool PANEL_SETUP_CONSTRAINTS::TransferDataFromWindow()
{
    // These are all stored in project file, not board, so no need for OnModify()

    wxString message;

    if( wxWindow* field = firstInvalidField( &message ) )
    {
        PAGED_DIALOG::GetDialog( this )->SetError( message, this, field );
        return false;
    }

    m_BrdSettings->m_UseHeightForLengthCalcs = m_useHeightForLengthCalcs->GetValue();
    m_BrdSettings->m_ZoneKeepExternalFillets = m_allowExternalFilletsOpt->GetValue();

    writeEditedDefaults();
    return true;
}


bool PANEL_SETUP_CONSTRAINTS::StageEditedDefaults()
{
    return !firstInvalidField( nullptr ) && writeEditedDefaults();
}


std::vector<std::pair<wxString, wxString>> PANEL_SETUP_CONSTRAINTS::StaleSearchValues() const
{
    std::vector<std::pair<wxString, wxString>> stale;

    for( const CONSTRAINT_FIELD& field : m_fields )
    {
        const int staged = m_BrdSettings->*( field.m_Member );
        auto*     entry = dynamic_cast<wxTextEntry*>( field.m_Ctrl );

        if( entry && staged != field.m_Loaded && field.m_Binder->GetIntValue() == field.m_Loaded )
            stale.emplace_back( entry->GetValue(), m_Frame->StringFromValue( staged, true ) );
    }

    const int stagedSpokes = m_BrdSettings->m_MinResolvedSpokes;

    if( stagedSpokes != m_spokeCountLoaded && m_minResolvedSpokeCountCtrl->GetValue() == m_spokeCountLoaded )
    {
        stale.emplace_back( m_minResolvedSpokeCountCtrl->GetTextValue(),
                            wxString::Format( wxS( "%d" ), stagedSpokes ) );
    }

    return stale;
}


wxWindow* PANEL_SETUP_CONSTRAINTS::firstInvalidField( wxString* aMessage ) const
{
    BOARD_DESIGN_SETTINGS temp( *m_BrdSettings );

    applyValues( temp );

    std::map<wxString, wxWindow*> fieldToControl = { { wxS( "min_resolved_spokes" ), m_minResolvedSpokeCountCtrl } };

    for( const CONSTRAINT_FIELD& field : m_fields )
        fieldToControl[field.m_Setting] = field.m_Ctrl;

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


bool PANEL_SETUP_CONSTRAINTS::writeEditedDefaults()
{
    bool written = false;

    // A constraint can also be set with the rules read against it, so a field the user did not
    // touch here must not write back over an edit made on that page
    if( m_minResolvedSpokeCountCtrl->GetValue() != m_spokeCountLoaded )
    {
        m_spokeCountLoaded = m_minResolvedSpokeCountCtrl->GetValue();
        m_BrdSettings->m_MinResolvedSpokes = m_spokeCountLoaded;
        written = true;
    }

    for( CONSTRAINT_FIELD& field : m_fields )
    {
        int value = field.m_Binder->GetIntValue();

        if( value != field.m_Loaded )
        {
            field.m_Loaded = value;
            m_BrdSettings->*( field.m_Member ) = value;
            written = true;
        }
    }

    return written;
}


void PANEL_SETUP_CONSTRAINTS::applyValues( BOARD_DESIGN_SETTINGS& aSettings ) const
{
    aSettings.m_UseHeightForLengthCalcs = m_useHeightForLengthCalcs->GetValue();
    aSettings.m_ZoneKeepExternalFillets = m_allowExternalFilletsOpt->GetValue();

    aSettings.m_MinResolvedSpokes = m_minResolvedSpokeCountCtrl->GetValue();

    for( const CONSTRAINT_FIELD& field : m_fields )
        aSettings.*( field.m_Member ) = field.m_Binder->GetIntValue();
}


void PANEL_SETUP_CONSTRAINTS::RefreshDefaults()
{
    for( CONSTRAINT_FIELD& field : m_fields )
    {
        field.m_Loaded = m_BrdSettings->*( field.m_Member );
        field.m_Binder->SetValue( field.m_Loaded );
    }

    m_spokeCountLoaded = m_BrdSettings->m_MinResolvedSpokes;
    m_minResolvedSpokeCountCtrl->SetValue( m_spokeCountLoaded );

    updateWarnings();
}


void PANEL_SETUP_CONSTRAINTS::showOverridingRules( const wxString& aSetting, wxWindow* aAnchor, bool aFromHover )
{
    if( m_popup )
        static_cast<OVERRIDING_RULES_POPUP*>( m_popup )->CloseNow();

    DRC_CONSTRAINT_T constraint = NULL_CONSTRAINT;
    std::vector<int> indices;

    if( PANEL_SETUP_CONSTRAINT_RULES::RulesCanOverride( aSetting, &constraint ) )
        indices = m_document->IndicesSetting( constraint );

    const NET_SETTINGS&                        netSettings = *m_Frame->Prj().GetProjectFile().NetSettings();
    const std::map<int, std::vector<wxString>> netclasses =
            PANEL_SETUP_CONSTRAINT_RULES::NetclassValuesFor( aSetting, netSettings );
    const wxString defaultsPage = PANEL_SETUP_CONSTRAINT_RULES::PageEditingDefault( aSetting );

    if( indices.empty() && netclasses.empty() && defaultsPage.IsEmpty() )
        return;

    OVERRIDING_RULES_POPUP* popup = new OVERRIDING_RULES_POPUP( this );

    if( !indices.empty() && constraint == CLEARANCE_CONSTRAINT )
        popup->AddHeading( _( "User clearance rules" ) );
    else if( !indices.empty() )
        popup->AddHeading( _( "Overridden by user rules" ) );

    static DRC_PANEL_MATCHER matcher;

    for( int index : indices )
    {
        const DRC_RE_LOADED_PANEL_ENTRY& entry = m_document->Entries()[index];

        // A rule kept as text is listed on the page of the constraint it sets
        DRC_RULE_EDITOR_CONSTRAINT_NAME type =
                entry.panelType == CUSTOM_RULE ? matcher.GetHomePanel( constraint ) : entry.panelType;
        wxString page = PANEL_SETUP_CONSTRAINT_RULES::PageTitleFor( type );
        wxString value = CONSTRAINT_RULES_LIST::ConstraintText( entry, type, m_Frame );
        wxString detail;

        if( entry.condition.IsEmpty() && entry.layerSource.IsEmpty() )
            detail = wxString::Format( _( "%s, everywhere" ), value );
        else if( entry.condition.IsEmpty() )
            detail = wxString::Format( _( "%s, on %s" ), value, entry.layerSource );
        else if( entry.layerSource.IsEmpty() )
            detail = wxString::Format( _( "%s, where %s" ), value, entry.condition );
        else
            detail = wxString::Format( _( "%s, on %s where %s" ), value, entry.layerSource, entry.condition );

        popup->AddEntry( entry.ruleName, detail,
                         [this, page, type, index]()
                         {
                             openRuleInEditor( this, page, type, index );
                         } );
    }

    // A net class sets its own value instead of displacing the board one, which stays the
    // minimum, so these are listed apart from the rules. They are all set in one place, so each
    // row opens that page rather than a class of its own.
    if( !netclasses.empty() )
    {
        popup->AddHeading( _( "Net class values" ) );

        for( const auto& [value, names] : netclasses )
        {
            wxString classes;

            for( const wxString& name : names )
            {
                if( !classes.IsEmpty() )
                    classes << wxS( ", " );

                classes << name;
            }

            popup->AddEntry( classes, m_Frame->StringFromValue( value, true ),
                             [this]()
                             {
                                 if( PAGED_DIALOG* dialog = PAGED_DIALOG::GetDialog( this ) )
                                     dialog->ShowPage( _( "Net Classes" ) );
                             } );
        }
    }

    // The same value is set on that page too, so the popup is also the way to reach it
    if( !defaultsPage.IsEmpty() )
    {
        popup->AddLink( wxString::Format( _( "Edit on the %s page" ), defaultsPage ),
                        [this, defaultsPage]()
                        {
                            if( PAGED_DIALOG* dialog = PAGED_DIALOG::GetDialog( this ) )
                                dialog->ShowPage( defaultsPage );
                        } );
    }

    // Neither the icon nor the field may raise its own help over the popup. The icon's is blank
    // and only there to stop the field's showing through it, so it is simply put back.
    aAnchor->UnsetToolTip();

    for( const CONSTRAINT_FIELD& field : m_fields )
    {
        if( field.m_Setting == aSetting && field.m_Ctrl->GetToolTip() )
        {
            m_mutedField = field.m_Ctrl;
            m_mutedTip = field.m_Ctrl->GetToolTipText();
            field.m_Ctrl->UnsetToolTip();
            break;
        }
    }

    popup->SetOnDismiss(
            [this, aAnchor]()
            {
                aAnchor->SetToolTip( wxS( " " ) );

                if( m_mutedField )
                    m_mutedField->SetToolTip( m_mutedTip );

                m_mutedField = nullptr;
                wxToolTip::Enable( true );
                m_popup = nullptr;
                m_popupAnchor = nullptr;
                m_closeTimer.Stop();
            } );

    m_popup = popup;
    m_popupAnchor = aAnchor;
    m_popupFromHover = aFromHover;

    popup->PopupUnder( aAnchor );

    // A popup a click opened stays until it is dismissed; one a hover opened follows the pointer
    if( aFromHover )
        m_closeTimer.Start( 250 );
}


void PANEL_SETUP_CONSTRAINTS::updateWarnings()
{
    m_validationTimer.Stop();

    BOARD_DESIGN_SETTINGS candidate( *m_BrdSettings );
    applyValues( candidate );

    EDA_UNITS                    units = m_Frame->GetUserUnits();
    std::map<wxString, wxString> messages;

    for( const std::vector<BOARD_DESIGN_SETTINGS::VALIDATION_ERROR>& issues :
         { candidate.ValidateDesignRules( units ), candidate.GetDesignRuleWarnings( units ) } )
    {
        for( const BOARD_DESIGN_SETTINGS::VALIDATION_ERROR& issue : issues )
        {
            wxString& msg = messages[issue.setting_name];

            if( !msg.IsEmpty() )
                msg += wxS( "\n" );

            msg += issue.error_message;
        }
    }

    for( const CONSTRAINT_FIELD& field : m_fields )
    {
        auto it = messages.find( field.m_Setting );
        bool show = it != messages.end() && field.m_Ctrl->IsShown();

        field.m_Warning->SetWarning( show ? it->second : wxString() );
    }
}


void PANEL_SETUP_CONSTRAINTS::CopySettings( const BOARD_DESIGN_SETTINGS& aFrom, BOARD_DESIGN_SETTINGS& aTo )
{
    using BDS = BOARD_DESIGN_SETTINGS;

    // Must cover every m_fields member or an import or OK drops it
    for( int BDS::* member :
         { &BDS::m_MinClearance, &BDS::m_MinConn, &BDS::m_TrackMinWidth, &BDS::m_ViasMinAnnularWidth,
           &BDS::m_ViasMinSize, &BDS::m_MinThroughDrill, &BDS::m_MicroViasMinSize, &BDS::m_MicroViasMinDrill,
           &BDS::m_HoleToHoleMin, &BDS::m_HoleClearance, &BDS::m_SilkClearance, &BDS::m_MinGrooveWidth,
           &BDS::m_MinSilkTextHeight, &BDS::m_MinSilkLineWidth, &BDS::m_CopperEdgeClearance, &BDS::m_MaxError,
           &BDS::m_MinResolvedSpokes } )
    {
        aTo.*member = aFrom.*member;
    }

    aTo.m_UseHeightForLengthCalcs = aFrom.m_UseHeightForLengthCalcs;
    aTo.m_ZoneKeepExternalFillets = aFrom.m_ZoneKeepExternalFillets;
}
