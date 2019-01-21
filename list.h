#ifndef __LIST_H__
#define __LIST_H__

#define LIST_POISON (void*)(0xDEADBEEFDEADBEEF)

#define container_of(ptr, type, member) ({              \
    const typeof(((type *)0)->member) * __mptr = (ptr); \
    (type *)((char *)__mptr - offsetof(type, member));  \
})

struct list_head {
    struct list_head* prev;
    struct list_head* next;
};

static inline void INIT_LIST_HEAD(struct list_head *list) {
    list->next = list;
    list->prev = list;
}

static inline void INIT_LIST_ENTRY(struct list_head* entry) {
    entry->next = LIST_POISON;
    entry->prev = LIST_POISON;
}

static inline bool list_entry_is_valid(struct list_head* entry) {
    return ((entry->next != LIST_POISON) && (entry->prev != LIST_POISON));
}

static inline void __list_add(struct list_head* new,
                              struct list_head* prev,
                              struct list_head* next) {
    next->prev = new;
    new->next = next;
    new->prev = prev;
    prev->next = new;
}

static inline void list_add_head(struct list_head *new,
                                 struct list_head *head) {
    __list_add(new, head, head->next);
}

static inline void list_add_tail(struct list_head *new,
                                 struct list_head *head) {
    __list_add(new, head->prev, head);
}

static inline void __list_del(struct list_head * prev,
                              struct list_head * next) {
    next->prev = prev;
    prev->next = next;
}

static inline void list_del(struct list_head* entry) {
    if (list_entry_is_valid(entry)) {
        __list_del(entry->prev, entry->next);

        entry->prev = LIST_POISON;
        entry->next = LIST_POISON;
    }
}

static inline bool list_is_empty(struct list_head* head) {
    return (head->next == head);
}

#define list_entry(ptr, type, member) container_of(ptr, type, member)

#define list_next_entry(pos, member) \
    list_entry((pos)->member.next, typeof(*(pos)), member)

#define list_first_entry(ptr, type, member) \
    list_entry((ptr)->next, type, member)

#define list_for_each_entry(pos, head, member)               \
    for (pos = list_first_entry(head, typeof(*pos), member); \
         &pos->member != (head);                             \
         pos = list_next_entry(pos, member))

#define list_for_each_entry_safe(pos, n, head, member)       \
    for (pos = list_first_entry(head, typeof(*pos), member), \
         n = list_next_entry(pos, member);                   \
         &pos->member != (head);                             \
         pos = n, n = list_next_entry(n, member))

#endif /* !__LIST_H__ */

