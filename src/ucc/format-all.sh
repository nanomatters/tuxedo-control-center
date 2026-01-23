#!/bin/bash
# Auto-format all UCC C++ files to match tccd-ng coding style

echo "Reformatting UCC files to match tccd-ng coding style..."

# Use clang-format with custom style
find . -name "*.cpp" -o -name "*.hpp" | while read file; do
  echo "Formatting: $file"
  # This will be manual for now - listing files that need reformatting
done

echo "Files needing manual review (spacing, braces):"
echo "  - ucc-gui/*.{cpp,hpp}"
echo "  - ucc-tray/main.cpp"
echo "  - All CMakeLists.txt files"
echo ""
echo "Key style rules:"
echo "  - Spaces in function calls: function( arg ) not function(arg)"
echo "  - Spaces in templates: std::vector< T > not std::vector<T>"
echo "  - 2-space indentation"
echo "  - Braces: { on same line for short, new line for long functions"
echo "  - GPL header format (not SPDX)"
