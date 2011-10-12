
#ifndef __PROTOS_H__
#define __PROTOS_H__

#include "colors.h"


class EDA_DRAW_PANEL;
class EDA_DRAW_FRAME;
class PICKED_ITEMS_LIST;
class SCH_EDIT_FRAME;
class LIB_EDIT_FRAME;
class CMP_LIBRARY;
class SCH_COMPONENT;
class SCH_SCREEN;
class SCH_ITEM;
class PLOTTER;
class SCH_SHEET;
class NETLIST_OBJECT;


/****************/
/* DATABASE.CPP */
/****************/
void DisplayCmpDoc( wxString& Name );
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
void DrawDanglingSymbol( EDA_DRAW_PANEL* panel, wxDC* DC, const wxPoint& pos, int Color );


/***********************************/
/* dialogs/dialog_color_config.cpp */
/***********************************/
EDA_Colors ReturnLayerColor( int Layer );


/***************/
/* NETLIST.CPP */
/***************/
/**
 * Function IsBusLabel
 * test if the \a aLabel has a bus notation.
 *
 * @param aLabel A wxString object containing the label to test.
 * @return false if text is not a bus notattion otherwise true is returned.
 */
bool IsBusLabel( const wxString& aLabel );


/***************/
/* PINEDIT.CPP */
/***************/
void InstallPineditFrame( LIB_EDIT_FRAME* parent, wxDC* DC, const wxPoint& pos );


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

/***************/
/* LIBARCH.CPP */
/***************/

bool LibArchive( wxWindow* frame, const wxString& ArchFullFileName );


/***************/
/* OPTIONS.CPP */
/***************/
void DisplayOptionFrame( SCH_EDIT_FRAME* parent, const wxPoint& framepos );


/****************/
/* CONTROLE.CPP */
/****************/
void RemoteCommand( const char* cmdline );


/* Prototypes in netlist_control.cpp */
void FreeNetObjectsList( std::vector <NETLIST_OBJECT*>& aNetObjectslist );

/**
 * Function ReturnUserNetlistTypeName
 * to retrieve user netlist type names
 * @param first_item = true: return first name of the list, false = return next
 * @return a wxString : name of the type netlist or empty string
 * this function must be called first with "first_item" = true
 * and after with "first_item" = false to get all the other existing netlist
 * names
 */
wxString ReturnUserNetlistTypeName( bool first_item );

#endif  /* __PROTOS_H__ */
