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

void U_EMRCREATEBRUSHINDIRECT_draw(const char *contents, FILE *out,
                                   drawingStates *states) {
    FLAG_SUPPORTED;
    if (states->verbose) {
        U_EMRCREATEBRUSHINDIRECT_print(contents, states);
    }
    PU_EMRCREATEBRUSHINDIRECT pEmr = (PU_EMRCREATEBRUSHINDIRECT)(contents);

    uint16_t index = pEmr->ihBrush;
    returnOutOfOTIndex(index);
    if (pEmr->lb.lbStyle == U_BS_SOLID) {
        states->objectTable[index].fill_red = pEmr->lb.lbColor.Red;
        states->objectTable[index].fill_green = pEmr->lb.lbColor.Green;
        states->objectTable[index].fill_blue = pEmr->lb.lbColor.Blue;
        states->objectTable[index].fill_mode = U_BS_SOLID;
        states->objectTable[index].fill_set = true;
    } else if (pEmr->lb.lbStyle == U_BS_HATCHED) {
        states->objectTable[index].fill_recidx =
            pEmr->ihBrush; // used if the hatch needs to be redone due to
                           // bkMode, textmode, etc. changes
        states->objectTable[index].fill_red = pEmr->lb.lbColor.Red;
        states->objectTable[index].fill_green = pEmr->lb.lbColor.Green;
        states->objectTable[index].fill_blue = pEmr->lb.lbColor.Blue;
        states->objectTable[index].fill_mode = U_BS_HATCHED;
        states->objectTable[index].fill_set = true;
    } else {
        states->objectTable[index].fill_red = pEmr->lb.lbColor.Red;
        states->objectTable[index].fill_green = pEmr->lb.lbColor.Green;
        states->objectTable[index].fill_blue = pEmr->lb.lbColor.Blue;
        states->objectTable[index].fill_mode = pEmr->lb.lbStyle;
        states->objectTable[index].fill_set = true;
    }
}
void U_EMRCREATECOLORSPACE_draw(const char *contents, FILE *out,
                                drawingStates *states) {
    FLAG_IGNORED;
    if (states->verbose) {
        U_EMRCREATECOLORSPACE_print(contents, states);
    }
    // PU_EMRCREATECOLORSPACE pEmr = (PU_EMRCREATECOLORSPACE)(contents);
}
void U_EMRCREATECOLORSPACEW_draw(const char *contents, FILE *out,
                                 drawingStates *states) {
    FLAG_IGNORED;
    if (states->verbose) {
        U_EMRCREATECOLORSPACEW_print(contents, states);
    }
    // PU_EMRCREATECOLORSPACEW pEmr = (PU_EMRCREATECOLORSPACEW)(contents);
}
void U_EMRCREATEDIBPATTERNBRUSHPT_draw(const char *contents, FILE *out,
                                       drawingStates *states) {
    PU_EMRCREATEMONOBRUSH pEmr = (PU_EMRCREATEMONOBRUSH)(contents);

    // check that the header is not outside of the emf file
    returnOutOfEmf(contents + pEmr->offBmi);
    returnOutOfEmf(contents + pEmr->offBmi + sizeof(U_BITMAPINFOHEADER));

    // get the header
    PU_BITMAPINFOHEADER BmiSrc = (PU_BITMAPINFOHEADER)(contents + pEmr->offBmi);

    // check that the bitmap is not outside the emf file
    returnOutOfEmf(contents + pEmr->offBits);
    returnOutOfEmf(contents + pEmr->offBits + pEmr->cbBits);

    const unsigned char *BmpSrc =
        (const unsigned char *)(contents + pEmr->offBits);
    emfImageLibrary *image = image_library_writer(contents, out, states, BmiSrc,
                                                  pEmr->cbBits, BmpSrc);
    if (image) {
        // draw image;
        uint16_t index = pEmr->ihBrush;
        returnOutOfOTIndex(index);
        states->objectTable[index].fill_idx = image->id;
        states->objectTable[index].fill_red =
            states->currentDeviceContext.fill_red;
        states->objectTable[index].fill_green =
            states->currentDeviceContext.fill_green;
        states->objectTable[index].fill_blue =
            states->currentDeviceContext.fill_blue;
        states->objectTable[index].fill_mode = U_BS_MONOPATTERN;
        states->objectTable[index].fill_set = true;
    }
    FLAG_SUPPORTED;
    if (states->verbose) {
        U_EMRCREATEDIBPATTERNBRUSHPT_print(contents, states);
    }
}
void U_EMRCREATEMONOBRUSH_draw(const char *contents, FILE *out,
                               drawingStates *states) {
    PU_EMRCREATEMONOBRUSH pEmr = (PU_EMRCREATEMONOBRUSH)(contents);

    // check that the header is not outside of the emf file
    returnOutOfEmf(contents + pEmr->offBmi);
    returnOutOfEmf(contents + pEmr->offBmi + sizeof(U_BITMAPINFOHEADER));

    // get the header
    PU_BITMAPINFOHEADER BmiSrc = (PU_BITMAPINFOHEADER)(contents + pEmr->offBmi);

    // check that the bitmap is not outside the emf file
    returnOutOfEmf(contents + pEmr->offBits);
    returnOutOfEmf(contents + pEmr->offBits + pEmr->cbBits);

    const unsigned char *BmpSrc =
        (const unsigned char *)(contents + pEmr->offBits);
    emfImageLibrary *image = image_library_writer(contents, out, states, BmiSrc,
                                                  pEmr->cbBits, BmpSrc);
    if (image) {
        // draw image;
        uint16_t index = pEmr->ihBrush;
        returnOutOfOTIndex(index);
        states->objectTable[index].fill_idx = image->id;
        states->objectTable[index].fill_red =
            states->currentDeviceContext.fill_red;
        states->objectTable[index].fill_green =
            states->currentDeviceContext.fill_green;
        states->objectTable[index].fill_blue =
            states->currentDeviceContext.fill_blue;
        states->objectTable[index].fill_mode = U_BS_MONOPATTERN;
        states->objectTable[index].fill_set = true;
    }
    FLAG_SUPPORTED;
    if (states->verbose) {
        U_EMRCREATEMONOBRUSH_print(contents, states);
    }
}
void U_EMRCREATEPALETTE_draw(const char *contents, FILE *out,
                             drawingStates *states) {
    FLAG_IGNORED;
    if (states->verbose) {
        U_EMRCREATEPALETTE_print(contents, states);
    }
    // PU_EMRCREATEPALETTE pEmr = (PU_EMRCREATEPALETTE)(contents);
}
void U_EMRCREATEPEN_draw(const char *contents, FILE *out,
                         drawingStates *states) {
    FLAG_PARTIAL;
    if (states->verbose) {
        U_EMRCREATEPEN_print(contents, states);
    }

    PU_EMRCREATEPEN pEmr = (PU_EMRCREATEPEN)(contents);

    uint32_t index = pEmr->ihPen;
    returnOutOfOTIndex(index);
    states->objectTable[index].stroke_set = true;
    states->objectTable[index].stroke_red = pEmr->lopn.lopnColor.Red;
    states->objectTable[index].stroke_blue = pEmr->lopn.lopnColor.Blue;
    states->objectTable[index].stroke_green = pEmr->lopn.lopnColor.Green;
    states->objectTable[index].stroke_mode = pEmr->lopn.lopnStyle;
    states->objectTable[index].stroke_width = pEmr->lopn.lopnWidth.x;
}
void U_EMREXTCREATEFONTINDIRECTW_draw(const char *contents, FILE *out,
                                      drawingStates *states) {
    FLAG_PARTIAL;
    if (states->verbose) {
        U_EMREXTCREATEFONTINDIRECTW_print(contents, states);
    }
    PU_EMREXTCREATEFONTINDIRECTW pEmr =
        (PU_EMREXTCREATEFONTINDIRECTW)(contents);
    uint16_t index = pEmr->ihFont;
    returnOutOfOTIndex(index);
    if (states->objectTable[index].font_name != NULL) {
        free(states->objectTable[index].font_name);
        states->objectTable[index].font_name = NULL;
    }

    if (states->objectTable[index].font_family != NULL) {
        free(states->objectTable[index].font_family);
        states->objectTable[index].font_family = NULL;
    }

    U_LOGFONT logfont;

    if (pEmr->emr.nSize ==
        sizeof(U_EMREXTCREATEFONTINDIRECTW)) { // holds logfont_panose
        U_LOGFONT_PANOSE lfp = pEmr->elfw;
        logfont = pEmr->elfw.elfLogFont;
        char *fullname =
            U_Utf16leToUtf8(lfp.elfFullName, U_LF_FULLFACESIZE, NULL);
        states->objectTable[index].font_name = fullname;
    } else { // holds logfont
        logfont = *(PU_LOGFONT) & (pEmr->elfw);
    }
    char *family = U_Utf16leToUtf8(logfont.lfFaceName, U_LF_FACESIZE, NULL);
    states->objectTable[index].font_width = abs(logfont.lfWidth);
    states->objectTable[index].font_height = abs(logfont.lfHeight);
    states->objectTable[index].font_weight = logfont.lfWeight;
    states->objectTable[index].font_italic = logfont.lfItalic;
    states->objectTable[index].font_underline = logfont.lfUnderline;
    states->objectTable[index].font_strikeout = logfont.lfStrikeOut;
    states->objectTable[index].font_escapement = (logfont.lfEscapement % 3600);
    states->objectTable[index].font_orientation =
        (logfont.lfOrientation % 3600);
    states->objectTable[index].font_family = family;
    states->objectTable[index].font_set = true;
    states->objectTable[index].font_charset = logfont.lfCharSet;
}
void U_EMREXTCREATEPEN_draw(const char *contents, FILE *out,
                            drawingStates *states) {
    FLAG_PARTIAL;
    if (states->verbose) {
        U_EMREXTCREATEPEN_print(contents, states);
    }
    PU_EMREXTCREATEPEN pEmr = (PU_EMREXTCREATEPEN)(contents);
    uint32_t index = pEmr->ihPen;
    returnOutOfOTIndex(index);

    PU_EXTLOGPEN pen = (PU_EXTLOGPEN) & (pEmr->elp);
    states->objectTable[index].stroke_set = true;
    states->objectTable[index].stroke_red = pen->elpColor.Red;
    states->objectTable[index].stroke_blue = pen->elpColor.Blue;
    states->objectTable[index].stroke_green = pen->elpColor.Green;
    states->objectTable[index].stroke_mode = pen->elpPenStyle;
    states->objectTable[index].stroke_width = pen->elpWidth;
}

#ifdef __cplusplus
}
#endif
/* vim:set shiftwidth=2 softtabstop=2 expandtab: */
