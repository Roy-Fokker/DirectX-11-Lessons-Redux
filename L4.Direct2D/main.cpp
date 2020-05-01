#ifdef _DEBUG
// CRT Memory Leak detection
#define _CRTDBG_MAP_ALLOC  
#include <stdlib.h>  
#include <crtdbg.h>
#endif

#include "window.h"
#include "clock.h"

#include "direct2d_text_cube.h"

auto main() -> int
{
#ifdef _DEBUG
	// Detects memory leaks upon program exit
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif

	using namespace dx11_lessons;

	constexpr int wnd_width{ 1280 },
	              wnd_height{ wnd_width * 10 / 16 };

	auto wnd = window(L"DirectX 11 Lesson: Direct2D",
	                  { wnd_width, wnd_height });

	auto dtc = direct2d_text_cube(wnd.handle());
	wnd.set_message_callback(window::message_type::keypress,
	                         [&](uintptr_t wParam, uintptr_t lParam) -> bool
	{
		return dtc.on_keypress(wParam, lParam);
	});
	wnd.set_message_callback(window::message_type::resize,
	                         [&](uintptr_t wParam, uintptr_t lParam) -> bool
	{
		return dtc.on_resize(wParam, lParam);
	});

	auto clk = game_clock();

	wnd.show();
	clk.reset();

	while (wnd.handle() and not dtc.exit())
	{
		clk.tick();
		wnd.process_messages();

		dtc.update(clk);
		dtc.render();
	}

	return 0;
}