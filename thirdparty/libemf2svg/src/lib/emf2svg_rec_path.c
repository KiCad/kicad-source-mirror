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

void U_EMRBEGINPATH_draw(const char *contents, FILE *out,
                         drawingStates *states) {
    FLAG_SUPPORTED;
    if (states->verbose) {
        U_EMRBEGINPATH_print(contents, states);
    }
    pathStack *stack = states->emfStructure.pathStack;
    if (stack == NULL) {
        states->Error = true;
        return;
    }
    if (stack->pathStruct.wtBeforeSet) {
        if (stack->pathStruct.wtBeforeiMode) {
            bool draw = transform_set(states, stack->pathStruct.wtBeforexForm,
                                      stack->pathStruct.wtBeforeiMode);
            if (draw)
                transform_draw(states, out);
        } else {
            states->currentDeviceContext.worldTransform =
                stack->pathStruct.wtBeforexForm;
            transform_draw(states, out);
        }
    }
    fprintf(out, "<%spath d=\"", states->nameSpaceString);
    // free previously recorded path
    free_path(&(states->currentPath));
    states->inPath = true;
    UNUSED(contents);
}
void U_EMRENDPATH_draw(const char *contents, FILE *out, drawingStates *states) {
    FLAG_PARTIAL;
    if (states->verbose) {
        U_EMRENDPATH_print(contents, states);
    }
    fprintf(out, "\" ");
    states->inPath = false;
    bool filled = false;
    bool stroked = false;
    pathStack *stack = states->emfStructure.pathStack;
    if (stack == NULL) {
        states->Error = true;
        return;
    }
    uint32_t fillOffset = stack->pathStruct.fillOffset;
    uint32_t strokeOffset = stack->pathStruct.strokeOffset;
    uint32_t strokeFillOffset = stack->pathStruct.strokeFillOffset;
    if (fillOffset != 0)
        fill_draw(states, out, &filled, &stroked);
    if (strokeOffset != 0)
        stroke_draw(states, out, &filled, &stroked);
    if (strokeFillOffset != 0) {
        fill_draw(states, out, &filled, &stroked);
        stroke_draw(states, out, &filled, &stroked);
    }
    if (!filled)
        fprintf(out, "fill=\"none\" ");
    if (!stroked)
        fprintf(out, "stroke=\"none\" ");

    fprintf(out, "/>\n");
    if (stack->pathStruct.wtAfterSet) {
        if (stack->pathStruct.wtBeforeiMode) {
            bool draw = transform_set(states, stack->pathStruct.wtAfterxForm,
                                      stack->pathStruct.wtAfteriMode);
            if (draw)
                transform_draw(states, out);
        } else {
            states->currentDeviceContext.worldTransform =
                stack->pathStruct.wtAfterxForm;
            transform_draw(states, out);
        }
    }
    states->emfStructure.pathStack = stack->next;
    free(stack);
    UNUSED(contents);
}
void U_EMRFLATTENPATH_draw(const char *contents, FILE *out,
                           drawingStates *states) {
    FLAG_IGNORED;
    if (states->verbose) {
        U_EMRFLATTENPATH_print(contents, states);
    }
    UNUSED(contents);
}
void U_EMRABORTPATH_draw(const char *contents, FILE *out,
                         drawingStates *states) {
    FLAG_IGNORED;
    if (states->verbose) {
        U_EMRABORTPATH_print(contents, states);
    }
    // free previously recorded path
    free_path(&(states->currentPath));
    UNUSED(contents);
}
void U_EMRWIDENPATH_draw(const char *contents, FILE *out,
                         drawingStates *states) {
    FLAG_IGNORED;
    if (states->verbose) {
        U_EMRWIDENPATH_print(contents, states);
    }
    UNUSED(contents);
}

#ifdef __cplusplus
}
#endif
/* vim:set shiftwidth=2 softtabstop=2 expandtab: */
