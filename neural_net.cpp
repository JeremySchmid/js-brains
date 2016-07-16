#ifndef NEURAL_NET_H
	#include "neural_net.h"
#endif
#include "limits.h"

/*
internal boolint NeuralNetInitialize (neural_net* Net)

internal void NeuralNetUpdate (neural_net* Net, float* SensorValues, float* MotorValues, int Reward)
*/
#if 1
#define DebugAssert(Expression) if (!(Expression)) {DebugState->DebugForce = 1; *(int*)0 = 0;} //{NeuralNetInitialize(Net);}

internal void NeuralNetCleanFirings (neural_net* Net)
{
	for (int NeuronIndex = 0; NeuronIndex < Net->NumNeurons; NeuronIndex++)
	{
		neuron* Neuron = &Net->Neurons[NeuronIndex];

		Neuron->Firing = 0;
	}
}

internal void NeuralNetCopy (neural_net* OriginalNet, neural_net* TargetNet)
{

	*TargetNet = *OriginalNet;

	for (int NeuronIndex = 0; NeuronIndex < OriginalNet->NumNeurons; NeuronIndex++)
	{
		neuron* Neuron = &OriginalNet->Neurons[NeuronIndex];
		neuron* TargetNeuron = &TargetNet->Neurons[NeuronIndex];

		*TargetNeuron = *Neuron;

		for (int DendriteIndex = 0; DendriteIndex < OriginalNet->NumDendrites; DendriteIndex++)
		{
			dendrite* Dendrite = &Neuron->Dendrites[DendriteIndex];
			dendrite* TargetDendrite = &TargetNeuron->Dendrites[DendriteIndex]; 

			*TargetDendrite = *Dendrite;

		}
	}
}

internal void DendriteInitialize (neural_net* Net, neuron* Neuron, dendrite* Dendrite)
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

	float Base = 1.0f / Net->NumDendrites;
	float Factor = (float)((double)rand() / (double)RAND_MAX / 2.0f);
	int FactorSign = (rand() % 2 == 1 ? 1 : -1);
	int Sign = (rand() % 2 == 1 ? 1 : -1);

	Dendrite->Strength = Sign * Base * (1.0f + Factor * FactorSign);

	//Dendrite->CurrentWobble = 0.0f;
	//Dendrite->WobbleLimit = Dendrite->Strength * 0.5f;
	//Dendrite->WobbleStep = 0.005f;
	//Dendrite->WobblePiVal = (float)((double)rand() / (double)RAND_MAX * 6.28);
	//Dendrite->WobbleMagnitude = Dendrite->Strength * 0.1f;

}

internal boolint NeuralNetInitialize (neural_net* Net)
{

	for (int NeuronIndex = 0; NeuronIndex < Net->NumNeurons; NeuronIndex++) {
		
		neuron* Neuron = &Net->Neurons[NeuronIndex];

		Neuron->Firing = 0; //(float)((double)rand() / (double)RAND_MAX);

		Neuron->NumCurrentDendrites = 0;

		if (NeuronIndex >= Net->NumSensorNeurons)
		{
			//Dendrite Generation
			for (int DendriteIndex = 0; DendriteIndex < Net->NumDendrites; DendriteIndex++)
			{
				dendrite* Dendrite = &Neuron->Dendrites[DendriteIndex];

				DendriteInitialize(Net, Neuron, Dendrite);

				Neuron->NumCurrentDendrites++;
				//make it impossible to make two dendrites pointing to the same neuron?

			}
		}
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

internal void NeuralNetUpdate (debug_state* DebugState, neural_net* Net, float* SensorValues, float* MotorValues)
{
	neural_net Temp = *Net;

	NeuralNetCopy(Net, &Temp);

	for (int NeuronIndex = 0; NeuronIndex < Net->NumNeurons; NeuronIndex++)
	{

		neuron* TempNeuron = &Temp.Neurons[NeuronIndex];
		neuron* NewNeuron = &Net->Neurons[NeuronIndex];

		float TestFiring = 0;

		if (NeuronIndex >= Net->NumSensorNeurons)
		{
			for (int DendriteIndex = 0; DendriteIndex < Net->NumDendrites; DendriteIndex++)
			{
				int SenderNumber = TempNeuron->Dendrites[DendriteIndex].Sender;

				dendrite* TempDendrite = &TempNeuron->Dendrites[DendriteIndex];
				dendrite* NewDendrite = &NewNeuron->Dendrites[DendriteIndex];

				neuron* NeuronOfDendrite = &Net->Neurons[SenderNumber];
				//float Predamp = NeuronOfDendrite->Firing * (OldDendrite->Strength + OldDendrite->CurrentWobble);
				//TestFiring += (float)(SignOf(Predamp) * sqrt(AbsVal(Predamp)));
				TestFiring += NeuronOfDendrite->Firing * TempDendrite->Strength;

				if (NeuronIndex >= Net->NumSensorNeurons)
				{
					DebugAssert(isfinite(TestFiring));
				}

				//change dendritestrengths according to their relative magnitudes of influencing their neuron?? how to implement dendrites being recently used becoming susceptible to reward?
				//make dendrite-strength the likelihood that the NeuronOfDendrite will be read and used, or maybe make it a value shoved through a 1-expdecay function? -- not useful; adds non-useful randomness to the calculations
				//use this ^ to implement reward susceptibility? right now reward is equally important to all dendrites and neurons, and thats not useful in separatingthem? - cant know if they're actually moving in sync and all until you implement grabbing all the fucken data -> that ^ cant be used - the brain uses the strength of the dendrite to determine frequency(in ur system intensity) and the randomness is both mostly removed thru large-scale-predictable randomness and not useful in your system -> implement a minimum value a neuron must fire to fire at all? use that as reward susceptibility? make the neuron firings andor dendritestrengths logistic or 1-expdecay funcs?
				//remove the dendrites being checked on the sensor neurons - not really important, but they're a source of infinities/crashes that dont actually matter to the system

				if (NeuronIndex == DebugState->NeuronToDraw
						&& DendriteIndex == DebugState->DendriteToDraw
						&& DebugState->CreatureIndex == DebugState->CreatureToDraw)
				{
					//DebugState->DebugGraph[DebugState->DebugNum % DebugState->GraphSize] = Net->Reward;
					//DebugState->DebugGraph[DebugState->DebugNum % DebugState->GraphSize] = NewDendrite->Strength; 
					//DebugState->DebugGraph[DebugState->DebugNum % DebugState->GraphSize] = TempNeuron->Firing;
					//DebugState->DebugNum++;
				}
			}

			float Persistence = .98f;
			TestFiring = (TempNeuron->Firing * Persistence) + (TestFiring * (1.0f - Persistence));

			DebugAssert(isfinite(TestFiring));
		}
		else if (NeuronIndex < Net->NumSensorNeurons)
		{
			TestFiring = SensorValues[NeuronIndex];
			DebugAssert(isfinite(TestFiring));
		}

		NewNeuron->Firing = TestFiring;

		int MotorNumber = Net->NumNeurons - 1 - NeuronIndex;
		if (MotorNumber < Net->NumMotorNeurons)
		{
			MotorValues[MotorNumber] = NewNeuron->Firing;
			DebugAssert(MotorValues[MotorNumber] == MotorValues[MotorNumber]);
		}
		
	}
	return;
}

#endif
