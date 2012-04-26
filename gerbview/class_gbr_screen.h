/**
 * @file class_gbr_screen.h
 */

#ifndef CLASS_GBR_SCREEN_H_
#define CLASS_GBR_SCREEN_H_


#include <base_units.h>
#include <class_pcb_screen.h>

#define ZOOM_FACTOR( x )       ( x * IU_PER_DECIMILS )


/* Handle info to display a board */
class GBR_SCREEN : public PCB_SCREEN
{
public:

    /**
     * Constructor
     * @param aPageSizeIU is the size of the initial paper page in internal units.
     */
    GBR_SCREEN( const wxSize& aPageSizeIU );

    ~GBR_SCREEN();

    GBR_SCREEN* Next() { return (GBR_SCREEN*) Pnext; }

    virtual int MilsToIuScalar();
};

#endif  // CLASS_GBR_SCREEN_H_
