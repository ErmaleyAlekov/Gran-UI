#ifndef GRAN_EDITOR_MESSAGES_HPP
#define GRAN_EDITOR_MESSAGES_HPP
namespace editor
{
    enum class Message
    {
        UNTITLED, READY, OPEN_FAILED, SAVE_FAILED, SAVED, CHANGED_ON_DISK,
        SAVE_QUESTION, CLOSE, NO_DOCUMENT, SEARCH, SOURCE, RUN, EXTENSIONS,
        SETTINGS, ACCOUNTS, PROBLEMS, OUTPUT, TERMINAL, COMMANDS, NOT_FOUND,
        FOLDER_FAILED, NO_CLOSED, COUNT
    };
    // Каталог сообщений RU/EN. / RU/EN message catalogue.
    const char* message(Message id);
}
#endif
