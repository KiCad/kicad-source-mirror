/////////////////////////////////////////////////////////////////////////////
// Name:        dialog_backanno.h
// Purpose:     
// Author:      Frank Bennett
// Modified by: 
// Created:     Wed Apr 30 16:40:21 MDT 2008
// RCS-ID:      
// Copyright:   License GNU
// Licence:     
/////////////////////////////////////////////////////////////////////////////

#ifndef _DIALOG_BACKANNO_H_
#define _DIALOG_BACKANNO_H_

/*!
 * Includes
 */

////@begin includes
#include "wx/valtext.h"
////@end includes

/*!
 * Forward declarations
 */

////@begin forward declarations
////@end forward declarations

/*!
 * Control identifiers
 */

////@begin control identifiers
#define ID_DIALOG 10000
#define SYMBOL_WINEDA_BACKANNOFRAME_STYLE wxCAPTION|wxSYSTEM_MENU|wxCLOSE_BOX|MAYBE_RESIZE_BORDER
#define SYMBOL_WINEDA_BACKANNOFRAME_TITLE _("EESchema Back Annotate")
#define SYMBOL_WINEDA_BACKANNOFRAME_IDNAME ID_DIALOG
#define SYMBOL_WINEDA_BACKANNOFRAME_SIZE wxSize(400, 300)
#define SYMBOL_WINEDA_BACKANNOFRAME_POSITION wxDefaultPosition
#define ID_TEXTCTRL1 10008
#define BACKANNO_SHEET 10001
#define FIND_HIERARCHY 10002
#define FIND_NEXT 10005
#define FIND_MARKERS 10003
#define FIND_NEXT_MARKER 10006
#define LOCATE_IN_LIBRARIES 10004
////@end control identifiers

/*!
 * Compatibility
 */

#ifndef wxCLOSE_BOX
#define wxCLOSE_BOX 0x1000
#endif

/*!
 * WinEDA_BackannoFrame class declaration
 */

class WinEDA_BackannoFrame: public wxDialog
{    
    DECLARE_DYNAMIC_CLASS( WinEDA_BackannoFrame )
    DECLARE_EVENT_TABLE()

public:
    /// Constructors
    WinEDA_BackannoFrame( );
    WinEDA_BackannoFrame( WinEDA_SchematicFrame* parent, wxWindowID id = SYMBOL_WINEDA_BACKANNOFRAME_IDNAME, const wxString& caption = SYMBOL_WINEDA_BACKANNOFRAME_TITLE, const wxPoint& pos = SYMBOL_WINEDA_BACKANNOFRAME_POSITION, const wxSize& size = SYMBOL_WINEDA_BACKANNOFRAME_SIZE, long style = SYMBOL_WINEDA_BACKANNOFRAME_STYLE );

    /// Creation
    bool Create( wxWindow* parent, wxWindowID id = SYMBOL_WINEDA_BACKANNOFRAME_IDNAME, const wxString& caption = SYMBOL_WINEDA_BACKANNOFRAME_TITLE, const wxPoint& pos = SYMBOL_WINEDA_BACKANNOFRAME_POSITION, const wxSize& size = SYMBOL_WINEDA_BACKANNOFRAME_SIZE, long style = SYMBOL_WINEDA_BACKANNOFRAME_STYLE );

////@begin WinEDA_BackannoFrame event handler declarations

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for BACKANNO_SHEET
    void OnBackannoSheetClick( wxCommandEvent& event );

////@end WinEDA_BackannoFrame event handler declarations

////@begin WinEDA_BackannoFrame member function declarations

    /// Retrieves bitmap resources
    wxBitmap GetBitmapResource( const wxString& name );

    /// Retrieves icon resources
    wxIcon GetIconResource( const wxString& name );
////@end WinEDA_BackannoFrame member function declarations

    /// Should we show tooltips?
    static bool ShowToolTips();

    void BackannoSchematicItem(wxCommandEvent& event);
    bool ReadInputStuffFile(const wxString & FullFileName);
    void LoadStuffFile( wxCommandEvent& event );

////@begin WinEDA_BackannoFrame member variables
    wxTextCtrl* m_NewTextCtrl;
////@end WinEDA_BackannoFrame member variables
	WinEDA_SchematicFrame * m_Parent;
};

#endif
    // _DIALOG_FIND_H_
