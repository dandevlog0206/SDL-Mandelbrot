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
#include "mandelbrot_cuda.h"
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

	settings.accelerator   = Acc::GPU_CUDA;
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

	showBasicUI(mandelbrot);
	showAcceleratorUI(mandelbrot);
	showAppearanceUI(mandelbrot);
	showMoreSettingsUI(mandelbrot);
	showScreenshotUI(mandelbrot);

	ImGui::SetWindowSize({ 260, ImGui::GetContentRegionAvail().y });

	showErrorMessage();

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
	auto pos         = mandelbrot->getPosition();
	auto scale       = mandelbrot->getScale();
	auto iter        = mandelbrot->getIteration();
	auto color_map   = mandelbrot->getColormap();
	auto color_scale = mandelbrot->getColorScale();
	auto smooth      = mandelbrot->getColorSmooth();

	mandelbrot->stop();

	if (settings.accelerator == Acc::CPU)
		mandelbrot = make_unique<Mandelbrot>(renderer);
	else if (settings.accelerator == Acc::CPU_TBB)
		mandelbrot = make_unique<MandelbrotTBB>(renderer);
	else if (settings.accelerator == Acc::GPU_CUDA)
		mandelbrot = make_unique<MandelbrotCUDA>(renderer);

	mandelbrot->setPosition(pos.real(), pos.imag());
	mandelbrot->setScale(scale);
	mandelbrot->setIteration(iter);
	mandelbrot->setColormap(color_map);
	mandelbrot->setColorScale(color_scale);
	mandelbrot->setColorSmooth(smooth);
}

void GUI::showBasicUI(std::unique_ptr<Mandelbrot>& mandelbrot)
{
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
}

void GUI::showMoreSettingsUI(std::unique_ptr<Mandelbrot>& mandelbrot)
{
	if (ImGui::CollapsingHeader("more settings...")) {
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
	}

	if (settings.auto_iter) {
		auto iter    = mandelbrot->getIteration();
		auto mag     = 1. / mandelbrot->getScale();
		int new_iter = settings.initial_iter * (log(mag + 1) - log(2) + 1.);
		if (iter != new_iter) mandelbrot->setIteration(new_iter);
	}
}

void GUI::showAcceleratorUI(std::unique_ptr<Mandelbrot>& mandelbrot)
{
	if (ImGui::CollapsingHeader("accelerator")) {
		static const char* items[] = { "CPU", "CPU - TBB", "GPU - CUDA" };

		ImGui::Text("accelerator :");
		if (ImGui::Combo(IMGUI_NO_LABEL, (int*)&settings.accelerator, items, 3))
			acceleratorChanged(mandelbrot);

		if (settings.accelerator == Acc::CPU_TBB) {
			auto* man_tbb = dynamic_cast<MandelbrotTBB*>(mandelbrot.get()); // will not fail
			auto value    = (int)man_tbb->getMaxConcurrency();
			ImGui::Text("max concurrency:");
			if (ImGui::InputInt(IMGUI_NO_LABEL, &value, 1)) {
				value = SDL_clamp(value, 1, thread::hardware_concurrency());
				man_tbb->setMaxConcurrency(value);
			}
		} else if (settings.accelerator == Acc::GPU_CUDA) {
			static const char* sizes[] = { "1x1", "2x2", "4x4", "8x8", "16x16" };

			auto* man_cuda = dynamic_cast<MandelbrotCUDA*>(mandelbrot.get()); // will not fail

			auto block_size = (int)log2(man_cuda->getBlockSize());
			ImGui::Text("block size:");
			if (ImGui::Combo(IMGUI_NO_LABEL, &block_size, sizes, 5))
				man_cuda->setBlockSize(pow(block_size + 1, 2));

			auto sample_total = (int)man_cuda->getTotalSample();
			ImGui::Text("total sample:");
			if (ImGui::InputInt(IMGUI_NO_LABEL, &sample_total))
				man_cuda->setTotalSample(sample_total = max(sample_total, 1));
			
			auto spl = (int)man_cuda->getSamplePerLaunch();
			ImGui::Text("sample per launch:");
			ImGui::InputInt(IMGUI_NO_LABEL, &spl);
			man_cuda->setSamplePerLaunch(spl = SDL_clamp(spl, 1, sample_total));

			ImGui::Text("sampled: %d", man_cuda->getSampleCount());
		}
	}
}

void GUI::showAppearanceUI(std::unique_ptr<Mandelbrot>& mandelbrot)
{
	if (ImGui::CollapsingHeader("appearance")) {
		static const char* items[] = { "gray", "ultra", "viridis", "magma", "inferno", "turbo" };
		
		auto color_map    = mandelbrot->getColormap();
		float color_scale = mandelbrot->getColorScale();
		auto smooth       = mandelbrot->getColorSmooth();

		ImGui::Text("colormap :");
		if (ImGui::Combo(IMGUI_NO_LABEL, (int*)&color_map, items, 6))
			mandelbrot->setColormap(color_map);
		
		ImGui::Text("color scale :");
		if (ImGui::DragFloat(IMGUI_NO_LABEL, &color_scale, 1.f, 1.f, 128, "%f", ImGuiSliderFlags_Logarithmic))
			mandelbrot->setColorScale(color_scale);

		ImGui::Text("smooth color :");
		if (ImGui::Checkbox(IMGUI_NO_LABEL, &smooth))
			mandelbrot->setColorSmooth(smooth);
	}
}

void GUI::showScreenshotUI(std::unique_ptr<Mandelbrot>& mandelbrot)
{
	int width, height;
	SDL_GetWindowSize(window, &width, &height);

	if (ImGui::CollapsingHeader("screenshot")) {
		ImGui::Text("directory :");
		ImGui::InputText(IMGUI_NO_LABEL, settings.capture_dir, 255);
		ImGui::Checkbox("no UI", &settings.capture_no_ui);
		if (ImGui::Button("capture")) {
			if (settings.capture_no_ui) {
				saveCapture(mandelbrot->getSurface());
			} else {
				SDL_Surface* surface = SDL_CreateRGBSurface(0, width, height, 32, 0, 0, 0, 0);
				auto* format = (uint32_t*)(surface->format);
				SDL_RenderReadPixels(renderer, 0, *format, surface->pixels, width * 4);
				saveCapture(surface);
				SDL_FreeSurface(surface);
			}
		}
	}
}

void GUI::showErrorMessage()
{
	if (clock() - msg_timepoint < 3000) {
		auto* drawlist = ImGui::GetForegroundDrawList();
		auto position  = ImGui::GetWindowPos();
		ImColor color  = { 255, 0, 0 };

		position.y += ImGui::GetWindowHeight();
		drawlist->AddText(position, color, message.c_str());
	}
}