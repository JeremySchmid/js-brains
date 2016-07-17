/*TODO: THIS IS NOT A FINAL PLATFORM LAYER

  - Saved game locations
  - Getting a handle to our own executable file
  - Asset loading path
  - Threading (launch a thread)
  - Raw Input (support for multiple keyboards)
  - Sleep/timeBeginPeriod
  - ClipCursor() for multimonitor support
  - Fullscreen support
  - WM_SETCURSOR (control cursor visibility)
  - QueryCancelAutoplay
  - WM_ACTIVATEAPP (for when we are not the active application
  - Blit speed improvements (BitBlt)
  - Hardware acceleration (OpenGL or Direct3D or BOTH??)
  - GetKeyboardLayout (for French keyboards, int'l WASD support)

  JUST A PARTIAL LIST THERE!

*/

#include "base.h"

#include <malloc.h>
#include <windows.h>
#include <xinput.h>
#include <dsound.h>
#include <stdio.h>

#include "win32.h"

global_var boolint GlobalRunning;
global_var boolint GlobalPause;
global_var boolint GlobalFast;
global_var win32_offscreen_buffer GlobalBuffer;
global_var LPDIRECTSOUNDBUFFER GlobalSecondaryBuffer;
global_var int64_t GlobalPerfCountFrequency;

#define DIRECT_SOUND_CREATE(name) HRESULT WINAPI name(LPCGUID pcGuidDevice, LPDIRECTSOUND *ppDS, LPUNKNOWN pUnkOuter);
typedef DIRECT_SOUND_CREATE(direct_sound_create);

DEBUG_PLATFORM_FREE_FILE_MEMORY(DEBUGPlatformFreeFileMemory)
{
	VirtualFree(Memory, 0, MEM_RELEASE);
}

DEBUG_PLATFORM_READ_ENTIRE_FILE(DEBUGPlatformReadEntireFile)
{
	debug_read_file_result Result = {};

	HANDLE File;
	File = CreateFile(Filename, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);

	if(File != INVALID_HANDLE_VALUE)
	{
		LARGE_INTEGER FileSize = {};
		GetFileSizeEx(File, &FileSize);
		if (FileSize.QuadPart != 0)
		{
			Result.Contents = VirtualAlloc(0, (SIZE_T)FileSize.QuadPart, MEM_RESERVE|MEM_COMMIT, PAGE_EXECUTE_READWRITE);
			Result.ContentSize = SafeTruncateUInt64((uint64_t)FileSize.QuadPart);
			if (Result.Contents)
			{
				DWORD BytesRead;
				if (ReadFile(File, Result.Contents, Result.ContentSize, &BytesRead, 0) && Result.ContentSize == BytesRead)
				{
					//File read successfully
				}
				else {
					DEBUGPlatformFreeFileMemory(Thread, Result.Contents);
					Result.Contents = 0;
				}
			}
			else {
				//logging
			}
		}
		else {
			//logging
		}
		CloseHandle(File);
	}
	else {
		//logging
	}

	if (GetLastError() == ERROR_FILE_NOT_FOUND)
	{
		//logging and popup or something
		OutputDebugStringA("File NOT FOUND.\n");
	}
	return Result;
}

DEBUG_PLATFORM_WRITE_ENTIRE_FILE(DEBUGPlatformWriteEntireFile)
{
	boolint Result = false;

	HANDLE File;
	File = CreateFile(Filename, GENERIC_WRITE, 0, 0, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);

	if(File != INVALID_HANDLE_VALUE)
	{		
		DWORD BytesWritten;
		uint32_t MemorySize32 = SafeTruncateUInt64(MemorySize);	
		if (WriteFile(File, Memory, MemorySize32, &BytesWritten, 0))
		{
			//File written successfully
			Result = (MemorySize32 == BytesWritten);
		}
		else {
			//logging
		}
		CloseHandle(File);
	}
	else {
		//logging
	}
	return Result;
}

void CatStrings(size_t SourceACount, char* SourceA, size_t SourceBCount, char* SourceB, size_t DestCount, char* Dest)
{
	for (int Index = 0; Index < (int)SourceACount; Index++)
	{
		*Dest++ = *SourceA++;
	}
	for (int Index = 0; Index < (int)SourceBCount; Index++)
	{
		*Dest++ = *SourceB++;
	}
	*Dest++ = 0;
}

internal int StringLength(char* String)
{
	int Count = 0;
	while (*String++)
	{
		++Count;
	}
	return Count;
}

internal void Win32BuildExePathFileName(win32_state* State, char* FileName, int DestCount, char* Dest)
{
	CatStrings((size_t)(State->OnePastLastExeFileNameSlash - State->ExeFileName), State->ExeFileName, (size_t)StringLength(FileName), FileName, (size_t)DestCount, Dest);
}

internal void Win32DebugDrawVertical(win32_offscreen_buffer* BackBuffer, int X, int Top, int Bottom, uint32_t Color)
{
	if (Top <= 0)
	{
		Top = 0;
	}
	if (Bottom > BackBuffer->Height)
	{
		Bottom = BackBuffer->Height;
	}
	if (X >= 0 && X < BackBuffer->Width)
	{
		uint8_t* Pixel = (uint8_t*)BackBuffer->Memory + X * BackBuffer->BytesPerPixel + Top * BackBuffer->Pitch;
		for (int Y = Top; Y < Bottom; ++Y)
		{
			*(uint32_t*)Pixel = Color;
			Pixel += BackBuffer->Pitch;
		}
	}
}

inline void Win32DrawSoundBufferMarker (win32_offscreen_buffer* BackBuffer, win32_sound_output* SoundOutput, float C, int PadX, int Top, int Bottom, DWORD Value, uint32_t Color)
{
	int X = PadX + (int)(C * (float)Value);
	if (X >= 0 && X < BackBuffer->Width)
	{
		Win32DebugDrawVertical(BackBuffer, X, Top, Bottom, Color);
	}
}

internal void Win32DebugSyncDisplay (win32_offscreen_buffer* BackBuffer, int MarkerCount, int CurrentMarkerIndex, win32_debug_time_marker* Markers, win32_sound_output* SoundOutput, float TargetSecondsPerFrame)
{
	//TODO Draw where we're writing out sound
	int PadX = 16;
	int PadY = 16;

	int LineHeight = 64;

	float C = ((float)BackBuffer->Width - 2 * PadX) / (float)SoundOutput->SecondaryBufferSize;
	for (int MarkerIndex = 0; MarkerIndex < MarkerCount; MarkerIndex++)
	{
		win32_debug_time_marker* ThisMarker = &Markers[MarkerIndex];
		DWORD PlayColor = 0x00ffffff;
		DWORD WriteColor = 0x00ff0000;
		DWORD ExpectedFlipColor = 0x00ffff00;
		DWORD PlayWindowColor = 0x00ff00ff;
		int Top = PadY;
		int Bottom = PadY + LineHeight;
		if (MarkerIndex == CurrentMarkerIndex)
		{
			Top += LineHeight + PadY;
			Bottom += LineHeight + PadY;

			int FirstTop = Top;

			Win32DrawSoundBufferMarker (BackBuffer, SoundOutput, C, PadX, Top, Bottom, ThisMarker->OutputPlayCursor, PlayColor);
			Win32DrawSoundBufferMarker (BackBuffer, SoundOutput, C, PadX, Top, Bottom, ThisMarker->OutputWriteCursor, WriteColor);

			Top += LineHeight + PadY;
			Bottom += LineHeight + PadY;

			Win32DrawSoundBufferMarker (BackBuffer, SoundOutput, C, PadX, Top, Bottom, ThisMarker->OutputLocation, PlayColor);
			Win32DrawSoundBufferMarker (BackBuffer, SoundOutput, C, PadX, Top, Bottom, ThisMarker->OutputLocation + ThisMarker->OutputByteCount, WriteColor);
			Top += LineHeight + PadY;
			Bottom += LineHeight + PadY;
			Win32DrawSoundBufferMarker (BackBuffer, SoundOutput, C, PadX, FirstTop, Bottom, ThisMarker->ExpectedFlipPlayCursor, ExpectedFlipColor);
		}

		Win32DrawSoundBufferMarker (BackBuffer, SoundOutput, C, PadX, Top, Bottom, ThisMarker->FlipPlayCursor, PlayColor);
		Win32DrawSoundBufferMarker (BackBuffer, SoundOutput, C, PadX, Top, Bottom, ThisMarker->FlipPlayCursor + 480 * SoundOutput->BytesPerSample, PlayWindowColor);
		Win32DrawSoundBufferMarker (BackBuffer, SoundOutput, C, PadX, Top, Bottom, ThisMarker->FlipWriteCursor, WriteColor);
	}
}

//Support for x input get state
#define X_INPUT_GET_STATE(name) DWORD WINAPI name(DWORD dwUserIndex, XINPUT_STATE* pState)
typedef X_INPUT_GET_STATE(x_input_get_state);
X_INPUT_GET_STATE(XInputGetStateStub)
{
	return (ERROR_DEVICE_NOT_CONNECTED);
}
global_var x_input_get_state* XInputGetState_ = XInputGetStateStub;
#define XInputGetState XInputGetState_

//support for x input set state
#define X_INPUT_SET_STATE(name) DWORD WINAPI name(DWORD dwUserIndex, XINPUT_VIBRATION* pVibration)
typedef X_INPUT_SET_STATE(x_input_set_state);
X_INPUT_SET_STATE(XInputSetStateStub)
{
	return (ERROR_DEVICE_NOT_CONNECTED);
}
global_var x_input_set_state* XInputSetState_ = XInputSetStateStub;
#define XInputSetState XInputSetState_

inline FILETIME Win32GetLastWriteTime(char* FileName)
{
	FILETIME LastWriteTime = {};

	WIN32_FILE_ATTRIBUTE_DATA Data;
	if (GetFileAttributesEx(FileName, GetFileExInfoStandard, &Data))
	{
		LastWriteTime = Data.ftLastWriteTime;
	}

	return LastWriteTime;
}

internal win32_game_code Win32LoadGameCode(char* SourceDLLName, char* TempDLLName)
{
	win32_game_code Result = {};

	Result.DLLLastWriteTime = Win32GetLastWriteTime(SourceDLLName);

	CopyFile(SourceDLLName, TempDLLName, FALSE);
	Result.GameCodeDLL = LoadLibraryA(TempDLLName);
	if (Result.GameCodeDLL)
	{
		Result.GetSoundSamples = (game_get_sound_samples*)GetProcAddress(Result.GameCodeDLL, "GameGetSoundSamples");
		Result.Update = (game_update*)GetProcAddress(Result.GameCodeDLL, "GameUpdate");
		Result.Render = (game_render*)GetProcAddress(Result.GameCodeDLL, "GameRender");
		Result.IsValid = (Result.GetSoundSamples && Result.Update && Result.Render);
	}
	else {
		Result.GetSoundSamples = 0;
		Result.Update = 0;
		Result.Render = 0;
		Result.IsValid = false;
	}

	return Result;
}

internal void Win32UnloadGameCode(win32_game_code* GameCode)
{
	if (GameCode->GameCodeDLL)
	{
		FreeLibrary(GameCode->GameCodeDLL);
		GameCode->GameCodeDLL = 0;
	}

	GameCode->GetSoundSamples = 0;
	GameCode->Update = 0;
	GameCode->Render = 0;
	GameCode->IsValid = false;
}

internal void Win32LoadXInput(void)
{
	//TODO: Test on Windows 8
	HMODULE XInputLibrary = LoadLibraryA("xinput1_4.dll");

	if (!XInputLibrary)
	{
		//TODO: Diagnostic
		XInputLibrary = LoadLibraryA("xinput1_3.dll");
	}

	if (!XInputLibrary)
	{
		//TODO: Diagnostic
		XInputLibrary = LoadLibraryA("xinput9_1_0.dll");
	}

	if (XInputLibrary)
	{
		XInputGetState_ = (x_input_get_state *)GetProcAddress(XInputLibrary, "XInputGetState");
		XInputSetState_ = (x_input_set_state *)GetProcAddress(XInputLibrary, "XInputSetState");
		//TODO: Diagnostic
	}
	else {
		//TODO: Diagnostic
	}

}

internal win32_window_dimension win32_GetWindowDimension(HWND hWnd)
{
	RECT ClientRect;
	GetClientRect(hWnd, &ClientRect);

	win32_window_dimension Window;

	Window.Width = ClientRect.right - ClientRect.left;
	Window.Height = ClientRect.bottom - ClientRect.top;

	return Window;
}

internal void Win32ResizeVisualBuffer(win32_offscreen_buffer* Buffer, int Width, int Height)
{
	//Bulletproof this
	//maybe dont free first, try free after first, then free first if that fails


	if(Buffer->Memory)
	{
		VirtualFree(Buffer->Memory, 0, MEM_RELEASE);
	}


	Buffer->Width = Width;
	Buffer->Height = Height;
	Buffer->BytesPerPixel = 4;

	Buffer->Info.bmiHeader.biSize = sizeof(Buffer->Info.bmiHeader);
	Buffer->Info.bmiHeader.biWidth = Buffer->Width;

	//note: when biHeight is negative it treats the memory passed as top-left start instead of bottom-left
	Buffer->Info.bmiHeader.biHeight = -Buffer->Height;
	Buffer->Info.bmiHeader.biPlanes = 1;
	Buffer->Info.bmiHeader.biBitCount = 32;
	Buffer->Info.bmiHeader.biCompression = BI_RGB;

	//TODO bulletproof this.

	int BitmapMemorySize = Buffer->BytesPerPixel * Buffer->Width * Buffer->Height;
	Buffer->Memory = VirtualAlloc(0, (SIZE_T)BitmapMemorySize, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
	Buffer->Pitch = Buffer->Width * Buffer->BytesPerPixel;

	return;
}

internal void Win32DrawToWindow(HDC DeviceContext, win32_offscreen_buffer* Buffer, int WindowWidth, int WindowHeight)
{

	//Prototyping means we'll blit 1-to-1 pixels so there won't be artifacts with stretching as we code the renderer
	int XOffset = (WindowWidth - Buffer->Width) / 2;
	int YOffset = (WindowHeight - Buffer->Height) / 2;

	if (XOffset < 0)
	{
		XOffset = 0;
	}
	if (YOffset < 0)
	{
		YOffset = 0;
	}

	PatBlt(DeviceContext, 0, 0, WindowWidth, YOffset, BLACKNESS);
	PatBlt(DeviceContext, 0, YOffset, XOffset, WindowHeight, BLACKNESS);
	PatBlt(DeviceContext, XOffset + Buffer->Width, 0, WindowWidth, WindowHeight, BLACKNESS);
	PatBlt(DeviceContext, 0, YOffset + Buffer->Height, WindowWidth, WindowHeight, BLACKNESS);
	StretchDIBits(DeviceContext,
			XOffset, YOffset, Buffer->Width, Buffer->Height,
			0, 0, Buffer->Width, Buffer->Height,
			Buffer->Memory, &Buffer->Info, DIB_RGB_COLORS, SRCCOPY);


}

LRESULT CALLBACK Win32MainWindowCallback(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	LRESULT Result = 0;

	switch (uMsg)
	{
		case WM_SYSKEYDOWN:
		case WM_SYSKEYUP:
		case WM_KEYDOWN:
		case WM_KEYUP:
			{
				Assert(!"Keyboard input has been dispatched instead of us handling it...");
			} break;

		case WM_DESTROY:
			{
				OutputDebugStringA("WM_DESTROY\n");
				GlobalRunning = false; //handle with error and recreate window
			} break;

		case WM_CLOSE:
			{
				OutputDebugStringA("WM_CLOSE\n");
				GlobalRunning = false; //replace with an are you sure kind of message
			} break;

		case WM_ACTIVATEAPP:
			{
				if (wParam)
				{
					SetLayeredWindowAttributes(hWnd, RGB(0, 0, 0), 255, LWA_ALPHA);
				}
				else {
					SetLayeredWindowAttributes(hWnd, RGB(0, 0, 0), 128, LWA_ALPHA);
				}
			} break;

		case WM_PAINT:
			{
				PAINTSTRUCT Paint;
				HDC DeviceContext = BeginPaint(hWnd, &Paint);
				int X = Paint.rcPaint.left;
				int Y = Paint.rcPaint.top;
				int Width = Paint.rcPaint.right - Paint.rcPaint.left;
				int Height = Paint.rcPaint.bottom - Paint.rcPaint.top;

				win32_window_dimension Dim = win32_GetWindowDimension(hWnd);
				Win32DrawToWindow(DeviceContext, &GlobalBuffer, Dim.Width, Dim.Height);

				EndPaint(hWnd, &Paint);
			} break;

		default:
			{
				//			OutputDebugStringA("default\n");
				Result = DefWindowProc(hWnd,uMsg,wParam,lParam);
			} break;
	}

	return (Result);
}

internal void Win32InitDSound(HWND Window, int32_t SamplesPerSecond, int32_t BufferSize)
{
	//note: load the library
	HMODULE DSoundLibrary = LoadLibraryA("dsound.dll");

	if (DSoundLibrary)
	{

		//note: get a DirectSound object! - cooperative mode
		direct_sound_create* DirectSoundCreate = (direct_sound_create*)(GetProcAddress(DSoundLibrary, "DirectSoundCreate"));

		//Double check this works on XP
		LPDIRECTSOUND DirectSound;
		if (DirectSoundCreate && SUCCEEDED(DirectSoundCreate(0, &DirectSound, 0)))
		{
			WAVEFORMATEX WaveFormat = {};
			WaveFormat.wFormatTag = WAVE_FORMAT_PCM;
			WaveFormat.nChannels = 2;
			WaveFormat.nSamplesPerSec = (DWORD)SamplesPerSecond;
			WaveFormat.wBitsPerSample = 16;
			WaveFormat.nBlockAlign = (WORD)(WaveFormat.nChannels*WaveFormat.wBitsPerSample / 8);
			WaveFormat.nAvgBytesPerSec = WaveFormat.nSamplesPerSec*WaveFormat.nBlockAlign;
			WaveFormat.cbSize;
			if(SUCCEEDED(DirectSound->SetCooperativeLevel(Window, DSSCL_PRIORITY)))
			{
				DSBUFFERDESC BufferDescription = {};
				BufferDescription.dwSize = sizeof(BufferDescription);
				BufferDescription.dwFlags = DSBCAPS_PRIMARYBUFFER;

				//TODO DSBCAPS_GLABALFOCUS?
				LPDIRECTSOUNDBUFFER PrimaryBuffer;
				HRESULT Error = DirectSound->CreateSoundBuffer(&BufferDescription, &PrimaryBuffer, 0);
				if (SUCCEEDED(Error))
				{
					if(SUCCEEDED(PrimaryBuffer->SetFormat(&WaveFormat)))
					{
						//we finally set the format
						OutputDebugStringA("Primary buffer format set.\n");
					}
					else {
						//TODO: loggirg
					}
				}
				else {
					//TODO: logging
				}
			}
			else {
				//Diagnostic
			}
			//TODO DBSCAPS_GETCURRENTPOSITION2
			DSBUFFERDESC BufferDescription = {};
			BufferDescription.dwSize = sizeof(BufferDescription);
			BufferDescription.dwFlags = 0;
			BufferDescription.dwBufferBytes = (DWORD)BufferSize;
			BufferDescription.lpwfxFormat = &WaveFormat;

			HRESULT Error = DirectSound->CreateSoundBuffer(&BufferDescription, &GlobalSecondaryBuffer, 0);	
			if (SUCCEEDED(Error))
			{	
				OutputDebugStringA("Secondary buffer set.\n");
			}
		}
		else {
			//TODO: Diagnostic
		}
	}
	else {
		//TODO: Diagnostic
	}
}

internal void Win32ClearSoundBuffer (win32_sound_output* SoundOutput)
{
	VOID* Region1;
	DWORD Region1Size;
	VOID* Region2;
	DWORD Region2Size;

	if(SUCCEEDED(GlobalSecondaryBuffer->Lock(0, (DWORD)SoundOutput->SecondaryBufferSize, &Region1, &Region1Size, &Region2, &Region2Size, 0)))
	{
		uint8_t* DestSample = (uint8_t*)Region1;
		for (DWORD ByteIndex = 0; ByteIndex < Region1Size; ByteIndex++)
		{
			*DestSample++ = 0;
		}
		DestSample = (uint8_t*)Region2;
		for (DWORD ByteIndex = 0; ByteIndex < Region2Size; ByteIndex++)
		{
			*DestSample++ = 0;
		}
		GlobalSecondaryBuffer->Unlock(Region1, Region1Size, Region2, Region2Size);
	}
}

internal void Win32FillSoundBuffer (win32_sound_output* SoundOutput, DWORD ByteToStartLock, DWORD BytesToWrite, game_sound_output_buffer *SourceBuffer)
{
	VOID* Region1;
	DWORD Region1Size;
	VOID* Region2;
	DWORD Region2Size;

	if(SUCCEEDED(GlobalSecondaryBuffer->Lock(ByteToStartLock, BytesToWrite, &Region1, &Region1Size, &Region2, &Region2Size, 0)))
	{
		//TODO: assert Region1Size and 2 are valid
		//TODO: collapse these two loops
		DWORD Region1SampleCount = Region1Size / SoundOutput->BytesPerSample;
		int16_t* DestSample = (int16_t*)Region1;
		int16_t* SourceSample = SourceBuffer->Samples;
		for (DWORD SampleIndex = 0; SampleIndex < Region1SampleCount; SampleIndex++)
		{
			*DestSample++ = *SourceSample++;
			*DestSample++ = *SourceSample++;
			SoundOutput->RunningSampleIndex++;
		}

		DWORD Region2SampleCount = Region2Size/SoundOutput->BytesPerSample;
		DestSample = (int16_t*)Region2;
		for (DWORD SampleIndex = 0; SampleIndex < Region2SampleCount; SampleIndex++)
		{
			*DestSample++ = *SourceSample++;
			*DestSample++ = *SourceSample++;
			SoundOutput->RunningSampleIndex++;
		}

		GlobalSecondaryBuffer->Unlock(Region1, Region1Size, Region2, Region2Size);
	}
}

internal void Win32ProcessXInputDigitalButton(DWORD XInputButtonState, game_button_state* OldButtonState, game_button_state* NewButtonState, DWORD ButtonBit)
{
	NewButtonState->EndedDown = !!(XInputButtonState & ButtonBit);
	NewButtonState->HalfTransitionCount = (OldButtonState->EndedDown != NewButtonState->EndedDown) ? 1 : 0;
}

internal void Win32ProcessKeyboardStateChange(game_button_state* NewButtonState, boolint IsDown)
{
	if (NewButtonState->EndedDown != IsDown)
	{
		NewButtonState->EndedDown = !!IsDown;
		NewButtonState->HalfTransitionCount++;
	}
}

internal float Win32AnalogTriggerProcessingAndDeadzone(BYTE Trigger)
{
	float Result = 0;

	if (Trigger > 30)
	{
		Result = (float)Trigger / 255.0f;
	}
	return Result;
}

internal void Win32AnalogRoundDeadzone(float* AnalogTriggers[], SHORT* ControllerInputStick[], int XIndex, float Deadzone)
{
	int YIndex = XIndex + 1;

	double Hypotenuse = sqrt((double)*ControllerInputStick[XIndex] * (double)*ControllerInputStick[XIndex] + (double)*ControllerInputStick[YIndex] * (double)*ControllerInputStick[YIndex]) / 32768.0;

	if (Hypotenuse <= Deadzone && Hypotenuse >= -Deadzone)
	{
		*AnalogTriggers[XIndex] = 0.0f;
		*AnalogTriggers[YIndex] = 0.0f;
	}
	else {
		*AnalogTriggers[XIndex] = (float)*ControllerInputStick[XIndex] / 32767.0f / (float) Hypotenuse * ((float)Hypotenuse - Deadzone);
		*AnalogTriggers[YIndex] = (float)*ControllerInputStick[YIndex] / 32767.0f / (float) Hypotenuse * ((float)Hypotenuse - Deadzone);
	}

	return;
}

internal void Win32AnalogProcessing(XINPUT_GAMEPAD* Pad, game_controller_input* Controller)
{
	float LT, RT, LX, LY, RX, RY;
	float* AnalogTriggers [6] = {&LT, &RT, &LX, &LY, &RX, &RY};

	BYTE* ControllerInputTrigger[2] = {&Pad->bLeftTrigger, &Pad->bRightTrigger};
	SHORT* ControllerInputStick[6] = {0, 0, &Pad->sThumbLX, &Pad->sThumbLY, &Pad->sThumbRX, &Pad->sThumbRY};

	float Deadzone = .10f;
	for (int AnalogIndex = 0; AnalogIndex < 6; AnalogIndex++)
	{	
		if	(AnalogIndex < 2)
		{

			*AnalogTriggers[AnalogIndex] = Win32AnalogTriggerProcessingAndDeadzone(*ControllerInputTrigger[AnalogIndex]);
			if (*AnalogTriggers[AnalogIndex] > (1.0f - Deadzone) && Controller->Buttons[AnalogIndex].EndedDown == 0)
			{
				Controller->Buttons[AnalogIndex].EndedDown = 1;
				Controller->Buttons[AnalogIndex].HalfTransitionCount++;
			}
			else if (*AnalogTriggers[AnalogIndex] < Deadzone && Controller->Buttons[AnalogIndex].EndedDown == 1)
			{
				Controller->Buttons[AnalogIndex].EndedDown = 0;
				Controller->Buttons[AnalogIndex].HalfTransitionCount++;
			}
			Controller->Analogs[AnalogIndex].Average *= Controller->Analogs[AnalogIndex].NumberSoFar;
			Controller->Analogs[AnalogIndex].Average += *AnalogTriggers[AnalogIndex];
			Controller->Analogs[AnalogIndex].NumberSoFar++;
			Controller->Analogs[AnalogIndex].Average /= Controller->Analogs[AnalogIndex].NumberSoFar;
		}
		else if (AnalogIndex % 2 == 0)
		{
			Win32AnalogRoundDeadzone(AnalogTriggers, ControllerInputStick, AnalogIndex, Deadzone);

			for (int XOrY = 0; XOrY < 2; XOrY++)
			{
				int ButtonIndex = AnalogIndex * 2 - 2 + XOrY * 2;
				if (*AnalogTriggers[AnalogIndex] > (1.0f - Deadzone) && Controller->Buttons[ButtonIndex].EndedDown == 0)
				{
					Controller->Buttons[ButtonIndex].EndedDown = 1;
					Controller->Buttons[ButtonIndex].HalfTransitionCount++;
				}
				else if (*AnalogTriggers[AnalogIndex] < Deadzone && Controller->Buttons[ButtonIndex].EndedDown == 1)
				{
					Controller->Buttons[ButtonIndex].EndedDown = 0;
					Controller->Buttons[ButtonIndex].HalfTransitionCount++;
				}
				ButtonIndex++;
				if (-(*AnalogTriggers[AnalogIndex]) > (1.0f - Deadzone) && Controller->Buttons[ButtonIndex].EndedDown == 0)
				{
					Controller->Buttons[ButtonIndex].EndedDown = 1;
					Controller->Buttons[ButtonIndex].HalfTransitionCount++;
				}
				else if (-(*AnalogTriggers[AnalogIndex]) < Deadzone && Controller->Buttons[ButtonIndex].EndedDown == 1)
				{
					Controller->Buttons[ButtonIndex].EndedDown = 0;
					Controller->Buttons[ButtonIndex].HalfTransitionCount++;
				}

				Controller->Analogs[AnalogIndex + XOrY].Average *= Controller->Analogs[AnalogIndex + XOrY].NumberSoFar;
				Controller->Analogs[AnalogIndex + XOrY].Average += *AnalogTriggers[AnalogIndex + XOrY];
				Controller->Analogs[AnalogIndex + XOrY].NumberSoFar++;
				Controller->Analogs[AnalogIndex + XOrY].Average /= Controller->Analogs[AnalogIndex + XOrY].NumberSoFar;
			}
		}
	}
}

inline LARGE_INTEGER Win32GetWallClock(void)
{
	LARGE_INTEGER Result;
	QueryPerformanceCounter(&Result);
	return Result;
}

inline float Win32GetSecondsElapsed(LARGE_INTEGER Start, LARGE_INTEGER End)
{
	float Result = (float)(End.QuadPart - Start.QuadPart) / (float) GlobalPerfCountFrequency;
	return Result;
}

internal void Win32GetInputFileLocation(win32_state* State, int SlotIndex, boolint InputStream, int DestCount, char* Dest)
{
	char Temp[64];
	wsprintf(Temp, "loop_edit_%d_%s.hmi", SlotIndex, InputStream ? "input" : "state");
	Win32BuildExePathFileName(State, Temp, DestCount, Dest);
}

internal win32_replay_buffer* Win32GetReplayBuffer(win32_state* State, int Index)
{
	Assert(Index < ArrayCount(State->ReplayBuffers) && Index >= 0);
	win32_replay_buffer* Result = &State->ReplayBuffers[Index];
	return Result;
}

internal void Win32BeginRecordingInput(win32_state* State, int InputRecordingIndex)
{
	win32_replay_buffer* ReplayBuffer = Win32GetReplayBuffer(State, InputRecordingIndex);

	if(ReplayBuffer->MemoryBlock)
	{
		State->InputRecordingIndex = InputRecordingIndex;

		char FileName[WIN32_STATE_FILE_NAME_COUNT];
		Win32GetInputFileLocation(State, InputRecordingIndex, true, sizeof(FileName) - 1, FileName);
		State->RecordingHandle = CreateFile(FileName, GENERIC_WRITE, 0, 0, CREATE_ALWAYS, 0, 0);

		CopyMemory(ReplayBuffer->MemoryBlock, State->GameMemoryBlock, State->TotalSize);
	}
}

internal void Win32EndRecordingInput(win32_state* State)
{
	CloseHandle(State->RecordingHandle);
	State->InputRecordingIndex = -1;
}

internal void Win32BeginPlaybackInput(win32_state* State, int InputPlaybackIndex)
{
	win32_replay_buffer* ReplayBuffer = Win32GetReplayBuffer(State, InputPlaybackIndex);

	if(ReplayBuffer->MemoryBlock)
	{

		State->InputPlaybackIndex = InputPlaybackIndex;

		char FileName[WIN32_STATE_FILE_NAME_COUNT];
		Win32GetInputFileLocation(State, InputPlaybackIndex, true, sizeof(FileName) - 1, FileName);
		State->PlaybackHandle = CreateFile(FileName, GENERIC_READ, 0, 0, OPEN_EXISTING, 0, 0);

		CopyMemory(State->GameMemoryBlock, ReplayBuffer->MemoryBlock, State->TotalSize);
	}
}

internal void Win32EndPlaybackInput(win32_state* State)
{
	CloseHandle(State->PlaybackHandle);
	State->InputPlaybackIndex = -1;
}

internal void Win32RecordInput(win32_state* State, game_input* NewInput)
{
	DWORD BytesWritten;
	WriteFile(State->RecordingHandle, NewInput, sizeof(*NewInput), &BytesWritten, 0);

}

internal void Win32PlaybackInput(win32_state* State, game_input* NewInput)
{
	DWORD BytesRead = 0;
	if (ReadFile(State->PlaybackHandle, NewInput, sizeof(*NewInput), &BytesRead, 0))
	{
		if (BytesRead == 0)
		{
			//Note: We ran out of input, so loop this bitch
			int PlaybackIndex = State->InputPlaybackIndex;
			Win32EndPlaybackInput(State);
			Win32BeginPlaybackInput(State, PlaybackIndex);
			ReadFile(State->PlaybackHandle, NewInput, sizeof(*NewInput), &BytesRead, 0);
		}
	}
}

internal void Win32ProcessPendingMessages(win32_state* State, game_controller_input* KeyboardController)
{
	MSG Message;
	while(PeekMessage(&Message, 0, 0, 0, PM_REMOVE))
	{
		if (Message.message == WM_QUIT)
		{
			GlobalRunning = false;
		}

		switch (Message.message)
		{
			case WM_SYSKEYDOWN:
			case WM_SYSKEYUP:
			case WM_KEYDOWN:
			case WM_KEYUP:
				{
					uint64_t VKCode = Message.wParam;
					bool WasDown = ((Message.lParam & (1 << 30)) != 0);
					bool IsDown = ((Message.lParam & (1 << 31)) == 0);
					if (WasDown != IsDown)
					{
						switch (VKCode)
						{
							//TODO: consider, what, a table of function pointers to carry out rebinding keys, and switch the pointer values or something similar? for rebindable keys
							case VK_OEM_COMMA:
								{ //,
									Win32ProcessKeyboardStateChange(&KeyboardController->LeftStickUp, IsDown);

								} break;

							case 'A':
								{
									Win32ProcessKeyboardStateChange(&KeyboardController->LeftStickLeft, IsDown);


								} break;

							case 'O':
								{
									Win32ProcessKeyboardStateChange(&KeyboardController->LeftStickDown, IsDown);

								} break;

							case 'E':
								{
									Win32ProcessKeyboardStateChange(&KeyboardController->LeftStickRight, IsDown);

								} break;

							case VK_OEM_7:
								{ //'
									Win32ProcessKeyboardStateChange(&KeyboardController->LeftShoulder, IsDown);
								} break;

							case VK_OEM_PERIOD:
								{
									Win32ProcessKeyboardStateChange(&KeyboardController->RightShoulder, IsDown);
								} break;

							case VK_UP:
								{
									Win32ProcessKeyboardStateChange(&KeyboardController->YButton, IsDown);
								} break;

							case VK_DOWN:
								{
									Win32ProcessKeyboardStateChange(&KeyboardController->AButton, IsDown);
								} break;

							case VK_RIGHT:
								{
									Win32ProcessKeyboardStateChange(&KeyboardController->BButton, IsDown);
								} break;

							case VK_LEFT:
								{
									Win32ProcessKeyboardStateChange(&KeyboardController->XButton, IsDown);
								} break;

							case VK_LBUTTON:
								{
								} break;

							case VK_RBUTTON:
								{
								} break;

							case VK_ESCAPE:
								{
									Win32ProcessKeyboardStateChange(&KeyboardController->Back, IsDown);
								} break;

							case VK_RETURN:
								{
									Win32ProcessKeyboardStateChange(&KeyboardController->Start, IsDown);
								} break;

							case VK_SPACE:
								{
									if (IsDown)
									{
										GlobalPause = !GlobalPause;
									}
								} break;
							
							case 'M':
								{
									if (IsDown)
									{
										GlobalFast = !GlobalFast;
									}
								} break;

							case 'F':
								{
									if (IsDown)
									{
										Win32EndPlaybackInput(State);
										if (State->InputRecordingIndex == -1)
										{
											Win32BeginRecordingInput(State, State->ReplayBufferChoice);
										}
										else {
											Win32EndRecordingInput(State);
											Win32BeginPlaybackInput(State, State->ReplayBufferChoice);
										}
									}
								} break;

							case 'G':
								{
									if (IsDown)
									{
										Win32EndRecordingInput(State);
										Win32EndPlaybackInput(State);
										Win32BeginPlaybackInput(State, State->ReplayBufferChoice);
									}
								} break;

							case 'C':
								{
									if (IsDown)
									{
										Win32EndRecordingInput(State);
										Win32EndPlaybackInput(State);
									}
								} break;

							case '1':
							case '2':
							case '3':
							case '4':
								{
									State->ReplayBufferChoice = (int)(VKCode - 0x31); //translates vkcode of '1' to the int 0
								} break;
						}
					}

					if ((VKCode == VK_F4) && (Message.lParam & (1 << 29)))
					{
						GlobalRunning = false;
					}


				} break;

			default:
				{
					TranslateMessage(&Message);
					DispatchMessageA(&Message);
				} break;

		}
	}
}

internal void Win32GetExeFileName(win32_state* State)
{
	//Note: never use Max Path in user-facing code - dangerous, can lead to bad results
	DWORD SizeOfFileName = GetModuleFileNameA(0, State->ExeFileName, sizeof(State->ExeFileName));
	State->OnePastLastExeFileNameSlash = State->ExeFileName;
	for (char* Scan = State->ExeFileName; *Scan; ++Scan)
	{
		if (*Scan == '\\')
		{
			State->OnePastLastExeFileNameSlash = Scan + 1;
		}
	}

}

void GetInput(win32_state* State, HWND Window, game_input* OldInput, game_input* NewInput)
{

	game_controller_input* OldKeyboardInput = GetController(OldInput, 0);
	game_controller_input* NewKeyboardInput = GetController(NewInput, 0);
	*NewKeyboardInput = *OldKeyboardInput;
	NewKeyboardInput->IsConnected = true;
	for (int ButtonIndex = 0; ButtonIndex < ArrayCount(NewKeyboardInput->Buttons); ButtonIndex++)
	{
		NewKeyboardInput->Buttons[ButtonIndex].HalfTransitionCount = 0;
	}							

	Win32ProcessPendingMessages(State, NewKeyboardInput);


	POINT MouseP;
	GetCursorPos(&MouseP);
	ScreenToClient(Window, &MouseP);
	NewInput->MouseX = MouseP.x;
	NewInput->MouseY = MouseP.y;
	NewInput->MouseZ = 0; //TODO: Support Mousewheel?
	Win32ProcessKeyboardStateChange(&NewInput->MouseButtons[0], GetKeyState(VK_LBUTTON) & (1 << 15));
	Win32ProcessKeyboardStateChange(&NewInput->MouseButtons[1], GetKeyState(VK_MBUTTON) & (1 << 15));
	Win32ProcessKeyboardStateChange(&NewInput->MouseButtons[2], GetKeyState(VK_RBUTTON) & (1 << 15));
	Win32ProcessKeyboardStateChange(&NewInput->MouseButtons[3], GetKeyState(VK_XBUTTON1) & (1 << 15));
	Win32ProcessKeyboardStateChange(&NewInput->MouseButtons[4], GetKeyState(VK_XBUTTON2) & (1 << 15));

	//TODO: Need to not poll disconnected controllers to avoid framerate hit on older XINPUT libraries - check for controllers intermittently
	//TODO: Should we poll this more frequently?
	DWORD MaxControllerCount = XUSER_MAX_COUNT;
	if(MaxControllerCount > ArrayCount(NewInput->Controllers) - 1)
	{
		MaxControllerCount = ArrayCount(NewInput->Controllers) - 1;
	}
	for(DWORD ControllerIndex = 0; ControllerIndex < MaxControllerCount; ControllerIndex++)
	{

		DWORD OurControllerIndex = ControllerIndex + 1;
		game_controller_input* OldController = GetController(OldInput, (int)OurControllerIndex);
		game_controller_input* NewController = GetController(NewInput, (int)OurControllerIndex);


		XINPUT_STATE ControllerState;
		if(XInputGetState(ControllerIndex, &ControllerState) == ERROR_SUCCESS)
		{
			NewController->IsConnected = true;
			NewController->IsAnalog = OldController->IsAnalog;
			NewController->LX.NumberSoFar = 0;
			NewController->LY.NumberSoFar = 0;
			NewController->RX.NumberSoFar = 0;
			NewController->RY.NumberSoFar = 0;

			for (int ButtonIndex = 0; ButtonIndex < ArrayCount(NewController->Buttons); ButtonIndex++)
			{
				NewController->Buttons[ButtonIndex].HalfTransitionCount = 0;
			}							

			//See if ControllerState.dwPacketNumber increments too rapidly
			//Controller is plugged in

			XINPUT_GAMEPAD* Pad = &ControllerState.Gamepad;

			//TODO: implement DPAD and everything else
			boolint Up = (Pad->wButtons & XINPUT_GAMEPAD_DPAD_UP);
			boolint Down = (Pad->wButtons & XINPUT_GAMEPAD_DPAD_DOWN);
			boolint Left = (Pad->wButtons & XINPUT_GAMEPAD_DPAD_LEFT);
			boolint Right = (Pad->wButtons & XINPUT_GAMEPAD_DPAD_RIGHT);

			boolint Start = (Pad->wButtons & XINPUT_GAMEPAD_START);
			boolint Back = (Pad->wButtons & XINPUT_GAMEPAD_BACK);
			boolint LeftAnalog = (Pad->wButtons & XINPUT_GAMEPAD_LEFT_THUMB);
			boolint RightAnalog = (Pad->wButtons & XINPUT_GAMEPAD_RIGHT_THUMB);

			Win32AnalogProcessing(Pad, NewController);

			//TODO: Min/Max Macros!

			Win32ProcessXInputDigitalButton(Pad->wButtons, &OldController->AButton, &NewController->AButton, XINPUT_GAMEPAD_A);
			Win32ProcessXInputDigitalButton(Pad->wButtons, &OldController->BButton, &NewController->BButton, XINPUT_GAMEPAD_B);
			Win32ProcessXInputDigitalButton(Pad->wButtons, &OldController->XButton, &NewController->XButton, XINPUT_GAMEPAD_X);
			Win32ProcessXInputDigitalButton(Pad->wButtons, &OldController->YButton, &NewController->YButton, XINPUT_GAMEPAD_Y);
			Win32ProcessXInputDigitalButton(Pad->wButtons, &OldController->LeftShoulder, &NewController->LeftShoulder, XINPUT_GAMEPAD_LEFT_SHOULDER);
			Win32ProcessXInputDigitalButton(Pad->wButtons, &OldController->RightShoulder, &NewController->RightShoulder, XINPUT_GAMEPAD_RIGHT_SHOULDER);
			Win32ProcessXInputDigitalButton(Pad->wButtons, &OldController->Back, &NewController->Back, XINPUT_GAMEPAD_BACK);
			Win32ProcessXInputDigitalButton(Pad->wButtons, &OldController->Start, &NewController->Start, XINPUT_GAMEPAD_START);


		}
		else {
			NewController->IsConnected = false;
			//Controller is not available
		}
	}

	return;
}

void ResetInput(game_input* OldInput, game_input* NewInput)
{
	game_input* Temp = NewInput;
	NewInput = OldInput;
	OldInput = Temp;

	return;
}

int CALLBACK WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int ShowCode)
{
	win32_state State = {};
	State.InputRecordingIndex = -1;
	State.InputPlaybackIndex = -1;
	Win32GetExeFileName(&State);

	char SourceGameCodeDLLFullPath[WIN32_STATE_FILE_NAME_COUNT];
	Win32BuildExePathFileName(&State, "base.dll", sizeof(SourceGameCodeDLLFullPath), SourceGameCodeDLLFullPath);

	char TempGameCodeDLLFullPath[WIN32_STATE_FILE_NAME_COUNT];
	Win32BuildExePathFileName(&State, "base.tmp.dll", sizeof(TempGameCodeDLLFullPath), TempGameCodeDLLFullPath);

	LARGE_INTEGER PerfCountFrequencyResult;
	QueryPerformanceFrequency(&PerfCountFrequencyResult);
	GlobalPerfCountFrequency = PerfCountFrequencyResult.QuadPart;

	//note: set the windows scheduler granularity to 1ms
	UINT DesiredSchedulerMilliseconds = 1;
	boolint SleepIsGranular = (timeBeginPeriod(DesiredSchedulerMilliseconds) == TIMERR_NOERROR);

	Win32LoadXInput();

	WNDCLASSA WindowClass = {};

	//1080p will be 1920x1080 - wait until gpu is drawing stuff	
	int WindowWidth = 960;
	int WindowHeight = 540;

	Win32ResizeVisualBuffer(&GlobalBuffer, WindowWidth, WindowHeight);

	WindowClass.style = CS_HREDRAW|CS_VREDRAW|CS_OWNDC;
	WindowClass.lpfnWndProc = Win32MainWindowCallback;
	WindowClass.hInstance = hInstance;
	WindowClass.lpszClassName = "GposClass";

	if (RegisterClassA(&WindowClass))
	{
		HWND Window =
			CreateWindowEx(
					/*WS_EX_TOPMOST|*/WS_EX_LAYERED,
					WindowClass.lpszClassName,
					"GIANT PIECE OF SHIT",
					WS_OVERLAPPEDWINDOW|WS_VISIBLE,
					100,
					100,
					WindowWidth + 16,
					WindowHeight + 40,
					0,
					0,
					hInstance,
					0);
		if(Window)
		{
			//set the refresh rate
			DEVMODE CurrentDisplayDevice = {};
			CurrentDisplayDevice.dmSize = sizeof(DEVMODE);
			CurrentDisplayDevice.dmDriverExtra = 0;
			float MonitorRefreshHz = 60.0f;
			/*his code
			  HDC RefreshDC = GetDC(Window);
			  int Win32RefreshRate = GetDeviceCaps(RefreshDC, VREFRESH);
			  if (Win32RefreshRate > 1)
			  {
			  MonitorRefreshHz = (float)Win32RefreshRate;
			  }
			  ReleaseDC(Window, RefreshDC);
			  *///end his code
			//my code
			if (EnumDisplaySettings(0, ENUM_CURRENT_SETTINGS,  &CurrentDisplayDevice))
			{
				MonitorRefreshHz = (float)CurrentDisplayDevice.dmDisplayFrequency;
			}
			//my code

			float GameUpdateHz = MonitorRefreshHz;//0.5f;
			float TargetSecondsPerFrame = 1.0f / GameUpdateHz;

			//Graphics stuff
			int XOffset = 0;
			int YOffset = 0;




			//change buffer size to 60 seconds later (once starting sound fx stuff)
			win32_sound_output SoundOutput = {};

			SoundOutput.SamplesPerSecond = 48000;
			SoundOutput.RunningSampleIndex = 0;
			SoundOutput.BytesPerSample = sizeof(int16_t)*2;
			SoundOutput.SecondaryBufferSize = SoundOutput.SamplesPerSecond*SoundOutput.BytesPerSample;
			SoundOutput.SineTime = 0;
			//TODO actually compute this variance to find the lowest reasonable value
			SoundOutput.SafetySampleBytes = (int)((float)SoundOutput.SamplesPerSecond * (float)SoundOutput.BytesPerSample / (float)GameUpdateHz * 0.5f);

			Win32InitDSound(Window, SoundOutput.SamplesPerSecond, SoundOutput.SecondaryBufferSize);
			Win32ClearSoundBuffer(&SoundOutput);
			GlobalSecondaryBuffer->Play(0, 0, DSBPLAY_LOOPING);

			//Pool the VirtualAlloc over and such
			int16_t* Samples = (int16_t*)VirtualAlloc(0, (SIZE_T)SoundOutput.SecondaryBufferSize, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
#if INTERNAL_BUILD
			LPVOID BaseAddress = (LPVOID)Terabytes(1);
#else
			LPVOID BaseAddress = 0;
#endif


			game_memory GameMemory = {};
			GameMemory.PermanentStorageSize = Megabytes(64);
			GameMemory.TransientStorageSize = Gigabytes(1);
			GameMemory.DEBUGPlatformReadEntireFile = DEBUGPlatformReadEntireFile;
			GameMemory.DEBUGPlatformFreeFileMemory = DEBUGPlatformFreeFileMemory;
			GameMemory.DEBUGPlatformWriteEntireFile = DEBUGPlatformWriteEntireFile;

			//TODO: handle various memory footprints (USING SYSTEM METRICS)
			//TODO: Use MEM_LARGE_PAGES when not on Windows xp?
			//TODO: Split TransientStorage into game transient and cache transient
			//Only game transient need be stored for the replay feature
			State.TotalSize = GameMemory.PermanentStorageSize + GameMemory.TransientStorageSize;
			State.GameMemoryBlock = VirtualAlloc(BaseAddress, (SIZE_T)State.TotalSize, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
			GameMemory.PermanentStorage = State.GameMemoryBlock;
			GameMemory.TransientStorage = ((uint8_t*)GameMemory.PermanentStorage + GameMemory.PermanentStorageSize);

			for (int ReplayIndex = 0; ReplayIndex < ArrayCount(State.ReplayBuffers); ReplayIndex++)
			{
				win32_replay_buffer* ReplayBuffer = &State.ReplayBuffers[ReplayIndex];

				Win32GetInputFileLocation(&State, ReplayIndex, false, sizeof(ReplayBuffer->FileName) - 1, ReplayBuffer->FileName);

				ReplayBuffer->FileHandle = CreateFile(ReplayBuffer->FileName, GENERIC_READ|GENERIC_WRITE, 0, 0, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);

				LARGE_INTEGER FileSize;
				FileSize.QuadPart = (LONGLONG)State.TotalSize;
				ReplayBuffer->MemoryMap = CreateFileMapping(ReplayBuffer->FileHandle, 0, PAGE_READWRITE, (DWORD)FileSize.HighPart, FileSize.LowPart, 0);

				ReplayBuffer->MemoryBlock = MapViewOfFile(ReplayBuffer->MemoryMap, FILE_MAP_ALL_ACCESS, 0, 0, State.TotalSize);

				if(ReplayBuffer->MemoryBlock)
				{
				}
				else {
					//TODO: Diagnostic
				}
			}

			if (Samples && GameMemory.PermanentStorage && GameMemory.TransientStorage)
			{
				int DebugTimeMarkerIndex = 0;
				win32_debug_time_marker DebugTimeMarkers[30] = {};

				//input
				game_input Input[2] = {};
				game_input* NewInput = &Input[0];
				game_input* OldInput = &Input[1];

				//time measurement
				LARGE_INTEGER LastCounter = Win32GetWallClock();
				LARGE_INTEGER FlipWallClock = Win32GetWallClock();
				float UpdateAndInputTime = 0;
				float RenderTime = 0;

				int64_t CyclesPerSecond = GlobalPerfCountFrequency;


				boolint SoundIsValid = false;
				DWORD AudioLatencyBytes = 0;
				float AudioLatencySeconds = 0;

				GlobalRunning = true;
				GlobalPause = false;
				GlobalFast = true;
				win32_game_code Game = Win32LoadGameCode(SourceGameCodeDLLFullPath, TempGameCodeDLLFullPath);

				uint64_t LastCycleCount = __rdtsc();
				while (GlobalRunning)
				{
					NewInput->dtForFrame = TargetSecondsPerFrame;

					FILETIME NewDLLWriteTime = Win32GetLastWriteTime(SourceGameCodeDLLFullPath);
					if (CompareFileTime(&NewDLLWriteTime, &Game.DLLLastWriteTime) != 0)
					{
						Win32UnloadGameCode(&Game);
						Game = Win32LoadGameCode(SourceGameCodeDLLFullPath, TempGameCodeDLLFullPath);
					}

					LARGE_INTEGER PreInput = Win32GetWallClock();
					GetInput(&State, Window, OldInput, NewInput);

					thread_context Thread = {};

					game_offscreen_buffer Buffer = {};
					Buffer.Memory = GlobalBuffer.Memory;
					Buffer.Width = GlobalBuffer.Width;
					Buffer.Height = GlobalBuffer.Height;
					Buffer.Pitch = GlobalBuffer.Pitch;
					Buffer.BytesPerPixel = GlobalBuffer.BytesPerPixel;

					if (State.InputRecordingIndex != -1)
					{
						Win32RecordInput(&State, NewInput);
					}
					if (State.InputPlaybackIndex != -1)
					{
						Win32PlaybackInput(&State, NewInput);
					}

					if (!GlobalPause)
					{

						if(Game.Update)
						{
							for (int UpdateIndex = 0; UpdateIndex < NUMTESTCYCLES; UpdateIndex++)
							{
								if (UpdateIndex == NUMTESTCYCLES - 5 && Game.Render)
								{
									Game.Render(&Thread, &GameMemory, &Buffer);
								}
								Game.Update(&Thread, &GameMemory, NewInput);
							}
						}
						LARGE_INTEGER PostUpdate = Win32GetWallClock();
						UpdateAndInputTime = Win32GetSecondsElapsed(PreInput, PostUpdate);

#if 0
						LARGE_INTEGER AudioWallClock = Win32GetWallClock();
						float FromBeginToAudioSeconds = Win32GetSecondsElapsed(FlipWallClock, AudioWallClock);

						DWORD PlayCursor;
						DWORD WriteCursor;
						if(SUCCEEDED(GlobalSecondaryBuffer->GetCurrentPosition(&PlayCursor, &WriteCursor)))
						{

							/*
							 * Here is how the sound output computation works:
							 *
							 *	We define a safety value that is the number of samples
							 *	we think our game update loop may vary by (lets say up to 2 ms).
							 *
							 * When we wake to write audio, we'll look and see
							 * what the paly cursor position is and then forecast ahead
							 * where we think the play cursor will be on the next frame boundary.
							 *
							 * We'll then look to see if the write cursor is before that
							 * by at least our safety value.
							 * If it is, the target fill position is that frame boundary plus one
							 * frame, giving us perfect audio sync in the case of a low latency card.
							 *
							 * If the write cursor is AFTER that safety margin,
							 * then we assume we can never sync the audio perfectly,
							 * so we'll write one frame's worth of audio plus the safety
							 * margin's worth of guard samples.
							 */
							if (!SoundIsValid)
							{
								SoundOutput.RunningSampleIndex = WriteCursor / SoundOutput.BytesPerSample;
								SoundIsValid = true;
							}


							DWORD ByteToStartLock = (SoundOutput.RunningSampleIndex*SoundOutput.BytesPerSample) % SoundOutput.SecondaryBufferSize;

							DWORD ExpectedSoundBytesPerFrame = (DWORD)((float)SoundOutput.SamplesPerSecond * (float)SoundOutput.BytesPerSample / GameUpdateHz);
							float SecondsLeftUntilFlip = TargetSecondsPerFrame - FromBeginToAudioSeconds;
							DWORD ExpectedBytesUntilFlip = (DWORD)((SecondsLeftUntilFlip/TargetSecondsPerFrame)*(float)ExpectedSoundBytesPerFrame);

							DWORD ExpectedFrameBoundaryByte = PlayCursor + ExpectedSoundBytesPerFrame;

							DWORD SafeWriteCursor = WriteCursor;
							if (SafeWriteCursor < PlayCursor)
							{
								SafeWriteCursor += SoundOutput.SecondaryBufferSize;
							}
							SafeWriteCursor += SoundOutput.SafetySampleBytes;
							Assert(SafeWriteCursor > PlayCursor);

							boolint AudioCardIsLowLatency = (SafeWriteCursor < ExpectedFrameBoundaryByte);

							DWORD TargetCursor = 0;
							if (AudioCardIsLowLatency)
							{
								TargetCursor = ExpectedFrameBoundaryByte + ExpectedSoundBytesPerFrame;
							}
							else {
								TargetCursor = WriteCursor + ExpectedSoundBytesPerFrame + SoundOutput.SafetySampleBytes;
							}
							TargetCursor %= SoundOutput.SecondaryBufferSize;

							DWORD BytesToWrite = TargetCursor - ByteToStartLock;
							if(ByteToStartLock > TargetCursor)
							{
								BytesToWrite += SoundOutput.SecondaryBufferSize;
							}
							game_sound_output_buffer SoundBuffer = {};
							SoundBuffer.SamplesPerSecond = SoundOutput.SamplesPerSecond;
							SoundBuffer.SampleCount = (int)BytesToWrite / SoundOutput.BytesPerSample;
							SoundBuffer.Samples = Samples;

							if(Game.GetSoundSamples)
							{
								Game.GetSoundSamples(&Thread, &GameMemory, &SoundBuffer);
							}

							//DirectSound Output Test
#if INTERNAL_BUILD
							win32_debug_time_marker* Marker = &DebugTimeMarkers[DebugTimeMarkerIndex];
							Marker->OutputPlayCursor = PlayCursor;
							Marker->OutputWriteCursor = WriteCursor;
							Marker->OutputLocation = ByteToStartLock;
							Marker->OutputByteCount = BytesToWrite;
							Marker->ExpectedFlipPlayCursor = ExpectedFrameBoundaryByte;

							AudioLatencyBytes = WriteCursor - PlayCursor;
							if (PlayCursor > WriteCursor)
							{
								AudioLatencyBytes += SoundOutput.SecondaryBufferSize;
							}
							AudioLatencySeconds = (float)AudioLatencyBytes / (float)SoundOutput.BytesPerSample / (float)SoundOutput.SamplesPerSecond;

							//char TextBuffer[256];
							//sprintf_s(TextBuffer, "PC: %d, WC: %d, BTSL: %d, TC: %d, BTW: %d, DELTA: %d (%fs)\n", (uint32_t)PlayCursor, (uint32_t)WriteCursor, (uint32_t)ByteToStartLock, (uint32_t)TargetCursor, (uint32_t)BytesToWrite, (uint32_t)AudioLatencyBytes, AudioLatencySeconds);
							//OutputDebugStringA(TextBuffer);
#endif

							Win32FillSoundBuffer(&SoundOutput, ByteToStartLock, BytesToWrite, &SoundBuffer);
						}
						else {
							SoundIsValid = false;
						}
#endif
						LARGE_INTEGER WorkCounter = Win32GetWallClock();
						float WorkSecondsElapsed = Win32GetSecondsElapsed(LastCounter, WorkCounter);

						//TODO: NOT TESTED YET: PROBABLY BUGGY
						float SecondsElapsedForFrame = WorkSecondsElapsed;

						DWORD MSLeft = (DWORD)(1000.0f * (TargetSecondsPerFrame - SecondsElapsedForFrame));
/*						if (SecondsElapsedForFrame > TargetSecondsPerFrame)
						{
							LARGE_INTEGER Prerender = Win32GetWallClock();
							if (Game.Render)
							{
								Game.Render(&Thread, &GameMemory, &Buffer);
							}
							LARGE_INTEGER Postrender = Win32GetWallClock();
							RenderTime = Win32GetSecondsElapsed(Prerender, Postrender);

							//TODO: MISSED FRAME RATE
							//logging
						}
						else
						{
							if (GlobalFast)
							{
								while ((TargetSecondsPerFrame - WorkSecondsElapsed - RenderTime) / UpdateAndInputTime > 2 && RenderTime != 0)
								{
									PreInput = Win32GetWallClock();
									ResetInput(OldInput, NewInput);
									GetInput(&State, Window, OldInput, NewInput);

									if(Game.Update)
									{
										Game.Update(&Thread, &GameMemory, NewInput);
									}
									PostUpdate = Win32GetWallClock();
									UpdateAndInputTime = Win32GetSecondsElapsed(PreInput, PostUpdate);

									WorkSecondsElapsed = Win32GetSecondsElapsed(LastCounter, PostUpdate);
									MSLeft = (DWORD)(1000.0f * (TargetSecondsPerFrame - WorkSecondsElapsed));

								}
							}
							LARGE_INTEGER Prerender = Win32GetWallClock();
							if (Game.Render)
							{
								Game.Render(&Thread, &GameMemory, &Buffer);
							}
							LARGE_INTEGER Postrender = Win32GetWallClock();
							RenderTime = Win32GetSecondsElapsed(Prerender, Postrender);


						}
*/
						if (SleepIsGranular && SecondsElapsedForFrame < TargetSecondsPerFrame)
						{
							if (MSLeft > 0)
							{
								Sleep(MSLeft);
							}
						}

						while (SecondsElapsedForFrame < TargetSecondsPerFrame)
						{
							SecondsElapsedForFrame = Win32GetSecondsElapsed(LastCounter, Win32GetWallClock());
						}

						uint64_t EndCycleCount = __rdtsc();
						uint64_t CyclesElapsed = EndCycleCount - LastCycleCount;
						LastCycleCount = EndCycleCount;

						LARGE_INTEGER EndCounter = Win32GetWallClock();
						float MSPerFrame = 1000.0f * Win32GetSecondsElapsed(LastCounter, EndCounter);
						LastCounter = EndCounter;	
						float FPS = 1000.0f / (float)MSPerFrame;
						float MCPF = ((float)CyclesElapsed * 0.000001f);

						char DebugTimingBuffer[256];
						sprintf_s(DebugTimingBuffer, "%.02fms/f, %.02ff/s, %.02fMc/f\n", MSPerFrame, FPS, MCPF);
						OutputDebugStringA(DebugTimingBuffer);

						win32_window_dimension Dim = win32_GetWindowDimension(Window);
#if INTERNAL_BUILD
						//note, debugtimemarkerindex is wrong on the zeroth
						// Win32DebugSyncDisplay(&GlobalBuffer, ArrayCount(DebugTimeMarkers), DebugTimeMarkerIndex - 1, DebugTimeMarkers, &SoundOutput, TargetSecondsPerFrame);
#endif

						HDC DeviceContext = GetDC(Window);
						Win32DrawToWindow(DeviceContext, &GlobalBuffer, Dim.Width, Dim.Height);
						ReleaseDC(Window, DeviceContext);

						FlipWallClock = Win32GetWallClock();

						/*
						 * #if INTERNAL_BUILD
						 * if(SUCCEEDED(GlobalSecondaryBuffer->GetCurrentPosition(&PlayCursor, &WriteCursor)))
						 * {
						 * Assert(DebugTimeMarkerIndex < 30);
						 * win32_debug_time_marker* Marker = &DebugTimeMarkers[DebugTimeMarkerIndex];
						 * Marker->FlipPlayCursor = PlayCursor;
						 Marker->FlipWriteCursor = WriteCursor;
						 DebugTimeMarkerIndex += 1;
						 DebugTimeMarkerIndex %= 30;
						 }
#endif
*/

						ResetInput(OldInput, NewInput);
					}
				}
			}
			else{
				//TODO logging
			}
		}
		else {
			//TODO logging
		}
	}
	else {
		//TODO logging
	}
	timeEndPeriod(DesiredSchedulerMilliseconds);
	return 0;
}

//TODO: THIS IS NOT A FINAL PLATFORM LAYER

