/////////////////////////////////////////////////////////////////////////////
// Name:        optionsframe.h
// Purpose:     
// Author:      jean-pierre charras
// Modified by: 
// Created:     01/27/04 14:48:57
// RCS-ID:      
// Copyright:   suite kicad
// Licence:     
/////////////////////////////////////////////////////////////////////////////

#ifndef _OPTIONSFRAME_H_
#define _OPTIONSFRAME_H_

#ifdef __GNUG__
#pragma interface "optionsframe.h"
#endif

/*!
 * Includes
 */

////@begin includes
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
#define ID_CHECKBOX 10001
#define ID_RADIOBOX 10002
#define ID_RADIOBOX1 10003
#define ID_RADIOBOX3 10005
#define ID_RADIOBOX2 10004
#define ID_BUTTON 10006
#define ID_BUTTON1 10007
////@end control identifiers

/*!
 * DisplayOptionFrame class declaration
 */

class DisplayOptionFrame: public wxDialog
{    
    DECLARE_CLASS( DisplayOptionFrame )
    DECLARE_EVENT_TABLE()

public:
    /// Constructors
    DisplayOptionFrame( );
    DisplayOptionFrame( wxWindow* parent, wxWindowID id = -1, const wxString& caption = _("optionsframe"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize, long style = wxCAPTION|wxRESIZE_BORDER|wxSYSTEM_MENU );

    /// Creation
    bool Create( wxWindow* parent, wxWindowID id = -1, const wxString& caption = _("optionsframe"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize, long style = wxCAPTION|wxRESIZE_BORDER|wxSYSTEM_MENU );

    /// Creates the controls and sizers
    void CreateControls();

////@begin DisplayOptionFrame event handler declarations

////@end DisplayOptionFrame event handler declarations

////@begin DisplayOptionFrame member function declarations

////@end DisplayOptionFrame member function declarations

    /// Should we show tooltips?
    static bool ShowToolTips();

////@begin DisplayOptionFrame member variables
    wxCheckBox* m_ShowGridButt;
    wxRadioBox* m_SelGridSize;
    wxRadioBox* m_SelShowPins;
    wxRadioBox* m_Selunits;
    WinEDA_DrawFrame * m_Parent;
////@end DisplayOptionFrame member variables
};

#endif
    // _OPTIONSFRAME_H_
