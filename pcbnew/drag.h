/**
 * @file drag.h
 * @brief Useful class and functions used to drag tracks
 */


#include <vector>
#include <wx/gdicmn.h>
#include "class_track.h"


class wxDC;
class EDA_DRAW_PANEL;
class MODULE;
class D_PAD;


/** Helper class to handle a list of track segments to drag
 * and has info to undo/abort the move command
 * a DRAG_SEGM manage one track segment or a via
 */
class DRAG_SEGM
{
public:
    TRACK*  m_Segm;         /* pointer to the segment a "dragger */
    D_PAD*  m_Pad_Start;    /* pointer to the pad origin if origin segment of pad */
    D_PAD*  m_Pad_End;      /* pointer to the pad end if end segment of pad */
    int     m_Flag;         /* indicator flags */

private:
    wxPoint m_StartInitialValue;
    wxPoint m_EndInitialValue;    // For abort: initial m_Start and m_End values for m_Segm


public:

    DRAG_SEGM( TRACK* segm );
    ~DRAG_SEGM() {};

    void SetInitialValues()
    {
        m_Segm->m_Start = m_StartInitialValue;
        m_Segm->m_End   = m_EndInitialValue;
    }
};

/* Variables */

// a list of DRAG_SEGM items used to move or drag tracks.
// Each DRAG_SEGM item points a segment to move.
extern std::vector<DRAG_SEGM> g_DragSegmentList;

/* Functions */
void DrawSegmentWhileMovingFootprint( EDA_DRAW_PANEL* panel, wxDC* DC );
void Build_Drag_Liste( EDA_DRAW_PANEL* panel, wxDC* DC, MODULE* Module );
void Build_1_Pad_SegmentsToDrag( EDA_DRAW_PANEL* panel, wxDC* DC, D_PAD* PtPad );
void Collect_TrackSegmentsToDrag( EDA_DRAW_PANEL* panel, wxDC* DC,
                                  wxPoint& point, int LayerMask, int net_code );


/**
 * Function EraseDragList
 * clear the .m_Flags of all track segments managed by in g_DragSegmentList
 * and clear the list.
 * In order to avoid useless memory allocation, the memory is not freed
 * and will be reused when creating a new list
 */
void EraseDragList();

/* Add the segment"Track" to the drag list, and erase it from screen
 * flag = STARTPOINT (if the point to drag is the start point of Track)
 * or ENDPOINT
 */
void AddSegmentToDragList( EDA_DRAW_PANEL* panel, wxDC* DC, int flag, TRACK* Track );
