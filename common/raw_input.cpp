#include "raw_input.h"

#include <cppitertools\enumerate.hpp>
#include <cassert>

using namespace dx11_lessons;

namespace
{
	constexpr auto use_buffered_raw_input = true;
	constexpr auto page_id = 0x01; 
	constexpr auto usage_id = std::array{
		0x06, // keyboard
		0x02, // mouse
	};
	constexpr auto raw_device_desc = std::array{
		RAWINPUTDEVICE{ page_id, usage_id[static_cast<int>(input_device::keyboard)] },
		RAWINPUTDEVICE{ page_id, usage_id[static_cast<int>(input_device::mouse)] },
	};

	auto translate_to_button(uint16_t vKey, uint16_t sCode, uint16_t flags) -> input_button
	{
		// return if the Key is out side of our enumeration range
		if (vKey > static_cast<uint16_t>(input_button::oem_clear) ||
			vKey == static_cast<uint16_t>(input_button::none))
		{
			return input_button::none;
		}

		// figure out which key was press, in cases where there are duplicates (e.g numpad)
		const bool isE0 = ((flags & RI_KEY_E0) != 0);
		const bool isE1 = ((flags & RI_KEY_E1) != 0);

		switch (static_cast<input_button>(vKey))
		{
			case input_button::pause:
				sCode = (isE1) ? 0x45 : MapVirtualKey(vKey, MAPVK_VK_TO_VSC);
				// What happens here????
				break;
			case input_button::shift:
				vKey = MapVirtualKey(sCode, MAPVK_VSC_TO_VK_EX);
				break;
			case input_button::control:
				return ((isE0) ? input_button::right_control : input_button::left_control);
				
			case input_button::alt:
				return ((isE0) ? input_button::right_alt : input_button::left_alt);
				
			case input_button::enter:
				return ((isE0) ? input_button::separator : input_button::enter);
				
			case input_button::insert:
				return ((!isE0) ? input_button::num_pad_0 : input_button::insert);
				
			case input_button::del:
				return ((!isE0) ? input_button::decimal : input_button::del);
				
			case input_button::home:
				return ((!isE0) ? input_button::num_pad_7 : input_button::home);
				
			case input_button::end:
				return ((!isE0) ? input_button::num_pad_1 : input_button::end);
				
			case input_button::prior:
				return ((!isE0) ? input_button::num_pad_9 : input_button::prior);
				
			case input_button::next:
				return ((!isE0) ? input_button::num_pad_3 : input_button::next);
				
			case input_button::left_arrow:
				return ((!isE0) ? input_button::num_pad_4 : input_button::left_arrow);
				
			case input_button::right_arrow:
				return ((!isE0) ? input_button::num_pad_6 : input_button::right_arrow);
				
			case input_button::up_arrow:
				return ((!isE0) ? input_button::num_pad_8 : input_button::up_arrow);
				
			case input_button::down_arrow:
				return ((!isE0) ? input_button::num_pad_2 : input_button::down_arrow);
				
			case input_button::clear:
				return ((!isE0) ? input_button::num_pad_5 : input_button::clear);
				
		}

		return static_cast<input_button>(vKey);
	}

	auto translate_to_button(uint16_t btnFlags) -> input_button
	{
		auto btn = input_button::none;

		// Which button was pressed?
		if ((btnFlags & RI_MOUSE_BUTTON_1_DOWN) or (btnFlags & RI_MOUSE_BUTTON_1_UP))
		{
			btn = input_button::left_button; // MK_LBUTTON;
		}
		else if ((btnFlags & RI_MOUSE_BUTTON_2_DOWN) or (btnFlags & RI_MOUSE_BUTTON_2_UP))
		{
			btn = input_button::right_button; // MK_RBUTTON;
		}
		else if ((btnFlags & RI_MOUSE_BUTTON_3_DOWN) or (btnFlags & RI_MOUSE_BUTTON_3_UP))
		{
			btn = input_button::middle_button; // MK_MBUTTON;
		}
		else if ((btnFlags & RI_MOUSE_BUTTON_4_DOWN) or (btnFlags & RI_MOUSE_BUTTON_4_UP))
		{
			btn = input_button::extra_button_1; // MK_XBUTTON1;
		}
		else if ((btnFlags & RI_MOUSE_BUTTON_5_DOWN) or (btnFlags & RI_MOUSE_BUTTON_5_UP))
		{
			btn = input_button::extra_button_2; // MK_XBUTTON2;
		}

		return btn;
	}
}

raw_input::raw_input(HWND hwnd, const std::vector<input_device> &devices) :
	hWnd(hwnd)
{
	auto rid = std::vector<RAWINPUTDEVICE>{};

	for (auto device : devices)
	{
		rid.push_back(raw_device_desc.at(static_cast<int>(device)));
		rid.back().hwndTarget = hWnd;
	}

	auto result = ::RegisterRawInputDevices(rid.data(),
	                                        static_cast<uint32_t>(rid.size()),
	                                        sizeof(RAWINPUTDEVICE));
	assert(result == TRUE);
}

raw_input::~raw_input() = default;

void raw_input::process_messages()
{
	auto input_msg_count = 0u;

	if constexpr (use_buffered_raw_input)
	{
		// Use Buffered Raw Input
		// This means no using WM_INPUT in windows message proc.

		auto raw_input_buffer_size = static_cast<uint32_t>(input_buffer.size() * sizeof(RAWINPUT));
		input_msg_count = GetRawInputBuffer(&input_buffer.at(0),
		                                    &raw_input_buffer_size,
		                                    sizeof(RAWINPUTHEADER));

		assert(input_msg_count >= 0); // if asserted issues with getting data
	}
	else
	{
		// Use Unbuffered Raw Input
		// use PeekMessage restricted to WM_INPUT to get all the RAWINPUT data

		auto msg = MSG{};
		auto has_more_messages = BOOL{ TRUE };

		while (has_more_messages == TRUE && input_msg_count < input_buffer.size())
		{
			has_more_messages = PeekMessage(&msg, hWnd, WM_INPUT, WM_INPUT, PM_NOYIELD | PM_REMOVE);

			if (has_more_messages == FALSE)
			{
				continue;
			}

			auto raw_input_size = static_cast<uint32_t>(sizeof(RAWINPUT));
			auto bytes_copied = GetRawInputData(reinterpret_cast<HRAWINPUT>(msg.lParam),
			                                    RID_INPUT,
			                                    &input_buffer[input_msg_count],
			                                    &raw_input_size,
			                                    sizeof(RAWINPUTHEADER));
			assert(bytes_copied >= 0); // if asserted issues with copying data

			input_msg_count++;
		}
	}

	process_input(input_msg_count);
}

auto raw_input::is_button_down(input_button button) const -> bool
{
	return buttons_down.at(static_cast<uint8_t>(button));
}

auto raw_input::get_axis_value(input_axis axis) const -> int16_t
{
	return get_axis_value(axis, false);
}

auto raw_input::get_axis_value(input_axis axis, bool absolute) const -> int16_t
{
	if (absolute)
	{
		return axis_values_absolute.at(static_cast<uint8_t>(axis));
	}

	return axis_values_relative.at(static_cast<uint8_t>(axis));
}

void raw_input::process_input(uint32_t count)
{
	// Reset relative positions for all axis
	axis_values_relative.fill(0);

	for (auto &&[i, raw_data] : input_buffer
	                          | iter::enumerate)
	{
		if (i >= count)
			break;

		switch (raw_data.header.dwType)
		{
			case RIM_TYPEKEYBOARD:
				process_keyboard_input(raw_data.data.keyboard);
				break;
			case RIM_TYPEMOUSE:
				process_mouse_input(raw_data.data.mouse);
				break;
			default:
				assert(false);
				break;
		}
	}
}

void raw_input::process_keyboard_input(const RAWKEYBOARD &data)
{
	auto vKey = data.VKey;
	auto sCode = data.MakeCode;
	auto flags = data.Flags;
	auto kState = false;

	if (!(flags & RI_KEY_BREAK))
	{
		kState = true;
	}

	auto button = translate_to_button(vKey, sCode, flags);
	vKey = static_cast<uint16_t>(button);

	// TODO: Figure out what new key states [up, down, pressed]

	// Is this key a toggle key? if so change toggle state
	if (button == input_button::caps_lock
	    or button == input_button::num_lock
	    or button == input_button::scroll_lock)
	{
		kState = !buttons_down[vKey];
	}

	// Update the Keyboard state array
	buttons_down[vKey] = kState;

	// Update the Keyboard state where there are duplicate 
	// i.e Shift, Ctrl, and Alt
	switch (button)
	{
		case input_button::left_shift:
		case input_button::right_shift:
			buttons_down[static_cast<uint16_t>(input_button::shift)] = kState;
			break;
		case input_button::left_control:
		case input_button::right_control:
			buttons_down[static_cast<uint16_t>(input_button::control)] = kState;
			break;
		case input_button::left_alt:
		case input_button::right_alt:
			buttons_down[static_cast<uint16_t>(input_button::alt)] = kState;
			break;
	}
}

void raw_input::process_mouse_input(const RAWMOUSE &data)
{
	auto btnFlags = data.usButtonFlags;

	auto button = translate_to_button(btnFlags);
	auto vBtn = static_cast<uint16_t>(button);

	// What is the button state?
	bool btnState{ false };
	if ((btnFlags & RI_MOUSE_BUTTON_1_DOWN)
	    || (btnFlags & RI_MOUSE_BUTTON_2_DOWN)
	    || (btnFlags & RI_MOUSE_BUTTON_3_DOWN)
	    || (btnFlags & RI_MOUSE_BUTTON_4_DOWN)
	    || (btnFlags & RI_MOUSE_BUTTON_5_DOWN))
	{
		btnState = true;
	}
	// TODO: Figure out what new key states [up, down, pressed]
	buttons_down[vBtn] = btnState;


	int32_t xPos = data.lLastX;
	int32_t yPos = data.lLastY;
	int16_t rWheel = data.usButtonData;


	if (data.usFlags & MOUSE_MOVE_ABSOLUTE)
	{
		// Update Axis values
		axis_values_relative[static_cast<int8_t>(input_axis::x)] = xPos;
		axis_values_relative[static_cast<int8_t>(input_axis::y)] = yPos;
		// Update Absolute values for axis
		axis_values_absolute[static_cast<int8_t>(input_axis::x)] = xPos;
		axis_values_absolute[static_cast<int8_t>(input_axis::y)] = yPos;
	}
	else
	{
		// Update Axis values
		axis_values_relative[static_cast<int8_t>(input_axis::x)] += xPos;
		axis_values_relative[static_cast<int8_t>(input_axis::y)] += yPos;
		// Update Absolute values for axis
		axis_values_absolute[static_cast<int8_t>(input_axis::x)] += xPos;
		axis_values_absolute[static_cast<int8_t>(input_axis::y)] += yPos;
	}

	// Vertical wheel data
	if (btnFlags & RI_MOUSE_WHEEL)
	{
		axis_values_relative[static_cast<int8_t>(input_axis::rx)] += rWheel;
		axis_values_absolute[static_cast<int8_t>(input_axis::rx)] += rWheel;
	}

	// Horizontal wheel data 
	if (btnFlags & RI_MOUSE_HWHEEL)
	{
		axis_values_relative[static_cast<int8_t>(input_axis::ry)] += rWheel;
		axis_values_absolute[static_cast<int8_t>(input_axis::ry)] += rWheel;
	}
}
