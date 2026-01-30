#!/usr/bin/env python3
"""
Tuxedo Control Center Keyboard Zone Mapping Utility
Converts zone numbers to proper key labels in the keyboard visualizer
Keyboard Layout: German (QWERTZ)
Note: Y and Z positions are swapped compared to US QWERTY layout
"""

ZONE_TO_KEY = {
    0: "Left Ctrl",
    1: "Unused/Unknown",
    2: "Fn",
    3: "Left Windows",
    4: "Left Alt",
    5: "Unused/Unknown",
    6: "Unused/Unknown",
    7: "Space",
    8: "Unused/Unknown",
    9: "Unused/Unknown",
    10: "Right Alt",
    11: "Unused/Unknown",
    12: "Right Ctrl",
    13: "Left Arrow",
    14: "Up Arrow",
    15: "Right Arrow",
    16: "Numpad 0",
    17: "Numpad ,",
    18: "Down Arrow",
    19: "Unused/Unknown",
    20: "Unused/Unknown",
    21: "Unused/Unknown",
    22: "Left Shift",
    23: "<",
    24: "y",
    25: "x",
    26: "c",
    27: "v",
    28: "b",
    29: "n",
    30: "m",
    31: ",",
    32: ".",
    33: "-",
    34: "Unused/Unknown",
    35: "Right Shift",
    36: "Numpad 1",
    37: "Numpad 2",
    38: "Numpad 3",
    39: "Numpad Enter",
    40: "Unused/Unknown",
    41: "Unused/Unknown",
    42: "Caps Lock",
    43: "Unused/Unknown",
    44: "a",
    45: "s",
    46: "d",
    47: "f",
    48: "g",
    49: "h",
    50: "j",
    51: "k",
    52: "l",
    53: "ö",
    54: "ä",
    55: "#",
    56: "Unused/Unknown",
    57: "Numpad 4",
    58: "Numpad 5",
    59: "Numpad 6",
    60: "Unused/Unknown",
    61: "Unused/Unknown",
    62: "Unused/Unknown",
    63: "Tab",
    64: "Unused/Unknown",
    65: "q",
    66: "w",
    67: "e",
    68: "r",
    69: "t",
    70: "z",
    71: "u",
    72: "i",
    73: "o",
    74: "p",
    75: "ü",
    76: "+",
    77: "Enter",
    78: "Numpad 7",
    79: "Numpad 8",
    80: "Numpad 9",
    81: "Numpad +",
    82: "Unused/Unknown",
    83: "Unused/Unknown",
    84: "^",
    85: "1",
    86: "2",
    87: "3",
    88: "4",
    89: "5",
    90: "6",
    91: "7",
    92: "8",
    93: "9",
    94: "0",
    95: "ß",
    96: "'",
    97: "Unused/Unknown",
    98: "Backspace",
    99: "Num Lock",
    100: "Numpad /",
    101: "Numpad *",
    102: "Numpad -",
    103: "Unused/Unknown",
    104: "Unused/Unknown",
    105: "Esc",
    106: "F1",
    107: "F2",
    108: "F3",
    109: "F4",
    110: "F5",
    111: "F6",
    112: "F7",
    113: "F8",
    114: "F9",
    115: "F10",
    116: "F11",
    117: "F12",
    118: "Print Screen",
    119: "Insert",
    120: "Delete",
    121: "Home",
    122: "End",
    123: "Page Up",
    124: "Page Down",
    125: "Unused/Unknown",
}

def get_key_label(zone_id):
    """Get the key label for a given zone ID"""
    return ZONE_TO_KEY.get(zone_id, f"Zone {zone_id}")

def generate_keyboard_layout_code():
    """Generate C++ code for the keyboard layout with proper labels"""
    code_lines = []

    # This would need to be integrated into the actual keyboard layout generation
    # in KeyboardVisualizerWidget.cpp

    for zone in range(18):  # Assuming 18 zones based on current mapping
        key_label = get_key_label(zone)
        code_lines.append(f'  zoneId = currentZone++;')
        code_lines.append(f'  createKeyButton( zoneId, "{key_label}", row, col );')

    return '\n'.join(code_lines)

if __name__ == "__main__":
    print("Tuxedo Keyboard Zone Mappings (German QWERTZ Layout):")
    print("Note: Y and Z positions are swapped compared to US QWERTY")
    print()
    for zone, key in ZONE_TO_KEY.items():
        print(f"  Zone {zone}: {key}")

    print(f"\nTotal mapped zones: {len(ZONE_TO_KEY)}")
    print(f"Unused zones: {sum(1 for k, v in ZONE_TO_KEY.items() if 'Unused' in v)}")