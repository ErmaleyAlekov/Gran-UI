#ifndef CHAT_PLATFORM_CHAT_OPTIONS_HPP
#define CHAT_PLATFORM_CHAT_OPTIONS_HPP

#include "gran/app/application.hpp"
#include "ollama/ollama_client.hpp"

namespace chat
{
    // Параметры командной строки / The command line parameters
    struct ChatOptions
    {
        gran::AppOptions app;
        chat::OllamaEndpoint endpoint;
        std::string markup;
        std::string ask;
    };

    std::uint16_t parsePort(const std::string& text);
    ChatOptions parseOptions(const std::vector<std::string>& argv);
}

#endif
