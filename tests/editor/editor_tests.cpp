#include <iostream>
#include <fstream>
#include <filesystem>
#include <stdexcept>
#include "gran/app/application.hpp"
#include "ui/controller.hpp"
#include "../../examples/editor/core/text.hpp"

namespace fs = std::filesystem;
unsigned checks = 0;

void require(bool condition, const std::string& message) {
    ++checks;
    if (!condition) {
        throw std::runtime_error(message);
    }
}

void setupTestEnvironment() {
    fs::create_directories("scratchpad/editor-test-data");
    std::ofstream a("scratchpad/editor-test-data/a.cpp");
    a << "int main() { return 0; }";
    a.close();
    std::ofstream b("scratchpad/editor-test-data/b.xml");
    b << "<root><child>Content</child></root>";
    b.close();
}



int main() {
    setupTestEnvironment();

    gran::Application app;
    editor::Controller controller(app);
    app.loadFile("samples/editor.xml");
    controller.start("");

    unsigned docId1 = 0;
    unsigned docId2 = 0;
    unsigned docId3 = 0;
    unsigned docId4 = 0;
    unsigned docId5 = 0;

    try {
        // Test initial new doc
        docId1 = 1;
        require(app.find("editorTabs")->children.size() == 1, "Initial document count should be 1");
        require(controller.document(docId1)->page->visible, "Active document should be the first one");

        // Test open a.cpp
        controller.openDocument("scratchpad/editor-test-data/a.cpp");
        docId2 = 2;
        require(app.find("editorTabs")->children.size() == 2, "Document count after opening a.cpp should be 2");
        require(controller.document(docId2)->page->visible, "Active document should be the second one");
        require(controller.document(docId2)->page->content.text == "int main() { return 0; }", "Content of a.cpp should be correct");

        // Test open canonical alias no duplicate count in editorTabs children
        controller.openDocument("scratchpad/editor-test-data/a.cpp");
        require(app.find("editorTabs")->children.size() == 2, "Document count after reopening a.cpp should be 2");
        require(controller.document(docId2)->page->visible, "Active document should still be the second one");

        // Test edit page via controller.document(id)->page->content.text and controller.changed
        controller.document(docId2)->page->content.text = "int main() { return 1; }";
        controller.document(docId2)->page->content.caret = 7;
        controller.changed(*controller.document(docId2)->page);
        require(controller.document(docId2)->page->content.text == "int main() { return 1; }", "Content of a.cpp should be updated");

        // Test switch b.xml and back retains text/caret
        controller.openDocument("scratchpad/editor-test-data/b.xml");
        docId3 = 3;
        require(app.find("editorTabs")->children.size() == 3, "Document count after opening b.xml should be 3");
        require(controller.document(docId3)->page->visible, "Active document should be the third one");
        require(controller.document(docId3)->page->content.text == "<root><child>Content</child></root>", "Content of b.xml should be correct");

        controller.select(docId2);
        require(controller.document(docId2)->page->visible, "Active document should switch back to the second one");
        require(controller.document(docId2)->page->content.text == "int main() { return 1; }", "Content of a.cpp should be retained");
        require(controller.document(docId2)->page->content.caret == 7, "Caret must survive tab switching");

        // Test closing inactive clean b preserves active a
        controller.requestClose(docId3);
        require(app.find("editorTabs")->children.size() == 2, "Document count after closing b.xml should be 2");
        require(controller.document(docId2)->page->visible, "Active document should still be the second one");

        // Test dirty a close shows savePrompt, cancel keeps, close again confirmSave persists to disk
        controller.document(docId2)->page->content.text = "int main() { return 2; }";
        controller.changed(*controller.document(docId2)->page);
        controller.requestClose(docId2);
        require(app.find("savePromptText")->content.text.find("a.cpp") != std::string::npos,
            "Dirty close must prepare confirmation for the correct document");
        controller.execute("newFile", 0);
        require(app.find("editorTabs")->children.size() == 2, "Pending confirmation must block competing actions");
        require(app.find("editorTabs")->children.size() == 2, "Document count after requesting close should be 2");
        require(controller.document(docId2)->page->visible, "Active document should still be the second one");
        require(controller.document(docId2)->page->content.text == "int main() { return 2; }", "Content of a.cpp should be retained");

        controller.answerClose("cancelClose");
        require(app.find("editorTabs")->children.size() == 2, "Document count after canceling close should be 2");
        require(controller.document(docId2)->page->visible, "Active document should still be the second one");
        require(controller.document(docId2)->page->content.text == "int main() { return 2; }", "Content of a.cpp should be retained");

        controller.requestClose(docId2);
        controller.answerClose("confirmSave");
        require(app.find("editorTabs")->children.size() == 1, "Document count after confirming close should be 1");
        require(controller.document(docId1)->page->visible, "Active document should switch to the first one");
        require(controller.document(docId2) == nullptr, "Document 2 should be null after closing");
        std::string disk;
        require(editor::readDocument("scratchpad/editor-test-data/a.cpp", disk), "Saved file must be readable");
        require(disk == "int main() { return 2; }", "Save confirmation must write the buffer to disk");

        // Test reopen restores saved a
        controller.reopen();
        docId4 = 4;
        require(app.find("editorTabs")->children.size() == 2, "Document count after reopening a.cpp should be 2");
        require(controller.document(docId4)->page->visible, "Active document should be the fourth one");
        require(controller.document(docId4)->page->content.text == "int main() { return 2; }", "Content of a.cpp should be restored");

        // Test clean closeAll leads zero tabs and visible emptyEditor
        controller.closeAll(false);
        require(app.find("editorTabs")->children.size() == 0, "Document count after closing all should be 0");
        require(app.find("emptyEditor")->visible, "Editor should be visible after closing all");

        // Test newFile works again
        controller.newDocument();
        docId5 = 5;
        require(app.find("editorTabs")->children.size() == 1, "Document count after creating a new file should be 1");
        require(controller.document(docId5)->page->visible, "Active document should be the fifth one");

        require(editor::cursorLabel(u8"аб\nя", 7) == "Ln 2, Col 2", "Cursor counts UTF-8 codepoints");
        require(editor::cursorLabel("a\r\nb", 99) == "Ln 2, Col 2", "Cursor clamps and handles CRLF");
        require(editor::languageLabel("file.xml") == "XML", "XML language");
        require(editor::languageLabel("file.cpp") == "C++", "C++ language");
        require(editor::languageLabel("CMakeLists.txt") == "CMake", "CMake filename");
        controller.openDocument("scratchpad/editor-test-data/a.cpp");
        controller.document(6)->page->content.text = "local change";
        std::ofstream("scratchpad/editor-test-data/a.cpp") << "external change";
        require(!controller.saveDocument(6, false), "External changes must block saving");
        require(editor::readDocument("scratchpad/editor-test-data/a.cpp", disk) && disk == "external change",
            "External contents must remain intact");
        std::ofstream("scratchpad/editor-test-data/binary.dat", std::ios::binary).put('\0');
        require(!editor::readDocument("scratchpad/editor-test-data/binary.dat", disk), "Reject binary files");
        require(!editor::writeDocument("scratchpad/editor-test-data/missing/file.txt", "x"), "Failed save reports failure");
        std::cout << checks << " checks passed." << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "Test failed: " << e.what() << std::endl;
        return 1;
    }


    return 0;
}
