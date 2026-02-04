# GTK4 Calculator (C)

## Project Structure
- `main.c`: Entry point of the application.
- `src/ui.c`: Builds the GTK interface, buttons, and interaction logic.
- `src/calc_eval.c` + `include/calc_eval.h`: Expression evaluation engine and functions.
- `src/style_manager.c` + `include/style_manager.h`: Loads CSS files and manages system theme (light/dark).
- `assets/dark.css` and `assets/light.css`: Application appearance.

## Quick Editing
- To change colors, sizes, or style: edit `assets/dark.css` and `assets/light.css`.
- To change calculation logic or add new functions: edit `src/calc_eval.c`.
- To change button layout or interface: edit `src/ui.c`.

## Build and Run
Build the application:
```bash
make clean && make
```