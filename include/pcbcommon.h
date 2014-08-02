/**
 * @file pcbcommon.h
 */

#ifndef PCBCOMMON_H_
#define PCBCOMMON_H_


#include <dlist.h>
#include <layers_id_colors_and_visibility.h>


#define MIN_DRAW_WIDTH      1               ///< Minimum trace drawing width.


class D_PAD;
class TRACK;
class BOARD;
class DISPLAY_OPTIONS;

extern DISPLAY_OPTIONS DisplayOpt;

extern int g_CurrentVersionPCB;

/// List of segments of the trace currently being drawn.
extern DLIST<TRACK> g_CurrentTrackList;

#define g_CurrentTrackSegment g_CurrentTrackList.GetLast()    ///< most recently created segment

#define g_FirstTrackSegment   g_CurrentTrackList.GetFirst()   ///< first segment created

/// Utility for comma separated lists
void AccumulateDescription( wxString &aDesc, const wxString &aItem );

#endif  // PCBCOMMON_H_
