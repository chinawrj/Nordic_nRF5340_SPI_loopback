---
name: "daily-iteration"
description: "Daily iteration workflow — planning, task execution, regression gate, progress tracking"
---

# Skill: Daily Iteration Workflow

## Purpose

Define the AI-agent-driven daily development iteration, including planning, execution, progress validation, and daily reporting.

**When to use:**
- You need a structured development rhythm
- The project spans more than one day
- You need to track progress and milestones

**When not to use:**
- A one-off small task
- A simple project that does not need iteration

## Prerequisites

- Requirements document is in place (`requirements.md`)
- Workflow agent is configured
- Development environment is ready

## Procedure

### 1. Daily iteration model

```
┌─────────────────────────────────────────────────┐
│                 daily iteration loop              │
│                                                  │
│  ┌──────────┐   ┌──────────┐   ┌──────────┐    │
│  │ Morning  │──▶│ Execute  │──▶│ Evening  │    │
│  │ Planning │   │ & Test   │   │ Review   │    │
│  └──────────┘   └──────────┘   └──────────┘    │
│       │                              │           │
│       └────────── next day ◀─────────┘           │
└─────────────────────────────────────────────────┘
```

### 2. Morning Planning

At the start of each day:

```markdown
## Day N Plan

### Yesterday recap
- Completed: [list completed tasks]
- Unfinished: [list unfinished items with reasons]
- Blockers: [list blockers]

### Today's goals
1. [Goal 1] - estimated effort
2. [Goal 2] - estimated effort
3. [Goal 3] - estimated effort

### Risks and dependencies
- [risk item]

### Acceptance checkpoints
- [ ] [checkpoint 1]
- [ ] [checkpoint 2]
```

### 3. Execute & Test

Each task follows the flow:

```
write code → compile check → flash test → serial verify → Web UI verify
    ↑                                                         │
    └────── fix issues ◀─────────────────────────────────────┘
```

Key rules:
- Run tests immediately after each task
- Test failures must be fixed before moving to the next task
- Git-commit at each milestone

### 3.5 Regression Gate ⚠️ mandatory daily

Before the Evening Review, run the regression check to make sure new work did not break already-completed features:

```bash
# Run the acceptance script to cover every already-signed-off feature
bash verify-acceptance.sh
```

Rules:
- Verification scope is **monotonically increasing** with milestones
- Any regression must be fixed the same day — never kicked to tomorrow
- Only proceed to the wrap-up commit after regression passes

### 4. Evening Review

At the end of each day:

```markdown
## Day N Review

### Completion status
| Task | Status | Notes |
|------|--------|-------|
| Task 1 | ✅ done | |
| Task 2 | ⚠️ partial | reason: ... |
| Task 3 | ❌ not started | blocker: ... |

### Code quality
- New lines of code: N
- Test pass rate: N%
- Known issues: [list]

### Tomorrow's plan
- [top-priority task]

### Technical notes
- [record key learnings of the day]
```

### 5. Milestone review

Run a milestone review every 3–5 days:

```markdown
## Milestone M: {{MILESTONE_NAME}}

### Goal completion
- [x] sub-goal 1
- [ ] sub-goal 2 (70% progress)
- [ ] sub-goal 3 (not started)

### Overall progress: N%

### Need to adjust the plan? Yes/No
### Adjustments: ...
```

### 6. Refactor window

Schedule a refactor every 5 iteration days:
- Review code complexity
- Eliminate technical debt
- Optimise performance hotspots
- Update documentation

## Work log format

All logs live under `docs/daily-logs/`:

```
docs/daily-logs/
├── day-001.md
├── day-002.md
├── ...
└── milestone-1-review.md
```

## Self-Test

> Validate the document generation and tracking mechanism of the daily iteration workflow.

### Self-test steps

```bash
# Test 1: docs directory can be created
mkdir -p /tmp/__selftest_daily__/docs/daily-logs && \
  echo "SELF_TEST_PASS: docs_dir" || echo "SELF_TEST_FAIL: docs_dir"

# Test 2: Markdown template renders
cat > /tmp/__selftest_daily__/docs/daily-logs/day-001.md << 'PLAN'
## Day 1 Plan
### Yesterday recap
- Completed: project init
### Today's goals
1. [x] scaffold the project
2. [ ] implement WiFi connection
### Acceptance checkpoints
- [x] compile passes
PLAN
grep -c '\[x\]' /tmp/__selftest_daily__/docs/daily-logs/day-001.md | \
  xargs -I{} bash -c '[ {} -ge 1 ] && echo "SELF_TEST_PASS: plan_format" || echo "SELF_TEST_FAIL: plan_format"'

# Test 3: git available for commit tracking
command -v git &>/dev/null && echo "SELF_TEST_PASS: git_available" || echo "SELF_TEST_FAIL: git_available"

rm -rf /tmp/__selftest_daily__
```

### Blind Test

**Test prompt:**
```
You are an AI development assistant. Read this skill, then for a project named "test-project"
produce a complete Day 1 working plan document including:
- Morning plan (3 concrete goals)
- Execution record template
- Evening review template
The project goal is "nRF5340 SPI loopback + BLE Heart Rate clean build".
Output the complete Markdown document.
```

**Acceptance criteria:**
- [ ] The agent produces a document with all three sections
- [ ] Goals are concrete and actionable (not vague)
- [ ] The document uses checkbox format
- [ ] The agent references the template in this skill

## Success Criteria

- [ ] Daily plan and review documents are produced
- [ ] Task execution has corresponding test verification
- [ ] Git commits stay aligned with task completion
- [ ] Milestones are reviewed on schedule
