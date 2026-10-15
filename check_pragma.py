import argparse
from pathlib import Path


def check_pragma_once(target_dir):
  root_path = Path(target_dir)

  if not root_path.exists() or not root_path.is_dir():
    print(f"[Error] Not a valid directory path: {target_dir}")
    return

  hpp_files = list(root_path.rglob("*.hpp"))
  if not hpp_files:
    print(f"No .hpp files found in the specified path: {target_dir}")
    return

  passed_files = []
  failed_files = []

  print(f"Inspecting a total of {len(hpp_files)} .hpp files...\n")

  for file_path in hpp_files:
    has_pragma = False
    try:
      with open(file_path, "r", encoding="utf-8", errors="ignore") as f:
        for line in f:
          stripped = line.strip()
          # Skip empty lines
          if not stripped:
            continue
          # Check if the first valid line is #pragma once
          if stripped == "#pragma once":
            has_pragma = True
          break
    except Exception as e:
      print(f"[Warning] Error reading file ({file_path}): {e}")
      continue

    if has_pragma:
      passed_files.append(file_path)
    else:
      failed_files.append(file_path)

  # Print results
  print("=" * 50)
  print(f"Inspection Complete (Total files: {len(hpp_files)})")
  print("=" * 50)

  print(f"\n[Passed] Contains #pragma once ({len(passed_files)}):")
  for p in passed_files:
    print(f"  - {p}")

  print(f"\n[Failed] Missing #pragma once ({len(failed_files)}):")
  for f in failed_files:
    print(f"  - {f}")


if __name__ == "__main__":
  parser = argparse.ArgumentParser(
      description="Check whether C++ .hpp files start with #pragma once."
  )
  parser.add_argument(
      "path",
      nargs="?",
      default=".",
      help="Top-level directory path to inspect (default: current directory)",
  )

  args = parser.parse_args()
  check_pragma_once(args.path)
  
  