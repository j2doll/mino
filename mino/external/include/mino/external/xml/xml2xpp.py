import sys
import os
import argparse
import xml.etree.ElementTree as ET
from collections import defaultdict
import re

def infer_cpp_type(val_str: str) -> str:
    """문자열 값을 분석하여 적절한 C++ 타입을 추론합니다."""
    val = val_str.strip()
    if not val:
        return "std::string"
    if val.lower() in ("true", "false"):
        return "bool"
    try:
        int(val)
        return "int"
    except ValueError:
        pass
    try:
        float(val)
        return "double"
    except ValueError:
        pass
    return "std::string"

def clean_tag(tag: str) -> str:
    """XML 태그/네임스페이스를 C++ 식별자(PascalCase)로 정규화합니다."""
    tag = re.sub(r"\{.*?\}", "", tag)
    parts = tag.replace("-", "_").split("_")
    return "".join(p.capitalize() for p in parts)

def generate_cpp_structs(xml_content: str) -> str:
    """XML 문자열로부터 직렬화/역직렬화 메서드가 포함된 C++ 구조체를 생성합니다."""
    if isinstance(xml_content, str):
        root = ET.fromstring(xml_content.encode("utf-8"))
    else:
        root = ET.fromstring(xml_content)

    struct_definitions = []
    generated_structs = set()

    def parse_element(elem: ET.Element) -> str:
        struct_name = clean_tag(elem.tag)

        # 자식 엘리먼트 그룹화 (문자열 태그만 수집하여 주석 필터링)
        child_groups = defaultdict(list)
        for child in elem:
            if isinstance(child.tag, str):
                child_groups[child.tag].append(child)

        fields = []

        # 1. XML 속성(Attribute) 수집
        for attr_name, attr_val in elem.attrib.items():
            attr_type = infer_cpp_type(attr_val)
            safe_name = attr_name.replace("-", "_")
            fields.append({
                "name": safe_name,
                "orig_xml_name": attr_name,
                "cpp_type": attr_type,
                "is_nested": False,
                "is_array": False,
                "kind": "attribute"
            })

        # 2. 자식 요소가 없고 텍스트와 속성을 동시에 가지는 경우 (예: <price unit="KRW">25000</price>)
        has_children = len(child_groups) > 0
        has_text = elem.text and elem.text.strip()
        if not has_children and len(elem.attrib) > 0 and has_text:
            text_type = infer_cpp_type(elem.text)
            fields.append({
                "name": "value",
                "orig_xml_name": "",
                "cpp_type": text_type,
                "is_nested": False,
                "is_array": False,
                "kind": "text_body"
            })

        # 3. 자식 요소(Element) 수집
        for tag, children in child_groups.items():
            is_array = len(children) > 1
            sample = children[0]
            # 자식이 또 다른 하위 태그를 갖거나 속성을 가지면 복합 구조체로 판정
            is_nested = (len(sample) > 0 or len(sample.attrib) > 0)
            safe_name = tag.replace("-", "_")

            if is_nested:
                child_struct_type = parse_element(sample)
                field_cpp_type = f"std::vector<{child_struct_type}>" if is_array else child_struct_type
                target_type = child_struct_type
            else:
                base_type = infer_cpp_type(sample.text or "")
                field_cpp_type = f"std::vector<{base_type}>" if is_array else base_type
                target_type = base_type

            fields.append({
                "name": safe_name,
                "orig_xml_name": tag,
                "cpp_type": field_cpp_type,
                "target_type": target_type,
                "is_nested": is_nested,
                "is_array": is_array,
                "kind": "element"
            })

        if struct_name not in generated_structs:
            lines = [f"struct {struct_name} {{"]

            # 멤버 변수 정의
            for f in fields:
                lines.append(f"    {f['cpp_type']} {f['name']};")
            lines.append("")

            # -------------------------------------------------------------
            # 역직렬화 (deserialize)
            # -------------------------------------------------------------
            lines.append("    void deserialize(const pugi::xml_node& node) {")
            lines.append(f'        if (node.empty()) throw std::runtime_error("Node {struct_name} is empty or not found.");')
            lines.append("")

            for f in fields:
                orig = f["orig_xml_name"]
                name = f["name"]

                if f["kind"] == "attribute":
                    lines.append(f'        auto attr_{name} = node.attribute("{orig}");')
                    lines.append(f'        if (attr_{name}.empty()) throw std::runtime_error("Required attribute \'{orig}\' is missing in {struct_name}.");')
                    if f["cpp_type"] == "int":
                        lines.append(f"        try {{ {name} = std::stoi(attr_{name}.value()); }}")
                        lines.append(f'        catch (...) {{ throw std::runtime_error("Failed to parse int attribute \'{orig}\' in {struct_name}."); }}')
                    elif f["cpp_type"] == "double":
                        lines.append(f"        try {{ {name} = std::stod(attr_{name}.value()); }}")
                        lines.append(f'        catch (...) {{ throw std::runtime_error("Failed to parse double attribute \'{orig}\' in {struct_name}."); }}')
                    elif f["cpp_type"] == "bool":
                        lines.append(f"        {name} = attr_{name}.as_bool();")
                    else:
                        lines.append(f"        {name} = attr_{name}.as_string();")

                elif f["kind"] == "text_body":
                    if f["cpp_type"] == "int":
                        lines.append(f"        try {{ {name} = std::stoi(node.text().get()); }}")
                        lines.append(f'        catch (...) {{ throw std::runtime_error("Failed to parse inner int text in {struct_name}."); }}')
                    elif f["cpp_type"] == "double":
                        lines.append(f"        try {{ {name} = std::stod(node.text().get()); }}")
                        lines.append(f'        catch (...) {{ throw std::runtime_error("Failed to parse inner double text in {struct_name}."); }}')
                    elif f["cpp_type"] == "bool":
                        lines.append(f"        {name} = node.text().as_bool();")
                    else:
                        lines.append(f"        {name} = node.text().as_string();")

                elif f["kind"] == "element":
                    if f["is_array"]:
                        lines.append(f"        {name}.clear();")
                        lines.append(f'        for (auto child : node.children("{orig}")) {{')
                        if f["is_nested"]:
                            lines.append(f"            {f['target_type']} item;")
                            lines.append("            item.deserialize(child);")
                            lines.append(f"            {name}.push_back(item);")
                        else:
                            if f["target_type"] == "int":
                                lines.append(f"            try {{ {name}.push_back(std::stoi(child.text().get())); }}")
                                lines.append(f'            catch (...) {{ throw std::runtime_error("Failed to parse int element \'{orig}\'."); }}')
                            elif f["target_type"] == "double":
                                lines.append(f"            try {{ {name}.push_back(std::stod(child.text().get())); }}")
                                lines.append(f'            catch (...) {{ throw std::runtime_error("Failed to parse double element \'{orig}\'."); }}')
                            elif f["target_type"] == "bool":
                                lines.append(f"            {name}.push_back(child.text().as_bool());")
                            else:
                                lines.append(f"            {name}.push_back(child.text().as_string());")
                        lines.append("        }")
                    else:
                        lines.append(f'        auto child_{name} = node.child("{orig}");')
                        lines.append(f'        if (child_{name}.empty()) throw std::runtime_error("Required element \'{orig}\' is missing in {struct_name}.");')
                        if f["is_nested"]:
                            lines.append(f"        {name}.deserialize(child_{name});")
                        else:
                            if f["cpp_type"] == "int":
                                lines.append(f"        try {{ {name} = std::stoi(child_{name}.text().get()); }}")
                                lines.append(f'        catch (...) {{ throw std::runtime_error("Failed to parse int element \'{orig}\' in {struct_name}."); }}')
                            elif f["cpp_type"] == "double":
                                lines.append(f"        try {{ {name} = std::stod(child_{name}.text().get()); }}")
                                lines.append(f'        catch (...) {{ throw std::runtime_error("Failed to parse double element \'{orig}\' in {struct_name}."); }}')
                            elif f["cpp_type"] == "bool":
                                lines.append(f"        {name} = child_{name}.text().as_bool();")
                            else:
                                lines.append(f"        {name} = child_{name}.text().as_string();")
                lines.append("")

            lines.append("    }")
            lines.append("")

            # -------------------------------------------------------------
            # 직렬화 (serialize)
            # -------------------------------------------------------------
            lines.append("    void serialize(pugi::xml_node& node) const {")
            lines.append(f'        if (node.empty()) throw std::runtime_error("Cannot serialize {struct_name} into an empty XML node.");')
            lines.append("")

            for f in fields:
                orig = f["orig_xml_name"]
                name = f["name"]

                if f["kind"] == "attribute":
                    if f["cpp_type"] == "bool":
                        lines.append(f'        node.append_attribute("{orig}").set_value({name} ? "true" : "false");')
                    else:
                        lines.append(f'        node.append_attribute("{orig}").set_value({name});')

                elif f["kind"] == "text_body":
                    if f["cpp_type"] == "bool":
                        lines.append(f'        node.text().set({name} ? "true" : "false");')
                    else:
                        lines.append(f'        node.text().set({name});')

                elif f["kind"] == "element":
                    if f["is_array"]:
                        lines.append(f"        for (const auto& item : {name}) {{")
                        lines.append(f'            auto child = node.append_child("{orig}");')
                        if f["is_nested"]:
                            lines.append("            item.serialize(child);")
                        else:
                            if f["target_type"] == "bool":
                                lines.append('            child.text().set(item ? "true" : "false");')
                            else:
                                lines.append("            child.text().set(item);")
                        lines.append("        }")
                    else:
                        lines.append(f'        auto child_{name} = node.append_child("{orig}");')
                        if f["is_nested"]:
                            lines.append(f"        {name}.serialize(child_{name});")
                        else:
                            if f["cpp_type"] == "bool":
                                lines.append(f'        child_{name}.text().set({name} ? "true" : "false");')
                            else:
                                lines.append(f'        child_{name}.text().set({name});')

            lines.append("    }")
            lines.append("};\n")

            struct_definitions.append("\n".join(lines))
            generated_structs.add(struct_name)

        return struct_name

    parse_element(root)

    header = """#pragma once

#include <iostream>
#include <string>
#include <vector>
#include <stdexcept>
#include <filesystem>
#include "mino/external/third-party/pugixml/pugixml.hpp"

"""
    return header + "\n".join(struct_definitions)


SAMPLE_XML = """<?xml version="1.0" encoding="UTF-8"?>
<!-- Catalog Sample Data -->
<catalog>
    <book id="bk101" category="fiction">
        <title>XML Guide</title>
        <price unit="KRW">25000</price>
        <description>
            <![CDATA[Special characters (<, >, &) preserved]]>
        </description>
    </book>
</catalog>
"""

def show_example():
    """인자가 없을 때 영문 도움말 및 예제 결과를 출력합니다."""
    print("=" * 70)
    print("[INFO] No arguments provided. Showing usage and sample output.")
    print("=" * 70)
    print("\nUsage:")
    print("  python xml2xpp.py <xml_file_path> [-o <output_hpp_path>]")
    print("  python xml2xpp.py -h, --help\n")
    print("Examples:")
    print("  python xml2xpp.py catalog.xml")
    print("  python xml2xpp.py catalog.xml -o ./include/Catalog.hpp\n")
    print("-" * 70)
    print("[Sample XML Input]")
    print("-" * 70)
    print(SAMPLE_XML.strip())
    print("\n" + "-" * 70)
    print("[Generated C++ Code Preview]")
    print("-" * 70)
    print(generate_cpp_structs(SAMPLE_XML))
    print("=" * 70)

def main():
    if len(sys.argv) == 1:
        show_example()
        sys.exit(0)

    parser = argparse.ArgumentParser(
        prog="xml2xpp.py",
        description="Converts XML structures into C++ structs with serialization and deserialization support."
    )
    parser.add_argument("xml_path", help="Path to the input XML file")
    parser.add_argument(
        "-o", "--output",
        dest="output_path",
        default=None,
        help="Path for the output C++ header file (default: <input_filename>.hpp)"
    )

    args = parser.parse_args()

    if not os.path.isfile(args.xml_path):
        print(f"[ERROR] Input file not found: {args.xml_path}", file=sys.stderr)
        sys.exit(1)

    output_file = args.output_path
    if not output_file:
        base_name = os.path.splitext(args.xml_path)[0]
        output_file = f"{base_name}.hpp"

    try:
        with open(args.xml_path, "r", encoding="utf-8") as f:
            xml_content = f.read()

        cpp_code = generate_cpp_structs(xml_content)

        out_dir = os.path.dirname(output_file)
        if out_dir:
            os.makedirs(out_dir, exist_ok=True)

        with open(output_file, "w", encoding="utf-8") as f:
            f.write(cpp_code)

        print(f"[SUCCESS] C++ header exported to: {output_file}")

    except ET.ParseError as e:
        print(f"[ERROR] XML Parse Error: {e}", file=sys.stderr)
        sys.exit(1)
    except Exception as e:
        print(f"[ERROR] Generation Failed: {e}", file=sys.stderr)
        sys.exit(1)

if __name__ == "__main__":
    main()
