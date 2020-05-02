#pragma once

#include <Windows.h>
#include <cstdint>
#include <vector>
#include <array>

namespace dx11_lessons
{
	enum class input_device
	{
		keyboard,
		mouse,
	};

	enum class input_button : uint8_t;
	enum class input_axis : uint8_t;

	class raw_input
	{
	public:
		raw_input() = delete;
		raw_input(HWND hWnd, const std::vector<input_device> &devices);
		~raw_input();

		void process_messages();

		auto is_button_down(input_button button) const -> bool;
		auto get_axis_value(input_axis axis) const -> int16_t;
		auto get_axis_value(input_axis axis, bool absolute) const -> int16_t;

	private:
		void process_input(uint32_t count);
		void process_keyboard_input(const RAWKEYBOARD &data);
		void process_mouse_input(const RAWMOUSE &data);

	private:
		HWND hWnd;

		alignas(8)
			std::array<RAWINPUT, 128> input_buffer{};

		std::array<bool, 256> buttons_down{};
		std::array<int16_t, 5> axis_values_relative{},
		                       axis_values_absolute{};
	};

	enum class input_button : uint8_t
	{
		none              = 0x00, // No key was pressed
		left_button       = 0x01, // Left mouse button
		right_button      = 0x02, // Right mouse button
		cancel            = 0x03, // Cancel key
		middle_button     = 0x04, // Middle mouse button
		extra_button_1    = 0x05, // X1 mouse button
		extra_button_2    = 0x06, // X2 mouse button
		//                  0x07 is undefined
		back              = 0x08,
		tab               = 0x09,
		//                  0x0A - 0B are reserved
		clear             = 0x0c, // The CLEAR key
		enter             = 0x0d, // The Enter key
		//                  0x0E - 0F are undefined
		shift             = 0x10, // The Shift key
		control           = 0x11, // The Ctrl key
		alt               = 0x12, // The Alt key
		pause             = 0x13, // The Pause key
		caps_lock         = 0x14, // The Caps Lock key
		kana_mode         = 0x15, // IMI Kana mode
		hanguel_mode      = 0x15, // IMI Hanguel mode (Use HangulMode)
		hangul_mode       = 0x15, // IMI Hangul mode
		//                  0x16 is undefined
		junja_mode        = 0x17, // IMI Janja mode
		jinal_mode        = 0x18, // IMI Final mode
		hanja_mode        = 0x19, // IMI Hanja mode
		kanji_mode        = 0x19, // IMI Kanji mode
		//                  0x1A is undefined
		escape            = 0x1b, // The ESC key
		imi_convert       = 0x1c, // IMI convert key
		imi_no_convert    = 0x1d, // IMI noconvert key
		imi_accept        = 0x1e, // IMI accept key
		imi_mode_change   = 0x1f, // IMI mode change key
		space             = 0x20, // The Space key
		prior             = 0x21, // The Page Up key
		pageUp            = 0x21, // The Page Up key
		next              = 0x22, // The Page Down key
		pageDown          = 0x22, // The Page Down key
		end               = 0x23, // The End key
		home              = 0x24, // The Home key
		left_arrow        = 0x25, // The Left arrow key
		up_arrow          = 0x26, // The Up arrow key
		right_arrow       = 0x27, // The Right arrow key
		down_arrow        = 0x28, // The Down arrow key
		select            = 0x29, // The Select key
		print             = 0x2a, // The Print key
		execute           = 0x2b, // The Execute key
		print_screen      = 0x2c, // The Print Screen key
		snapshot          = 0x2c, // The Print Screen key
		insert            = 0x2d, // The Insert key
		del               = 0x2e, // The Delete key
		help              = 0x2f, // The Help key
		D0                = 0x30, // 0
		D1                = 0x31, // 1
		D2                = 0x32, // 2
		D3                = 0x33, // 3
		D4                = 0x34, // 4
		D5                = 0x35, // 5
		D6                = 0x36, // 6
		D7                = 0x37, // 7
		D8                = 0x38, // 8
		D9                = 0x39, // 9
		//                  0x3A - 40 are undefined
		A                 = 0x41, // A
		B                 = 0x42, // B
		C                 = 0x43, // C
		D                 = 0x44, // D
		E                 = 0x45, // E
		F                 = 0x46, // F
		G                 = 0x47, // G
		H                 = 0x48, // H
		I                 = 0x49, // I
		J                 = 0x4a, // J
		K                 = 0x4b, // K
		L                 = 0x4c, // L
		M                 = 0x4d, // M
		N                 = 0x4e, // N
		O                 = 0x4f, // O
		P                 = 0x50, // P
		Q                 = 0x51, // Q
		R                 = 0x52, // R
		S                 = 0x53, // S
		T                 = 0x54, // T
		U                 = 0x55, // U
		V                 = 0x56, // V
		W                 = 0x57, // W
		X                 = 0x58, // X
		Y                 = 0x59, // Y
		Z                 = 0x5a, // Z
		left_win          = 0x5b, // Left Windows key
		right_win         = 0x5c, // Right Windows key
		apps              = 0x5d, // Apps key
		//                  0x5E is reserved
		sleep             = 0x5f, // The Sleep key
		num_pad_0         = 0x60, // The Numeric keypad 0 key
		num_pad_1         = 0x61, // The Numeric keypad 1 key
		num_pad_2         = 0x62, // The Numeric keypad 2 key
		num_pad_3         = 0x63, // The Numeric keypad 3 key
		num_pad_4         = 0x64, // The Numeric keypad 4 key
		num_pad_5         = 0x65, // The Numeric keypad 5 key
		num_pad_6         = 0x66, // The Numeric keypad 6 key
		num_pad_7         = 0x67, // The Numeric keypad 7 key
		num_pad_8         = 0x68, // The Numeric keypad 8 key
		num_pad_9         = 0x69, // The Numeric keypad 9 key
		multiply          = 0x6a, // The Multiply key
		add               = 0x6b, // The Add key
		separator         = 0x6c, // The Separator key
		subtract          = 0x6d, // The Subtract key
		decimal           = 0x6e, // The Decimal key
		divide            = 0x6f, // The Divide key
		F1                = 0x70, // The F1 key
		F2                = 0x71, // The F2 key
		F3                = 0x72, // The F3 key
		F4                = 0x73, // The F4 key
		F5                = 0x74, // The F5 key
		F6                = 0x75, // The F6 key
		F7                = 0x76, // The F7 key
		F8                = 0x77, // The F8 key
		F9                = 0x78, // The F9 key
		F10               = 0x79, // The F10 key
		F11               = 0x7a, // The F11 key
		F12               = 0x7b, // The F12 key
		F13               = 0x7c, // The F13 key
		F14               = 0x7d, // The F14 key
		F15               = 0x7e, // The F15 key
		F16               = 0x7f, // The F16 key
		F17               = 0x80, // The F17 key
		F18               = 0x81, // The F18 key
		F19               = 0x82, // The F19 key
		F20               = 0x83, // The F20 key
		F21               = 0x84, // The F21 key
		F22               = 0x85, // The F22 key
		F23               = 0x86, // The F23 key
		F24               = 0x87, // The F24 key
		//                  0x88 - 8f are unassigned
		num_lock          = 0x90, // The Num Lock key
		scroll_lock       = 0x91, // The Scroll Lock key
		//                  0x92 - 96 are OEM specific
		//                  0x97 - 9f are unassigned
		left_shift        = 0xa0, // The Left Shift key
		right_shift       = 0xa1, // The Right Shift key
		left_control      = 0xa2, // The Left Control key
		right_control     = 0xa3, // The Right Control key
		left_alt          = 0xa4, // The Left Alt key
		right_alt         = 0xa5, // The Right Alt key
		browser_back      = 0xa6, // The Browser Back key
		browser_forward   = 0xa7, // The Browser Forward key
		browser_refresh   = 0xa8, // The Browser Refresh key
		browser_stop      = 0xa9, // The Browser Stop key
		browser_search    = 0xaa, // The Browser Search key
		browser_favorites = 0xab, // The Browser Favorites key
		browser_home      = 0xac, // The Browser Home key
		volume_mute       = 0xad, // The Volume Mute key
		volume_down       = 0xae, // The Volume Down key
		volume_up         = 0xaf, // The Volume Up key
		media_next_track  = 0xb0, // The Next Track key
		media_previous_track = 0xb1, // The Previous Track key
		media_stop        = 0xb2, // The Stop Media key
		media_play_pause  = 0xb3, // The Play/Pause Media key
		launch_mail       = 0xb4, // The Start Mail key
		select_media      = 0xb5, // The Select Media key
		launch_application_1 = 0xb6, // The Launch Application 1 key.
		launch_application_2 = 0xb7, // The Launch Application 2 key.
		//                  0xB8 - B9 are reserved
		oem_semicolon     = 0xba, 
		oem_1             = 0xba, // Used for miscellaneous characters; it can vary by keyboard.  For the US standard keyboard, the ';:' key
		oem_plus          = 0xbb, // For any country/region, the '+' key
		oem_comma         = 0xbc, // For any country/region, the ',' key
		oem_minus         = 0xbd, // For any country/region, the '-' key
		oem_period        = 0xbe, // For any country/region, the '.' key
		oem_question      = 0xbf,
		oem_2             = 0xbf, // Used for miscellaneous characters; it can vary by keyboard. For the US standard keyboard, the '/?' key
		oem_tilde         = 0xc0,
		oem_3             = 0xc0, // Used for miscellaneous characters; it can vary by keyboard. For the US standard keyboard, the '`~' key
		//                  0xC1 - D7 are reserved
		//                  0xD8 - DA are unassigned
		oem_openBrackets  = 0xdb,
		oem_4             = 0xdb, // Used for miscellaneous characters; it can vary by keyboard. For the US standard keyboard, the '[{' key
		oem_pipe          = 0xdc,
		oem_5             = 0xdc, // Used for miscellaneous characters; it can vary by keyboard. For the US standard keyboard, the '\|' key
		oem_closeBrackets = 0xdd,
		oem_6             = 0xdd, // Used for miscellaneous characters; it can vary by keyboard. For the US standard keyboard, the ']}' key
		oem_quotes        = 0xde,
		oem_7             = 0xde, // Used for miscellaneous characters; it can vary by keyboard. For the US standard keyboard, the 'single-quote/double-quote' key
		oem_8             = 0xdf, // Used for miscellaneous characters; it can vary by keyboard.
		//                  0xE0 is reserved
		//                  0xE1 is OEM specific
		oem_backslash     = 0xe2,
		oem_102           = 0xe2, // Either the angle bracket key or the backslash key on the RT 102-key keyboard
		//                  0xE3 - E4 OEM specific
		process_key       = 0xe5, // IME Process key
		//                  0xE6 is OEM specific
		packet            = 0xe7, // Used to pass Unicode characters as if they were keystrokes. The Packet key value is the low word of a 32-bit virtual-key value used for non-keyboard input methods.
		//                  0xE8 is unassigned
		//                  0xE9 - F5 OEM specific
		attn              = 0xf6, // The Attn key
		cr_sel            = 0xf7, // The CrSel key
		ex_sel            = 0xf8, // The ExSel key
		erase_eof         = 0xf9, // The Erase EOF key
		play              = 0xfa, // The Play key
		zoom              = 0xfb, // The Zoom key
		no_name           = 0xfc, // Reserved
		pa_1              = 0xfd, // The PA1 key
		oem_clear         = 0xfe, // The Clear key
	};

	enum class input_axis : uint8_t
	{
		none,
		x,
		y,
		rx,
		ry,
	};
}