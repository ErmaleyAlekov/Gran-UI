import granui.Application;
import java.util.Map;

// Только бизнес-операция: элементы списка строятся ядром из шаблона XML.
// Only a business operation: the core builds list elements from the XML template.
public final class Counter {
    public static void main(String[] args) {
        try (var app = new Application(args[0])) {
            app.loadFile("bindings/examples/counter.xml");
            app.onAction("add", event -> app.modelInsert("rows", Map.of("text", app.read("name", "text"))));
            app.run();
        }
    }
}
