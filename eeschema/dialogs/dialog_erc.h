/////////////////////////////////////////////////////////////////////////////
// Name:        dialog_erc.h
// Author:      jean-pierre Charras
// Licence:    GPL
/////////////////////////////////////////////////////////////////////////////

#ifndef _DIALOG_ERC_H_
#define _DIALOG_ERC_H_

#include <wx/htmllbox.h>
#include <vector>


#include <dialog_erc_base.h>

/* Variable locales */
extern int           DiagErc[PIN_NMAX][PIN_NMAX];
extern bool          DiagErcTableInit; // go to true after DiagErc init
extern int           DefaultDiagErc[PIN_NMAX][PIN_NMAX];

/*  Control identifiers */
#define ID_MATRIX_0 1800

/*!
 * DIALOG_ERC class declaration
 */

class DIALOG_ERC : public DIALOG_ERC_BASE
{
    DECLARE_EVENT_TABLE()

private:
    SCH_EDIT_FRAME* m_parent;
    wxBitmapButton* m_buttonList[PIN_NMAX][PIN_NMAX];
    bool            m_initialized;
    const SCH_MARKER* m_lastMarkerFound;
    static bool     m_writeErcFile;

public:
    DIALOG_ERC( SCH_EDIT_FRAME* parent );
    ~DIALOG_ERC();

private:
    void Init();

    // from DIALOG_ERC_BASE:
	void OnCloseErcDialog( wxCloseEvent& event );
    void OnErcCmpClick( wxCommandEvent& event );
    void OnEraseDrcMarkersClick( wxCommandEvent& event );
    void OnButtonCloseClick( wxCommandEvent& event );
    void OnResetMatrixClick( wxCommandEvent& event );
    void OnLeftClickMarkersList( wxCommandEvent& event );

    // Double click on a marker info:
    void OnLeftDblClickMarkersList( wxCommandEvent& event );

    void TestErc( wxArrayString* aMessagesList );
    void DisplayERC_MarkersList();
    void SelLocal( wxCommandEvent& event );
    void SelNewCmp( wxCommandEvent& event );
    void ResetDefaultERCDiag( wxCommandEvent& event );
    void ChangeErrorLevel( wxCommandEvent& event );
    void ReBuildMatrixPanel();
    void setDRCMatrixButtonState( wxBitmapButton *aButton, int aState );
    void updateMarkerCounts( SCH_SCREENS *screens );
};


#endif

// _DIALOG_ERC_H_
