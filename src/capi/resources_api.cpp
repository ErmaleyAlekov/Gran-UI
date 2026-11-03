#include "gran/capi/internal/bridge.hpp"
#include "gran/res/resources.hpp"

// Блок копируется в реестр процесса: память вызывающего можно освободить сразу / The block is copied into the process registry: the caller memory may be freed at once
gran_status gran_resources_mount(const void* data, size_t size)
{
    GRAN_CAPI_TRY
    gran::capi::require(data != nullptr && size > 0);
    gran::res::mountBlob(data, size);
    GRAN_CAPI_END
}
