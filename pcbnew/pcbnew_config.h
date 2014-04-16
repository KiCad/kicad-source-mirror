/**
 * @file pcbnew_config.h
 * @brief Configuration parameters for Pcbnew.
 */

#ifndef _PCBNEW_CONFIG_H_
#define _PCBNEW_CONFIG_H_

#include <config_params.h>
#include <colors_selection.h>

/* Useful macro : */
#define LOC_COLOR(layer)            &g_ColorsSettings.m_LayersColors[layer]
#define ITEM_COLOR(item_visible)    &g_ColorsSettings.m_ItemsColors[item_visible]


#endif    // _PCBNEW_CONFIG_H_
