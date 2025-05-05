#include "includes.h"
#include "yolo.h"
#include "Globals.h"
#include "emulation.h"
#include "trigger.h"
#include "config.h"
#include "aimbot.h"
#include "target_detector.h"
#include "aim_assist.h"
#include "bezier_engine.h"
#include "image_utils.h"
#include "tracker.h"
#include "extrapolator.h"

extern LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

Present oPresent;
HWND window = NULL;
WNDPROC oWndProc;
ID3D11Device* pDevice = NULL;
ID3D11DeviceContext* pContext = NULL;
ID3D11RenderTargetView* mainRenderTargetView;
CD3D11_TEXTURE2D_DESC pStageDesc;
ID3D11Texture2D* pStage = nullptr;
ID3D11Texture2D* pFrame = nullptr;
ID3D11ShaderResourceView* image = nullptr;
ID3D11ShaderResourceView* image2 = nullptr;
std::array<std::array<UINT, NEURAL_NETWORK_INPUT_Y>, NEURAL_NETWORK_INPUT_X> pixels;
//std::array<std::array<UINT, NEURAL_NETWORK_INPUT_Y>, NEURAL_NETWORK_INPUT_X> pixels_test;
std::array<std::array<UINT, NEURAL_NETWORK_INPUT_Y>, NEURAL_NETWORK_INPUT_X> NNPixels;
int frame_index = 0;
void InitImGui()
{
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();
	io.ConfigFlags = ImGuiConfigFlags_NoMouseCursorChange;
	ImGui_ImplWin32_Init(window);
	ImGui_ImplDX11_Init(pDevice, pContext);
}

LRESULT __stdcall WndProc(const HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {

	if (true && ImGui_ImplWin32_WndProcHandler(hWnd, uMsg, wParam, lParam))
		return true;

	return CallWindowProc(oWndProc, hWnd, uMsg, wParam, lParam);
}

void initcapture(IDXGISwapChain* pSwapChain)
{
	if (!pSwapChain)
	{
		throw std::runtime_error("Failed to get swap chain");
		return;
	}

	HRESULT hr = pSwapChain->GetBuffer(0, __uuidof(pFrame), (void**)&pFrame);

	if (FAILED(hr) || !pFrame)
	{
		throw std::runtime_error("Failed to get frame pointer");
		return;
	}

	pFrame->GetDesc(&pStageDesc);
	pStageDesc.BindFlags = 0u;
	pStageDesc.Usage = D3D11_USAGE_STAGING;
	pStageDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
	hr = pDevice->CreateTexture2D(&pStageDesc, nullptr, &pStage);

	if (FAILED(hr)) return;
}

void GetFramePtr(IDXGISwapChain* pSwapChain)
{
	HRESULT hr = pSwapChain->GetBuffer(0, __uuidof(pFrame), (void**)&pFrame);

	if (FAILED(hr) || !pFrame)
	{
		throw std::runtime_error("Failed to get frame pointer");
		return;
	}

	pContext->CopyResource(pStage, pFrame);

	D3D11_MAPPED_SUBRESOURCE map = { 0 };
	map.RowPitch = pStageDesc.Width * 4;
	map.DepthPitch = pStageDesc.Height * 4;
	hr = pContext->Map(pStage, 0u, D3D11_MAP_READ, 0u, &map);

	if (FAILED(hr))
	{
		throw std::runtime_error("Failed to map pixel data");
		return;
	}

	const unsigned char* pixelData = static_cast<unsigned char*>(map.pData);

	if (pixelData == nullptr)
	{
		throw std::runtime_error("Failed to map pixel data");
		return;
	}

	//int startx = SCREEN_WIDTH/2 - neural_network_input_x/2;
	//int starty = SCREEN_HEIGHT/2 - neural_network_input_y/2;

	constexpr int startx = 0;
	constexpr int starty = 0;

	const int stride = map.RowPitch;
	const gsl::span<const unsigned char> pixelDataSpan(pixelData, map.RowPitch * pStageDesc.Height);

	for (int y = starty; y < starty + NEURAL_NETWORK_INPUT_Y; y++)
	{
		const int yOffset = (starty + y) * stride;
		for (int x = startx; x < startx + NEURAL_NETWORK_INPUT_X; x++)
		{
			const int offset = yOffset + (startx + x) * 4;
			const unsigned char r = pixelDataSpan[offset + 2];
			const unsigned char g = pixelDataSpan[offset + 1];
			const unsigned char b = pixelDataSpan[offset + 0];
			const unsigned char a = pixelDataSpan[offset + 3];

			const UINT color = a << 24 | r | g << 8 | b << 16;

			pixels[y][x] = color;
		}
	}


	pContext->Unmap(pStage, 0u);
}

void load_image_from_array1()
{
	if (image != nullptr) {
		image->Release();
		image = nullptr;
	}
	D3D11_TEXTURE2D_DESC desc;
	ZeroMemory(&desc, sizeof(desc));
	desc.Width = NEURAL_NETWORK_INPUT_X;
	desc.Height = NEURAL_NETWORK_INPUT_Y;
	desc.MipLevels = 1;
	desc.ArraySize = 1;
	desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	desc.SampleDesc.Count = 1;
	desc.SampleDesc.Quality = 0;
	desc.Usage = D3D11_USAGE_DEFAULT;
	desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
	desc.CPUAccessFlags = 0;
	desc.MiscFlags = 0;

	D3D11_SUBRESOURCE_DATA initData;
	initData.pSysMem = pixels.data();
	initData.SysMemPitch = NEURAL_NETWORK_INPUT_X * sizeof(UINT);
	initData.SysMemSlicePitch = 0;

	ID3D11Texture2D* pTexture = nullptr;
	pDevice->CreateTexture2D(&desc, &initData, &pTexture);

	if (pTexture == nullptr)
	{
		throw std::runtime_error("Failed to create texture from image data");
		return;
	}

	// Create shader resource view
	D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
	srvDesc.Format = desc.Format;
	srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MostDetailedMip = 0;
	srvDesc.Texture2D.MipLevels = 1;

	pDevice->CreateShaderResourceView(pTexture, &srvDesc, &image);

	// Clean up texture resource
	pTexture->Release();
}

//void load_image_from_array2()
//{
//	if (image2 != nullptr) {
//		image2->Release();
//		image2 = nullptr;
//	}
//
//	D3D11_TEXTURE2D_DESC desc;
//	ZeroMemory(&desc, sizeof(desc));
//	desc.Width = NEURAL_NETWORK_INPUT_X;
//	desc.Height = NEURAL_NETWORK_INPUT_Y;
//	desc.MipLevels = 1;
//	desc.ArraySize = 1;
//	desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
//	desc.SampleDesc.Count = 1;
//	desc.SampleDesc.Quality = 0;
//	desc.Usage = D3D11_USAGE_DEFAULT;
//	desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
//	desc.CPUAccessFlags = 0;
//	desc.MiscFlags = 0;
//
//	D3D11_SUBRESOURCE_DATA initData;
//	initData.pSysMem = pixels_test.data();
//	initData.SysMemPitch = NEURAL_NETWORK_INPUT_X * sizeof(UINT);
//	initData.SysMemSlicePitch = 0;
//
//	ID3D11Texture2D* pTexture = nullptr;
//	pDevice->CreateTexture2D(&desc, &initData, &pTexture);
//
//	if (pTexture == nullptr)
//	{
//		return;
//	}
//
//	// Create shader resource view
//	D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
//	srvDesc.Format = desc.Format;
//	srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
//	srvDesc.Texture2D.MostDetailedMip = 0;
//	srvDesc.Texture2D.MipLevels = 1;
//
//	pDevice->CreateShaderResourceView(pTexture, &srvDesc, &image2);
//
//	// Clean up texture resource
//	pTexture->Release();
//}

bool init = false;
HRESULT __stdcall hkPresent(IDXGISwapChain* pSwapChain, UINT SyncInterval, UINT Flags)
{
	//auto start = std::chrono::high_resolution_clock::now();
	if (!init)
	{
		if (SUCCEEDED(pSwapChain->GetDevice(__uuidof(ID3D11Device), (void**)&pDevice)))
		{
			initcapture(pSwapChain);

			pDevice->GetImmediateContext(&pContext);
			DXGI_SWAP_CHAIN_DESC sd;
			pSwapChain->GetDesc(&sd);
			window = sd.OutputWindow;
			ID3D11Texture2D* pBackBuffer;
			pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&pBackBuffer);
			pDevice->CreateRenderTargetView(pBackBuffer, NULL, &mainRenderTargetView);
			pBackBuffer->Release();
			oWndProc = (WNDPROC)SetWindowLongPtr(window, GWLP_WNDPROC, (LONG_PTR)WndProc);
			InitImGui();
			init = true;
		}

		else
			return oPresent(pSwapChain, SyncInterval, Flags);
	}

	GetFramePtr(pSwapChain);
	std::copy(&pixels[0][0], &pixels[0][0] + NEURAL_NETWORK_INPUT_X * NEURAL_NETWORK_INPUT_Y, &NNPixels[0][0]);
	//std::copy(&pixels[0][0], &pixels[0][0] + NEURAL_NETWORK_INPUT_X * NEURAL_NETWORK_INPUT_Y, &pixels_test[0][0]);
	bool trigger_should_shoot = trigger_check(pixels);
	emulation->trigger_should_shoot = trigger_should_shoot;
	emulation->shoot_trigger();
	int time_now = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count();
	if (time_now - emulation->trigger_last_time > 20)
	{
		emulation->trigger_should_reset = true;
	}

	std::vector<Track> detections;
	std::pair<int, int> xy_of_index;
	std::pair<int, int> wh_of_index;
	float confidence_of_index;

	{
		std::lock_guard<std::mutex> lock(tracker->tracks_mutex);
		detections = tracker->tracks;

		xy_of_index = get_xy_of_index(0);
		wh_of_index = get_wh_of_index(0);
		confidence_of_index = get_confidence_of_index(0);
	}

	if (config->DRAW_DETECTION_RECTANGLES) draw_detections(pixels, detections);
	if (config->DRAW_AIMBOT) draw_aimbot(pixels);
	if (config->DRAW_AIM_ASSIST) draw_aim_assist(pixels);
	if (config->DRAW_AIMBOT_TARGET) draw_aimbot_target(pixels, aimbot->target);
	if (config->DRAW_AIMASSIST_TARGET) draw_aim_assist_target(pixels, aim_assist->target);

	//if(config->FILL) fill_outline(pixels, pixels_test);

	ImGui_ImplDX11_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();

	load_image_from_array1();
	//load_image_from_array2();

	ImGui::Begin("ImGui Window");

	if (ImGui::BeginTabBar("Nigger"))
	{
		if (ImGui::BeginTabItem("Debug"))
		{
			ImGui::Text("Detections: %d", detections.size());
			ImGui::Text("Confidence: %f", confidence_of_index);
			ImGui::Text("X: %d Y: %d", xy_of_index.first, xy_of_index.second);
			ImGui::Text("W: %d H: %d", wh_of_index.first, wh_of_index.second);
			ImGui::Text("FPS: %f", ImGui::GetIO().Framerate);
			ImGui::Text("Trigger: %s", emulation->trigger_on ? "On" : "Off");
			ImGui::Text("Trigger Should Shoot: %s", trigger_should_shoot ? "Yes" : "No");
			ImGui::Text("ADS: %s", emulation->ads_on ? "On" : "Off");
			ImGui::Text("NN Run Time: %lld", std::chrono::duration_cast<std::chrono::milliseconds>(config->NN_RUN_TIME).count());
			ImGui::SliderFloat("Neural Network FPS Limit", &config->NN_FPS, 1.f, 120.f);
			//ImGui::SliderFloat("Triggerbot FPS Limit", &config->TRIGGER_BOT_FPS, 1.f, 240.f);
			ImGui::Text("Left click down %s", emulation->mouse_button_down ? "Yes" : "No");
			ImGui::SliderFloat("Model Head Confidence Threshold", &config->modelHeadConfidenceThreshold, 0.f, 1.f);
			ImGui::SliderFloat("Model Body Confidence Threshold", &config->modelBodyConfidenceThreshold, 0.f, 1.f);
			ImGui::SliderFloat("Model Iso Ball Confidence Threshold", &config->modelIsoBallConfidenceThreshold, 0.f, 1.f);
			ImGui::SliderFloat("Model Reyna Flash Confidence Threshold", &config->modelReynaFlashConfidenceThreshold, 0.f, 1.f);
			ImGui::SliderFloat("Tracker High Threshold", &config->trackerHighThreshold, 0.f, 1.f);
			ImGui::SliderFloat("Tracker Match Threshold", &config->trackerMatchThreshold, 0.f, 1.f);
			ImGui::SliderFloat("Tracker FPS", &config->trackerFps, 1.f, 240.f);
			ImGui::SliderInt("Tracker Track Buffer", &config->trackerTrackBuffer, 1, 60);
			ImGui::Checkbox("Draw Detections", &config->DRAW_DETECTION_RECTANGLES);
			ImGui::DragInt("Full 360", &target_detector->Full360val);
			if (ImGui::Button("Test full 360"))
			{
				Sleep(3000);
				emulation->aimbot2(target_detector->Full360(), 0);
			}
			if (ImGui::Button("Test aimbot"))
			{
				Sleep(3000);
				aimbot->test_aimbot();
			}
			ImGui::EndTabItem();
		}
		if (ImGui::BeginTabItem("Trigger"))
		{
			ImGui::SliderFloat("HUE_MIN", &config->HUE_MIN, 0.0f, 360.0f);
			ImGui::SliderFloat("HUE_MAX", &config->HUE_MAX, 0.0f, 360.0f);
			ImGui::SliderFloat("SATURATION_MIN", &config->SATURATION_MIN, 0.0f, 1.0f);
			ImGui::SliderFloat("SATURATION_MAX", &config->SATURATION_MAX, 0.0f, 1.0f);
			ImGui::SliderFloat("VALUE_MIN", &config->VALUE_MIN, 0.0f, 1.0f);
			ImGui::SliderFloat("VALUE_MAX", &config->VALUE_MAX, 0.0f, 1.0f);
			ImGui::SliderInt("PIXELBOX_SIZE", &config->PIXELBOX_SIZE, 0, 20);
			ImGui::SliderInt("FILL_ITERATIONS", &config->FILL_ITERATIONS, 0, 10);
			ImGui::Checkbox("FILL", &config->FILL);
			ImGui::SliderInt("MIN_DELAY", &config->MIN_DELAY, 1, 1000);
			ImGui::SliderInt("MAX_DELAY", &config->MAX_DELAY, 1, 1000);
			ImGui::EndTabItem();
		}
		if (ImGui::BeginTabItem("Aim assist"))
		{
			ImGui::Checkbox("Aim Assist", &config->AIM_ASSIST_ON);
			if (config->AIM_ASSIST_ON)
			{
				ImGui::Checkbox("Draw Aim Assist", &config->DRAW_AIM_ASSIST);
				ImGui::SliderInt("Aimbot Time", &config->AIM_ASSIST_TIME, 1, 1000);
				ImGui::SliderFloat("Aim Assist FOV", &config->AIM_ASSIST_FOV, 0.f, 30.f);
				ImGui::SliderInt("Aim Assist Dampening", &config->AIM_ASSIST_DAMPENING, 0, 100);
				ImGui::Text("Aim Assist X: %d Aim Assist Y: %d", aim_assist->x_movement, aim_assist->y_movement);
				ImGui::ListBox("Aimbot Mode", &config->aim_assist_mode, config->aim_assist_modes, IM_ARRAYSIZE(config->aim_assist_modes));
			}
			ImGui::EndTabItem();
		}
		if (ImGui::BeginTabItem("Aimbot"))
		{
			ImGui::Checkbox("Aimbot", &config->AIMBOT_ON);
			if (config->AIMBOT_ON)
			{
				ImGui::Checkbox("Draw Aimbot", &config->DRAW_AIMBOT);
				ImGui::SliderInt("Aimbot Time", &config->AIMBOT_TIME, 1, 100);
				ImGui::SliderFloat("Aimbot FOV", &config->AIMBOT_FOV, 0.f, 30.f);
				ImGui::SliderInt("Aimbot Dampening", &config->AIMBOT_DAMPENING, 0, 100);
				//ImGui::ListBox("Aimbot Mode", &config->aimbot_mode, config->aimbot_modes, IM_ARRAYSIZE(config->aimbot_modes));
			}
			ImGui::EndTabItem();
		}
		if (ImGui::BeginTabItem("Target selector"))
		{
			ImGui::Checkbox("Draw Aimbot Stats", &config->DRAW_AIMBOT_TARGET);
			ImGui::Checkbox("Draw Aim Assist Stats", &config->DRAW_AIMASSIST_TARGET);
			if (config->DRAW_AIMBOT_TARGET && config->DRAW_AIMASSIST_TARGET)
			{
				if (ImGui::BeginTable("TargetComparison", 3, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg))
				{
					Target t1 = aimbot->target, t2 = aim_assist->target;
					ImGui::TableNextRow();
					ImGui::TableNextColumn(); ImGui::Text("Field");
					ImGui::TableNextColumn(); ImGui::Text("Aimbot Target");
					ImGui::TableNextColumn(); ImGui::Text("Aim Assist Target");

					auto display_row = [](const char* field_name, auto value1, auto value2)
						{
							ImGui::TableNextRow();
							ImGui::TableNextColumn(); ImGui::Text("%s", field_name);
							ImGui::TableNextColumn(); ImGui::Text("%s", std::to_string(value1).c_str());
							ImGui::TableNextColumn(); ImGui::Text("%s", std::to_string(value2).c_str());
						};

					display_row("Target Type", t1.target_type, t2.target_type);
					display_row("X Movement", t1.x_movement, t2.x_movement);
					display_row("Y Movement", t1.y_movement, t2.y_movement);
					display_row("X Pixels", t1.x_pixels, t2.x_pixels);
					display_row("Y Pixels", t1.y_pixels, t2.y_pixels);
					display_row("Is On Head", t1.is_on_head, t2.is_on_head);
					display_row("Is On Body", t1.is_on_body, t2.is_on_body);
					display_row("Priority", t1.priority, t2.priority);

					ImGui::EndTable();
				}
			}
			ImGui::EndTabItem();
		}
		if(ImGui::BeginTabItem("Extrapolator"))
		{
			ImGui::Checkbox("Toggle Extrapolation Aimbot", &config->TOGGLE_EXTRAPOLATION_AIMBOT);
			ImGui::Checkbox("Toggle Extrapolation Aim Assist", &config->TOGGLE_EXTRAPOLATION_AIMASSIST);
			ImGui::SliderInt("Extrapolation Buffer", &config->EXTRAPOLATION_BUFFER, 1, 10);
			ImGui::SliderInt("Extrapolation Offset", &config->EXTRAPOLATION_OFFSET, 0, 100);
			ImGui::Text("Extrapolated X: %d Extrapolated Y: %d", extrapolator->extrapolated_x, extrapolator->extrapolated_y);
			ImGui::Text("Movement X: %d movement Y: %d", extrapolator->movement_x, extrapolator->movement_y);
			ImGui::EndTabItem();
		}
		//if (ImGui::BeginTabItem("Debug Messages"))
		//{
		//	for (int i = 0; i < config->debug_messages.size(); i++)
		//	{
		//		ImGui::Text(config->debug_messages[i].c_str());
		//	}
		//	ImGui::EndTabItem();
		//}
	}
	ImGui::EndTabBar();

	ImGui::Image(image, ImVec2(NEURAL_NETWORK_INPUT_X, NEURAL_NETWORK_INPUT_Y));

	ImGui::End();

	ImGui::Render();

	pContext->OMSetRenderTargets(1, &mainRenderTargetView, NULL);
	ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
	if (image != nullptr) {
		image->Release();
		image = nullptr;
	}
	if (image2 != nullptr) {
		image2->Release();
		image2 = nullptr;
	}

	//auto end = std::chrono::high_resolution_clock::now();
	//auto elapsed = end - start;

	//precise_sleep(0.000001 * ((1000000.f / config->TRIGGER_BOT_FPS) - std::chrono::duration_cast<std::chrono::microseconds>(elapsed).count()));

	return oPresent(pSwapChain, SyncInterval, Flags);
}

DWORD WINAPI MainThread(LPVOID lpReserved)
{
	bool init_hook = false;
	do
	{
		if (kiero::init(kiero::RenderType::D3D11) == kiero::Status::Success)
		{
			kiero::bind(8, (void**)&oPresent, hkPresent);
			init_hook = true;
		}
	} while (!init_hook);
	return TRUE;
}

DWORD WINAPI MainThread2(LPVOID lpReserved)
{
	emulation = std::make_shared<Emulation>();
	config = std::make_shared<Config>();
	aimbot = std::make_shared<Aimbot>();
	target_detector = std::make_shared<TargetDetector>();
	aim_assist = std::make_shared<AimAssist>();
	bezier_engine = std::make_shared<BezierEngine>();
	tracker = std::make_shared<Tracker>();
	extrapolator = std::make_shared<Extrapolator>();

	bezier_engine->init_cubic_bezier({ 0, 0 }, { 0, 1 }, { 1, 1 }, { 1, 1 });
	target_detector->init();
	extrapolator->init();

	inference.init("C:\\Users\\Niku\\Desktop\\neural_networks\\valorant_best_yolov10s_no_pretrain.onnx");
	std::thread emulation_thread = std::thread([]() {
		if (emulation->init() == -1)
		{
			//throw std::runtime_error("Failed to initialize emulation");
			return;
		}
		emulation->loop();
		});

	std::thread aimbot_thread = std::thread([]() {
		while (true)
		{
			aimbot->loop();
			Sleep(1);
		}
		});

	std::array<std::array<UINT, NEURAL_NETWORK_INPUT_Y>, NEURAL_NETWORK_INPUT_X> NNPixelsRuntime{};
	while (true)
	{
		const auto start = std::chrono::high_resolution_clock::now();
		std::copy(&NNPixels[0][0], &NNPixels[0][0] + NEURAL_NETWORK_INPUT_X * NEURAL_NETWORK_INPUT_Y, &NNPixelsRuntime[0][0]);
		process_frame(NNPixelsRuntime);
		const auto end = std::chrono::high_resolution_clock::now();
		const auto nn_run_time_first = end - start;
		const long long miliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(nn_run_time_first).count();
		config->NN_RUN_TIME = std::chrono::high_resolution_clock::now() - start;
		auto time_to_sleep = std::chrono::milliseconds(1000) / config->NN_FPS - nn_run_time_first;
		if (time_to_sleep.count() > 0)
		{
			std::this_thread::sleep_for(time_to_sleep);
		}
	}

	return TRUE;
}

BOOL WINAPI DllMain(HMODULE hMod, DWORD dwReason, LPVOID lpReserved)
{
	switch (dwReason)
	{
	case DLL_PROCESS_ATTACH:
		DisableThreadLibraryCalls(hMod);
		CreateThread(nullptr, 0, MainThread2, hMod, 0, nullptr);
		CreateThread(nullptr, 0, MainThread, hMod, 0, nullptr);
		break;
	case DLL_PROCESS_DETACH:
		kiero::shutdown();
		break;
	default:
		break;
	}
	return TRUE;
}