#!/bin/bash

echo "Running gcov on commands.c"
gcov -o build/CMakeFiles/test_commands.dir/src/commands.c.gcda ../src/commands.c

echo "Cleaning up unnecessary files..."
rm -f *.h.gcov

echo "Capturing coverage with lcov..."
lcov --directory build/CMakeFiles --capture --output-file coverage.info

echo "Generating HTML report with genhtml..."
genhtml coverage.info --output-directory coverage_report

echo "Coverage report generated in coverage_report/"

