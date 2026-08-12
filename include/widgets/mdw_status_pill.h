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
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#ifndef MDW_STATUS_PILL_H
#define MDW_STATUS_PILL_H

#include <kicommon.h>
#include <mdw/client/mdw_types.h>
#include <mdw/mdw_claim_text.h>
#include <widgets/mdw_document_tracker.h>

#include <functional>
#include <memory>
#include <vector>

#include <wx/panel.h>
#include <wx/string.h>


/**
 * The Design Workspace status of the document a frame is showing, drawn as a colored pill in the status bar.
 * It names who is editing the document, its lifecycle state and whether the workspace is reachable, and opens
 * a menu of the actions that state allows. Hidden when the integration is off or the document is not managed.
 *
 * Metadata is fetched off the UI thread whenever the session or its events change, so the pill reads
 * "Checking" rather than guessing until the backend answers.
 */
class KICOMMON_API MDW_STATUS_PILL : public wxPanel
{
public:
    using STATE = MDW_DOCUMENT_TRACKER::STATE;

    MDW_STATUS_PILL( wxWindow* aParent );

    ~MDW_STATUS_PILL() override;

    /// Replace the displayed state and repaint, hiding the pill when not bound.
    void SetState( const STATE& aState );

    const STATE& GetState() const { return m_state; }

    /// The label shown for a state, aging a claim against aNow.
    static wxString LabelFor( const STATE& aState, const wxDateTime& aNow );

    /// The claim actions the menu offers, in menu order.
    static std::vector<MDW_CLAIM_ACTION> ActionsFor( const STATE& aState );

    /**
     * Point the pill at the project at aProjectPath and the document aDocument resolves, rereading it
     * whenever the project's connection changes, a source reports an event or the frame is activated.
     * Every hosting frame calls this on project open and close and when its document changes.
     */
    void TrackProject( const wxString& aProjectPath, bool aEnabled, MDW_DOCUMENT_TRACKER::DOCUMENT_RESOLVER aDocument );

    /// Adds "Show in Version Control" to the menu.
    void SetShowInVersionControl( std::function<void()> aHandler ) { m_showVersionControl = std::move( aHandler ); }

    /**
     * Track a field of the parent status bar, repositioning over it whenever the bar resizes. The pill's
     * parent must be that status bar, and the pill sets the field's width, zero while hidden.
     */
    void PlaceInStatusBar( int aFieldId );

    bool AcceptsFocus() const override { return IsShown() && m_state.bound; }
    bool AcceptsFocusFromKeyboard() const override { return AcceptsFocus(); }

private:
    /// The status bar field width that fits the pill's longest fixed label in aBar's font.
    static int FieldWidth( wxWindow* aBar );

    /// Size the tracked status bar field to the pill, or to nothing while it is hidden.
    void fitField();
    void runAction( const MDW_CLAIM_ACTION& aAction );
    void showMenu();

    /// Warn in the frame's infobar when the document opens under, or comes under, another user's claim.
    void announceClaim();

    void onPaint( wxPaintEvent& aEvent );
    void onKey( wxKeyEvent& aEvent );
    void onParentSize( wxSizeEvent& aEvent );
    void onActivate( wxActivateEvent& aEvent );

    void reposition();

    STATE                                 m_state;
    std::unique_ptr<MDW_DOCUMENT_TRACKER> m_tracker;  ///< Set by TrackProject
    wxWindow*                             m_topLevel; ///< Whose activation rereads the document
    std::function<void()>                 m_showVersionControl;
    int                                   m_statusFieldId; ///< -1 when not bar-hosted
    wxString                              m_announcedClaim; ///< The last foreign claim announced, or empty
};

#endif // MDW_STATUS_PILL_H
