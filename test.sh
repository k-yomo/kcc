#!/bin/bash
assert() {
  expected="$1"
  input="$2"

  ./main "$input" > tmp.s
  cc -o tmp tmp.s
  ./tmp
  actual="$?"

  if [ "$actual" = "$expected" ]; then
    echo "$input => $actual"
  else
    echo "$input => $expected expected, but got $actual"
    exit 1
  fi
}

assert 0 0
assert 42 42
assert 32 "1+10-9+30"
assert 32 " 1 + 10- 9 +30"
assert 47 "5+6*7"
assert 30 "5*( 12- 6)"
assert 4 "(3+5)/2"

echo OK