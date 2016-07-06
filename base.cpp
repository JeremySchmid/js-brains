#include "stdlib.h"
#include "stdio.h"
#include "base.h"
#include "base_intrinsics.h"
#include "base_tiles.h"
#include "neural_net.h"

#include "base_tiles.cpp"

#include "base_random.h"

#include "neural_net.cpp"

void GameOutputSound(game_state* State, game_sound_output_buffer *SoundBuffer)
{
	/*
	int16_t ToneVolume = 2048;
	int16_t WavePeriod = (int16_t)(SoundBuffer->SamplesPerSecond/State->ToneHz);
	
	int16_t *SampleOut = SoundBuffer->Samples;
	for (int SampleIndex = 0; SampleIndex < SoundBuffer->SampleCount; SampleIndex++) {
		float SineValue = sinf(State->SineTime);
		int16_t SampleValue = (int16_t)(SineValue * ToneVolume);
		*SampleOut++ = SampleValue;
		*SampleOut++ = SampleValue;

		State->SineTime += 2.0f*Pi32*1.0f/(float)WavePeriod;
		if (State->SineTime > 2*Pi32) {
			State->SineTime -= 2*Pi32;	
		}
	}
	*/
}

internal void DrawRectangle(game_offscreen_buffer* Buffer, float RealMinX, float RealMinY, float RealMaxX, float RealMaxY, float R, float G, float B) //will round the floats to integer and fill up to max values (exclusive)
//Floating Point Color tomorrow
{

	int32_t MinX = RoundFloatToInt32(RealMinX);
	int32_t MaxX = RoundFloatToInt32(RealMaxX);
	int32_t MinY = RoundFloatToInt32(RealMinY);
	int32_t MaxY = RoundFloatToInt32(RealMaxY);

	if(MinX < 0) {
		MinX = 0;
	}

	if(MinY < 0) {
		MinY = 0;
	}

	if(MaxX > Buffer->Width) {
		MaxX = Buffer->Width;
	}

	if(MaxY > Buffer->Height) {
		MaxY = Buffer->Height;
	}
	
	if(R < 0) {
		R = 0;
	}

	if(R > 1.0) {
		R = 1.0;
	}

	if(G < 0) {
		G = 0;
	}

	if(G > 1.0) {
		G = 1.0;
	}

	if(B < 0) {
		B = 0;
	}

	if(B > 1.0) {
		B = 1.0;
	}


	uint32_t Color = (uint32_t)((RoundFloatToUInt32(R * 255.0f) << 16) |
										(RoundFloatToUInt32(G * 255.0f) << 8) |
										(RoundFloatToUInt32(B * 255.0f) << 0));
	
	uint8_t* Row = (uint8_t*)Buffer->Memory + MinX*Buffer->BytesPerPixel + MinY * Buffer->Pitch;
	
	for (int Y = MinY; Y < MaxY; Y++) {
		uint32_t* Pixel = (uint32_t*)Row;
	
		for (int X = MinX; X < MaxX; X++) {
			*Pixel++ = Color;
		}
		Row += Buffer->Pitch;
	}
}

internal void DrawPixel(game_offscreen_buffer* Buffer, float RealX, float RealY, float R, float G, float B)
{
	DrawRectangle(Buffer, RealX, RealY, RealX + 1.0f, RealY + 1.0f, R, G, B);

}

void DrawLine(game_offscreen_buffer* Buffer, int XStart, int YStart, int XEnd, int YEnd, float Red, float Blue, float Green)
{
	int Rise = YEnd - YStart;
	int Run = XEnd - XStart;

	float Slope = (float)((double)Rise / (double)Run);

	int YProgress = 0;
	int XProgress = 0;

	while ((abs(YProgress) < abs(Rise) || abs(XProgress) < abs(Run))
			&& abs(YProgress) < 1000
			&& abs(XProgress) < 1000)
	{
		float Progress = (float)YProgress / (float)XProgress;
		if (SignOf(Slope) * (Slope - Progress) > 0 || Run == 0)
		{
			YProgress += 1 * SignOf((float)Rise);
			DrawPixel(Buffer, (float)(XStart + XProgress), (float)(YStart + YProgress), Red, Blue, Green);
		}
		else //if (SignOf(Slope) * (Slope - Progress) < 0 || Slope == Progress)
		{
			XProgress += 1 * SignOf((float)Run);
			DrawPixel(Buffer, (float)(XStart + XProgress), (float)(YStart + YProgress), Red, Blue, Green);
		}
	}
}


float CalculateCreatureFitness(creature* Creature)
{
	float YRootFitness = (Creature->GoalY - Creature->PositionY) / 540.0f;
	float XRootFitness = (Creature->GoalX - Creature->PositionX) / 960.0f;
	float UndampedFitness = -(YRootFitness * YRootFitness + XRootFitness * XRootFitness);
	float FinalFitness = (float)(SignOf(UndampedFitness) * sqrt(AbsVal(UndampedFitness)));
	return FinalFitness;

}

void ColorValues(float Value, float* R, float* G, float* B)
{
	*R = -Value * 10.0f;
	*G = AbsVal(Value);
	*B = Value * 10.0f;

	//if (*G > 1.0f)
	{
//		*B = 1.0f - Value / 255.0f / 255.0f;
//		*R = *B;
	}

	return;
}

boolint ButtonToggled(game_button_state* Button)
{
	int HalfTransitionCount = Button->HalfTransitionCount % 4;
	return ((HalfTransitionCount == 2
			|| (Button->EndedDown && HalfTransitionCount == 1))
			|| (!Button->EndedDown
					&& HalfTransitionCount == 3));
}

float LogisticFunc (float Value)
{

	float Result = (float)((2.0f / (1.0f + 1.0f * pow(10.0f, -Value))) - 1.0f);

	return Result;
}

extern "C" GAME_UPDATE_AND_RENDER(GameUpdateAndRender)
{
	Assert(&Input->Controllers->Buttons[ArrayCount(Input->Controllers->Buttons)] == &Input->Controllers->Terminator);
	Assert(sizeof(game_state) <= Memory->PermanentStorageSize);

	game_state* State = (game_state*) Memory->PermanentStorage;
	
	int NumOfNeurons = NUMNEURONS;
	int MaxDendrites = NUMDENDRITES;
	int NumOfCreatures = 200;
	
	for (int ControllerIndex = 0; ControllerIndex < ArrayCount(Input->Controllers); ControllerIndex++)
	{
	game_controller_input* Controller = GetController(Input, ControllerIndex);
	
		if (ButtonToggled(&Controller->Start))	
		{
			State->DebugState.DebugMode = !State->DebugState.DebugMode;
		}	
		if(Controller->Back.EndedDown)
		{
			Memory->IsInitialized = false;
			DrawRectangle(Buffer, 0.0f, 0.0f, (float)Buffer->Width, (float)Buffer->Height, 0.0f, 0.0f, 0.0f);

		}
		if (Controller->IsAnalog)
		{
		}
	}

	if (!Memory->IsInitialized)
	{
		srand(6);

		debug_state* DebugState = &State->DebugState;
		//DebugState->DebugFile = fopen("errlog.txt", "w");
		DebugState->DebugNum = 0;
		DebugState->DebugMode = 0;
		DebugState->CreatureToDraw = 100;
		DebugState->NeuronToDraw = 0;
		DebugState->DendriteToDraw = 0;

		InitializeArena(&State->WorldArena, 
								Memory->PermanentStorageSize - sizeof(game_state), 
								(uint8_t*)Memory->PermanentStorage + sizeof(game_state));
		
		State->Creatures = PushArray(&State->WorldArena, creature, NumOfCreatures);
		for (int CreatureIndex = 0; CreatureIndex < NumOfCreatures; CreatureIndex++)
		{
			creature* Creature = State->Creatures + CreatureIndex;
			Creature->PositionX = (float)(rand() % 960);
			Creature->PositionY = (float)(rand() % 540);
			Creature->GoalX = rand() % 960;
			Creature->GoalY = rand() % 540;
			Creature->Age = 0;
			Creature->CurrentFitness = CalculateCreatureFitness(Creature);
		}
		
		State->Nets = PushArray(&State->WorldArena, neural_net, NumOfCreatures);
		for (int NetIndex = 0; NetIndex < NumOfCreatures; NetIndex++)
		{
			neural_net* Net = State->Nets + NetIndex;
			Net->NumOfNeurons = NumOfNeurons;
			Net->NumDendrites = MaxDendrites;
			Net->NumSensorValues = 6;
			Net->NumMotorValues = 2;
			NeuralNetInitialize(Net);
		}
		State->World = PushStruct(&State->WorldArena, world);
		world* World = State->World;
		World->TileMap = PushStruct(&State->WorldArena, tile_map);

		tile_map* TileMap = World->TileMap;

		//Note: currently set to 256x256 tilechunks
		TileMap->ChunkShift = 8;
		TileMap->ChunkDim = (uint32_t)(1 << TileMap->ChunkShift);
		TileMap->ChunkMask = TileMap->ChunkDim - 1;
	
		TileMap->TileChunkCountX = 8;
		TileMap->TileChunkCountY = 8;
		TileMap->TileChunkCountZ = 1;

		TileMap->TileChunks = PushArray(&State->WorldArena, 
													tile_chunk,
													TileMap->TileChunkCountX
													*TileMap->TileChunkCountY
													*TileMap->TileChunkCountZ);

		TileMap->TileSideInMeters = 1.0f;

		Memory->IsInitialized = true;
	}
	
	world *World = State->World;
	tile_map* TileMap = World->TileMap;

	int32_t TileSideInPixels = 1;
	float PixelsPerMeter = (float)TileSideInPixels / (float)TileMap->TileSideInMeters;
	
	float LowerLeftX = -(float)TileSideInPixels / 2 + 50;
	float LowerLeftY = (float)Buffer->Height - 50;

	debug_state* DebugState = &State->DebugState;

	if (!DebugState->DebugForce)
	{

		DrawRectangle(Buffer, 0.0f, 0.0f, (float)Buffer->Width, (float)Buffer->Height, 0.0f, 0.0f, 0.0f);

		float XCoordinates[15];
		float YCoordinates[15];
		
		for (int Index = 0; Index < 15; Index++)
		{
			float RPolar = 250.0f;
			float ThetaPolar = 2 * Pi32 / 15.0f * Index;

			float X = 480.0f + RPolar * 1.5f * (float)sin(ThetaPolar);
			float Y = 270.0f - RPolar * (float)cos(ThetaPolar);

			XCoordinates[Index] = X;
			YCoordinates[Index] = Y;
		}
#if 1
		for (int NeuronIndex = 0; NeuronIndex < NUMNEURONS; NeuronIndex++)
		{
			neural_net* Net = State->Nets + DebugState->CreatureToDraw;
			
			neuron* Neuron = Net->Neurons + NeuronIndex * 2 + Net->Toggle;

			float NeuronScale1, NeuronScale2, NeuronScale3;
			ColorValues(Neuron->Firing, &NeuronScale1, &NeuronScale2, &NeuronScale3);
			
			DrawRectangle(Buffer, XCoordinates[NeuronIndex] - 18, YCoordinates[NeuronIndex] - 18, XCoordinates[NeuronIndex] + 19, YCoordinates[NeuronIndex] + 19, NeuronScale1, NeuronScale2, NeuronScale3);
			

			for (int DendriteIndex = 0; DendriteIndex < NUMDENDRITES; DendriteIndex++)
			{
				dendrite* Dendrite = &Neuron->Dendrites[DendriteIndex];
				float DendriteScale1, DendriteScale2, DendriteScale3;
				ColorValues(Dendrite->Strength + Dendrite->CurrentWobble, &DendriteScale1, &DendriteScale2, &DendriteScale3);
				DrawLine(Buffer, (int)(XCoordinates[NeuronIndex]), (int)(YCoordinates[NeuronIndex]), (int)(XCoordinates[Dendrite->Sender]), (int)(YCoordinates[Dendrite->Sender]), DendriteScale1, DendriteScale2, DendriteScale3);
				//drawline (curved?) function from sender position to current neuron position - additive so that both directions can be shown on one line?
			}

		}
#endif
		

		//update physics, then neural nets, draw them in their respective squares, then their "intentions for next turn"
		for (int CreatureIndex = 0; CreatureIndex < NumOfCreatures; CreatureIndex++)
		{
			creature* Creature = State->Creatures + CreatureIndex;
			neural_net* Net = State->Nets + CreatureIndex;

			Creature->PositionX += Creature->VelocityX;
			Creature->PositionY += Creature->VelocityY;
			
			Creature->LastFitness = Creature->CurrentFitness;
			Creature->CurrentFitness = CalculateCreatureFitness(Creature);
			float FitnessChange = Creature->CurrentFitness - Creature->LastFitness;
			float Reward = LogisticFunc(FitnessChange);
			Assert(isfinite(Reward));
			
			Creature->Age++;

			float* SensorValues = (float*)PushArray(&State->WorldArena, float, Net->NumSensorValues);
			float* MotorValues = (float*)PushArray(&State->WorldArena, float, Net->NumMotorValues);

			SensorValues[0] = Creature->PositionX / 960.0f;
			SensorValues[1] = Creature->PositionY / 540.0f;
			SensorValues[2] = Creature->GoalX / 960.0f;
			SensorValues[3] = Creature->GoalY / 540.0f;
			SensorValues[4] = Creature->VelocityX;
			SensorValues[5] = Creature->VelocityY;


			for (int UpdateIndex = 0; UpdateIndex < 10; UpdateIndex++)
			{
			//	NeuralNetUpdate(Net, SensorValues, MotorValues, 0.0f);
			}

			DebugState->CreatureIndex = CreatureIndex;
			
			NeuralNetUpdate(DebugState, Net, SensorValues, MotorValues, Reward);
			
			if (CreatureIndex == DebugState->CreatureToDraw)
			{

				float FitnessScale1, FitnessScale2, FitnessScale3;
				ColorValues(FitnessChange, &FitnessScale1, &FitnessScale2, &FitnessScale3);

				DrawRectangle(Buffer, Creature->PositionX - 4, Creature->PositionY - 4, Creature->PositionX + 3, Creature->PositionY + 3, FitnessScale1, FitnessScale2, FitnessScale3);
				DrawLine(Buffer, (int)Creature->GoalX, (int)Creature->GoalY, (int)Creature->PositionX, (int)Creature->PositionY, 1.0f, 1.0f, 0.0f);

			}
			
			Creature->VelocityX = MotorValues[0];
			Creature->VelocityY = MotorValues[1];

			PopArray(&State->WorldArena, float, Net->NumSensorValues);
			PopArray(&State->WorldArena, float, Net->NumMotorValues);
		}
	}
	if (DebugState->DebugMode || DebugState->DebugForce)
	{

		//DrawRectangle(Buffer, 0.0f, 0.0f, (float)Buffer->Width, (float)Buffer->Height, 0.0f, 0.0f, 0.0f);

		float MaxValue = 0;
		for (int i = 0; i < DebugState->DebugNum && i < 960; i++)
		{
			if (AbsVal(DebugState->DebugGraph[i]) * 1.05f > MaxValue)
			{
				MaxValue = 1.05f * AbsVal(DebugState->DebugGraph[i]);
			}
		}

		for (int j = 0; j < DebugState->DebugNum && j < 960; j++)
		{
		DrawPixel(Buffer, (float)j, (float)Buffer->Height / 2.0f * (1.0f - DebugState->DebugGraph[j] / MaxValue), 1.0f, 1.0f, 1.0f);
		}

	}
}


//must be very fast, no more than 1-2 ms right now
//reduce the pressure on its performance by measuring it
//or asking about it, etc.
extern "C" GAME_GET_SOUND_SAMPLES(GameGetSoundSamples)
{
	game_state* State = (game_state*) Memory->PermanentStorage;
	GameOutputSound(State, SoundBuffer);
}

	
void RenderWeirdGradient(game_offscreen_buffer* Buffer, int XOffset, int YOffset)
{

	uint8_t *Row = (uint8_t *)Buffer->Memory;
	for (int y = 0; y < Buffer->Height; ++y)
	{
		uint32_t *Pixel = (uint32_t*) Row;
		for (int x = 0; x < Buffer->Width; ++x)
		{
			uint8_t Blue = (uint8_t)(y + YOffset);
			uint8_t Green = (uint8_t)(x + XOffset);

			*Pixel++ = (uint32_t)(Blue | (Green << 8));
		}
		Row += Buffer->Pitch;
	}

}
