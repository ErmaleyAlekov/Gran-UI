#include "gran/platform/cocoa/internal/cocoa_window_impl.hpp"
#include "gran/platform/file_dialog.hpp"

namespace gran::platform::cocoa
{
    namespace
    {
        /*
        ** Расширения из шаблонов «*.png;*.jpg» в список типов панели. Своих вкладок типов у панелей macOS нет,
        ** поэтому берётся объединение всех фильтров; шаблон «*» или «*.*» — любые файлы (false).
        **
        ** The extensions from the "*.png;*.jpg" patterns into the panel type list. The macOS panels have no own type tabs,
        ** so the union of all the filters is taken; a "*" or "*.*" pattern means any files (false).
        */
        bool addPatterns(NSMutableArray<NSString*>* types, const std::string& patterns)
        {
            std::size_t start = 0;

            while (start <= patterns.size())
            {
                std::size_t end = patterns.find(';', start);
                if (end == std::string::npos)
                    end = patterns.size();
                std::string pattern = patterns.substr(start, end - start);
                if (pattern == "*" || pattern == "*.*")
                    return false;
                NSString* extension = nil;
                if (pattern.size() > 2 && pattern.compare(0, 2, "*.") == 0)
                    extension = [NSString stringWithUTF8String:pattern.c_str() + 2];
                if (extension != nil)
                    [types addObject:extension];
                start = end + 1;
            }
            return true;
        }

        // Типы панели; nil — любые файлы. Без фильтров, но с расширением по умолчанию — только оно
        // The panel types; nil — any files. Without filters but with a default extension — only that one
        NSArray<NSString*>* fileTypes(const FileDialogOptions& options)
        {
            NSMutableArray<NSString*>* types = [NSMutableArray array];

            for (const FileFilter& filter : options.filters)
            {
                if (!addPatterns(types, filter.patterns))
                    return nil;
            }
            NSString* fallback = [NSString stringWithUTF8String:options.defaultExtension.c_str()];
            if (types.count == 0 && fallback.length != 0)
                [types addObject:fallback];
            if (types.count == 0)
                return nil;
            return types;
        }

        // Панель по виду диалога: сохранение — NSSavePanel, открытие и папка — NSOpenPanel
        // The panel by the dialog kind: saving — NSSavePanel, opening and a folder — NSOpenPanel
        NSSavePanel* makePanel(FileDialogKind kind)
        {
            if (kind == FileDialogKind::SAVE)
                return [NSSavePanel savePanel];
            NSOpenPanel* panel = [NSOpenPanel openPanel];
            panel.allowsMultipleSelection = kind == FileDialogKind::OPEN_MULTIPLE;
            panel.canChooseDirectories = kind == FileDialogKind::FOLDER;
            panel.canChooseFiles = kind != FileDialogKind::FOLDER;
            panel.canCreateDirectories = kind == FileDialogKind::FOLDER;
            return panel;
        }

        /*
        ** Заголовок, начальная папка, имя файла и типы. Расширение по умолчанию NSSavePanel допишет сама: оно
        ** первое в списке типов, а allowsOtherFileTypes разрешает написать другое.
        **
        ** The title, the initial folder, the file name and the types. NSSavePanel appends the default extension itself:
        ** it is the first in the type list, and allowsOtherFileTypes allows typing another one.
        */
        void configure(NSSavePanel* panel, FileDialogKind kind, const FileDialogOptions& options)
        {
            NSString* title = [NSString stringWithUTF8String:options.title.c_str()];
            NSString* folder = [NSString stringWithUTF8String:options.folder.c_str()];
            NSString* name = [NSString stringWithUTF8String:options.fileName.c_str()];

            if (title.length != 0)
                panel.message = title;
            if (folder.length != 0)
                panel.directoryURL = [NSURL fileURLWithPath:folder isDirectory:YES];
            if (name.length != 0 && kind == FileDialogKind::SAVE)
                panel.nameFieldStringValue = name;
            if (kind == FileDialogKind::FOLDER)
                return;
            panel.allowedFileTypes = fileTypes(options);
            panel.allowsOtherFileTypes = YES;
        }
    }
}

namespace gran::platform
{
    /*
    ** Модальный системный диалог (runModal — модален для всей программы, поэтому owner не нужен): пока он открыт,
    ** окна копят события. Пути — UTF-8 (path у NSURL файла); отказ — пустой список.
    **
    ** A modal system dialog (runModal is modal for the whole program, so owner is not needed): while it is open, the
    ** windows accumulate events. The paths are UTF-8 (path of a file NSURL); a refusal is an empty list.
    */
    std::vector<std::string> showFileDialog(FileDialogKind kind, const FileDialogOptions& options, Window* owner)
    {
        std::vector<std::string> paths;

        (void)owner;
        cocoa::ensureApplication();
        @autoreleasepool
        {
            NSSavePanel* panel = cocoa::makePanel(kind);
            cocoa::configure(panel, kind, options);
            if ([panel runModal] != NSModalResponseOK)
                return paths;
            // Панель открытия — подкласс панели сохранения: id приводится к нему неявно
            // The open panel is a subclass of the save panel: id converts to it implicitly
            id any = panel;
            NSOpenPanel* open = any;
            NSArray<NSURL*>* urls = @[];
            if (kind == FileDialogKind::SAVE && panel.URL != nil)
                urls = @[panel.URL];
            if (kind != FileDialogKind::SAVE)
                urls = open.URLs;
            for (NSURL* url in urls)
            {
                if (url.path.UTF8String != nullptr)
                    paths.emplace_back(url.path.UTF8String);
            }
        }
        return paths;
    }
}
