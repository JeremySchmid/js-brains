#if !defined(BASE_H)

#include "base_platform.h"
#include "base_tiles.h"
#include "neural_net.h"

//SLOW_CODE - 0 no slow code 1 slow code allowed
//HANDMADE_INTERNAL 0 no dev code 1 dev code allowed

#define internal static
#define local_persist static
#define global_var static

#define Pi32 3.141592653589793238f

#if SLOW_CODE
	//TODO: complete assertion macro
	#define Assert(Expression) if (!(Expression)) {*(int*)0 = 0;}
#else
	#define Assert(Expression)
#endif

#define Kilobytes(Value) ((Value) * 1024LL)
#define Megabytes(Value) ((Value) * 1024LL * 1024)
#define Gigabytes(Value) ((Value) * 1024LL * 1024 * 1024)
#define Terabytes(Value) ((Value) * 1024LL * 1024 * 1024 * 1024)

#define ArrayCount(Array) (sizeof(Array) / sizeof((Array)[0]))
#define Zero(Variable) for (int i = 0; i < sizeof(Variable); i++) {char* p = &(char)(Variable); *(p + i) = 0;}
//TODO: swap, min, max ....macros??

uint32_t SafeTruncateUInt64(uint64_t Value)
{
	//TODO: defines for max values
	Assert(Value <= 0xffffffff);
	uint32_t Result = (uint32_t) Value;
	return Result;
}

inline game_controller_input* GetController (game_input* Input, int ControllerIndex)
{
	Assert(ControllerIndex >= 0);
	Assert(ControllerIndex < ArrayCount(Input->Controllers));
	
	game_controller_input* Result = &Input->Controllers[ControllerIndex];
	return Result;
}

typedef struct memory_arena
{
	memory_index Size;
	uint8_t* Base;
	memory_index Used;

} memory_arena;

internal void InitializeArena(memory_arena* Arena, memory_index Size, uint8_t* Base)
{
	Arena->Size = Size;
	Arena->Base = Base;
	Arena->Used = 0;
}

internal void* PushSize_(memory_arena* Arena, memory_index Size)
{
	Assert((Arena->Used + Size) <= Arena->Size);
	void* Result = Arena->Base + Arena->Used;
	Arena->Used += Size;
	return Result;
}

internal void PopSize_(memory_arena* Arena, memory_index Size)
{
	Assert(Arena->Used >= Size);
	Arena->Used -= Size;
}

#define PushStruct(Arena, type) (type*)PushSize_(Arena, sizeof(type))
#define PushArray(Arena, type, Count) (type*)PushSize_(Arena, (Count)*sizeof(type))
#define PopStruct(Arena, type) PopSize_(Arena, sizeof(type))
#define PopArray(Arena, type, Count) PopSize_(Arena, (Count)*sizeof(type))

typedef struct world
{
	tile_map* TileMap;

} world;

typedef struct creature
{
	float PositionX;
	float PositionY;

	int GoalX;
	int GoalY;

	float VelocityX;
	float VelocityY;

	int Age;

	float CurrentFitness;
	float LastFitness;

} creature;

typedef struct debug_state
{
	//FILE* DebugFile;

	boolint DebugMode;
	boolint DebugForce;

	float DebugGraph[1000];
	int DebugNum;

	int CreatureIndex;
	int CreatureToDraw;
	int NeuronToDraw;
	int DendriteToDraw;

} debug_state;

typedef struct game_state
{
	debug_state DebugState;

	creature* Creatures;
	neural_net* Nets;

	memory_arena WorldArena;
	world* World;

	tile_map_position PlayerP;	

} game_state;

#define BASE_H
#endif
