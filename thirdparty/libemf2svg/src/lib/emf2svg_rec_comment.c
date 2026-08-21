#ifdef __cplusplus
extern "C" {
#endif

#ifndef DARWIN
#define _POSIX_C_SOURCE 200809L
#endif
#include "emf2svg_private.h"
#include "emf2svg_print.h"
#include "pmf2svg.h"
#include "pmf2svg_print.h"
#include <stdio.h>
#include <stdlib.h>

bool U_EMRCOMMENT_is_emfplus(const char *contents, const char *blimit) {
    char *src;
    uint32_t cIdent, cbData;
    bool ret = false;
    PU_EMRCOMMENT pEmr = (PU_EMRCOMMENT)(contents);
    cbData = pEmr->cbData;
    src = (char *)&(pEmr->Data); // default
    if (cbData >= 4) {
        cIdent = *(uint32_t *)(src);
        if (cIdent == U_EMR_COMMENT_EMFPLUSRECORD)
            ret = true;
    }
    return ret;
}

void U_EMRCOMMENT_draw(const char *contents, FILE *out, drawingStates *states,
                       const char *blimit, size_t off) {
    FLAG_IGNORED;
    if (states->verbose) {
        U_EMRCOMMENT_print(contents, states, blimit, off);
    }
    char *src;
    uint32_t cIdent, cIdent2, cbData;
    size_t loff;
    int recsize;
    static int recnum = 0;

    PU_EMRCOMMENT pEmr = (PU_EMRCOMMENT)(contents);

    /* There are several different types of comments */

    cbData = pEmr->cbData;
    src = (char *)&(pEmr->Data); // default
    if (cbData >= 4) {
        /* Since the comment is just a big bag of bytes the emf endian code
           cannot safely touch
           any of its payload.  This is the only record type with that
           limitation.  Try to determine
           what the contents are even if more byte swapping is required. */
        cIdent = *(uint32_t *)(src);
        if (U_BYTE_SWAP) {
            U_swap4(&(cIdent), 1);
        }
        if (cIdent == U_EMR_COMMENT_PUBLIC) {
            PU_EMRCOMMENT_PUBLIC pEmrp = (PU_EMRCOMMENT_PUBLIC)pEmr;
            cIdent2 = pEmrp->pcIdent;
            if (U_BYTE_SWAP) {
                U_swap4(&(cIdent2), 1);
            }
            src = (char *)&(pEmrp->Data);
            cbData -= 8;
        } else if (cIdent == U_EMR_COMMENT_SPOOL) {
            PU_EMRCOMMENT_SPOOL pEmrs = (PU_EMRCOMMENT_SPOOL)pEmr;
            cIdent2 = pEmrs->esrIdent;
            if (U_BYTE_SWAP) {
                U_swap4(&(cIdent2), 1);
            }
            src = (char *)&(pEmrs->Data);
            cbData -= 8;
        } else if (cIdent == U_EMR_COMMENT_EMFPLUSRECORD) {
            PU_EMRCOMMENT_EMFPLUS pEmrpl = (PU_EMRCOMMENT_EMFPLUS)pEmr;
            src = (char *)&(pEmrpl->Data);
            if (states->emfplus) {
                loff = 16; /* Header size of the header part of an EMF+ comment
                              record */
                if (states->verbose) {
                    printf("\n   =====================%s START EMF+ RECORD "
                           "ANALYSING %s=====================\n\n",
                           KCYN, KNRM);
                }
                while (loff < cbData + 12) { // EMF+ records may not fill the
                                             // entire comment, cbData value
                                             // includes cIdent, but not U_EMR
                                             // or cbData
                    returnOutOfEmf(src);
                    recsize = U_pmf_onerec_draw(src, blimit, recnum, loff + off,
                                                out, states);
                    if (states->verbose) {
                        U_pmf_onerec_print(src, blimit, recnum, loff + off, out,
                                           states);
                    }
                    if (recsize <= 0)
                        break;
                    loff += recsize;
                    src += recsize;
                    recnum++;
                }
                if (states->verbose) {
                    printf("\n   ======================%s END EMF+ RECORD "
                           "ANALYSING %s======================\n",
                           KBLU, KNRM);
                }
            }
            return;
        }
    }
}

#ifdef __cplusplus
}
#endif
/* vim:set shiftwidth=2 softtabstop=2 expandtab: */
