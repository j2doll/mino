import argparse
import json
import os
import sys
from typing import Any, Dict, List, Set

# 터미널 ANSI 색상 코드 정의
class Colors:
    RESET = "\033[0m"
    BOLD = "\033[1m"
    CYAN = "\033[36m"
    GREEN = "\033[32m"
    YELLOW = "\033[33m"
    BLUE = "\033[34m"
    MAGENTA = "\033[35m"
    RED = "\033[31m"

def to_pascal_case(snake_str: str) -> str:
    """snake_case 또는 일반 문자열을 PascalCase로 변환"""
    clean_str = ''.join(c if c.isalnum() or c == '_' else '_' for c in snake_str)
    components = clean_str.split('_')
    return ''.join(x.capitalize() for x in components if x)

class JsonToCppStruct:
    def __init__(self, use_macro: bool = True):
        """
        :param use_macro: True면 NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE 매크로 사용,
                          False면 to_json / from_json 함수를 직접 작성
        """
        self.use_macro = use_macro
        self.generated_structs: List[str] = []
        self.struct_names: Set[str] = set()

    def _get_unique_struct_name(self, base_name: str) -> str:
        name = to_pascal_case(base_name) or "GeneratedStruct"
        count = 1
        unique_name = name
        while unique_name in self.struct_names:
            unique_name = f"{name}_{count}"
            count += 1
        self.struct_names.add(unique_name)
        return unique_name

    def _infer_type(self, key: str, value: Any) -> str:
        if value is None:
            return "std::nullptr_t"
        elif isinstance(value, bool):
            return "bool"
        elif isinstance(value, int):
            return "int"
        elif isinstance(value, float):
            return "double"
        elif isinstance(value, str):
            return "std::string"
        elif isinstance(value, dict):
            struct_name = self._get_unique_struct_name(key)
            self._generate_struct(struct_name, value)
            return struct_name
        elif isinstance(value, list):
            if not value:
                return "std::vector<std::string>"
            
            first_elem = value[0]
            if isinstance(first_elem, dict):
                item_struct_name = self._get_unique_struct_name(f"{key}_item")
                self._generate_struct(item_struct_name, first_elem)
                return f"std::vector<{item_struct_name}>"
            else:
                elem_type = self._infer_type(key, first_elem)
                return f"std::vector<{elem_type}>"
        else:
            return "std::string"

    def _generate_serializer_macro(self, struct_name: str, fields: List[str]) -> str:
        if not fields:
            return ""
        field_args = ", ".join(fields)
        return f"NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE({struct_name}, {field_args})"

    def _generate_serializer_functions(self, struct_name: str, fields: List[str]) -> str:
        to_json_lines = [f'    j["{f}"] = obj.{f};' for f in fields]
        to_json_body = "\n".join(to_json_lines)
        to_json_func = (
            f"inline void to_json(nlohmann::json& j, const {struct_name}& obj) {{\n"
            f"{to_json_body}\n"
            f"}}"
        )

        from_json_lines = [f'    j.at("{f}").get_to(obj.{f});' for f in fields]
        from_json_body = "\n".join(from_json_lines)
        from_json_func = (
            f"inline void from_json(const nlohmann::json& j, {struct_name}& obj) {{\n"
            f"{from_json_body}\n"
            f"}}"
        )

        return f"{to_json_func}\n\n{from_json_func}"

    def _generate_struct(self, struct_name: str, data: Dict[str, Any]):
        field_declarations = []
        field_names = []

        for key, value in data.items():
            field_type = self._infer_type(key, value)
            field_declarations.append(f"    {field_type} {key};")
            field_names.append(key)

        struct_code = f"struct {struct_name} {{\n" + "\n".join(field_declarations) + "\n};"
        
        if self.use_macro:
            serializer_code = self._generate_serializer_macro(struct_name, field_names)
        else:
            serializer_code = self._generate_serializer_functions(struct_name, field_names)

        full_code = f"{struct_code}\n\n{serializer_code}" if serializer_code else struct_code
        self.generated_structs.append(full_code)

    def convert(self, json_str: str, root_name: str = "Root") -> str:
        """JSON 문자열을 C++ 구조체 코드로 변환"""
        data = json.loads(json_str)
        self.generated_structs.clear()
        self.struct_names.clear()

        if isinstance(data, list):
            if not data or not isinstance(data[0], dict):
                return "// Root is a list of primitive types."
            root_type = self._get_unique_struct_name(f"{root_name}Item")
            self._generate_struct(root_type, data[0])
        elif isinstance(data, dict):
            root_type = self._get_unique_struct_name(root_name)
            self._generate_struct(root_type, data)
        else:
            return "// Invalid JSON object or array."

        headers = (
            "#include <string>\n"
            "#include <vector>\n"
            "#include <cstddef>\n"
            "#include <nlohmann/json.hpp>\n\n"
        )
        return headers + "\n\n".join(self.generated_structs)

    def convert_file(self, file_path: str, root_name: str = None, encoding: str = "utf-8") -> str:
        """JSON 파일을 읽어 C++ 구조체 코드로 변환"""
        if not os.path.exists(file_path):
            raise FileNotFoundError(f"File not found: {file_path}")

        if root_name is None:
            base_filename = os.path.splitext(os.path.basename(file_path))[0]
            root_name = to_pascal_case(base_filename)

        with open(file_path, "r", encoding=encoding) as f:
            json_str = f.read()

        return self.convert(json_str, root_name=root_name)

    def save_to_file(self, file_path: str, output_path: str, root_name: str = None, encoding: str = "utf-8"):
        """JSON 파일을 C++ 헤더 파일(.hpp)로 직접 저장"""
        code = self.convert_file(file_path, root_name=root_name, encoding=encoding)
        with open(output_path, "w", encoding=encoding) as f:
            f.write(code)


def run_examples():
    """인자 없이 실행되었을 때 동작하는 예시 함수"""
    sample_json = """{
  "user_id": 1024,
  "username": "coder123",
  "is_active": true,
  "profile": {
    "bio": "Hello World",
    "age": 28
  },
  "tags": [
    "c++",
    "json"
  ]
}"""

    print(f"{Colors.BOLD}{Colors.MAGENTA}// ============== Example json to cpp ===================={Colors.RESET}\n")

    # 1. 원본 JSON 출력 (Cyan/하늘색)
    print('root_name = UserResponse')
    print(f"{Colors.BOLD}{Colors.CYAN}// ==================== Original JSON ===================={Colors.RESET}")
    print(f"{Colors.CYAN}{sample_json}{Colors.RESET}")
    print("\n" + "=" * 58 + "\n")

    # 2. 변환된 C++ 구조체 출력 (Green/초록색)
    converter_macro = JsonToCppStruct(use_macro=True)
    cpp_output = converter_macro.convert(sample_json, root_name="UserResponse")
    
    print(f"{Colors.BOLD}{Colors.GREEN}// ================= Generated C++ Struct ================{Colors.RESET}")
    print(f"{Colors.GREEN}{cpp_output}{Colors.RESET}")


def main():
    parser = argparse.ArgumentParser(description="Convert JSON file to C++ structs with nlohmann/json support.")
    
    parser.add_argument("input", nargs="?", help="Path to the input .json file (optional, runs example if omitted)")
    parser.add_argument("-o", "--output", help="Output file path (e.g., Output.hpp). Prints to console if omitted.")
    parser.add_argument("-r", "--root-name", help="Name of the root struct (defaults to input file name)")
    parser.add_argument("--no-macro", action="store_true", help="Generate explicit to_json/from_json functions instead of macros")

    args = parser.parse_args()

    # 인자가 없으면 예시 실행 후 도움말을 색상과 함께 출력
    if not args.input:
        run_examples()
        print("\n" + "=" * 58 + "\n")
        print(f"{Colors.BOLD}{Colors.YELLOW}// ================= Argument Usage ====================={Colors.RESET}")
        print(Colors.YELLOW, end="")
        parser.print_help()
        print(Colors.RESET, end="")
        return

    use_macro = not args.no_macro
    converter = JsonToCppStruct(use_macro=use_macro)

    try:
        if args.output:
            converter.save_to_file(args.input, args.output, root_name=args.root_name)
            print(f"{Colors.GREEN}Generated C++ structs saved to: {args.output}{Colors.RESET}")
        else:
            code = converter.convert_file(args.input, root_name=args.root_name)
            print(code)
    except Exception as e:
        print(f"{Colors.RED}Error: {e}{Colors.RESET}", file=sys.stderr)
        sys.exit(1)


if __name__ == "__main__":
    main()

