#include "framework/test_framework.hpp"
#include "suites/test_suites.hpp"
#include "gran/utils/file_system.hpp"

#include <filesystem>
#include <fstream>
#include <string>
#include <cstdlib>
#include <system_error>

/*
** Края слоя путей, найденные ревью Т-57: длинные пути Windows (> 260 символов) и висячие ссылки.
** Тест сам создаёт ссылки через std::filesystem; на Windows без права создавать ссылки (нет режима разработчика)
** проверка ссылок пропускается.
**
** The path layer edges found by the T-57 review: long Windows paths (> 260 symbols) and dangling links.
** The test creates the links itself through std::filesystem; on Windows without the right to create links (no developer mode)
** the link check is skipped.
*/
namespace
{
    // Удаление дерева через сам слой: std::filesystem::remove_all из libstdc++ MinGW зависает на путях длиннее 260 символов
    // Removing a tree through the layer itself: std::filesystem::remove_all of the MinGW libstdc++ hangs on paths longer than 260 symbols
    void removeTree(const std::string& path)
    {
        for (const gran::fs::DirectoryEntry& entry : gran::fs::listDirectory(path))
        {
            if (entry.info.directory && !entry.info.link)
                removeTree(gran::fs::joinPath(path, entry.name));
            else
                gran::fs::removePath(gran::fs::joinPath(path, entry.name));
        }
        gran::fs::removePath(path);
    }

    // Чистая папка теста / A clean test folder
    std::string freshFolder(const char* name)
    {
        std::string root = gran::fs::joinPath(gran::fs::temporaryDirectory(), name);

        removeTree(root);
        gran::fs::createDirectories(root);
        return root;
    }

    // Путь длиннее 300 символов: создать, записать, найти обходом, переименовать, удалить
    // A path longer than 300 symbols: create, write, find by walking, rename, delete
    void testLongPath()
    {
        std::string root = freshFolder("gran_fs_long");
        std::string deep = root;
        std::ofstream output;

        for (int level = 0; level < 5; ++level)
            deep = gran::fs::joinPath(deep, std::string(60, static_cast<char>('a' + level)));
        TEST_ASSERT(deep.size() > 300);
        TEST_ASSERT(gran::fs::createDirectories(deep) && gran::fs::isDirectory(deep));
        gran::fs::openFile(output, gran::fs::joinPath(deep, "file.txt"));
        output << "long";
        output.close();
        TEST_ASSERT(!output.fail());
        TEST_ASSERT(gran::fs::queryPath(gran::fs::joinPath(deep, "file.txt")).size == 4);
        TEST_ASSERT(gran::fs::listFiles(root, true).size() == 1);
        TEST_ASSERT(gran::fs::renamePath(gran::fs::joinPath(deep, "file.txt"), gran::fs::joinPath(deep, "moved.txt")));
        TEST_ASSERT(gran::fs::removePath(gran::fs::joinPath(deep, "moved.txt")));
        TEST_ASSERT(gran::fs::removePath(deep) && !gran::fs::pathExists(deep));
        removeTree(root);
        TEST_ASSERT(!gran::fs::pathExists(root));
    }

    // Висячая ссылка — пути нет (как stat на POSIX), но link == true, и removePath удаляет саму ссылку
    // A dangling link means no path (as stat on POSIX), but link == true, and removePath deletes the link itself
    void testDanglingLink()
    {
        std::string root = freshFolder("gran_fs_links");
        std::string target = gran::fs::joinPath(root, "target.txt");
        std::string link = gran::fs::joinPath(root, "link.txt");
        std::error_code error;
        std::ofstream output;

        gran::fs::openFile(output, target);
        output << "abc";
        output.close();
        std::filesystem::create_symlink(std::filesystem::u8path(target), std::filesystem::u8path(link), error);
        if (error)
            return;
        TEST_ASSERT(gran::fs::queryPath(link).exists && gran::fs::queryPath(link).regular);
        TEST_ASSERT(gran::fs::queryPath(link).link && gran::fs::queryPath(link).size == 3);
        TEST_ASSERT(gran::fs::removePath(target));
        TEST_ASSERT(!gran::fs::queryPath(link).exists && gran::fs::queryPath(link).link);
        TEST_ASSERT(!gran::fs::pathExists(link) && gran::fs::listFiles(root, true).empty());
        TEST_ASSERT(gran::fs::removePath(link) && !gran::fs::queryPath(link).link);
    }

#if defined(_WIN32)
    /*
    ** Точка соединения (mklink /J) не требует прав, в отличие от символической ссылки: ссылка на папку, обход в неё не
    ** спускается, после удаления цели она висячая, removePath удаляет саму точку, а не цель.
    **
    ** A junction (mklink /J) needs no rights, unlike a symbolic link: a link to a folder, the walk does not enter it,
    ** after the target is deleted it dangles, removePath deletes the point itself, not the target.
    */
    void testJunction()
    {
        std::string root = freshFolder("gran_fs_junction");
        std::string target = gran::fs::joinPath(root, "target");
        std::string junction = gran::fs::joinPath(root, "junction");
        std::string paths = "\"" + junction + "\" \"" + target + "\"";
        std::ofstream output;

        // mklink понимает только «\»; ключи («/J») не трогаем / mklink understands only "\"; the switches ("/J") stay untouched
        for (char& symbol : paths)
        {
            if (symbol == '/')
                symbol = '\\';
        }
        std::string command = "mklink /J " + paths + " >NUL";
        gran::fs::createDirectories(target);
        gran::fs::openFile(output, gran::fs::joinPath(target, "inside.txt"));
        output.close();
        if (std::system(command.c_str()) != 0)
            return;
        TEST_ASSERT(gran::fs::queryPath(junction).directory && gran::fs::queryPath(junction).link);
        TEST_ASSERT(gran::fs::listFiles(root, true).size() == 1);
        TEST_ASSERT(gran::fs::removePath(gran::fs::joinPath(target, "inside.txt")) && gran::fs::removePath(target));
        TEST_ASSERT(!gran::fs::queryPath(junction).exists && gran::fs::queryPath(junction).link);
        TEST_ASSERT(gran::fs::removePath(junction) && !gran::fs::queryPath(junction).link);
        removeTree(root);
    }
#endif
}

namespace suites
{
    void registerFileSystemEdgeTests(test::TestRunner& runner)
    {
        runner.add("utils: fs long path", testLongPath);
        runner.add("utils: fs dangling link", testDanglingLink);
#if defined(_WIN32)
        runner.add("utils: fs junction", testJunction);
#endif
    }
}
