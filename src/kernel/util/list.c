#include <memory.h>
#include <util.h>

void listAdd(ListElement **list, void *data) {
    ListElement *element = malloc(sizeof(ListElement));
    element->next = *list;
    element->data = data;
    *list = element;
}

void *listPopFirst(ListElement **list) {
    if (!*list) {
        return NULL;
    }
    void *result = (*list)->data;
    *list = (*list)->next;
    return result;
}
