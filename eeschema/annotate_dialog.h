/////////////////////////////////////////////////////////////////////////////

// Name:        annotate_dialog.h
// Purpose:
// Author:      jean-pierre Charras
// Modified by:
// Created:     05/02/2006 12:31:28
// RCS-ID:
// Copyright:   License GNU
// Licence:
/////////////////////////////////////////////////////////////////////////////


#ifndef _ANNOTATE_DIALOG_H_
#define _ANNOTATE_DIALOG_H_

#if defined (__GNUG__) && !defined (NO_GCC_PRAGMA)
#pragma interface "annotate_dialog.h"
#endif

/*!
 * Includes
 */

#include "fctsys.h"

#include "common.h"
#include "program.h"
#include "libcmp.h"
#include "general.h"


/*!
 * Control identifiers
 */

#define ID_DIALOG                10000
#define ID_ENTIRE_SCHEMATIC      10001
#define ID_CURRENT_PAGE          10002
#define ID_RESET_ANNOTATION      10003
#define ID_SORT_BY_POSITION      10004
#define ID_SORT_BY_VALUE         10005

#define ANNOTATE_DIALOG_STYLE    wxDEFAULT_DIALOG_STYLE | MAYBE_RESIZE_BORDER
#define ANNOTATE_DIALOG_TITLE    _( "Annotate" )


/*!
 * Compatibility
 */

#ifndef wxCLOSE_BOX
#define wxCLOSE_BOX 0x1000
#endif

/*!
 * WinEDA_AnnotateFrame class declaration
 */

class WinEDA_AnnotateFrame : public wxDialog
{
    DECLARE_DYNAMIC_CLASS( WinEDA_AnnotateFrame )

public:

    /// Constructors
    WinEDA_AnnotateFrame();
    WinEDA_AnnotateFrame( WinEDA_SchematicFrame* parent,
                          wxWindowID id = wxID_ANY,
                          const wxString& caption = ANNOTATE_DIALOG_TITLE,
                          const wxPoint& pos = wxDefaultPosition,
                          const wxSize& size = wxDefaultSize,
                          long style = ANNOTATE_DIALOG_STYLE );

    /// Creation
    bool Create( wxWindow* parent,
                 wxWindowID id = wxID_ANY,
                 const wxString& caption = ANNOTATE_DIALOG_TITLE,
                 const wxPoint& pos = wxDefaultPosition,
                 const wxSize& size = wxDefaultSize,
                 long style = ANNOTATE_DIALOG_STYLE );

    /// Creates the controls and sizers
    void    CreateControls();

    /// Retrieves bitmap resources
    wxBitmap    GetBitmapResource( const wxString& name );

    /// Retrieves icon resources
    wxIcon      GetIconResource( const wxString& name );

    bool GetLevel( void );
    bool GetResetItems( void );
    bool GetSortOrder( void );

    /// Should we show tooltips?
    static bool ShowToolTips();

    WinEDA_SchematicFrame* m_Parent;

private:
    void OnClear( wxCommandEvent& event );
    void OnApply( wxCommandEvent& event );

    wxRadioButton* m_rbEntireSchematic;
    wxRadioButton* m_rbSortByPosition;
    wxCheckBox* m_cbResetAnnotation;
    wxButton* m_btnClear;

    DECLARE_EVENT_TABLE()
};

#endif

// _ANNOTATE_DIALOG_H_
