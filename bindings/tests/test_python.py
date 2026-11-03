import unittest
import sys
import os
import gc
import ctypes as C
from pathlib import Path

# Add repo bindings/python to sys.path
sys.path.insert(0, str(Path(__file__).resolve().parents[2] / "bindings" / "python"))

from granui import Application, SIZE_MAX

class TestApplication(unittest.TestCase):
    def setUp(self):
        backend = 1 if os.name == "nt" else 3
        self.app = Application(backend=backend, warp=True, exit_after=2.0)
        self.app.load_string('<Window title="BindingsPython" width="320" height="200"><Stack id="root"><Button id="go" text="Go" onClick="business"/><Text id="text">Привет 🌲</Text></Stack></Window>')

    def tearDown(self):
        self.app.close()

    def test_context_manager(self):
        with Application() as app:
            self.assertIsNotNone(app._app)
        self.assertFalse(app._app)

    def test_utf8_non_bmp_text(self):
        self.app.load_string('<Window title="BindingsPython" width="320" height="200"><Stack id="root"><Button id="go" text="Go" onClick="business"/><Text id="text">Привет 🌲</Text></Stack></Window>')
        text = self.app.read("text", "text")
        self.assertEqual(text, "Привет 🌲")

    def test_model_operations(self):
        self.app.model_insert("test_model", {"field1": "value1"})
        count = self.app.model_count("test_model")
        self.assertEqual(count, 1)
        self.assertEqual(self.app.model_read("test_model", 0, "field1"), "value1")
        self.app.model_set("test_model", 0, {"field1": "value2"})
        self.assertEqual(self.app.model_read("test_model", 0, "field1"), "value2")
        self.app.model_remove("test_model", 0)
        count = self.app.model_count("test_model")
        self.assertEqual(count, 0)
        self.app.model_clear("test_model")
        count = self.app.model_count("test_model")
        self.assertEqual(count, 0)

    def test_models(self):
        # Дерево, плоское представление, прокси с сортировкой, выделение / A tree, a flat view, a sorted proxy, a selection
        top = self.app.tree_insert("org", 0, {"name": "Б"})
        self.app.tree_insert("org", top, {"name": "Б1"})
        self.app.tree_insert("org", 0, {"name": "А"})
        self.assertEqual(len(self.app.tree_children("org")), 2)
        self.app.create_tree_list("orgRows", "org", sort="name", expanded=1)
        self.assertEqual(self.app.model_count("orgRows"), 3)
        self.assertEqual(self.app.model_read("orgRows", 0, "name"), "А")
        self.assertEqual(self.app.tree_row("orgRows", 2)["depth"], 1)
        for value in ("2", "10", "1"):
            self.app.model_insert("nums", {"n": value})
        self.app.create_proxy("sorted", "nums", sort="n:number desc")
        self.assertEqual(self.app.model_read("sorted", 0, "n"), "10")
        self.assertEqual(self.app.source_index("sorted", 0), 1)
        self.app.create_selection("pick", "sorted")
        seen = []
        self.app.on_selection_changed("pick", seen.append)
        self.app.selection_change("pick", "range", 0, 1)
        self.assertEqual(self.app.selection("pick"), (None, [0, 1]))
        self.assertEqual(seen, ["pick"])

    def test_events_listen_dispatch(self):
        # Своё событие: всплытие от go к root, слушатель кода и обработчик разметки on="ping:named"
        # A custom event: bubbling from go to root, a code listener and the on="ping:named" markup handler
        self.app.command("appendMarkup", "root", '<Stack id="box" on="ping:named"><Button id="inner" text="I"/></Stack>')
        seen = []
        def listener(event):
            seen.append((event.type, event.target, event.current, event.phase, event.detail))
            event.prevent_default()
        def named(event):
            seen.append(("named", event.current))
        number = self.app.listen("root", "ping", listener)
        self.app.on_event("named", named)
        self.assertFalse(self.app.dispatch("inner", "ping", "data"))
        self.assertEqual(seen, [("named", "box"), ("ping", "inner", "root", 3, "data")])
        self.app.unlisten("root", number)
        self.assertTrue(self.app.dispatch("inner", "ping"))

    def test_invalid_id_errors(self):
        with self.assertRaises(RuntimeError):
            self.app.read("invalid_id", "property")

    def test_valid_xml_kept_after_invalid_xml(self):
        self.app.load_string('<Window title="BindingsPython" width="320" height="200"><Stack id="root"><Button id="go" text="Go" onClick="business"/><Text id="text">Привет 🌲</Text></Stack></Window>')
        with self.assertRaises(RuntimeError):
            self.app.load_string('<InvalidXML>')
        text = self.app.read("text", "text")
        self.assertEqual(text, "Привет 🌲")

    def test_command_append_markup_remove(self):
        self.app.command("appendMarkup", "root", '<Button id="new_button" text="New Button"/>')
        self.assertEqual(self.app.read("new_button", "text"), "New Button")
        self.app.command("remove", "new_button")
        with self.assertRaises(RuntimeError):
            self.app.read("new_button", "text")

    def test_commands(self):
        self.app.load_string('<Window title="BindingsPython" width="320" height="200"><Resources><Command id="saveDoc" text="Save" key="Mod+S" action="business"/></Resources><Stack id="root"><Button id="save" command="saveDoc"/></Stack></Window>')
        calls = []
        self.app.on_action("business", lambda event: calls.append(event["target"]))
        self.assertEqual(self.app.read("save", "text"), "Save")
        self.app.run_command("saveDoc")
        self.assertEqual(calls, ["root"])
        self.app.set_command_enabled("saveDoc", False)
        self.assertEqual(self.app.read("save", "enabled"), "false")
        self.app.run_command("saveDoc")
        self.assertEqual(len(calls), 1)
        self.app.set_command_enabled("saveDoc", True)
        self.assertEqual(self.app.read("save", "enabled"), "true")

    def test_undo_stack(self):
        # Команды gran:undo/gran:redo следуют за стеком; шаг кладётся выполненным, release — при отбрасывании
        # The gran:undo/gran:redo commands follow the stack; a step is pushed done, release — when dropped
        self.app.load_string('<Window title="BindingsPython" width="320" height="200"><Resources><Command id="undo" key="Mod+Z" action="gran:undo"/><Command id="redo" key="Mod+Y" action="gran:redo"/></Resources><Stack id="root"><Button id="u" command="undo" text="U"/><Button id="r" command="redo" text="R"/></Stack></Window>')
        values, changes = [0], []
        self.app.on_undo_changed(lambda stack, state: changes.append((stack, state)))
        self.assertEqual(self.app.read("u", "enabled"), "false")
        values[0] = 1
        self.app.push_undo(lambda: values.__setitem__(0, 0), lambda: values.__setitem__(0, 1))
        self.assertEqual(self.app.undo_state(), (True, False, False))
        self.assertEqual(self.app.read("u", "enabled"), "true")
        self.app.run_command("undo")
        self.assertEqual(values[0], 0)
        self.assertTrue(self.app.undo_state().clean)
        self.assertEqual(self.app.read("r", "enabled"), "true")
        self.app.redo()
        self.assertEqual(values[0], 1)
        self.app.set_undo_clean()
        self.assertTrue(self.app.undo_state().clean)
        self.assertEqual(changes[-1][0], "")
        self.app.clear_undo()
        self.assertEqual(len(self.app._undo_steps), 0)
        self.app.push_undo(lambda: None, lambda: None, "doc")
        self.assertEqual(self.app.undo_state("doc").can_undo, True)
        self.assertEqual(self.app.undo_state().can_undo, False)
        self.app.set_undo_limit(1, "doc")
        self.app.on_undo_changed(None)

    def test_live_markup(self):
        # Правка атрибута идёт в живую разметку; ошибка схемы оставляет прежнее значение; сохранённый XML её содержит
        # An attribute edit goes into the live markup; a schema error keeps the former value; the saved XML holds it
        self.assertIsNone(self.app.markup_attribute("root", "padding"))
        self.app.set_markup_attribute("root", "padding", "6")
        self.assertEqual(self.app.markup_attribute("root", "padding"), "6")
        with self.assertRaises(RuntimeError):
            self.app.set_markup_attribute("root", "padding", "wide")
        self.assertEqual(self.app.markup_attribute("root", "padding"), "6")
        self.assertIn('padding="6"', self.app.save_markup())
        self.app.set_markup_attribute("root", "padding", None)
        self.assertIsNone(self.app.markup_attribute("root", "padding"))
        self.assertEqual(self.app.read("text", "text"), "Привет 🌲")

    def test_components(self):
        # Экземпляр компонента из библиотеки samples/components/widgets.xml; правка параметра перестраивает развёртку
        # A component instance from the samples/components/widgets.xml library; a parameter edit rebuilds the expansion
        root = Path(__file__).resolve().parents[2]
        library = (root / "samples" / "components" / "widgets.xml").as_posix()
        self.app.load_string('<Window xmlns:xi="http://www.w3.org/2001/XInclude" xmlns:w="urn:gran:samples:widgets">'
                             '<Resources><xi:include href="' + library + '"/></Resources>'
                             '<w:Badge id="badge" text="Новое"/></Window>')
        self.assertEqual(self.app.markup_attribute("badge", "text"), "Новое")
        self.app.set_markup_attribute("badge", "text", "Старое")
        self.assertIn('<w:Badge id="badge" text="Старое"/>', self.app.save_markup())
        self.assertIn('name="Badge"', self.app.component_schema("urn:gran:samples:widgets"))
        self.assertEqual(self.app.component_schema("urn:none"), "")

    @unittest.skipUnless(os.name == "nt", "this case injects Win32 keyboard messages")
    def test_live_test_app(self):
        happened = []
        def on_action(snapshot):
            self.assertEqual(snapshot["target"], "go")
            self.assertEqual(snapshot["text"], "Go")
            self.assertEqual(snapshot["model"], "")
            self.assertEqual(snapshot["index"], SIZE_MAX)
            happened.append(True)
            self.app.apply([("text", "text", "done")])
            self.app.command("close")

        self.app.on_action("business", on_action)
        self.app.post(lambda: self.app.model_insert("test_model", {"field1": "value1"}))
        def press_button():
            user32 = C.windll.user32
            user32.FindWindowW.argtypes = [C.c_wchar_p, C.c_wchar_p]
            user32.FindWindowW.restype = C.c_void_p
            user32.PostMessageW.argtypes = [C.c_void_p, C.c_uint, C.c_size_t, C.c_ssize_t]
            user32.PostMessageW.restype = C.c_bool
            hwnd = user32.FindWindowW(None, "BindingsPython")
            self.assertTrue(hwnd)
            self.assertEqual(self.app.model_count("test_model"), 1)
            self.assertTrue(user32.PostMessageW(hwnd, 0x0100, 9, 0))
            self.assertTrue(user32.PostMessageW(hwnd, 0x0100, 13, 0))
        self.app.timer(0.05, press_button)
        self.app.load_string('<Window title="BindingsPython" width="320" height="200"><Stack id="root"><Button id="go" text="Go" onClick="business"/><Text id="text">Привет 🌲</Text></Stack></Window>')
        gc.collect()
        self.app.run()
        self.assertEqual(happened, [True])
        self.assertEqual(self.app.read("text", "text"), "done")

    def test_timer_callback(self):
        def task():
            self.app.model_insert("test_model", {"field1": "value2"})
            count = self.app.model_count("test_model")
            self.assertEqual(count, 2)
            self.app.command("close")

        self.app.model_insert("test_model", {"field1": "value1"})
        self.app.timer(0.05, task)
        self.app.run()

    def test_owned_modal_window(self):
        # Модальное окно с владельцем: результат close_window приходит в on_closed / A modal window with an owner: the close_window result comes to on_closed
        results = []
        def closed(result):
            results.append(result)
            self.app.command("close")
        def open_dialog():
            self.app.open_window('<Window title="Dialog" width="200" height="120" kind="dialog"><Text id="dlg">?</Text></Window>',
                                 is_file=False, owner="root", modal=True, on_closed=closed)
        self.app.timer(0.05, open_dialog)
        self.app.timer(0.4, lambda: self.app.close_window("dlg", "ok"))
        self.app.run()
        self.assertEqual(results, ["ok"])

    def test_timer_raises_value_error(self):
        def task():
            raise ValueError('callback marker')

        self.app.timer(0.05, task)
        with self.assertRaises(ValueError) as context:
            self.app.run()
        self.assertEqual(str(context.exception), 'callback marker')

    def test_app_close_inside_timer(self):
        def task():
            with self.assertRaises(RuntimeError):
                self.app.close()
            self.assertEqual(self.app.read("text", "text"), "Привет 🌲")
            self.app.command("close")

        self.app.timer(0.05, task)
        self.app.run()

    def test_tasks_released_after_call(self):
        # Одноразовые задачи и снятый таймер не копятся до close / One-shot tasks and a cleared timer do not pile up until close
        calls = []
        for _ in range(1000):
            self.app.post(lambda: calls.append(1))
        interval = self.app.timer(0.005, lambda: calls.append(2), repeat=True)

        def finish():
            self.app.clear_timer(interval)
            self.app.command("close")

        self.app.timer(0.05, finish)
        self.assertEqual(self.app.pending_tasks(), 1002)
        self.app.run()
        self.assertEqual(calls.count(1), 1000)
        self.assertEqual(self.app.pending_tasks(), 0)

    def test_binding_expressions(self):
        self.app.load_string('<Window><Resources><Variable name="flag" type="boolean" value="true"/></Resources>'
                             '<Stack><Input id="q" value="{$query}"/><Text id="t">{$size ; kb}</Text>'
                             '<Box id="b" visible="{$flag}"/></Stack></Window>')
        self.app.set_converter("kb", lambda value: value + " КБ")
        self.app.set_variable("size", 12)
        self.app.set_variable("flag", "false")
        self.app.apply([("q", "text", "поиск")])
        self.app.refresh_bindings()
        self.assertEqual(self.app.read("t", "text"), "12 КБ")
        self.assertEqual(self.app.read("b", "visible"), "false")
        self.assertEqual(self.app.variable("flag"), "false")
        self.assertEqual(self.app.variable("query"), "поиск")
        self.assertIsNone(self.app.variable("missing"))

    def test_use_after_close(self):
        self.app.close()
        with self.assertRaises(RuntimeError):
            self.app.load_string('<Window title="BindingsPython" width="320" height="200"><Stack id="root"><Button id="go" text="Go" onClick="business"/><Text id="text">Привет 🌲</Text></Stack></Window>')

if __name__ == '__main__':
    unittest.main()
