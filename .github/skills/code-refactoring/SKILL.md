---
name: "code-refactoring"
description: "Cyclical code-refactoring strategy — trigger evaluation, safe refactoring flow, embedded memory optimisation"
---

# Skill: Code Refactoring Strategy

## Purpose

Define a strategy and process for cyclical refactoring so that code quality is maintained throughout continuous development.

**When to use:**
- A planned refactor every 5 iteration days
- When code smells are detected
- A cleanup phase after a feature is complete
- When code complexity exceeds thresholds

**When not to use:**
- Early prototyping (make it work first)
- In the middle of an urgent bug fix
- Modules whose code quality is already good

## Prerequisites

- All current tests pass (green before refactoring)
- Clean git working tree (no uncommitted changes)
- The refactoring goal is defined

## Procedure

### 1. Refactoring triggers

Evaluate the following metrics before each refactor:

| Metric | Threshold | How to measure |
|--------|-----------|----------------|
| Function length | > 50 lines | `wc -l` |
| File length | > 500 lines | `wc -l` |
| Duplicate code | > 3 places | `grep` search |
| TODO/FIXME | > 5 | `grep -rn "TODO\|FIXME"` |
| Nesting depth | > 4 levels | code review |
| Globals | > 10 | `grep` search |

### 2. Refactoring checklist

```markdown
## Refactor checklist - Day N

### Code checks
- [ ] Find overly long functions and split them
- [ ] Extract duplicated code into shared functions
- [ ] Eliminate magic numbers, define constants
- [ ] Check naming consistency
- [ ] Clean up unused #include / import
- [ ] Resolve all TODO/FIXME

### Architecture checks
- [ ] Inter-module dependencies reasonable
- [ ] Interfaces clear (inputs/outputs explicit)
- [ ] Error handling consistent
- [ ] Memory management correct (important for embedded)

### Documentation checks
- [ ] README in sync with code
- [ ] Key functions documented
- [ ] Configuration notes complete
```

### 3. Safe refactoring flow

```bash
# Step 1: ensure tests pass
west build --sysbuild && bash verify-acceptance.sh && echo "green ✅"

# Step 2: create a refactor branch
git checkout -b refactor/day-N-cleanup

# Step 3: perform the refactor (small, frequent commits)
# ... code changes ...
git add -A && git commit -m "refactor: extract SPI init to dedicated module"

# ... more changes ...
git add -A && git commit -m "refactor: replace magic numbers with constants"

# Step 4: verify after refactoring
west build --sysbuild && bash verify-acceptance.sh && echo "still green after refactor ✅"

# Step 5: merge
git checkout main && git merge refactor/day-N-cleanup
```

### 4. Zephyr / embedded-specific refactoring

#### Memory optimisation
```c
// Before: large buffer on the stack
void spi_test(void) {
    uint8_t buf[4096];  // may overflow the thread stack
    // ...
}

// After: use a static allocation
static uint8_t s_buf[4096];
void spi_test(void) {
    // use s_buf
}
```

#### Modularisation
```
// Before: everything in main.c
main.c (800 lines)

// After: split by feature
main.c           (50 lines)  - entry and init dispatch
spi_loopback.c   (150 lines) - SPI loopback test
ble_hrs.c        (150 lines) - BLE Heart Rate Service
```

#### Unified error handling
```c
// Define a unified error-check macro (Zephyr style)
#define CHECK_ERR(x, msg) do { \
    int err = (x); \
    if (err) { \
        LOG_ERR("%s failed: %d", msg, err); \
        return err; \
    } \
} while (0)
```

### 5. Refactor report

Produce a report after each refactor:

```markdown
## Refactor Report - Day N

### Change summary
- Split files: main.c → 5 module files
- Remove duplication: 3 duplicated blocks extracted into shared functions
- Cleanup: removed 8 TODOs, 2 unused variables

### Code metrics delta
| Metric | Before | After |
|--------|--------|-------|
| Max function lines | 120 | 45 |
| File count | 2 | 6 |
| TODO count | 12 | 4 |

### Test results
- Build: ✅ passed
- Acceptance script: ✅ verify-acceptance.sh all PASS
```

## Self-Test

> Validate the refactoring workflow's tools and process.

### Self-test steps

```bash
# Test 1: git available and supports branch operations
TMP_REPO=$(mktemp -d)
cd "$TMP_REPO" && git init -q && \
  echo "init" > file.txt && git add . && git commit -q -m "init" && \
  git checkout -q -b refactor/test && \
  echo "refactored" > file.txt && git add . && git commit -q -m "refactor: test" && \
  git checkout -q main && git merge -q refactor/test && \
  echo "SELF_TEST_PASS: git_branch_workflow" || echo "SELF_TEST_FAIL: git_branch_workflow"
rm -rf "$TMP_REPO"

# Test 2: code-analysis tools available
command -v grep &>/dev/null && command -v wc &>/dev/null && \
  echo "SELF_TEST_PASS: code_analysis_tools" || echo "SELF_TEST_FAIL: code_analysis_tools"

# Test 3: TODO/FIXME detection logic
TMP_FILE=$(mktemp)
cat > "$TMP_FILE" << 'EOF'
// TODO: fix this
// FIXME: memory leak
void ok_function() {}
// TODO: another one
EOF
COUNT=$(grep -c 'TODO\|FIXME' "$TMP_FILE")
[ "$COUNT" -eq 3 ] && echo "SELF_TEST_PASS: todo_detection ($COUNT found)" || echo "SELF_TEST_FAIL: todo_detection (expected 3, got $COUNT)"
rm "$TMP_FILE"

# Test 4: long-function detection
TMP_SRC=$(mktemp --suffix=.c 2>/dev/null || mktemp)
for i in $(seq 1 60); do echo "line $i;" >> "$TMP_SRC"; done
LINES=$(wc -l < "$TMP_SRC")
[ "$LINES" -gt 50 ] && echo "SELF_TEST_PASS: long_function_detect ($LINES lines)" || echo "SELF_TEST_FAIL: long_function_detect"
rm "$TMP_SRC"
```

### Blind Test

**Test prompt:**
```
You are an AI development assistant. Read this skill, then analyse the following code for refactoring:

void handle_everything() {
    // 60 lines of WiFi init
    // 40 lines of HTTP server
    // 30 lines of sensor reads
    // 5 TODOs and 2 FIXMEs
    // 3 duplicated blocks of error handling
}

1. Identify every refactoring trigger (per the threshold table in the skill)
2. Propose a specific refactor plan (which modules to split into)
3. Produce a refactor report template
```

**Acceptance criteria:**
- [ ] The agent identifies the function is too long (130 lines > 50-line threshold)
- [ ] The agent identifies TODO/FIXME exceeds the threshold (7 > 5)
- [ ] The agent identifies the duplicated code
- [ ] The agent proposes splitting into wifi_manager, http_server, sensor_reader
- [ ] The agent produces a refactor report in the skill's format

## Success Criteria

- [ ] All tests continue to pass before and after the refactor
- [ ] Code metrics measurably improve
- [ ] Clean git history with small, frequent refactor commits
