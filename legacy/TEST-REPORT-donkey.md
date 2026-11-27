# Donkey validation

- Two clean builds: passed.
- Static image checks: passed twice.
- Native host context-switch test: 1,000 scheduler runs per build, passed.
- SHA-256 comparison of kernel and image across builds: see build/test.log.
- QEMU: unavailable; boot, interrupts and scheduler in kernel mode remain unverified.
- Host test does not establish process isolation or preemptive scheduling.
