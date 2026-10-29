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

    def test_use_after_close(self):
        self.app.close()
        with self.assertRaises(RuntimeError):
            self.app.load_string('<Window title="BindingsPython" width="320" height="200"><Stack id="root"><Button id="go" text="Go" onClick="business"/><Text id="text">Привет 🌲</Text></Stack></Window>')

if __name__ == '__main__':
    unittest.main()
