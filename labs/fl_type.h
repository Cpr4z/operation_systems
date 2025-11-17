#pragma once
#include <stdint.h>

typedef enum {
    FL_TYPE_INT,
    FL_TYPE_DOUBLE,
    FL_TYPE_PTR,
    FL_TYPE_STR,
    FL_TYPE_NONE
} fl_type_t;

typedef struct {
    fl_type_t type;
    union {
        int64_t i;
        double d;
        void* p;
    } as;
} fl_value_t;

static inline fl_value_t fl_val_int(int64_t value) { return (fl_value_t){FL_TYPE_INT, {.i = value}}; }
static inline fl_value_t fl_val_double(double value) { return (fl_value_t){FL_TYPE_DOUBLE, {.d = value}}; }
static inline fl_value_t fl_val_ptr(void* value) { return (fl_value_t){FL_TYPE_PTR, {.p = value}}; }