#!/bin/sh
# The kit gate. `make check` runs it from the kit directory after the build.
set -eu
fail=0
count=0
for prog in examples/*.lang; do
  out=$(build/langc check "$prog" 2>&1) || true
  count=$((count + 1))
  if [ "$out" != "ok" ]; then
    echo "FAIL $prog: $out"
    fail=$((fail + 1))
  fi
done
echo "examples: $count checked, $fail failed"

tmp=${TMPDIR:-/tmp}/langc-gate.$$
mkdir -p "$tmp"
awk 'BEGIN { printf "def deep : Nat := " } { } END {
  for (i = 0; i < 1100; i++) printf "("; printf "1"; for (i = 0; i < 1100; i++) printf ")"; print "" }' </dev/null >"$tmp/deep.lang"

# refusals DIR LABEL: each line of DIR/expect.txt is `FILE CODE`. `langc check`
# must exit 1 and print `langc: CODE: ...` on stderr.
refusals() {
  checked=0
  while read -r file code; do
    path="$1/$file"
    [ "$file" = "deep.lang" ] && path="$tmp/deep.lang"
    status=0
    build/langc check "$path" 2>"$tmp/err" >/dev/null || status=$?
    first=$(head -n 1 "$tmp/err")
    checked=$((checked + 1))
    case "$status:$first" in
      "1:langc: $code: "*) ;;
      *) echo "FAIL $file: want exit 1 and $code, got exit $status: $first"; fail=$((fail + 1)) ;;
    esac
  done <"$1/expect.txt"
  echo "$2 refusals: $checked checked"
}
refusals test/parse parse
refusals test/check check

# A refl refusal shows both normal forms.
build/langc check test/check/bad-refl.lang 2>"$tmp/err" >/dev/null || true
if ! rg -q -F 'the left side normalizes to 1201, the right side normalizes to 1200' "$tmp/err"; then
  echo "FAIL bad-refl.lang: the message does not show both normal forms"
  fail=$((fail + 1))
fi

# Each line of test/eval/expect.txt is `FILE NAME [ARGS] => OUTPUT`. `langc eval`
# must exit 0 and print OUTPUT. A trap also prints `langc: EVAL_OVERFLOW: NAME: ...`
# on stderr. `=> trap CODE` expects `trap` and `langc: CODE: NAME: ...` instead.
evals=0
while IFS= read -r line; do
  want=${line#* => }
  set -- ${line%% => *}
  file=$1
  name=$2
  shift 2
  status=0
  got=$(build/langc eval "examples/$file.lang" "$name" "$@" 2>"$tmp/err") || status=$?
  first=$(head -n 1 "$tmp/err")
  want_err=""
  case "$want" in
    trap) want_err="langc: EVAL_OVERFLOW: $name: " ;;
    "trap "*) want_err="langc: ${want#trap }: $name: "; want=trap ;;
  esac
  evals=$((evals + 1))
  case "$status|$got|$first" in
    "0|$want|$want_err"*) ;;
    *) echo "FAIL eval $file $name: want $want, got exit $status: $got $first"; fail=$((fail + 1)) ;;
  esac
done <test/eval/expect.txt
echo "evals: $evals checked"

# eval_refused STATUS CODE NAME [ARGS]: `langc eval examples/entries.lang NAME
# ARGS` must exit STATUS and print `langc: CODE: NAME: ...` on stderr.
eval_refused() {
  want_status=$1
  code=$2
  shift 2
  status=0
  build/langc eval examples/entries.lang "$@" 2>"$tmp/err" >/dev/null || status=$?
  case "$status:$(head -n 1 "$tmp/err")" in
    "$want_status:langc: $code: $1: "*) ;;
    *) echo "FAIL eval $*: want exit $want_status and $code, got exit $status"; fail=$((fail + 1)) ;;
  esac
}
eval_refused 2 EVAL_ARGS addPrice 1
eval_refused 2 EVAL_ARGS addPrice x 1
eval_refused 2 EVAL_ARGS addPrice 18446744073709551616 1
eval_refused 2 EVAL_ARGS pick 2 4 9
eval_refused 1 EVAL_ENTRY nope

status=0
build/langc 2>/dev/null || status=$?
[ "$status" -eq 2 ] || { echo "FAIL usage: want exit 2, got $status"; fail=$((fail + 1)); }
status=0
build/langc check "$tmp/missing.lang" 2>/dev/null || status=$?
[ "$status" -eq 2 ] || { echo "FAIL missing file: want exit 2, got $status"; fail=$((fail + 1)); }
# JSON: `langc build` of each example must equal test/json/NAME.json byte for
# byte, and each golden must parse as JSON.
jsons=0
for prog in examples/*.lang; do
  name=$(basename "$prog" .lang)
  jsons=$((jsons + 1))
  if ! build/langc build "$prog" >"$tmp/out.json" 2>"$tmp/err" || ! cmp -s "$tmp/out.json" "test/json/$name.json"; then
    echo "FAIL build $prog: the output differs from test/json/$name.json"
    fail=$((fail + 1))
  fi
done
if ! node -e 'for (const f of process.argv.slice(1)) JSON.parse(require("fs").readFileSync(f, "utf8"))' test/json/*.json; then
  echo "FAIL: a JSON golden does not parse"
  fail=$((fail + 1))
fi
echo "json: $jsons builds checked"

# Each line of test/emit/expect.txt is `FILE CODE`. `langc build` must exit 1,
# print `langc: CODE: ...` on stderr and print nothing on stdout.
while read -r file code; do
  status=0
  build/langc build "test/emit/$file" >"$tmp/out.json" 2>"$tmp/err" || status=$?
  case "$status:$(head -n 1 "$tmp/err"):$(wc -c <"$tmp/out.json" | tr -d ' ')" in
    "1:langc: $code: "*":0") ;;
    *) echo "FAIL build $file: want exit 1, $code and no stdout, got exit $status"; fail=$((fail + 1)) ;;
  esac
done <test/emit/expect.txt
echo "build refusals: $(wc -l <test/emit/expect.txt | tr -d ' ') checked"

# -o writes the same document; a refused build leaves no file.
build/langc build examples/formers.lang -o "$tmp/o.json" && cmp -s "$tmp/o.json" test/json/formers.json \
  || { echo "FAIL build -o: the file differs"; fail=$((fail + 1)); }
build/langc build test/emit/trap.lang -o "$tmp/no.json" 2>/dev/null || true
[ ! -e "$tmp/no.json" ] || { echo "FAIL build -o: a refused build wrote a file"; fail=$((fail + 1)); }
# A Fin argument of `langc eval` must be below its size.
status=0
build/langc eval examples/fin.lang valueOf 3 3 >/dev/null 2>"$tmp/err" || status=$?
case "$status:$(head -n 1 "$tmp/err")" in
"2:langc: EVAL_ARGS: valueOf: "*) ;;
*) echo "FAIL eval: a Fin argument out of range"; fail=$((fail + 1)) ;;
esac
status=0
build/langc build examples/formers.lang -x 2>/dev/null >/dev/null || status=$?
[ "$status" -eq 2 ] || { echo "FAIL build usage: want exit 2, got $status"; fail=$((fail + 1)); }

# The reader: with --json, each JSON golden reads and writes back with no change.
trips=0
for doc in test/json/*.json; do
  if build/langc read --json "$doc" > "$tmp/read.json" 2>"$tmp/err" && cmp -s "$tmp/read.json" "$doc"; then
    trips=$((trips + 1))
  else
    echo "FAIL read $doc: the document does not read back"; fail=$((fail + 1))
  fi
done
echo "round trips: $trips checked"
# The typed read: one line `name : type = value` for each instance of each
# golden. `langc verify` compares the golden with the example (each value,
# each instance and the bytes of `langc build`) and prints its lines with the
# formatter of `langc eval`. Each read line must equal the verify line at the
# same position. Each read value must also have a matching no-argument eval
# expectation, checked independently by the eval group above.
reads=0
for doc in test/json/*.json; do
  name=$(basename "$doc" .json)
  status=0
  build/langc read "$doc" > "$tmp/typed.txt" 2>"$tmp/err" || status=$?
  [ "$status" -eq 0 ] || { echo "FAIL read $doc: exit $status: $(head -n 1 "$tmp/err")"; fail=$((fail + 1)); }
  status=0
  build/langc verify "examples/$name.lang" "$doc" > "$tmp/verify.txt" 2>"$tmp/err" || status=$?
  [ "$status" -eq 0 ] || { echo "FAIL verify $doc: exit $status: $(head -n 1 "$tmp/err")"; fail=$((fail + 1)); }
  want=$(awk '{ n += gsub(/\{"name":"/, "") } END { print n + 0 }' "$doc")
  got=$(wc -l < "$tmp/typed.txt" | tr -d ' ')
  [ "$got" -eq "$want" ] || { echo "FAIL read $doc: $got lines for $want instances"; fail=$((fail + 1)); }
  got=$(wc -l < "$tmp/verify.txt" | tr -d ' ')
  [ "$got" -eq "$want" ] || { echo "FAIL verify $doc: $got lines for $want instances"; fail=$((fail + 1)); }
  same=$(awk 'FILENAME == ARGV[1] { line[FNR] = $0; next }
    (FNR in line) && line[FNR] == $0 { n++ } END { print n + 0 }' "$tmp/typed.txt" "$tmp/verify.txt")
  [ "$same" -eq "$want" ] || { echo "FAIL read $doc: $same of $want read lines equal the verify lines"; fail=$((fail + 1)); }
  evaluated=$(awk -v file="$name" 'FILENAME == ARGV[1] {
    if ($1 == file && $3 == "=>") {
      inst = $2; sub(/^[^ ]+ [^ ]+ => /, ""); expected[inst] = $0
    }
    next
  }
  {
    inst = $0; sub(/ : .*/, "", inst)
    value = $0; sub(/^.* = /, "", value)
    if ((inst in expected) && "value:" value == "value:" expected[inst]) n++
  }
  END { print n + 0 }' test/eval/expect.txt "$tmp/typed.txt")
  [ "$evaluated" -eq "$want" ] || { echo "FAIL read $doc: $evaluated of $want read values have matching eval expectations"; fail=$((fail + 1)); }
  reads=$((reads + same))
done
echo "reads: $reads checked"
# White space and escapes read to the format of the writer.
build/langc read --json test/read/spaces.json 2>/dev/null | cmp -s - test/json/formers.json \
  || { echo "FAIL read: test/read/spaces.json does not read to test/json/formers.json"; fail=$((fail + 1)); }
build/langc read --json test/read/escapes.json 2>/dev/null | cmp -s - test/read/escapes.out \
  || { echo "FAIL read: test/read/escapes.json does not read to test/read/escapes.out"; fail=$((fail + 1)); }
printf '{"auction-lang":1,"instances":[{"n\134u0061me":"q","type":"Nat","value":["a\134"b","c\134\134d","\134u0041"]}]}\n' > "$tmp/quotes.json"
printf '{"auction-lang":1,"instances":[{"name":"q","type":"Nat","value":["a\134u0022b","c\134u005cd","A"]}]}\n' > "$tmp/quotes.out"
build/langc read --json "$tmp/quotes.json" 2>/dev/null | cmp -s - "$tmp/quotes.out" \
  || { echo "FAIL read: the quote and backslash escapes"; fail=$((fail + 1)); }
printf '{"auction-lang":1,"instances":[{"name":"caf\134u00e9","type":"Nat","value":1}]}\n' > "$tmp/high.json"
# Nesting: the reader permits 2008 levels (the writer writes at most 2005).
deep() {
  awk -v n="$1" 'BEGIN { s = ""; e = ""; for (i = 0; i < n; i++) { s = s "["; e = e "]" }
    printf "{\"auction-lang\":1,\"instances\":[{\"name\":\"deep\",\"type\":\"Nat\",\"value\":%s%s}]}\n", s, e }'
}
deep 2005 > "$tmp/deep-ok.json"
build/langc read --json "$tmp/deep-ok.json" 2>/dev/null | cmp -s - "$tmp/deep-ok.json" \
  || { echo "FAIL read: 2008 levels do not read back"; fail=$((fail + 1)); }
deep 2006 > "$tmp/deep.json"
# Size: a document of 16 MiB reads, and 1 byte more is refused.
awk 'BEGIN { s = " "; for (i = 0; i < 20; i++) s = s s; h = "{\"auction-lang\":1,\"instances\":[]}"
  printf "%s", h; for (i = 0; i < 15; i++) printf "%s", s; printf "%s", substr(s, 1 + length(h)) }' > "$tmp/big-ok.json"
build/langc read --json "$tmp/big-ok.json" 2>/dev/null | cmp -s - test/json/entries.json \
  || { echo "FAIL read: a document of 16 MiB does not read"; fail=$((fail + 1)); }
cp "$tmp/big-ok.json" "$tmp/big.json"
printf ' ' >> "$tmp/big.json"
# Type length: the typed reader permits a `type` of 4096 bytes (584 x `List (`
# around `Fin 1000`). With `Fin 10000` the type has 4097 bytes.
listy() {
  awk -v n="$1" -v t="$2" 'BEGIN { s = ""; e = ""; for (i = 0; i < n; i++) { s = s "List ("; e = e ")" }
    printf "%s%s%s", s, t, e }'
}
printf '{"auction-lang":1,"instances":[{"name":"t","type":"%s","value":[]}]}\n' "$(listy 584 'Fin 1000')" > "$tmp/type-ok.json"
printf '{"auction-lang":1,"instances":[{"name":"t","type":"%s","value":[]}]}\n' "$(listy 584 'Fin 10000')" > "$tmp/type.json"
# Read refusals. A document that test/read does not hold is in $tmp.
checked=0
while read -r file code; do
  doc=test/read/$file
  [ -e "$doc" ] || doc=$tmp/$file
  status=0
  build/langc read "$doc" >/dev/null 2>"$tmp/err" || status=$?
  case "$status:$(head -n 1 "$tmp/err")" in
  "1:langc: $code: "*) checked=$((checked + 1)) ;;
  *) echo "FAIL read $file: want $code, got $status $(head -n 1 "$tmp/err")"; fail=$((fail + 1)) ;;
  esac
done < test/read/expect.txt
echo "read refusals: $checked checked"
# verify_refused STATUS CODE ARGS...: `langc verify ARGS` must exit STATUS,
# print `langc: CODE: ...` on stderr and print nothing on stdout.
verify_refused() {
  want=$1
  code=$2
  shift 2
  status=0
  build/langc verify "$@" >"$tmp/out" 2>"$tmp/err" || status=$?
  first=$(head -n 1 "$tmp/err")
  case "$status:$(wc -c <"$tmp/out" | tr -d ' '):$first" in
  "$want:0:langc: $code: "*) verifies=$((verifies + 1)) ;;
  *) echo "FAIL verify $*: want exit $want, $code and no output; got exit $status: $first"; fail=$((fail + 1)) ;;
  esac
}
# Verify refusals. Each line of test/verify/expect.txt is `FILE CODE`: FILE is
# test/json/functions.json with one change. Then a usage and an IO refusal.
verifies=0
while read -r file code; do
  verify_refused 1 "$code" examples/functions.lang "test/verify/$file"
done < test/verify/expect.txt
verify_refused 2 USAGE examples/functions.lang
verify_refused 2 IO examples/functions.lang "$tmp/none.json"
echo "verify refusals: $verifies checked"
# Limits (slice D4). A value line has no size cut: each eval line and read line
# of test/limits/long.lang is equal to the awk line in full, and the verify
# lines are equal to the read lines. The writer refuses a type of more than
# 4096 bytes or of 200 nested levels. A type of 4096 bytes or of 199 levels
# builds, reads and verifies. A value line of 1999 levels prints in full, and
# eval and read refuse a value line of 2000 levels (O18, slice E1).
limits=0
pass() { limits=$((limits + 1)); }
miss() { echo "FAIL limits: $1"; fail=$((fail + 1)); }
# refused CODE VERB ARGS...: `langc VERB ARGS` must exit 1 with CODE and no stdout.
refused() {
  code=$1
  shift
  status=0
  build/langc "$@" >"$tmp/out" 2>"$tmp/err" || status=$?
  case "$status:$(head -n 1 "$tmp/err"):$(wc -c <"$tmp/out" | tr -d ' ')" in
  "1:langc: $code: "*":0") pass ;;
  *) miss "$*: want exit 1, $code and no stdout, got exit $status" ;;
  esac
}
# build_refused FILE CODE: `langc build FILE` must exit 1 with CODE and no stdout.
build_refused() { refused "$2" build "$1"; }
# read_back PROG DOC: DOC reads, and `langc verify PROG DOC` prints the read lines.
read_back() {
  build/langc read "$2" >"$tmp/read.txt" 2>/dev/null \
    && build/langc verify "$1" "$2" 2>/dev/null | cmp -s - "$tmp/read.txt"
}
awk 'BEGIN { printf "["; for (i = 0; i < 128; i++) { printf "%s[", (i ? ", " : "")
  for (j = 0; j < 128; j++) printf "%s%s/1", (j ? ", " : ""), (i == j ? 1 : 0); printf "]" } printf "]" }' > "$tmp/id"
{ printf 'cons '; cat "$tmp/id"; printf ' nil\n'; } > "$tmp/wide.want"
{ printf 'pair '; cat "$tmp/id"; printf ' 7\n'; } > "$tmp/both.want"
{ printf 'wide : List (Matrix 128 128) = '; cat "$tmp/wide.want"
  printf 'both : Prod (Matrix 128 128) Nat = '; cat "$tmp/both.want"; } > "$tmp/long.want"
[ "$(wc -c < "$tmp/wide.want")" -gt 65537 ] && pass || miss "the wide line is not longer than 65536 bytes"
for name in wide both; do
  build/langc eval test/limits/long.lang "$name" > "$tmp/$name.got" 2>/dev/null \
    && cmp -s "$tmp/$name.got" "$tmp/$name.want" && pass || miss "eval $name: not the full line"
done
build/langc build test/limits/long.lang -o "$tmp/long.json" 2>/dev/null \
  && build/langc read "$tmp/long.json" 2>/dev/null | cmp -s - "$tmp/long.want" && pass || miss "read long.json: not the full lines"
read_back test/limits/long.lang "$tmp/long.json" && pass || miss "verify long.json: not the read lines"
[ "$(listy 584 'Fin 1000' | wc -c)" -eq 4096 ] && pass || miss "the boundary type does not have 4096 bytes"
printf 'def t : %s := nil\n' "$(listy 584 'Fin 1000')" > "$tmp/t4096.lang"
printf 'def t : %s := nil\n' "$(listy 584 'Fin 10000')" > "$tmp/t4097.lang"
build/langc build "$tmp/t4096.lang" -o "$tmp/t4096.json" 2>/dev/null \
  && cmp -s "$tmp/t4096.json" "$tmp/type-ok.json" && pass || miss "a type of 4096 bytes does not build"
read_back "$tmp/t4096.lang" "$tmp/t4096.json" && pass || miss "a type of 4096 bytes does not read back"
build_refused "$tmp/t4097.lang" JSON_TYPE_SIZE
# Nesting: n levels of `Prod (...) Nat`. The printer cuts at 200 levels.
nest() {
  awk -v n="$1" 'BEGIN { t = "Nat"; v = "0"; for (i = 0; i < n; i++) { t = "Prod (" t ") Nat"; v = "pair (" v ") 0" }
    printf "def d : %s := %s\n", t, v }'
}
nest 199 > "$tmp/nest199.lang"
nest 200 > "$tmp/nest200.lang"
build/langc build "$tmp/nest199.lang" -o "$tmp/nest199.json" 2>/dev/null \
  && read_back "$tmp/nest199.lang" "$tmp/nest199.json" && pass || miss "a type of 199 levels does not build and read back"
build_refused "$tmp/nest200.lang" JSON_TYPE_SIZE
# Value depth. `fpair N`: a pair of a function and 0, with N levels of `natAdd`
# in the body, so N + 2 levels. `sdoc K`: a document of `pack K P`, where P has
# K levels of `pair` (a literal stops at 1000 levels, the parser limit).
fpair() {
  printf 'def f : Prod (Nat -> Nat) Nat := pair (fun x => fold (fun a => natAdd a 1) x %s) 0\n' "$1"
}
sdoc() {
  awk -v k="$1" 'BEGIN { s = ""; e = ""; for (i = 0; i < k; i++) { s = s "{\"first\":"; e = e ",\"second\":0}" }
    printf "{\"auction-lang\":1,\"instances\":[{\"name\":\"s\",\"type\":\"Sigma (n : Nat) (fold (fun t => Prod t Nat) Nat n)\",\"value\":{\"witness\":%d,\"payload\":%s0%s}}]}\n", k, s, e }'
}
fpair 1997 > "$tmp/f1999.lang"
fpair 1998 > "$tmp/f2000.lang"
awk 'BEGIN { s = ""; e = ""; for (i = 1; i < 1997; i++) { s = s "natAdd ("; e = e ") 1" }
  printf "pair (fun x => %snatAdd x 1%s) 0\n", s, e }' > "$tmp/f1999.want"
build/langc eval "$tmp/f1999.lang" f > "$tmp/f1999.got" 2>/dev/null \
  && cmp -s "$tmp/f1999.got" "$tmp/f1999.want" && pass || miss "eval of 1999 levels: not the full line"
refused EVAL_PRINT_DEPTH eval "$tmp/f2000.lang" f
sdoc 1999 > "$tmp/s1999.json"
sdoc 2000 > "$tmp/s2000.json"
awk 'BEGIN { s = ""; e = ""; for (i = 0; i < 1999; i++) { s = s "(pair "; e = e " 0)" }
  printf "s : Sigma (n : Nat) (fold (fun t => Prod t Nat) Nat n) = pack 1999 %s0%s\n", s, e }' > "$tmp/s1999.want"
build/langc read "$tmp/s1999.json" 2>/dev/null | cmp -s - "$tmp/s1999.want" && pass || miss "read of 1999 levels: not the full line"
refused READ_PRINT_DEPTH read "$tmp/s2000.json"
echo "limits: $limits checked"
# Queries. A line is PROG DOC NAME [ARGS...] => OUTPUT, or
# PROG DOC NAME => refuse CODE. PROG is a program of test/query.
queries=0
while read -r prog doc rest; do
  name_args=${rest%% => *}
  want=${rest#* => }
  status=0
  got=$(build/langc eval "test/query/$prog.lang" --read "$doc" $name_args 2>"$tmp/err") || status=$?
  first=$(head -n 1 "$tmp/err")
  case "$want" in
  "refuse "*) want_status=1; want_got=""; want_err="langc: ${want#refuse }: " ;;
  *) want_status=0; want_got=$want; want_err="" ;;
  esac
  case "$status|$got|$first" in
  "$want_status|$want_got|$want_err"*) queries=$((queries + 1)) ;;
  *) echo "FAIL query $prog $name_args: want $want, got exit $status: $got $first"; fail=$((fail + 1)) ;;
  esac
done < test/query/expect.txt
echo "queries: $queries checked"
build/langc build test/query/matrix.lang --read test/json/matrix.json -o "$tmp/q.json" 2>/dev/null \
  && [ "$(build/langc read "$tmp/q.json" | wc -l | tr -d ' ')" = 4 ] \
  || { echo "FAIL build --read: the document does not hold the 4 program instances only"; fail=$((fail + 1)); }
status=0
build/langc eval test/query/plain.lang --read >/dev/null 2>&1 || status=$?
[ "$status" -eq 2 ] || { echo "FAIL eval --read with no document: want exit 2, got $status"; fail=$((fail + 1)); }
build/read-typed-test || fail=$((fail + 1))

rm -rf "$tmp"

if rg -n '\x{2013}|\x{2014}' . >/dev/null; then
  echo "FAIL: an em-dash or en-dash is in the kit"
  fail=$((fail + 1))
fi
echo "gate: $fail failures"
[ "$fail" -eq 0 ]
