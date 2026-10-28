from pathlib import Path
import sys

root = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(root / "bindings/python"))
from granui import Application

# Только бизнес-операция: представление записи и привязка определены в XML.
# Only a business operation: record presentation and binding are defined in XML.
with Application() as app:
    app.load_file(str(root / "bindings/examples/counter.xml"))
    app.on_action("add", lambda event: app.model_insert("rows", {"text": app.read("name", "text")}))
    app.run()
