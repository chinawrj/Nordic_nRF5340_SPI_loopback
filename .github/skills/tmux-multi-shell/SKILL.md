---
name: "tmux-multi-shell"
description: "tmux multi-terminal management — idempotent session creation, command completion waiting, exit-code detection, output capture"
---

# Skill: tmux Multi-Terminal Management (AI-agent-optimised edition)

## Purpose

Provide an AI agent with reliable multi-terminal automation. Core value: **run commands in parallel across isolated windows, reliably wait for completion, capture full output, and determine success/failure.**

**When to use:**
- You need to run multiple terminal tasks concurrently (build + flash + serial monitor)
- You need a persistent terminal session (survives agent restarts)
- You need to send a command and reliably capture its result

**When not to use:**
- A single terminal is enough
- Non-CLI environments

## Prerequisites

- `tmux` installed (macOS: `brew install tmux`, Linux: `apt install tmux`)
- The terminal supports tmux

## Procedure

### 1. Idempotent project-session creation (P0)

> **Key:** the agent may restart or retry, so always use `has-session` to avoid duplicate creation.

```bash
# Idempotent create — skip if the session already exists
tmux has-session -t {{PROJECT_NAME}} 2>/dev/null || {
  tmux new-session -d -s {{PROJECT_NAME}}
  tmux rename-window -t {{PROJECT_NAME}}:0 'edit'
  tmux new-window -t {{PROJECT_NAME}} -n 'build'
  tmux new-window -t {{PROJECT_NAME}} -n 'flash'
  tmux new-window -t {{PROJECT_NAME}} -n 'monitor'
  echo "[tmux] Session '{{PROJECT_NAME}}' created with 4 windows"
}

# Verify the session is ready
tmux list-windows -t {{PROJECT_NAME}} -F '#{window_index}:#{window_name}'
```

```bash
# Idempotent add of a single window — skip if it already exists
tmux list-windows -t {{PROJECT_NAME}} -F '#{window_name}' | grep -q '^build$' || \
  tmux new-window -t {{PROJECT_NAME}} -n 'build'
```

### 2. Standard window layout

| Window index | Name | Purpose |
|--------------|------|---------|
| 0 | edit | code editing / git operations |
| 1 | build | compilation |
| 2 | flash | firmware flashing |
| 3 | monitor | serial log monitoring |

### 3. Send commands to a specific window

```bash
# Compile in the build window
tmux send-keys -t {{PROJECT_NAME}}:build 'west build --sysbuild' C-m

# Flash in the flash window (requires hardware)
tmux send-keys -t {{PROJECT_NAME}}:flash 'west flash' C-m

# Start the serial monitor (requires hardware)
tmux send-keys -t {{PROJECT_NAME}}:monitor 'minicom -D /dev/ttyACM0 -b 115200' C-m
```

### 4. Wait for command completion + exit-code detection (P0)

> **Core pattern:** the AI-agent operation loop — send command → wait for completion → read exit code → judge success/failure.

**Principle:** wrap the real command with a sentinel and poll `capture-pane` until the sentinel appears.

```bash
# === Send a command with a sentinel ===
# Format: actual command; then echo sentinel + exit code
SENTINEL="__DONE_$(date +%s)__"
tmux send-keys -t {{PROJECT_NAME}}:build \
  "west build --sysbuild; echo \"${SENTINEL}_EXIT_\$?\"" C-m

# === Poll with a timeout ===
TIMEOUT=120  # seconds
ELAPSED=0
INTERVAL=2   # check every 2 seconds
sleep 1      # give the command time to start
while [ $ELAPSED -lt $TIMEOUT ]; do
  OUTPUT=$(tmux capture-pane -t {{PROJECT_NAME}}:build -p -S -1000)
  if echo "$OUTPUT" | grep -q "${SENTINEL}_EXIT_[0-9]"; then
    # Extract the exit code
    EXIT_CODE=$(echo "$OUTPUT" | grep -o "${SENTINEL}_EXIT_[0-9][0-9]*" | head -1 | grep -o '[0-9]*$')
    if [ "$EXIT_CODE" = "0" ]; then
      echo "[OK] Command succeeded (exit code 0)"
    else
      echo "[FAIL] Command failed (exit code $EXIT_CODE)"
    fi
    break
  fi
  sleep $INTERVAL
  ELAPSED=$((ELAPSED + INTERVAL))
done

if [ $ELAPSED -ge $TIMEOUT ]; then
  echo "[TIMEOUT] Command did not complete within ${TIMEOUT}s"
fi
```

**Simplified — wrapped as a function:**

```bash
# tmux_exec: run a command in the given window, wait for completion, return the exit code
# Usage: tmux_exec <session:window> <command> [timeout_seconds]
tmux_exec() {
  local target="$1"
  local cmd="$2"
  local timeout="${3:-120}"
  local sentinel="__DONE_${RANDOM}__"

  # Send command
  tmux send-keys -t "$target" "$cmd; echo \"${sentinel}_EXIT_\$?\"" C-m

  # Poll
  local elapsed=0
  sleep 1  # give the command time to start
  while [ $elapsed -lt $timeout ]; do
    local output
    output=$(tmux capture-pane -t "$target" -p -S -1000)
    local match
    match=$(echo "$output" | grep -o "${sentinel}_EXIT_[0-9][0-9]*" | head -1)
    if [ -n "$match" ]; then
      echo "$output"  # print the full content for the caller to analyse
      local code
      code=$(echo "$match" | grep -o '[0-9]*$')
      return "${code:-1}"
    fi
    sleep 2
    elapsed=$((elapsed + 2))
  done

  echo "[TIMEOUT] after ${timeout}s"
  return 124  # match GNU timeout
}

# Example
tmux_exec "{{PROJECT_NAME}}:build" "west build --sysbuild" 300
if [ $? -eq 0 ]; then
  echo "Build succeeded, proceeding to flash..."
  tmux_exec "{{PROJECT_NAME}}:flash" "west flash" 60
fi
```

### 5. Full output capture (P0)

> **Key:** by default `capture-pane -p` only captures the visible pane (~50 lines). A build error may be hundreds of lines earlier, so the AI must read the full history.

```bash
# Capture the last 1000 history lines (covers most build output)
tmux capture-pane -t {{PROJECT_NAME}}:build -p -S -1000

# Capture everything (from the start of the buffer)
tmux capture-pane -t {{PROJECT_NAME}}:build -p -S -

# Capture and save to a file (useful for very long output)
tmux capture-pane -t {{PROJECT_NAME}}:build -p -S - > /tmp/build-output.txt
wc -l /tmp/build-output.txt  # check line count

# Only read the last N lines (save agent context window)
tmux capture-pane -t {{PROJECT_NAME}}:build -p -S -1000 | tail -50
```

**Enlarge the history buffer (before creating the session):**

```bash
# Scrollback set to 10000 lines (default 2000)
tmux set-option -g history-limit 10000
```

### 6. Timeout handling and command abortion (P1)

> **Scenario:** the command hangs (waiting for input, network stall), the AI must abort and recover.

```bash
# Option 1: send Ctrl+C to abort the current command
tmux send-keys -t {{PROJECT_NAME}}:build C-c

# Option 2: send Ctrl+C and wait for the shell prompt to recover
tmux send-keys -t {{PROJECT_NAME}}:build C-c
sleep 1
# Check the shell prompt is back (by sending an empty echo test)
tmux send-keys -t {{PROJECT_NAME}}:build 'echo __SHELL_READY__' C-m
sleep 1
tmux capture-pane -t {{PROJECT_NAME}}:build -p | grep -q __SHELL_READY__ && \
  echo "Shell recovered" || echo "Shell still stuck"

# Option 3: kill and recreate the window (last resort)
tmux kill-window -t {{PROJECT_NAME}}:build
tmux new-window -t {{PROJECT_NAME}} -n 'build'
```

**`tmux_exec` already has built-in timeout handling** (see section 4); it returns exit code 124 on timeout. A typical chain:

```bash
tmux_exec "{{PROJECT_NAME}}:build" "west build --sysbuild" 300
rc=$?
case $rc in
  0)   echo "Success" ;;
  124) echo "Timeout — sending Ctrl+C"
       tmux send-keys -t {{PROJECT_NAME}}:build C-c ;;
  *)   echo "Failed with exit code $rc" ;;
esac
```

### 7. Structured output parsing (P1)

> **Goal:** pull AI-consumable structured information out of the raw `capture-pane` text instead of hoping `grep` hits the right line.

```bash
# === Parse west build output ===
OUTPUT=$(tmux capture-pane -t {{PROJECT_NAME}}:build -p -S -1000)

# Error count
ERROR_COUNT=$(echo "$OUTPUT" | grep -c "^.*error:")
WARNING_COUNT=$(echo "$OUTPUT" | grep -c "^.*warning:")

# Compile progress (ninja format: [nn/mm])
PROGRESS=$(echo "$OUTPUT" | grep -oE '\[[0-9]+/[0-9]+\]' | tail -1)

# Firmware-size information
IMAGE_SIZE=$(echo "$OUTPUT" | grep -oE 'Memory region.*Used' | head -3)

# Summary report
echo "=== Build Summary ==="
echo "Errors:   $ERROR_COUNT"
echo "Warnings: $WARNING_COUNT"
echo "Progress: $PROGRESS"
echo "Firmware: $IMAGE_SIZE"
```

```bash
# === Generic command-output parsing pattern ===
# Extract the full context of the first error (±3 lines)
tmux capture-pane -t {{PROJECT_NAME}}:build -p -S -1000 | \
  grep -n "error:" | head -1 | cut -d: -f1 | \
  xargs -I{} sed -n "$(({}>=3?{}-3:1)),$(({} + 3))p" <(
    tmux capture-pane -t {{PROJECT_NAME}}:build -p -S -1000
  )
```

### 8. Parallel-command coordination (P1)

> **Scenario:** launch tasks in multiple windows simultaneously, and wait for all of them to finish before continuing.

```bash
# === Parallel execution, collect all results ===

# Launch a command in each window (each with its own sentinel)
S1="__DONE_build_${RANDOM}__"
S2="__DONE_test_${RANDOM}__"

tmux send-keys -t {{PROJECT_NAME}}:build "make build; echo \"${S1}_EXIT_\$?\"" C-m
tmux send-keys -t {{PROJECT_NAME}}:edit  "make test; echo \"${S2}_EXIT_\$?\"" C-m

# Wait for all commands to finish
TIMEOUT=300
ELAPSED=0
BUILD_DONE=0
TEST_DONE=0

while [ $ELAPSED -lt $TIMEOUT ] && { [ $BUILD_DONE -eq 0 ] || [ $TEST_DONE -eq 0 ]; }; do
  if [ $BUILD_DONE -eq 0 ]; then
    tmux capture-pane -t {{PROJECT_NAME}}:build -p -S -1000 | grep -q "${S1}_EXIT_" && BUILD_DONE=1
  fi
  if [ $TEST_DONE -eq 0 ]; then
    tmux capture-pane -t {{PROJECT_NAME}}:edit -p -S -1000 | grep -q "${S2}_EXIT_" && TEST_DONE=1
  fi
  sleep 2
  ELAPSED=$((ELAPSED + 2))
done

# Report
echo "Build: $([ $BUILD_DONE -eq 1 ] && echo 'completed' || echo 'TIMEOUT')"
echo "Test:  $([ $TEST_DONE -eq 1 ] && echo 'completed' || echo 'TIMEOUT')"
```

### 9. Session management

```bash
# List all sessions
tmux list-sessions

# Check session existence (for scripts)
tmux has-session -t {{PROJECT_NAME}} 2>/dev/null && echo "exists" || echo "not found"

# Attach to the session (for human debugging)
tmux attach-session -t {{PROJECT_NAME}}

# Clean shutdown — send Ctrl+C to every window, then destroy
for win in $(tmux list-windows -t {{PROJECT_NAME}} -F '#{window_name}'); do
  tmux send-keys -t "{{PROJECT_NAME}}:${win}" C-c 2>/dev/null
done
sleep 1
tmux kill-session -t {{PROJECT_NAME}}
```

## Self-Test

> Validate that tmux is usable and that all P0/P1 capabilities work.

### Self-test steps

```bash
#!/bin/bash
# self-test for tmux-multi-shell
# Run: bash skills/tmux-multi-shell/self-test.sh

SESSION="__tmux_skill_test__"
PASS=0
FAIL=0

test_case() {
  local name=$1; shift
  if "$@"; then
    echo "SELF_TEST_PASS: $name"
    PASS=$((PASS + 1))
  else
    echo "SELF_TEST_FAIL: $name"
    FAIL=$((FAIL + 1))
  fi
}

cleanup() { tmux kill-session -t $SESSION 2>/dev/null; }
trap cleanup EXIT

# --- Test 1: tmux installed ---
test_case "tmux_installed" command -v tmux

# --- Test 2: idempotent session creation (P0) ---
test_case "idempotent_session" bash -c '
  SESSION="__tmux_skill_test__"
  tmux kill-session -t $SESSION 2>/dev/null
  # First create
  tmux has-session -t $SESSION 2>/dev/null || tmux new-session -d -s $SESSION
  # Repeat call does not error
  tmux has-session -t $SESSION 2>/dev/null || tmux new-session -d -s $SESSION
  # Verify only one exists
  COUNT=$(tmux list-sessions -F "#{session_name}" | grep -c "^${SESSION}$")
  [ "$COUNT" = "1" ]
'

# --- Test 3: multi-window creation + send-keys ---
test_case "multi_window" bash -c '
  SESSION="__tmux_skill_test__"
  tmux new-window -t $SESSION -n win_test 2>/dev/null
  tmux send-keys -t $SESSION:win_test "echo hello_tmux_test" C-m
  sleep 1
  tmux capture-pane -t $SESSION:win_test -p | grep -q hello_tmux_test
'

# --- Test 4: command completion wait + exit code (P0) ---
test_case "wait_and_exit_code" bash -c '
  SESSION="__tmux_skill_test__"
  SENTINEL="__DONE_TEST_$$__"
  tmux send-keys -t $SESSION:win_test "true; echo \"${SENTINEL}_EXIT_\$?\"" C-m
  sleep 1
  ELAPSED=0
  while [ $ELAPSED -lt 10 ]; do
    OUTPUT=$(tmux capture-pane -t $SESSION:win_test -p -S -100)
    MATCH=$(echo "$OUTPUT" | grep -o "${SENTINEL}_EXIT_[0-9][0-9]*" | head -1)
    if [ -n "$MATCH" ]; then
      CODE=$(echo "$MATCH" | grep -o "[0-9]*$")
      [ "$CODE" = "0" ] && exit 0 || exit 1
    fi
    sleep 1
    ELAPSED=$((ELAPSED + 1))
  done
  exit 1  # timeout
'

# --- Test 5: detect failure exit code (P0) ---
test_case "detect_failure_exit_code" bash -c '
  SESSION="__tmux_skill_test__"
  SENTINEL="__DONE_FAIL_$$__"
  tmux send-keys -t $SESSION:win_test "false; echo \"${SENTINEL}_EXIT_\$?\"" C-m
  sleep 1
  ELAPSED=0
  while [ $ELAPSED -lt 10 ]; do
    OUTPUT=$(tmux capture-pane -t $SESSION:win_test -p -S -100)
    MATCH=$(echo "$OUTPUT" | grep -o "${SENTINEL}_EXIT_[0-9][0-9]*" | head -1)
    if [ -n "$MATCH" ]; then
      CODE=$(echo "$MATCH" | grep -o "[0-9]*$")
      [ "$CODE" = "1" ] && exit 0 || exit 1  # expect exit code = 1
    fi
    sleep 1
    ELAPSED=$((ELAPSED + 1))
  done
  exit 1
'

# --- Test 6: full output capture (P0) ---
test_case "full_output_capture" bash -c '
  SESSION="__tmux_skill_test__"
  # Produce 200 lines of output (beyond the visible pane)
  tmux send-keys -t $SESSION:win_test "for i in \$(seq 1 200); do echo \"LINE_\$i\"; done" C-m
  sleep 2
  # Capture with -S -1000 and verify we can see line 1
  OUTPUT=$(tmux capture-pane -t $SESSION:win_test -p -S -1000)
  echo "$OUTPUT" | grep -q "LINE_1" && echo "$OUTPUT" | grep -q "LINE_200"
'

# --- Test 7: timeout abort (P1) ---
test_case "timeout_abort" bash -c '
  SESSION="__tmux_skill_test__"
  # Send a command that will block
  tmux send-keys -t $SESSION:win_test "sleep 60" C-m
  sleep 1
  # Ctrl+C abort
  tmux send-keys -t $SESSION:win_test C-c
  sleep 1
  # Verify the shell recovered — can run a new command
  tmux send-keys -t $SESSION:win_test "echo __RECOVERED__" C-m
  sleep 1
  tmux capture-pane -t $SESSION:win_test -p | grep -q __RECOVERED__
'

# --- Test 8: parallel coordination (P1) ---
test_case "parallel_coordination" bash -c '
  SESSION="__tmux_skill_test__"
  tmux new-window -t $SESSION -n win_para 2>/dev/null
  S1="__PARA1_$$__"
  S2="__PARA2_$$__"
  tmux send-keys -t $SESSION:win_test "sleep 1; echo \"${S1}_EXIT_\$?\"" C-m
  tmux send-keys -t $SESSION:win_para "sleep 1; echo \"${S2}_EXIT_\$?\"" C-m
  ELAPSED=0; DONE1=0; DONE2=0
  while [ $ELAPSED -lt 15 ] && { [ $DONE1 -eq 0 ] || [ $DONE2 -eq 0 ]; }; do
    [ $DONE1 -eq 0 ] && tmux capture-pane -t $SESSION:win_test -p -S -100 | grep -q "${S1}_EXIT_" && DONE1=1
    [ $DONE2 -eq 0 ] && tmux capture-pane -t $SESSION:win_para -p -S -100 | grep -q "${S2}_EXIT_" && DONE2=1
    sleep 1
    ELAPSED=$((ELAPSED + 1))
  done
  [ $DONE1 -eq 1 ] && [ $DONE2 -eq 1 ]
'

# --- Summary ---
echo ""
echo "Results: $PASS passed, $FAIL failed"
exit $FAIL
```

### Expected results

| Test | Priority | Expected output | Capability verified |
|------|----------|-----------------|---------------------|
| tmux_installed | - | `SELF_TEST_PASS` | tmux available |
| idempotent_session | P0 | `SELF_TEST_PASS` | duplicate creates do not conflict |
| multi_window | - | `SELF_TEST_PASS` | multi-window + send-keys |
| wait_and_exit_code | P0 | `SELF_TEST_PASS` | sentinel wait + exit code 0 |
| detect_failure_exit_code | P0 | `SELF_TEST_PASS` | detect command failure (exit=1) |
| full_output_capture | P0 | `SELF_TEST_PASS` | `-S -1000` reads full history |
| timeout_abort | P1 | `SELF_TEST_PASS` | Ctrl+C abort + shell recovery |
| parallel_coordination | P1 | `SELF_TEST_PASS` | multi-window parallel wait |

### Blind Test

**Scenario:**
The AI agent needs to build a project inside tmux, wait for completion, check success, and extract the error when it fails.

**Test prompt:**
```
You are an AI development assistant. Read this skill, then:
1. Idempotently create a tmux session named "testproj" (do not duplicate-create if it exists)
2. Ensure there is a build window
3. In the build window run "echo compile-start && sleep 2 && echo compile-done"
4. Use the sentinel pattern to wait for the command to finish and capture the exit code
5. Use full output capture (-S -1000) to verify both "compile-start" and "compile-done" are in the output
6. Then run a failing command "exit 42" (use bash -c so the shell is not killed),
   verify you can detect the non-zero exit code
7. Finally clean up the session
```

**Acceptance criteria:**
- [ ] The agent idempotently creates via `has-session` (not just `new-session`)
- [ ] The agent uses sentinel + polling to wait (not `sleep` guesswork)
- [ ] The agent correctly extracts the exit code and judges success/failure
- [ ] The agent uses `-S -1000` or `-S -` to capture full output
- [ ] The agent detects the non-zero exit code of the failing command
- [ ] The agent cleans up the test session at the end

**Common failure modes:**
- Using `sleep 5` instead of sentinel waiting → unreliable, the sentinel pattern must be enforced
- Direct `new-session` without checking `has-session` → errors on duplicate create
- `capture-pane -p` without `-S` → incomplete output
- Running `exit 42` directly in the window exits the shell → use `bash -c "exit 42"`

## Success Criteria

- [ ] tmux session created idempotently with all required windows
- [ ] Each window can independently receive and run commands
- [ ] Sentinel pattern reliably waits for command completion and extracts exit codes
- [ ] `capture-pane -S -1000` can retrieve full output history
- [ ] On timeout, Ctrl+C can abort and the shell recovers
- [ ] Parallel commands across multiple windows can be coordinated
