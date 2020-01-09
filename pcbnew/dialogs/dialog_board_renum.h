/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2019 Brian Piccioni brian@documenteddesigns.com
 * Copyright (C) 1992-2019 KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#ifndef DIALOG_BOARD_RENUM_H_
#define DIALOG_BOARD_RENUM_H_

#include    <wx/wx.h>
#include    <dialogs/dialog_board_renum_base.h>
#include    <class_board.h>
#include    <class_module.h>
#include    <class_text_mod.h>
#include    <layers_id_colors_and_visibility.h>
#include    <stdint.h>
#include    <pcb_base_frame.h>
#include    <pcb_edit_frame.h>
#include    <project.h>
#include    <netlist_reader/pcb_netlist.h>

#include    <fctsys.h>
#include    <tool/actions.h>
#include    <frame_type.h>
#include    <tool/tool_manager.h>
#include    <unistd.h>

#define SORTXFIRST          0b000       //Sort on X
#define SORTYFIRST          0b100       //Sort on Y
#define ASCENDINGFIRST      0b000       //Sort low to high
#define DESCENDINGFIRST     0b010       //Sort high to low
#define ASCENDINGSECOND     0b000       //Sort low to high
#define DESCENDINGSECOND    0b001       //Sort high to low

#define MINGRID             0.001
#define DEFAULT_GRID        1.000

#define LOGFILENAME         "RenumberLog.txt"

#define SetSortCodes( DirArray, Code ) { \
    SortYFirst = (( DirArray[Code] & SORTYFIRST ) != 0 ); \
    DescendingFirst = (( DirArray[Code] & DESCENDINGFIRST ) != 0 ); \
    DescendingSecond = (( DirArray[Code] & DESCENDINGSECOND ) != 0 ); \
 }

enum ActionCode {
    UpdateRefDes, EmptyRefDes, InvalidRefDes, Exclude
};

struct RefDesChange {
    timestamp_t TimeStamp;     //Module unique ID
    wxString NewRefDes;         //The new reference designation (F_U21)
    wxString OldRefDesString;   //What the old refdes preamble + number was
    bool Front;             //True if on the front of the board
    ActionCode Action;            //Used to skip (if #, etc)
};

struct RefDesInfo {
    bool Front;                 //True if on the front of the board
    timestamp_t TimeStamp;     //Module unique ID
    wxString RefDesString;      //What its refdes is
    int x, y;                   //The coordinates
    int roundedx, roundedy;     //The coordinates after rounding.
    LIB_ID FPID;
};

struct RefDesTypeStr {
    wxString RefDesType;
    unsigned int RefDesCount;
};

class DIALOG_BOARD_RENUM: public DIALOG_BOARD_RENUM_BASE {
public:

    DIALOG_BOARD_RENUM(PCB_EDIT_FRAME *aParentFrame);
    ~DIALOG_BOARD_RENUM();

    void ShowWarning(const wxString &aMessage);
    void ShowMessage(const wxString &message);
    void FatalError(const wxString &message);
    void GetParameters(void);

    int LoadPCBFile(struct KiCadFile &Schematic);
    void RenumKiCadPCB(void);
    void WriteLogFile(void);
    PCB_EDIT_FRAME *m_frame;
    BOARD *m_board;
    MODULES m_modules;

    wxString m_LogFile;
    wxString m_ChangeFile;
    std::vector<RefDesChange> m_ChangeArray;
    std::vector<RefDesInfo> m_FrontModules;
    std::vector<RefDesInfo> m_BackModules;
    std::vector<RefDesTypeStr> m_RefDesTypes;
    std::vector<wxString> m_ExcludeArray;

private:

    void OnRenumberClick(wxCommandEvent &event) override;
    void OKDone(wxCommandEvent &event) override;

    void FlushFiles(void);
    void WriteRenumFile(const char *aFileType, wxString &aBuffer);
    void LogModules(wxString &aMessage, std::vector<RefDesInfo> &aModules);
    void LogRefDesTypes(void);
    void LogChangeArray(void);
    void LogExcludeList(void);
    void LogMessage(wxString &aMessage);
    void LogMessage(std::string &aMessage);

    void BuildModuleList(std::vector<RefDesInfo> &aBadRefDes);
    void BuildChangeArray(std::vector<RefDesInfo> &aModules,
            unsigned int aStartRefDes, std::vector<RefDesInfo> &aBadRefDes);
    void BuildExcludeList(void);

    int RoundToGrid(int aCoord);
    RefDesChange* GetNewRefDes(MODULE *aMod);
    std::string CoordTowxString(int aX, int aY);

};

class MyApp: public wxApp {
public:
    virtual bool OnInit() override;
};

#endif /* DIALOG_BOARD_RENUMCLASSES_H_ */
