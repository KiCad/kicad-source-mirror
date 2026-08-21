#ifdef __cplusplus
extern "C" {
#endif

#ifndef DARWIN
#define _POSIX_C_SOURCE 200809L
#endif

#include "emf2svg_img_utils.h"
#include "emf2svg_private.h"
#include <stdint.h>
#include <stdio.h>

void clip_rgn_mix(drawingStates *states, PATH *path, uint32_t mode) {
    // FIXME need to handle clip region definition properly.
    // Need to calculate new clipping form.
    // it's not trivial, use a lib like polyclip (cpp) which in turn
    // needs to convert Bezier/ARC curves in segment.
    switch (mode) {
    case U_RGN_NONE:
    case U_RGN_AND:
    case U_RGN_OR:
    case U_RGN_XOR:
    case U_RGN_DIFF:
    case U_RGN_COPY:
    default:
        free_path(&(states->currentDeviceContext.clipRGN));
        copy_path(path, &(states->currentDeviceContext.clipRGN));
        break;
    }
}

void clip_rgn_draw(drawingStates *states, FILE *out) {
    if (!(states->inPath) && states->currentDeviceContext.clipRGN != NULL) {
        states->currentDeviceContext.clipID = get_id(states);
        fprintf(out, "<%sdefs><%sclipPath id=\"clip-%d\">",
                states->nameSpaceString, states->nameSpaceString,
                states->currentDeviceContext.clipID);
        fprintf(out, "<%spath d=\"", states->nameSpaceString);
        draw_path(states->currentDeviceContext.clipRGN, out);
        fprintf(out, "Z\" />");
        fprintf(out, "</clipPath></defs>\n");
    }
}

#ifdef __cplusplus
}
#endif
/* vim:set shiftwidth=2 softtabstop=2 expandtab: */
