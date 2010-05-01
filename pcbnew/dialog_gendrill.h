/////////////////////////////////////////////////////////////////////////////
// Name:        dialog_gendrill.h
// Author:      jean-pierre Charras
// Created:     2010 apr 30
// Licence:     GPL
/////////////////////////////////////////////////////////////////////////////

#ifndef _DIALOG_GENDRILL_H_
#define _DIALOG_GENDRILL_H_

#include "dialog_gendrill_base.h"

class DIALOG_GENDRILL: public DIALOG_GENDRILL_BASE
{

private:
	WinEDA_PcbFrame*  m_Parent;
	int  m_PadsHoleCount;
	int m_ThroughViasCount;
	int m_MicroViasCount;
	int m_BlindOrBuriedViasCount;

public:
    DIALOG_GENDRILL( WinEDA_PcbFrame* parent );
    ~DIALOG_GENDRILL();

private:
    /// Initialises member variables
    void initDialog();

    /// wxEVT_COMMAND_RADIOBOX_SELECTED event handler for ID_SEL_DRILL_UNITS
    void OnSelDrillUnitsSelected( wxCommandEvent& event );

    /// wxEVT_COMMAND_RADIOBOX_SELECTED event handler for ID_SEL_ZEROS_FMT
    void OnSelZerosFmtSelected( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_OK
    void OnOkClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_CANCEL
    void OnCancelClick( wxCommandEvent& event );


private:
    void    InitDisplayParams(void);
    void    SetParams(void);
    void    GenDrillFiles( wxCommandEvent& event );
    void    GenDrillMap( const wxString aFileName, std::vector<HOLE_INFO> & aHoleListBuffer, std::vector<DRILL_TOOL> & aToolListBuffer, int format );
    void    UpdatePrecisionOptions( wxCommandEvent& event );
    void    UpdateConfig();
	void    Write_Excellon_Header( FILE * aFile);
    void    GenDrillReport( const wxString aFileName );
	int     Create_Drill_File_EXCELLON(FILE *excellon_dest,
		std::vector<HOLE_INFO> & aHoleListBuffer,
		std::vector<DRILL_TOOL> & aToolListBuffer );
	int 	Gen_Liste_Tools( std::vector<DRILL_TOOL> & buffer, bool print_header );
};

#endif      // _DIALOG_GENDRILL_H_
