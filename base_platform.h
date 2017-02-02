#if !defined(BASE_PLATFORM_H)

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <math.h> //implement sin and sqrt ourselves...

typedef int boolint;

typedef size_t memory_index;


typedef struct thread_context
{
	int Placeholder;
} thread_context;

int AsciiToIndex (int Character)
{
	return (Character - 'A');
}

//NOTE: Services that the platform layer provides to the game


#if INTERNAL_BUILD
//note: These are NOT for doing anything in the shipping game - they are blocking and the write doesn't protect against lost data!
//
typedef struct debug_read_file_result
{
	void *Contents;
	uint32_t ContentSize;

} debug_read_file_result;

#define DEBUG_PLATFORM_FREE_FILE_MEMORY(name) void name(thread_context* Thread, void* Memory)
typedef DEBUG_PLATFORM_FREE_FILE_MEMORY(debug_platform_free_file_memory);

#define DEBUG_PLATFORM_READ_ENTIRE_FILE(name) debug_read_file_result name(thread_context* Thread, char* Filename)
typedef DEBUG_PLATFORM_READ_ENTIRE_FILE(debug_platform_read_entire_file);

#define DEBUG_PLATFORM_WRITE_ENTIRE_FILE(name) boolint name(thread_context* Thread, char* Filename, uint64_t MemorySize, void* Memory)
typedef DEBUG_PLATFORM_WRITE_ENTIRE_FILE(debug_platform_write_entire_file);

DEBUG_PLATFORM_FREE_FILE_MEMORY(DEBUGPlatformFreeFileMemory);
DEBUG_PLATFORM_READ_ENTIRE_FILE(DEBUGPlatformReadEntireFile);
DEBUG_PLATFORM_WRITE_ENTIRE_FILE(DEBUGPlatformWriteEntireFile);
#endif


//TODO: Services that the game provides to the platform layer
//May expand in the future - sound on separate thread, etc

//FOUR THINGS - timing, controller/keyboard input, bitmap buffer to use, sound buffer to use

//TODO: in the future, rendering _specifically_ will become a three-tiered abstraction
typedef struct game_offscreen_buffer
{
	void *Memory;
	int Width;
	int Height;
	int Pitch;
	int BytesPerPixel;
	
} game_offscreen_buffer;

typedef struct game_sound_output_buffer
{
	int SamplesPerSecond;
	int SampleCount;
	int16_t *Samples;
	
} game_sound_output_buffer;

#define NUM_KEYS_HANDLED 51
typedef enum key
{
	UNKNOWN,

	A, B, C, D, E, F, G, H, I, J, K, L, M, N, O, P, Q, R, S, T, U, V, W, X, Y, Z,

	NUM_0, NUM_1, NUM_2, NUM_3, NUM_4, NUM_5, NUM_6, NUM_7, NUM_8, NUM_9,

	UP,
	DOWN,
	LEFT,
	RIGHT,

	CTRL,
	ALT,
	SHIFT,

	ENTER,
	ESCAPE,
	SPACE,
	
	PERIOD,
	COMMA,
	SEMICOLON,
	QUOTE,

} key;

typedef struct key_state
{
	int HalfTransitionCount;
	boolint EndedDown;
	
} key_state;

typedef enum key_action
{
	PRESS,
	REPEAT,
	RELEASE,

} key_action;

typedef struct key_press
{
	key Pressed;
	key_action Action;
	boolint Ctrl;
	boolint Alt;
	boolint Shift;

} key_press;

typedef struct keyboard_input
{
	key_press KeyPresses[100];
	int KeyPressesIndex;

} keyboard_input;

typedef struct game_input
{

	key_state MouseButtons[5];
	int MouseX, MouseY, MouseZ;

	keyboard_input KeyboardInput;
	
	float dtForFrame;
	//Insert clock values here
	//TODO: what do we want to pass here?
	//TODO(Jeremy): use a fixed point of 64.64? floats are inaccurate at high numbers and people may leave the program running - http://home.comcast.net/~tom_forsyth/blog.wiki.html#[[A%20matter%20of%20precision]]
	//float SecondsElapsed;
} game_input;

typedef struct game_memory
{
	boolint IsInitialized;
	
	uint64_t PermanentStorageSize;
	void *PermanentStorage; //Note: REQUIRED to be cleared to zero at startup
	
	uint64_t TransientStorageSize;
	void *TransientStorage; //Note: REQUIRED to be cleared to zero at startup

	debug_platform_read_entire_file* DEBUGPlatformReadEntireFile;
	debug_platform_write_entire_file* DEBUGPlatformWriteEntireFile;
	debug_platform_free_file_memory* DEBUGPlatformFreeFileMemory;
	
} game_memory;

#define GAME_GET_SOUND_SAMPLES(name) void name(thread_context* Thread, game_memory* Memory, game_sound_output_buffer* SoundBuffer)
typedef GAME_GET_SOUND_SAMPLES(game_get_sound_samples);

#define GAME_UPDATE(name) void name(thread_context* Thread, game_memory* Memory, game_input *Input)
typedef GAME_UPDATE(game_update);

#define GAME_RENDER(name) void name(thread_context* Thread, game_memory* Memory, game_offscreen_buffer* Buffer)
typedef GAME_RENDER(game_render);

#ifdef __cplusplus
}
#endif


#define BASE_PLATFORM_H
#endif
