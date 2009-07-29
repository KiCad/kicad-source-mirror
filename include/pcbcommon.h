
#ifndef __PCBCOMMON_H__
#define __PCBCOMMON_H__

#include "pcbstruct.h"
#include "dlist.h"

#define L_MIN_DESSIN 1  /* Min width segments to allow draws with tickness */

class DPAD;
class BOARD_ITEM;
class PCB_SCREEN;
class DISPLAY_OPTIONS;
class EDA_BoardDesignSettings;

/* Look up Table for conversion one layer number -> one bit layer mask: */
extern int g_TabOneLayerMask[LAYER_COUNT];
/* Look up Table for conversion copper layer count -> general copper layer
 * mask: */
extern int g_TabAllCopperLayerMask[NB_COPPER_LAYERS];



extern wxArrayString   g_LibName_List;    // library list to load
extern DISPLAY_OPTIONS DisplayOpt;

extern wxString PcbExtBuffer;
extern wxString g_SaveFileName;
extern wxString NetExtBuffer;
extern wxString NetCmpExtBuffer;
extern const wxString ModuleFileExtension;

extern const wxString ModuleFileWildcard;

extern wxString g_ViaType_Name[4];

extern int g_CurrentVersionPCB;
extern int g_AnchorColor;
extern int g_ModuleTextCMPColor;
extern int g_ModuleTextCUColor;
extern int g_ModuleTextNOVColor;
extern int g_PadCUColor;
extern int g_PadCMPColor;



/* variables generales */
extern int    g_TimeOut;            // Timer for automatic saving
extern int    g_SaveTime;           // Time for next saving

// Current design settings:
extern class EDA_BoardDesignSettings g_DesignSettings;

extern DLIST<TRACK> g_CurrentTrackList;

#define g_CurrentTrackSegment    \
    g_CurrentTrackList.GetLast()    ///< most recently created segment
#define g_FirstTrackSegment      \
    g_CurrentTrackList.GetFirst()   ///< first segment created

extern PCB_SCREEN* ScreenPcb;       /* Ecran principal */
extern BOARD*      g_ModuleEditor_Pcb;

/* Pad editing */
extern wxString g_Current_PadName;  // Last used pad name (pad num)

extern D_PAD    g_Pad_Master;

#endif  /*  __PCBCOMMON_H__ */
