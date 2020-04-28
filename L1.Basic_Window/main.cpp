#ifdef _DEBUG
// CRT Memory Leak detection
#define _CRTDBG_MAP_ALLOC  
#include <stdlib.h>  
#include <crtdbg.h>
#endif

#include "window.h"
#include "clock.h"

#include "direct3d11.h"
#include "render_pass.h"

auto main() -> int
{
#ifdef _DEBUG
	// Detects memory leaks upon program exit
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif

	using namespace dx11_lessons;

	constexpr int wnd_width{ 1280 },
	              wnd_height{ wnd_width * 10 / 16 };

	auto wnd = window(L"DirectX 11 Lesson: Basic Window",
	                  { wnd_width, wnd_height });

	auto dx = direct3d11(wnd.handle());
	auto rp = render_pass(dx.get_device(), dx.get_swapchain());

	auto clk = game_clock();

	wnd.show();
	clk.reset();
	while (wnd.handle())
	{
		clk.tick();
		wnd.process_messages();

		rp.activate(dx.get_context());
		rp.clear(dx.get_context(), { 0.4f, 0.5f, 0.4f, 1.0f });

		dx.present(true);
	}

	return 0;
}