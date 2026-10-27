#include "snapshot.hpp"

#include "gran/binding/list_bindings.hpp"

namespace snapshot
{
    /*
    ** Привязки нужны только на время построения: кадр один, модели больше не меняются.
    ** Их уничтожение лишь отписывает их от моделей — построенные узлы остаются в документе.
    **
    ** The bindings are needed only while building: there is one frame, the models do not change anymore.
    ** Destroying them only unsubscribes them from the models — the built nodes stay in the document.
    */
    void buildLists(gran::markup::UiDocument& document)
    {
        gran::binding::ListBindings lists;

        lists.adoptData(document.data);
        if (document.root)
            lists.bind(*document.root);
    }
}
