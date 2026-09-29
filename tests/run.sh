#!/bin/bash
# Runs every tests/*.in through ./fs and compares the output with tests/*.out
# To accept a new expected output after a deliberate change:
#   ./fs < tests/NAME.in > tests/NAME.out

cd "$(dirname "$0")/.." || exit 1

pass=0
fail=0
for input in tests/*.in; do
    name=$(basename "$input" .in)
    expected="tests/$name.out"
    if ./fs < "$input" | diff -u "$expected" - > /dev/null; then
        echo "PASS $name"
        pass=$((pass + 1))
    else
        echo "FAIL $name"
        ./fs < "$input" | diff -u "$expected" - | head -30
        fail=$((fail + 1))
    fi
done

echo "$pass passed, $fail failed"
[ "$fail" -eq 0 ]
