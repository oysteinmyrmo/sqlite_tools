#pragma once
#define SQLT_ASSERT(x) if (x); else abort();

#define SQLT_FUZZY_EPSILON 0.0001

#define SQLT_FUZZY_ASSERT(val, correctVal) \
{ \
    SQLT_ASSERT(val <= correctVal + SQLT_FUZZY_EPSILON); \
    SQLT_ASSERT(val >= correctVal - SQLT_FUZZY_EPSILON); \
}
