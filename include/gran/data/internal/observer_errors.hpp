#ifndef GRAN_DATA_INTERNAL_OBSERVER_ERRORS_HPP
#define GRAN_DATA_INTERNAL_OBSERVER_ERRORS_HPP

#include <exception>

namespace gran::data::internal
{
    // При замене документа остальные подписчики должны увидеть и изменение, и его отмену.
    // During document replacement the remaining subscribers must see both the change and its reversal.
    class ObserverErrors
    {
    public:
        ObserverErrors() noexcept;
        ~ObserverErrors();
        ObserverErrors(const ObserverErrors&) = delete;
        ObserverErrors& operator=(const ObserverErrors&) = delete;
        static void capture();
        static void checkCurrent();
        void check() const;
    private:
        static thread_local ObserverErrors* active;
        ObserverErrors* previous;
        std::exception_ptr first;
    };
}
#endif
