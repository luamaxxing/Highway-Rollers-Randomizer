#include "revision.h"
#include <string.h>

bool hr_module_name_matches(const char *name)
{
    if (!name)
        return false;
    const char *base = name;
    for (const char *p = name; *p; ++p)
        if (*p == '/' || *p == '\\')
            base = p + 1;
    return strcmp(base, "npg027.rpl") == 0 || strcmp(base, "npg027") == 0;
}

const HrRevision *hr_find_revision(uint64_t title_id)
{
    switch (title_id)
    {
    case UINT64_C(0x0005000010137D00):
        return &ntsc_u;
    default:
        return NULL;
    }
}