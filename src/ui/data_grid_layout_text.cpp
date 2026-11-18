#include "gran/ui/data_grid.hpp"
#include "gran/core/gran_exception.hpp"
#include "gran/style/value_parsers.hpp"

#include <cmath>
#include <locale>
#include <sstream>

namespace gran::ui
{
    namespace
    {
        // Самая широкая сохранённая ширина: больше — испорченная запись настроек / The widest saved width: more is a damaged settings entry
        constexpr float MAX_SAVED_WIDTH = 100000.0f;

        // Ширина изменённого столбца текстом: px — число до десятых, auto — «auto» (других пользователь не задаёт) / The changed column width as text: px — a number to tenths, auto — "auto" (the user sets no others)
        std::string widthText(const style::GridTrack& width)
        {
            std::ostringstream text;

            if (width.max.kind != style::TrackBoundKind::PIXELS)
                return "auto";
            text.imbue(std::locale::classic());
            text << std::round(width.max.value * 10.0f) / 10.0f;
            return text.str();
        }

        /*
        ** Ширина из сохранённой раскладки: «auto» или неотрицательное конечное число пикселей. Раскладка приходит
        ** из файла настроек (его мог испортить кто угодно), поэтому ошибка — не исключение, а «ширины нет»:
        ** столбец остаётся с шириной из разметки.
        **
        ** A width from a saved layout: "auto" or a non-negative finite number of pixels. The layout comes
        ** from the settings file (anyone could damage it), so an error is not an exception but "no width":
        ** the column keeps the width from the markup.
        */
        bool parseWidth(const std::string& text, style::GridTrack& width)
        {
            if (text == "auto")
            {
                width = style::GridTrack{};
                return true;
            }
            try
            {
                float value = style::parseNumber(text);
                if (!(value >= 0.0f && value <= MAX_SAVED_WIDTH))
                    return false;
                width.min = style::TrackBound{style::TrackBoundKind::PIXELS, value};
                width.max = width.min;
                return true;
            }
            catch (const GranException&)
            {
                return false;
            }
        }

        /*
        ** Токен «ключ[:ширина]»: первый ещё не поставленный столбец с этим ключом встаёт следующим (повтор ключа
        ** в испорченной раскладке не дублирует столбец), годная ширина становится шириной пользователя.
        **
        ** A "key[:width]" token: the first not yet placed column with this key stands next (a repeated key
        ** in a damaged layout does not duplicate a column), a valid width becomes the user width.
        */
        void placeToken(std::vector<GridColumnSpec>& specs, std::vector<bool>& placed, std::vector<GridColumnSpec>& order,
            const std::string& token)
        {
            std::size_t colon = token.find(':');
            std::string key = token.substr(0, colon);
            style::GridTrack width;

            for (std::size_t index = 0; index < specs.size(); ++index)
            {
                if (placed[index] || specs[index].column.key != key)
                    continue;
                placed[index] = true;
                if (colon != std::string::npos && parseWidth(token.substr(colon + 1), width))
                {
                    // Сохранённая ширина — в тех же пределах, что у мыши: 0 из настроек иначе «потерял» бы столбец (ревью Т-66)
                    // A saved width is within the same limits as for the mouse: 0 from the settings would otherwise "lose" the column (review T-66)
                    if (width.max.kind == style::TrackBoundKind::PIXELS)
                        width.min.value = width.max.value = limitColumnWidth(specs[index].column, width.max.value);
                    specs[index].column.width = width;
                    specs[index].column.resized = true;
                }
                order.push_back(std::move(specs[index]));
                return;
            }
        }
    }

    // Столбец без ключа в раскладку не попадает (его ключ задаёт читатель разметки: id, field или «#номер») / A column without a key does not get into the layout (the markup reader sets its key: id, field or "#index")
    std::string gridLayoutText(const GridState& state)
    {
        std::string text;

        for (const GridColumn& column : state.columns)
        {
            if (column.key.empty())
                continue;
            if (!text.empty())
                text += ' ';
            text += column.key;
            if (column.resized)
                text += ":" + widthText(column.width);
        }
        return text;
    }

    // Не поставленные раскладкой столбцы (новые в этой версии программы) идут следом в порядке разметки / The columns the layout did not place (new in this program version) follow in the markup order
    bool applyGridLayout(std::vector<GridColumnSpec>& specs, const std::string& text)
    {
        std::istringstream tokens(text);
        std::vector<bool> placed(specs.size(), false);
        std::vector<GridColumnSpec> order;
        std::string token;

        while (tokens >> token)
            placeToken(specs, placed, order, token);
        bool applied = !order.empty();
        for (std::size_t index = 0; index < specs.size(); ++index)
        {
            if (!placed[index])
                order.push_back(std::move(specs[index]));
        }
        specs = std::move(order);
        return applied;
    }
}
