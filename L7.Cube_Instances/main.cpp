#ifdef _DEBUG
// CRT Memory Leak detection
#define _CRTDBG_MAP_ALLOC  
#include <stdlib.h>  
#include <crtdbg.h>
#endif

#include "window.h"
#include "clock.h"
#include "raw_input.h"

#include "cube_instances.h"

auto main() -> int
{
#ifdef _DEBUG
	// Detects memory leaks upon program exit
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif

	using namespace dx11_lessons;

	constexpr int wnd_width{ 1280 },
	              wnd_height{ wnd_width * 10 / 16 };

	auto wnd = window(L"DirectX 11 Lesson: Instances of Cube",
	                  { wnd_width, wnd_height });

	auto ci = cube_instances(wnd.handle());

	wnd.set_message_callback(window::message_type::resize,
	                         [&](uintptr_t wParam, uintptr_t lParam) -> bool
	{
		return ci.on_resize(wParam, lParam);
	});

	auto clk = game_clock();
	auto input = raw_input(wnd.handle(), { input_device::keyboard, input_device::mouse });

	wnd.show();
	clk.reset();

	while (wnd.handle() and not ci.exit())
	{
		clk.tick();
		input.process_messages();
		wnd.process_messages();

		ci.update(clk, input);
		ci.render();
	}

	return 0;
}