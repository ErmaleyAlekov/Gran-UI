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


if __name__ == "__main__":
    unittest.main()
