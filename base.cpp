#include "stdlib.h"
#include "stdio.h"

#include "magic_numbers.h"
#include "base.h"
#include "base_intrinsics.h"
#include "base_tiles.h"
#include "neural_net.h"

#include "base_tiles.cpp"

#include "base_random.h"

#include "neural_net.cpp"

void GameOutputSound(state* State, game_sound_output_buffer *SoundBuffer)
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

void DrawLine(game_offscreen_buffer* Buffer, float XStart, float YStart, float XEnd, float YEnd, float Red, float Blue, float Green)
{

	float Rise = YEnd - YStart;
	float Run = XEnd - XStart;
	
	float Slope = (float)((double)Rise / (double)Run);

	if (Run == 0 && IsFloatInBounds(XStart, 0.0f, 960.0f))
	{
		ForceFloatInBounds(&YStart, 0.0f, 960.0f);
		ForceFloatInBounds(&YEnd, 0.0f, 960.0f);

		int Direction = SignOf(YEnd - YStart);
		if (!Direction)
		{
			Direction = 1;
		}
		for (int Length = 0; Length <= YEnd - YStart; Length += Direction)
		{
			DrawPixel(Buffer, XStart, YStart + Length, Red, Blue, Green);
		}
	}
	else if (AbsVal(Slope) < 1)
	{
		ForceFloatInBounds(&XStart, 0.0f, 960.0f);
		ForceFloatInBounds(&XEnd, 0.0f, 960.0f);
		
		int Direction = SignOf(XEnd - XStart);
		if (!Direction)
		{
			Direction = 1;
		}
		for (float Length = 0.0f; AbsVal(Length) <= AbsVal(XEnd - XStart); Length += Direction)
		{
			DrawPixel(Buffer, XStart + Length, YStart + Length * Slope, Red, Blue, Green);
		}

	}
	else
	{
		ForceFloatInBounds(&YStart, 0.0f, 960.0f);
		ForceFloatInBounds(&YEnd, 0.0f, 960.0f);
		
		int Direction = SignOf(YEnd - YStart);
		if (!Direction)
		{
			Direction = 1;
		}
		for (float Length = 0.0f; AbsVal(Length) <= AbsVal(YEnd - YStart); Length += Direction)
		{
			DrawPixel(Buffer, XStart + Length / Slope, YStart + Length, Red, Blue, Green);
		}

	}

	/*
	float YProgress = 0.0f;
	float XProgress = 0.0f;

	int LastChange = (AbsVal(Slope) >= 1 ? 1 : 0);

	do {
		float Progress = YProgress / XProgress;
		if (SignOf(Slope) * (Slope - Progress) > 0 || Run == 0)
		{
			YProgress += 1.0f * SignOf(Rise);
			if (LastChange != 0)
			{
				DrawPixel(Buffer, XStart + XProgress, YStart + YProgress, Red, Blue, Green);
			}
		}
		else //if (SignOf(Slope) * (Slope - Progress) < 0 || Slope == Progress)
		{
			XProgress += 1 * SignOf(Run);
			if (LastChange != 1)
			{
				DrawPixel(Buffer, XStart + XProgress, YStart + YProgress, Red, Blue, Green);
			}
		}
	} while ((AbsVal(YProgress) < AbsVal(Rise) || AbsVal(XProgress) < AbsVal(Run))
			&& AbsVal(YProgress) < 1000.0f
			&& AbsVal(XProgress) < 1000.0f);
*/
}


float CalculateCreatureFitness(creature* Creature)
{
	float Result;

	float YRelPos = Creature->GoalY - Creature->PositionY;
	float XRelPos = Creature->GoalX - Creature->PositionX;
	float NegDistance = -(float)(YRelPos * YRelPos + XRelPos * XRelPos);
	//float VelocityFitness = -(float)sqrt(Creature->VelocityX * Creature->VelocityX + Creature->VelocityY * Creature->VelocityY);

	Result = NegDistance;// + VelocityFitness;
	Assert(isfinite(Result));
	return Result;

}

void ColorValues(float Value, float* R, float* G, float* B)
{
	*R = -Value * 4.0f;
	*G = AbsVal(Value) / 4.0f;
	*B = Value * 4.0f;
/*
	if (*G > 1.0f)
	{
		if (Value > 0)
		{
			*B = 1.0f - Value / 100.0f;
		}
		else
		{
			*R = 1.0f - Value / 100.0f;
		}
	}
*/
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

	float Result = (float)(-1.0f + (2.0f / (1.0f + 1.0f * pow(2.0f, -50.0f * Value))));

	return Result;
}

void CreatureInitialize(creature* Creature)
{
	Creature->PositionX = 960 / 2;//(float)(rand() % 960);
	Creature->PositionY = 540 / 2;//(float)(rand() % 540);
	Creature->VelocityX = 0;
	Creature->VelocityY = 0;
	Creature->GoalX = rand() % 960;
	Creature->GoalY = rand() % 540;
	Creature->Age = 0;
}

void DoOneCycle(state* State)
{
	debug_state* DebugState = &State->DebugState;
	for (int CreatureIndex = 0; CreatureIndex < State->NumCreatures; CreatureIndex++)
	{
		creature* Creature = State->Creatures + CreatureIndex;
		neural_net* Net = State->Nets + CreatureIndex;

		float* SensorValues = (float*)PushArray(&State->WorldArena, float, Net->NumSensorNeurons);
		float* MotorValues = (float*)PushArray(&State->WorldArena, float, Net->NumMotorNeurons);

		SensorValues[0] = Creature->PositionX / 960.0f;
		SensorValues[1] = Creature->PositionY / 540.0f;
		SensorValues[2] = Creature->GoalX / 960.0f;
		SensorValues[3] = Creature->GoalY / 540.0f;
		SensorValues[4] = Creature->VelocityX;
		SensorValues[5] = Creature->VelocityY;

		DebugState->CreatureIndex = CreatureIndex;

		NeuralNetUpdate(DebugState, Net, SensorValues, MotorValues);

		Creature->VelocityX = MotorValues[0];
		Creature->VelocityY = -MotorValues[1];

		PopArray(&State->WorldArena, float, Net->NumSensorNeurons);
		PopArray(&State->WorldArena, float, Net->NumMotorNeurons);

		Creature->PositionX += Creature->VelocityX;
		Creature->PositionY += Creature->VelocityY;

		Net->LastFitness = Net->CurrentFitness;
		Net->CurrentFitness = CalculateCreatureFitness(Creature);

		float FitnessChange = Net->CurrentFitness - Net->LastFitness;

		Net->Reward = FitnessChange;
		//Net->Reward = LogisticFunc(FitnessChange);
		Assert(isfinite(Net->Reward));

		Net->PastFitness[Creature->Age] = Net->CurrentFitness;

		Creature->Age++;

	}

	if (State->Creatures->Age > NUMTESTCYCLES - 1)
	{
		float AverageFitness[NUMCREATURES];
		for (int CreatureIndex = 0; CreatureIndex < State->NumCreatures; CreatureIndex++)
		{
			neural_net* Net = State->Nets + CreatureIndex;
			float Average = 0;
			for (int FitnessNum = 0; FitnessNum < NUMTESTCYCLES; FitnessNum++)
			{
				Average += Net->PastFitness[FitnessNum]; 
			}
			AverageFitness[CreatureIndex] = Average / (float)NUMTESTCYCLES;
		}

		float FirstHalfFitness = 0;
		float SecondHalfFitness = 0;

		for (int CreatureIndex = 0; CreatureIndex < State->NumCreatures / 2; CreatureIndex++)
		{
			FirstHalfFitness += AverageFitness[CreatureIndex];
		}
		FirstHalfFitness /= State->NumCreatures / 2;

		DebugState->DebugGraph1[DebugState->DebugNum1 % DebugState->GraphSize] = FirstHalfFitness;
		DebugState->DebugNum1++;

		for (int CreatureIndex = State->NumCreatures / 2; CreatureIndex < State->NumCreatures; CreatureIndex++)
		{
			SecondHalfFitness += AverageFitness[CreatureIndex];
		}
		SecondHalfFitness /= State->NumCreatures / 2;

		DebugState->DebugGraph2[DebugState->DebugNum2 % DebugState->GraphSize] = SecondHalfFitness;
		DebugState->DebugNum2++;

		for (int CreatureIndex = 0; CreatureIndex < State->NumCreatures / 2; CreatureIndex++)
		{
			CreatureInitialize(State->Creatures + CreatureIndex);
			*(State->Creatures + CreatureIndex + State->NumCreatures / 2) = *(State->Creatures + CreatureIndex);
			//CreatureInitialize(State->Creatures + CreatureIndex + State->NumCreatures / 2);
			neural_net* FirstHalfNet = State->Nets + CreatureIndex;
			neural_net* SecondHalfNet = State->Nets + CreatureIndex + State->NumCreatures / 2;
			if (FirstHalfFitness >= SecondHalfFitness)
			{
				NeuralNetCleanFirings(FirstHalfNet);
				NeuralNetCopy(FirstHalfNet, SecondHalfNet);
			}
			else
			{
				NeuralNetCleanFirings(SecondHalfNet);
				NeuralNetCopy(SecondHalfNet, FirstHalfNet);
			}
		}

		int RandomNeuron = rand() % (State->Nets->NumNeurons - State->Nets->NumSensorNeurons) + State->Nets->NumSensorNeurons;
		int RandomDendrite = rand() % State->Nets->NumDendrites;

		int RandomSign = (rand() % 2 == 0 ? 1 : -1);
		float RandomAmount = (rand() % 2 == 0 ? 1.1f : 0.90f);

		int RandomIf = rand() % 30;

		for (int CreatureIndex = State->NumCreatures / 2; CreatureIndex < State->NumCreatures; CreatureIndex++)
		{
			neural_net* Net = State->Nets + CreatureIndex;
			neuron* Neuron = &Net->Neurons[RandomNeuron];
			dendrite* Dendrite = &Neuron->Dendrites[RandomDendrite];

			if (RandomIf > 25)
			{
				Dendrite->Strength *= -1.0f;
			}
			else if (RandomIf > 23)
			{
				Dendrite->Strength *= (RandomSign > 0 ? 0.5f : 2.0f);
			}
			else if (RandomIf > 21)
			{
				Dendrite->Sender = -1;
				while (Dendrite->Sender == -1)
				{
					int TestSender = rand() % Net->NumNeurons;

					for (int i = 0; i < Neuron->NumCurrentDendrites; i++)
					{		
						if (TestSender == Neuron->Dendrites[i].Sender)
						{
							TestSender = -1;
						}
					}
					Dendrite->Sender = TestSender;
				}

			}
			else
			{
				Dendrite->Strength *= RandomAmount;
			}
		}
	}
}

extern "C" GAME_UPDATE(GameUpdate)
{
	Assert(&Input->Controllers->Buttons[ArrayCount(Input->Controllers->Buttons)] == &Input->Controllers->Terminator);
	Assert(sizeof(state) <= Memory->PermanentStorageSize);

	state* State = (state*) Memory->PermanentStorage;

	if (!Memory->IsInitialized)
	{
		srand(4);

		debug_state* DebugState = &State->DebugState;
		//DebugState->DebugFile = fopen("errlog.txt", "w");
		DebugState->DebugNum1 = 0;
		DebugState->DebugNum2 = 0;
		DebugState->GraphSize = GRAPHSIZE;
		DebugState->DebugMode = 1;
		DebugState->CreatureToDraw = 6;
		DebugState->NeuronToDraw = 7;
		DebugState->DendriteToDraw = 1;
		DebugState->Fast = 1;

		InitializeArena(&State->WorldArena, 
								Memory->PermanentStorageSize - sizeof(state), 
								(uint8_t*)Memory->PermanentStorage + sizeof(state));
		
		State->NumCreatures = NUMCREATURES;
		State->Creatures = PushArray(&State->WorldArena, creature, State->NumCreatures);
		for (int CreatureIndex = 0; CreatureIndex < State->NumCreatures; CreatureIndex++)
		{
			creature* Creature = State->Creatures + CreatureIndex;
			CreatureInitialize(Creature);
		}
		
		State->Nets = PushArray(&State->WorldArena, neural_net, State->NumCreatures);
		

		neural_net* InitialNet = State->Nets;
		InitialNet->NumNeurons = NUMNEURONS;
		InitialNet->NumDendrites = NUMDENDRITES;
		InitialNet->NumSensorNeurons = 6;
		InitialNet->NumMotorNeurons = 2;
		NeuralNetInitialize(InitialNet);
		InitialNet->CurrentFitness = CalculateCreatureFitness(State->Creatures);

		for (int NetIndex = 1; NetIndex < State->NumCreatures; NetIndex++)
		{
			neural_net* Net = State->Nets + NetIndex;
			NeuralNetCopy(InitialNet, Net);
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
		}
		if (Controller->IsAnalog)
		{
		}
	}

	keyboard_input* Keyboard = &Input->KeyboardInput;
	if (ButtonToggled(&Keyboard->Keys[AsciiToIndex('M')]))	
	{
		State->DebugState.Fast = !State->DebugState.Fast;
	}	

	if (!State->DebugState.DebugForce)
	{
		if (State->DebugState.Fast)
		{
			while(State->Creatures->Age != NUMTESTCYCLES - 2)
			{
				DoOneCycle(State);
			}
			DoOneCycle(State);
		}
		else
		{
			DoOneCycle(State);
		}
	}
}


//must be very fast, no more than 1-2 ms right now
//reduce the pressure on its performance by measuring it
//or asking about it, etc.
extern "C" GAME_GET_SOUND_SAMPLES(GameGetSoundSamples)
{
	state* State = (state*) Memory->PermanentStorage;
	GameOutputSound(State, SoundBuffer);
}


extern "C" GAME_RENDER(GameRender)
{
	Assert(sizeof(state) <= Memory->PermanentStorageSize);

	if(Memory->IsInitialized == false)
	{
		//Crash();
		DrawRectangle(Buffer, 0.0f, 0.0f, (float)Buffer->Width, (float)Buffer->Height, 0.0f, 0.0f, 0.0f);
		return;
	}

	state* State = (state*) Memory->PermanentStorage;

	debug_state* DebugState = &State->DebugState;

	world* World = State->World;	
	tile_map* TileMap = World->TileMap;

	int32_t TileSideInPixels = 1;
	float PixelsPerMeter = (float)TileSideInPixels / (float)TileMap->TileSideInMeters;

	DrawRectangle(Buffer, 0.0f, 0.0f, (float)Buffer->Width, (float)Buffer->Height, 0.0f, 0.0f, 0.0f);


	for (int CreatureIndex = 0; CreatureIndex < State->NumCreatures; CreatureIndex++)
	{
		creature* Creature = State->Creatures + CreatureIndex;
		//if (CreatureIndex == DebugState->CreatureToDraw)
		//{
			float FitnessScale1, FitnessScale2, FitnessScale3;
			ColorValues(Creature->Reward, &FitnessScale1, &FitnessScale2, &FitnessScale3);

			//DrawRectangle(Buffer, Creature->PositionX - 4, Creature->PositionY - 4, Creature->PositionX + 3, Creature->PositionY + 3, FitnessScale1, FitnessScale2, FitnessScale3);
			DrawLine(Buffer, (float)Creature->GoalX, (float)Creature->GoalY, (float)Creature->PositionX, (float)Creature->PositionY, 0.0f, 1.0f, 0.0f);

		//}

	}

	if (!DebugState->DebugForce)
	{

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

			neuron* Neuron = Net->Neurons + NeuronIndex;

			float NeuronScale1, NeuronScale2, NeuronScale3;
			ColorValues(Neuron->Firing, &NeuronScale1, &NeuronScale2, &NeuronScale3);

			DrawRectangle(Buffer, XCoordinates[NeuronIndex] - 18, YCoordinates[NeuronIndex] - 18, XCoordinates[NeuronIndex] + 19, YCoordinates[NeuronIndex] + 19, NeuronScale1, NeuronScale2, NeuronScale3);

			if (NeuronIndex >= Net->NumSensorNeurons)
			{
				for (int DendriteIndex = 0; DendriteIndex < Neuron->NumCurrentDendrites; DendriteIndex++)
				{
					dendrite* Dendrite = &Neuron->Dendrites[DendriteIndex];
					float DendriteScale1, DendriteScale2, DendriteScale3;
					ColorValues(Dendrite->Strength, &DendriteScale1, &DendriteScale2, &DendriteScale3);
					DrawLine(Buffer, XCoordinates[NeuronIndex], YCoordinates[NeuronIndex], XCoordinates[Dendrite->Sender], YCoordinates[Dendrite->Sender], DendriteScale1, DendriteScale2, DendriteScale3);
					//drawline (curved?) function from sender position to current neuron position - additive so that both directions can be shown on one line?
				}
			}
		}
#endif
	}

	if (DebugState->DebugMode || DebugState->DebugForce)
	{

		float MaxValue = 0;
		float Average[960] = {0};
		int AverageProgress = 0;
		int DebugProgress = 0;

		int SumCount = 1 + (DebugState->DebugNum1 / 960);
		if (SumCount > DebugState->GraphSize / 960)
		{
			SumCount = DebugState->GraphSize / 960;
		}

		int End = DebugState->DebugNum1 / SumCount;
		if (End > DebugState->GraphSize / SumCount)
		{
			End = DebugState->GraphSize / SumCount;
		}

		while (AverageProgress < End)
		{
			float Sum = 0;
			for (int Count = 0; Count < SumCount; Count++)
			{
				Sum += DebugState->DebugGraph1[DebugProgress % DebugState->GraphSize];
				DebugProgress++;
			}

			Average[AverageProgress] = Sum / (float)SumCount;
			AverageProgress++;
		}

		for (int j = 0; j < AverageProgress; j++)
		{
			if (AbsVal(Average[j]) * 1.05f > MaxValue)
			{
				MaxValue = 1.05f * AbsVal(Average[j]);
			}
		}

		for (int j = 0; j < AverageProgress; j++)
		{
			DrawPixel(Buffer, (float)j, (float)Buffer->Height / 2.0f, 1.0f, 1.0f, 1.0f);

			float EndPixel = (float)Buffer->Height / 2.0f * (1.0f - Average[j] / MaxValue);
			/*			float StartPixel = (float)Buffer->Height / 2.0f * (1.0f - (Average[j - 1] / MaxValue)) + 1.0f * SignOf(Average[j - 1] - Average[j]);
						if (j == 0)
						{
						StartPixel = EndPixel;
						}
						DrawLine(Buffer, (float)j, StartPixel, (float)j, EndPixel, 1.0f, 1.0f, 1.0f);			*/
			DrawLine(Buffer, (float)j, EndPixel, (float)j, EndPixel + 3, 1.0f, 1.0f, 1.0f);
			//DrawPixel(Buffer, (float)j, EndPixel, 1.0f, 1.0f, 1.0f);
		}

	}

}
