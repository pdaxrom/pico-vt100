# TODO

## VT100 / VT52 features

- Implement real `ESC #3`, `ESC #4`, `ESC #5`, `ESC #6` double-height / double-width line modes.
- Implement `CSI q` (`DECLL`, LED control).
- Implement `CSI y` (self test).
- Implement private `DSR` replies such as `CSI ?6n` and other missing query/report paths.
- Expand `SM/RM` and private `DECSET/DECRST` coverage beyond the current subset:
  - `4`, `20`
  - `?1`, `?2`, `?5`, `?6`, `?7`, `?25`
- Decide whether any `OSC`, `DCS`, `SOS`, `PM`, `APC` sequences need semantic handling instead of skip-only behavior.
- Add 8-bit C1 control forms such as `CSI`=`0x9B`, `OSC`=`0x9D`, etc.
- Extend charset support beyond `US`, `UK`, `DEC Special`, `G0..G3`.
- Implement a real input encoder for `DECCKM` and keypad application mode instead of storing flags only.
- Add blink timer support for `SGR blink`.
- Add scrollback support.
- Add UTF-8 support.

## Validation

- Run `vttest` against the terminal and fix concrete incompatibilities.
- Validate behavior with real TUI programs:
  - `vi`
  - `less`
  - `dialog`
  - `top`
- Add on-device regression coverage on real hardware in addition to host-side parser tests.

## Current local work

- Review and commit the current local `DECSC/DECRC` mode-save changes and explicit `ESC #3/#4/#5/#6` safe no-op handling.

## Suggested order

- Add private `DSR` / mode query replies.
- Add input encoding for cursor and keypad application modes.
- Run `vttest` and iterate on the failures.
