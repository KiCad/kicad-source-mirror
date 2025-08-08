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

#include "dialogs/dialog_assign_netclass.h"

#include <wx/regex.h>

#include <widgets/wx_html_report_box.h>
#include <project.h>
#include <project/project_file.h>
#include <project/net_settings.h>
#include <eda_base_frame.h>
#include <string_utils.h>


wxString UpgradeGlobStarToRegex( const wxString& aPattern )
{
    // Match a '*' that is NOT preceded by '.'
    // (needs lookbehind, so no std::regex)
    static const wxRegEx globStarPattern( wxT( R"((?<!\.)\*)" ) );

    wxString result = aPattern;
    globStarPattern.ReplaceAll( &result, ".*" );
    return result;
}


static wxString GetStringCommonPrefix( const wxArrayString& aSet )
{
    if( aSet.empty() )
        return wxEmptyString;

    wxString commonPrefix = *aSet.begin();

    for( const wxString& str : aSet )
    {
        const size_t minLength = std::min( commonPrefix.size(), str.size() );
        size_t matchUntil = 0;
        while( matchUntil < minLength && commonPrefix[matchUntil] == str[matchUntil] )
        {
            ++matchUntil;
        }

        commonPrefix = commonPrefix.substr( 0, matchUntil );

        // If the common prefix is empty, we can stop early
        // as there is no common prefix.
        if( commonPrefix.empty() )
            break;
    }

    return commonPrefix;
}


/**
 * Propose a netclass pattern for a set of net names.
 *
 * This is a bit of a fudge, as there is no true answer for a perfect pattern.
 * (e.g. if you have A0 and A1, is the answer A*, A[0-9], A[0|1] or A0|A1, or
 * something else?)
 *
 * Also if you select A0, A1 and there is an A2 in the schematics,
 * is the answer to include A2 or exclude it? E.g. A*, A0|A1 are not the same.
 *
 * For now, we just glue the net names together with a pipe, handle the most basic
 * case of a single prefix if we can. No attempt is made to see if a star is
 * safe (i.e. the options given are all the options there are).
 */
static wxString GetNetclassPatternForSet( const std::set<wxString>& aNetNames )
{
    if( aNetNames.empty() )
        return wxEmptyString;

    if( aNetNames.size() == 1 )
    {
        return *aNetNames.begin();
    }

    wxArrayString netNames;
    for( const wxString& netName : aNetNames )
    {
        // If the net name contains a '*' (e..g it was a bus prefix),
        // we CAN use it in a pipe-separated regex pattern, but it has to be
        // upgraded from a glob star to a regex star.
        netNames.Add( UpgradeGlobStarToRegex( netName ) );
    }

    // Sort the net names to have a consistent order.
    StrNumSort( netNames, CASE_SENSITIVITY::INSENSITIVE );

    // Get the common prefix of all net names.
    const wxString commonPrefix = GetStringCommonPrefix( netNames );

    if( !commonPrefix.IsEmpty() && commonPrefix != wxT( "/" ) )
    {
        // If the common prefix is not empty, we can use it to simplify the pattern.
        // This only works for one prefix, but with tries or similar, we can find
        // multiple prefixes if that's something we want to do.

        wxArrayString netTails;

        for( const wxString& netName : netNames )
        {
            // Add the tail of the net name after the common prefix.
            netTails.Add( netName.Mid( commonPrefix.size() ) );
        }

        return commonPrefix + + wxT("(") + wxJoin( netTails, wxT( '|' ) ) + wxT(")");
    }

    // No better ideas, just a straight pipe-separated list of net names.
    return wxJoin( netNames, wxT( '|' ) );
}


DIALOG_ASSIGN_NETCLASS::DIALOG_ASSIGN_NETCLASS( EDA_BASE_FRAME* aParent,
                                                const std::set<wxString>& aNetNames,
                                                const std::set<wxString> aCandidateNetNames,
                                                const std::function<void( const std::vector<wxString>& )>& aPreviewer ) :
        DIALOG_ASSIGN_NETCLASS_BASE( aParent ),
        m_frame( aParent ),
        m_selectedNetNames( aNetNames ),
        m_netCandidates( aCandidateNetNames ),
        m_previewer( aPreviewer )
{
    m_matchingNets->SetFont( KIUI::GetInfoFont( this ) );
    m_info->SetFont( KIUI::GetInfoFont( this ).Italic() );

    // @translate the string below.
    if( aParent->GetFrameType() == FRAME_PCB_EDITOR )
        m_info->SetLabel( wxT( "Note: complete netclass assignments can be edited in Board "
                               "Setup > Project." ) );

    SetupStandardButtons();

    finishDialogSettings();
}


bool DIALOG_ASSIGN_NETCLASS::TransferDataToWindow()
{
    if( !wxWindow::TransferDataToWindow() )
        return false;

    std::shared_ptr<NET_SETTINGS>& netSettings = m_frame->Prj().GetProjectFile().m_NetSettings;

    m_netclassCtrl->Append( NETCLASS::Default );

    for( const auto& [name, netclass] : netSettings->GetNetclasses() )
        m_netclassCtrl->Append( name );

    if( m_netclassCtrl->GetCount() > 1 )
        m_netclassCtrl->SetSelection( 1 );  // First non-Default netclass
    else
        m_netclassCtrl->SetSelection( 0 );  // Default netclass

    const wxString initialNetclassPattern = GetNetclassPatternForSet( m_selectedNetNames );
    m_patternCtrl->SetValue( initialNetclassPattern );

    return true;
}


bool DIALOG_ASSIGN_NETCLASS::TransferDataFromWindow()
{
    std::shared_ptr<NET_SETTINGS>& netSettings = m_frame->Prj().GetProjectFile().m_NetSettings;

    if( m_patternCtrl->GetValue().IsEmpty() )
        return true;

    netSettings->SetNetclassPatternAssignment( m_patternCtrl->GetValue(),
                                               m_netclassCtrl->GetStringSelection() );

    return true;
}


void DIALOG_ASSIGN_NETCLASS::onPatternText( wxCommandEvent& aEvent )
{
    wxString pattern = m_patternCtrl->GetValue();

    if( pattern != m_lastPattern )
    {
        m_matchingNets->Clear();

        std::vector<wxString> matchingNetNames;

        if( !pattern.IsEmpty() )
        {
            EDA_COMBINED_MATCHER matcher( pattern, CTX_NETCLASS );

            m_matchingNets->Report( _( "<b>Currently matching nets:</b>" ) );

            for( const wxString& net : m_netCandidates )
            {
                if( matcher.StartsWith( net ) )
                {
                    m_matchingNets->Report( net );
                    matchingNetNames.push_back( net );
                }
            }
        }

        m_matchingNets->Flush();

        m_previewer( matchingNetNames );
        m_lastPattern = pattern;
    }
}
