#ifndef CHAT_PLATFORM_FATAL_ERROR_HPP
#define CHAT_PLATFORM_FATAL_ERROR_HPP

#include <string>

namespace chat
{
    /*
    ** Фатальная ошибка до или вместо окна чата (разметка, видеокарта, WinSock). Программа
    ** собрана без консоли, поэтому текст показывается окном сообщения ОС, а не в std::cerr.
    **
    ** A fatal error before or instead of the chat window (the markup, the video card, WinSock). The program
    ** is built without a console, so the text is shown in an OS message box, not in std::cerr.
    */
    void showFatalError(const std::string& text) noexcept;
}

#endif
