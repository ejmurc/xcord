# xcord

## Usage

Build the project:
```bash
cmake --preset release
cmake --build --preset release
```

The resulting executable will be available at `build/release/main`.

## Development

### Build Instructions
Debug build:

```bash
cmake --preset debug
cmake --build --preset debug
```

Rebuild:
```bash
cmake --build --preset debug
```

### Sanitizers

Enable AddressSanitizer and UndefinedBehaviorSanitizer:
```bash
cmake --build --preset debug -DENABLE_SANITIZERS=ON
cmake --build --preset debug
```

### Code Formatting
This project uses `clang-format` to enforce a consistent coding style.

Format all files:
```bash
cmake --build --preset debug --target format
```

Check formatting without modifying files:
```bash
cmake --build --preset debug --target format-check
```
