#include "framework/test_framework.hpp"

#include <iostream>
#include <utility>

namespace test
{
    // Добавление теста в реестр
    // Adds a test to the registry
    void TestRunner::add(std::string name, TestFunction function)
    {
        _cases.push_back(TestCase{std::move(name), function});
    }

    // Последовательный запуск всех тестов с итоговой сводкой
    // Runs all tests one after another with a final summary
    int TestRunner::runAll() const
    {
        int failed = 0;

        for (const TestCase& testCase : _cases)
        {
            if (!runOne(testCase))
                ++failed;
        }
        std::cout << "\nИтого: " << _cases.size() << " тестов, пройдено "
                  << (static_cast<int>(_cases.size()) - failed)
                  << ", упало " << failed << '\n';
        return failed;
    }

    /*
    ** Один тест. Отдельно ловим TestFailure (проваленная проверка) и любое
    ** другое исключение (неожиданный сбой в коде библиотеки).
    **
    ** One test. TestFailure (a failed check) and any other exception
    ** (an unexpected failure in the library code) are caught separately.
    */
    bool TestRunner::runOne(const TestCase& testCase) const
    {
        try
        {
            testCase.function();
            std::cout << "[ OK ] " << testCase.name << '\n';
            return true;
        }
        catch (const TestFailure& failure)
        {
            std::cout << "[FAIL] " << testCase.name << "\n       " << failure.what() << '\n';
        }
        catch (const std::exception& error)
        {
            std::cout << "[FAIL] " << testCase.name
                      << "\n       неожиданное исключение: " << error.what() << '\n';
        }
        return false;
    }
}
