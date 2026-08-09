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

#include "mdw_baseline_design.h"

#include <mdw/client/mdw_client.h>
#include <mdw/client/mdw_paged_read.h>
#include <mdw/client/mdw_source.h>

#include "mdw_op_state.h"

#include <algorithm>
#include <atomic>
#include <iterator>
#include <map>
#include <mutex>
#include <set>
#include <string_utils.h>
#include <tuple>

#include <wx/intl.h>


namespace
{

MDW_PART_FACTS factsOf( kiapi::mdw::PlacedPartStatus aStatus, const wxString& aWhy )
{
    MDW_PART_FACTS facts;
    facts.statuses.push_back( aStatus );
    facts.message = aWhy;
    return facts;
}


wxString sourceName( const std::shared_ptr<MDW_SOURCE>& aSource, const wxString& aLocator )
{
    wxString name = aSource ? aSource->Info().display_name : wxString();
    return name.IsEmpty() ? aLocator : name;
}


/// The child ops behind one composite op. Only pending child callbacks own a run, so a cancelled or
/// finished run frees itself; the composite's canceller holds it weakly
struct OP_FAN
{
    std::shared_ptr<MDW_OP_STATE> state = std::make_shared<MDW_OP_STATE>();
    std::atomic<size_t>           waiting{ 0 };

    std::mutex          mutex;
    std::vector<MDW_OP> ops;
    bool                cancelled = false;

    void Keep( MDW_OP aOp )
    {
        std::lock_guard<std::mutex> lock( mutex );

        if( cancelled )
            return;

        ops.push_back( std::move( aOp ) );
    }

    void Cancel()
    {
        std::vector<MDW_OP> stale;

        {
            std::lock_guard<std::mutex> lock( mutex );
            cancelled = true;
            stale.swap( ops );
        }

        // Destroying an op waits for its running callback, which may itself want the mutex
        stale.clear();

        // The children's callbacks are gone, so nothing else would ever retire the composite
        state->Complete( {} );
    }
};


template <typename RUN>
MDW_OP composite( const std::shared_ptr<RUN>& aRun )
{
    aRun->state->SetCanceller(
            [weak = std::weak_ptr<RUN>( aRun )]()
            {
                if( std::shared_ptr<RUN> live = weak.lock() )
                    live->Cancel();
            } );

    return MDW_OP( aRun->state );
}


/// Owner calls run first, and the project's check once the last one answers
struct DESIGN_CHECK_RUN : OP_FAN, std::enable_shared_from_this<DESIGN_CHECK_RUN>
{
    std::shared_ptr<MDW_SOURCE> project;
    MDW_ITEM_REF                projectRef;
    wxString                    revisionId;
    MDW_DESIGN_CHECK            result;

    void OwnerAnswered()
    {
        if( --waiting == 0 )
            StartProjectCheck();
    }

    void StartProjectCheck()
    {
        std::vector<MDW_BASELINE_PART> parts;

        for( const MDW_DESIGN_PART& part : result.parts )
            parts.push_back( part.baseline );

        Keep( project->Baselines().Check(
                projectRef, revisionId, parts,
                [self = shared_from_this()]( const MDW_RESULT& aResult, const MDW_BASELINE_VERDICT& aVerdict )
                {
                    self->result.checks = aVerdict.checks;
                    self->result.hold = aVerdict.hold;
                    self->result.holder = aVerdict.holder;
                    self->Finish( aResult );
                } ) );
    }

    void Finish( const MDW_RESULT& aResult )
    {
        state->Answer( aResult, aResult.IsOk() ? result : MDW_DESIGN_CHECK() );
    }
};


using PART_KEY =
        std::tuple<MDW_PLACED_KIND, wxString, wxString, wxString, wxString, wxString, wxString, wxString, wxString>;


PART_KEY partKey( const MDW_DESIGN_PART& aPart )
{
    const MDW_BASELINE_PART& baseline = aPart.baseline;

    // Without a member the library id is all that tells two unmanaged parts apart
    return PART_KEY{ aPart.kind,
                     baseline.item.source,
                     baseline.item.item_id,
                     baseline.item.item_id.IsEmpty() ? baseline.lib_id : wxString(),
                     aPart.pin_label,
                     baseline.content_digest,
                     baseline.part.source,
                     baseline.part.item_id,
                     baseline.part_revision_id };
}

} // namespace


std::vector<MDW_DESIGN_PART> MdwGroupPlacedParts( const std::vector<MDW_PLACED_INPUT>& aInputs )
{
    std::vector<MDW_DESIGN_PART> parts;
    std::map<PART_KEY, size_t>   index;

    for( const MDW_PLACED_INPUT& input : aInputs )
    {
        MDW_DESIGN_PART part;
        part.kind = input.kind;
        part.pin_label = input.pin_label;
        part.team_source = input.team_source;
        part.team_locator = input.team_locator;
        part.baseline.item = input.member.value_or( MDW_ITEM_REF() );
        part.baseline.lib_id = input.lib_id;
        part.baseline.content_digest = input.content_digest;
        part.baseline.part = input.part;
        part.baseline.part_revision_id = input.part_revision_id;

        auto [it, inserted] = index.emplace( partKey( part ), parts.size() );

        if( inserted )
            parts.push_back( std::move( part ) );

        std::vector<wxString>& references = parts[it->second].baseline.references;

        // A multi-unit symbol reports its reference once per unit
        if( std::find( references.begin(), references.end(), input.reference ) == references.end() )
            references.push_back( input.reference );
    }

    // Every reader of a release, its comment included, lists designators in natural order
    for( MDW_DESIGN_PART& part : parts )
    {
        std::sort( part.baseline.references.begin(), part.baseline.references.end(),
                   []( const wxString& aA, const wxString& aB )
                   {
                       return StrNumCmp( aA, aB, CMP_T::IGNORE_CASE ) < 0;
                   } );
    }

    return parts;
}


std::vector<MDW_BASELINE_EXCEPTION> MdwCarryExceptions( const std::vector<MDW_DESIGN_PART>&        aOld,
                                                        const std::vector<MDW_BASELINE_EXCEPTION>& aExceptions,
                                                        const std::vector<MDW_DESIGN_PART>&        aNew )
{
    std::map<PART_KEY, unsigned> index;

    for( unsigned i = 0; i < aNew.size(); ++i )
        index.emplace( partKey( aNew[i] ), i );

    std::vector<MDW_BASELINE_EXCEPTION> carried;

    for( const MDW_BASELINE_EXCEPTION& exception : aExceptions )
    {
        if( exception.part_index >= aOld.size() )
            continue;

        auto it = index.find( partKey( aOld[exception.part_index] ) );

        if( it == index.end() )
            continue;

        carried.push_back( exception );
        carried.back().part_index = it->second;
    }

    return carried;
}


MDW_OP MdwCheckDesign( const std::shared_ptr<MDW_SOURCE>& aProject, const MDW_ITEM_REF& aProjectRef,
                       const wxString& aRevisionId, std::vector<MDW_DESIGN_PART> aParts,
                       MDW_DONE<MDW_DESIGN_CHECK> aDone, MDW_SOURCE_LOOKUP aLookup )
{
    if( !aProject )
        return MdwNotSupported<MDW_DESIGN_CHECK>( std::move( aDone ) );

    if( !aLookup )
    {
        aLookup = []( const wxString& aLocator )
        {
            return MDW_CLIENT::Get().FindSource( aLocator );
        };
    }

    auto run = std::make_shared<DESIGN_CHECK_RUN>();
    run->project = aProject;
    run->projectRef = aProjectRef;
    run->revisionId = aRevisionId;
    run->result.parts = std::move( aParts );
    run->state->Hold<MDW_DESIGN_CHECK>( std::move( aDone ) );

    MDW_OP op = composite( run );

    const wxString                          projectLocator = aProject->Info().locator;
    std::map<wxString, std::vector<size_t>> byOwner;

    for( size_t i = 0; i < run->result.parts.size(); ++i )
    {
        MDW_BASELINE_PART& part = run->result.parts[i].baseline;
        const wxString&    teamSource = run->result.parts[i].team_source;
        const wxString&    teamLocator = run->result.parts[i].team_locator;

        // The project backend echoes owner facts it is given, so facts from an earlier check would stick
        part.owner_facts.reset();

        // A team part its source cannot vouch for here may never have been released, so it blocks
        if( part.item.item_id.IsEmpty() && !teamSource.IsEmpty() )
        {
            std::shared_ptr<MDW_SOURCE> owner = teamLocator.IsEmpty() ? nullptr : aLookup( teamLocator );
            wxString                    why;

            if( !owner )
            {
                why = wxString::Format( _( "'%s' is not set up on this computer. Connect it, then open Release "
                                           "Design again" ),
                                        teamSource );
            }
            else if( owner->Info().backend.backend_id.IsEmpty() )
            {
                why = wxString::Format( _( "'%s' is not connected. Connect it, then open Release Design again" ),
                                        teamSource );
            }
            else
            {
                why = wxString::Format( _( "'%s' never released this part" ), teamSource );
            }

            part.owner_facts = factsOf( kiapi::mdw::PPS_UNVERIFIED, why );
        }
        else if( part.item.item_id.IsEmpty() )
        {
            part.owner_facts = factsOf( kiapi::mdw::PPS_UNKNOWN, _( "Not from a team library" ) );
        }
        else if( part.item.source != projectLocator )
        {
            byOwner[part.item.source].push_back( i );
        }
    }

    struct ASK
    {
        std::shared_ptr<MDW_SOURCE>    source;
        std::vector<size_t>            indices;
        std::vector<MDW_BASELINE_PART> parts;
    };

    std::vector<ASK> asks;

    for( auto& [locator, indices] : byOwner )
    {
        std::shared_ptr<MDW_SOURCE> owner = aLookup( locator );
        std::optional<MDW_PART_FACTS> facts;

        // A member nobody could vouch for blocks, since it may never have been released
        if( !owner )
        {
            facts = factsOf( kiapi::mdw::PPS_UNVERIFIED,
                             wxString::Format( _( "%s is not set up on this computer" ), locator ) );
        }
        else if( owner->Info().backend.backend_id.IsEmpty() )
        {
            facts = factsOf( kiapi::mdw::PPS_UNVERIFIED, wxString::Format( _( "%s is not connected" ), locator ) );
        }
        else if( !owner->Has( MDW_CAPABILITY_GROUP::REVISIONS ) )
        {
            facts = factsOf( kiapi::mdw::PPS_UNKNOWN,
                             wxString::Format( _( "%s does not track releases" ), sourceName( owner, locator ) ) );
        }

        if( facts )
        {
            for( size_t i : indices )
                run->result.parts[i].baseline.owner_facts = facts;
        }
        else
        {
            ASK& ask = asks.emplace_back( ASK{ owner, indices, {} } );

            for( size_t i : indices )
                ask.parts.push_back( run->result.parts[i].baseline );
        }
    }

    run->waiting = asks.size();

    if( asks.empty() )
        run->StartProjectCheck();

    // Under worker delivery an answer may land before the next ask starts, so nothing below reads run->result
    for( ASK& ask : asks )
    {
        run->Keep( ask.source->Baselines().Facts(
                ask.parts,
                [live = run, indices = ask.indices, name = sourceName( ask.source, ask.source->Info().locator )](
                        const MDW_RESULT& aResult, const std::vector<MDW_PART_FACTS>& aFacts )
                {
                    for( size_t k = 0; k < indices.size(); ++k )
                    {
                        MDW_BASELINE_PART& part = live->result.parts[indices[k]].baseline;

                        if( aResult.IsOk() )
                        {
                            part.owner_facts = aFacts[k];
                        }
                        else
                        {
                            wxString why = MdwErrorText( aResult );
                            part.owner_facts =
                                    factsOf( kiapi::mdw::PPS_UNVERIFIED,
                                             wxString::Format( _( "%s could not be checked: %s" ), name, why ) );
                        }
                    }

                    live->OwnerAnswered();
                } ) );
    }

    return op;
}


wxString MdwPlacedStatusText( const MDW_PART_FACTS& aFacts )
{
    const wxString& matched = aFacts.matched_revision_label;
    const wxString& latest = aFacts.latest_revision_label;
    wxArrayString   parts;

    for( kiapi::mdw::PlacedPartStatus status : aFacts.statuses )
    {
        switch( status )
        {
        case kiapi::mdw::PPS_UNKNOWN:
        case kiapi::mdw::PPS_UNVERIFIED:
        default: parts.Add( aFacts.message.IsEmpty() ? _( "Not checked" ) : aFacts.message ); break;

        case kiapi::mdw::PPS_RELEASED: parts.Add( wxString::Format( _( "Released, revision %s" ), matched ) ); break;

        case kiapi::mdw::PPS_NEVER_RELEASED: parts.Add( _( "Never released" ) ); break;

        case kiapi::mdw::PPS_DIFFERS:
            parts.Add( latest.IsEmpty() ? _( "Differs from every release" )
                                        : wxString::Format( _( "Differs from revision %s" ), latest ) );
            break;

        case kiapi::mdw::PPS_OUTDATED:
            parts.Add( wxString::Format( _( "Revision %s, newer revision %s exists" ), matched, latest ) );
            break;

        case kiapi::mdw::PPS_NOT_USABLE:
            parts.Add( wxString::Format( _( "Revision %s may not be used in designs" ), matched ) );
            break;

        case kiapi::mdw::PPS_UNPUBLISHED:
            parts.Add( wxString::Format( _( "Revision %s not published yet" ), matched ) );
            break;

        case kiapi::mdw::PPS_NOT_FOR_NEW_DESIGNS:
            parts.Add( wxString::Format( _( "Revision %s is not for new designs" ), matched ) );
            break;
        }
    }

    if( parts.IsEmpty() )
        return aFacts.message.IsEmpty() ? _( "Not checked" ) : aFacts.message;

    wxString text = parts[0];

    for( size_t i = 1; i < parts.size(); ++i )
        text << wxS( "; " ) << parts[i];

    return text;
}


bool MdwReviewIsOf( const kiapi::mdw::Review& aReview, const MDW_ITEM_REF& aItem )
{
    const kiapi::workspace::ItemRef& item = aReview.item();

    // An empty source is the answering source's own item
    return !item.item_id().empty() && item.item_id() == aItem.item_id.utf8_string()
           && ( item.source().empty() || item.source() == aItem.source.utf8_string() );
}


std::vector<kiapi::mdw::Review> MdwReviewsOf( const kiapi::mdw::ListReviewsReply& aList, const MDW_ITEM_REF& aItem )
{
    std::vector<kiapi::mdw::Review> reviews;

    std::copy_if( aList.reviews().begin(), aList.reviews().end(), std::back_inserter( reviews ),
                  [&]( const kiapi::mdw::Review& aReview )
                  {
                      return MdwReviewIsOf( aReview, aItem );
                  } );

    return reviews;
}


MDW_REVIEW_GATE MdwReviewGate( const kiapi::mdw::ListReviewsReply& aList, const MDW_ITEM_REF& aItem,
                               const wxString& aRevisionId )
{
    MDW_REVIEW_GATE gate;

    // An unstated revision on either side never proves approval of the one being released
    gate.approved = !aRevisionId.IsEmpty()
                    && std::any_of( aList.reviews().begin(), aList.reviews().end(),
                                    [&]( const kiapi::mdw::Review& aReview )
                                    {
                                        return aReview.state_category() == kiapi::mdw::RS_APPROVED
                                               && aReview.revision_id() == aRevisionId.utf8_string()
                                               && MdwReviewIsOf( aReview, aItem );
                                    } );

    gate.blocks = aList.approval_required() && !gate.approved;
    return gate;
}


namespace
{

bool sameItem( const MDW_ITEM_REF& aA, const MDW_ITEM_REF& aB )
{
    return !aA.item_id.IsEmpty() && aA.item_id == aB.item_id && aA.source == aB.source;
}


bool sameItem( const kiapi::workspace::ItemRef& aA, const MDW_ITEM_REF& aB )
{
    return !aA.item_id().empty() && aA.item_id() == aB.item_id.utf8_string() && aA.source() == aB.source.utf8_string();
}


/// Every page of each source's answer, then a match of each change order against the design
struct ECO_RUN : OP_FAN, std::enable_shared_from_this<ECO_RUN>
{
    std::vector<MDW_ITEM_REF>    items;
    MDW_ITEM_REF                 projectRef;
    std::vector<MDW_DESIGN_PART> parts;
    MDW_DESIGN_ECOS              result;
    size_t                       failed = 0;
    size_t                       asked = 0;
    MDW_RESULT                   lastFailure;

    // Each source pages on its own, so a failing one leaves the others' answers standing
    void Ask( const std::shared_ptr<MDW_SOURCE>& aSource )
    {
        Keep( MdwReadAllPages<kiapi::mdw::Eco>(
                [aSource, ecoItems = items]( const MDW_PAGE& aPage, MDW_DONE<MDW_PAGED<kiapi::mdw::Eco>> aDone )
                {
                    return aSource->Ecos().List( ecoItems, false, aPage, std::move( aDone ) );
                },
                200,
                [self = shared_from_this(), aSource]( const MDW_RESULT&                 aResult,
                                                      const MDW_PAGED<kiapi::mdw::Eco>& aRead )
                {
                    self->Answered( aSource, aResult, aRead );
                } ) );
    }

    void Answered( const std::shared_ptr<MDW_SOURCE>& aSource, const MDW_RESULT& aResult,
                   const MDW_PAGED<kiapi::mdw::Eco>& aPage )
    {
        const MDW_SOURCE_INFO info = aSource->Info();

        if( !aResult.IsOk() )
        {
            ++failed;
            lastFailure = aResult;

            wxString why = MdwErrorText( aResult );
            result.failures.push_back( wxString::Format( wxS( "%s: %s" ), sourceName( aSource, info.locator ), why ) );
        }
        else
        {
            for( const kiapi::mdw::Eco& eco : aPage.rows )
                add( info.locator, eco );
        }

        if( --waiting == 0 )
            Finish();
    }

    void add( const wxString& aSource, const kiapi::mdw::Eco& aEco )
    {
        for( const MDW_SOURCED_ECO& known : result.ecos )
        {
            if( known.source == aSource && known.eco.eco_id() == aEco.eco_id() )
                return;
        }

        const size_t index = result.ecos.size();
        bool         design = false;

        for( const kiapi::workspace::ItemRef& affected : aEco.affected_items() )
        {
            design |= sameItem( affected, projectRef );

            for( size_t i = 0; i < parts.size(); ++i )
            {
                const MDW_BASELINE_PART& part = parts[i].baseline;
                std::vector<size_t>&     onPart = result.parts[i];

                if( ( sameItem( affected, part.item ) || sameItem( affected, part.part ) )
                    && ( onPart.empty() || onPart.back() != index ) )
                {
                    onPart.push_back( index );
                }
            }
        }

        if( design )
            result.design.push_back( index );

        result.ecos.push_back( { aSource, aEco } );
    }

    void Finish()
    {
        MDW_RESULT outcome = failed == asked ? lastFailure : MDW_RESULT::Ok();

        state->Answer( outcome, outcome.IsOk() ? result : MDW_DESIGN_ECOS() );
    }
};


struct REVIEW_RUN : OP_FAN, std::enable_shared_from_this<REVIEW_RUN>
{
    std::shared_ptr<MDW_SOURCE>  source;
    MDW_ITEM_REF                 item;
    wxString                     revisionId;
    MDW_READ_CAP                 cap = MDW_READ_CAP::FAIL;
    bool                         unattributed = false;

    void Ask( const wxString& aRevisionId )
    {
        auto approvalRequired = std::make_shared<bool>( false );

        Keep( MdwReadAllPages<kiapi::mdw::Review>(
                [src = source, ref = item, aRevisionId,
                 approvalRequired]( const MDW_PAGE& aPage, MDW_DONE<MDW_PAGED<kiapi::mdw::Review>> aPageDone )
                {
                    return src->Reviews().List(
                            ref, aRevisionId, aPage,
                            [approvalRequired, aPageDone = std::move( aPageDone )](
                                    const MDW_RESULT& aResult, const kiapi::mdw::ListReviewsReply& aList )
                            {
                                *approvalRequired |= aList.approval_required();
                                aPageDone( aResult, { { aList.reviews().begin(), aList.reviews().end() },
                                                      wxString::FromUTF8( aList.next_page_token() ),
                                                      std::nullopt } );
                            } );
                },
                50,
                [self = shared_from_this(), approvalRequired]( const MDW_RESULT&                    aResult,
                                                               const MDW_PAGED<kiapi::mdw::Review>& aRead )
                {
                    self->Answered( aResult, aRead, *approvalRequired );
                },
                cap ) );
    }

    void Answered( const MDW_RESULT& aResult, const MDW_PAGED<kiapi::mdw::Review>& aRead, bool aApprovalRequired )
    {
        // A backend that cannot attribute reviews to revisions refuses the filter
        if( aResult.error == MDW_ERROR_CODE::NOT_SUPPORTED && !revisionId.IsEmpty() && !unattributed )
        {
            unattributed = true;
            Ask( wxEmptyString );
            return;
        }

        MDW_REVIEW_READ read;
        read.reviews = aRead.rows;
        read.approval_required = aApprovalRequired;
        read.partial = !aRead.next_page_token.IsEmpty();
        read.unattributed = unattributed;

        state->Answer( aResult, read );
    }
};

} // namespace


MDW_OP MdwListDesignEcos( const std::vector<std::shared_ptr<MDW_SOURCE>>& aSources, const MDW_ITEM_REF& aProjectRef,
                          const std::vector<MDW_DESIGN_PART>& aParts, MDW_DONE<MDW_DESIGN_ECOS> aDone )
{
    std::vector<std::shared_ptr<MDW_SOURCE>> asking;

    for( const std::shared_ptr<MDW_SOURCE>& source : aSources )
    {
        if( source && source->Has( MDW_CAPABILITY_GROUP::ECO ) )
            asking.push_back( source );
    }

    if( asking.empty() )
        return MdwNotSupported<MDW_DESIGN_ECOS>( std::move( aDone ) );

    auto run = std::make_shared<ECO_RUN>();
    run->projectRef = aProjectRef;
    run->parts = aParts;
    run->result.parts.resize( aParts.size() );
    run->state->Hold<MDW_DESIGN_ECOS>( std::move( aDone ) );
    run->items.push_back( aProjectRef );

    for( const MDW_DESIGN_PART& part : aParts )
    {
        for( const MDW_ITEM_REF& ref : { part.baseline.item, part.baseline.part } )
        {
            if( ref.item_id.IsEmpty() )
                continue;

            if( std::none_of( run->items.begin(), run->items.end(),
                              [&]( const MDW_ITEM_REF& aKnown )
                              {
                                  return sameItem( aKnown, ref );
                              } ) )
            {
                run->items.push_back( ref );
            }
        }
    }

    run->asked = asking.size();
    run->waiting = asking.size();

    MDW_OP op = composite( run );

    for( const std::shared_ptr<MDW_SOURCE>& source : asking )
        run->Ask( source );

    return op;
}


MDW_OP MdwReadReviews( const std::shared_ptr<MDW_SOURCE>& aSource, const MDW_ITEM_REF& aItem,
                       const wxString& aRevisionId, MDW_DONE<MDW_REVIEW_READ> aDone, MDW_READ_CAP aCap )
{
    if( !aSource || !aSource->Has( MDW_CAPABILITY_GROUP::REVIEW ) )
        return MdwNotSupported<MDW_REVIEW_READ>( std::move( aDone ) );

    auto run = std::make_shared<REVIEW_RUN>();
    run->source = aSource;
    run->item = aItem;
    run->revisionId = aRevisionId;
    run->state->Hold<MDW_REVIEW_READ>( std::move( aDone ) );
    run->cap = aCap;

    MDW_OP op = composite( run );
    run->Ask( aRevisionId );
    return op;
}


MDW_OP MdwListDesignReviews( const std::shared_ptr<MDW_SOURCE>& aSource, const MDW_ITEM_REF& aItem,
                             MDW_DONE<kiapi::mdw::ListReviewsReply> aDone )
{
    // No revision filter, so an approval of another revision still reaches the status line
    return MdwReadReviews( aSource, aItem, wxEmptyString,
                           [aDone = std::move( aDone )]( const MDW_RESULT& aResult, const MDW_REVIEW_READ& aRead )
                           {
                               kiapi::mdw::ListReviewsReply list;

                               if( aResult.IsOk() )
                               {
                                   list.mutable_reviews()->Add( aRead.reviews.begin(), aRead.reviews.end() );
                                   list.set_approval_required( aRead.approval_required );
                               }

                               if( aDone )
                                   aDone( aResult, list );
                           } );
}
