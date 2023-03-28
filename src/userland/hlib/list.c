#include <hlib.h>

void _listAdd(void *(malloc)(uint32_t), ListElement **list, void *data) {
    ListElement *element = malloc(sizeof(ListElement));
    element->data = data;
    element->next = NULL;
    if (!*list) {
        *list = element;
        return;
    }
    ListElement *current = *list;
    while (current->next) {
        current = current->next;
    }
    current->next = element;
}

void *listPopFirst(ListElement **list) {
    if (!*list) {
        return NULL;
    }
    ListElement *resultElement = *list;
    void *result = resultElement->data;
    *list = (*list)->next;
    free(resultElement);
    return result;
}

uint32_t listCount(ListElement *list) {
    uint32_t i = 0;
    foreach (list, void *, element, { i++; })
        ;
    return i;
}

void *listGet(ListElement *list, uint32_t position) {
    for (uint32_t i = 0; i < position; i++) {
        list = list->next;
    }
    return list->data;
}

bool listRemoveValue(ListElement **list, void *value) {
    if (!*list) {
        return false;
    }
    ListElement *element = *list, *previous = NULL;
    while (element) {
        if (element->data == value) {
            if (previous) {
                previous->next = element->next;
            } else {
                *list = element->next;
            }
            free(element);
            return true;
        }
        previous = element;
        element = element->next;
    }
    return false;
}
