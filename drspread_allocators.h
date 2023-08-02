//
// Copyright © 2023, David Priver <david@davidpriver.com>
//
#ifndef DRSP_ALLOCATORS_H
#define DRSP_ALLOCATORS_H
#include <stddef.h>
#include <stdlib.h>

static inline
void*_Nullable
drsp_alloc(size_t old_sz, const void*_Nullable old, size_t new_sz, size_t alignment);

static inline
int
drsp_report_leaks(void);


#define free adsljasdk
#define realloc akdsjaskd
#define calloc asdkajsd
#define malloc aksjdaksjd

#endif
