#!/bin/bash

# Test script for reveal command
echo "=== Testing reveal command ==="

# Create test directories and files for testing
mkdir -p test_reveal_dir
cd test_reveal_dir

# Create some test files and directories
touch file1.txt file2.c .hidden_file
mkdir dir1 dir2 .hidden_dir
touch dir1/nested_file.txt

echo "Test directory structure created:"
ls -la

echo ""
echo "=== Running reveal command tests ==="

# Test 1: Basic reveal (current directory)
echo "Test 1: reveal (current directory)"
echo "reveal" | ../shell.out

echo ""

# Test 2: reveal with -a flag (show hidden files)
echo "Test 2: reveal -a (show hidden files)"
echo "reveal -a" | ../shell.out

echo ""

# Test 3: reveal with -l flag (line format)
echo "Test 3: reveal -l (line format)"
echo "reveal -l" | ../shell.out

echo ""

# Test 4: reveal with both flags
echo "Test 4: reveal -al (both flags)"
echo "reveal -al" | ../shell.out

echo ""

# Test 5: reveal specific directory
echo "Test 5: reveal dir1 (specific directory)"
echo "reveal dir1" | ../shell.out

echo ""

# Test 6: reveal parent directory
echo "Test 6: reveal .. (parent directory)"
echo "reveal .." | ../shell.out

echo ""

# Test 7: reveal home directory
echo "Test 7: reveal ~ (home directory)"
echo "reveal ~" | ../shell.out

echo ""

# Test 8: reveal non-existent directory
echo "Test 8: reveal nonexistent (should show error)"
echo "reveal nonexistent" | ../shell.out

echo ""

# Test 9: Multiple flags in different format
echo "Test 9: reveal -a -l (separate flags)"
echo "reveal -a -l" | ../shell.out

echo ""

# Clean up
cd ..
rm -rf test_reveal_dir

echo "=== reveal command tests completed ==="
