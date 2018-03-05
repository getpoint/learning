#include "link_list.h"

int link_list_add_tail(LINK_LIST *head, LINK_LIST *new)
{
    LINK_LIST *prev, *pos;

    prev = head;

    for (pos = head->next; pos != head; pos = pos->next) {
        prev = pos;
    }

    prev->next = new;
    new->next = head;

    return 0;
}

int link_list_del(LINK_LIST *head, LINK_LIST *todel)
{
    LINK_LIST *prev, *pos;

    if (todel == head) {
        return -1;
    }

    prev = head;

    for (pos = head->next; pos != todel; pos = pos->next) {
        if (pos == head) {
            return -1;
        }

        prev = pos;
    }

    prev->next = pos->next;
    return 0;
}
