using GranUI;

// Только бизнес-операция: внешний вид записи задаёт общий XML.
// Only a business operation: the shared XML defines record presentation.
using var app = new Application();
app.LoadFile("bindings/examples/counter.xml");
app.OnAction("add", _ => app.ModelInsert("rows", new Dictionary<string, string> { ["text"] = app.Read("name", "text") }));
app.Run();
