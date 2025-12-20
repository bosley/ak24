#!/bin/bash

set -e

KERNEL_DIR="kernel"
BOLD='\033[1m'
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m'

echo -e "${BOLD}AK24 Documentation Coverage Check${NC}\n"

total_headers=0
documented_headers=0
undocumented_headers=()

check_file() {
    local file=$1

    if grep -q "@file" "$file"; then
        return 0
    else
        return 1
    fi
}

find "$KERNEL_DIR" -name "*.h" -not -path "*/test/*" | while read -r header; do
    total_headers=$((total_headers + 1))

    if check_file "$header"; then
        documented_headers=$((documented_headers + 1))
        echo -e "${GREEN}✓${NC} $header"
    else
        echo -e "${RED}✗${NC} $header ${YELLOW}(missing @file documentation)${NC}"
    fi
done

echo ""
echo -e "${BOLD}Summary:${NC}"

header_count=$(find "$KERNEL_DIR" -name "*.h" -not -path "*/test/*" | wc -l | tr -d ' ')
documented_count=$(find "$KERNEL_DIR" -name "*.h" -not -path "*/test/*" -exec grep -l "@file" {} \; 2>/dev/null | wc -l | tr -d ' ')

if [ "$header_count" -eq 0 ]; then
    echo "No header files found"
    exit 0
fi

coverage=$((documented_count * 100 / header_count))

echo "Total headers: $header_count"
echo "Documented: $documented_count"
echo "Coverage: ${coverage}%"

if [ "$coverage" -eq 100 ]; then
    echo -e "${GREEN}${BOLD}Perfect! All headers are documented.${NC}"
elif [ "$coverage" -ge 80 ]; then
    echo -e "${GREEN}Good coverage, but some headers need documentation.${NC}"
elif [ "$coverage" -ge 50 ]; then
    echo -e "${YELLOW}Moderate coverage. Consider documenting more headers.${NC}"
else
    echo -e "${RED}Low coverage. Many headers need documentation.${NC}"
fi

echo ""
echo "Run 'make docs' to generate HTML documentation."
echo "See docs/DOXYGEN_GUIDE.md for documentation guidelines."
