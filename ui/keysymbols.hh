// This Source Code Form is licensed MPL-2.0: http://mozilla.org/MPL/2.0

/**
 * @file
 * @brief User interface keyboard symbols, see also Rapicorn::KeyValue.
 */

/* This file is based on X.Org, the respective copyright notes are reproduced
 * below. For updates, use:
 * http://cgit.freedesktop.org/xorg/proto/x11proto/plain/keysymdef.h
 * sed -e '/define/s/\b\(0[xX][0-9a-fA-F]\+\)\b/\1,/' -e 's/#define *\bXK_/KEY_/g' \
 *     -e 's/\b\(KEY_[A-Za-z0-9_]\+\) /\1 = /'        -e 's,\(\s\)/\*\(.*\)\s\+[*]/,\1///<\2,'
 */


/***********************************************************
Copyright 1987, 1994, 1998  The Open Group

Permission to use, copy, modify, distribute, and sell this software and its
documentation for any purpose is hereby granted without fee, provided that
the above copyright notice appear in all copies and that both that
copyright notice and this permission notice appear in supporting
documentation.

The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE OPEN GROUP BE LIABLE FOR ANY CLAIM, DAMAGES OR
OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the name of The Open Group shall
not be used in advertising or otherwise to promote the sale, use or
other dealings in this Software without prior written authorization
from The Open Group.


Copyright 1987 by Digital Equipment Corporation, Maynard, Massachusetts

                        All Rights Reserved

Permission to use, copy, modify, and distribute this software and its
documentation for any purpose and without fee is hereby granted,
provided that the above copyright notice appear in all copies and that
both that copyright notice and this permission notice appear in
supporting documentation, and that the name of Digital not be
used in advertising or publicity pertaining to distribution of the
software without specific, written prior permission.

DIGITAL DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING
ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL
DIGITAL BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR
ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,
ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
SOFTWARE.

******************************************************************/


/* TTY function keys, cleverly chosen to map to ASCII */
KEY_BackSpace =                     0xff08,  ///< Back space, back char
KEY_Tab =                           0xff09,
KEY_Linefeed =                      0xff0a,  ///< Linefeed, LF
KEY_Clear =                         0xff0b,
KEY_Return =                        0xff0d,  ///< Return, enter
KEY_Pause =                         0xff13,  ///< Pause, hold
KEY_Scroll_Lock =                   0xff14,
KEY_Sys_Req =                       0xff15,
KEY_Escape =                        0xff1b,
KEY_Delete =                        0xffff,  ///< Delete, rubout
/* International & multi-key character composition */
KEY_Multi_key =                     0xff20,  ///< Multi-key character compose
KEY_Codeinput =                     0xff37,
KEY_SingleCandidate =               0xff3c,
KEY_MultipleCandidate =             0xff3d,
KEY_PreviousCandidate =             0xff3e,
/* Cursor control & motion */
KEY_Home =                          0xff50,
KEY_Left =                          0xff51,  ///< Move left, left arrow
KEY_Up =                            0xff52,  ///< Move up, up arrow
KEY_Right =                         0xff53,  ///< Move right, right arrow
KEY_Down =                          0xff54,  ///< Move down, down arrow
KEY_Prior =                         0xff55,  ///< Prior, previous
KEY_Page_Up =                       0xff55,
KEY_Next =                          0xff56,  ///< Next
KEY_Page_Down =                     0xff56,
KEY_End =                           0xff57,  ///< EOL
KEY_Begin =                         0xff58,  ///< BOL
/* Misc functions */
KEY_Select =                        0xff60,  ///< Select, mark
KEY_Print =                         0xff61,
KEY_Execute =                       0xff62,  ///< Execute, run, do
KEY_Insert =                        0xff63,  ///< Insert, insert here
KEY_Undo =                          0xff65,
KEY_Redo =                          0xff66,  ///< Redo, again
KEY_Menu =                          0xff67,
KEY_Find =                          0xff68,  ///< Find, search
KEY_Cancel =                        0xff69,  ///< Cancel, stop, abort, exit
KEY_Help =                          0xff6a,  ///< Help
KEY_Break =                         0xff6b,
KEY_Mode_switch =                   0xff7e,  ///< Character set switch
KEY_script_switch =                 0xff7e,  ///< Alias for mode_switch
KEY_Num_Lock =                      0xff7f,
/* Keypad functions, keypad numbers cleverly chosen to map to ASCII */
KEY_KP_Space =                      0xff80,  ///< Space
KEY_KP_Tab =                        0xff89,
KEY_KP_Enter =                      0xff8d,  ///< Enter
KEY_KP_F1 =                         0xff91,  ///< PF1, KP_A, ...
KEY_KP_F2 =                         0xff92,
KEY_KP_F3 =                         0xff93,
KEY_KP_F4 =                         0xff94,
KEY_KP_Home =                       0xff95,
KEY_KP_Left =                       0xff96,
KEY_KP_Up =                         0xff97,
KEY_KP_Right =                      0xff98,
KEY_KP_Down =                       0xff99,
KEY_KP_Prior =                      0xff9a,
KEY_KP_Page_Up =                    0xff9a,
KEY_KP_Next =                       0xff9b,
KEY_KP_Page_Down =                  0xff9b,
KEY_KP_End =                        0xff9c,
KEY_KP_Begin =                      0xff9d,
KEY_KP_Insert =                     0xff9e,
KEY_KP_Delete =                     0xff9f,
KEY_KP_Equal =                      0xffbd,  ///< Equals
KEY_KP_Multiply =                   0xffaa,
KEY_KP_Add =                        0xffab,
KEY_KP_Separator =                  0xffac,  ///< Separator, often comma
KEY_KP_Subtract =                   0xffad,
KEY_KP_Decimal =                    0xffae,
KEY_KP_Divide =                     0xffaf,
KEY_KP_0 =                          0xffb0,
KEY_KP_1 =                          0xffb1,
KEY_KP_2 =                          0xffb2,
KEY_KP_3 =                          0xffb3,
KEY_KP_4 =                          0xffb4,
KEY_KP_5 =                          0xffb5,
KEY_KP_6 =                          0xffb6,
KEY_KP_7 =                          0xffb7,
KEY_KP_8 =                          0xffb8,
KEY_KP_9 =                          0xffb9,
/* Function Keys */
KEY_F1 =                            0xffbe,
KEY_F2 =                            0xffbf,
KEY_F3 =                            0xffc0,
KEY_F4 =                            0xffc1,
KEY_F5 =                            0xffc2,
KEY_F6 =                            0xffc3,
KEY_F7 =                            0xffc4,
KEY_F8 =                            0xffc5,
KEY_F9 =                            0xffc6,
KEY_F10 =                           0xffc7,
KEY_F11 =                           0xffc8,
KEY_F12 =                           0xffc9,
KEY_F13 =                           0xffca,
KEY_F14 =                           0xffcb,
KEY_F15 =                           0xffcc,
KEY_F16 =                           0xffcd,
KEY_F17 =                           0xffce,
KEY_F18 =                           0xffcf,
KEY_F19 =                           0xffd0,
KEY_F20 =                           0xffd1,
KEY_F21 =                           0xffd2,
KEY_F22 =                           0xffd3,
KEY_F23 =                           0xffd4,
KEY_F24 =                           0xffd5,
KEY_F25 =                           0xffd6,
KEY_F26 =                           0xffd7,
KEY_F27 =                           0xffd8,
KEY_F28 =                           0xffd9,
KEY_F29 =                           0xffda,
KEY_F30 =                           0xffdb,
KEY_F31 =                           0xffdc,
KEY_F32 =                           0xffdd,
KEY_F33 =                           0xffde,
KEY_F34 =                           0xffdf,
KEY_F35 =                           0xffe0,
/* Modifiers */
KEY_Shift_L =                       0xffe1,  ///< Left shift
KEY_Shift_R =                       0xffe2,  ///< Right shift
KEY_Control_L =                     0xffe3,  ///< Left control
KEY_Control_R =                     0xffe4,  ///< Right control
KEY_Caps_Lock =                     0xffe5,  ///< Caps lock
KEY_Shift_Lock =                    0xffe6,  ///< Shift lock
KEY_Meta_L =                        0xffe7,  ///< Left meta
KEY_Meta_R =                        0xffe8,  ///< Right meta
KEY_Alt_L =                         0xffe9,  ///< Left alt
KEY_Alt_R =                         0xffea,  ///< Right alt
KEY_Super_L =                       0xffeb,  ///< Left super
KEY_Super_R =                       0xffec,  ///< Right super
KEY_Hyper_L =                       0xffed,  ///< Left hyper
KEY_Hyper_R =                       0xffee,  ///< Right hyper
/* from Appendix C of "The X Keyboard Extension: Protocol Specification") */
KEY_ISO_Lock =                      0xfe01,
KEY_ISO_Level2_Latch =              0xfe02,
KEY_ISO_Level3_Shift =              0xfe03,
KEY_ISO_Level3_Latch =              0xfe04,
KEY_ISO_Level3_Lock =               0xfe05,
KEY_ISO_Level5_Shift =              0xfe11,
KEY_ISO_Level5_Latch =              0xfe12,
KEY_ISO_Level5_Lock =               0xfe13,
KEY_ISO_Group_Shift =               0xff7e,  ///< Alias for mode_switch
KEY_ISO_Group_Latch =               0xfe06,
KEY_ISO_Group_Lock =                0xfe07,
KEY_ISO_Next_Group =                0xfe08,
KEY_ISO_Next_Group_Lock =           0xfe09,
KEY_ISO_Prev_Group =                0xfe0a,
KEY_ISO_Prev_Group_Lock =           0xfe0b,
KEY_ISO_First_Group =               0xfe0c,
KEY_ISO_First_Group_Lock =          0xfe0d,
KEY_ISO_Last_Group =                0xfe0e,
KEY_ISO_Last_Group_Lock =           0xfe0f,
KEY_ISO_Left_Tab =                  0xfe20,
KEY_ISO_Move_Line_Up =              0xfe21,
KEY_ISO_Move_Line_Down =            0xfe22,
KEY_ISO_Partial_Line_Up =           0xfe23,
KEY_ISO_Partial_Line_Down =         0xfe24,
KEY_ISO_Partial_Space_Left =        0xfe25,
KEY_ISO_Partial_Space_Right =       0xfe26,
KEY_ISO_Set_Margin_Left =           0xfe27,
KEY_ISO_Set_Margin_Right =          0xfe28,
KEY_ISO_Release_Margin_Left =       0xfe29,
KEY_ISO_Release_Margin_Right =      0xfe2a,
KEY_ISO_Release_Both_Margins =      0xfe2b,
KEY_ISO_Fast_Cursor_Left =          0xfe2c,
KEY_ISO_Fast_Cursor_Right =         0xfe2d,
KEY_ISO_Fast_Cursor_Up =            0xfe2e,
KEY_ISO_Fast_Cursor_Down =          0xfe2f,
KEY_ISO_Continuous_Underline =      0xfe30,
KEY_ISO_Discontinuous_Underline =   0xfe31,
KEY_ISO_Emphasize =                 0xfe32,
KEY_ISO_Center_Object =             0xfe33,
KEY_ISO_Enter =                     0xfe34,
KEY_First_Virtual_Screen =          0xfed0,
KEY_Prev_Virtual_Screen =           0xfed1,
KEY_Next_Virtual_Screen =           0xfed2,
KEY_Last_Virtual_Screen =           0xfed4,
KEY_Terminate_Server =              0xfed5,
KEY_AccessX_Enable =                0xfe70,
KEY_AccessX_Feedback_Enable =       0xfe71,
KEY_RepeatKeys_Enable =             0xfe72,
KEY_SlowKeys_Enable =               0xfe73,
KEY_BounceKeys_Enable =             0xfe74,
KEY_StickyKeys_Enable =             0xfe75,
KEY_MouseKeys_Enable =              0xfe76,
KEY_MouseKeys_Accel_Enable =        0xfe77,
KEY_Overlay1_Enable =               0xfe78,
KEY_Overlay2_Enable =               0xfe79,
KEY_AudibleBell_Enable =            0xfe7a,
KEY_Pointer_Left =                  0xfee0,
KEY_Pointer_Right =                 0xfee1,
KEY_Pointer_Up =                    0xfee2,
KEY_Pointer_Down =                  0xfee3,
KEY_Pointer_UpLeft =                0xfee4,
KEY_Pointer_UpRight =               0xfee5,
KEY_Pointer_DownLeft =              0xfee6,
KEY_Pointer_DownRight =             0xfee7,
KEY_Pointer_Button_Dflt =           0xfee8,
KEY_Pointer_Button1 =               0xfee9,
KEY_Pointer_Button2 =               0xfeea,
KEY_Pointer_Button3 =               0xfeeb,
KEY_Pointer_Button4 =               0xfeec,
KEY_Pointer_Button5 =               0xfeed,
KEY_Pointer_DblClick_Dflt =         0xfeee,
KEY_Pointer_DblClick1 =             0xfeef,
KEY_Pointer_DblClick2 =             0xfef0,
KEY_Pointer_DblClick3 =             0xfef1,
KEY_Pointer_DblClick4 =             0xfef2,
KEY_Pointer_DblClick5 =             0xfef3,
KEY_Pointer_Drag_Dflt =             0xfef4,
KEY_Pointer_Drag1 =                 0xfef5,
KEY_Pointer_Drag2 =                 0xfef6,
KEY_Pointer_Drag3 =                 0xfef7,
KEY_Pointer_Drag4 =                 0xfef8,
KEY_Pointer_Drag5 =                 0xfefd,
KEY_Pointer_EnableKeys =            0xfef9,
KEY_Pointer_Accelerate =            0xfefa,
KEY_Pointer_DfltBtnNext =           0xfefb,
KEY_Pointer_DfltBtnPrev =           0xfefc,
/* ASCII & ISO/IEC 8859-1 = Unicode U+0020..U+00FF */
KEY_space =                         0x0020,  ///< U+0020 SPACE
KEY_exclam =                        0x0021,  ///< U+0021 EXCLAMATION MARK
KEY_quotedbl =                      0x0022,  ///< U+0022 QUOTATION MARK
KEY_numbersign =                    0x0023,  ///< U+0023 NUMBER SIGN
KEY_dollar =                        0x0024,  ///< U+0024 DOLLAR SIGN
KEY_percent =                       0x0025,  ///< U+0025 PERCENT SIGN
KEY_ampersand =                     0x0026,  ///< U+0026 AMPERSAND
KEY_apostrophe =                    0x0027,  ///< U+0027 APOSTROPHE
KEY_quoteright =                    0x0027,  ///< deprecated
KEY_parenleft =                     0x0028,  ///< U+0028 LEFT PARENTHESIS
KEY_parenright =                    0x0029,  ///< U+0029 RIGHT PARENTHESIS
KEY_asterisk =                      0x002a,  ///< U+002A ASTERISK
KEY_plus =                          0x002b,  ///< U+002B PLUS SIGN
KEY_comma =                         0x002c,  ///< U+002C COMMA
KEY_minus =                         0x002d,  ///< U+002D HYPHEN-MINUS
KEY_period =                        0x002e,  ///< U+002E FULL STOP
KEY_slash =                         0x002f,  ///< U+002F SOLIDUS
KEY_0 =                             0x0030,  ///< U+0030 DIGIT ZERO
KEY_1 =                             0x0031,  ///< U+0031 DIGIT ONE
KEY_2 =                             0x0032,  ///< U+0032 DIGIT TWO
KEY_3 =                             0x0033,  ///< U+0033 DIGIT THREE
KEY_4 =                             0x0034,  ///< U+0034 DIGIT FOUR
KEY_5 =                             0x0035,  ///< U+0035 DIGIT FIVE
KEY_6 =                             0x0036,  ///< U+0036 DIGIT SIX
KEY_7 =                             0x0037,  ///< U+0037 DIGIT SEVEN
KEY_8 =                             0x0038,  ///< U+0038 DIGIT EIGHT
KEY_9 =                             0x0039,  ///< U+0039 DIGIT NINE
KEY_colon =                         0x003a,  ///< U+003A COLON
KEY_semicolon =                     0x003b,  ///< U+003B SEMICOLON
KEY_less =                          0x003c,  ///< U+003C LESS-THAN SIGN
KEY_equal =                         0x003d,  ///< U+003D EQUALS SIGN
KEY_greater =                       0x003e,  ///< U+003E GREATER-THAN SIGN
KEY_question =                      0x003f,  ///< U+003F QUESTION MARK
KEY_at =                            0x0040,  ///< U+0040 COMMERCIAL AT
KEY_bracketleft =                   0x005b,  ///< U+005B LEFT SQUARE BRACKET
KEY_backslash =                     0x005c,  ///< U+005C REVERSE SOLIDUS
KEY_bracketright =                  0x005d,  ///< U+005D RIGHT SQUARE BRACKET
KEY_asciicircum =                   0x005e,  ///< U+005E CIRCUMFLEX ACCENT
KEY_underscore =                    0x005f,  ///< U+005F LOW LINE
KEY_grave =                         0x0060,  ///< U+0060 GRAVE ACCENT
KEY_quoteleft =                     0x0060,  ///< deprecated
KEY_braceleft =                     0x007b,  ///< U+007B LEFT CURLY BRACKET
KEY_bar =                           0x007c,  ///< U+007C VERTICAL LINE
KEY_braceright =                    0x007d,  ///< U+007D RIGHT CURLY BRACKET
KEY_asciitilde =                    0x007e,  ///< U+007E TILDE
KEY_nobreakspace =                  0x00a0,  ///< U+00A0 NO-BREAK SPACE
