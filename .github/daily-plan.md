# nRF5340_SPI_loopback — Daily Work Plan

## Project Goal

Interview technical task — implement a SPIM4 32MHz SPI loopback + BLE Heart Rate Service on the nRF5340 DK, and deliver a GitHub repository with a clean build.

---

## Day N Plan Template

### Morning planning

**Date**: YYYY-MM-DD
**Iteration day**: Day N

#### Yesterday recap
- Completed:
- Unfinished:
- Blockers:

#### Today's goals
1. [ ] Goal 1
2. [ ] Goal 2
3. [ ] Goal 3

#### Risks and dependencies
- 

---

### Execution log

#### Task 1: 
- Start time: 
- End time: 
- Build result: ✅/❌ (`west build` passed?)
- Notes: 

#### Task 2: 
- Start time: 
- End time: 
- Build result: ✅/❌
- Notes: 

---

### Evening review

#### Completion status
| Task | Status | Notes |
|------|--------|-------|
| Task 1 | | |
| Task 2 | | |

#### Code quality
- New lines of code: 
- Compiler warnings: 
- Git commits: 

#### Build verification checkpoints
- [ ] `west build --sysbuild` passes
- [ ] DTS overlay applied (spi4 configured correctly)
- [ ] Kconfig applied (SPI + BLE enabled)
- [ ] `merged.hex` present

#### Regression gate (mandatory every day)
- [ ] `bash verify-acceptance.sh` — every already-completed feature item PASSes
- [ ] No regressions (new features did not break previously-passing items)
- [ ] Regression check passed before the wrap-up commit

#### Top priorities for tomorrow
1. 
2. 

#### Technical notes
- 

---

## Milestone Tracking

### M1: Project skeleton + environment validation
- [ ] NCS toolchain ready
- [ ] Project directory structure complete
- [ ] `west build --cmake-only` passes

### M2: SPI Loopback implementation
- [ ] SPIM4 DTS overlay complete
- [ ] `spi_loopback.c/h` implemented
- [ ] Running in a dedicated thread
- [ ] Build passes

### M3: BLE Heart Rate implementation
- [ ] BLE stack configured
- [ ] `ble_hrs.c/h` implemented
- [ ] sysbuild + ipc_radio build passes

### M4: Integration + acceptance + release
- [ ] SPI + BLE integrated
- [ ] All P0 acceptance criteria pass
- [ ] README + LLM disclosure complete
- [ ] `verify-acceptance.sh` all PASS
- [ ] Git history cleaned up
- [ ] GitHub repo published
- [ ] Interviewer invited
- [ ] Fresh clone test passes

## Acceptance Criteria Progress

### P0 must-pass
- [ ] Clean build passes
- [ ] `merged.hex` present
- [ ] GitHub repo accessible
- [ ] LLM usage disclosure
- [ ] `.gitignore` correct
- [ ] DTS/Kconfig/sysbuild checks pass

### P1 strongly recommended
- [ ] ROM/RAM reports clean
- [ ] Zero warnings
- [ ] Consistent code style
- [ ] Clean git history

### P2 bonus
- [ ] bsim compile
- [ ] Twister integration
