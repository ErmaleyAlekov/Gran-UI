#ifndef CHAT_COMMON_CHUNKED_DECODER_HPP
#define CHAT_COMMON_CHUNKED_DECODER_HPP

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>

namespace chat
{
    /*
    ** Потоковый декодер Transfer-Encoding: chunked (RFC 9112, 7.1) — конечный автомат
    ** (паттерн State): байты приходят кусками любой длины, граница куска может пройти где угодно —
    ** посреди размера, данных или CRLF. Расширения «;имя=знач» и завершающие заголовки пропускаются.
    ** Голый LF вместо CRLF принимается (терпимость к простым серверам). Ошибка — ProtocolException.
    **
    ** A streaming Transfer-Encoding: chunked decoder (RFC 9112, 7.1) — a finite state machine
    ** (the State pattern): bytes arrive in pieces of any length, a piece boundary may fall anywhere —
    ** in the middle of a size, data or CRLF. The ";name=value" extensions and the trailer fields are skipped.
    ** A bare LF instead of CRLF is accepted (tolerance to simple servers). An error is ProtocolException.
    */
    class ChunkedDecoder
    {
    public:
        // Байты из сети → полезные данные дописываются в out; после конца потока остаток игнорируется
        // Bytes from the network → the payload is appended to out; after the stream end the rest is ignored
        void feed(std::string_view data, std::string& out);

        // Пришёл последний (нулевой) блок и завершающая пустая строка / The last (zero) block and the final empty line have arrived
        bool finished() const noexcept;

    private:
        // Фаза автомата / The machine phase
        enum class Phase : std::uint8_t { SIZE, DATA, DATA_END, TRAILER, DONE };

        // Собирает строку до LF; true — строка готова (в _line, без CR/LF) / Collects a line up to LF; true — the line is ready (in _line, without CR/LF)
        bool takeLine(std::string_view data, std::size_t& position);

        // Готовая строка в зависимости от фазы / A ready line depending on the phase
        void lineDone();

        Phase _phase = Phase::SIZE;
        std::string _line;
        std::uint64_t _remaining = 0;
    };

    // Предел длины служебной строки (размер, завершающий заголовок) / The service line length limit (a size, a trailer field)
    constexpr std::size_t CHUNK_MAX_LINE = 4096;
}

#endif
