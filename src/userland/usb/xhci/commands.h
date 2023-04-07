#ifndef COMMANDS_H
#define COMMANDS_H

#include <stdint.h>
#include <usb.h>

extern void addressDevice(XHCIController *controller, void *inputContext,
                          uint32_t slotNumber, bool BSR);
extern void configureEndpoint(XHCIController *controller, void *inputContext,
                              uint32_t slotNumber, bool deconfigure);
extern void evaluateContext(XHCIController *controller, void *inputContext,
                            uint32_t slotNumber);

extern uint32_t requestSlotIndex(XHCIController *controller);

extern void *usbGetDeviceDescriptor(XHCIController *controller,
                                    XHCIInputContext *inputContext,
                                    TrbRing *ring, uint32_t slotIndex,
                                    uint32_t value, uint32_t index,
                                    void *buffer);

extern TrbRing *createTRB(XHCIController *controller,
                          XHCIInputContext *inputContext, uint32_t slotIndex);

#endif
