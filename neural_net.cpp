#ifndef NEURAL_NET_H
	#include "neural_net.h"
#endif
#include "limits.h"

/*
internal boolint NeuralNetInitialize (neural_net* Net, void** Memory, uint64_t* MemorySize)

internal void NeuralNetUpdate (neural_net* Net, float* SensorValues, float* MotorValues, int RewardPunish)
*/
#if 1
#define DebugAssert(Expression) if (!(Expression)) {DebugState->DebugForce = 1; *(int*)0 = 0;}
internal void DendriteInitialize(neural_net* Net, dendrite* Dendrite, dendrite* OtherDendrite)
{
	Dendrite->Sender = rand() % Net->NumOfNeurons;

	float Base = 1.0f / Net->NumDendrites;
	float Factor = (float)((double)rand() / (double)RAND_MAX);
	int FactorSign = (rand() % 2 == 1 ? 1 : -1);
	int Sign = (rand() % 2 == 1 ? 1 : -1);

	Dendrite->Strength = Sign * Base * (1.0f + Factor * FactorSign);

	Dendrite->CurrentWobble = 0.0f;
	Dendrite->WobblePiVal = (float)((double)rand() / (double)RAND_MAX * 6.28);
	Dendrite->WobbleMagnitude = Dendrite->Strength * 0.1f;

	*OtherDendrite = *Dendrite;
}

internal boolint NeuralNetInitialize (neural_net* Net)
{

	Net->Toggle = 0;

	for (int NeuronIndex = 0; NeuronIndex < Net->NumOfNeurons; NeuronIndex++) {
		
		neuron* Neuron = &Net->Neurons[NeuronIndex * 2 + Net->Toggle];
		neuron* PairedNeuron = &Net->Neurons[NeuronIndex * 2 + !Net->Toggle];

		Neuron->Firing = (float)((double)rand() / (double)RAND_MAX); //0;

		//Dendrite Generation
		for (int DendriteIndex = 0; DendriteIndex < Net->NumDendrites; DendriteIndex++) {

			dendrite* Dendrite = &Neuron->Dendrites[DendriteIndex];
			dendrite* PairedDendrite = &PairedNeuron->Dendrites[DendriteIndex];

			DendriteInitialize(Net, Dendrite, PairedDendrite);

			//make it impossible to make two dendrites pointing to the same neuron
			
		}
		*PairedNeuron = *Neuron;
	}
	return true;
}
/*
internal int32_t CheckMagnitude (int64_t TestFiring)
{
	if (TestFiring > 2147483647) {
		return 2147483647;
	}
	else if (TestFiring < -2147483648) {
		return -2147483648;
	}
	else {
		return (int32_t) TestFiring;
	}
}
*/

internal int Sign (float Number)
{
	int Result = 0;

	if (Number < 0)
	{
		Result = -1;
	}
	else if (Number > 0)
	{
		Result = 1;
	}

	return Result;
}

internal float AbsoluteValue (float Number)
{
	float Result = Number;
	if (Result < 0)
	{
		Result = -Result;
	}
	return Result;
}

internal void NeuralNetUpdate (debug_state* DebugState, neural_net* Net, float* SensorValues, float* MotorValues, float RewardPunish, boolint Debug)
{
	for (int NeuronIndex = 0; NeuronIndex < Net->NumOfNeurons; NeuronIndex++)
	{

		neuron* OldNeuron = &Net->Neurons[NeuronIndex * 2 + Net->Toggle];
		neuron* NewNeuron = &Net->Neurons[NeuronIndex * 2 + !Net->Toggle];

		float TestFiring = 0;

		for (int DendriteIndex = 0; DendriteIndex < Net->NumDendrites; DendriteIndex++)
		{
			int SenderNumber = OldNeuron->Dendrites[DendriteIndex].Sender * 2 + Net->Toggle;

			dendrite* OldDendrite = &OldNeuron->Dendrites[DendriteIndex];
			dendrite* NewDendrite = &NewNeuron->Dendrites[DendriteIndex];
			
			neuron* NeuronOfDendrite = &Net->Neurons[SenderNumber];
			float Predamp = NeuronOfDendrite->Firing * (OldDendrite->Strength + OldDendrite->CurrentWobble);
			TestFiring += (float)(SignOf(Predamp) * sqrt(AbsVal(Predamp)));
			//TestFiring += NeuronOfDendrite->Firing * (OldDendrite->Strength + OldDendrite->CurrentWobble);

			DebugAssert(isfinite(TestFiring));

			//exchange wobblemagnitube and wobblepival with wobblestep and wobblelimit??
			//TestWobbleMagnitude needs a far better system -> this results in >-1 values flipping the sign, and anything with abs(val)>10 is just absurdly ballooning the WM enough that it completely dwarfs whatever the dendriteStrength is
			DebugAssert(isfinite(RewardPunish));
			float WobbleFactor = (float)(1.0 + AbsVal(RewardPunish));
			
			float TestWobbleMagnitude = NewDendrite->WobbleMagnitude * WobbleFactor;
			if (isfinite(TestWobbleMagnitude))
			{
				NewDendrite->WobbleMagnitude = TestWobbleMagnitude;
			}
			DebugAssert(isfinite(NewDendrite->WobbleMagnitude));
			
			if(OldDendrite->WobbleMagnitude < 0.5f)
			{
				NewDendrite->WobbleMagnitude = 0.5f;
				DebugAssert(isfinite(NewDendrite->WobbleMagnitude)); 
			}

			if (RewardPunish > 0)
			{
				
				NewDendrite->Strength = OldDendrite->Strength + 0.5f * OldDendrite->CurrentWobble;
				DebugAssert(isfinite(NewDendrite->Strength));
				NewDendrite->WobbleMagnitude /= WobbleFactor;
			}

			NewDendrite->WobblePiVal = OldDendrite->WobblePiVal + 0.01f;
			if (NewDendrite->WobblePiVal >= 6.28f)
			{
				NewDendrite->WobblePiVal -= 6.28f;
			}

			NewDendrite->CurrentWobble = (float)(NewDendrite->WobbleMagnitude * sin(NewDendrite->WobblePiVal));
			DebugAssert(isfinite(NewDendrite->CurrentWobble));
			

		}

		TestFiring /= Net->NumDendrites;

		float Persistence = .98f;
		TestFiring = (OldNeuron->Firing * Persistence) + (TestFiring * (1.0f - Persistence));
		
		DebugAssert(isfinite(TestFiring));

		if (NeuronIndex < Net->NumSensorValues)
		{
			TestFiring = SensorValues[NeuronIndex];
			DebugAssert(isfinite(TestFiring));
		}

		if (TestFiring == TestFiring && NeuronIndex == 14 && Debug == 1)
		{
			DebugState->DebugGraph[DebugState->DebugNum % 960] = RewardPunish;
			DebugState->DebugNum++;
			//if (DebugState->DebugNum >= 1000)
			//{
			//	DebugState->DebugMode = 1;
			//}
		}

		NewNeuron->Firing = TestFiring;
		
		int MotorNumber = Net->NumOfNeurons - 1 - NeuronIndex;
		if (MotorNumber < Net->NumMotorValues)
		{
			MotorValues[MotorNumber] = NewNeuron->Firing;
			DebugAssert(MotorValues[MotorNumber] == MotorValues[MotorNumber]);
		}
		
	}
	
	Net->Toggle = !Net->Toggle;
	return;
}

#endif
