#include "gui.h"

#include <sstream>
#include <imgui_impl_sdl2.h>
#include <imgui_impl_sdlrenderer2.h>

#include "mandelbrot.h"
#include "time.h"

using namespace std;

template <class T>
ostream& operator<<(ostream& os, complex<T> c) {
	os << c.real() << (c.imag() < 0 ? " - " : " + ") << abs(c.imag()) << "i";
	return os;
}

GUI::GUI(SDL_Renderer* renderer)
	: renderer(renderer)
{
	window = SDL_RenderGetWindow(renderer);

	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	auto& io = ImGui::GetIO();

	ImGui_ImplSDL2_InitForSDLRenderer(window, renderer);
	ImGui_ImplSDLRenderer2_Init(renderer);

	io.Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\consola.ttf", 15);

	settings.accelerator   = Acc::CPU;
	settings.move_speed    = 300.f;
	settings.scroll_scale  = 1.1f;
	settings.scaleToCursor = true;
}

GUI::~GUI()
{
	ImGui_ImplSDLRenderer2_Shutdown();
	ImGui_ImplSDL2_Shutdown();
	ImGui::DestroyContext();
}

void GUI::processEvent(SDL_Event* event)
{
	ImGui_ImplSDL2_ProcessEvent(event);
}

bool GUI::mouseCaptured()
{
	return ImGui::GetIO().WantCaptureMouse;
}

void GUI::render(const Mandelbrot* mandelbrot)
{
	ImGui_ImplSDLRenderer2_NewFrame();
	ImGui_ImplSDL2_NewFrame(window);
	ImGui::NewFrame();
	ImGui::Begin("control", nullptr, ImGuiWindowFlags_NoResize);

	stringstream ss;
	int px, py, width, height;

	SDL_GetMouseState(&px, &py);
	SDL_GetWindowSize(window, &width, &height);

	ss << "resolution: " << width << "X" << height << "\n";
	ss << "mouse : (" << px << ", " << py << ")\n";
	ss << "fps   : " << round(Time::fps * 10) / 10;
	ss << "(" << round(10000. * Time::dt) / 10. << "ms)\n";
	ss << "cursor: " << mandelbrot->pixelToComplex(px, py) << "\n";
	ss << "pos   : " << mandelbrot->getPosition() << "\n";
	ss << "scale : " << mandelbrot->getScale() << "\n";
	ss << "iter  : " << mandelbrot->getIteration() << "\n";
	ImGui::Text(ss.str().c_str());

	if (ImGui::Button("reset parameters"))
		settings.reset_params = true;

	if (ImGui::TreeNode("more settings...")) {
		static const char* items[] = { "CPU", "CPU - TBB" };
		ImGui::Text("accelerator :");
		ImGui::Combo("", (int*)&settings.accelerator, items, 2);

		ImGui::Text("move speed (pixel/s) :");
		ImGui::SliderFloat("##1", &settings.move_speed, 200.f, 600.f);

		ImGui::Text("scroll scale :");
		ImGui::SliderFloat("##2", &settings.scroll_scale, 1.1f, 10.f, "%f", 32);

		ImGui::Checkbox("scale to cursor", &settings.scaleToCursor);
		ImGui::TreePop();
	}

	auto content_hegiht = ImGui::GetContentRegionAvail().y;
	ImGui::SetWindowSize({ 260, content_hegiht });

	ImGui::End();
	ImGui::Render();
}

void GUI::draw()
{
	ImGui_ImplSDLRenderer2_RenderDrawData(ImGui::GetDrawData());
}