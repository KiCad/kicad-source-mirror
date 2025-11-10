/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 * @author Jon Evans <jon@craftyjon.com>
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
#include "convert_tool.h"

#include <ranges>

#include <wx/statline.h>
#include <wx/checkbox.h>
#include <wx/button.h>
#include <wx/radiobut.h>

#include <bitmaps.h>
#include <dialog_shim.h>
#include <widgets/unit_binder.h>
#include <board.h>
#include <board_commit.h>
#include <board_design_settings.h>
#include <collectors.h>
#include <confirm.h>
#include <convert_basic_shapes_to_polygon.h>
#include <dialogs/dialog_outset_items.h>
#include <footprint.h>
#include <footprint_edit_frame.h>
#include <geometry/roundrect.h>
#include <geometry/shape_compound.h>
#include <pcb_edit_frame.h>
#include <pcb_shape.h>
#include <pcb_track.h>
#include <pcb_barcode.h>
#include <pad.h>
#include <string_utils.h>
#include <tool/tool_manager.h>
#include <tools/edit_tool.h>
#include <tools/pcb_actions.h>
#include <tools/pcb_selection_tool.h>
#include <tools/pcb_tool_utils.h>
#include <trigo.h>
#include <macros.h>
#include <zone.h>


class CONVERT_SETTINGS_DIALOG : public DIALOG_SHIM
{
public:
    CONVERT_SETTINGS_DIALOG( EDA_DRAW_FRAME* aParent, CONVERT_SETTINGS* aSettings,
                             bool aShowCopyLineWidthOption, bool aShowCenterlineOption,
                             bool aShowBoundingHullOption ) :
            DIALOG_SHIM( aParent, wxID_ANY, _( "Conversion Settings" ), wxDefaultPosition,
                         wxDefaultSize, wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER ),
            m_settings( aSettings )
    {
        // Allow each distinct version of the dialog to have a different size
        m_hash_key = TO_UTF8( wxString::Format( wxS( "%s%c%c%c" ),
                                                GetTitle(),
                                                aShowCopyLineWidthOption,
                                                aShowCenterlineOption,
                                                aShowBoundingHullOption ) );

        wxBoxSizer* mainSizer = new wxBoxSizer( wxVERTICAL );
        wxBoxSizer* topSizer = new wxBoxSizer( wxVERTICAL );
        SetSizer( mainSizer );

        m_rbMimicLineWidth = new wxRadioButton( this, wxID_ANY, _( "Copy line width of first object" ) );

        if( aShowCopyLineWidthOption )
            topSizer->Add( m_rbMimicLineWidth, 0, wxLEFT|wxRIGHT, 5 );
        else
            m_rbMimicLineWidth->Hide();

        m_rbCenterline = new wxRadioButton( this, wxID_ANY, _( "Use centerlines" ) );

        if( aShowCenterlineOption )
        {
            topSizer->AddSpacer( 6 );
            topSizer->Add( m_rbCenterline, 0, wxLEFT|wxRIGHT, 5 );
        }
        else
        {
            m_rbCenterline->Hide();
        }

        m_rbBoundingHull = new wxRadioButton( this, wxID_ANY, _( "Create bounding hull" ) );

        m_gapLabel = new wxStaticText( this, wxID_ANY, _( "Gap:" ) );
        m_gapCtrl = new wxTextCtrl( this, wxID_ANY );
        m_gapUnits = new wxStaticText( this, wxID_ANY, _( "mm" ) );
        m_gap = new UNIT_BINDER( aParent, m_gapLabel, m_gapCtrl, m_gapUnits );

        m_widthLabel = new wxStaticText( this, wxID_ANY, _( "Line width:" ) );
        m_widthCtrl = new wxTextCtrl( this, wxID_ANY );
        m_widthUnits = new wxStaticText( this, wxID_ANY, _( "mm" ) );
        m_width = new UNIT_BINDER( aParent, m_widthLabel, m_widthCtrl, m_widthUnits );

        if( aShowBoundingHullOption )
        {
            topSizer->AddSpacer( 6 );
            topSizer->Add( m_rbBoundingHull, 0, wxLEFT|wxRIGHT, 5 );

            wxBoxSizer* hullParamsSizer = new wxBoxSizer( wxHORIZONTAL );
            hullParamsSizer->Add( m_gapLabel, 0, wxALIGN_CENTRE_VERTICAL, 5 );
            hullParamsSizer->Add( m_gapCtrl, 1, wxALIGN_CENTRE_VERTICAL|wxLEFT|wxRIGHT, 3 );
            hullParamsSizer->Add( m_gapUnits, 0, wxALIGN_CENTRE_VERTICAL, 5 );
            hullParamsSizer->AddSpacer( 18 );
            hullParamsSizer->Add( m_widthLabel, 0, wxALIGN_CENTRE_VERTICAL, 5 );
            hullParamsSizer->Add( m_widthCtrl, 1, wxALIGN_CENTRE_VERTICAL|wxLEFT|wxRIGHT, 3 );
            hullParamsSizer->Add( m_widthUnits, 0, wxALIGN_CENTRE_VERTICAL, 5 );

            topSizer->AddSpacer( 2 );
            topSizer->Add( hullParamsSizer, 0, wxLEFT, 26 );

            topSizer->AddSpacer( 15 );
        }
        else
        {
            m_rbBoundingHull->Hide();
            m_gap->Show( false, true );
            m_width->Show( false, true );
        }

        m_cbDeleteOriginals = new wxCheckBox( this, wxID_ANY, _( "Delete source objects after conversion" ) );
        topSizer->Add( m_cbDeleteOriginals, 0, wxALL, 5 );

        mainSizer->Add( topSizer, 1, wxALL|wxEXPAND, 10 );

        wxBoxSizer* buttonsSizer = new wxBoxSizer( wxHORIZONTAL );
        buttonsSizer->AddStretchSpacer();

        wxStdDialogButtonSizer* sdbSizer = new wxStdDialogButtonSizer();
        wxButton* sdbSizerOK = new wxButton( this, wxID_OK );
        sdbSizer->AddButton( sdbSizerOK );
        wxButton* sdbSizerCancel = new wxButton( this, wxID_CANCEL );
        sdbSizer->AddButton( sdbSizerCancel );
        sdbSizer->Realize();

        buttonsSizer->Add( sdbSizer, 1, 0, 5 );
        mainSizer->Add( buttonsSizer, 0, wxALL|wxEXPAND, 5 );

        SetupStandardButtons();

        m_rbMimicLineWidth->Connect( wxEVT_COMMAND_RADIOBUTTON_SELECTED,
                                     wxCommandEventHandler( CONVERT_SETTINGS_DIALOG::onRadioButton ),
                                     nullptr, this );
        m_rbCenterline->Connect( wxEVT_COMMAND_RADIOBUTTON_SELECTED,
                                 wxCommandEventHandler( CONVERT_SETTINGS_DIALOG::onRadioButton ),
                                 nullptr, this );
        m_rbBoundingHull->Connect( wxEVT_COMMAND_RADIOBUTTON_SELECTED,
                                   wxCommandEventHandler( CONVERT_SETTINGS_DIALOG::onRadioButton ),
                                   nullptr, this );

        finishDialogSettings();
    }

    ~CONVERT_SETTINGS_DIALOG()
    {
        delete m_gap;
        delete m_width;
    };

protected:
    bool TransferDataToWindow() override
    {
        switch( m_settings->m_Strategy )
        {
        case COPY_LINEWIDTH: m_rbMimicLineWidth->SetValue( true ); break;
        case CENTERLINE:     m_rbCenterline->SetValue( true );     break;
        case BOUNDING_HULL:  m_rbBoundingHull->SetValue( true );   break;
        }

        m_gap->Enable( m_rbBoundingHull->GetValue() );
        m_width->Enable( m_rbBoundingHull->GetValue() );
        m_gap->SetValue( m_settings->m_Gap );
        m_width->SetValue( m_settings->m_LineWidth );

        m_cbDeleteOriginals->SetValue( m_settings->m_DeleteOriginals );
        return true;
    }

    bool TransferDataFromWindow() override
    {
        if( m_rbBoundingHull->GetValue() )
            m_settings->m_Strategy = BOUNDING_HULL;
        else if( m_rbCenterline->GetValue() )
            m_settings->m_Strategy = CENTERLINE;
        else
            m_settings->m_Strategy = COPY_LINEWIDTH;

        m_settings->m_Gap = m_gap->GetIntValue();
        m_settings->m_LineWidth = m_width->GetIntValue();

        m_settings->m_DeleteOriginals = m_cbDeleteOriginals->GetValue();
        return true;
    }

    void onRadioButton( wxCommandEvent& aEvent )
    {
        m_gap->Enable( m_rbBoundingHull->GetValue() );
        m_width->Enable( m_rbBoundingHull->GetValue() );
    }

private:
    CONVERT_SETTINGS* m_settings;

    wxRadioButton*    m_rbMimicLineWidth;
    wxRadioButton*    m_rbCenterline;
    wxRadioButton*    m_rbBoundingHull;
    wxStaticText*     m_gapLabel;
    wxTextCtrl*       m_gapCtrl;
    wxStaticText*     m_gapUnits;
    UNIT_BINDER*      m_gap;
    wxStaticText*     m_widthLabel;
    wxTextCtrl*       m_widthCtrl;
    wxStaticText*     m_widthUnits;
    UNIT_BINDER*      m_width;
    wxCheckBox*       m_cbDeleteOriginals;
};


CONVERT_TOOL::CONVERT_TOOL() :
    PCB_TOOL_BASE( "pcbnew.Convert" ),
    m_selectionTool( nullptr ),
    m_menu( nullptr ),
    m_frame( nullptr )
{
    initUserSettings();
}


CONVERT_TOOL::~CONVERT_TOOL()
{
    delete m_menu;
}


using S_C   = SELECTION_CONDITIONS;
using P_S_C = PCB_SELECTION_CONDITIONS;


bool CONVERT_TOOL::Init()
{
    m_selectionTool = m_toolMgr->GetTool<PCB_SELECTION_TOOL>();
    m_frame         = getEditFrame<PCB_BASE_FRAME>();

    // Create a context menu and make it available through selection tool
    m_menu = new CONDITIONAL_MENU( this );
    m_menu->SetIcon( BITMAPS::convert );
    m_menu->SetUntranslatedTitle( _HKI( "Create from Selection" ) );

    static const std::vector<KICAD_T> padTypes =     { PCB_PAD_T };
    static const std::vector<KICAD_T> toArcTypes =   { PCB_ARC_T,
                                                       PCB_TRACE_T,
                                                       PCB_SHAPE_LOCATE_SEGMENT_T };
    static const std::vector<KICAD_T> shapeTypes =   { PCB_SHAPE_LOCATE_SEGMENT_T,
                                                       PCB_SHAPE_LOCATE_RECT_T,
                                                       PCB_SHAPE_LOCATE_CIRCLE_T,
                                                       PCB_SHAPE_LOCATE_ARC_T,
                                                       PCB_SHAPE_LOCATE_BEZIER_T,
                                                       PCB_FIELD_T,
                                                       PCB_TEXT_T,
                                                       PCB_BARCODE_T };
    static const std::vector<KICAD_T> trackTypes =   { PCB_TRACE_T,
                                                       PCB_ARC_T,
                                                       PCB_VIA_T };
    static const std::vector<KICAD_T> toTrackTypes = { PCB_SHAPE_LOCATE_SEGMENT_T,
                                                       PCB_SHAPE_LOCATE_ARC_T };
    static const std::vector<KICAD_T> polyTypes =   { PCB_ZONE_T,
                                                      PCB_SHAPE_LOCATE_POLY_T,
                                                      PCB_SHAPE_LOCATE_RECT_T };
    static const std::vector<KICAD_T> outsetTypes = { PCB_PAD_T, PCB_SHAPE_T };

    auto shapes          = S_C::OnlyTypes( shapeTypes ) && P_S_C::SameLayer();
    auto graphicToTrack  = S_C::OnlyTypes( toTrackTypes );
    auto anyTracks       = S_C::MoreThan( 0 ) && S_C::OnlyTypes( trackTypes ) && P_S_C::SameLayer();
    auto anyPolys        = S_C::OnlyTypes( polyTypes );
    auto anyPads         = S_C::OnlyTypes( padTypes );

    auto canCreateArcs   = S_C::Count( 1 ) && S_C::OnlyTypes( toArcTypes );
    auto canCreateArray  = S_C::MoreThan( 0 );
    auto canCreatePoly   = shapes || anyPolys || anyTracks;

    auto canCreateOutset = S_C::OnlyTypes( outsetTypes );

    if( m_frame->IsType( FRAME_FOOTPRINT_EDITOR ) )
        canCreatePoly = shapes || anyPolys || anyTracks || anyPads;

    auto canCreateLines    = anyPolys;
    auto canCreateTracks   = anyPolys || graphicToTrack;
    auto canCreate         = canCreatePoly
                                || canCreateLines
                                || canCreateTracks
                                || canCreateArcs
                                || canCreateArray
                                || canCreateOutset;

    m_menu->AddItem( PCB_ACTIONS::convertToPoly, canCreatePoly );

    if( m_frame->IsType( FRAME_PCB_EDITOR ) )
        m_menu->AddItem( PCB_ACTIONS::convertToZone, canCreatePoly );

    m_menu->AddItem( PCB_ACTIONS::convertToKeepout, canCreatePoly );
    m_menu->AddItem( PCB_ACTIONS::convertToLines, canCreateLines );
    m_menu->AddItem( PCB_ACTIONS::outsetItems, canCreateOutset );
    m_menu->AddSeparator();

    // Currently the code exists, but tracks are not really existing in footprints
    // only segments on copper layers
    if( m_frame->IsType( FRAME_PCB_EDITOR ) )
        m_menu->AddItem( PCB_ACTIONS::convertToTracks, canCreateTracks );

    m_menu->AddItem( PCB_ACTIONS::convertToArc, canCreateArcs );

    m_menu->AddSeparator();
    m_menu->AddItem( PCB_ACTIONS::createArray, canCreateArray );

    CONDITIONAL_MENU& selToolMenu = m_selectionTool->GetToolMenu().GetMenu();
    selToolMenu.AddMenu( m_menu, canCreate, 100 );

    return true;
}


void CONVERT_TOOL::initUserSettings()
{
    m_userSettings.m_Strategy = CENTERLINE;
    m_userSettings.m_Gap = 0;
    m_userSettings.m_LineWidth = 0;
    m_userSettings.m_DeleteOriginals = true;
}


int CONVERT_TOOL::CreatePolys( const TOOL_EVENT& aEvent )
{
    BOARD_DESIGN_SETTINGS&      bds = m_frame->GetBoard()->GetDesignSettings();
    std::vector<SHAPE_POLY_SET> polys;
    PCB_LAYER_ID                destLayer = m_frame->GetActiveLayer();
    FOOTPRINT*                  parentFootprint = nullptr;

    PCB_SELECTION& selection = m_selectionTool->RequestSelection(
            []( const VECTOR2I& aPt, GENERAL_COLLECTOR& aCollector, PCB_SELECTION_TOOL* sTool )
            {
            } );

    if( selection.Empty() )
        return 0;

    auto getPolys =
            [&]( CONVERT_SETTINGS cfg )
            {
                polys.clear();

                for( EDA_ITEM* item : selection )
                    item->ClearTempFlags();

                SHAPE_POLY_SET polySet;

                polySet.Append( makePolysFromClosedGraphics( selection.GetItems(), cfg.m_Strategy ) );

                if( cfg.m_Strategy == BOUNDING_HULL )
                {
                    polySet.Append( makePolysFromOpenGraphics( selection.GetItems(), 0 ) );

                    polySet.ClearArcs();
                    polySet.Simplify();

                    // Now inflate the bounding hull by cfg.m_Gap
                    polySet.Inflate( cfg.m_Gap, CORNER_STRATEGY::ROUND_ALL_CORNERS, bds.m_MaxError,
                                     ERROR_OUTSIDE );
                }
                else
                {
                    polySet.Append( makePolysFromChainedSegs( selection.GetItems(), cfg.m_Strategy ) );
                }

                if( polySet.IsEmpty() )
                    return false;

                for( int ii = 0; ii < polySet.OutlineCount(); ++ii )
                {
                    polys.emplace_back( SHAPE_POLY_SET( polySet.COutline( ii ) ) );

                    for( int jj = 0; jj < polySet.HoleCount( ii ); ++jj )
                        polys.back().AddHole( polySet.Hole( ii, jj ) );
                }

                return true;
            };

    // Pre-flight getPolys() to see if there's anything to convert.
    CONVERT_SETTINGS preflightSettings = m_userSettings;
    preflightSettings.m_Strategy = BOUNDING_HULL;

    if( !getPolys( preflightSettings ) )
        return 0;

    if( selection.Front() && selection.Front()->IsBOARD_ITEM() )
        parentFootprint = static_cast<BOARD_ITEM*>( selection.Front() )->GetParentFootprint();

    PCB_LAYER_ID layer = m_frame->GetActiveLayer();
    BOARD_COMMIT commit( m_frame );

    if( aEvent.IsAction( &PCB_ACTIONS::convertToPoly ) )
    {
        bool showCopyLineWidth = true;

        // No copy-line-width option for pads
        if( selection.Front()->Type() == PCB_PAD_T )
        {
            if( m_userSettings.m_Strategy == COPY_LINEWIDTH )
                m_userSettings.m_Strategy = CENTERLINE;

            showCopyLineWidth = false;
        }

        CONVERT_SETTINGS previousSettings = m_userSettings;

        CONVERT_SETTINGS_DIALOG dlg( m_frame, &m_userSettings, showCopyLineWidth, true, true );

        if( dlg.ShowModal() != wxID_OK )
            return 0;

        CONVERT_SETTINGS resolvedSettings = m_userSettings;

        if( resolvedSettings.m_Strategy != CENTERLINE )
        {
            if( resolvedSettings.m_LineWidth == 0 )
                resolvedSettings.m_LineWidth = bds.m_LineThickness[ bds.GetLayerClass( layer ) ];
        }

        if( resolvedSettings.m_Strategy == BOUNDING_HULL )
        {
            if( resolvedSettings.m_Gap > 0 )
                resolvedSettings.m_Gap += KiROUND( (double) resolvedSettings.m_LineWidth / 2.0 );
        }

        if( !getPolys( resolvedSettings ) )
        {
            wxString msg;

            if( resolvedSettings.m_Strategy == BOUNDING_HULL )
                msg = _( "Resulting polygon would be empty" );
            else
                msg = _( "Objects must form a closed shape" );

            DisplayErrorMessage( m_frame, _( "Could not convert selection" ), msg );

            m_userSettings = previousSettings;
            return 0;
        }

        for( const SHAPE_POLY_SET& poly : polys )
        {
            PCB_SHAPE* graphic = new PCB_SHAPE( parentFootprint );

            if( resolvedSettings.m_Strategy == COPY_LINEWIDTH )
            {
                BOARD_ITEM* topLeftItem = nullptr;
                VECTOR2I    pos;

                for( EDA_ITEM* item : selection )
                {
                    if( !item->IsBOARD_ITEM() )
                        continue;

                    BOARD_ITEM* candidate = static_cast<BOARD_ITEM*>( item );

                    if( candidate->HasLineStroke() )
                    {
                        pos = candidate->GetPosition();

                        if( !topLeftItem
                            || ( pos.x < topLeftItem->GetPosition().x )
                            || ( topLeftItem->GetPosition().x == pos.x
                                    && pos.y < topLeftItem->GetPosition().y ) )
                        {
                            topLeftItem = candidate;
                            resolvedSettings.m_LineWidth = topLeftItem->GetStroke().GetWidth();
                        }
                    }
                }
            }

            graphic->SetShape( SHAPE_T::POLY );
            graphic->SetStroke( STROKE_PARAMS( resolvedSettings.m_LineWidth, LINE_STYLE::SOLID,
                                               COLOR4D::UNSPECIFIED ) );
            graphic->SetFilled( resolvedSettings.m_Strategy == CENTERLINE );
            graphic->SetLayer( destLayer );
            graphic->SetPolyShape( poly );

            commit.Add( graphic );
        }
    }
    else
    {
        // Creating zone or keepout
        PCB_BASE_EDIT_FRAME*  frame    = getEditFrame<PCB_BASE_EDIT_FRAME>();
        BOARD_ITEM_CONTAINER* parent   = frame->GetModel();
        ZONE_SETTINGS         zoneInfo = bds.GetDefaultZoneSettings();

        bool nonCopper = IsNonCopperLayer( destLayer );
        zoneInfo.m_Layers.reset().set( destLayer );
        zoneInfo.m_Name.Empty();

        int ret;

        // No copy-line-width option for zones/keepouts
        if( m_userSettings.m_Strategy == COPY_LINEWIDTH )
            m_userSettings.m_Strategy = CENTERLINE;

        if( aEvent.IsAction( &PCB_ACTIONS::convertToKeepout ) )
        {
            zoneInfo.SetIsRuleArea( true );
            ret = InvokeRuleAreaEditor( frame, &zoneInfo, board(), &m_userSettings );
        }
        else if( nonCopper )
        {
            zoneInfo.SetIsRuleArea( false );
            ret = InvokeNonCopperZonesEditor( frame, &zoneInfo, &m_userSettings );
        }
        else
        {
            zoneInfo.SetIsRuleArea( false );
            ret = InvokeCopperZonesEditor( frame, nullptr, &zoneInfo, &m_userSettings );
        }

        if( ret == wxID_CANCEL )
            return 0;

        if( !getPolys( m_userSettings ) )
            return 0;

        for( const SHAPE_POLY_SET& poly : polys )
        {
            ZONE* zone = new ZONE( parent );

            *zone->Outline() = poly;
            zone->HatchBorder();

            zoneInfo.ExportSetting( *zone );

            commit.Add( zone );
        }
    }

    if( m_userSettings.m_DeleteOriginals )
    {
        PCB_SELECTION selectionCopy = selection;
        m_selectionTool->ClearSelection();

        for( EDA_ITEM* item : selectionCopy )
        {
            if( item->GetFlags() & SKIP_STRUCT )
                commit.Remove( item );
        }
    }

    if( aEvent.IsAction( &PCB_ACTIONS::convertToPoly ) )
    {
        if( m_userSettings.m_DeleteOriginals )
            commit.Push( _( "Convert to Polygon" ) );
        else
            commit.Push( _( "Create Polygon" ) );
    }
    else
    {
        if( m_userSettings.m_DeleteOriginals )
            commit.Push( _( "Convert to Zone" ) );
        else
            commit.Push( _( "Create Zone" ) );
    }

    return 0;
}


SHAPE_POLY_SET CONVERT_TOOL::makePolysFromChainedSegs( const std::deque<EDA_ITEM*>& aItems,
                                                       CONVERT_STRATEGY aStrategy )
{
    // TODO: This code has a somewhat-similar purpose to ConvertOutlineToPolygon but is slightly
    // different, so this remains a separate algorithm.  It might be nice to analyze the dfiferences
    // in requirements and refactor this.

    // Using a large epsilon here to allow for sloppy drawing can cause the algorithm to miss very
    // short segments in a converted bezier.  So use an epsilon only large enough to cover for
    // rouding errors in the conversion.
    int chainingEpsilon = 100; // max dist from one endPt to next startPt in IU

    SHAPE_POLY_SET poly;

    // Stores pairs of (anchor, item) where anchor == 0 -> SEG.A, anchor == 1 -> SEG.B
    std::map<VECTOR2I, std::vector<std::pair<int, EDA_ITEM*>>> connections;
    std::deque<EDA_ITEM*> toCheck;

    auto closeEnough =
            []( const VECTOR2I& aLeft, const VECTOR2I& aRight, int aLimit )
            {
                return ( aLeft - aRight ).SquaredEuclideanNorm() <= SEG::Square( aLimit );
            };

    auto findInsertionPoint =
            [&]( const VECTOR2I& aPoint ) -> VECTOR2I
            {
                if( connections.count( aPoint ) )
                    return aPoint;

                for( const auto& candidatePair : connections )
                {
                    if( closeEnough( aPoint, candidatePair.first, chainingEpsilon ) )
                        return candidatePair.first;
                }

                return aPoint;
            };

    for( EDA_ITEM* item : aItems )
    {
        if( std::optional<SEG> seg = getStartEndPoints( item ) )
        {
            toCheck.push_back( item );
            connections[findInsertionPoint( seg->A )].emplace_back( std::make_pair( 0, item ) );
            connections[findInsertionPoint( seg->B )].emplace_back( std::make_pair( 1, item ) );
        }
    }

    while( !toCheck.empty() )
    {
        std::vector<BOARD_ITEM*> insertedItems;

        EDA_ITEM* candidate = toCheck.front();
        toCheck.pop_front();

        if( candidate->GetFlags() & SKIP_STRUCT )
            continue;

        SHAPE_LINE_CHAIN outline;

        auto insert =
                [&]( EDA_ITEM* aItem, const VECTOR2I& aAnchor, bool aDirection )
                {
                    if( aItem->IsType( { PCB_ARC_T, PCB_SHAPE_LOCATE_ARC_T } ) )
                    {
                        SHAPE_ARC arc;

                        if( aItem->Type() == PCB_ARC_T )
                        {
                            PCB_ARC* pcb_arc = static_cast<PCB_ARC*>( aItem );
                            arc = *static_cast<SHAPE_ARC*>( pcb_arc->GetEffectiveShape().get() );
                        }
                        else
                        {
                            PCB_SHAPE* pcb_shape = static_cast<PCB_SHAPE*>( aItem );
                            arc = SHAPE_ARC( pcb_shape->GetStart(), pcb_shape->GetArcMid(),
                                             pcb_shape->GetEnd(), pcb_shape->GetWidth() );
                        }

                        if( aDirection )
                            outline.Append( aAnchor == arc.GetP0() ? arc : arc.Reversed() );
                        else
                            outline.Insert( 0, aAnchor == arc.GetP0() ? arc : arc.Reversed() );

                        insertedItems.push_back( static_cast<BOARD_ITEM*>( aItem ) );
                    }
                    else if( aItem->IsType( { PCB_SHAPE_LOCATE_BEZIER_T } ) )
                    {
                        PCB_SHAPE* graphic = static_cast<PCB_SHAPE*>( aItem );

                        if( aAnchor == graphic->GetStart() )
                        {
                            for( const VECTOR2I& pt : graphic->GetBezierPoints() )
                            {
                                if( aDirection )
                                    outline.Append( pt );
                                else
                                    outline.Insert( 0, pt );
                            }

                        }
                        else
                        {
                            for( const VECTOR2I& pt : std::ranges::reverse_view( graphic->GetBezierPoints() ) )
                            {
                                if( aDirection )
                                    outline.Append( pt );
                                else
                                    outline.Insert( 0, pt );
                            }
                        }

                        insertedItems.push_back( static_cast<BOARD_ITEM*>( aItem ) );
                    }
                    else if( std::optional<SEG> nextSeg = getStartEndPoints( aItem ) )
                    {
                        VECTOR2I& point = ( aAnchor == nextSeg->A ) ? nextSeg->B : nextSeg->A;

                        if( aDirection )
                            outline.Append( point );
                        else
                            outline.Insert( 0, point );

                        insertedItems.push_back( static_cast<BOARD_ITEM*>( aItem ) );
                    }
                };

        // aDirection == true for walking "right" and appending to the end of points
        // false for walking "left" and prepending to the beginning
        std::function<void( EDA_ITEM*, const VECTOR2I&, bool )> process =
                [&]( EDA_ITEM* aItem, const VECTOR2I& aAnchor, bool aDirection )
                {
                    if( aItem->GetFlags() & SKIP_STRUCT )
                        return;

                    aItem->SetFlags( SKIP_STRUCT );

                    insert( aItem, aAnchor, aDirection );

                    std::optional<SEG> anchors = getStartEndPoints( aItem );
                    wxASSERT( anchors );

                    VECTOR2I nextAnchor = ( aAnchor == anchors->A ) ? anchors->B : anchors->A;

                    for( std::pair<int, EDA_ITEM*> pair : connections[nextAnchor] )
                    {
                        if( pair.second == aItem )
                            continue;

                        process( pair.second, nextAnchor, aDirection );
                    }
                };

        std::optional<SEG> anchors = getStartEndPoints( candidate );
        wxASSERT( anchors );

        // Start with the first object and walk "right"
        // Note if the first object is an arc, we don't need to insert its first point here, the
        // whole arc will be inserted at anchor B inside process()
        if( !candidate->IsType( { PCB_ARC_T, PCB_SHAPE_LOCATE_ARC_T } ) )
            insert( candidate, anchors->A, true );

        process( candidate, anchors->B, true );

        // check for any candidates on the "left"
        EDA_ITEM* left = nullptr;

        for( std::pair<int, EDA_ITEM*> possibleLeft : connections[anchors->A] )
        {
            if( possibleLeft.second != candidate )
            {
                left = possibleLeft.second;
                break;
            }
        }

        if( left )
            process( left, anchors->A, false );

        if( outline.PointCount() < 3
                || !closeEnough( outline.GetPoint( 0 ), outline.GetPoint( -1 ), chainingEpsilon ) )
        {
            for( EDA_ITEM* item : insertedItems )
                item->ClearFlags( SKIP_STRUCT );

            continue;
        }

        outline.SetClosed( true );

        poly.AddOutline( outline );

        if( aStrategy == BOUNDING_HULL )
        {
            for( BOARD_ITEM* item : insertedItems )
                item->TransformShapeToPolygon( poly, UNDEFINED_LAYER, 0, item->GetMaxError(), ERROR_INSIDE );
        }

        insertedItems.clear();
    }

    return poly;
}


SHAPE_POLY_SET CONVERT_TOOL::makePolysFromOpenGraphics( const std::deque<EDA_ITEM*>& aItems, int aGap )
{
    SHAPE_POLY_SET poly;

    for( EDA_ITEM* item : aItems )
    {
        if( item->GetFlags() & SKIP_STRUCT )
            continue;

        switch( item->Type() )
        {
        case PCB_SHAPE_T:
        {
            PCB_SHAPE* shape = static_cast<PCB_SHAPE*>( item );

            if( shape->IsClosed() )
                continue;

            shape->TransformShapeToPolygon( poly, UNDEFINED_LAYER, aGap, shape->GetMaxError(), ERROR_INSIDE );
            shape->SetFlags( SKIP_STRUCT );

            break;
        }

        case PCB_TRACE_T:
        case PCB_ARC_T:
        case PCB_VIA_T:
        {
            PCB_TRACK* track = static_cast<PCB_TRACK*>( item );

            track->TransformShapeToPolygon( poly, UNDEFINED_LAYER, aGap, track->GetMaxError(), ERROR_INSIDE );
            track->SetFlags( SKIP_STRUCT );

            break;
        }

        default:
            continue;
        }
    }

    return poly;
}


SHAPE_POLY_SET CONVERT_TOOL::makePolysFromClosedGraphics( const std::deque<EDA_ITEM*>& aItems,
                                                          CONVERT_STRATEGY aStrategy )
{
    SHAPE_POLY_SET poly;

    for( EDA_ITEM* item : aItems )
    {
        if( item->GetFlags() & SKIP_STRUCT )
            continue;

        switch( item->Type() )
        {
        case PCB_SHAPE_T:
        {
            PCB_SHAPE* shape = static_cast<PCB_SHAPE*>( item );
            FILL_T     wasFilled = shape->GetFillMode();

            if( !shape->IsClosed() )
                continue;

            if( shape->GetShape() == SHAPE_T::CIRCLE && aStrategy != BOUNDING_HULL )
            {
                VECTOR2I  c = shape->GetCenter();
                int       R = shape->GetRadius();
                SHAPE_ARC arc( c - VECTOR2I( R, 0 ), c + VECTOR2I( R, 0 ), c - VECTOR2I( R, 0 ), 0 );

                poly.NewOutline();
                poly.Append( arc );
            }
            else
            {
                if( aStrategy != BOUNDING_HULL )
                    shape->SetFilled( true );

                shape->TransformShapeToPolygon( poly, UNDEFINED_LAYER, 0, shape->GetMaxError(), ERROR_INSIDE,
                                                aStrategy != BOUNDING_HULL );

                if( aStrategy != BOUNDING_HULL )
                    shape->SetFillMode( wasFilled );
            }

            shape->SetFlags( SKIP_STRUCT );
            break;
        }

        case PCB_ZONE_T:
            poly.Append( *static_cast<ZONE*>( item )->Outline() );
            item->SetFlags( SKIP_STRUCT );
            break;

        case PCB_FIELD_T:
        case PCB_TEXT_T:
        {
            PCB_TEXT* text = static_cast<PCB_TEXT*>( item );
            text->TransformTextToPolySet( poly, 0, text->GetMaxError(), ERROR_INSIDE );
            text->SetFlags( SKIP_STRUCT );
            break;
        }

        case PCB_BARCODE_T:
        {
            PCB_BARCODE* barcode = static_cast<PCB_BARCODE*>( item );

            if( aStrategy == BOUNDING_HULL )
                barcode->GetBoundingHull( poly, UNDEFINED_LAYER, 0, barcode->GetMaxError(), ERROR_INSIDE );
            else
                barcode->TransformShapeToPolySet( poly, UNDEFINED_LAYER, 0, barcode->GetMaxError(), ERROR_INSIDE );

            barcode->SetFlags( SKIP_STRUCT );
            break;
        }

        case PCB_PAD_T:
        {
            PAD* pad = static_cast<PAD*>( item );

            pad->Padstack().ForEachUniqueLayer(
                    [&]( PCB_LAYER_ID aLayer )
                    {
                        SHAPE_POLY_SET layerPoly;
                        pad->TransformShapeToPolygon( layerPoly, aLayer, 0, pad->GetMaxError(), ERROR_INSIDE );
                        poly.BooleanAdd( layerPoly );
                    } );

            pad->SetFlags( SKIP_STRUCT );
            break;
        }


        default:
            continue;
        }
    }

    return poly;
}


int CONVERT_TOOL::CreateLines( const TOOL_EVENT& aEvent )
{
    auto& selection = m_selectionTool->RequestSelection(
            []( const VECTOR2I& aPt, GENERAL_COLLECTOR& aCollector, PCB_SELECTION_TOOL* sTool )
            {
                for( int i = aCollector.GetCount() - 1; i >= 0; --i )
                {
                    BOARD_ITEM* item = aCollector[i];

                    switch( item->Type() )
                    {
                    case PCB_SHAPE_T:
                        switch( static_cast<PCB_SHAPE*>( item )->GetShape() )
                        {
                        case SHAPE_T::SEGMENT:
                        case SHAPE_T::ARC:
                        case SHAPE_T::POLY:
                        case SHAPE_T::RECTANGLE:
                            break;

                        default:
                            aCollector.Remove( item );
                        }

                        break;

                    case PCB_ZONE_T:
                        break;

                    default:
                        aCollector.Remove( item );
                    }
                }
            } );

    if( selection.Empty() )
        return 0;

    BOARD_COMMIT          commit( m_frame );
    PCB_BASE_EDIT_FRAME*  frame       = getEditFrame<PCB_BASE_EDIT_FRAME>();
    FOOTPRINT_EDIT_FRAME* fpEditor    = dynamic_cast<FOOTPRINT_EDIT_FRAME*>( m_frame );
    FOOTPRINT*            footprint   = nullptr;
    PCB_LAYER_ID          targetLayer = m_frame->GetActiveLayer();
    BOARD_ITEM_CONTAINER* parent      = frame->GetModel();

    if( fpEditor )
        footprint = fpEditor->GetBoard()->GetFirstFootprint();

    auto handleGraphicSeg =
            [&]( EDA_ITEM* aItem )
            {
                if( aItem->Type() != PCB_SHAPE_T )
                    return false;

                PCB_SHAPE* graphic = static_cast<PCB_SHAPE*>( aItem );

                if( graphic->GetShape() == SHAPE_T::SEGMENT )
                {
                    PCB_TRACK* track = new PCB_TRACK( parent );

                    track->SetLayer( targetLayer );
                    track->SetStart( graphic->GetStart() );
                    track->SetEnd( graphic->GetEnd() );
                    track->SetWidth( graphic->GetWidth() );
                    commit.Add( track );

                    return true;
                }
                else if( graphic->GetShape() == SHAPE_T::ARC )
                {
                    PCB_ARC* arc = new PCB_ARC( parent );

                    arc->SetLayer( targetLayer );
                    arc->SetStart( graphic->GetStart() );
                    arc->SetEnd( graphic->GetEnd() );
                    arc->SetMid( graphic->GetArcMid() );
                    arc->SetWidth( graphic->GetWidth() );
                    commit.Add( arc );

                    return true;
                }

                return false;
            };

    auto addGraphicChain =
            [&]( const SHAPE_LINE_CHAIN& aChain, std::optional<int> aWidth )
            {
                for( size_t si = 0; si < aChain.GetSegmentCount(); ++si )
                {
                    const SEG seg = aChain.GetSegment( si );

                    if( seg.Length() == 0 )
                        continue;

                    if( aChain.IsArcSegment( si ) )
                        continue;

                    PCB_SHAPE* graphic = new PCB_SHAPE( footprint, SHAPE_T::SEGMENT );

                    graphic->SetLayer( targetLayer );
                    graphic->SetStart( seg.A );
                    graphic->SetEnd( seg.B );

                    if( aWidth && *aWidth > 0 )
                        graphic->SetWidth( *aWidth );

                    commit.Add( graphic );
                }

                for( size_t ai = 0; ai < aChain.ArcCount(); ++ai )
                {
                    const SHAPE_ARC& arc = aChain.Arc( ai );

                    if( arc.GetP0() == arc.GetP1() )
                        continue;

                    PCB_SHAPE* graphic = new PCB_SHAPE( footprint, SHAPE_T::ARC );

                    graphic->SetLayer( targetLayer );
                    graphic->SetFilled( false );
                    graphic->SetArcGeometry( arc.GetP0(), arc.GetArcMid(), arc.GetP1() );

                    if( aWidth && *aWidth > 0 )
                        graphic->SetWidth( *aWidth );

                    commit.Add( graphic );
                }
            };

    auto addTrackChain =
            [&]( const SHAPE_LINE_CHAIN& aChain, std::optional<int> aWidth )
            {
                for( size_t si = 0; si < aChain.GetSegmentCount(); ++si )
                {
                    const SEG seg = aChain.GetSegment( si );

                    if( seg.Length() == 0 )
                        continue;

                    if( aChain.IsArcSegment( si ) )
                        continue;

                    PCB_TRACK* track = new PCB_TRACK( parent );

                    track->SetLayer( targetLayer );
                    track->SetStart( seg.A );
                    track->SetEnd( seg.B );

                    if( aWidth && *aWidth > 0 )
                        track->SetWidth( *aWidth );

                    commit.Add( track );
                }

                for( size_t ai = 0; ai < aChain.ArcCount(); ++ai )
                {
                    const SHAPE_ARC& arc = aChain.Arc( ai );

                    if( arc.GetP0() == arc.GetP1() )
                        continue;

                    PCB_ARC* trackArc = new PCB_ARC( parent );

                    trackArc->SetLayer( targetLayer );
                    trackArc->SetStart( arc.GetP0() );
                    trackArc->SetEnd( arc.GetP1() );
                    trackArc->SetMid( arc.GetArcMid() );

                    if( aWidth && *aWidth > 0 )
                        trackArc->SetWidth( *aWidth );

                    commit.Add( trackArc );
                }
            };

    auto processChain =
            [&]( const SHAPE_LINE_CHAIN& aChain, std::optional<int> aWidth )
            {
                if( aChain.GetSegmentCount() == 0 && aChain.ArcCount() == 0 )
                    return;

                if( aEvent.IsAction( &PCB_ACTIONS::convertToLines ) )
                {
                    addGraphicChain( aChain, aWidth );
                }
                else if( fpEditor )
                {
                    addGraphicChain( aChain, aWidth );
                }
                else
                {
                    addTrackChain( aChain, aWidth );
                }
            };

    auto processPolySet =
            [&]( const SHAPE_POLY_SET& aPoly, std::optional<int> aWidth )
            {
                for( int oi = 0; oi < aPoly.OutlineCount(); ++oi )
                {
                    processChain( aPoly.COutline( oi ), aWidth );

                    for( int hi = 0; hi < aPoly.HoleCount( oi ); ++hi )
                        processChain( aPoly.CHole( oi, hi ), aWidth );
                }
            };

    if( aEvent.IsAction( &PCB_ACTIONS::convertToTracks ) )
    {
        if( !IsCopperLayer( targetLayer ) )
        {
            targetLayer = frame->SelectOneLayer( F_Cu, LSET::AllNonCuMask() );

            if( targetLayer == UNDEFINED_LAYER )    // User canceled
                return true;
        }
    }
    else
    {
        CONVERT_SETTINGS_DIALOG dlg( m_frame, &m_userSettings, false, false, false );

        if( dlg.ShowModal() != wxID_OK )
            return true;
    }

    for( EDA_ITEM* item : selection )
    {
        if( !item->IsBOARD_ITEM() )
            continue;

        if( handleGraphicSeg( item ) )
            continue;

        BOARD_ITEM& boardItem = static_cast<BOARD_ITEM&>( *item );
        std::optional<int> itemWidth = GetBoardItemWidth( boardItem );

        if( boardItem.Type() == PCB_SHAPE_T )
        {
            PCB_SHAPE* graphic = static_cast<PCB_SHAPE*>( item );

            switch( graphic->GetShape() )
            {
            case SHAPE_T::RECTANGLE:
            {
                SHAPE_RECT rect( graphic->GetStart(), graphic->GetEnd() );
                ROUNDRECT  rrect( rect, graphic->GetCornerRadius(), true );
                SHAPE_POLY_SET poly;

                rrect.TransformToPolygon( poly, graphic->GetMaxError() );
                processPolySet( poly, itemWidth );
                break;
            }

            case SHAPE_T::POLY:
                processPolySet( graphic->GetPolyShape(), itemWidth );
                break;

            default:
                wxFAIL_MSG( wxT( "Unhandled graphic shape type in PolyToLines" ) );
                break;
            }
        }
        else if( boardItem.Type() == PCB_ZONE_T )
        {
            ZONE* zone = static_cast<ZONE*>( item );
            processPolySet( *zone->Outline(), itemWidth );
        }
        else
        {
            wxFAIL_MSG( wxT( "Unhandled type in PolyToLines" ) );
        }
    }

    if( m_userSettings.m_DeleteOriginals )
    {
        PCB_SELECTION selectionCopy = selection;
        m_selectionTool->ClearSelection();

        for( EDA_ITEM* item : selectionCopy )
            commit.Remove( item );
    }

    commit.Push( _( "Create Lines" ) );

    return 0;
}


int CONVERT_TOOL::SegmentToArc( const TOOL_EVENT& aEvent )
{
    auto& selection = m_selectionTool->RequestSelection(
            []( const VECTOR2I& aPt, GENERAL_COLLECTOR& aCollector, PCB_SELECTION_TOOL* sTool )
            {
                for( int i = aCollector.GetCount() - 1; i >= 0; --i )
                {
                    BOARD_ITEM* item = aCollector[i];

                    if( !item->IsType( { PCB_SHAPE_T, PCB_TRACE_T, PCB_ARC_T } ) )
                        aCollector.Remove( item );
                }
            } );

    if( selection.Empty() || !selection.Front()->IsBOARD_ITEM() )
        return -1;

    BOARD_ITEM* source = static_cast<BOARD_ITEM*>( selection.Front() );
    VECTOR2I    start, end, mid;

    // Offset the midpoint along the normal a little bit so that it's more obviously an arc
    const double offsetRatio = 0.1;

    if( std::optional<SEG> seg = getStartEndPoints( source ) )
    {
        start = seg->A;
        end   = seg->B;

        VECTOR2I normal = ( seg->B - seg->A ).Perpendicular().Resize( offsetRatio * seg->Length() );
        mid = seg->Center() + normal;
    }
    else
    {
        return -1;
    }

    PCB_BASE_EDIT_FRAME*  frame  = getEditFrame<PCB_BASE_EDIT_FRAME>();
    BOARD_ITEM_CONTAINER* parent = frame->GetModel();
    PCB_LAYER_ID          layer = source->GetLayer();
    BOARD_COMMIT          commit( m_frame );

    if( source->Type() == PCB_SHAPE_T && static_cast<PCB_SHAPE*>( source )->GetShape() == SHAPE_T::SEGMENT )
    {
        PCB_SHAPE* line = static_cast<PCB_SHAPE*>( source );
        PCB_SHAPE* arc  = new PCB_SHAPE( parent, SHAPE_T::ARC );

        VECTOR2I center = CalcArcCenter( start, mid, end );

        arc->SetFilled( false );
        arc->SetLayer( layer );
        arc->SetStroke( line->GetStroke() );

        arc->SetCenter( center );
        arc->SetStart( start );
        arc->SetEnd( end );

        commit.Add( arc );
    }
    else if( source->Type() == PCB_SHAPE_T && static_cast<PCB_SHAPE*>( source )->GetShape() == SHAPE_T::ARC )
    {
        PCB_SHAPE* source_arc = static_cast<PCB_SHAPE*>( source );
        PCB_ARC*   arc  = new PCB_ARC( parent );

        arc->SetLayer( layer );
        arc->SetWidth( source_arc->GetWidth() );
        arc->SetStart( start );
        arc->SetMid( source_arc->GetArcMid() );
        arc->SetEnd( end );

        commit.Add( arc );
    }
    else if( source->Type() == PCB_TRACE_T )
    {
        PCB_TRACK* line = static_cast<PCB_TRACK*>( source );
        PCB_ARC*   arc  = new PCB_ARC( parent );

        arc->SetLayer( layer );
        arc->SetWidth( line->GetWidth() );
        arc->SetStart( start );
        arc->SetMid( mid );
        arc->SetEnd( end );

        commit.Add( arc );
    }
    else if( source->Type() == PCB_ARC_T )
    {
        PCB_ARC* source_arc = static_cast<PCB_ARC*>( source );
        PCB_SHAPE* arc  = new PCB_SHAPE( parent, SHAPE_T::ARC );

        arc->SetFilled( false );
        arc->SetLayer( layer );
        arc->SetWidth( source_arc->GetWidth() );

        arc->SetArcGeometry( source_arc->GetStart(), source_arc->GetMid(), source_arc->GetEnd() );
        commit.Add( arc );
    }

    commit.Push( _( "Create Arc" ) );

    return 0;
}


std::optional<SEG> CONVERT_TOOL::getStartEndPoints( EDA_ITEM* aItem )
{
    switch( aItem->Type() )
    {
    case PCB_SHAPE_T:
    {
        PCB_SHAPE* shape = static_cast<PCB_SHAPE*>( aItem );

        switch( shape->GetShape() )
        {
        case SHAPE_T::SEGMENT:
        case SHAPE_T::ARC:
        case SHAPE_T::POLY:
        case SHAPE_T::BEZIER:
            if( shape->GetStart() == shape->GetEnd() )
                return std::nullopt;

            return std::make_optional<SEG>( VECTOR2I( shape->GetStart() ), VECTOR2I( shape->GetEnd() ) );

        default:
            return std::nullopt;
        }
    }

    case PCB_TRACE_T:
    {
        PCB_TRACK* line = static_cast<PCB_TRACK*>( aItem );
        return std::make_optional<SEG>( VECTOR2I( line->GetStart() ), VECTOR2I( line->GetEnd() ) );
    }

    case PCB_ARC_T:
    {
        PCB_ARC* arc = static_cast<PCB_ARC*>( aItem );
        return std::make_optional<SEG>( VECTOR2I( arc->GetStart() ), VECTOR2I( arc->GetEnd() ) );
    }

    default:
        return std::nullopt;
    }
}


int CONVERT_TOOL::OutsetItems( const TOOL_EVENT& aEvent )
{
    PCB_BASE_EDIT_FRAME& frame = *getEditFrame<PCB_BASE_EDIT_FRAME>();
    PCB_SELECTION&       selection = m_selectionTool->RequestSelection(
            []( const VECTOR2I& aPt, GENERAL_COLLECTOR& aCollector, PCB_SELECTION_TOOL* sTool )
            {
                // Iterate from the back so we don't have to worry about removals.
                for( int i = aCollector.GetCount() - 1; i >= 0; --i )
                {
                    BOARD_ITEM* item = aCollector[i];

                    // We've converted the polygon and rectangle to segments, so drop everything
                    // that isn't a segment at this point
                    if( !item->IsType( { PCB_PAD_T, PCB_SHAPE_T } ) )
                        aCollector.Remove( item );
                }

                sTool->FilterCollectorForLockedItems( aCollector );
            } );

    BOARD_COMMIT commit( this );

    for( EDA_ITEM* item : selection )
        item->ClearFlags( STRUCT_DELETED );

    // List of thing to select at the end of the operation
    // (doing it as we go will invalidate the iterator)
    std::vector<BOARD_ITEM*> items_to_select_on_success;

    // Handle modifications to existing items by the routine
    // How to deal with this depends on whether we're in the footprint editor or not
    // and whether the item was conjured up by decomposing a polygon or rectangle
    auto item_modification_handler =
            [&]( BOARD_ITEM& aItem )
            {
            };

    bool any_items_created = false;

    auto item_creation_handler =
            [&]( std::unique_ptr<BOARD_ITEM> aItem )
            {
                any_items_created = true;
                items_to_select_on_success.push_back( aItem.get() );
                commit.Add( aItem.release() );
            };

    auto item_removal_handler =
            [&]( BOARD_ITEM& aItem )
            {
                // If you do an outset on a FP pad, do you really want to delete
                // the parent?
                if( !aItem.GetParentFootprint() )
                {
                    commit.Remove( &aItem );
                }
            };

    // Combine these callbacks into a CHANGE_HANDLER to inject in the ROUTINE
    ITEM_MODIFICATION_ROUTINE::CALLABLE_BASED_HANDLER change_handler( item_creation_handler,
                                                                      item_modification_handler,
                                                                      item_removal_handler );

    // Persistent settings between dialog invocations
    // Init with some sensible defaults
    static OUTSET_ROUTINE::PARAMETERS outset_params_fp_edit{
        pcbIUScale.mmToIU( 0.25 ), // A common outset value
        false,
        false,
        true,
        F_CrtYd,
        frame.GetDesignSettings().GetLineThickness( F_CrtYd ),
        pcbIUScale.mmToIU( 0.01 ),
        false,
    };

    static OUTSET_ROUTINE::PARAMETERS outset_params_pcb_edit{
        pcbIUScale.mmToIU( 1 ),
        true,
        true,
        true,
        Edge_Cuts, // Outsets often for slots?
        frame.GetDesignSettings().GetLineThickness( Edge_Cuts ),
        std::nullopt,
        false,
    };

    OUTSET_ROUTINE::PARAMETERS& outset_params = IsFootprintEditor() ? outset_params_fp_edit
                                                                    : outset_params_pcb_edit;

    {
        DIALOG_OUTSET_ITEMS dlg( frame, outset_params );

        if( dlg.ShowModal() == wxID_CANCEL )
            return 0;
    }

    OUTSET_ROUTINE outset_routine( frame.GetModel(), change_handler, outset_params );

    for( EDA_ITEM* item : selection )
    {
        if( !item->IsBOARD_ITEM() )
            continue;

        BOARD_ITEM* board_item = static_cast<BOARD_ITEM*>( item );
        outset_routine.ProcessItem( *board_item );
    }

    // Deselect all the original items
    m_selectionTool->ClearSelection();

    // Select added and modified items
    for( BOARD_ITEM* item : items_to_select_on_success )
        m_selectionTool->AddItemToSel( item, true );

    if( any_items_created )
        m_toolMgr->ProcessEvent( EVENTS::SelectedEvent );

    // Notify other tools of the changes
    m_toolMgr->ProcessEvent( EVENTS::SelectedItemsModified );

    commit.Push( outset_routine.GetCommitDescription() );

    if( const std::optional<wxString> msg = outset_routine.GetStatusMessage() )
        frame.ShowInfoBarMsg( *msg );

    return 0;
}


void CONVERT_TOOL::setTransitions()
{
    // clang-format off
    Go( &CONVERT_TOOL::CreatePolys,    PCB_ACTIONS::convertToPoly.MakeEvent() );
    Go( &CONVERT_TOOL::CreatePolys,    PCB_ACTIONS::convertToZone.MakeEvent() );
    Go( &CONVERT_TOOL::CreatePolys,    PCB_ACTIONS::convertToKeepout.MakeEvent() );
    Go( &CONVERT_TOOL::CreateLines,    PCB_ACTIONS::convertToLines.MakeEvent() );
    Go( &CONVERT_TOOL::CreateLines,    PCB_ACTIONS::convertToTracks.MakeEvent() );
    Go( &CONVERT_TOOL::SegmentToArc,   PCB_ACTIONS::convertToArc.MakeEvent() );
    Go( &CONVERT_TOOL::OutsetItems,    PCB_ACTIONS::outsetItems.MakeEvent() );
    // clang-format on
}
