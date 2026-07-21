#!/usr/bin/env python3
# -*- coding: utf-8 -*-

"""
Generate NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE macros from C++ header files

  // hello.hpp
  struct Person {
      std::string name;
      int age;
      double height;
      std::vector<std::string> tags;
  };
  struct Asset {
      int id;
      std::string serial_number;
  };

  $ python generate_nlohmann_macro.py hello.hpp

  // --- Generated Macros for hello.hpp ---
  NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(Person, name, age, height, tags)
  NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(Asset, id, serial_number)
"""

import re
import sys
import argparse

def extract_macros(header_text):
    # 1. Extract struct/class name and its internal content
    struct_pattern = re.compile(r'(?:struct|class)\s+(\w+)\s*\{([\s\S]*?)\};')
    
    # 2. Extract member variable names
    # - Captures the variable name before the semicolon
    # - Handles optional initializers like {0} or = 0
    member_pattern = re.compile(r'(?:^|;)\s*[\w:<>, ]+\s+(\w+)\s*(?:\{.*\}|=.*)?\s*;')

    results = []
    structs = struct_pattern.findall(header_text)
    
    for struct_name, content in structs:
        # Preprocessing: Remove single-line (//) and multi-line (/* */) comments
        clean_content = re.sub(r'//.*|/\*[\s\S]*?\*/', '', content)
        
        members = []
        for line in clean_content.split('\n'):
            line = line.strip()
            if not line: continue
            
            # Add a semicolon at the start to ensure the regex matches the first line correctly
            match = member_pattern.search(';' + line) 
            if match:
                members.append(match.group(1))
        
        if members:
            member_list = ", ".join(members)
            results.append(f"NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE({struct_name}, {member_list})")
            
    return results

def main():
    parser = argparse.ArgumentParser(description="Automatically generate nlohmann/json macros from C++ headers.")
    parser.add_argument("file", help="Path to the .hpp or .h file to analyze")
    
    if len(sys.argv) < 2:
        parser.print_help()
        sys.exit(1)

    args = parser.parse_args()

    try:
        with open(args.file, 'r', encoding='utf-8') as f:
            content = f.read()
            
        macros = extract_macros(content)
        
        if not macros:
            print(f"// No valid structs or members found in '{args.file}'.")
        else:
            print(f"// --- Generated Macros for {args.file} ---")
            for m in macros:
                print(m)
                
    except FileNotFoundError:
        print(f"Error: File '{args.file}' not found.")
    except Exception as e:
        print(f"An unexpected error occurred: {e}")

if __name__ == "__main__":
    main()
