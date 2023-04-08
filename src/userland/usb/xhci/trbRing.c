#include "xhci.h"
#include <usb.h>

XHCITRB *enqueueCommand(TrbRing *ring, XHCITRB *trb) {
    trb->control |= ring->cycle;
    memcpy((void *)trb, (void *)&ring->trbs[ring->enqueue], sizeof(XHCITRB));
    XHCITRB *result = &ring->physical[ring->enqueue];
    ring->enqueue++;
    if (ring->enqueue == ring->size - 1) {
        if (ring->trbs[ring->enqueue].control & 1) {
            ring->trbs[ring->enqueue].control &= ~1;
        } else {
            ring->trbs[ring->enqueue].control |= 1;
        }
        if (ring->trbs[ring->enqueue].control & 1) {
            ring->cycle ^= 1;
        }
        ring->enqueue = 0;
    }
    return result;
}

XHCITRB *trbRingFetch(TrbRing *ring, uint32_t *index) {
    if ((ring->trbs[ring->dequeue].control & 1) != ring->cycle) {
        return NULL;
    }
    if (index) {
        *index = ring->dequeue;
    }
    XHCITRB *result = &ring->trbs[ring->dequeue];
    ring->dequeue++;
    if (ring->dequeue == ring->size) {
        ring->dequeue = 0;
        ring->cycle ^= -1;
    }
    return result;
}

void setupTrbRing(TrbRing *ring, uint32_t size) {
    ring->trbs = requestMemory(1, 0, 0);
    ring->physical = getPhysicalAddress((void *)ring->trbs);
    ring->cycle = true;
    ring->enqueue = 0;
    ring->dequeue = 0;
    ring->size = size;
    // define link to beginning
    ring->trbs[ring->size - 1].dataLow = U32(ring->physical);
    ring->trbs[ring->size - 1].control |= 1 << 1 | COMMAND_TYPE(6);
}

TrbRing *createSlotTRB(SlotXHCI *slot) {
    TrbRing *ring = malloc(sizeof(TrbRing));
    setupTrbRing(ring, 256);
    slot->inputContext->deviceContext.endpoints[0].transferDequeuePointerLow =
        U32(ring->physical) | 1;
    slot->inputContext->deviceContext.endpoints[0].transferDequeuePointerHigh =
        0;
    return ring;
}
