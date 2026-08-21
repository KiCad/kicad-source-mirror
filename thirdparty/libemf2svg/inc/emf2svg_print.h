/**
  @file uemf_print.h

  @brief Prototypes for functions for printing records from EMF files.
  */

/*
File:      uemf_print.h
Version:   0.0.5
Date:      14-FEB-2013
Author:    David Mathog, Biology Division, Caltech
email:     mathog@caltech.edu
Copyright: 2013 David Mathog and California Institute of Technology (Caltech)
*/

#ifdef __cplusplus
extern "C" {
#endif

#include "uemf.h"
#include <stddef.h> /* for offsetof() macro */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define KNRM "\x1B[0m"
#define KRED "\x1B[31m"
#define KGRN "\x1B[32m"
#define KYEL "\x1B[33m"
#define KBLU "\x1B[34m"
#define KMAG "\x1B[35m"
#define KCYN "\x1B[36m"
#define KWHT "\x1B[37m"

#define verbose_printf(...)                                                    \
    if (states->verbose)                                                       \
        printf(__VA_ARGS__);
#define FLAG_SUPPORTED                                                         \
    verbose_printf("   Status:         %sSUPPORTED%s\n", KGRN, KNRM);
#define FLAG_IGNORED                                                           \
    verbose_printf("   Status:         %sIGNORED%s\n", KRED, KNRM);
#define FLAG_PARTIAL                                                           \
    verbose_printf("   Status:         %sPARTIAL SUPPORT%s\n", KYEL, KNRM);
#define FLAG_UNUSED                                                            \
    verbose_printf("   Status:         %sUNUSED%s\n", KMAG, KNRM);
#define FLAG_RESET verbose_printf("%s", KNRM);

#define U_MWT_SET 4 //!< Transform is basic SET

#define BUFFERSIZE 1024
//! \cond

/* manipulate device context */

void stroke_print(drawingStates *states);
void fill_print(drawingStates *states);
void point16_print(drawingStates *states, U_POINT16 pt);
void point_print(drawingStates *states, U_POINT pt);

/* prototypes for objects used in EMR records */
void hexbytes_print(drawingStates *states, uint8_t *buf, unsigned int num);
void colorref_print(drawingStates *states, U_COLORREF color);
void rgbquad_print(drawingStates *states, U_RGBQUAD color);
void rectl_print(drawingStates *states, U_RECTL rect);
void sizel_print(drawingStates *states, U_SIZEL sz);
void pointl_print(drawingStates *states, U_POINTL pt);
void lcs_gamma_print(drawingStates *states, U_LCS_GAMMA lg);
void lcs_gammargb_print(drawingStates *states, U_LCS_GAMMARGB lgr);
void trivertex_print(drawingStates *states, U_TRIVERTEX tv);
void gradient3_print(drawingStates *states, U_GRADIENT3 g3);
void gradient4_print(drawingStates *states, U_GRADIENT4 g4);
void logbrush_print(drawingStates *states, U_LOGBRUSH lb);
void xform_print(drawingStates *states, U_XFORM xform);
void ciexyz_print(drawingStates *states, U_CIEXYZ ciexyz);
void ciexyztriple_print(drawingStates *states, U_CIEXYZTRIPLE cie3);
void logcolorspacea_print(drawingStates *states, U_LOGCOLORSPACEA lcsa);
void logcolorspacew_print(drawingStates *states, U_LOGCOLORSPACEW lcsa);
void panose_print(drawingStates *states, U_PANOSE panose);
void logfont_print(drawingStates *states, U_LOGFONT lf);
void logfont_panose_print(drawingStates *states, U_LOGFONT_PANOSE lfp);
int bitmapinfoheader_print(drawingStates *states, const char *Bmih);
void bitmapinfo_print(drawingStates *states, const char *Bmi,
                      const char *blimit);
void blend_print(drawingStates *states, U_BLEND blend);
void extlogpen_print(drawingStates *states, const PU_EXTLOGPEN elp);
void logpen_print(drawingStates *states, U_LOGPEN lp);
void logpltntry_print(drawingStates *states, U_LOGPLTNTRY lpny);
void logpalette_print(drawingStates *states, const PU_LOGPALETTE lp);
void rgndataheader_print(drawingStates *states, U_RGNDATAHEADER rdh);
void rgndata_print(drawingStates *states, const PU_RGNDATA rd,
                   const char *blimit);
void coloradjustment_print(drawingStates *states, U_COLORADJUSTMENT ca);
void pixelformatdescriptor_print(drawingStates *states,
                                 U_PIXELFORMATDESCRIPTOR pfd);
void emrtext_print(drawingStates *states, const char *emt, const char *record,
                   const char *blimit, int type);

/* prototypes for EMR records */
void U_EMRNOTIMPLEMENTED_print(const char *name, const char *contents,
                               drawingStates *states);
void U_EMRHEADER_print(const char *contents, drawingStates *states);
void U_EMRPOLYBEZIER_print(const char *contents, drawingStates *states);
void U_EMRPOLYGON_print(const char *contents, drawingStates *states);
void U_EMRPOLYLINE_print(const char *contents, drawingStates *states);
void U_EMRPOLYBEZIERTO_print(const char *contents, drawingStates *states);
void U_EMRPOLYLINETO_print(const char *contents, drawingStates *states);
void U_EMRPOLYPOLYLINE_print(const char *contents, drawingStates *states);
void U_EMRPOLYPOLYGON_print(const char *contents, drawingStates *states);
void U_EMRSETWINDOWEXTEX_print(const char *contents, drawingStates *states);
void U_EMRSETWINDOWORGEX_print(const char *contents, drawingStates *states);
void U_EMRSETVIEWPORTEXTEX_print(const char *contents, drawingStates *states);
void U_EMRSETVIEWPORTORGEX_print(const char *contents, drawingStates *states);
void U_EMRSETBRUSHORGEX_print(const char *contents, drawingStates *states);
void U_EMREOF_print(const char *contents, drawingStates *states);
void U_EMRSETPIXELV_print(const char *contents, drawingStates *states);
void U_EMRSETMAPPERFLAGS_print(const char *contents, drawingStates *states);
void U_EMRSETMAPMODE_print(const char *contents, drawingStates *states);
void U_EMRSETBKMODE_print(const char *contents, drawingStates *states);
void U_EMRSETPOLYFILLMODE_print(const char *contents, drawingStates *states);
void U_EMRSETROP2_print(const char *contents, drawingStates *states);
void U_EMRSETSTRETCHBLTMODE_print(const char *contents, drawingStates *states);
void U_EMRSETTEXTALIGN_print(const char *contents, drawingStates *states);
void U_EMRSETCOLORADJUSTMENT_print(const char *contents, drawingStates *states);
void U_EMRSETTEXTCOLOR_print(const char *contents, drawingStates *states);
void U_EMRSETBKCOLOR_print(const char *contents, drawingStates *states);
void U_EMROFFSETCLIPRGN_print(const char *contents, drawingStates *states);
void U_EMRMOVETOEX_print(const char *contents, drawingStates *states);
void U_EMRSETMETARGN_print(const char *contents, drawingStates *states);
void U_EMREXCLUDECLIPRECT_print(const char *contents, drawingStates *states);
void U_EMRINTERSECTCLIPRECT_print(const char *contents, drawingStates *states);
void U_EMRSCALEVIEWPORTEXTEX_print(const char *contents, drawingStates *states);
void U_EMRSCALEWINDOWEXTEX_print(const char *contents, drawingStates *states);
void U_EMRSAVEDC_print(const char *contents, drawingStates *states);
void U_EMRRESTOREDC_print(const char *contents, drawingStates *states);
void U_EMRSETWORLDTRANSFORM_print(const char *contents, drawingStates *states);
void U_EMRMODIFYWORLDTRANSFORM_print(const char *contents,
                                     drawingStates *states);
void U_EMRSELECTOBJECT_print(const char *contents, drawingStates *states);
void U_EMRCREATEPEN_print(const char *contents, drawingStates *states);
void U_EMRCREATEBRUSHINDIRECT_print(const char *contents,
                                    drawingStates *states);
void U_EMRDELETEOBJECT_print(const char *contents, drawingStates *states);
void U_EMRANGLEARC_print(const char *contents, drawingStates *states);
void U_EMRELLIPSE_print(const char *contents, drawingStates *states);
void U_EMRRECTANGLE_print(const char *contents, drawingStates *states);
void U_EMRROUNDRECT_print(const char *contents, drawingStates *states);
void U_EMRARC_print(const char *contents, drawingStates *states);
void U_EMRCHORD_print(const char *contents, drawingStates *states);
void U_EMRPIE_print(const char *contents, drawingStates *states);
void U_EMRSELECTPALETTE_print(const char *contents, drawingStates *states);
void U_EMRCREATEPALETTE_print(const char *contents, drawingStates *states);
void U_EMRSETPALETTEENTRIES_print(const char *contents, drawingStates *states);
void U_EMRRESIZEPALETTE_print(const char *contents, drawingStates *states);
void U_EMRREALIZEPALETTE_print(const char *contents, drawingStates *states);
void U_EMREXTFLOODFILL_print(const char *contents, drawingStates *states);
void U_EMRLINETO_print(const char *contents, drawingStates *states);
void U_EMRARCTO_print(const char *contents, drawingStates *states);
void U_EMRPOLYDRAW_print(const char *contents, drawingStates *states);
void U_EMRSETARCDIRECTION_print(const char *contents, drawingStates *states);
void U_EMRSETMITERLIMIT_print(const char *contents, drawingStates *states);
void U_EMRBEGINPATH_print(const char *contents, drawingStates *states);
void U_EMRENDPATH_print(const char *contents, drawingStates *states);
void U_EMRCLOSEFIGURE_print(const char *contents, drawingStates *states);
void U_EMRFILLPATH_print(const char *contents, drawingStates *states);
void U_EMRSTROKEANDFILLPATH_print(const char *contents, drawingStates *states);
void U_EMRSTROKEPATH_print(const char *contents, drawingStates *states);
void U_EMRFLATTENPATH_print(const char *contents, drawingStates *states);
void U_EMRWIDENPATH_print(const char *contents, drawingStates *states);
void U_EMRSELECTCLIPPATH_print(const char *contents, drawingStates *states);
void U_EMRABORTPATH_print(const char *contents, drawingStates *states);
void U_EMRCOMMENT_print(const char *contents, drawingStates *states,
                        const char *blimit, size_t off);
void U_EMRFILLRGN_print(const char *contents, drawingStates *states);
void U_EMRFRAMERGN_print(const char *contents, drawingStates *states);
void U_EMRINVERTRGN_print(const char *contents, drawingStates *states);
void U_EMRPAINTRGN_print(const char *contents, drawingStates *states);
void U_EMREXTSELECTCLIPRGN_print(const char *contents, drawingStates *states);
void U_EMRBITBLT_print(const char *contents, drawingStates *states);
void U_EMRSTRETCHBLT_print(const char *contents, drawingStates *states);
void U_EMRMASKBLT_print(const char *contents, drawingStates *states);
void U_EMRPLGBLT_print(const char *contents, drawingStates *states);
void U_EMRSETDIBITSTODEVICE_print(const char *contents, drawingStates *states);
void U_EMRSTRETCHDIBITS_print(const char *contents, drawingStates *states);
void U_EMREXTCREATEFONTINDIRECTW_print(const char *contents,
                                       drawingStates *states);
void U_EMREXTTEXTOUTA_print(const char *contents, drawingStates *states);
void U_EMREXTTEXTOUTW_print(const char *contents, drawingStates *states);
void U_EMRPOLYBEZIER16_print(const char *contents, drawingStates *states);
void U_EMRPOLYGON16_print(const char *contents, drawingStates *states);
void U_EMRPOLYLINE16_print(const char *contents, drawingStates *states);
void U_EMRPOLYBEZIERTO16_print(const char *contents, drawingStates *states);
void U_EMRPOLYLINETO16_print(const char *contents, drawingStates *states);
void U_EMRPOLYPOLYLINE16_print(const char *contents, drawingStates *states);
void U_EMRPOLYPOLYGON16_print(const char *contents, drawingStates *states);
void U_EMRPOLYDRAW16_print(const char *contents, drawingStates *states);
void U_EMRCREATEMONOBRUSH_print(const char *contents, drawingStates *states);
void U_EMRCREATEDIBPATTERNBRUSHPT_print(const char *contents,
                                        drawingStates *states);
void U_EMREXTCREATEPEN_print(const char *contents, drawingStates *states);
void U_EMRSETICMMODE_print(const char *contents, drawingStates *states);
void U_EMRCREATECOLORSPACE_print(const char *contents, drawingStates *states);
void U_EMRSETCOLORSPACE_print(const char *contents, drawingStates *states);
void U_EMRDELETECOLORSPACE_print(const char *contents, drawingStates *states);
void U_EMRPIXELFORMAT_print(const char *contents, drawingStates *states);
void U_EMRSMALLTEXTOUT_print(const char *contents, drawingStates *states);
void U_EMRALPHABLEND_print(const char *contents, drawingStates *states);
void U_EMRSETLAYOUT_print(const char *contents, drawingStates *states);
void U_EMRTRANSPARENTBLT_print(const char *contents, drawingStates *states);
void U_EMRGRADIENTFILL_print(const char *contents, drawingStates *states);
void U_EMRCREATECOLORSPACEW_print(const char *contents, drawingStates *states);
void U_emf_onerec_print(const char *contents, const char *blimit, int recnum,
                        size_t off, drawingStates *states);
//! \endcond

#ifdef __cplusplus
}
#endif

/* vim:set shiftwidth=2 softtabstop=2 expandtab: */
