#ifdef __cplusplus
extern "C" {
#endif

#ifndef DARWIN
#define _POSIX_C_SOURCE 200809L
#endif
#include "emf2svg_private.h"
#include "emf2svg_print.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void U_EMRDELETECOLORSPACE_draw(const char *contents, FILE *out,
                                drawingStates *states) {
    FLAG_IGNORED;
    if (states->verbose) {
        U_EMRDELETECOLORSPACE_print(contents, states);
    }
}
void U_EMRDELETEOBJECT_draw(const char *contents, FILE *out,
                            drawingStates *states) {
    FLAG_SUPPORTED;
    if (states->verbose) {
        U_EMRDELETEOBJECT_print(contents, states);
    }
    PU_EMRDELETEOBJECT pEmr = (PU_EMRDELETEOBJECT)(contents);
    uint16_t index = pEmr->ihObject;
    returnOutOfOTIndex(index);
    freeObject(states, index);
}
void U_EMRREALIZEPALETTE_draw(const char *contents, FILE *out,
                              drawingStates *states) {
    FLAG_IGNORED;
    if (states->verbose) {
        U_EMRREALIZEPALETTE_print(contents, states);
    }
    UNUSED(contents);
}
void U_EMRRESIZEPALETTE_draw(const char *contents, FILE *out,
                             drawingStates *states) {
    FLAG_IGNORED;
    if (states->verbose) {
        U_EMRRESIZEPALETTE_print(contents, states);
    }
}
void U_EMRSELECTOBJECT_draw(const char *contents, FILE *out,
                            drawingStates *states) {
    FLAG_PARTIAL;
    if (states->verbose) {
        U_EMRSELECTOBJECT_print(contents, states);
    }
    PU_EMRSELECTOBJECT pEmr = (PU_EMRSELECTOBJECT)(contents);
    uint32_t index = pEmr->ihObject;
    if (index & U_STOCK_OBJECT) {
        switch (index) {
        case (U_WHITE_BRUSH):
            states->currentDeviceContext.fill_red = 0xFF;
            states->currentDeviceContext.fill_blue = 0xFF;
            states->currentDeviceContext.fill_green = 0xFF;
            states->currentDeviceContext.fill_mode = U_BS_SOLID;
            break;
        case (U_LTGRAY_BRUSH):
            states->currentDeviceContext.fill_red = 0xC0;
            states->currentDeviceContext.fill_blue = 0xC0;
            states->currentDeviceContext.fill_green = 0xC0;
            states->currentDeviceContext.fill_mode = U_BS_SOLID;
            break;
        case (U_GRAY_BRUSH):
            states->currentDeviceContext.fill_red = 0x80;
            states->currentDeviceContext.fill_blue = 0x80;
            states->currentDeviceContext.fill_green = 0x80;
            states->currentDeviceContext.fill_mode = U_BS_SOLID;
            break;
        case (U_DKGRAY_BRUSH):
            states->currentDeviceContext.fill_red = 0x40;
            states->currentDeviceContext.fill_blue = 0x40;
            states->currentDeviceContext.fill_green = 0x40;
            states->currentDeviceContext.fill_mode = U_BS_SOLID;
            break;
        case (U_BLACK_BRUSH):
            states->currentDeviceContext.fill_red = 0x00;
            states->currentDeviceContext.fill_blue = 0x00;
            states->currentDeviceContext.fill_green = 0x00;
            states->currentDeviceContext.fill_mode = U_BS_SOLID;
            break;
        case (U_NULL_BRUSH):
            states->currentDeviceContext.fill_mode = U_BS_NULL;
            break;
        case (U_WHITE_PEN):
            states->currentDeviceContext.stroke_red = 0xFF;
            states->currentDeviceContext.stroke_blue = 0xFF;
            states->currentDeviceContext.stroke_green = 0xFF;
            states->currentDeviceContext.stroke_mode = U_PS_SOLID;
            break;
        case (U_BLACK_PEN):
            states->currentDeviceContext.stroke_red = 0x00;
            states->currentDeviceContext.stroke_blue = 0x00;
            states->currentDeviceContext.stroke_green = 0x00;
            states->currentDeviceContext.stroke_mode = U_PS_SOLID;
            break;
        case (U_NULL_PEN):
            states->currentDeviceContext.stroke_mode = U_PS_NULL;
            break;
        case (U_OEM_FIXED_FONT):
            break;
        case (U_ANSI_FIXED_FONT):
            break;
        case (U_ANSI_VAR_FONT):
            break;
        case (U_SYSTEM_FONT):
            break;
        case (U_DEVICE_DEFAULT_FONT):
            break;
        case (U_DEFAULT_PALETTE):
            break;
        case (U_SYSTEM_FIXED_FONT):
            break;
        case (U_DEFAULT_GUI_FONT):
            break;
        default:
            break;
        }
    } else {
        returnOutOfOTIndex(index);
        if (states->objectTable[index].fill_set) {
            states->currentDeviceContext.fill_red =
                states->objectTable[index].fill_red;
            states->currentDeviceContext.fill_blue =
                states->objectTable[index].fill_blue;
            states->currentDeviceContext.fill_green =
                states->objectTable[index].fill_green;
            states->currentDeviceContext.fill_mode =
                states->objectTable[index].fill_mode;
            states->currentDeviceContext.fill_idx =
                states->objectTable[index].fill_idx;
        } else if (states->objectTable[index].stroke_set) {
            states->currentDeviceContext.stroke_red =
                states->objectTable[index].stroke_red;
            states->currentDeviceContext.stroke_blue =
                states->objectTable[index].stroke_blue;
            states->currentDeviceContext.stroke_green =
                states->objectTable[index].stroke_green;
            states->currentDeviceContext.stroke_mode =
                states->objectTable[index].stroke_mode;
            states->currentDeviceContext.stroke_width =
                states->objectTable[index].stroke_width;
        } else if (states->objectTable[index].font_set) {
            states->currentDeviceContext.font_width =
                states->objectTable[index].font_width;
            states->currentDeviceContext.font_height =
                states->objectTable[index].font_height;
            states->currentDeviceContext.font_weight =
                states->objectTable[index].font_weight;
            states->currentDeviceContext.font_italic =
                states->objectTable[index].font_italic;
            states->currentDeviceContext.font_underline =
                states->objectTable[index].font_underline;
            states->currentDeviceContext.font_strikeout =
                states->objectTable[index].font_strikeout;
            states->currentDeviceContext.font_escapement =
                states->objectTable[index].font_escapement;
            states->currentDeviceContext.font_orientation =
                states->objectTable[index].font_orientation;
            states->currentDeviceContext.font_charset =
                states->objectTable[index].font_charset;

            if (states->currentDeviceContext.font_name != NULL) {
                free(states->currentDeviceContext.font_name);
                states->currentDeviceContext.font_name = NULL;
            }
            if (states->objectTable[index].font_name != NULL) {
                size_t len = strlen(states->objectTable[index].font_name);
                states->currentDeviceContext.font_name =
                    calloc((len + 1), sizeof(char));
                strcpy(states->currentDeviceContext.font_name,
                       states->objectTable[index].font_name);
            }
            if (states->currentDeviceContext.font_family != NULL) {
                free(states->currentDeviceContext.font_family);
                states->currentDeviceContext.font_family = NULL;
            }
            if (states->objectTable[index].font_family != NULL) {
                size_t len = strlen(states->objectTable[index].font_family);
                states->currentDeviceContext.font_family =
                    calloc((len + 1), sizeof(char));
                strcpy(states->currentDeviceContext.font_family,
                       states->objectTable[index].font_family);
            }
        }
    }
}
void U_EMRSELECTPALETTE_draw(const char *contents, FILE *out,
                             drawingStates *states) {
    FLAG_IGNORED;
    if (states->verbose) {
        U_EMRSELECTPALETTE_print(contents, states);
    }
}
void U_EMRSETCOLORSPACE_draw(const char *contents, FILE *out,
                             drawingStates *states) {
    FLAG_IGNORED;
    if (states->verbose) {
        U_EMRSETCOLORSPACE_print(contents, states);
    }
}
void U_EMRSETPALETTEENTRIES_draw(const char *contents, FILE *out,
                                 drawingStates *states) {
    FLAG_IGNORED;
    if (states->verbose) {
        U_EMRSETPALETTEENTRIES_print(contents, states);
    }
    // PU_EMRSETPALETTEENTRIES pEmr = (PU_EMRSETPALETTEENTRIES)(contents);
}

#ifdef __cplusplus
}
#endif
/* vim:set shiftwidth=2 softtabstop=2 expandtab: */
