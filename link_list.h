#ifndef LINK_LIST_H

# define LINK_LIST_H 1

typedef struct link_list {
    struct link_list *next;
} LINK_LIST;

#define offsetof(__type, __member) \
    ((size_t) &((__type *)0)->__member)

#define link_list_init(__link) \
    ((__link)->next = __link)

#define link_list_entry(__link, __type, __member) \
    ((__type *)((char *)__link - offsetof(__type, __member)))

#define link_list_get_head(__head, __type, __member) \
    (((__head)->next != (__head)) ? link_list_entry((__head)->next, __type, __member) : NULL)

int link_list_add_tail(LINK_LIST *head, LINK_LIST *new);
int link_list_del(LINK_LIST *head, LINK_LIST *todel);

#endif /* LINK_LIST_H */
