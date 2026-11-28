"""Совместимость схемы разметки / Markup schema compatibility."""
from xml.etree import ElementTree as ET
from compat_schema_02 import reviewed_addition

XSD = "{http://www.w3.org/2001/XMLSchema}"
# Исправления сравнений xs:boolean между 0.2 и 0.3 / Typed boolean fixes between 0.2 and 0.3
BOOLEAN_FIXES = {
    "not(@virtual = 'true' or @virtual = '1') or exists(@items)": "not(@virtual = true()) or exists(@items)",
    "not(@collapsed = 'true') or @collapse != 'none'": "not(@collapsed = true()) or @collapse != 'none'",
    "not((@required = 'true' or @required = '1') and exists(@default))":
        "not(@required = true() and exists(@default))",
}


def identity(node):
    return node.tag, node.get("name"), node.get("ref")


def addition(parent, child):
    if child.tag == XSD + "attribute":
        return (parent.find(XSD + "anyAttribute") is None and child.get("use", "optional") == "optional"
                and child.get("default") is None and child.get("fixed") is None)
    if child.tag == XSD + "element":
        return parent.tag in (XSD + "choice", XSD + "schema") or (
            parent.tag in (XSD + "sequence", XSD + "all") and child.get("minOccurs") == "0")
    return parent.tag == XSD + "schema" and child.tag in (XSD + "simpleType", XSD + "complexType")


def compare(old, new, strict, boolean_fixes, path="schema"):
    attrs = dict(old.attrib)
    transition = boolean_fixes and not strict
    if (transition and path == "schema/PlainAction/union/simpleType/restriction/pattern"
            and new.get("value", "").startswith(old.attrib["value"] + "|")):
        attrs["value"] = new.get("value")
    if boolean_fixes and old.tag == XSD + "assert" and BOOLEAN_FIXES.get(old.get("test")) == new.get("test"):
        attrs["test"] = new.get("test")
    if identity(old) != identity(new) or attrs != new.attrib:
        raise ValueError(f"XSD {path}: {old.attrib} -> {new.attrib}")
    before = [node for node in old if node.tag != XSD + "annotation"]
    index = 0
    for node in new:
        if node.tag == XSD + "annotation":
            continue
        if transition and reviewed_addition(path, node):
            continue
        label = path + "/" + (node.get("name") or node.tag.removeprefix(XSD))
        if index < len(before) and identity(before[index]) == identity(node):
            compare(before[index], node, strict, boolean_fixes, label)
            index += 1
        elif strict or not addition(new, node):
            raise ValueError(f"XSD {label}: unexpected declaration {node.attrib}")
    if index < len(before):
        raise ValueError(f"XSD {path}: missing declaration {before[index].attrib}")
