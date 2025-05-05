#pragma once
#define WIN32_LEAN_AND_MEAN

#include <Windows.h>
#include <WinSock2.h>
#include <vector>
#include <string>
#include <d3d11.h>
#include <dxgi.h>
#include <deque>
#include <gsl/gsl>
#include "kiero/kiero.h"
#include "imgui/imgui.h"
#include "imgui/imgui_impl_win32.h"
#include "imgui/imgui_impl_dx11.h"
#include "opencv2/core.hpp"
#include "opencv2/imgproc.hpp"
#include "onnxruntime_cxx_api.h"
#include "dml_provider_factory.h"
#include <fstream>
#include <iostream>
#include <random>
#include <chrono>
#include <cstddef>
#include <limits>
#include <map>
#include <memory>
#include "Eigen/Dense"
#include <unordered_set>

#pragma comment(lib, "ws2_32.lib")

typedef HRESULT(__stdcall* Present) (IDXGISwapChain* pSwapChain, UINT SyncInterval, UINT Flags);
typedef LRESULT(CALLBACK* WNDPROC)(HWND, UINT, WPARAM, LPARAM);
typedef uintptr_t PTR;