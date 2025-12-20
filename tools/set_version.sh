#!/bin/bash

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
VERSION_FILE="$PROJECT_ROOT/VERSION"

BOLD='\033[1m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

if [ ! -f "$VERSION_FILE" ]; then
    echo -e "${YELLOW}Error: VERSION file not found at $VERSION_FILE${NC}"
    exit 1
fi

VERSION=$(cat "$VERSION_FILE" | tr -d '[:space:]')

if [ -z "$VERSION" ]; then
    echo -e "${YELLOW}Error: VERSION file is empty${NC}"
    exit 1
fi

CMAKE_VERSION=$(echo "$VERSION" | sed 's/-.*$//')

echo -e "${BOLD}${BLUE}AK24 Version Update Tool${NC}"
echo -e "Setting version to: ${GREEN}${VERSION}${NC}"
echo -e "CMake version: ${GREEN}${CMAKE_VERSION}${NC}\n"

update_count=0

update_file() {
    local file=$1
    local pattern=$2
    local replacement=$3

    if [ ! -f "$file" ]; then
        echo -e "${YELLOW}Warning: File not found: $file${NC}"
        return
    fi

    if grep -q "$pattern" "$file"; then
        sed -i.bak "s|$pattern|$replacement|g" "$file"
        rm -f "${file}.bak"
        echo -e "${GREEN}✓${NC} Updated: $(basename $(dirname $(dirname "$file")))/$(basename "$file")"
        update_count=$((update_count + 1))
    fi
}

echo -e "${BOLD}Updating kernel module headers...${NC}"

update_file "$PROJECT_ROOT/kernel/arbuff/include/arbuff.h" \
    '#define AK24_ARBUFF_VERSION ".*"' \
    "#define AK24_ARBUFF_VERSION \"$VERSION\""

update_file "$PROJECT_ROOT/kernel/context/include/context.h" \
    '#define AK24_CONTEXT_VERSION ".*"' \
    "#define AK24_CONTEXT_VERSION \"$VERSION\""

update_file "$PROJECT_ROOT/kernel/forms/include/forms.h" \
    '#define AK24_FORMS_VERSION ".*"' \
    "#define AK24_FORMS_VERSION \"$VERSION\""

update_file "$PROJECT_ROOT/kernel/lambda/include/lambda.h" \
    '#define AK24_LAMBDA_VERSION ".*"' \
    "#define AK24_LAMBDA_VERSION \"$VERSION\""

update_file "$PROJECT_ROOT/kernel/list/include/list.h" \
    '#define AK24_LIST_VERSION ".*"' \
    "#define AK24_LIST_VERSION \"$VERSION\""

update_file "$PROJECT_ROOT/kernel/log/include/log.h" \
    '#define AK24_LOG_VERSION ".*"' \
    "#define AK24_LOG_VERSION \"$VERSION\""

update_file "$PROJECT_ROOT/kernel/map/include/map.h" \
    '#define AK24_MAP_VERSION ".*"' \
    "#define AK24_MAP_VERSION \"$VERSION\""

echo -e "\n${BOLD}Updating SVG logos...${NC}"
for svg in "$PROJECT_ROOT"/assets/*.svg; do
    if [ -f "$svg" ]; then
        update_file "$svg" \
            '>v[0-9][^<]*</text>' \
            ">v$VERSION</text>"
    fi
done

echo -e "\n${BOLD}Updating CMakeLists.txt...${NC}"
update_file "$PROJECT_ROOT/CMakeLists.txt" \
    'project(ak24 VERSION [0-9][^ ]*' \
    "project(ak24 VERSION $CMAKE_VERSION"

echo -e "\n${BOLD}Updating Doxyfile...${NC}"
update_file "$PROJECT_ROOT/Doxyfile" \
    'PROJECT_NUMBER         = ".*"' \
    "PROJECT_NUMBER         = \"$VERSION\""

echo -e "\n${BOLD}Updating documentation template...${NC}"
if [ -f "$PROJECT_ROOT/docs/DOXYGEN_TEMPLATE.h" ]; then
    update_file "$PROJECT_ROOT/docs/DOXYGEN_TEMPLATE.h" \
        '#define AK24_MODULE_VERSION ".*"' \
        "#define AK24_MODULE_VERSION \"$VERSION\""
fi

echo -e "\n${GREEN}${BOLD}✓ Version update complete!${NC}"
echo -e "Updated ${update_count} files to version ${GREEN}${VERSION}${NC}"
echo -e "\n${BLUE}Note:${NC} CMake VERSION uses ${CMAKE_VERSION} (pre-release suffix removed)"
echo -e "\nRun ${BLUE}make docs${NC} to regenerate documentation with new version."
