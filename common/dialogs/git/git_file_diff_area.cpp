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

#include <fstream>
#include <optional>
#include <utility>

#include <wx/filename.h>
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

std::string readAll( const wxString& aFile )
{
    if( aFile.IsEmpty() )
        return std::string();

    std::ifstream in( aFile.fn_str(), std::ios::binary );
    return std::string( ( std::istreambuf_iterator<char>( in ) ), std::istreambuf_iterator<char>() );
}


// ShowSides diffs on the UI thread, so a larger file would stall it
constexpr unsigned long SIDES_TEXT_LIMIT = 1024 * 1024; // 1 MiB


bool tooLargeToDiffHere( const wxString& aFile )
{
    if( aFile.IsEmpty() )
        return false;

    const wxULongLong size = wxFileName::GetSize( aFile );

    return size != wxInvalidSize && size > SIDES_TEXT_LIMIT;
}


// False, with the error set, when either side is not plain text
bool diffTextSides( GIT_FILE_SIDES& aSides, std::vector<KIGIT::TEXT_DIFF_LINE>& aLines )
{
    std::string before = readAll( aSides.before );
    std::string after = readAll( aSides.after );

    if( KIGIT::TextLooksBinary( before ) || KIGIT::TextLooksBinary( after ) )
    {
        aSides.error = _( "This is not a plain text file, so there is no comparison to show." );
        return false;
    }

    aLines = KIGIT::BuildTextDiff( KIGIT::SplitTextLines( before ), KIGIT::SplitTextLines( after ) );
    return true;
}


// Reads every sheet a hierarchy reaches, so it runs on a worker where a slow share cannot stall the UI
void checkDrawingSize( GIT_FILE_SIDES& aSides )
{
    if( aSides.kind == KICAD_DIFF::DOC_KIND::SCH || aSides.kind == KICAD_DIFF::DOC_KIND::PCB )
    {
        aSides.tooLargeToPreview =
                KIGIT::DrawingTooLargeToPreview( aSides.before, aSides.after, aSides.kind == KICAD_DIFF::DOC_KIND::SCH,
                                                 aSides.beforeRoot, aSides.afterRoot );
    }
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
        m_beforeAbsent( nullptr ),
        m_afterAbsent( nullptr ),
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
    ++m_shown;
    m_query.Cancel();
    destroyCanvases();
}


void GIT_FILE_DIFF_AREA::ShowFile( const std::shared_ptr<KIGIT_COMMON>& aSession, const MATERIALIZE& aMaterialize,
                                   const wxString& aBeforeLabel, const wxString& aAfterLabel )
{
    ShowNote( _( "Loading comparison..." ) );

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

                if( result->sides.kind == KICAD_DIFF::DOC_KIND::UNKNOWN
                    && !diffTextSides( result->sides, result->textLines ) )
                {
                    return;
                }

                checkDrawingSize( result->sides );
                result->ready = true;
            },
            [this, result, aSession, aBeforeLabel, aAfterLabel]()
            {
                if( m_frame->Prj().GetGitCommon() != aSession )
                    return;

                if( !result->ready )
                {
                    ShowNote( result->sides.error );
                    return;
                }

                // Kiface loading and canvas creation must stay on the UI thread.
                showDiff( result->sides, result->textLines, aBeforeLabel, aAfterLabel );
            } );
}


void GIT_FILE_DIFF_AREA::ShowSides( const GIT_FILE_SIDES& aSides, const wxString& aBeforeLabel,
                                    const wxString& aAfterLabel, std::shared_ptr<void> aFiles,
                                    std::function<void()> aShown )
{
    // A git comparison still loading must not replace these sides when it lands
    m_query.Cancel();

    const unsigned shown = ++m_shown;

    GIT_FILE_SIDES                     sides = aSides;
    std::vector<KIGIT::TEXT_DIFF_LINE> textLines;

    if( sides.error.IsEmpty() && sides.kind == KICAD_DIFF::DOC_KIND::UNKNOWN )
    {
        if( tooLargeToDiffHere( sides.before ) || tooLargeToDiffHere( sides.after ) )
            sides.error = _( "This file is too large to preview." );
        else
            diffTextSides( sides, textLines );
    }

    const bool drawing = sides.kind == KICAD_DIFF::DOC_KIND::SCH || sides.kind == KICAD_DIFF::DOC_KIND::PCB;

    if( !sides.error.IsEmpty() || !drawing )
    {
        if( sides.error.IsEmpty() )
            showDiff( sides, textLines, aBeforeLabel, aAfterLabel );
        else
            ShowNote( sides.error );

        if( aShown )
            aShown();

        return;
    }

    auto checked = std::make_shared<GIT_FILE_SIDES>( sides );

    m_sizeCheck.Start(
            "Preview size check",
            [checked, aFiles]( const KIGIT_COMMON::CANCEL_TOKEN& )
            {
                // A failed check is doubt, and the completion must still run to show it
                try
                {
                    checkDrawingSize( *checked );
                }
                catch( const std::exception& )
                {
                    checked->tooLargeToPreview = true;
                }
            },
            // Holds the files until they are drawn, as the worker may drop its copy first
            [this, checked, shown, aBeforeLabel, aAfterLabel, aShown, aFiles]()
            {
                if( shown == m_shown )
                    showDiff( *checked, {}, aBeforeLabel, aAfterLabel );

                if( aShown )
                    aShown();
            } );
}


void GIT_FILE_DIFF_AREA::ShowNote( const wxString& aNote )
{
    ++m_shown;
    destroyCanvases();
    m_note->SetLabelText( aNote );
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

    const bool drawing = aSides.kind == KICAD_DIFF::DOC_KIND::SCH || aSides.kind == KICAD_DIFF::DOC_KIND::PCB;

    if( aSides.kind == KICAD_DIFF::DOC_KIND::UNKNOWN )
    {
        showTextDiff( aTextLines );
    }
    else if( drawing && aSides.tooLargeToPreview )
    {
        ShowNote( _( "This drawing is too large to preview. Double click the file to open its comparison." ) );
        shown = false;
    }
    else if( drawing )
    {
        shown = showDrawingDiff( aSides.kind, aSides.before, aSides.after, aBeforeLabel, aAfterLabel );
    }
    else
    {
        ShowNote( _( "Double click the file to open its comparison." ) );
    }

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
        ShowNote( reporter.HasMessage() ? reporter.GetMessages() : _( "Could not compare this file." ) );
        return false;
    }

    KICAD_DIFF::DIFF_COLOR_THEME theme;
    KICAD_DIFF::DIFF_SCENE       scene = KICAD_DIFF::BuildScene( diff, theme );
    scene.docKind = aKind;

    KICAD_DIFF::CHANGE_BOX_LOOKUP olderBox = [this]( const KIID_PATH& aChange ) -> std::optional<BOX2I>
    {
        return aChange.empty() ? std::nullopt : m_beforeCanvas->ContextItemBBox( aChange.back() );
    };

    // Both sides open on the same change. The user pans from there and the other side follows.
    std::optional<BOX2I> focus = KICAD_DIFF::SideBySideFocusBox( scene, olderBox );
    m_beforeCanvas->SetFocusBox( focus );
    m_afterCanvas->SetFocusBox( focus );

    KICAD_DIFF::SIDE_BY_SIDE_SCENES sides = KICAD_DIFF::SplitSceneBySide( scene, olderBox );

    m_beforeLabel->SetLabelText( aBeforeLabel );
    m_afterLabel->SetLabelText( aAfterLabel );
    m_beforeCanvas->SetScene( std::move( sides.reference ) );
    m_afterCanvas->SetScene( std::move( sides.comparison ) );

    // A side with no file has nothing for its canvas to frame, which then never paints
    auto showSide = []( WIDGET_DIFF_CANVAS* aCanvas, wxStaticText* aAbsent, bool aPresent )
    {
        aCanvas->Show( aPresent );
        aAbsent->Show( !aPresent );
        aCanvas->GetParent()->Layout();
    };

    showSide( m_beforeCanvas, m_beforeAbsent, !aBefore.IsEmpty() );
    showSide( m_afterCanvas, m_afterAbsent, !aAfter.IsEmpty() );

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

    auto makeSide = [&]( wxStaticText** aLabel, WIDGET_DIFF_CANVAS** aCanvas, wxStaticText** aAbsent )
    {
        wxPanel*    side = new wxPanel( m_canvasSplitter );
        wxBoxSizer* sizer = new wxBoxSizer( wxVERTICAL );

        *aLabel = new wxStaticText( side, wxID_ANY, wxEmptyString );
        *aCanvas = new WIDGET_DIFF_CANVAS( side, backend );
        *aAbsent = new wxStaticText( side, wxID_ANY, _( "The file is not in this version." ) );
        ( *aAbsent )->Hide();

        sizer->Add( *aLabel, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 4 );
        sizer->Add( *aCanvas, 1, wxEXPAND );
        sizer->Add( *aAbsent, 0, wxALIGN_CENTER_HORIZONTAL | wxALL, 8 );
        side->SetSizer( sizer );
        return side;
    };

    wxPanel* before = makeSide( &m_beforeLabel, &m_beforeCanvas, &m_beforeAbsent );
    wxPanel* after = makeSide( &m_afterLabel, &m_afterCanvas, &m_afterAbsent );

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
    m_beforeAbsent = nullptr;
    m_afterAbsent = nullptr;
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
