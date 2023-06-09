#ifndef DLIST_H
#define DLIST_H

struct __list{
    void *data;
    struct __list *next;
};

struct __list_booster{
    struct __list **points, *end;
    int pointsCount, len, info;
    void (*compare)(void *a, void *b);
    void (*destruct)(void *data);
};

struct __booster_info{
    void *target;
    int index, error;
};

#include <stdlib.h>
#include <string.h>

/* boosters and lists inits */
extern struct __list makeEmptyList();
extern struct __list makeListFromArr(void *array, unsigned int _element_size, unsigned int _elements_count);
extern struct __list_booster makeBooster(struct __list *list, unsigned int pointRate);
extern struct __list_booster makeEmptyBooster(unsigned int pointRate);

/* Basic lists (list) functions */
extern void *listGet(struct __list *_this, int index);
long listFind(struct __list *_this, const void *data, char (*compare)(const void *, void *)); //return -1 if no item in this
extern struct __list *listFrame(struct __list *_this, int index);
extern int listAppendLink(struct __list *_this, void *data);
extern int listAppend(struct __list *_this, void *data, unsigned int size);
extern void *listResolve(struct __list *_this, unsigned int size);
// extern void *__booster_resolve(struct __list_booster *_this, unsigned int size); LEGACY
extern int listLen(struct __list *_this);
extern int unbindList(struct __list *_this); //WARNING! It's just removes list chain, NOT CONTENT!
extern int earaseList(struct __list *_this); //WARNING! It's will remove ALL data with content inside!
extern int listUnbind(struct __list *_this, int index); //REMOVING POINTER to content, not content
extern int listRemove(struct __list *_this, int index); //REMOVING CONTENT with pointer

/* Basic lists (booster) operators */
extern struct __booster_info __booster_assamble(struct __list_booster *_this);
extern struct __booster_info __booster_len(struct __list_booster *_this);
extern struct __booster_info __booster_find(struct __list_booster *_this, void *target);
extern struct __booster_info __booster_get(struct __list_booster *_this, int index);
extern struct __booster_info __booster_append(struct __list_booster *_this, int size);
extern struct __booster_info __booster_remove(struct __list_booster *_this, int index);
extern struct __booster_info __booster_delete(struct __list_booster *_this, void *target);

/* Extra python-like operations */
extern struct __booster_info __booster_invert(struct __list_booster *_this);
extern struct __booster_info __booster_py_expression(struct __list_booster *_this, const char *_expression);
    /* Strong-casts */
extern struct __booster_info __boster_py_toString(struct __list_booster *_this, const char *mainFormat, const char *elementFormat);

/* For low-level */
extern void *__booster_make_C_arr(struct __list_booster *_this, unsigned int _element_size, unsigned int _elements_count);
extern void *__booster_Write_C_arr(struct __list_booster *_this, unsigned int _element_size, unsigned int _elements_count);

#endif