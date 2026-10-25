/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "panel_setup_constraint_rules.h"

#include <map>

#include <board.h>
#include <board_design_settings.h>
#include <netclass.h>
#include <pcb_edit_frame.h>
#include <project.h>
#include <project/net_settings.h>
#include <project/project_file.h>
#include <widgets/paged_dialog.h>
#include <widgets/wx_treebook.h>
#include <wx/hyperlink.h>
#include <widgets/ui_common.h>
#include <wx/scrolwin.h>
#include <wx/sizer.h>
#include <wx/statline.h>
#include <wx/stattext.h>

#include "constraint_rules_list.h"


PANEL_SETUP_CONSTRAINT_RULES::PANEL_SETUP_CONSTRAINT_RULES( wxWindow* aParent, PCB_EDIT_FRAME* aFrame,
                                                            DRC_RULE_DOCUMENT* aDocument, FAMILY aFamily,
                                                            size_t aSection ) :
        wxPanel( aParent, wxID_ANY ),
        m_frame( aFrame ),
        m_settings( &aFrame->GetBoard()->GetDesignSettings() ),
        m_document( aDocument )
{
    std::vector<CONSTRAINT_SECTION> sections = SectionsFor( aFamily );

    if( aSection < sections.size() )
        m_sections.push_back( std::move( sections[aSection] ) );

    wxBoxSizer* mainSizer = new wxBoxSizer( wxVERTICAL );

    m_scrolled = new wxScrolledWindow( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxVSCROLL );
    m_scrolled->SetScrollRate( 0, 10 );
    m_scrolled->SetSizer( new wxBoxSizer( wxVERTICAL ) );

    m_note = nullptr;

    if( !m_sections.empty() && !m_sections[0].defaults.empty() )
    {
        m_noteText = _( "The value is resolved in this order: the board default, then the matching net class, "
                        "then the rules from top to bottom. The last match wins." );
        m_note = new wxStaticText( m_scrolled, wxID_ANY, m_noteText );
        m_note->SetFont( KIUI::GetInfoFont( this ).Italic() );
        m_scrolled->GetSizer()->Add( m_note, 0, wxALL, 8 );
    }

    m_lists.resize( m_sections.size(), nullptr );

    for( size_t i = 0; i < m_sections.size(); ++i )
        buildSection( i );

    mainSizer->Add( m_scrolled, 1, wxEXPAND );
    SetSizer( mainSizer );

    m_scrolled->Bind( wxEVT_SIZE,
                      [this]( wxSizeEvent& aEvent )
                      {
                          wrapLabels();
                          aEvent.Skip();
                      } );
}


void PANEL_SETUP_CONSTRAINT_RULES::buildSection( size_t aIndex )
{
    const CONSTRAINT_SECTION& section = m_sections[aIndex];

    wxSizer*      box = m_scrolled->GetSizer();
    wxWindow*     parent = m_scrolled;
    wxStaticText* heading = new wxStaticText( parent, wxID_ANY, section.title );
    wxBoxSizer*   settings = new wxBoxSizer( wxVERTICAL );

    heading->SetFont( heading->GetFont().Bold() );
    box->Add( heading, 0, wxLEFT | wxTOP | wxBOTTOM, 8 );
    box->Add( settings, 0, wxEXPAND | wxLEFT, 8 );

    if( !section.defaults.empty() )
    {
        wxFlexGridSizer* grid = new wxFlexGridSizer( 0, 3, 4, 8 );

        for( const CONSTRAINT_DEFAULT_FIELD& field : section.defaults )
        {
            wxStaticText* label = new wxStaticText( parent, wxID_ANY, field.label );
            wxStaticText* value = new wxStaticText( parent, wxID_ANY, wxEmptyString );

            grid->Add( label, 0, wxALIGN_CENTER_VERTICAL );
            grid->Add( value, 0, wxALIGN_CENTER_VERTICAL );

            m_fields.push_back( field );
            m_defaultLabels.push_back( value );
        }

        // The defaults are owned by the Constraints page. They are shown here because a rule is
        // read against them, but they are edited in one place only.
        wxBoxSizer*      row = new wxBoxSizer( wxHORIZONTAL );
        wxHyperlinkCtrl* link = new wxHyperlinkCtrl( parent, wxID_ANY, _( "Edit constraints" ), wxEmptyString );

        link->SetFont( KIUI::GetInfoFont( this ) );
        wxString owner = section.settingsPage.IsEmpty() ? _( "Constraints" ) : section.settingsPage;

        link->Bind( wxEVT_HYPERLINK,
                    [this, owner]( wxHyperlinkEvent& )
                    {
                        showPage( owner );
                    } );

        row->Add( grid, 0, wxALIGN_CENTER_VERTICAL );
        row->Add( link, 0, wxALIGN_CENTER_VERTICAL | wxLEFT, 16 );
        settings->Add( row, 0, wxALL, 5 );
    }

    if( section.netclassValue )
    {
        wxBoxSizer*      row = new wxBoxSizer( wxHORIZONTAL );
        wxStaticText*    label = new wxStaticText( parent, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize,
                                                   wxST_ELLIPSIZE_END );
        wxHyperlinkCtrl* link = new wxHyperlinkCtrl( parent, wxID_ANY, _( "Edit net classes" ), wxEmptyString );

        label->SetFont( KIUI::GetInfoFont( this ) );
        link->SetFont( KIUI::GetInfoFont( this ) );
        link->Bind( wxEVT_HYPERLINK,
                    [this]( wxHyperlinkEvent& )
                    {
                        showPage( _( "Net Classes" ) );
                    } );

        label->SetMinSize( wxSize( 1, -1 ) );
        row->Add( label, 1, wxALIGN_CENTER_VERTICAL );
        row->Add( link, 0, wxALIGN_CENTER_VERTICAL | wxLEFT | wxRIGHT, 12 );
        settings->Add( row, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 5 );

        m_netclassLabels.emplace_back( aIndex, label );
        m_netclassTexts.emplace_back();
    }

    if( section.ruleType )
    {
        if( !section.defaults.empty() )
            box->Add( new wxStaticLine( parent ), 0, wxEXPAND | wxLEFT | wxRIGHT, 5 );

        CONSTRAINT_RULES_LIST* list =
                new CONSTRAINT_RULES_LIST( parent, m_frame, m_document, *section.ruleType, section.title );
        list->SetTitleWindow( heading );
        list->SetChangeCallback(
                [this]()
                {
                    RefreshFromDocument();
                } );
        list->RefreshRules();
        box->Add( list, 1, wxEXPAND | wxALL, 8 );
        m_lists[aIndex] = list;
    }
}


void PANEL_SETUP_CONSTRAINT_RULES::updateNetclassLabels()
{
    std::shared_ptr<NET_SETTINGS>& netSettings = m_frame->Prj().GetProjectFile().NetSettings();

    for( size_t i = 0; i < m_netclassLabels.size(); ++i )
    {
        const CONSTRAINT_SECTION& section = m_sections[m_netclassLabels[i].first];

        // Classes sharing a value are listed together, so the common case reads as one entry.
        std::map<int, std::vector<wxString>> byValue;

        auto collect = [&]( const std::shared_ptr<NETCLASS>& aNetclass )
        {
            if( !aNetclass )
                return;

            if( std::optional<int> value = section.netclassValue( *aNetclass ) )
                byValue[*value].push_back( aNetclass->GetName() );
        };

        collect( netSettings->GetDefaultNetclass() );

        for( const auto& [name, netclass] : netSettings->GetNetclasses() )
            collect( netclass );

        wxString text;
        wxString fullText;

        for( const auto& [value, names] : byValue )
        {
            if( !text.IsEmpty() )
            {
                text << wxS( "    " );
                fullText << wxS( "\n" );
            }

            text << m_frame->StringFromValue( value, true ) << wxS( ": " );
            fullText << m_frame->StringFromValue( value, true ) << wxS( ": " );

            const size_t shown = 5;

            for( size_t n = 0; n < names.size(); ++n )
            {
                fullText << ( n ? wxS( ", " ) : wxS( "" ) ) << names[n];

                if( n < shown )
                    text << ( n ? wxS( ", " ) : wxS( "" ) ) << names[n];
            }

            if( names.size() > shown )
                text << wxString::Format( _( " and %zu more" ), names.size() - shown );
        }

        if( text.IsEmpty() )
        {
            m_netclassTexts[i] = _( "Net classes: no class sets this value" );
            m_netclassLabels[i].second->SetToolTip( wxEmptyString );
        }
        else
        {
            m_netclassTexts[i] = _( "Net classes:  " ) + text;
            m_netclassLabels[i].second->SetToolTip( fullText );
        }
    }

    wrapLabels();
}


void PANEL_SETUP_CONSTRAINT_RULES::showPage( const wxString& aTitle )
{
    PAGED_DIALOG* dialog = PAGED_DIALOG::GetDialog( this );

    if( !dialog )
        return;

    WX_TREEBOOK* book = dialog->GetTreebook();

    for( size_t i = 0; i < book->GetPageCount(); ++i )
    {
        if( book->GetPageText( i ) == aTitle )
        {
            book->SetSelection( i );
            return;
        }
    }
}


void PANEL_SETUP_CONSTRAINT_RULES::wrapLabels()
{
    int width = m_scrolled->GetClientSize().x - 180;

    if( width < 200 )
        return;

    if( m_note )
    {
        m_note->SetLabel( m_noteText );
        m_note->Wrap( GetClientSize().x - 32 );
    }

    for( size_t i = 0; i < m_netclassLabels.size(); ++i )
    {
        m_netclassLabels[i].second->SetLabel( m_netclassTexts[i] );
        m_netclassLabels[i].second->Wrap( width );
    }

    m_scrolled->Layout();
    m_scrolled->FitInside();
    m_scrolled->Layout();
}


void PANEL_SETUP_CONSTRAINT_RULES::updateDefaultLabels()
{
    for( size_t i = 0; i < m_defaultLabels.size(); ++i )
    {
        int value = m_settings->*( m_fields[i].member );

        if( i < m_importedDefaults.size() && m_importedDefaults[i] )
            value = *m_importedDefaults[i];

        m_defaultLabels[i]->SetLabel( m_frame->StringFromValue( value, true ) );
    }
}


bool PANEL_SETUP_CONSTRAINT_RULES::TransferDataToWindow()
{
    updateDefaultLabels();
    RefreshFromDocument();
    return true;
}


void PANEL_SETUP_CONSTRAINT_RULES::RefreshFromDocument()
{
    updateDefaultLabels();
    updateNetclassLabels();

    for( CONSTRAINT_RULES_LIST* list : m_lists )
    {
        if( list )
            list->RefreshRules();
    }
}


void PANEL_SETUP_CONSTRAINT_RULES::ImportSettingsFrom( BOARD* aBoard )
{
    BOARD_DESIGN_SETTINGS& other = aBoard->GetDesignSettings();

    m_importedDefaults.resize( m_fields.size() );

    for( size_t i = 0; i < m_fields.size(); ++i )
        m_importedDefaults[i] = other.*( m_fields[i].member );

    updateDefaultLabels();
}


bool PANEL_SETUP_CONSTRAINT_RULES::RulesCanOverride( const wxString& aSetting, DRC_RULE_EDITOR_CONSTRAINT_NAME* aType,
                                                     wxString* aPage )
{
    for( FAMILY family : { FAMILY::CLEARANCE, FAMILY::TRACKS_AND_VIAS, FAMILY::HOLES, FAMILY::SILKSCREEN,
                           FAMILY::SOLDER_MASK, FAMILY::HIGH_SPEED, FAMILY::FOOTPRINTS } )
    {
        for( const CONSTRAINT_SECTION& section : SectionsFor( family ) )
        {
            if( !section.ruleType )
                continue;

            for( const CONSTRAINT_DEFAULT_FIELD& field : section.defaults )
            {
                if( field.settingName != aSetting )
                    continue;

                if( aType )
                    *aType = *section.ruleType;

                if( aPage )
                    *aPage = section.title;

                return true;
            }
        }
    }

    return false;
}


std::vector<CONSTRAINT_SECTION> PANEL_SETUP_CONSTRAINT_RULES::SectionsFor( FAMILY aFamily )
{
    using BDS = BOARD_DESIGN_SETTINGS;

    auto clearance = []( const NETCLASS& aNetclass )
    {
        return aNetclass.GetClearanceOpt();
    };
    auto trackWidth = []( const NETCLASS& aNetclass )
    {
        return aNetclass.GetTrackWidthOpt();
    };
    auto viaDiameter = []( const NETCLASS& aNetclass )
    {
        return aNetclass.GetViaDiameterOpt();
    };
    auto viaDrill = []( const NETCLASS& aNetclass )
    {
        return aNetclass.GetViaDrillOpt();
    };
    auto uViaDiameter = []( const NETCLASS& aNetclass )
    {
        return aNetclass.GetuViaDiameterOpt();
    };

    switch( aFamily )
    {
    case FAMILY::CLEARANCE:
        return { { _( "Minimum clearance" ),
                   { { _( "Board default:" ), &BDS::m_MinClearance, wxS( "min_clearance" ) } },
                   MINIMUM_CLEARANCE,
                   clearance },
                 { _( "Copper to hole clearance" ),
                   { { _( "Board default:" ), &BDS::m_HoleClearance, wxS( "min_hole_clearance" ) } },
                   COPPER_TO_HOLE_CLEARANCE,
                   {} },
                 { _( "Copper to edge clearance" ),
                   { { _( "Board default:" ), &BDS::m_CopperEdgeClearance, wxS( "min_copper_edge_clearance" ) } },
                   COPPER_TO_EDGE_CLEARANCE,
                   {} },
                 { _( "Physical clearance" ), {}, PHYSICAL_CLEARANCE, {} },
                 { _( "Creepage distance" ),
                   { { _( "Minimum groove for creepage:" ), &BDS::m_MinGrooveWidth, wxS( "min_groove_width" ) } },
                   CREEPAGE_DISTANCE,
                   {} },
                 { _( "Minimum thermal relief spoke count" ), {}, MINIMUM_THERMAL_RELIEF_SPOKE_COUNT, {} } };

    case FAMILY::TRACKS_AND_VIAS:
        return { { _( "Minimum track width" ),
                   { { _( "Board default:" ), &BDS::m_TrackMinWidth, wxS( "min_track_width" ) } },
                   ROUTING_WIDTH,
                   trackWidth },
                 { _( "Minimum connection width" ),
                   { { _( "Board default:" ), &BDS::m_MinConn, wxS( "min_connection" ) } },
                   MINIMUM_CONNECTION_WIDTH,
                   {} },
                 { _( "Minimum annular width" ),
                   { { _( "Board default:" ), &BDS::m_ViasMinAnnularWidth, wxS( "min_via_annular_width" ) } },
                   MINIMUM_ANNULAR_WIDTH,
                   {} },
                 { _( "Minimum via diameter" ),
                   { { _( "Board default:" ), &BDS::m_ViasMinSize, wxS( "min_via_diameter" ) } },
                   MINIMUM_VIA_DIAMETER,
                   viaDiameter },
                 { _( "Via style" ), {}, VIA_STYLE, {} },
                 { _( "Maximum via count" ), {}, MAXIMUM_VIA_COUNT, {} } };

    case FAMILY::HOLES:
        return { { _( "Minimum drill size" ),
                   { { _( "Board default:" ), &BDS::m_MinThroughDrill, wxS( "min_through_hole_diameter" ) } },
                   MINIMUM_DRILL_SIZE,
                   viaDrill },
                 { _( "Hole size" ), {}, HOLE_SIZE, {} },
                 { _( "Hole to hole clearance" ),
                   { { _( "Board default:" ), &BDS::m_HoleToHoleMin, wxS( "min_hole_to_hole" ) } },
                   HOLE_TO_HOLE_DISTANCE,
                   {} },
                 { _( "Microvias" ),
                   { { _( "Minimum uVia diameter:" ), &BDS::m_MicroViasMinSize, wxS( "min_microvia_diameter" ) },
                     { _( "Minimum uVia hole:" ), &BDS::m_MicroViasMinDrill, wxS( "min_microvia_drill" ) } },
                   std::nullopt,
                   uViaDiameter },
                 { _( "Microvia stack depth" ), {}, MICROVIA_STACK_DEPTH, {} },
                 { _( "Hole aspect ratio" ), {}, HOLE_ASPECT_RATIO, {} } };

    case FAMILY::SILKSCREEN:
        return { { _( "Silk to silk clearance" ),
                   { { _( "Board default:" ), &BDS::m_SilkClearance, wxS( "min_silk_clearance" ) } },
                   SILK_TO_SILK_CLEARANCE,
                   {} },
                 { _( "Minimum text height and thickness" ),
                   { { _( "Minimum text height:" ), &BDS::m_MinSilkTextHeight, wxS( "min_text_height" ) },
                     { _( "Minimum text thickness:" ), &BDS::m_MinSilkTextThickness, wxS( "min_text_thickness" ) } },
                   MINIMUM_TEXT_HEIGHT_AND_THICKNESS,
                   {} },
                 { _( "Silk to solder mask clearance" ), {}, SILK_TO_SOLDERMASK_CLEARANCE, {} } };

    case FAMILY::HIGH_SPEED:
        return { { _( "Matched length for differential pairs" ), {}, MATCHED_LENGTH_DIFF_PAIR, {} },
                 { _( "Absolute length" ), {}, ABSOLUTE_LENGTH, {} },
                 { _( "Differential pair routing" ), {}, ROUTING_DIFF_PAIR, {} } };

    case FAMILY::SOLDER_MASK:
        return { { _( "Solder mask expansion" ),
                   { { _( "Board default:" ), &BDS::m_SolderMaskExpansion, wxS( "solder_mask_expansion" ) } },
                   SOLDERMASK_EXPANSION,
                   {},
                   _( "Solder Mask/Paste" ) },
                 { _( "Minimum solder mask sliver" ),
                   { { _( "Board default:" ), &BDS::m_SolderMaskMinWidth, wxS( "solder_mask_min_width" ) } },
                   MINIMUM_SOLDERMASK_SLIVER,
                   {},
                   _( "Solder Mask/Paste" ) },
                 { _( "Solder paste expansion" ),
                   { { _( "Board default:" ), &BDS::m_SolderPasteMargin, wxS( "solder_paste_margin" ) } },
                   SOLDERPASTE_EXPANSION,
                   {},
                   _( "Solder Mask/Paste" ) } };

    case FAMILY::FOOTPRINTS:
        return { { _( "Permitted layers" ), {}, PERMITTED_LAYERS, {} },
                 { _( "Allowed orientation" ), {}, ALLOWED_ORIENTATION, {} },
                 { _( "Vias under SMD pads" ), {}, VIAS_UNDER_SMD, {} },
                 { _( "Courtyard clearance" ), {}, COURTYARD_CLEARANCE, {} } };
    }

    return {};
}
