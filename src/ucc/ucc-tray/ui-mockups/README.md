UI Mockups for KDE tray application (3 tabs)

Files:
- `tray_dashboard.svg` — Dashboard tab mockup: compact gauges, power profile slider, battery card, quick actions.
- `tray_performance.svg` — Performance tab mockup: power profile selector, CPU/GPU power sliders, presets and Apply/Save.
- `tray_hardware.svg` — Hardware tab mockup: brightness slider, keyboard backlight, fan curve areas, Save/Revert/Apply buttons.

Notes & next steps:
- These are stylized sketches (dark KDE-friendly theme) to capture layout and interaction ideas.
- Next step: implement a QML prototype (Kirigami) using these layouts as a guide. I can scaffold QML components and a `TrayController` C++ wrapper to expose DBus properties.
- If you'd like, I can also create high-fidelity PNG exports and a short clickable prototype.

Design decisions:
- Keep the tray popup compact with 3 tabs to avoid overwhelming the user.
- Provide quick presets and an Apply button for performance changes to make actions explicit.
- Use editable curves for fan controls (replace with small `FanCurveEditor` component later).

Tell me which draft to refine (visual polish, spacing, or interaction flows) and I will proceed.