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

#include <widgets/mdw_change_orders.h>

#include <mdw/client/mdw_source.h>
#include <string_utils.h>

#include <wx/intl.h>

#include <algorithm>
#include <iterator>
#include <mutex>
#include <utility>


namespace
{

/// Opaque ids are never shown, so an ECO or review without a display id goes by its title
wxString shownId( const std::string& aDisplayId, const std::string& aTitle )
{
    return wxString::FromUTF8( aDisplayId.empty() ? aTitle : aDisplayId );
}


wxString reviewersWith( const kiapi::mdw::Review& aReview, std::initializer_list<kiapi::mdw::ReviewState> aVerdicts )
{
    wxString names;

    for( const kiapi::mdw::Reviewer& reviewer : aReview.reviewers() )
    {
        if( std::find( aVerdicts.begin(), aVerdicts.end(), reviewer.verdict() ) == aVerdicts.end() )
            continue;

        AccumulateDescription( names, MdwReviewerName( reviewer ) );
    }

    return names;
}


/// Lower ranks win; cancelled reviews never show
int reviewRank( kiapi::mdw::ReviewState aState )
{
    switch( aState )
    {
    case kiapi::mdw::RS_APPROVED: return 0;
    case kiapi::mdw::RS_OPEN:
    case kiapi::mdw::RS_UNKNOWN: return 1;
    case kiapi::mdw::RS_CHANGES_REQUESTED:
    case kiapi::mdw::RS_REJECTED: return 2;
    default: break;
    }

    return -1;
}


wxString attributeRows( const kiapi::workspace::AttributeSet& aValues, const kiapi::workspace::Schema& aSchema )
{
    std::vector<kiapi::workspace::AttributeSchema> ordered( aSchema.attributes().begin(), aSchema.attributes().end() );

    std::stable_sort( ordered.begin(), ordered.end(),
                      []( const kiapi::workspace::AttributeSchema& aLhs, const kiapi::workspace::AttributeSchema& aRhs )
                      {
                          return aLhs.display_order() < aRhs.display_order();
                      } );

    wxString html;

    for( const kiapi::workspace::AttributeSchema& attr : ordered )
    {
        for( const kiapi::workspace::AttributeValue& value : aValues.values() )
        {
            if( value.attr_id() == attr.attr_id() && !value.value().empty() )
            {
                html << wxS( "<tr><td><b>" ) << EscapeHTML( wxString::FromUTF8( attr.display_name() ) )
                     << wxS( "</b></td><td>" ) << EscapeHTML( wxString::FromUTF8( value.value() ) )
                     << wxS( "</td></tr>" );
            }
        }
    }

    return html;
}


wxString row( const wxString& aLabel, const wxString& aValue )
{
    if( aValue.IsEmpty() )
        return wxEmptyString;

    return wxS( "<tr><td><b>" ) + EscapeHTML( aLabel ) + wxS( "</b></td><td>" ) + EscapeHTML( aValue )
           + wxS( "</td></tr>" );
}


wxString row( const wxString& aLabel, const std::string& aValue )
{
    return row( aLabel, wxString::FromUTF8( aValue ) );
}


wxString browserLink( const std::string& aUrl )
{
    if( aUrl.empty() )
        return wxEmptyString;

    return wxS( "<p><a href=\"" ) + EscapeHTML( wxString::FromUTF8( aUrl ) ) + wxS( "\">" )
           + EscapeHTML( _( "Open in Browser" ) ) + wxS( "</a></p>" );
}

} // namespace


wxString MdwReviewerName( const kiapi::mdw::Reviewer& aReviewer )
{
    return aReviewer.display_name().empty() ? wxString( _( "Unnamed reviewer" ) )
                                            : wxString::FromUTF8( aReviewer.display_name() );
}


MDW_CHANGE_ORDERS_SUBJECT MdwDesignSubject( const wxString& aProjectName, const MDW_PROJECT_STATE& aState,
                                            MDW_CHANGE_ORDERS_PAGE aPage )
{
    MDW_CHANGE_ORDERS_SUBJECT subject;
    subject.name = aProjectName;
    subject.eco_items = { aState.item };
    subject.review_item = aState.item;
    subject.revision_id = aState.base_revision_id;
    subject.page = aPage;
    return subject;
}


bool MdwEcoIsOpen( kiapi::mdw::EcoState aState )
{
    return aState != kiapi::mdw::ECO_APPLIED && aState != kiapi::mdw::ECO_REJECTED
           && aState != kiapi::mdw::ECO_CANCELLED;
}


std::vector<kiapi::mdw::Eco> MdwOpenEcos( const std::vector<kiapi::mdw::Eco>& aEcos )
{
    std::vector<kiapi::mdw::Eco> open;

    std::copy_if( aEcos.begin(), aEcos.end(), std::back_inserter( open ),
                  []( const kiapi::mdw::Eco& aEco )
                  {
                      return MdwEcoIsOpen( aEco.state_category() );
                  } );

    return open;
}


wxString MdwEcoName( const kiapi::mdw::Eco& aEco )
{
    return shownId( aEco.display_id(), aEco.title() );
}


wxString MdwDesignEcoNote( const std::vector<kiapi::mdw::Eco>& aOpenEcos )
{
    if( aOpenEcos.empty() )
        return wxEmptyString;

    wxString ids;

    for( const kiapi::mdw::Eco& eco : aOpenEcos )
        AccumulateDescription( ids, MdwEcoName( eco ) );

    return wxString::Format( _( "Open change orders on this design: %s." ), ids );
}


const kiapi::mdw::Review* MdwPickReview( const std::vector<kiapi::mdw::Review>& aReviews, const wxString& aRevisionId )
{
    const kiapi::mdw::Review* best = nullptr;

    for( const kiapi::mdw::Review& review : aReviews )
    {
        int rank = reviewRank( review.state_category() );

        if( rank < 0 || ( !aRevisionId.IsEmpty() && review.revision_id() != aRevisionId.utf8_string() ) )
            continue;

        if( !best || rank < reviewRank( best->state_category() ) )
            best = &review;
    }

    return best;
}


std::vector<kiapi::mdw::Review> MdwReviewsOfRevision( const std::vector<kiapi::mdw::Review>& aReviews,
                                                      const wxString&                        aRevisionId )
{
    std::vector<kiapi::mdw::Review> kept;

    std::copy_if( aReviews.begin(), aReviews.end(), std::back_inserter( kept ),
                  [&]( const kiapi::mdw::Review& aReview )
                  {
                      return aRevisionId.IsEmpty() || aReview.revision_id() == aRevisionId.utf8_string();
                  } );

    return kept;
}


const kiapi::mdw::Review* MdwShownReview( const std::vector<kiapi::mdw::Review>& aReviews, const wxString& aRevisionId )
{
    if( const kiapi::mdw::Review* review = MdwPickReview( aReviews, aRevisionId ) )
        return review;

    if( aRevisionId.IsEmpty() )
        return nullptr;

    // An approval without a revision may cover this one, so it wins over another revision's
    for( const kiapi::mdw::Review& review : aReviews )
    {
        if( review.state_category() == kiapi::mdw::RS_APPROVED && review.revision_id().empty() )
            return &review;
    }

    for( const kiapi::mdw::Review& review : aReviews )
    {
        if( review.state_category() == kiapi::mdw::RS_APPROVED )
            return &review;
    }

    return nullptr;
}


wxString MdwReviewStatusLine( const std::vector<kiapi::mdw::Review>& aReviews, const wxString& aRevisionId,
                              const wxString& aSourceName )
{
    const kiapi::mdw::Review* review = MdwShownReview( aReviews, aRevisionId );

    if( !review )
        return aRevisionId.IsEmpty() ? _( "No reviews." ) : _( "No review for this revision." );

    const wxString id = shownId( review->display_id(), review->title() );

    if( !aRevisionId.IsEmpty() && review->revision_id() != aRevisionId.utf8_string() )
    {
        if( review->revision_id().empty() )
        {
            return wxString::Format( _( "Review %s is approved, but %s does not say which revision it covers." ), id,
                                     aSourceName );
        }

        return wxString::Format( _( "Approved for another revision (%s). This revision has no approved review." ), id );
    }

    wxString names;

    switch( review->state_category() )
    {
    case kiapi::mdw::RS_APPROVED:
        names = reviewersWith( *review, { kiapi::mdw::RS_APPROVED } );

        return names.IsEmpty() ? wxString::Format( _( "Review: Approved (%s)." ), id )
                               : wxString::Format( _( "Review: Approved by %s (%s)." ), names, id );

    case kiapi::mdw::RS_CHANGES_REQUESTED:
        names = reviewersWith( *review, { kiapi::mdw::RS_CHANGES_REQUESTED } );

        return names.IsEmpty() ? wxString::Format( _( "Review: Changes requested (%s)." ), id )
                               : wxString::Format( _( "Review: Changes requested by %s (%s)." ), names, id );

    case kiapi::mdw::RS_REJECTED:
        names = reviewersWith( *review, { kiapi::mdw::RS_REJECTED } );

        return names.IsEmpty() ? wxString::Format( _( "Review: Rejected (%s)." ), id )
                               : wxString::Format( _( "Review: Rejected by %s (%s)." ), names, id );

    default:
        names = reviewersWith( *review, { kiapi::mdw::RS_OPEN, kiapi::mdw::RS_UNKNOWN } );

        return names.IsEmpty() ? wxString::Format( _( "Review: Open (%s)." ), id )
                               : wxString::Format( _( "Review: Waiting for %s (%s)." ), names, id );
    }
}


wxString MdwEcoDetailsHtml( const kiapi::mdw::Eco& aEco, const kiapi::workspace::Schema& aSchema )
{
    wxString affected;

    for( const kiapi::workspace::ItemRef& item : aEco.affected_items() )
    {
        if( !item.path_hint().empty() )
            AccumulateDescription( affected, wxString::FromUTF8( item.path_hint() ) );
    }

    wxString html = wxS( "<h3>" ) + EscapeHTML( wxString::FromUTF8( aEco.display_id() ) ) + wxS( " " )
                    + EscapeHTML( wxString::FromUTF8( aEco.title() ) ) + wxS( "</h3><table>" );

    html << row( _( "Type" ), aEco.eco_type() ) << row( _( "State" ), aEco.state() )
         << row( _( "Author" ), aEco.author() ) << row( _( "Affects" ), affected )
         << attributeRows( aEco.attributes(), aSchema ) << wxS( "</table>" ) << browserLink( aEco.url() );

    return html;
}


wxString MdwReviewDetailsHtml( const kiapi::mdw::Review& aReview )
{
    wxString html = wxS( "<h3>" ) + EscapeHTML( wxString::FromUTF8( aReview.display_id() ) ) + wxS( " " )
                    + EscapeHTML( wxString::FromUTF8( aReview.title() ) ) + wxS( "</h3><table>" );

    html << row( _( "State" ), aReview.state() ) << row( _( "Author" ), aReview.author() );

    for( const kiapi::mdw::Reviewer& reviewer : aReview.reviewers() )
    {
        wxString verdict;

        switch( reviewer.verdict() )
        {
        case kiapi::mdw::RS_APPROVED: verdict = _( "Approved" ); break;
        case kiapi::mdw::RS_CHANGES_REQUESTED: verdict = _( "Changes requested" ); break;
        case kiapi::mdw::RS_REJECTED: verdict = _( "Rejected" ); break;
        case kiapi::mdw::RS_CANCELLED: verdict = _( "Withdrawn" ); break;
        default: verdict = _( "Waiting" ); break;
        }

        if( reviewer.required() )
            verdict << wxS( " " ) << _( "(required)" );

        html << row( MdwReviewerName( reviewer ), verdict );
    }

    html << wxS( "</table>" ) << browserLink( aReview.url() );

    return html;
}
