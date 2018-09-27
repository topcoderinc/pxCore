/*

 pxCore Copyright 2005-2018 John Robinson

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

     http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.

*/

// pxKeycodes.h

#ifndef PX_KEYCODES_H
#define PX_KEYCODES_H

#define PX_KEY_BACKSPACE 8
#define PX_KEY_TAB 	9
#define PX_KEY_ENTER 	13
#define PX_KEY_SHIFT 	16
#define PX_KEY_CTRL 	17
#define PX_KEY_ALT 	18
#define PX_KEY_PAUSE 	19
#define PX_KEY_CAPSLOCK 	20
#define PX_KEY_ESCAPE 	27
#define PX_KEY_SPACE 	32
#define PX_KEY_PAGEUP 	33
#define PX_KEY_PAGEDOWN 	34
#define PX_KEY_END 	35
#define PX_KEY_HOME 	36
#define PX_KEY_LEFT 	37
#define PX_KEY_UP 	38
#define PX_KEY_RIGHT 	39
#define PX_KEY_DOWN 	40
#define PX_KEY_INSERT 	45
#define PX_KEY_DELETE 	46
#define PX_KEY_ZERO 	48
#define PX_KEY_ONE 	49
#define PX_KEY_TWO 	50
#define PX_KEY_THREE 	51
#define PX_KEY_FOUR 	52
#define PX_KEY_FIVE 	53
#define PX_KEY_SIX 	54
#define PX_KEY_SEVEN 	55
#define PX_KEY_EIGHT 	56
#define PX_KEY_NINE 	57
#define PX_KEY_A 	65
#define PX_KEY_B 	66
#define PX_KEY_C 	67
#define PX_KEY_D 	68
#define PX_KEY_E 	69
#define PX_KEY_F 	70
#define PX_KEY_G 	71
#define PX_KEY_H 	72
#define PX_KEY_I 	73
#define PX_KEY_J 	74
#define PX_KEY_K 	75
#define PX_KEY_L 	76
#define PX_KEY_M 	77
#define PX_KEY_N 	78
#define PX_KEY_O 	79
#define PX_KEY_P 	80
#define PX_KEY_Q 	81
#define PX_KEY_R 	82
#define PX_KEY_S 	83
#define PX_KEY_T 	84
#define PX_KEY_U 	85
#define PX_KEY_V 	86
#define PX_KEY_W 	87
#define PX_KEY_X 	88
#define PX_KEY_Y 	89
#define PX_KEY_Z 	90
#define PX_KEY_WINDOWKEY_LEFT	91
#define PX_KEY_WINDOWKEY_RIGHT 	92
#define PX_KEY_SELECT 	93
#define PX_KEY_NUMPAD0 	96
#define PX_KEY_NUMPAD1 	97
#define PX_KEY_NUMPAD2 	98
#define PX_KEY_NUMPAD3 	99
#define PX_KEY_NUMPAD4 	100
#define PX_KEY_NUMPAD5 	101
#define PX_KEY_NUMPAD6 	102
#define PX_KEY_NUMPAD7 	103
#define PX_KEY_NUMPAD8 	104
#define PX_KEY_NUMPAD9 	105
#define PX_KEY_MULTIPLY 	106
#define PX_KEY_ADD 	107
#define PX_KEY_SUBTRACT 	109
#define PX_KEY_DECIMAL 	110
#define PX_KEY_DIVIDE 	111
#define PX_KEY_F1 	112
#define PX_KEY_F2 	113
#define PX_KEY_F3 	114
#define PX_KEY_F4 	115
#define PX_KEY_F5 	116
#define PX_KEY_F6 	117
#define PX_KEY_F7 	118
#define PX_KEY_F8 	119
#define PX_KEY_F9 	120
#define PX_KEY_F10 	121
#define PX_KEY_F11 	122
#define PX_KEY_F12 	123
#define PX_KEY_NUMLOCK 	144
#define PX_KEY_SCROLLLOCK 	145
#define PX_KEY_SEMICOLON 	186
#define PX_KEY_EQUALS 	187
#define PX_KEY_COMMA 	188
#define PX_KEY_DASH 	189
#define PX_KEY_PERIOD 	190
#define PX_KEY_FORWARDSLASH 	191
#define PX_KEY_GRAVEACCENT 	192
#define PX_KEY_OPENBRACKET 	219
#define PX_KEY_BACKSLASH 	220
#define PX_KEY_CLOSEBRACKET 	221
#define PX_KEY_SINGLEQUOTE 	222
#define PX_KEY_PRINTSCREEN 	44
#define PX_KEY_FASTFORWARD 	223
#define PX_KEY_REWIND 	224
#define PX_KEY_PLAY 	226
#define PX_KEY_PLAYPAUSE 	227

// flags used in onMouseDown and onMouseUp
#define PX_LEFTBUTTON       1
#define PX_MIDDLEBUTTON     2
#define PX_RIGHTBUTTON      4

// flags used in onMouseDown, onMouseUp, onKeyDown, onKeyUp
#define PX_MOD_SHIFT        8
#define PX_MOD_CONTROL      16
#define PX_MOD_ALT          32
#define PX_MOD_COMMAND      64

// flag used in onKeyDown
#define PX_KEYDOWN_REPEAT   128

#define PX_KEY_YELLOW       403
#define PX_KEY_BLUE         404
#define PX_KEY_RED          405
#define PX_KEY_GREEN        406
#endif //PX_KEYCODES_H
