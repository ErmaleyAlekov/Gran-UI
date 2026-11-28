"""Регрессии сравнения схем / Schema comparison regressions."""
import unittest
from xml.etree import ElementTree as ET

from compat_schema import compare


def schema(body):
    return ET.fromstring(f'<xs:complexType xmlns:xs="http://www.w3.org/2001/XMLSchema">{body}</xs:complexType>')


class CompatibilityTests(unittest.TestCase):
    def test_optional_attribute(self):
        old = schema("")
        new = schema('<xs:attribute name="count" type="xs:integer"/>')
        compare(old, new, False, False)
        with self.assertRaises(ValueError):
            compare(old, new, True, False)

    def test_required_attribute(self):
        with self.assertRaises(ValueError):
            compare(schema(""), schema('<xs:attribute name="count" use="required"/>'), False, False)

    def test_wildcard_narrowing(self):
        wildcard = '<xs:anyAttribute processContents="skip"/>'
        typed = '<xs:attribute name="count" type="xs:integer"/>'
        with self.assertRaises(ValueError):
            compare(schema(wildcard), schema(typed + wildcard), False, False)

    def test_sequence_order(self):
        first = '<xs:element name="First"/>'
        last = '<xs:element name="Last"/>'
        old = schema(f"<xs:sequence>{first}{last}</xs:sequence>")
        with self.assertRaises(ValueError):
            compare(old, schema(f"<xs:sequence>{last}{first}</xs:sequence>"), False, False)

    def test_release_assertion(self):
        existing = '<xs:assert test="not(contains(@value, &quot;$&quot;))"/>'
        reviewed = '<xs:assert test="not(@min) or number(@value) ge @min"/>'
        old, new = schema(existing), schema(reviewed + existing)
        compare(old, new, False, True, "schema/TokenType")
        for strict, transition, path in [(True, False, "schema/TokenType"), (False, True, "schema/Window")]:
            with self.assertRaises(ValueError):
                compare(old, new, strict, transition, path)

    def test_release_key_selector(self):
        key = ('<xs:key name="commandKey"><xs:selector xpath="Resources/Command"/>'
               '<xs:field xpath="@id"/></xs:key>')
        compare(schema(""), schema(key), False, True, "schema/Window")
        with self.assertRaises(ValueError):
            compare(schema(""), schema(key.replace("Resources/Command", ".//*")), False, True, "schema/Window")

    def test_plain_action_extension(self):
        path = "schema/PlainAction/union/simpleType/restriction/pattern"
        old = schema('<xs:pattern value="a|b"/>')[0]
        new = schema('<xs:pattern value="a|b|c"/>')[0]
        compare(old, new, False, True, path)
        with self.assertRaises(ValueError):
            compare(new, old, False, True, path)


if __name__ == "__main__":
    unittest.main()
