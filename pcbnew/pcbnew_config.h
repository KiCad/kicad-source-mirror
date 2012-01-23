/**
 * @file pcbnew_config.h
 * @brief Cconfiguration parameters for Pcbnew.
 */

#ifndef _PCBNEW_CONFIG_H_
#define _PCBNEW_CONFIG_H_

#include <param_config.h>
#include <colors_selection.h>

#define GROUP       wxT( "/pcbnew" )
#define GROUPLIB    wxT( "/pcbnew/libraries" )
#define GROUPCOMMON wxT( "/common" )

/* Useful macro : */
#define LOC_COLOR(layer)            &g_ColorsSettings.m_LayersColors[layer]
#define ITEM_COLOR(item_visible)    &g_ColorsSettings.m_ItemsColors[item_visible]


#endif    // _PCBNEW_CONFIG_H_
