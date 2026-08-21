#ifdef __cplusplus
extern "C" {
#endif

#ifndef DARWIN
#define _POSIX_C_SOURCE 200809L
#endif

#include "emf2svg_private.h"
#include "emf2svg_print.h"
#include <stdio.h>

void U_EMRMODIFYWORLDTRANSFORM_draw(const char *contents, FILE *out,
                                    drawingStates *states) {
    FLAG_SUPPORTED;
    if (states->verbose) {
        U_EMRMODIFYWORLDTRANSFORM_print(contents, states);
    }
    PU_EMRMODIFYWORLDTRANSFORM pEmr = (PU_EMRMODIFYWORLDTRANSFORM)(contents);
    bool draw = transform_set(states, pEmr->xform, pEmr->iMode);
    if (draw)
        transform_draw(states, out);
}
void U_EMRSETWORLDTRANSFORM_draw(const char *contents, FILE *out,
                                 drawingStates *states) {
    FLAG_SUPPORTED;
    if (states->verbose) {
        U_EMRSETWORLDTRANSFORM_print(contents, states);
    }
    PU_EMRSETWORLDTRANSFORM pEmr = (PU_EMRSETWORLDTRANSFORM)(contents);
    states->currentDeviceContext.worldTransform = pEmr->xform;
    transform_draw(states, out);
}

#ifdef __cplusplus
}
#endif
/* vim:set shiftwidth=2 softtabstop=2 expandtab: */
