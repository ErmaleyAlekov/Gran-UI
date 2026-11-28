"""Проверенные расширения XSD после 0.2 / Reviewed XSD extensions since 0.2."""
XSD = "{http://www.w3.org/2001/XMLSchema}"

# При отсутствии новых атрибутов условия истинны / True in the absence of the new attributes.
ASSERTIONS = {
    "schema/ElementBase": {
        "not(@minTargetSize) or not(number(@width) lt @minTargetSize or number(@height) lt @minTargetSize)",
    },
    "schema/TokenType": {
        "not(@min or @max) or @type = 'size' or @type = 'density' or @type = 'radius'",
        "not(@type = 'size' or @type = 'density' or @type = 'radius') or number(@value) ge 0",
        "not(@min) or number(@value) ge @min",
        "not(@max) or number(@value) le @max",
    },
}

# id команды уже обязателен и уникален в окне / Command ids were already required and unique.
# Остальные селекторы относятся только к новой Palette / Other selectors only cover the new Palette.
KEYS = {
    "commandKey": ("key", None, "Resources/Command", "@id"),
    "paletteCommandRef": ("keyref", "commandKey", "Resources/Palette/CommandRef", "@ref"),
    "uniquePaletteCommand": ("unique", None, "Resources/Palette/CommandRef", "@ref"),
    "uniquePaletteTemplate": ("unique", None, "Resources/Palette/Template", "@name"),
}


def reviewed_key(node):
    name = node.get("name")
    if name not in KEYS or len(node) != 2:
        return False
    tag, refer, selector, field = KEYS[name]
    attrs = {"name": name}
    if refer:
        attrs["refer"] = refer
    return (node.tag == XSD + tag and node.attrib == attrs
            and node[0].tag == XSD + "selector" and node[0].attrib == {"xpath": selector}
            and node[1].tag == XSD + "field" and node[1].attrib == {"xpath": field})


def reviewed_addition(path, node):
    if node.tag == XSD + "assert":
        return node.get("test") in ASSERTIONS.get(path, ()) and len(node.attrib) == 1
    if node.tag == XSD + "attributeGroup":
        if path == "schema" and node.attrib == {"name": "DesignLimits"}:
            return all(child.tag == XSD + "attribute" and set(child.attrib) == {"name", "type"}
                       for child in node)
        return path in ("schema/ElementBase", "schema/WindowType") and node.attrib == {"ref": "DesignLimits"}
    return path == "schema/Window" and reviewed_key(node)
