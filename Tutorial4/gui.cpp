#include "gui.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#ifndef __STDC_LIB_EXT1__
#define __STDC_LIB_EXT1__
#endif 

#include <sstream>
#include <imgui_impl_sdl2.h>
#include <imgui_impl_sdlrenderer2.h>
#include <stb_image_write.h>

#include "mandelbrot.h"
#include "time.h"

using namespace std;

static string getCaptureName() {
	time_t now = time(0);
	char buf[80];
	tm tstruct;
	localtime_s(&tstruct, &now);
	strftime(buf, sizeof(buf), "%Y-%m-%d-%H-%M-%S.png", &tstruct);
	return { buf };
}

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

	memset(settings.capture_dir, '\0', 100);
	strcat_s(settings.capture_dir, 100, "captures\\");
	settings.capture_no_ui = true;

	msg_timepoint = -3000;
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

bool GUI::keyCaptured() const
{
	return ImGui::GetIO().WantCaptureKeyboard;
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

	if (ImGui::TreeNode("screenshot")) {
		ImGui::Text("directory :");
		ImGui::InputText("##3", settings.capture_dir, 255);
		ImGui::Checkbox("no UI", &settings.capture_no_ui);
		if (ImGui::Button("capture")) {
			if (settings.capture_no_ui) {
				saveCapture(mandelbrot->getSurface());
			} else {
				SDL_Surface* surface = SDL_CreateRGBSurface(0, width, height, 32, 0, 0, 0, 0);
				auto* format         = (uint32_t*)(surface->format);
				SDL_RenderReadPixels(renderer, 0, *format, surface->pixels, width * 4);
				saveCapture(surface);
				SDL_FreeSurface(surface);
			}
		}
		ImGui::TreePop();
	}

	auto content_hegiht = ImGui::GetContentRegionAvail().y;
	ImGui::SetWindowSize({ 260, content_hegiht });

	if (clock() - msg_timepoint < 3000) {
		auto* drawlist = ImGui::GetForegroundDrawList();
		auto position  = ImGui::GetWindowPos();
		ImColor color  = { 255, 0, 0 };

		position.y += ImGui::GetWindowHeight();
		drawlist->AddText(position, color, message.c_str());
	}

	ImGui::End();
	ImGui::Render();
}

void GUI::draw()
{
	ImGui_ImplSDLRenderer2_RenderDrawData(ImGui::GetDrawData());
}

void GUI::saveCapture(const SDL_Surface* surface)
{
	auto& dir = settings.capture_dir;
	auto name = dir + getCaptureName();

	auto* copy = SDL_ConvertSurfaceFormat(const_cast<SDL_Surface*>(surface), SDL_PIXELFORMAT_RGB24, 0);
	int res = stbi_write_png(name.c_str(), copy->w, copy->h, 3, copy->pixels, copy->w * 3);
	SDL_FreeSurface(copy);

	if (res == 0) postErrorMessage("couldn't save capture!\ncheck if your directory exists");
}

void GUI::postErrorMessage(const char* msg)
{
	msg_timepoint = clock();
	message = msg;
}