
#ifndef __PROTOS_H__
#define __PROTOS_H__

#include <colors.h>

class EDA_DRAW_PANEL;
class EDA_DRAW_FRAME;
class PICKED_ITEMS_LIST;
class SCH_EDIT_FRAME;
class LIB_EDIT_FRAME;
class CMP_LIBRARY;
class SCH_COMPONENT;
class SCH_SCREEN;
class SCH_ITEM;


/****************/
/* DATABASE.CPP */
/****************/
//void DisplayCmpDoc( wxString& Name );
wxString DataBaseGetName( EDA_DRAW_FRAME* frame, wxString& Keys, wxString& BufName );


/*********************/
/* DANGLING_ENDS.CPP */
/*********************/
bool SegmentIntersect( wxPoint aSegStart, wxPoint aSegEnd, wxPoint aTestPoint );

// operations_on_item_lists.cpp
void DeleteItemsInList( EDA_DRAW_PANEL* panel, PICKED_ITEMS_LIST& aItemsList );

/**
 * Function DuplicateStruct
 * creates a new copy of given struct.
 * @param aDrawStruct = the SCH_ITEM to duplicate
 * @param aClone (defualt = true)
 *     if true duplicate also some parameters that must be unique
 *     (timestamp and sheet name)
 *      aClone must be false. use true only is undo/redo duplications
 */
SCH_ITEM* DuplicateStruct( SCH_ITEM* DrawStruct, bool aClone = false );


/****************/
/* EEREDRAW.CPP */
/****************/
void DrawDanglingSymbol( EDA_DRAW_PANEL* panel, wxDC* DC, const wxPoint& pos, EDA_COLOR_T Color );


/***********************************/
/* dialogs/dialog_color_config.cpp */
/***********************************/
EDA_COLOR_T ReturnLayerColor( int Layer );


/***************/
/* SELPART.CPP */
/***************/

/**
 * Function DisplayComponentsNamesInLib
 * Select component from list of components in this library
 *
 * If == NULL Library, selection of library REQUESTED
 * If only in research library
 *
 * Returns
 *   1 if selected component
 *   0 if canceled order
 */
int DisplayComponentsNamesInLib( EDA_DRAW_FRAME* frame,
                                 CMP_LIBRARY*    Library,
                                 wxString&       Buffer,
                                 wxString&       OldName );

/**
 * Function SelectLibraryFromList
 * displays a list of current loaded libraries, and allows the user to select
 * a library
 * This list is sorted, with the library cache always at end of the list
 */
CMP_LIBRARY* SelectLibraryFromList( EDA_DRAW_FRAME* frame );

/**
 * Get the name component from a library to load.
 *
 * If no library specified, there will be demand for selection of a library.
 * Returns
 *   1 if selected component
 *   0 if canceled order
 * Place the name of the selected component list in BufName
 */
int GetNameOfPartToLoad( EDA_DRAW_FRAME* frame, CMP_LIBRARY* Lib, wxString& BufName );


/****************/
/* CONTROLE.CPP */
/****************/
void RemoteCommand( const char* cmdline );


#endif  /* __PROTOS_H__ */
