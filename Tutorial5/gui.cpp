#include "gui.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#ifndef __STDC_LIB_EXT1__
#define __STDC_LIB_EXT1__
#endif 

#define STRINGIZE_DETAIL(x) #x
#define STRINGIZE(x) STRINGIZE_DETAIL(x)
#define IMGUI_NO_LABEL "##" STRINGIZE(__COUNTER__)

#include <sstream>
#include <imgui_impl_sdl2.h>
#include <imgui_impl_sdlrenderer2.h>
#include <stb_image_write.h>

#include "mandelbrot_tbb.h"
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

	settings.accelerator   = Acc::CPU_TBB;
	settings.move_speed    = 300.f;
	settings.scroll_scale  = 1.1f;
	settings.render_async  = true;
	settings.scaleToCursor = true;
	settings.auto_iter     = true;
	settings.initial_iter  = 32;

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

void GUI::update(std::unique_ptr<Mandelbrot>& mandelbrot)
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

	if (mandelbrot->isRendering())
		ImGui::TextColored(ImColor(255, 0, 0), "rendering...");
	else ImGui::TextColored(ImColor(0, 255, 0), "rendered");

	if (ImGui::Button("reset parameters")) {
		mandelbrot->setPosition(0., 0.);
		mandelbrot->setScale(1.);
		mandelbrot->setIteration(32);
	}

	if (ImGui::TreeNode("more settings...")) {
		static const char* items[] = { "CPU", "CPU - TBB" };
		ImGui::Text("accelerator :");
		if (ImGui::Combo(IMGUI_NO_LABEL, (int*)&settings.accelerator, items, 2))
			acceleratorChanged(mandelbrot);

		ImGui::Text("move speed (pixel/s) :");
		ImGui::SliderFloat(IMGUI_NO_LABEL, &settings.move_speed, 200.f, 600.f);

		ImGui::Text("scroll scale :");
		ImGui::SliderFloat(IMGUI_NO_LABEL, &settings.scroll_scale, 1.1f, 10.f, "%f", 32);

		ImGui::Checkbox("render async", &settings.render_async);

		ImGui::Checkbox("scale to cursor", &settings.scaleToCursor);

		ImGui::Checkbox("auto iter", &settings.auto_iter);
		if (settings.auto_iter) {
			ImGui::Text("initial iteration:");
			ImGui::InputInt(IMGUI_NO_LABEL, &settings.initial_iter);
		}
		ImGui::TreePop();
	}

	if (settings.auto_iter) {
		auto iter = mandelbrot->getIteration();
		auto mag = 1. / mandelbrot->getScale();
		int new_iter = settings.initial_iter * (log(mag + 1) - log(2) + 1.);
		if (iter != new_iter) mandelbrot->setIteration(new_iter);
	}

	if (ImGui::TreeNode("screenshot")) {
		ImGui::Text("directory :");
		ImGui::InputText(IMGUI_NO_LABEL, settings.capture_dir, 255);
		ImGui::Checkbox("no UI", &settings.capture_no_ui);
		if (ImGui::Button("capture")) {
			if (settings.capture_no_ui) {
				saveCapture(mandelbrot->getSurface());
			}
			else {
				SDL_Surface* surface = SDL_CreateRGBSurface(0, width, height, 32, 0, 0, 0, 0);
				auto* format = (uint32_t*)(surface->format);
				SDL_RenderReadPixels(renderer, 0, *format, surface->pixels, width * 4);
				saveCapture(surface);
				SDL_FreeSurface(surface);
			}
		}
		ImGui::TreePop();
	}

	ImGui::SetWindowSize({ 260, ImGui::GetContentRegionAvail().y });

	if (clock() - msg_timepoint < 3000) {
		auto* drawlist = ImGui::GetForegroundDrawList();
		auto position = ImGui::GetWindowPos();
		ImColor color = { 255, 0, 0 };

		position.y += ImGui::GetWindowHeight();
		drawlist->AddText(position, color, message.c_str());
	}

	ImGui::End();
}

void GUI::render()
{
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

void GUI::acceleratorChanged(std::unique_ptr<Mandelbrot>& mandelbrot)
{
	auto pos   = mandelbrot->getPosition();
	auto scale = mandelbrot->getScale();
	auto iter  = mandelbrot->getIteration();
	mandelbrot->stop();

	if (settings.accelerator == Acc::CPU)
		mandelbrot = make_unique<Mandelbrot>(renderer);
	else if (settings.accelerator == Acc::CPU_TBB)
		mandelbrot = make_unique<MandelbrotTBB>(renderer);

	mandelbrot->setPosition(pos.real(), pos.imag());
	mandelbrot->setScale(scale);
	mandelbrot->setIteration(iter);
}