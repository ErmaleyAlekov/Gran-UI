# Генератор данных примера samples/variable_rows.xml: 1000 заметок с текстом разной длины.
# Детерминированный (свой линейный конгруэнтный генератор), поэтому файл не меняется от запуска к запуску.
# The data generator for the samples/variable_rows.xml example: 1000 notes with texts of different lengths.
# Deterministic (an own linear congruential generator), so the file does not change from run to run.
#
# Запуск / Run: python tools/samples/make_notes.py samples/data/notes.json
import json
import sys

WORDS = ("окно список строка высота прокрутка запись разметка стиль шрифт кадр якорь среднее размер "
         "индекс дерево таблица столбец ячейка выделение фокус клавиша мышь колесо полоса").split()


class Lcg:
    # Генератор псевдослучайных чисел / A pseudo-random number generator
    def __init__(self, seed):
        self.state = seed

    def next(self, limit):
        self.state = (self.state * 1103515245 + 12345) % 2147483648
        return self.state % limit


def sentence(rng):
    # Предложение из 4–12 слов с заглавной буквы / A sentence of 4-12 words starting with a capital letter
    words = [WORDS[rng.next(len(WORDS))] for _ in range(4 + rng.next(9))]
    return words[0].capitalize() + " " + " ".join(words[1:]) + "."


def note(rng, number):
    # Блоки по 40 записей: короткие, затем длинные — самый трудный случай для оценки по среднему
    # Blocks of 40 records: short, then long — the hardest case for an estimate by the average
    count = rng.next(2)
    if (number // 40) % 2 == 1:
        count = 3 + rng.next(6)
    body = " ".join(sentence(rng) for _ in range(count))
    return {"number": number + 1, "title": sentence(rng)[:-1], "body": body}


def main():
    rng = Lcg(2026)
    # По записи на строку: файл компактный, а разница между версиями читается
    # A record per line: the file is compact, and a diff between versions is readable
    lines = [json.dumps(note(rng, number), ensure_ascii=False, separators=(",", ":")) for number in range(1000)]
    with open(sys.argv[1], "w", encoding="utf-8", newline="\n") as output:
        output.write('{"source":"Gran UI sample","notes":[\n' + ",\n".join(lines) + "\n]}\n")


if __name__ == "__main__":
    main()
