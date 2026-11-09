#ifndef GRAN_UTILS_URI_LIST_HPP
#define GRAN_UTILS_URI_LIST_HPP

#include <string>
#include <string_view>
#include <vector>

namespace gran
{
    /*
    ** Раскодировать %XX в байты (RFC 3986). Неполная или неверная последовательность (`%`, `%4`, `%zz`)
    ** — false: такой адрес не принимается, а не превращается в чужой путь. Байт 0 (`%00`) тоже запрещён:
    ** путь с ним обрезался бы в вызовах ОС.
    **
    ** Decode %XX into bytes (RFC 3986). An incomplete or invalid sequence (`%`, `%4`, `%zz`)
    ** is false: such an address is rejected instead of turning into somebody else's path. The zero byte (`%00`)
    ** is forbidden as well: a path with it would be cut in OS calls.
    */
    bool percentDecode(std::string_view text, std::string& out);

    /*
    ** Локальный путь из адреса `file:`: `file:///путь`, `file://localhost/путь`, `file:/путь`. Адрес с чужим
    ** узлом (`file://server/…`) и любые другие схемы (`http:`, `trash:`) — false: это не файл на этой машине.
    **
    ** A local path from a `file:` address: `file:///path`, `file://localhost/path`, `file:/path`. An address with
    ** another host (`file://server/…`) and any other schemes (`http:`, `trash:`) are false: that is not a file here.
    */
    bool fileUriPath(std::string_view uri, std::string& path);

    /*
    ** Пути файлов из списка `text/uri-list` (RFC 2483): строки через CRLF (терпим и голый LF), строки
    ** с `#` — комментарии, пустые и не-файловые адреса пропускаются. Порядок путей — как в списке.
    **
    ** The file paths from a `text/uri-list` (RFC 2483): lines separated by CRLF (a bare LF is tolerated too),
    ** lines with `#` are comments, empty and non-file addresses are skipped. The path order is as in the list.
    */
    std::vector<std::string> parseUriList(std::string_view text);
}

#endif
