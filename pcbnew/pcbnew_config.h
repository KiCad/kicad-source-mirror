/************************************************************/
/** pcbnew_config.h : configuration parameters for PCBNew  **/
/************************************************************/

#ifndef _PCBNEW_CONFIG_H_
#define _PCBNEW_CONFIG_H_

#include "param_config.h"
#include "colors_selection.h"


class BOARD_DESIGN_SETTINGS;


#define GROUP       wxT( "/pcbnew" )
#define GROUPLIB    wxT( "/pcbnew/libraries" )
#define GROUPCOMMON wxT( "/common" )

/* Useful macro : */
#define LOC_COLOR(layer) &g_ColorsSettings.m_LayersColors[layer]
#define ITEM_COLOR(item_visible) &g_ColorsSettings.m_ItemsColors[item_visible]

/* Configuration parameters. */
extern BOARD_DESIGN_SETTINGS boardDesignSettings;


#endif    // _PCBNEW_CONFIG_H_
