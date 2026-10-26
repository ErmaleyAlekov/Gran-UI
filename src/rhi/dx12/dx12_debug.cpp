#include "gran/rhi/dx12/internal/dx12_internal.hpp"
#include "gran/core/i18n.hpp"

#include <vector>

namespace gran::rhi::dx12
{
    namespace
    {
        // Текст одного сохранённого сообщения уровня ERROR/CORRUPTION или пустая строка
        // The text of one stored ERROR/CORRUPTION message or an empty string
        std::string readMessage(ID3D12InfoQueue* queue, UINT64 index)
        {
            SIZE_T length = 0;

            if (FAILED(queue->GetMessage(index, nullptr, &length)) || length < sizeof(D3D12_MESSAGE))
                return std::string();
            std::vector<unsigned char> storage(length);
            D3D12_MESSAGE* message = reinterpret_cast<D3D12_MESSAGE*>(storage.data());
            if (FAILED(queue->GetMessage(index, message, &length)))
                return std::string();
            if (message->Severity != D3D12_MESSAGE_SEVERITY_ERROR && message->Severity != D3D12_MESSAGE_SEVERITY_CORRUPTION)
                return std::string();
            return i18n::format(i18n::Message::GPU_DEBUG_MESSAGE, {"D3D12", message->pDescription});
        }
    }

    /*
    ** Очередь сообщений существует только с отладочным слоем. Предупреждения отбрасываются,
    ** ошибки возвращаются текстом, очередь очищается после чтения.
    **
    ** The message queue exists only with the debug layer. Warnings are dropped,
    ** errors are returned as text, the queue is cleared after reading.
    */
    std::vector<std::string> drainDebugMessages(ID3D12Device* device)
    {
        ComRef<ID3D12InfoQueue> queue;
        std::vector<std::string> messages;

        if (FAILED(device->QueryInterface(__uuidof(ID3D12InfoQueue), reinterpret_cast<void**>(queue.put()))))
            return messages;
        UINT64 count = queue->GetNumStoredMessages();
        for (UINT64 index = 0; index < count; ++index)
        {
            std::string text = readMessage(queue.get(), index);
            if (!text.empty())
                messages.push_back(text);
        }
        queue->ClearStoredMessages();
        return messages;
    }
}
