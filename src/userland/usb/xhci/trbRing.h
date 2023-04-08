#ifndef TRB_RING_H
#define TRB_RING_H

#include "xhci.h"

extern XHCITRB *enqueueCommand(TrbRing *ring, XHCITRB *trb);
extern XHCITRB *trbRingFetch(TrbRing *ring, uint32_t *index);
extern void setupTrbRing(TrbRing *ring, uint32_t size);

#endif
