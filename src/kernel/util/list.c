#include <memory.h>
#include <util.h>

void listAdd(ListElement **list, void *data) {
    ListElement *element = *list;
    if (!element) {
        *list = malloc(sizeof(ListElement));
        element = *list;
    } else {
        while (element->next) {
            element = element->next;
        }
        element->next = malloc(sizeof(ListElement));
        element = element->next;
    }
    element->next = NULL;
    element->data = data;
}

void *listPopFirst(ListElement **list) {
    if (!*list) {
        return NULL;
    }
    void *result = (*list)->data;
    *list = (*list)->next;
    return result;
}
