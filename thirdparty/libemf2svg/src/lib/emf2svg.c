#ifdef __cplusplus
extern "C" {
#endif

#ifndef DARWIN
#define _POSIX_C_SOURCE 200809L
#endif
#include "emf2svg.h"
#include "emf2svg_private.h"
#include "emf2svg_print.h"
#include "uemf_safe.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <internal-fmem.h>

int U_emf_onerec_is_emfp(const char *contents, const char *blimit, int recnum,
                         size_t off, bool *ret) {
    PU_ENHMETARECORD lpEMFR = (PU_ENHMETARECORD)(contents + off);
    unsigned int size;

    size = lpEMFR->nSize;
    contents += off;

    /* Check that the record size is OK, abort if not.
       Pointer math might wrap, so check both sides of the range */
    if (size < sizeof(U_EMR) || size > (size_t)(blimit - contents) ||
        !U_emf_record_safe(contents))
        return (-1);

    switch (lpEMFR->iType) {
    case U_EMR_COMMENT:
        *ret |= U_EMRCOMMENT_is_emfplus(contents, blimit);
        break;
    case U_EMR_EOF:
        size = 0;
        break;
    default:
        break;
    }
    return (size);
}

int U_emf_onerec_analyse(const char *contents, const char *blimit, int recnum,
                         size_t off, drawingStates *states) {
    PU_ENHMETARECORD lpEMFR = (PU_ENHMETARECORD)(contents + off);
    unsigned int size;

    size = lpEMFR->nSize;
    contents += off;

    /* Check that the record size is OK, abort if not.
       Pointer math might wrap, so check both sides of the range */
    if (size < sizeof(U_EMR) || size > (size_t)(blimit - contents) ||
        !U_emf_record_safe(contents))
        return (-1);

    switch (lpEMFR->iType) {
    case U_EMR_HEADER:
        break;
    case U_EMR_POLYBEZIER:
    case U_EMR_POLYGON:
    case U_EMR_POLYLINE:
    case U_EMR_POLYBEZIERTO:
    case U_EMR_POLYLINETO:
    case U_EMR_POLYPOLYLINE:
    case U_EMR_POLYPOLYGON:
    case U_EMR_SETWINDOWEXTEX:
    case U_EMR_LINETO:
    case U_EMR_ARCTO:
    case U_EMR_POLYDRAW:
    case U_EMR_POLYBEZIER16:
    case U_EMR_POLYGON16:
    case U_EMR_POLYLINE16:
    case U_EMR_POLYBEZIERTO16:
    case U_EMR_POLYLINETO16:
    case U_EMR_POLYPOLYLINE16:
    case U_EMR_POLYPOLYGON16:
    case U_EMR_POLYDRAW16:
        if (states->inPath) {
            states->pathDrawn = true;
        }
        break;
    case U_EMR_EOF:
        size = 0;
        break;
    case U_EMR_BEGINPATH:
        newPathStruct(states);
        states->inPath = true;
        break;
    case U_EMR_ENDPATH:
        states->inPath = false;
        states->pathDrawn = false;
        break;
    case U_EMR_FILLPATH:
        if (states->emfStructure.pathStackLast != NULL) {
            states->emfStructure.pathStackLast->pathStruct.fillOffset = off;
        }
        break;
    case U_EMR_STROKEANDFILLPATH:
        if (states->emfStructure.pathStackLast != NULL) {
            states->emfStructure.pathStackLast->pathStruct.strokeFillOffset =
                off;
        }
        break;
    case U_EMR_STROKEPATH:
        if (states->emfStructure.pathStackLast != NULL) {
            states->emfStructure.pathStackLast->pathStruct.strokeOffset = off;
        }
        break;
    case U_EMR_FLATTENPATH:
        if (states->emfStructure.pathStackLast != NULL) {
            states->emfStructure.pathStackLast->pathStruct.flattenOffset = off;
        }
        break;
    case U_EMR_WIDENPATH:
        if (states->emfStructure.pathStackLast != NULL) {
            states->emfStructure.pathStackLast->pathStruct.widdenOffset = off;
        }
        break;
    case U_EMR_SELECTCLIPPATH:
        if (states->emfStructure.pathStackLast != NULL) {
            states->emfStructure.pathStackLast->pathStruct.clipOffset = off;
        }
        break;
    case U_EMR_ABORTPATH:
        if (states->emfStructure.pathStackLast != NULL) {
            states->emfStructure.pathStackLast->pathStruct.abortOffset = off;
        }
        break;
    case U_EMR_SETWORLDTRANSFORM:
        if (states->inPath) {
            PU_EMRSETWORLDTRANSFORM pEmr = (PU_EMRSETWORLDTRANSFORM)(contents);
            states->currentDeviceContext.worldTransform = pEmr->xform;
            if (states->pathDrawn) {
                states->emfStructure.pathStackLast->pathStruct.wtBeforeSet =
                    true;
                states->emfStructure.pathStackLast->pathStruct.wtBeforeiMode =
                    0;
                states->emfStructure.pathStackLast->pathStruct.wtBeforexForm =
                    pEmr->xform;
            } else {
                states->emfStructure.pathStackLast->pathStruct.wtAfterSet =
                    true;
                states->emfStructure.pathStackLast->pathStruct.wtAfteriMode = 0;
                states->emfStructure.pathStackLast->pathStruct.wtAfterxForm =
                    pEmr->xform;
            }
        }
        break;
    case U_EMR_MODIFYWORLDTRANSFORM:
        if (states->inPath) {
            PU_EMRMODIFYWORLDTRANSFORM pEmr =
                (PU_EMRMODIFYWORLDTRANSFORM)(contents);
            states->currentDeviceContext.worldTransform = pEmr->xform;
            if (states->pathDrawn) {
                states->emfStructure.pathStackLast->pathStruct.wtBeforeSet =
                    true;
                states->emfStructure.pathStackLast->pathStruct.wtBeforeiMode =
                    pEmr->iMode;
                states->emfStructure.pathStackLast->pathStruct.wtBeforexForm =
                    pEmr->xform;
            } else {
                states->emfStructure.pathStackLast->pathStruct.wtAfterSet =
                    true;
                states->emfStructure.pathStackLast->pathStruct.wtAfteriMode =
                    pEmr->iMode;
                states->emfStructure.pathStackLast->pathStruct.wtAfterxForm =
                    pEmr->xform;
            }
        }
        break;
    case U_EMR_SETWINDOWORGEX:
    case U_EMR_SETVIEWPORTEXTEX:
    case U_EMR_SETVIEWPORTORGEX:
    case U_EMR_SETBRUSHORGEX:
    case U_EMR_SETPIXELV:
    case U_EMR_SETMAPPERFLAGS:
    case U_EMR_SETMAPMODE:
    case U_EMR_SETBKMODE:
    case U_EMR_SETPOLYFILLMODE:
    case U_EMR_SETROP2:
    case U_EMR_SETSTRETCHBLTMODE:
    case U_EMR_SETTEXTALIGN:
    case U_EMR_SETCOLORADJUSTMENT:
    case U_EMR_SETTEXTCOLOR:
    case U_EMR_SETBKCOLOR:
    case U_EMR_OFFSETCLIPRGN:
    case U_EMR_MOVETOEX:
    case U_EMR_SETMETARGN:
    case U_EMR_EXCLUDECLIPRECT:
    case U_EMR_INTERSECTCLIPRECT:
    case U_EMR_SCALEVIEWPORTEXTEX:
    case U_EMR_SCALEWINDOWEXTEX:
    case U_EMR_SAVEDC:
    case U_EMR_RESTOREDC:
    case U_EMR_SELECTOBJECT:
    case U_EMR_CREATEPEN:
    case U_EMR_CREATEBRUSHINDIRECT:
    case U_EMR_DELETEOBJECT:
    case U_EMR_ANGLEARC:
    case U_EMR_ELLIPSE:
    case U_EMR_RECTANGLE:
    case U_EMR_ROUNDRECT:
    case U_EMR_ARC:
    case U_EMR_CHORD:
    case U_EMR_PIE:
    case U_EMR_SELECTPALETTE:
    case U_EMR_CREATEPALETTE:
    case U_EMR_SETPALETTEENTRIES:
    case U_EMR_RESIZEPALETTE:
    case U_EMR_REALIZEPALETTE:
    case U_EMR_EXTFLOODFILL:
    case U_EMR_SETARCDIRECTION:
    case U_EMR_SETMITERLIMIT:
    case U_EMR_CLOSEFIGURE:
    case U_EMR_COMMENT:
    case U_EMR_FILLRGN:
    case U_EMR_FRAMERGN:
    case U_EMR_INVERTRGN:
    case U_EMR_PAINTRGN:
    case U_EMR_EXTSELECTCLIPRGN:
    case U_EMR_BITBLT:
    case U_EMR_STRETCHBLT:
    case U_EMR_MASKBLT:
    case U_EMR_PLGBLT:
    case U_EMR_SETDIBITSTODEVICE:
    case U_EMR_STRETCHDIBITS:
    case U_EMR_EXTCREATEFONTINDIRECTW:
    case U_EMR_EXTTEXTOUTA:
    case U_EMR_EXTTEXTOUTW:
    case U_EMR_CREATEMONOBRUSH:
    case U_EMR_CREATEDIBPATTERNBRUSHPT:
    case U_EMR_EXTCREATEPEN:
    // case U_EMR_POLYTEXTOUTA:            break;
    // case U_EMR_POLYTEXTOUTW:            break;
    case U_EMR_SETICMMODE:
    case U_EMR_CREATECOLORSPACE:
    case U_EMR_SETCOLORSPACE:
    case U_EMR_DELETECOLORSPACE:
    // case U_EMR_GLSRECORD:               break;
    // case U_EMR_GLSBOUNDEDRECORD:        break;
    case U_EMR_PIXELFORMAT:
    // case U_EMR_DRAWESCAPE:              break;
    // case U_EMR_EXTESCAPE:               break;
    // case U_EMR_UNDEF107:                break;
    case U_EMR_SMALLTEXTOUT:
    // case U_EMR_FORCEUFIMAPPING:         break;
    // case U_EMR_NAMEDESCAPE:             break;
    // case U_EMR_COLORCORRECTPALETTE:     break;
    // case U_EMR_SETICMPROFILEA:          break;
    // case U_EMR_SETICMPROFILEW:          break;
    case U_EMR_ALPHABLEND:
    case U_EMR_SETLAYOUT:
    case U_EMR_TRANSPARENTBLT:
    // case U_EMR_UNDEF117:                break;
    case U_EMR_GRADIENTFILL:
    // case U_EMR_SETLINKEDUFIS:           break;
    // case U_EMR_SETTEXTJUSTIFICATION:    break;
    // case U_EMR_COLORMATCHTOTARGETW:     break;
    case U_EMR_CREATECOLORSPACEW:
    default:
        // nothing to do for those records
        break;
        // case U_EMR_UNDEF69:                 break;
    } // end of switch
    return (size);
}
int U_emf_onerec_draw(const char *contents, const char *blimit, int recnum,
                      size_t off, FILE *out, drawingStates *states) {
    PU_ENHMETARECORD lpEMFR = (PU_ENHMETARECORD)(contents + off);
    unsigned int size;
    if (states->verbose) {
        U_emf_onerec_print(contents, blimit, recnum, off, states);
    }
#ifdef RECORD_INDEX
    if (recnum && !states->inPath) {
        fprintf(out, "<!-- begin record: %d -->\n", recnum);
    }
#endif /* RECORD_INDEX */
    size = lpEMFR->nSize;
    contents += off;

    /* Check that the record size is OK, abort if not.
       Pointer math might wrap, so check both sides of the range */
    if (size < sizeof(U_EMR) || size > (size_t)(blimit - contents) ||
        !U_emf_record_safe(contents))
        return (-1);

    switch (lpEMFR->iType) {
    case U_EMR_HEADER:
        U_EMRHEADER_draw(contents, out, states);
        break;
    case U_EMR_POLYBEZIER:
        U_EMRPOLYBEZIER_draw(contents, out, states);
        break;
    case U_EMR_POLYGON:
        U_EMRPOLYGON_draw(contents, out, states);
        break;
    case U_EMR_POLYLINE:
        U_EMRPOLYLINE_draw(contents, out, states);
        break;
    case U_EMR_POLYBEZIERTO:
        U_EMRPOLYBEZIERTO_draw(contents, out, states);
        break;
    case U_EMR_POLYLINETO:
        U_EMRPOLYLINETO_draw(contents, out, states);
        break;
    case U_EMR_POLYPOLYLINE:
        U_EMRPOLYPOLYLINE_draw(contents, out, states);
        break;
    case U_EMR_POLYPOLYGON:
        U_EMRPOLYPOLYGON_draw(contents, out, states);
        break;
    case U_EMR_SETWINDOWEXTEX:
        U_EMRSETWINDOWEXTEX_draw(contents, out, states);
        break;
    case U_EMR_SETWINDOWORGEX:
        U_EMRSETWINDOWORGEX_draw(contents, out, states);
        break;
    case U_EMR_SETVIEWPORTEXTEX:
        U_EMRSETVIEWPORTEXTEX_draw(contents, out, states);
        break;
    case U_EMR_SETVIEWPORTORGEX:
        U_EMRSETVIEWPORTORGEX_draw(contents, out, states);
        break;
    case U_EMR_SETBRUSHORGEX:
        U_EMRSETBRUSHORGEX_draw(contents, out, states);
        break;
    case U_EMR_EOF:
        U_EMREOF_draw(contents, out, states);
        size = 0;
        break;
    case U_EMR_SETPIXELV:
        U_EMRSETPIXELV_draw(contents, out, states);
        break;
    case U_EMR_SETMAPPERFLAGS:
        U_EMRSETMAPPERFLAGS_draw(contents, out, states);
        break;
    case U_EMR_SETMAPMODE:
        U_EMRSETMAPMODE_draw(contents, out, states);
        break;
    case U_EMR_SETBKMODE:
        U_EMRSETBKMODE_draw(contents, out, states);
        break;
    case U_EMR_SETPOLYFILLMODE:
        U_EMRSETPOLYFILLMODE_draw(contents, out, states);
        break;
    case U_EMR_SETROP2:
        U_EMRSETROP2_draw(contents, out, states);
        break;
    case U_EMR_SETSTRETCHBLTMODE:
        U_EMRSETSTRETCHBLTMODE_draw(contents, out, states);
        break;
    case U_EMR_SETTEXTALIGN:
        U_EMRSETTEXTALIGN_draw(contents, out, states);
        break;
    case U_EMR_SETCOLORADJUSTMENT:
        U_EMRSETCOLORADJUSTMENT_draw(contents, out, states);
        break;
    case U_EMR_SETTEXTCOLOR:
        U_EMRSETTEXTCOLOR_draw(contents, out, states);
        break;
    case U_EMR_SETBKCOLOR:
        U_EMRSETBKCOLOR_draw(contents, out, states);
        break;
    case U_EMR_OFFSETCLIPRGN:
        U_EMROFFSETCLIPRGN_draw(contents, out, states);
        break;
    case U_EMR_MOVETOEX:
        U_EMRMOVETOEX_draw(contents, out, states);
        break;
    case U_EMR_SETMETARGN:
        U_EMRSETMETARGN_draw(contents, out, states);
        break;
    case U_EMR_EXCLUDECLIPRECT:
        U_EMREXCLUDECLIPRECT_draw(contents, out, states);
        break;
    case U_EMR_INTERSECTCLIPRECT:
        U_EMRINTERSECTCLIPRECT_draw(contents, out, states);
        break;
    case U_EMR_SCALEVIEWPORTEXTEX:
        U_EMRSCALEVIEWPORTEXTEX_draw(contents, out, states);
        break;
    case U_EMR_SCALEWINDOWEXTEX:
        U_EMRSCALEWINDOWEXTEX_draw(contents, out, states);
        break;
    case U_EMR_SAVEDC:
        U_EMRSAVEDC_draw(contents, out, states);
        break;
    case U_EMR_RESTOREDC:
        U_EMRRESTOREDC_draw(contents, out, states);
        break;
    case U_EMR_SETWORLDTRANSFORM:
        U_EMRSETWORLDTRANSFORM_draw(contents, out, states);
        break;
    case U_EMR_MODIFYWORLDTRANSFORM:
        U_EMRMODIFYWORLDTRANSFORM_draw(contents, out, states);
        break;
    case U_EMR_SELECTOBJECT:
        U_EMRSELECTOBJECT_draw(contents, out, states);
        break;
    case U_EMR_CREATEPEN:
        U_EMRCREATEPEN_draw(contents, out, states);
        break;
    case U_EMR_CREATEBRUSHINDIRECT:
        U_EMRCREATEBRUSHINDIRECT_draw(contents, out, states);
        break;
    case U_EMR_DELETEOBJECT:
        U_EMRDELETEOBJECT_draw(contents, out, states);
        break;
    case U_EMR_ANGLEARC:
        U_EMRANGLEARC_draw(contents, out, states);
        break;
    case U_EMR_ELLIPSE:
        U_EMRELLIPSE_draw(contents, out, states);
        break;
    case U_EMR_RECTANGLE:
        U_EMRRECTANGLE_draw(contents, out, states);
        break;
    case U_EMR_ROUNDRECT:
        U_EMRROUNDRECT_draw(contents, out, states);
        break;
    case U_EMR_ARC:
        U_EMRARC_draw(contents, out, states);
        break;
    case U_EMR_CHORD:
        U_EMRCHORD_draw(contents, out, states);
        break;
    case U_EMR_PIE:
        U_EMRPIE_draw(contents, out, states);
        break;
    case U_EMR_SELECTPALETTE:
        U_EMRSELECTPALETTE_draw(contents, out, states);
        break;
    case U_EMR_CREATEPALETTE:
        U_EMRCREATEPALETTE_draw(contents, out, states);
        break;
    case U_EMR_SETPALETTEENTRIES:
        U_EMRSETPALETTEENTRIES_draw(contents, out, states);
        break;
    case U_EMR_RESIZEPALETTE:
        U_EMRRESIZEPALETTE_draw(contents, out, states);
        break;
    case U_EMR_REALIZEPALETTE:
        U_EMRREALIZEPALETTE_draw(contents, out, states);
        break;
    case U_EMR_EXTFLOODFILL:
        U_EMREXTFLOODFILL_draw(contents, out, states);
        break;
    case U_EMR_LINETO:
        U_EMRLINETO_draw(contents, out, states);
        break;
    case U_EMR_ARCTO:
        U_EMRARCTO_draw(contents, out, states);
        break;
    case U_EMR_POLYDRAW:
        U_EMRPOLYDRAW_draw(contents, out, states);
        break;
    case U_EMR_SETARCDIRECTION:
        U_EMRSETARCDIRECTION_draw(contents, out, states);
        break;
    case U_EMR_SETMITERLIMIT:
        U_EMRSETMITERLIMIT_draw(contents, out, states);
        break;
    case U_EMR_BEGINPATH:
        U_EMRBEGINPATH_draw(contents, out, states);
        break;
    case U_EMR_ENDPATH:
        U_EMRENDPATH_draw(contents, out, states);
        break;
    case U_EMR_CLOSEFIGURE:
        U_EMRCLOSEFIGURE_draw(contents, out, states);
        break;
    case U_EMR_FILLPATH:
        U_EMRFILLPATH_draw(contents, out, states);
        break;
    case U_EMR_STROKEANDFILLPATH:
        U_EMRSTROKEANDFILLPATH_draw(contents, out, states);
        break;
    case U_EMR_STROKEPATH:
        U_EMRSTROKEPATH_draw(contents, out, states);
        break;
    case U_EMR_FLATTENPATH:
        U_EMRFLATTENPATH_draw(contents, out, states);
        break;
    case U_EMR_WIDENPATH:
        U_EMRWIDENPATH_draw(contents, out, states);
        break;
    case U_EMR_SELECTCLIPPATH:
        U_EMRSELECTCLIPPATH_draw(contents, out, states);
        break;
    case U_EMR_ABORTPATH:
        U_EMRABORTPATH_draw(contents, out, states);
        break;
    //        case U_EMR_UNDEF69:                 U_EMRUNDEF69_draw(contents,
    //        out, states);                 break;
    case U_EMR_COMMENT:
        U_EMRCOMMENT_draw(contents, out, states, blimit, off);
        break;
    case U_EMR_FILLRGN:
        U_EMRFILLRGN_draw(contents, out, states);
        break;
    case U_EMR_FRAMERGN:
        U_EMRFRAMERGN_draw(contents, out, states);
        break;
    case U_EMR_INVERTRGN:
        U_EMRINVERTRGN_draw(contents, out, states);
        break;
    case U_EMR_PAINTRGN:
        U_EMRPAINTRGN_draw(contents, out, states);
        break;
    case U_EMR_EXTSELECTCLIPRGN:
        U_EMREXTSELECTCLIPRGN_draw(contents, out, states);
        break;
    case U_EMR_BITBLT:
        U_EMRBITBLT_draw(contents, out, states);
        break;
    case U_EMR_STRETCHBLT:
        U_EMRSTRETCHBLT_draw(contents, out, states);
        break;
    case U_EMR_MASKBLT:
        U_EMRMASKBLT_draw(contents, out, states);
        break;
    case U_EMR_PLGBLT:
        U_EMRPLGBLT_draw(contents, out, states);
        break;
    case U_EMR_SETDIBITSTODEVICE:
        U_EMRSETDIBITSTODEVICE_draw(contents, out, states);
        break;
    case U_EMR_STRETCHDIBITS:
        U_EMRSTRETCHDIBITS_draw(contents, out, states);
        break;
    case U_EMR_EXTCREATEFONTINDIRECTW:
        U_EMREXTCREATEFONTINDIRECTW_draw(contents, out, states);
        break;
    case U_EMR_EXTTEXTOUTA:
        U_EMREXTTEXTOUTA_draw(contents, out, states);
        break;
    case U_EMR_EXTTEXTOUTW:
        U_EMREXTTEXTOUTW_draw(contents, out, states);
        break;
    case U_EMR_POLYBEZIER16:
        U_EMRPOLYBEZIER16_draw(contents, out, states);
        break;
    case U_EMR_POLYGON16:
        U_EMRPOLYGON16_draw(contents, out, states);
        break;
    case U_EMR_POLYLINE16:
        U_EMRPOLYLINE16_draw(contents, out, states);
        break;
    case U_EMR_POLYBEZIERTO16:
        U_EMRPOLYBEZIERTO16_draw(contents, out, states);
        break;
    case U_EMR_POLYLINETO16:
        U_EMRPOLYLINETO16_draw(contents, out, states);
        break;
    case U_EMR_POLYPOLYLINE16:
        U_EMRPOLYPOLYLINE16_draw(contents, out, states);
        break;
    case U_EMR_POLYPOLYGON16:
        U_EMRPOLYPOLYGON16_draw(contents, out, states);
        break;
    case U_EMR_POLYDRAW16:
        U_EMRPOLYDRAW16_draw(contents, out, states);
        break;
    case U_EMR_CREATEMONOBRUSH:
        U_EMRCREATEMONOBRUSH_draw(contents, out, states);
        break;
    case U_EMR_CREATEDIBPATTERNBRUSHPT:
        U_EMRCREATEDIBPATTERNBRUSHPT_draw(contents, out, states);
        break;
    case U_EMR_EXTCREATEPEN:
        U_EMREXTCREATEPEN_draw(contents, out, states);
        break;
    // case U_EMR_POLYTEXTOUTA:            U_EMRPOLYTEXTOUTA_draw(contents, out,
    // states);            break;
    // case U_EMR_POLYTEXTOUTW:            U_EMRPOLYTEXTOUTW_draw(contents, out,
    // states);            break;
    case U_EMR_SETICMMODE:
        U_EMRSETICMMODE_draw(contents, out, states);
        break;
    case U_EMR_CREATECOLORSPACE:
        U_EMRCREATECOLORSPACE_draw(contents, out, states);
        break;
    case U_EMR_SETCOLORSPACE:
        U_EMRSETCOLORSPACE_draw(contents, out, states);
        break;
    case U_EMR_DELETECOLORSPACE:
        U_EMRDELETECOLORSPACE_draw(contents, out, states);
        break;
    // case U_EMR_GLSRECORD:               U_EMRGLSRECORD_draw(contents, out,
    // states);               break;
    // case U_EMR_GLSBOUNDEDRECORD:        U_EMRGLSBOUNDEDRECORD_draw(contents,
    // out, states);        break;
    case U_EMR_PIXELFORMAT:
        U_EMRPIXELFORMAT_draw(contents, out, states);
        break;
    // case U_EMR_DRAWESCAPE:              U_EMRDRAWESCAPE_draw(contents, out,
    // states);              break;
    // case U_EMR_EXTESCAPE:               U_EMREXTESCAPE_draw(contents, out,
    // states);               break;
    // case U_EMR_UNDEF107:                U_EMRUNDEF107_draw(contents, out,
    // states);                break;
    case U_EMR_SMALLTEXTOUT:
        U_EMRSMALLTEXTOUT_draw(contents, out, states);
        break;
    // case U_EMR_FORCEUFIMAPPING:         U_EMRFORCEUFIMAPPING_draw(contents,
    // out, states);         break;
    // case U_EMR_NAMEDESCAPE:             U_EMRNAMEDESCAPE_draw(contents, out,
    // states);             break;
    // case U_EMR_COLORCORRECTPALETTE:
    // U_EMRCOLORCORRECTPALETTE_draw(contents, out, states);     break;
    // case U_EMR_SETICMPROFILEA:          U_EMRSETICMPROFILEA_draw(contents,
    // out, states);          break;
    // case U_EMR_SETICMPROFILEW:          U_EMRSETICMPROFILEW_draw(contents,
    // out, states);          break;
    case U_EMR_ALPHABLEND:
        U_EMRALPHABLEND_draw(contents, out, states);
        break;
    case U_EMR_SETLAYOUT:
        U_EMRSETLAYOUT_draw(contents, out, states);
        break;
    case U_EMR_TRANSPARENTBLT:
        U_EMRTRANSPARENTBLT_draw(contents, out, states);
        break;
    // case U_EMR_UNDEF117:                U_EMRUNDEF117_draw(contents, out,
    // states);                break;
    case U_EMR_GRADIENTFILL:
        U_EMRGRADIENTFILL_draw(contents, out, states);
        break;
    // case U_EMR_SETLINKEDUFIS:           U_EMRSETLINKEDUFIS_draw(contents,
    // out, states);           break;
    // case U_EMR_SETTEXTJUSTIFICATION:
    // U_EMRSETTEXTJUSTIFICATION_draw(contents, out, states);    break;
    // case U_EMR_COLORMATCHTOTARGETW:
    // U_EMRCOLORMATCHTOTARGETW_draw(contents, out, states);     break;
    case U_EMR_CREATECOLORSPACEW:
        U_EMRCREATECOLORSPACEW_draw(contents, out, states);
        break;
    default:
        U_EMRNOTIMPLEMENTED_draw("?", contents, out, states);
        break;
    } // end of switch
    return (size);
}

int emf2svg(char *contents, size_t length, char ** fm_out, size_t * fm_out_length,
            generatorOptions *options) {
    size_t off = 0;
    size_t result;
    int OK = 1;
    int recnum = 0;
    PU_ENHMETARECORD pEmr;
    char *blimit;
    FILE *stream;
    fmem fm;
    if (!fm_out || !fm_out_length)
        return 0;
    *fm_out = NULL;
    *fm_out_length = 0;
    if (!contents || length < sizeof(U_EMR) || !options)
        return 0;
    fmem_init(&fm);

#if U_BYTE_SWAP
    // This is a Big Endian machine, EMF data is Little Endian
    U_emf_endian(contents, length, 0); // LE to BE
#endif

    drawingStates *states = (drawingStates *)calloc(1, sizeof(drawingStates));
    states->fixBrokenYTransform = false;
    states->verbose = options->verbose;
    states->viewPortExSet = false;
    states->windowExSet = false;
    states->emfplus = options->emfplus;
    states->imgWidth = options->imgWidth;
    states->fontConfig = options->fontConfig;
    states->imgHeight = options->imgHeight;
    states->endAddress = (intptr_t)contents + (intptr_t)length;
    if ((options->nameSpace != NULL) && (strlen(options->nameSpace) != 0)) {
        states->nameSpace = options->nameSpace;
        states->nameSpaceString =
            (char *)calloc(strlen(options->nameSpace) + 2, sizeof(char));
        sprintf(states->nameSpaceString, "%s%s", states->nameSpace, ":");
    } else {
        states->nameSpaceString = (char *)"";
    }

    states->svgDelimiter = options->svgDelimiter;
    states->currentDeviceContext.font_name = NULL;
    /* initialized to -1 because real size of states->objectTable is always
     * states->objectTableSize + 1 (for easier index manipulation since
     * indexes in emf files start at 1 and not 0)*/
    states->objectTableSize = -1;
    setTransformIdentity(states);

    blimit = contents + length;
    int err = 1;

    stream = fmem_open(&fm, "w");
    if (stream == NULL) {
        if (states->verbose) {
            printf("Failed to allocate output stream\n");
        }
        FLAG_RESET;
        err = 0;
        OK = 0;
    }

    // analyze emf structure
    while (OK) {
        if (off > length || length - off < sizeof(U_EMR)) { // normally should exit from while after EMREOF
                             // sets OK to false, this is most likely a corrupt
                             // EMF
            if (states->verbose) {
                printf("WARNING(scanning): record claims to extend beyond the "
                       "end of the EMF file\n");
            }
            OK = 0;
            err = 0;
            break;
        }

        pEmr = (PU_ENHMETARECORD)(contents + off);

        if (!recnum && (pEmr->iType != U_EMR_HEADER)) {
            if (states->verbose) {
                printf("WARNING(scanning): EMF file does not begin with an "
                       "EMR_HEADER record\n");
            }
            OK = 0;
            err = 0;
        }
        if (recnum && (pEmr->iType == U_EMR_HEADER)) {
            if (states->verbose) {
                printf("ABORTING(scanning): EMF contains two or more "
                       "EMR_HEADER records\n");
            }
            OK = 0;
            err = 0;
        }

        result = U_emf_onerec_analyse(contents, blimit, recnum, off, states);
        if (result == (size_t)-1 || states->Error) {
            if (states->verbose) {
                printf(
                    "ABORTING(scanning): invalid record - corrupted file?\n");
            }
            OK = 0;
            err = 0;
        } else if (!result) {
            OK = 0;
        } else {
            off += result;
            recnum++;
        }
    } // end of while
    FLAG_RESET;
    setTransformIdentity(states);

    // continue only if no previous errors
    if (err == 0) {
        OK = 0;
    } else {
        OK = 1;
    }

    off = 0;
    recnum = 0;
    while (OK) {
        if (off > length || length - off < sizeof(U_EMR)) { // normally should exit from while after EMREOF
                             // sets OK to false, this is most likely a corrupt
                             // EMF
            if (states->verbose) {
                printf("WARNING(converting): record claims to extend beyond "
                       "the end of the EMF file\n");
            }
            OK = 0;
            err = 0;
            break;
        }

        pEmr = (PU_ENHMETARECORD)(contents + off);

        result = U_emf_onerec_draw(contents, blimit, recnum, off, stream, states);
        if (result == (size_t)-1 || states->Error) {
            if (states->verbose) {
                printf(
                    "ABORTING(converting): invalid record - corrupted file?\n");
            }
            OK = 0;
            err = 0;
        } else if (!result) {
            OK = 0;
        } else {
            off += result;
            recnum++;
        }
    } // end of while
    FLAG_RESET;
    freeObjectTable(states);
    freePathStack(states->emfStructure.pathStack);
    free_path(&(states->currentPath));
    free(states->objectTable);
    freeDeviceContext(&(states->currentDeviceContext));
    freeDeviceContextStack(states);
    freeEmfImageLibrary(states);
    free(states);

    if (stream) {
        fflush(stream);
        void* out;
        fmem_mem(&fm, &out, fm_out_length);
        if (*fm_out_length) {
            *fm_out = (char*)malloc(*fm_out_length+1);
        }
        if (*fm_out) {
            memcpy((void*)(*fm_out), out, *fm_out_length);
            (*fm_out)[*fm_out_length] = 0;
        }
        else {
            err = 0;
        }
        fclose(stream);
        fmem_term(&fm);
    }

    return err;
}

int emf2svg_is_emfplus(char *contents, size_t length, bool *is_emfp) {
    size_t off = 0;
    size_t result;
    int OK = 1;
    int recnum = 0;
    PU_ENHMETARECORD pEmr;
    char *blimit;
    *is_emfp = false;
    if (!contents || length < sizeof(U_EMR))
        return 0;

#if U_BYTE_SWAP
    // This is a Big Endian machine, EMF data is Little Endian
    U_emf_endian(contents, length, 0); // LE to BE
#endif

    blimit = contents + length;
    int err = 1;

    // analyze emf structure
    while (OK) {
        if (off > length || length - off < sizeof(U_EMR)) { // normally should exit from while after EMREOF
                             // sets OK to false, this is most likely a corrupt
                             // EMF
            OK = 0;
            err = 0;
            break;
        }

        pEmr = (PU_ENHMETARECORD)(contents + off);

        if (!recnum && (pEmr->iType != U_EMR_HEADER)) {
            OK = 0;
            err = 0;
        }
        if (recnum && (pEmr->iType == U_EMR_HEADER)) {
            OK = 0;
            err = 0;
        }

        result = U_emf_onerec_is_emfp(contents, blimit, recnum, off, is_emfp);
        if (result == (size_t)-1) {
            OK = 0;
            err = 0;
        } else if (!result) {
            OK = 0;
        } else {
            off += result;
            recnum++;
        }
    } // end of while
    return err;
}

#ifdef __cplusplus
}
#endif
/* vim:set shiftwidth=2 softtabstop=2 expandtab: */
