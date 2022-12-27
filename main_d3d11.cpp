#include <platform/d3d11/system/platform_d3d11.h>
#include "scene_app.h"
#include <platform/win32/system/window_win32.h>

unsigned int sceLibcHeapSize = 128*1024*1024;	// Sets up the heap area size as 128MiB.

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR pScmdline, int iCmdshow)
{
	// initialisation
	gef::PlatformD3D11 platform(hInstance, 960, 544, false, true);

	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();
	io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
	io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
	ImGuiStyle& style = ImGui::GetStyle();
	if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
	{
		style.WindowRounding = 0.0f;
		style.Colors[ImGuiCol_WindowBg].w = 1.0f;
	}
	ImGui_ImplWin32_Init(platform.window()->hwnd());
	ImGui_ImplDX11_Init(platform.device(), platform.device_context());

	SceneApp myApp(platform);
	myApp.Run();

	//Shutdown
	ImGui_ImplDX11_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();
	return 0;
}