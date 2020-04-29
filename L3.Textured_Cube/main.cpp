#ifdef _DEBUG
// CRT Memory Leak detection
#define _CRTDBG_MAP_ALLOC  
#include <stdlib.h>  
#include <crtdbg.h>
#endif

#include "window.h"
#include "clock.h"

#include "draw_cube.h"

auto main() -> int
{
#ifdef _DEBUG
	// Detects memory leaks upon program exit
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif

	using namespace dx11_lessons;

	constexpr int wnd_width{ 1280 },
	              wnd_height{ wnd_width * 10 / 16 };

	auto wnd = window(L"DirectX 11 Lesson: Textured Cube",
	                  { wnd_width, wnd_height });

	auto dc = draw_cube(wnd.handle());
	wnd.set_message_callback(window::message_type::keypress,
	                         [&](uintptr_t wParam, uintptr_t lParam) -> bool
	{
		return dc.on_keypress(wParam, lParam);
	});
	wnd.set_message_callback(window::message_type::resize,
	                         [&](uintptr_t wParam, uintptr_t lParam) -> bool
	{
		return dc.on_resize(wParam, lParam);
	});

	auto clk = game_clock();

	wnd.show();
	clk.reset();

	while (wnd.handle() and not dc.exit())
	{
		clk.tick();
		wnd.process_messages();

		dc.update(clk);
		dc.render();
	}

	return 0;
}