/////////////////////////////////////////////////////////////////////////////
// Name:        plotdxf.h
// Purpose:     
// Author:      Lorenzo Marcantonio
// Modified by: 
// Created:     01/02/2006 08:37:24
// RCS-ID:      
// Copyright:   GNU License
// Licence:     
/////////////////////////////////////////////////////////////////////////////

#ifndef _PLOTDXF_H_
#define _PLOTDXF_H_

#if defined(__GNUG__) && !defined(NO_GCC_PRAGMA)
#pragma interface "plotdxf.h"
#endif

/*!
 * Includes
 */

////@begin includes
#include "wx/valgen.h"
////@end includes

/*!
 * Forward declarations
 */

////@begin forward declarations
class wxBoxSizer;
////@end forward declarations

/*!
 * Control identifiers
 */

////@begin control identifiers
#define ID_DIALOG 10000
#define ID_RADIOBOX 10001
#define ID_CHECKBOX 10005
#define ID_PLOT_DXF_CURRENT_EXECUTE 10003
#define ID_PLOT_DXF_ALL_EXECUTE 10004
#define ID_TEXTCTRL 10006
#define SYMBOL_WINEDA_PLOTDXFFRAME_STYLE wxDEFAULT_DIALOG_STYLE|wxCAPTION|wxSYSTEM_MENU|wxCLOSE_BOX|wxTAB_TRAVERSAL|MAYBE_RESIZE_BORDER
#define SYMBOL_WINEDA_PLOTDXFFRAME_TITLE _("EESchema Plot DXF")
#define SYMBOL_WINEDA_PLOTDXFFRAME_IDNAME ID_DIALOG
#define SYMBOL_WINEDA_PLOTDXFFRAME_SIZE wxSize(400, 300)
#define SYMBOL_WINEDA_PLOTDXFFRAME_POSITION wxDefaultPosition
////@end control identifiers

/*!
 * Compatibility
 */

#ifndef wxCLOSE_BOX
#define wxCLOSE_BOX 0x1000
#endif

/*!
 * WinEDA_PlotDXFFrame class declaration
 */

class WinEDA_PlotDXFFrame: public wxDialog
{    
    DECLARE_DYNAMIC_CLASS( WinEDA_PlotDXFFrame )
    DECLARE_EVENT_TABLE()

public:
    /// Constructors
    WinEDA_PlotDXFFrame( );
    WinEDA_PlotDXFFrame( WinEDA_DrawFrame* parent, wxWindowID id = SYMBOL_WINEDA_PLOTDXFFRAME_IDNAME, const wxString& caption = SYMBOL_WINEDA_PLOTDXFFRAME_TITLE, const wxPoint& pos = SYMBOL_WINEDA_PLOTDXFFRAME_POSITION, const wxSize& size = SYMBOL_WINEDA_PLOTDXFFRAME_SIZE, long style = SYMBOL_WINEDA_PLOTDXFFRAME_STYLE );

    /// Creation
    bool Create( wxWindow* parent, wxWindowID id = SYMBOL_WINEDA_PLOTDXFFRAME_IDNAME, const wxString& caption = SYMBOL_WINEDA_PLOTDXFFRAME_TITLE, const wxPoint& pos = SYMBOL_WINEDA_PLOTDXFFRAME_POSITION, const wxSize& size = SYMBOL_WINEDA_PLOTDXFFRAME_SIZE, long style = SYMBOL_WINEDA_PLOTDXFFRAME_STYLE );

    /// Creates the controls and sizers
    void CreateControls();

////@begin WinEDA_PlotDXFFrame event handler declarations

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_PLOT_DXF_CURRENT_EXECUTE
    void OnPlotDXFCurrentExecuteClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_PLOT_DXF_ALL_EXECUTE
    void OnPlotDXFAllExecuteClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_CANCEL
    void OnCancelClick( wxCommandEvent& event );

////@end WinEDA_PlotDXFFrame event handler declarations

////@begin WinEDA_PlotDXFFrame member function declarations

    /// Retrieves bitmap resources
    wxBitmap GetBitmapResource( const wxString& name );

    /// Retrieves icon resources
    wxIcon GetIconResource( const wxString& name );
////@end WinEDA_PlotDXFFrame member function declarations

    void InitOptVars();
    void CreateDXFFile(int AllPages);
    void PlotOneSheetDXF(const wxString & FileName,
			SCH_SCREEN * screen, Ki_PageDescr * sheet,
			wxPoint plot_offset, double scale);

    /// Should we show tooltips?
    static bool ShowToolTips();

////@begin WinEDA_PlotDXFFrame member variables
    wxRadioBox* m_PlotDXFColorOption;
    wxCheckBox* m_Plot_Sheet_Ref;
    wxButton* m_btClose;
    wxTextCtrl* m_MsgBox;
////@end WinEDA_PlotDXFFrame member variables
    WinEDA_DrawFrame * m_Parent;
    int PlotDXFColorOpt;
};

#endif
// _PLOTDXF_H_
