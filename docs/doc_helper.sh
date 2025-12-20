#!/bin/bash

BOLD='\033[1m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

echo -e "${BOLD}AK24 Documentation Helper${NC}\n"

if [ $# -eq 0 ]; then
    echo "Usage: $0 <header_file>"
    echo ""
    echo "This script helps you document a header file by:"
    echo "  1. Analyzing the file structure"
    echo "  2. Showing what needs to be documented"
    echo "  3. Providing relevant examples"
    echo ""
    echo "Example:"
    echo "  $0 kernel/atoms/include/atom.h"
    exit 1
fi

HEADER_FILE=$1

if [ ! -f "$HEADER_FILE" ]; then
    echo -e "${YELLOW}Error: File not found: $HEADER_FILE${NC}"
    exit 1
fi

echo -e "${BLUE}Analyzing: ${HEADER_FILE}${NC}\n"

has_file_doc=$(grep -c "@file" "$HEADER_FILE" || echo "0")

if [ "$has_file_doc" -gt 0 ]; then
    echo -e "${GREEN}✓ File has @file documentation${NC}"
else
    echo -e "${YELLOW}✗ Missing @file documentation${NC}"
    echo ""
    echo "Add this at the top of the file:"
    echo ""
    echo "/**"
    echo " * @file $(basename "$HEADER_FILE")"
    echo " * @brief Brief description of this module"
    echo " *"
    echo " * Detailed description of what this module does."
    echo " *"
    echo " * Key features:"
    echo " * - Feature 1"
    echo " * - Feature 2"
    echo " */"
    echo ""
fi

echo -e "\n${BOLD}Structures/Types Found:${NC}"
grep -E "^typedef (struct|enum)" "$HEADER_FILE" | head -10 || echo "None found"

echo -e "\n${BOLD}Functions Found:${NC}"
grep -E "^[a-zA-Z_][a-zA-Z0-9_]*\s+\*?[a-zA-Z_][a-zA-Z0-9_]*\s*\(" "$HEADER_FILE" | head -10 || echo "None found"

echo -e "\n${BOLD}Documentation Resources:${NC}"
echo "  • Template: docs/DOXYGEN_TEMPLATE.h"
echo "  • Guide: docs/DOXYGEN_GUIDE.md"
echo "  • Quick Ref: docs/DOXYGEN_QUICK_REF.md"
echo "  • Example: kernel/arbuff/include/arbuff.h"

echo -e "\n${BOLD}Next Steps:${NC}"
echo "  1. Add @file documentation at the top"
echo "  2. Document each typedef struct with @brief"
echo "  3. Add inline /**< */ comments for struct fields"
echo "  4. Document each function with @brief, @param, @return"
echo "  5. Add thread safety markers (@threadsafe or @notthreadsafe)"
echo "  6. Include usage examples for complex functions"
echo "  7. Run: make docs"
echo "  8. Run: ./docs/check_docs.sh"

echo ""
