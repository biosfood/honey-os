#ifndef COMMANDS_H
#define COMMANDS_H

#include "xhci.h"
#include <stdint.h>

extern CommandCompletionEvent *xhciCommand(XHCIController *controller,
                                           uint32_t dataLow, uint32_t dataHigh,
                                           uint32_t status, uint32_t control);
extern void addressDevice(SlotXHCI *slot, bool BSR);
extern void configureEndpoint(XHCIController *controller, void *inputContext,
                              uint32_t slotNumber, bool deconfigure);
extern void evaluateContext(XHCIController *controller, void *inputContext,
                            uint32_t slotNumber);

extern uint32_t requestSlotIndex(XHCIController *controller);

extern void *xhciGetDescriptor(SlotXHCI *slot, uint32_t value,
                                    uint32_t index, void *buffer);

extern TrbRing *createSlotTRB(SlotXHCI *slot);

#endif
