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
 * You should have received a copy of the GNU General Public License along
 * with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include <dialogs/git/git_file_diff_area.h>

#include <algorithm>
#include <fstream>
#include <optional>
#include <utility>

#include <wx/sizer.h>
#include <wx/splitter.h>
#include <wx/stattext.h>
#include <wx/stc/stc.h>
#include <wx/utils.h>

#include <dialogs/git/dialog_text_diff.h>
#include <diff_merge/diff_scene.h>
#include <diff_merge/merge_dispatch.h>
#include <eda_draw_frame.h>
#include <git/text_diff.h>
#include <kiway.h>
#include <project.h>
#include <reporter.h>
#include <view/view.h>
#include <widgets/widget_diff_canvas.h>


namespace
{

// Where to open the before and after canvases: on the first change, padded to a readable size.
std::optional<BOX2I> firstChangeBox( const KICAD_DIFF::DIFF_SCENE& aScene, KICAD_DIFF::DOC_KIND aKind )
{
    for( const std::vector<KICAD_DIFF::SCENE_SHAPE>* shapes :
         { &aScene.modifiedShapes, &aScene.addedShapes, &aScene.removedShapes } )
    {
        if( shapes->empty() )
            continue;

        BOX2I box = shapes->front().bbox;

        if( box.GetWidth() <= 0 || box.GetHeight() <= 0 )
            continue;

        int minSide = KICAD_DIFF::IuScaleForDocKind( aKind ).mmToIU( 50 );
        int padX = static_cast<int>( minSide - box.GetWidth() ) / 2;
        int padY = static_cast<int>( minSide - box.GetHeight() ) / 2;
        box.Inflate( std::max( 0, padX ), std::max( 0, padY ) );
        return box;
    }

    return std::nullopt;
}


std::string readAll( const wxString& aFile )
{
    if( aFile.IsEmpty() )
        return std::string();

    std::ifstream in( aFile.fn_str(), std::ios::binary );
    return std::string( ( std::istreambuf_iterator<char>( in ) ), std::istreambuf_iterator<char>() );
}


struct FILE_DIFF_RESULT
{
    // Keep the extracted files until the UI has loaded the drawing documents.
    std::unique_ptr<GIT_DIFF_WORKSPACE> workspace;
    GIT_FILE_SIDES                      sides;
    std::vector<KIGIT::TEXT_DIFF_LINE>  textLines;
    bool                                ready = false;
};

} // namespace


GIT_FILE_DIFF_AREA::GIT_FILE_DIFF_AREA( wxWindow* aParent, EDA_BASE_FRAME* aFrame ) :
        wxPanel( aParent ),
        m_frame( aFrame ),
        m_note( nullptr ),
        m_canvasSplitter( nullptr ),
        m_beforeLabel( nullptr ),
        m_afterLabel( nullptr ),
        m_beforeCanvas( nullptr ),
        m_afterCanvas( nullptr ),
        m_textDiff( nullptr ),
        m_legend( nullptr )
{
    wxBoxSizer* sizer = new wxBoxSizer( wxVERTICAL );

    // One of these shows at a time: a note, the two drawings, or the line diff.
    m_note = new wxStaticText( this, wxID_ANY, wxEmptyString );
    m_canvasSplitter = new wxSplitterWindow( this, wxID_ANY, wxDefaultPosition, wxDefaultSize,
                                             wxSP_LIVE_UPDATE | wxSP_3DSASH );
    m_canvasSplitter->SetMinimumPaneSize( FromDIP( 60 ) );
    m_canvasSplitter->SetSashGravity( 0.5 );
    m_textDiff = new wxStyledTextCtrl( this, wxID_ANY );

    // What the marks on the drawings mean, in the colours they are drawn with.
    KICAD_DIFF::DIFF_COLOR_THEME theme;
    m_legend = new wxBoxSizer( wxHORIZONTAL );

    for( const auto& [colour, label] : { std::make_pair( theme.added, _( "Added" ) ),
                                         std::make_pair( theme.removed, _( "Removed" ) ),
                                         std::make_pair( theme.modified, _( "Modified" ) ) } )
    {
        wxPanel* swatch = new wxPanel( this, wxID_ANY, wxDefaultPosition, FromDIP( wxSize( 12, 12 ) ) );
        swatch->SetBackgroundColour( colour.ToColour() );
        m_legend->Add( swatch, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 4 );
        m_legend->Add( new wxStaticText( this, wxID_ANY, label ), 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 16 );
    }

    sizer->Add( m_note, 0, wxEXPAND | wxALL, 4 );
    sizer->Add( m_canvasSplitter, 1, wxEXPAND );
    sizer->Add( m_legend, 0, wxLEFT | wxTOP, 4 );
    sizer->Add( m_textDiff, 1, wxEXPAND );
    SetSizer( sizer );

    m_note->Hide();
    m_canvasSplitter->Hide();
    sizer->Show( m_legend, false );
    m_textDiff->Hide();
}


GIT_FILE_DIFF_AREA::~GIT_FILE_DIFF_AREA()
{
    m_query.Cancel();

    // The canvases still point at the documents, so the documents go first.
    m_diffContext.reset();
}


void GIT_FILE_DIFF_AREA::Clear()
{
    m_query.Cancel();
    destroyCanvases();
}


void GIT_FILE_DIFF_AREA::ShowFile( const std::shared_ptr<KIGIT_COMMON>& aSession, const MATERIALIZE& aMaterialize,
                                   const wxString& aBeforeLabel, const wxString& aAfterLabel )
{
    showNote( _( "Loading comparison..." ) );

    auto result = std::make_shared<FILE_DIFF_RESULT>();
    result->sides.error = _( "Could not compare this file." );

    m_query.Run(
            aSession,
            [result, aMaterialize]( KIGIT_COMMON& aCommon )
            {
                result->workspace = std::make_unique<GIT_DIFF_WORKSPACE>();

                if( !result->workspace->IsOk() )
                {
                    result->sides.error = _( "Could not create a temporary directory for the diff." );
                    return;
                }

                if( !aMaterialize( aCommon, *result->workspace, result->sides ) )
                    return;

                if( result->sides.kind == KICAD_DIFF::DOC_KIND::UNKNOWN )
                {
                    std::string before = readAll( result->sides.before );
                    std::string after = readAll( result->sides.after );

                    if( KIGIT::TextLooksBinary( before ) || KIGIT::TextLooksBinary( after ) )
                    {
                        result->sides.error = _( "This is not a plain text file, so there is no comparison to show." );
                        return;
                    }

                    result->textLines =
                            KIGIT::BuildTextDiff( KIGIT::SplitTextLines( before ), KIGIT::SplitTextLines( after ) );
                }

                result->ready = true;
            },
            [this, result, aSession, aBeforeLabel, aAfterLabel]()
            {
                if( m_frame->Prj().GetGitCommon() != aSession )
                    return;

                if( !result->ready )
                {
                    showNote( result->sides.error );
                    return;
                }

                // Kiface loading and canvas creation must stay on the UI thread.
                showDiff( result->sides, result->textLines, aBeforeLabel, aAfterLabel );
            } );
}


void GIT_FILE_DIFF_AREA::showNote( const wxString& aNote )
{
    destroyCanvases();
    m_note->SetLabel( aNote );
    m_note->Show();
    m_canvasSplitter->Hide();
    GetSizer()->Show( m_legend, false );
    m_textDiff->Hide();
    Layout();
}


void GIT_FILE_DIFF_AREA::showDiff( const GIT_FILE_SIDES& aSides, const std::vector<KIGIT::TEXT_DIFF_LINE>& aTextLines,
                                   const wxString& aBeforeLabel, const wxString& aAfterLabel )
{
    bool shown = true;

    if( aSides.kind == KICAD_DIFF::DOC_KIND::UNKNOWN )
        showTextDiff( aTextLines );
    else if( aSides.kind == KICAD_DIFF::DOC_KIND::SCH || aSides.kind == KICAD_DIFF::DOC_KIND::PCB )
        shown = showDrawingDiff( aSides.kind, aSides.before, aSides.after, aBeforeLabel, aAfterLabel );
    else
        showNote( _( "Double click the file to open its comparison." ) );

    if( shown )
        Layout();
}


void GIT_FILE_DIFF_AREA::showTextDiff( const std::vector<KIGIT::TEXT_DIFF_LINE>& aLines )
{
    destroyCanvases();
    FillTextDiffControl( m_textDiff, aLines );
    m_note->Hide();
    m_canvasSplitter->Hide();
    GetSizer()->Show( m_legend, false );
    m_textDiff->Show();
}


bool GIT_FILE_DIFF_AREA::showDrawingDiff( KICAD_DIFF::DOC_KIND aKind, const wxString& aBefore,
                                          const wxString& aAfter, const wxString& aBeforeLabel,
                                          const wxString& aAfterLabel )
{
    ensureCanvases();

    // The old documents come off the canvases before the new ones go on.
    m_diffContext.reset();

    KICAD_DIFF::DOCUMENT_DIFF diff;
    WX_STRING_REPORTER        reporter;

    m_diffContext = KICAD_DIFF::DispatchAttachDiffCanvases( m_frame->Kiway(), aKind, aBefore, aAfter,
                                                            *m_beforeCanvas, *m_afterCanvas, diff, &reporter );

    if( !m_diffContext )
    {
        showNote( reporter.HasMessage() ? reporter.GetMessages() : _( "Could not compare this file." ) );
        return false;
    }

    KICAD_DIFF::DIFF_COLOR_THEME theme;
    KICAD_DIFF::DIFF_SCENE       scene = KICAD_DIFF::BuildScene( diff, theme );
    scene.docKind = aKind;

    // Both sides open on the same change. The user pans from there and the other side follows.
    std::optional<BOX2I> focus = firstChangeBox( scene, aKind );
    m_beforeCanvas->SetFocusBox( focus );
    m_afterCanvas->SetFocusBox( focus );

    KICAD_DIFF::SIDE_BY_SIDE_SCENES sides = KICAD_DIFF::SplitSceneBySide( scene );

    m_beforeLabel->SetLabel( aBeforeLabel );
    m_afterLabel->SetLabel( aAfterLabel );
    m_beforeCanvas->SetScene( std::move( sides.reference ) );
    m_afterCanvas->SetScene( std::move( sides.comparison ) );

    m_note->Hide();
    m_textDiff->Hide();
    m_canvasSplitter->Show();
    GetSizer()->Show( m_legend, true );
    return true;
}


void GIT_FILE_DIFF_AREA::ensureCanvases()
{
    if( m_beforeCanvas )
        return;

    // The editor's own backend where there is one. The project manager has no canvas.
    EDA_DRAW_PANEL_GAL::GAL_TYPE backend = EDA_DRAW_PANEL_GAL::GAL_FALLBACK;

    if( EDA_DRAW_FRAME* frame = dynamic_cast<EDA_DRAW_FRAME*>( m_frame ) )
    {
        if( frame->GetCanvas() )
            backend = frame->GetCanvas()->GetBackend();
    }

    auto makeSide = [&]( wxStaticText** aLabel, WIDGET_DIFF_CANVAS** aCanvas )
    {
        wxPanel*    side = new wxPanel( m_canvasSplitter );
        wxBoxSizer* sizer = new wxBoxSizer( wxVERTICAL );

        *aLabel = new wxStaticText( side, wxID_ANY, wxEmptyString );
        *aCanvas = new WIDGET_DIFF_CANVAS( side, backend );

        sizer->Add( *aLabel, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 4 );
        sizer->Add( *aCanvas, 1, wxEXPAND );
        side->SetSizer( sizer );
        return side;
    };

    wxPanel* before = makeSide( &m_beforeLabel, &m_beforeCanvas );
    wxPanel* after = makeSide( &m_afterLabel, &m_afterCanvas );

    m_canvasSplitter->SplitVertically( before, after );

    // There is nothing to select here, so the left button pans like the middle one.
    m_beforeCanvas->SetLeftDragPans( true );
    m_afterCanvas->SetLeftDragPans( true );

    followCanvas( m_beforeCanvas, m_afterCanvas );
    followCanvas( m_afterCanvas, m_beforeCanvas );
}


void GIT_FILE_DIFF_AREA::destroyCanvases()
{
    // The documents come off the canvases before the canvases go.
    m_diffContext.reset();

    if( !m_beforeCanvas )
        return;

    // The splitter keeps its pane pointers through DestroyChildren, and its best size reads
    // them, so take the panes out of it first.
    m_canvasSplitter->Unsplit();
    m_canvasSplitter->Initialize( nullptr );
    m_canvasSplitter->DestroyChildren();
    m_beforeCanvas = nullptr;
    m_afterCanvas = nullptr;
    m_beforeLabel = nullptr;
    m_afterLabel = nullptr;
}


void GIT_FILE_DIFF_AREA::followCanvas( WIDGET_DIFF_CANVAS* aLead, WIDGET_DIFF_CANVAS* aFollower )
{
    // The view controls handle the event after this, so the copy waits until they have.
    auto onInput = [this, aLead, aFollower]( wxMouseEvent& aEvent )
    {
        aEvent.Skip();

        CallAfter(
                [this, aLead, aFollower]()
                {
                    // The canvases may have gone with a file change before this ran.
                    if( aLead != m_beforeCanvas && aLead != m_afterCanvas )
                        return;

                    KIGFX::VIEW* lead = aLead->GetView();
                    KIGFX::VIEW* follower = aFollower->GetView();

                    if( follower->GetScale() == lead->GetScale() && follower->GetCenter() == lead->GetCenter() )
                        return;

                    follower->SetScale( lead->GetScale() );
                    follower->SetCenter( lead->GetCenter() );
                    aFollower->Refresh();
                } );
    };

    aLead->Bind( wxEVT_MOTION, onInput );
    aLead->Bind( wxEVT_MOUSEWHEEL, onInput );
    aLead->Bind( wxEVT_MAGNIFY, onInput );
    aLead->Bind( wxEVT_LEFT_UP, onInput );
    aLead->Bind( wxEVT_MIDDLE_UP, onInput );
}
