#ifndef XHCI_CONTROLLER_H
#define XHCI_CONTROLLER_H

#include <usb.h>

extern XHCIController *xhciSetup(uint32_t deviceId, uint32_t bar0,
                                 uint32_t interrupt);

#endif
