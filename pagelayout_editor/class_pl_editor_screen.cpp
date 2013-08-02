/**
 * @file class_pl_editor_screen.cpp
 */

#include <fctsys.h>
#include <common.h>
#include <macros.h>
#include <class_pl_editor_screen.h>
#include <base_units.h>
#include <pl_editor_id.h>


#define MM_GRID( x ) wxRealPoint( x * IU_PER_MM, x * IU_PER_MM )
#define ZOOM_FACTOR( x )  ( x * IU_PER_MM / 1000 )


/**
    Default zoom values.
    Roughly a 1.5 progression.
*/
static const double pl_editorZoomList[] =
{
    ZOOM_FACTOR( 5 ),
    ZOOM_FACTOR( 7.0 ),
    ZOOM_FACTOR( 10.0 ),
    ZOOM_FACTOR( 15.0 ),
    ZOOM_FACTOR( 22.0 ),
    ZOOM_FACTOR( 35.0 ),
    ZOOM_FACTOR( 50.0 ),
    ZOOM_FACTOR( 80.0 ),
    ZOOM_FACTOR( 120.0 ),
    ZOOM_FACTOR( 200.0 ),
    ZOOM_FACTOR( 350.0 ),
    ZOOM_FACTOR( 500.0 ),
    ZOOM_FACTOR( 750.0 ),
    ZOOM_FACTOR( 1000.0 ),
    ZOOM_FACTOR( 1500.0 ),
    ZOOM_FACTOR( 2000.0 ),
    ZOOM_FACTOR( 3000.0 ),
};


// Default grid sizes for PCB editor screens.
static GRID_TYPE pl_editorGridList[] =
{
    // predefined grid list in mm
    { ID_POPUP_GRID_LEVEL_1MM,      MM_GRID( 1.0 )     },
    { ID_POPUP_GRID_LEVEL_0_5MM,    MM_GRID( 0.5 )     },
    { ID_POPUP_GRID_LEVEL_0_25MM,   MM_GRID( 0.25 )    },
    { ID_POPUP_GRID_LEVEL_0_2MM,    MM_GRID( 0.2 )     },
    { ID_POPUP_GRID_LEVEL_0_1MM,    MM_GRID( 0.1 )     },
};


PL_EDITOR_SCREEN::PL_EDITOR_SCREEN( const wxSize& aPageSizeIU ) :
    BASE_SCREEN( SCREEN_T )
{
    for( unsigned i = 0; i < DIM( pl_editorZoomList );  ++i )
        m_ZoomList.push_back( pl_editorZoomList[i] );

    for( unsigned i = 0; i < DIM( pl_editorGridList );  ++i )
        AddGrid( pl_editorGridList[i] );

    // Set the working grid size to a reasonable value
    SetGrid( MM_GRID( 1.0 ) );

    SetZoom( ZOOM_FACTOR( 350 ) );            // a default value for zoom

    InitDataPoints( aPageSizeIU );
    m_NumberOfScreens = 2;
}


PL_EDITOR_SCREEN::~PL_EDITOR_SCREEN()
{
    ClearUndoRedoList();
}


// virtual function
int PL_EDITOR_SCREEN::MilsToIuScalar()
{
    return (int)IU_PER_MILS;
}


/* Virtual function needed by classes derived from BASE_SCREEN
 * this is a virtual pure function in BASE_SCREEN
 */
void PL_EDITOR_SCREEN::ClearUndoORRedoList( UNDO_REDO_CONTAINER& aList,
                                            int aItemCount )
{
    if( aItemCount == 0 )
        return;

    unsigned icnt = aList.m_CommandsList.size();

    if( aItemCount > 0 )
        icnt = aItemCount;

    for( unsigned ii = 0; ii < icnt; ii++ )
    {
        if( aList.m_CommandsList.size() == 0 )
            break;

        PICKED_ITEMS_LIST* curr_cmd = aList.m_CommandsList[0];
        aList.m_CommandsList.erase( aList.m_CommandsList.begin() );

        curr_cmd->ClearListAndDeleteItems();
        delete curr_cmd;    // Delete command
    }
}
