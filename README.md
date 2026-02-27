# Pomodoro Widget

A focus timer that alternates between 25-minute work sessions and 5-minute breaks. Pause state and remaining time are preserved across application restarts.

## Requirements

- Linux
- Qt 6.2+ (Widgets)
- CMake 3.21+
- C++20 compiler
- [`widget-sdk`](https://github.com/duh-dashboard/widget-sdk) installed

## Build

```sh
cmake -S . -B build -DCMAKE_PREFIX_PATH=~/.local
cmake --build build
cmake --install build --prefix ~/.local
```

The plugin installs to `~/.local/lib/dashboard/plugins/`.

## License

GPL-3.0-or-later — see [LICENSE](LICENSE).
