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
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef RHI_PRESENT_MODE_H
#define RHI_PRESENT_MODE_H

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>


/**
 * Presents allowed to wait for the display at once while frames are paced.
 */
constexpr int RHI_PRESENTS_IN_FLIGHT = 2;


/**
 * How a swapchain hands finished images to the display.
 */
enum class RHI_PRESENT_MODE : uint8_t
{
    FIFO,       /**< Waits for a vertical blank, never tears, adds up to a refresh of latency */
    MAILBOX,    /**< Replaces the queued image at each blank, never tears, no wait */
    IMMEDIATE   /**< Shows the image at once, tears unless a compositor owns the final present */
};


/**
 * Choose the present mode for a swapchain from the modes the surface offers.
 *
 * MAILBOX is preferred. Without it a composited window uses IMMEDIATE, since the compositor's
 * vsynced frame keeps it from tearing, and anything else uses FIFO, which every surface offers.
 * With @p aImmediateUnderCompositor a composited window prefers IMMEDIATE over MAILBOX because
 * Mesa's MAILBOX adds latency under a compositor.
 *
 * @param aComposited true when a compositor owns the final present.
 * @param aImmediateUnderCompositor true on Linux.
 */
inline RHI_PRESENT_MODE RhiChoosePresentMode( const std::vector<RHI_PRESENT_MODE>& aOffered, bool aComposited,
                                              bool aImmediateUnderCompositor = false )
{
    auto offered = [&]( RHI_PRESENT_MODE aMode )
    {
        return std::find( aOffered.begin(), aOffered.end(), aMode ) != aOffered.end();
    };

    if( aComposited && aImmediateUnderCompositor && offered( RHI_PRESENT_MODE::IMMEDIATE ) )
        return RHI_PRESENT_MODE::IMMEDIATE;

    if( offered( RHI_PRESENT_MODE::MAILBOX ) )
        return RHI_PRESENT_MODE::MAILBOX;

    if( aComposited && offered( RHI_PRESENT_MODE::IMMEDIATE ) )
        return RHI_PRESENT_MODE::IMMEDIATE;

    return RHI_PRESENT_MODE::FIFO;
}


/**
 * The earliest time the frame after one started at @p aFrameStart may start, for a window whose
 * presents do not wait for the display.
 *
 * With a known vertical blank this is the next blank after @p aFrameStart, so at most one frame
 * starts per refresh and frames do not drift against the display. Without one it is a refresh later.
 *
 * @param aRefreshHz the display's refresh rate, above zero.
 * @param aVblank a time the display refreshed, before or after @p aFrameStart.
 */
template <typename TIME_POINT>
TIME_POINT RhiNextFrameAt( TIME_POINT aFrameStart, double aRefreshHz, std::optional<TIME_POINT> aVblank )
{
    using DURATION = typename TIME_POINT::duration;

    const DURATION period = std::chrono::round<DURATION>( std::chrono::duration<double>( 1.0 / aRefreshHz ) );

    if( !aVblank || period <= DURATION::zero() )
        return aFrameStart + period;

    const DURATION since = aFrameStart - *aVblank;
    auto           refreshes = since / period;

    /**
     * Division truncates towards zero, and a blank reported after the frame start needs the floor.
     */
    if( since < DURATION::zero() && refreshes * period != since )
        --refreshes;

    return *aVblank + ( refreshes + 1 ) * period;
}


#endif // RHI_PRESENT_MODE_H
