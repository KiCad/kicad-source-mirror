/////////////////////////////////////////////////////////////////////////////
// Name:        dialog_erc.h
// Author:      jean-pierre Charras
// Licence:    GPL
/////////////////////////////////////////////////////////////////////////////

#ifndef _DIALOG_ERC_H_
#define _DIALOG_ERC_H_

#include <wx/htmllbox.h>
#include <vector>


#include "dialog_erc_base.h"

/* Variable locales */
extern int           DiagErc[PIN_NMAX][PIN_NMAX];
extern bool          DiagErcTableInit; // go to TRUE after DiagErc init
extern int           DefaultDiagErc[PIN_NMAX][PIN_NMAX];
extern const wxChar* CommentERC_H[];
extern const wxChar* CommentERC_V[];

/*  Control identifiers */
#define ID_MATRIX_0 1800

/*!
 * DIALOG_ERC class declaration
 */

class DIALOG_ERC : public DIALOG_ERC_BASE
{
    DECLARE_EVENT_TABLE()

private:
    SCH_EDIT_FRAME* m_Parent;
    wxBitmapButton* m_ButtonList[PIN_NMAX][PIN_NMAX];
    bool            m_Initialized;
    static bool     m_writeErcFile;

public:

    /// Constructors
    DIALOG_ERC( SCH_EDIT_FRAME* parent );

    void Init();

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_ERC_CMP
    void OnErcCmpClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_ERASE_DRC_MARKERS
    void OnEraseDrcMarkersClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_CANCEL
    void OnCancelClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_RESET_MATRIX
    void OnResetMatrixClick( wxCommandEvent& event );

    // Double click on a marker info:
	void OnLeftDClickMarkersList( wxCommandEvent& event );

    void TestErc( wxArrayString* aMessagesList );
    void DisplayERC_MarkersList();
    void SelLocal( wxCommandEvent& event );
    void SelNewCmp( wxCommandEvent& event );
    void ResetDefaultERCDiag( wxCommandEvent& event );
    void ChangeErrorLevel( wxCommandEvent& event );
    void ReBuildMatrixPanel();
};


#endif

// _DIALOG_ERC_H_
