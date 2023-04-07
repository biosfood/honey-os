#include "commands.h"
#include "../../hlib/include/syscalls.h"
#include "trbRing.h"
#include <hlib.h>
#include <usb.h>

CommandCompletionEvent *xhciCommand(XHCIController *controller,
                                    uint32_t dataLow, uint32_t dataHigh,
                                    uint32_t status, uint32_t control) {
    XHCITRB trb = {0};
    trb.dataLow = dataLow;
    trb.dataHigh = dataHigh;
    trb.status = status;
    trb.control = control;
    uint32_t commandAddress = U32(enqueueCommand(&controller->commands, &trb));
    controller->doorbells[0] = 0;
    uint32_t eventId = syscall(SYS_CREATE_EVENT, commandAddress, 0, 0, 0);
    return PTR(await(serviceId, eventId));
}

void addressDevice(XHCIController *controller, void *inputContext,
                   uint32_t slotNumber, bool BSR) {
    uint32_t control =
        COMMAND_TYPE(11) | COMMAND_SLOT_ID(slotNumber) | COMMAND_BSR(BSR);
    xhciCommand(controller, U32(inputContext), 0, 0, control);
}

void configureEndpoint(XHCIController *controller, void *inputContext,
                       uint32_t slotNumber, bool deconfigure) {
    uint32_t control = COMMAND_TYPE(12) | COMMAND_SLOT_ID(slotNumber) |
                       COMMAND_BSR(deconfigure);
    xhciCommand(controller, U32(inputContext), 0, 0, control);
}

void evaluateContext(XHCIController *controller, void *inputContext,
                     uint32_t slotNumber) {
    uint32_t control = COMMAND_TYPE(13) | COMMAND_SLOT_ID(slotNumber);
    xhciCommand(controller, U32(inputContext), 0, 0, control);
}

uint32_t requestSlotIndex(XHCIController *controller) {
    return xhciCommand(controller, 0, 0, 0, COMMAND_TYPE(9))->slotId;
}

void *usbGetDeviceDescriptor(XHCIController *controller,
                             XHCIInputContext *inputContext, TrbRing *ring,
                             uint32_t slotIndex, uint32_t value, uint32_t index,
                             void *buffer) {
    XHCISetupStageTRB setup = {0};
    setup.requestType = 0x80;
    setup.request = 6;
    setup.value = value;
    setup.index = index;
    setup.length = 4096;
    setup.transferLength = 8;
    setup.interruptOnCompletion = 0;
    setup.interrupterTarget = 0;
    setup.type = 2;
    setup.transferType = 3;
    setup.immediateData = 1;

    XHCIDataStageTRB data = {0};
    data.dataBuffer[0] = U32(getPhysicalAddress(buffer));
    data.inDirection = 1;
    data.transferSize = 4096;
    data.type = 3;
    data.interrupterTarget = 0;

    XHCIStatusStageTRB status = {0};
    status.inDirection = 1;
    status.evaluateNext = 0;
    status.interruptOnCompletion = 1;
    status.type = 4;

    enqueueCommand(ring, (void *)&setup);
    enqueueCommand(ring, (void *)&data);
    uint32_t commandAddress = U32(enqueueCommand(ring, (void *)&status));
    uint32_t eventId = syscall(SYS_CREATE_EVENT, commandAddress, 0, 0, 0);
    controller->doorbells[slotIndex] = 1;
    CommandCompletionEvent *completionEvent = PTR(await(serviceId, eventId));
    return buffer;
}
