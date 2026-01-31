/*
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "AssignKeyZones.hpp"

QMap<int, QString> assignKeyZones(const QString &layout)
{
  QMap<int, QString> mappings;
  
  if ( layout == "de" || layout == "deutsch" )
  {
    // QWERTZ layout (German)
    mappings[0] = "Strg"; // Ctrl
    mappings[1] = "Unbekannt"; // Unused/Unknown
    mappings[2] = "Fn";
    mappings[3] = "⊞";
    mappings[4] = "Alt";
    mappings[5] = "Unbekannt";
    mappings[6] = "Unbekannt";
    mappings[7] = "Leertaste"; // Space
    mappings[8] = "Unbekannt";
    mappings[9] = "Unbekannt";
    mappings[10] = "Alt";
    mappings[11] = "Unbekannt";
    mappings[12] = "Strg";
    mappings[13] = "←";
    mappings[14] = "↑";
    mappings[15] = "→";
    mappings[16] = "0";
    mappings[17] = ",";
    mappings[18] = "↓";
    mappings[19] = "Unbekannt";
    mappings[20] = "Unbekannt";
    mappings[21] = "Unbekannt";
    mappings[22] = "⇧"; // Shift
    mappings[23] = "<";
    mappings[24] = "Y";
    mappings[25] = "X";
    mappings[26] = "C";
    mappings[27] = "V";
    mappings[28] = "B";
    mappings[29] = "N";
    mappings[30] = "M";
    mappings[31] = ",";
    mappings[32] = ".";
    mappings[33] = "-";
    mappings[34] = "Unbekannt";
    mappings[35] = "⇧";
    mappings[36] = "1";
    mappings[37] = "2";
    mappings[38] = "3";
    mappings[39] = "↵"; // Enter
    mappings[40] = "Unbekannt";
    mappings[41] = "Unbekannt";
    mappings[42] = "Feststelltaste"; // Caps
    mappings[43] = "Unbekannt";
    mappings[44] = "A";
    mappings[45] = "S";
    mappings[46] = "D";
    mappings[47] = "F";
    mappings[48] = "G";
    mappings[49] = "H";
    mappings[50] = "J";
    mappings[51] = "K";
    mappings[52] = "L";
    mappings[53] = "Ö";
    mappings[54] = "Ä";
    mappings[55] = "#";
    mappings[56] = "Unbekannt";
    mappings[57] = "4";
    mappings[58] = "5";
    mappings[59] = "6";
    mappings[60] = "Unbekannt";
    mappings[61] = "Unbekannt";
    mappings[62] = "Unbekannt";
    mappings[63] = "Tab";
    mappings[64] = "Unbekannt";
    mappings[65] = "Q";
    mappings[66] = "W";
    mappings[67] = "E";
    mappings[68] = "R";
    mappings[69] = "T";
    mappings[70] = "Z";
    mappings[71] = "U";
    mappings[72] = "I";
    mappings[73] = "O";
    mappings[74] = "P";
    mappings[75] = "Ü";
    mappings[76] = "+";
    mappings[77] = "↵";
    mappings[78] = "7";
    mappings[79] = "8";
    mappings[80] = "9";
    mappings[81] = "+";
    mappings[82] = "Unbekannt";
    mappings[83] = "Unbekannt";
    mappings[84] = "^";
    mappings[85] = "1";
    mappings[86] = "2";
    mappings[87] = "3";
    mappings[88] = "4";
    mappings[89] = "5";
    mappings[90] = "6";
    mappings[91] = "7";
    mappings[92] = "8";
    mappings[93] = "9";
    mappings[94] = "0";
    mappings[95] = "ß";
    mappings[96] = "'";
    mappings[97] = "Unbekannt";
    mappings[98] = "⌫"; // Backspace
    mappings[99] = "Num";
    mappings[100] = "/";
    mappings[101] = "*";
    mappings[102] = "-";
    mappings[103] = "Unbekannt";
    mappings[104] = "Unbekannt";
    mappings[105] = "Esc";
    mappings[106] = "F1";
    mappings[107] = "F2";
    mappings[108] = "F3";
    mappings[109] = "F4";
    mappings[110] = "F5";
    mappings[111] = "F6";
    mappings[112] = "F7";
    mappings[113] = "F8";
    mappings[114] = "F9";
    mappings[115] = "F10";
    mappings[116] = "F11";
    mappings[117] = "F12";
    mappings[118] = "Druck"; // PrtSc
    mappings[119] = "Einfg"; // Ins
    mappings[120] = "Entf"; // Del
    mappings[121] = "Pos1"; // Home
    mappings[122] = "Ende"; // End
    mappings[123] = "Bild↑"; // PgUp
    mappings[124] = "Bild↓"; // PgDn
    mappings[125] = "Unbekannt";
  }
  else
  {
    // Default to QWERTY (US/English)
    mappings[0] = "Ctrl";
    mappings[1] = "Unused/Unknown";
    mappings[2] = "Fn";
    mappings[3] = "⊞";
    mappings[4] = "Alt";
    mappings[5] = "Unused/Unknown";
    mappings[6] = "Unused/Unknown";
    mappings[7] = "Space";
    mappings[8] = "Unused/Unknown";
    mappings[9] = "Unused/Unknown";
    mappings[10] = "Alt";
    mappings[11] = "Unused/Unknown";
    mappings[12] = "Ctrl";
    mappings[13] = "←";
    mappings[14] = "↑";
    mappings[15] = "→";
    mappings[16] = "0";
    mappings[17] = ",";
    mappings[18] = "↓";
    mappings[19] = "Unused/Unknown";
    mappings[20] = "Unused/Unknown";
    mappings[21] = "Unused/Unknown";
    mappings[22] = "⇧";
    mappings[23] = "<";
    mappings[24] = "Z"; // Swapped with Y
    mappings[25] = "X";
    mappings[26] = "C";
    mappings[27] = "V";
    mappings[28] = "B";
    mappings[29] = "N";
    mappings[30] = "M";
    mappings[31] = ",";
    mappings[32] = ".";
    mappings[33] = "-";
    mappings[34] = "Unused/Unknown";
    mappings[35] = "⇧";
    mappings[36] = "1";
    mappings[37] = "2";
    mappings[38] = "3";
    mappings[39] = "↵";
    mappings[40] = "Unused/Unknown";
    mappings[41] = "Unused/Unknown";
    mappings[42] = "Caps";
    mappings[43] = "Unused/Unknown";
    mappings[44] = "A";
    mappings[45] = "S";
    mappings[46] = "D";
    mappings[47] = "F";
    mappings[48] = "G";
    mappings[49] = "H";
    mappings[50] = "J";
    mappings[51] = "K";
    mappings[52] = "L";
    mappings[53] = ";"; // Instead of Ö
    mappings[54] = "'"; // Instead of Ä
    mappings[55] = "#";
    mappings[56] = "Unused/Unknown";
    mappings[57] = "4";
    mappings[58] = "5";
    mappings[59] = "6";
    mappings[60] = "Unused/Unknown";
    mappings[61] = "Unused/Unknown";
    mappings[62] = "Unused/Unknown";
    mappings[63] = "Tab";
    mappings[64] = "Unused/Unknown";
    mappings[65] = "Q";
    mappings[66] = "W";
    mappings[67] = "E";
    mappings[68] = "R";
    mappings[69] = "T";
    mappings[70] = "Y"; // Swapped with Z
    mappings[71] = "U";
    mappings[72] = "I";
    mappings[73] = "O";
    mappings[74] = "P";
    mappings[75] = "["; // Instead of Ü
    mappings[76] = "]"; // Instead of +
    mappings[77] = "↵";
    mappings[78] = "7";
    mappings[79] = "8";
    mappings[80] = "9";
    mappings[81] = "+";
    mappings[82] = "Unused/Unknown";
    mappings[83] = "Unused/Unknown";
    mappings[84] = "`"; // Instead of ^
    mappings[85] = "1";
    mappings[86] = "2";
    mappings[87] = "3";
    mappings[88] = "4";
    mappings[89] = "5";
    mappings[90] = "6";
    mappings[91] = "7";
    mappings[92] = "8";
    mappings[93] = "9";
    mappings[94] = "0";
    mappings[95] = "-"; // Instead of ß
    mappings[96] = "="; // Instead of '
    mappings[97] = "Unused/Unknown";
    mappings[98] = "⌫";
    mappings[99] = "Num";
    mappings[100] = "/";
    mappings[101] = "*";
    mappings[102] = "-";
    mappings[103] = "Unused/Unknown";
    mappings[104] = "Unused/Unknown";
    mappings[105] = "Esc";
    mappings[106] = "F1";
    mappings[107] = "F2";
    mappings[108] = "F3";
    mappings[109] = "F4";
    mappings[110] = "F5";
    mappings[111] = "F6";
    mappings[112] = "F7";
    mappings[113] = "F8";
    mappings[114] = "F9";
    mappings[115] = "F10";
    mappings[116] = "F11";
    mappings[117] = "F12";
    mappings[118] = "PrtSc";
    mappings[119] = "Ins";
    mappings[120] = "Del";
    mappings[121] = "Home";
    mappings[122] = "End";
    mappings[123] = "PgUp";
    mappings[124] = "PgDn";
    mappings[125] = "Unused/Unknown";
  }
  
  return mappings;
}