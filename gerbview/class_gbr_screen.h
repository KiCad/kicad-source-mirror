/**
 * @file class_gbr_screen.h
 */

#ifndef CLASS_GBR_SCREEN_H_
#define CLASS_GBR_SCREEN_H_


#include <base_units.h>
#include <class_base_screen.h>
#include <layers_id_colors_and_visibility.h>

#define ZOOM_FACTOR( x )       ( x * IU_PER_DECIMILS )


/* Handle info to display a board */
class GBR_SCREEN : public BASE_SCREEN
{
public:
    LAYER_NUM m_Active_Layer;
    /**
     * Constructor
     * @param aPageSizeIU is the size of the initial paper page in internal units.
     */
    GBR_SCREEN( const wxSize& aPageSizeIU );

    ~GBR_SCREEN();

    GBR_SCREEN* Next() const { return static_cast<GBR_SCREEN*>( Pnext ); }

//    void        SetNextZoom();
//    void        SetPreviousZoom();
//    void        SetLastZoom();

    virtual int MilsToIuScalar();

    /**
     * Function ClearUndoORRedoList
     * virtual pure in BASE_SCREEN, so it must be defined here
     */
    void ClearUndoORRedoList( UNDO_REDO_CONTAINER& aList, int aItemCount = -1 );
};


#endif  // CLASS_GBR_SCREEN_H_
