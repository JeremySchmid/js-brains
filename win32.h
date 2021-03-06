#if !defined(WIN32_H)

typedef struct win32_offscreen_buffer
{
	BITMAPINFO Info;
	void *Memory;
	int Width;
	int Height;
	int Pitch;
	int BytesPerPixel;

} win32_offscreen_buffer;

typedef struct win32_window_dimension
{
	int Width;
	int Height;

} win32_window_dimension;

typedef struct win32_sound_output
{
	int SamplesPerSecond;
	uint32_t RunningSampleIndex;
	int BytesPerSample;
	int SecondaryBufferSize;
	float SineTime;
	int LatencySampleCount;
	int SafetySampleBytes;
	//TODO Should RunningSampleIndex be in bytes
	//TODO: Math gets simpler if we add a BytesPerSecond field

} win32_sound_output;

typedef struct win32_debug_time_marker
{
	DWORD OutputPlayCursor;
	DWORD OutputWriteCursor;
	DWORD OutputLocation;
	DWORD OutputByteCount;
	DWORD ExpectedFlipPlayCursor;
	
	DWORD FlipPlayCursor;
	DWORD FlipWriteCursor;

} win32_debug_time_marker;

typedef struct win32_game_code
{
	HMODULE GameCodeDLL;
	FILETIME DLLLastWriteTime;

	//Note: Either of the callbacks can be null
	//MUST check before calling!
	game_get_sound_samples* GetSoundSamples;
	game_update* Update;
	game_render* Render;

	boolint IsValid;

} win32_game_code;

#define WIN32_STATE_FILE_NAME_COUNT MAX_PATH
typedef struct win32_replay_buffer
{
	HANDLE FileHandle;
	HANDLE MemoryMap;
	char FileName[WIN32_STATE_FILE_NAME_COUNT];
	void* MemoryBlock;

} win32_replay_buffer;

typedef struct win32_state
{
	uint64_t TotalSize;
	void* GameMemoryBlock;
	win32_replay_buffer ReplayBuffers[4];
	int ReplayBufferChoice;

	HANDLE RecordingHandle;
	int InputRecordingIndex; // init to -1

	HANDLE PlaybackHandle;
	int InputPlaybackIndex; // init to -1

	char ExeFileName[WIN32_STATE_FILE_NAME_COUNT];
	char* OnePastLastExeFileNameSlash;

} win32_state;



#define WIN32_H
#endif
