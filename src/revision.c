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

bool hr_get_revision(uint64_t title_id, HrRevision *out)
{
    if (!out)
        return false;

    HrRevision selected = base;
    selected.title_id = title_id;

    switch (title_id)
    {
    case UINT64_C(0x0005000010137D00): // NTSC-U
        break;
    case UINT64_C(0x0005000010137E00): // PAL
        break;
    case UINT64_C(0x000500001011A800): // NTSC-J
        selected.model_function_offset = 0x181c8u;
        break;
    default:
        return false;
    }

    *out = selected;
    return true;
}
