#include "json_memory.h"

#include <cJSON.h>
#include <esp_heap_caps.h>

namespace
{

// cJSON's allocations are small (a node is about 40 bytes, a response a few hundred), so with
// CONFIG_SPIRAM_MALLOC_ALWAYSINTERNAL they would all land in internal RAM, which is the scarce one.
// Speed doesn't matter at these sizes, so PSRAM comes first.
void *allocate_prefer_psram(size_t size)
{
    return heap_caps_malloc_prefer(size, 2,
                                   MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT,
                                   MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
}

} // namespace

void json_memory_prefer_psram(void)
{
    // heap_caps_free() frees memory from either region, so memory that cJSON allocated before this
    // call, or that other code frees with free(), is still released correctly.
    cJSON_Hooks hooks = {
        .malloc_fn = allocate_prefer_psram,
        .free_fn = heap_caps_free,
    };
    cJSON_InitHooks(&hooks);
}
