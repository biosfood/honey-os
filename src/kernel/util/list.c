#include <memory.h>
#include <util.h>

void listAdd(ListElement **list, void *data) {
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
    void *result = (*list)->data;
    *list = (*list)->next;
    return result;
}
