/////////////////////////////////////////////////////////////////////////////

// Name:        dialog_gendrill.h
// Author:      jean-pierre Charras
// Created:     2010 apr 30
// Licence:     GPL
/////////////////////////////////////////////////////////////////////////////

#ifndef _DIALOG_GENDRILL_H_
#define _DIALOG_GENDRILL_H_

#include "dialog_gendrill_base.h"

class DIALOG_GENDRILL : public DIALOG_GENDRILL_BASE
{
public:
    static int  m_UnitDrillIsInch;
    static int  m_ZerosFormat;
    static int  m_PrecisionFormat;
    static bool m_MinimalHeader;
    static bool m_Mirror;
    static bool m_DrillOriginIsAuxAxis; /* Axis selection (main / auxiliary)
                                         *  for drill origin coordinates */
    DRILL_PRECISION m_Precision;        // Selected precision for drill files
    wxPoint     m_FileDrillOffset;  // Drill offset: 0,0 for absolute coordiantes, or auxialry axis origin

private:
    WinEDA_PcbFrame* m_Parent;
    int m_PadsHoleCount;
    int m_ThroughViasCount;
    int m_MicroViasCount;
    int m_BlindOrBuriedViasCount;

public: DIALOG_GENDRILL( WinEDA_PcbFrame* parent );
    ~DIALOG_GENDRILL();

private:
    // Initialises member variables
    void initDialog();
    void InitDisplayParams( void );
    // event functions
    void OnSelDrillUnitsSelected( wxCommandEvent& event );
    void OnSelZerosFmtSelected( wxCommandEvent& event );
    void OnOkClick( wxCommandEvent& event );
    void OnCancelClick( wxCommandEvent& event );
    // Specific functions:
    void SetParams( void );
    void GenDrillOrReportFiles();
    void GenDrillMap( const wxString           aFileName,
                      std::vector<HOLE_INFO>&  aHoleListBuffer,
                      std::vector<DRILL_TOOL>& aToolListBuffer,
                      int                      format );
    void UpdatePrecisionOptions( );
    void UpdateConfig();
    void Write_Excellon_Header( FILE* aFile, bool aMinimalHeader, zeros_fmt aFormat );
    void GenDrillReport( const wxString aFileName );
    int  Create_Drill_File_EXCELLON( FILE*                    aFile,
                                     wxPoint aOffset,
                                     std::vector<HOLE_INFO>&  aHoleListBuffer,
                                     std::vector<DRILL_TOOL>& aToolListBuffer );
    int  Gen_Liste_Tools( std::vector<DRILL_TOOL>& buffer, bool print_header );
    /**
     * Return the selected format for coordinates, if not decimal
     */
    DRILL_PRECISION GetPrecison();
};

#endif      // _DIALOG_GENDRILL_H_
