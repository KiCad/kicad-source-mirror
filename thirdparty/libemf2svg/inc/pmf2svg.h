/* vim:set shiftwidth=2 softtabstop=2 expandtab: */
/**
  @file upmf_draw.h

  @brief Prototypes for functions for printing records from EMF files.
  */

/*
File:      upmf_draw.h
Version:   0.0.2
Date:      17-OCT-2013
Author:    David Mathog, Biology Division, Caltech
email:     mathog@caltech.edu
Copyright: 2013 David Mathog and California Institute of Technology (Caltech)
*/

#ifndef _UPMF_draw_
#define _UPMF_draw_

#define returnOutOfEmfP(a)                                                     \
    if (checkOutOfEMF(states, (void *)(a))) {                                  \
        return (0)                                                             \
    }

#ifdef __cplusplus
extern "C" {
#endif

#include "upmf.h" /* includes uemf.h */

/* prototypes for simple types and enums used in PMR records */
int U_PMF_CMN_HDR_draw(U_PMF_CMN_HDR Header, int precnum, int off, FILE *out,
                       drawingStates *states);
int U_PMF_UINT8_ARRAY_draw(const char *Start, const uint8_t *Array,
                           int Elements, char *End, FILE *out,
                           drawingStates *states);
int U_PMF_BRUSHTYPEENUMERATION_draw(int otype, FILE *out,
                                    drawingStates *states);
int U_PMF_HATCHSTYLEENUMERATION_draw(int hstype, FILE *out,
                                     drawingStates *states);
int U_PMF_OBJECTTYPEENUMERATION_draw(int otype, FILE *out,
                                     drawingStates *states);
int U_PMF_PATHPOINTTYPE_ENUM_draw(int Type, FILE *out, drawingStates *states);
int U_PMF_PX_FMT_ENUM_draw(int pfe, FILE *out, drawingStates *states);
int U_PMF_NODETYPE_draw(int Type, FILE *out, drawingStates *states);

/* prototypes for objects used in PMR records */
int U_PMF_BRUSH_draw(const char *contents, FILE *out, drawingStates *states);
int U_PMF_CUSTOMLINECAP_draw(const char *contents, const char *Which, FILE *out,
                             drawingStates *states);
int U_PMF_FONT_draw(const char *contents, FILE *out, drawingStates *states);
int U_PMF_IMAGE_draw(const char *contents, FILE *out, drawingStates *states);
int U_PMF_IMAGEATTRIBUTES_draw(const char *contents, FILE *out,
                               drawingStates *states);
int U_PMF_PATH_draw(const char *contents, FILE *out, drawingStates *states);
int U_PMF_PEN_draw(const char *contents, FILE *out, drawingStates *states);
int U_PMF_REGION_draw(const char *contents, FILE *out, drawingStates *states);
int U_PMF_STRINGFORMAT_draw(const char *contents, FILE *out,
                            drawingStates *states);
int U_PMF_ARGB_draw(const char *contents, FILE *out, drawingStates *states);
int U_PMF_BITMAP_draw(const char *contents, FILE *out, drawingStates *states);
int U_PMF_BITMAPDATA_draw(const char *contents, FILE *out,
                          drawingStates *states);
int U_PMF_BLENDCOLORS_draw(const char *contents, FILE *out,
                           drawingStates *states);
int U_PMF_BLENDFACTORS_draw(const char *contents, const char *type, FILE *out,
                            drawingStates *states);
int U_PMF_BOUNDARYPATHDATA_draw(const char *contents, FILE *out,
                                drawingStates *states);
int U_PMF_BOUNDARYPOINTDATA_draw(const char *contents, FILE *out,
                                 drawingStates *states);
int U_PMF_CHARACTERRANGE_draw(const char *contents, FILE *out,
                              drawingStates *states);
int U_PMF_COMPOUNDLINEDATA_draw(const char *contents, FILE *out,
                                drawingStates *states);
int U_PMF_COMPRESSEDIMAGE_draw(const char *contents, FILE *out,
                               drawingStates *states);
int U_PMF_CUSTOMENDCAPDATA_draw(const char *contents, FILE *out,
                                drawingStates *states);
int U_PMF_CUSTOMLINECAPARROWDATA_draw(const char *contents, FILE *out,
                                      drawingStates *states);
int U_PMF_CUSTOMLINECAPDATA_draw(const char *contents, FILE *out,
                                 drawingStates *states);
int U_PMF_CUSTOMLINECAPOPTIONALDATA_draw(const char *contents, uint32_t Flags,
                                         FILE *out, drawingStates *states);
int U_PMF_CUSTOMSTARTCAPDATA_draw(const char *contents, FILE *out,
                                  drawingStates *states);
int U_PMF_DASHEDLINEDATA_draw(const char *contents, FILE *out,
                              drawingStates *states);
int U_PMF_FILLPATHOBJ_draw(const char *contents, FILE *out,
                           drawingStates *states);
int U_PMF_FOCUSSCALEDATA_draw(const char *contents, FILE *out,
                              drawingStates *states);
int U_PMF_GRAPHICSVERSION_draw(const char *contents, FILE *out,
                               drawingStates *states);
int U_PMF_HATCHBRUSHDATA_draw(const char *contents, FILE *out,
                              drawingStates *states);
int U_PMF_LANGUAGEIDENTIFIER_draw(U_PMF_LANGUAGEIDENTIFIER LId, FILE *out,
                                  drawingStates *states);
int U_PMF_LINEARGRADIENTBRUSHDATA_draw(const char *contents, FILE *out,
                                       drawingStates *states);
int U_PMF_LINEARGRADIENTBRUSHOPTIONALDATA_draw(const char *contents, int BDFlag,
                                               FILE *out,
                                               drawingStates *states);
int U_PMF_LINEPATH_draw(const char *contents, FILE *out, drawingStates *states);
int U_PMF_METAFILE_draw(const char *contents, FILE *out, drawingStates *states);
int U_PMF_PALETTE_draw(const char *contents, FILE *out, drawingStates *states);
int U_PMF_PATHGRADIENTBRUSHDATA_draw(const char *contents, FILE *out,
                                     drawingStates *states);
int U_PMF_PATHGRADIENTBRUSHOPTIONALDATA_draw(const char *contents, int BDFlag,
                                             FILE *out, drawingStates *states);
int U_PMF_PATHPOINTTYPE_draw(const char *contents, FILE *out,
                             drawingStates *states);
int U_PMF_PATHPOINTTYPERLE_draw(const char *contents, FILE *out,
                                drawingStates *states);
int U_PMF_PENDATA_draw(const char *contents, FILE *out, drawingStates *states);
int U_PMF_PENOPTIONALDATA_draw(const char *contents, int Flags, FILE *out,
                               drawingStates *states);
int U_PMF_POINT_draw(const char **contents, FILE *out, drawingStates *states);
int U_PMF_POINTF_draw(const char **contents, FILE *out, drawingStates *states);
int U_PMF_POINTR_draw(const char **contents, U_FLOAT *Xpos, U_FLOAT *Ypos,
                      FILE *out, drawingStates *states);
int U_PMF_POINT_S_draw(U_PMF_POINT *Point, FILE *out, drawingStates *states);
int U_PMF_POINTF_S_draw(U_PMF_POINTF *Point, FILE *out, drawingStates *states);
int U_PMF_RECT_draw(const char **contents, FILE *out, drawingStates *states);
int U_PMF_RECTF_draw(const char **contents, FILE *out, drawingStates *states);
int U_PMF_RECT_S_draw(U_PMF_RECT *Rect, FILE *out, drawingStates *states);
int U_PMF_RECTF_S_draw(U_PMF_RECTF *Rect, FILE *out, drawingStates *states);
int U_PMF_REGIONNODE_draw(const char *contents, int Level, FILE *out,
                          drawingStates *states);
int U_PMF_REGIONNODECHILDNODES_draw(const char *contents, int Level, FILE *out,
                                    drawingStates *states);
int U_PMF_REGIONNODEPATH_draw(const char *contents, FILE *out,
                              drawingStates *states);
int U_PMF_SOLIDBRUSHDATA_draw(const char *contents, FILE *out,
                              drawingStates *states);
int U_PMF_STRINGFORMATDATA_draw(const char *contents, uint32_t TabStopCount,
                                uint32_t RangeCount, FILE *out,
                                drawingStates *states);
int U_PMF_TEXTUREBRUSHDATA_draw(const char *contents, FILE *out,
                                drawingStates *states);
int U_PMF_TEXTUREBRUSHOPTIONALDATA_draw(const char *contents, int HasImage,
                                        FILE *out, drawingStates *states);
int U_PMF_TRANSFORMMATRIX_draw(const char *contents, FILE *out,
                               drawingStates *states);
int U_PMF_TRANSFORMMATRIX2_draw(U_PMF_TRANSFORMMATRIX *Matrix, FILE *out,
                                drawingStates *states);
int U_PMF_ROTMATRIX2_draw(U_PMF_ROTMATRIX *Matrix, FILE *out,
                          drawingStates *states);
int U_PMF_IE_BLUR_draw(const char *contents, FILE *out, drawingStates *states);
int U_PMF_IE_BRIGHTNESSCONTRAST_draw(const char *contents, FILE *out,
                                     drawingStates *states);
int U_PMF_IE_COLORBALANCE_draw(const char *contents, FILE *out,
                               drawingStates *states);
int U_PMF_IE_COLORCURVE_draw(const char *contents, FILE *out,
                             drawingStates *states);
int U_PMF_IE_COLORLOOKUPTABLE_draw(const char *contents, FILE *out,
                                   drawingStates *states);
int U_PMF_IE_COLORMATRIX_draw(const char *contents, FILE *out,
                              drawingStates *states);
int U_PMF_IE_HUESATURATIONLIGHTNESS_draw(const char *contents, FILE *out,
                                         drawingStates *states);
int U_PMF_IE_LEVELS_draw(const char *contents, FILE *out,
                         drawingStates *states);
int U_PMF_IE_REDEYECORRECTION_draw(const char *contents, FILE *out,
                                   drawingStates *states);
int U_PMF_IE_SHARPEN_draw(const char *contents, FILE *out,
                          drawingStates *states);
int U_PMF_IE_TINT_draw(const char *contents, FILE *out, drawingStates *states);

/* prototypes for PMR records */
int U_PMR_OFFSETCLIP_draw(const char *contents, FILE *out,
                          drawingStates *states);
int U_PMR_RESETCLIP_draw(const char *contents, FILE *out,
                         drawingStates *states);
int U_PMR_SETCLIPPATH_draw(const char *contents, FILE *out,
                           drawingStates *states);
int U_PMR_SETCLIPRECT_draw(const char *contents, FILE *out,
                           drawingStates *states);
int U_PMR_SETCLIPREGION_draw(const char *contents, FILE *out,
                             drawingStates *states);
int U_PMR_COMMENT_draw(const char *contents, FILE *out, drawingStates *states);
int U_PMR_ENDOFFILE_draw(const char *contents, FILE *out,
                         drawingStates *states);
int U_PMR_GETDC_draw(const char *contents, FILE *out, drawingStates *states);
int U_PMR_HEADER_draw(const char *contents, FILE *out, drawingStates *states);
int U_PMR_CLEAR_draw(const char *contents, FILE *out, drawingStates *states);
int U_PMR_DRAWARC_draw(const char *contents, FILE *out, drawingStates *states);
int U_PMR_DRAWBEZIERS_draw(const char *contents, FILE *out,
                           drawingStates *states);
int U_PMR_DRAWCLOSEDCURVE_draw(const char *contents, FILE *out,
                               drawingStates *states);
int U_PMR_DRAWCURVE_draw(const char *contents, FILE *out,
                         drawingStates *states);
int U_PMR_DRAWDRIVERSTRING_draw(const char *contents, FILE *out,
                                drawingStates *states);
int U_PMR_DRAWELLIPSE_draw(const char *contents, FILE *out,
                           drawingStates *states);
int U_PMR_DRAWIMAGE_draw(const char *contents, FILE *out,
                         drawingStates *states);
int U_PMR_DRAWIMAGEPOINTS_draw(const char *contents, FILE *out,
                               drawingStates *states);
int U_PMR_DRAWLINES_draw(const char *contents, FILE *out,
                         drawingStates *states);
int U_PMR_DRAWPATH_draw(const char *contents, FILE *out, drawingStates *states);
int U_PMR_DRAWPIE_draw(const char *contents, FILE *out, drawingStates *states);
int U_PMR_DRAWRECTS_draw(const char *contents, const char *blimit, FILE *out,
                         drawingStates *states);
int U_PMR_DRAWSTRING_draw(const char *contents, FILE *out,
                          drawingStates *states);
int U_PMR_FILLCLOSEDCURVE_draw(const char *contents, FILE *out,
                               drawingStates *states);
int U_PMR_FILLELLIPSE_draw(const char *contents, FILE *out,
                           drawingStates *states);
int U_PMR_FILLPATH_draw(const char *contents, FILE *out, drawingStates *states);
int U_PMR_FILLPIE_draw(const char *contents, FILE *out, drawingStates *states);
int U_PMR_FILLPOLYGON_draw(const char *contents, FILE *out,
                           drawingStates *states);
int U_PMR_FILLRECTS_draw(const char *contents, const char *blimit, FILE *out,
                         drawingStates *states);
int U_PMR_FILLREGION_draw(const char *contents, FILE *out,
                          drawingStates *states);
int U_PMR_OBJECT_draw(const char *contents, const char *blimit,
                      U_OBJ_ACCUM *ObjCont, int term, FILE *out,
                      drawingStates *states);
int U_PMR_SERIALIZABLEOBJECT_draw(const char *contents, FILE *out,
                                  drawingStates *states);
int U_PMR_SETANTIALIASMODE_draw(const char *contents, FILE *out,
                                drawingStates *states);
int U_PMR_SETCOMPOSITINGMODE_draw(const char *contents, FILE *out,
                                  drawingStates *states);
int U_PMR_SETCOMPOSITINGQUALITY_draw(const char *contents, FILE *out,
                                     drawingStates *states);
int U_PMR_SETINTERPOLATIONMODE_draw(const char *contents, FILE *out,
                                    drawingStates *states);
int U_PMR_SETPIXELOFFSETMODE_draw(const char *contents, FILE *out,
                                  drawingStates *states);
int U_PMR_SETRENDERINGORIGIN_draw(const char *contents, FILE *out,
                                  drawingStates *states);
int U_PMR_SETTEXTCONTRAST_draw(const char *contents, FILE *out,
                               drawingStates *states);
int U_PMR_SETTEXTRENDERINGHINT_draw(const char *contents, FILE *out,
                                    drawingStates *states);
int U_PMR_BEGINCONTAINER_draw(const char *contents, FILE *out,
                              drawingStates *states);
int U_PMR_BEGINCONTAINERNOPARAMS_draw(const char *contents, FILE *out,
                                      drawingStates *states);
int U_PMR_ENDCONTAINER_draw(const char *contents, FILE *out,
                            drawingStates *states);
int U_PMR_RESTORE_draw(const char *contents, FILE *out, drawingStates *states);
int U_PMR_SAVE_draw(const char *contents, FILE *out, drawingStates *states);
int U_PMR_SETTSCLIP_draw(const char *contents, FILE *out,
                         drawingStates *states);
int U_PMR_SETTSGRAPHICS_draw(const char *contents, FILE *out,
                             drawingStates *states);
int U_PMR_MULTIPLYWORLDTRANSFORM_draw(const char *contents, FILE *out,
                                      drawingStates *states);
int U_PMR_RESETWORLDTRANSFORM_draw(const char *contents, FILE *out,
                                   drawingStates *states);
int U_PMR_ROTATEWORLDTRANSFORM_draw(const char *contents, FILE *out,
                                    drawingStates *states);
int U_PMR_SCALEWORLDTRANSFORM_draw(const char *contents, FILE *out,
                                   drawingStates *states);
int U_PMR_SETPAGETRANSFORM_draw(const char *contents, FILE *out,
                                drawingStates *states);
int U_PMR_SETWORLDTRANSFORM_draw(const char *contents, FILE *out,
                                 drawingStates *states);
int U_PMR_TRANSLATEWORLDTRANSFORM_draw(const char *contents, FILE *out,
                                       drawingStates *states);
int U_PMR_STROKEFILLPATH_draw(const char *contents, FILE *out,
                              drawingStates *states); /* not documented */
int U_PMR_MULTIFORMATSTART_draw(
    const char *contents, FILE *out,
    drawingStates *states); /* last of reserved but not used */
int U_PMR_MULTIFORMATSECTION_draw(
    const char *contents, FILE *out,
    drawingStates *states); /* last of reserved but not used */
int U_PMR_MULTIFORMATEND_draw(
    const char *contents, FILE *out,
    drawingStates *states); /* last of reserved but not used */

int U_pmf_onerec_draw(const char *contents, const char *blimit, int recnum,
                      int off, FILE *out, drawingStates *states);

#ifdef __cplusplus
}
#endif
#endif /* _UPMF_draw_ */
