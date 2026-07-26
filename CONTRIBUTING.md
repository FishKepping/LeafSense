# Contributing to LeafSense

Contributions are welcome.

LeafSense is an early-stage project, so discussion before a large implementation is strongly encouraged. Small fixes, tests, documentation, hardware observations, and reproducible bug reports are especially useful.

## Before contributing

Read:

- [Project charter](docs/PROJECT_CHARTER.md)
- [Architecture](docs/Architecture.md)
- [Roadmap](docs/Roadmap.md)
- [Development guide](docs/Development.md)
- [Testing guide](docs/Testing.md)

Check that the proposed work belongs to the active roadmap phase.

## Contribution workflow

1. Fork the repository.
2. Create a focused feature branch.
3. Make the smallest change that fully solves the problem.
4. Add or update tests.
5. Build with compiler warnings enabled.
6. Run all tests.
7. Update relevant documentation.
8. Add a concise entry under `Unreleased` in `CHANGELOG.md`.
9. Open a pull request with a clear explanation.

Suggested branch names:

```text
feature/esphome-bus-adapter
fix/interrupt-map-decoding
tests/telemetry-overflow
docs/driver-recovery
```

## Pull request description

Include:

- What changed.
- Why it changed.
- Which milestone or issue it supports.
- How it was tested.
- Whether hardware was used.
- Any remaining limitation.
- Any public API impact.

## Code standards

- Use C++17.
- Follow the existing formatting style.
- Keep hardware access behind interfaces.
- Do not introduce ESPHome dependencies into native core or driver logic.
- Prefer fixed-size containers for fixed-size sensor data.
- Avoid heap allocation in capture paths unless there is a measured need.
- Use descriptive identifiers.
- Document public APIs.
- Keep platform publication separate from sensor acquisition.
- Make errors and unavailable data explicit.
- Avoid unrelated refactoring in a feature pull request.

## Testing

A change should normally include tests for:

- Expected behavior.
- Important boundaries.
- Invalid input.
- Relevant bus or hardware failure.
- Recovery behavior where applicable.
- Availability and diagnostics where applicable.

Run:

```bash
cmake -S . -B build -DLEAFSENSE_BUILD_TESTS=ON
cmake --build build
ctest --test-dir build --output-on-failure
```

For Visual Studio:

```powershell
cmake --build build --config Debug
ctest --test-dir build -C Debug --output-on-failure
```

## Hardware changes

State clearly whether a change was:

- Native-test only.
- Compiled for ESP32.
- Tested with an ESP32.
- Tested with a physical AMG8833.
- Tested over an extended runtime.

Do not describe untested hardware behavior as verified.

## Documentation

Use Mermaid diagrams when they communicate structure or flow more clearly than a long paragraph.

Keep diagrams:

- Focused.
- Text-readable in GitHub.
- Consistent with actual class names.
- Clearly marked as planned when they describe future behavior.

## Commit guidance

Good commit messages explain behavior:

```text
Add flat telemetry projection for AMG8833 snapshots
Preserve frame availability when interrupt-map read fails
Document automatic recovery flow
```

Avoid vague messages:

```text
Updates
Fix stuff
Milestone
```

## Scope and compatibility

ESPHome is the primary platform. Reusability is encouraged where it remains simple.

Do not add a broad abstraction only for hypothetical future use. Add extension points when a real requirement demonstrates the need.

Once an API is used by the ESPHome platform layer or a release, changes should preserve compatibility where practical or include a migration note.

## License

By contributing, you agree that your contribution will be licensed under the project's MIT License.
