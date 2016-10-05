#ifndef NEURAL_NET_H
	#include "neural_net.h"
#endif

#include "magic_numbers.h"
#include "base.h"
#include "limits.h"

#if 1
#define DebugAssert(Expression) if (!(Expression)) {DebugState->DebugForce = 1; *(int*)0 = 0;} //{NeuralNetInitialize(Net);}

internal void NeuralNetCleanFirings (neural_net* Net)
{
	for (int NeuronIndex = 0; NeuronIndex < Net->NumNeurons; NeuronIndex++)
	{
		neuron* Neuron = &Net->Neurons[NeuronIndex * 2 + Net->Toggle];
		neuron* PairedNeuron = &Net->Neurons[NeuronIndex * 2 + !Net->Toggle];

		Neuron->Firing = 0;
		PairedNeuron->Firing = 0;
	}
}

internal void NeuralNetCopy (neural_net* OriginalNet, neural_net* TargetNet)
{

	*TargetNet = *OriginalNet;

	for (int NeuronIndex = 0; NeuronIndex < OriginalNet->NumNeurons * 2; NeuronIndex++)
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
	float FactorDet = (float)((double)rand() / (double)RAND_MAX);
	float Factor = (rand() % 2 == 0 ? 1.0f + FactorDet : 1.0f / (1.0f + FactorDet));
	int Sign = (rand() % 2 == 0 ? 1 : -1);

	Dendrite->Strength = Sign * Base * Factor;
	
	Dendrite->RewardSusceptibility = 0;
	//Dendrite->CurrentWobble = 0.0f;
	//Dendrite->WobbleLimit = Dendrite->Strength * 0.5f;
	//Dendrite->WobbleStep = 0.005f;
	//Dendrite->WobblePiVal = (float)((double)rand() / (double)RAND_MAX * 6.28);
	//Dendrite->WobbleMagnitude = Dendrite->Strength * 0.1f;

}

internal boolint NeuralNetInitialize (neural_net* Net)
{

	for (int NeuronIndex = 0; NeuronIndex < Net->NumNeurons; NeuronIndex++) {
		
		neuron* Neuron = &Net->Neurons[NeuronIndex * 2 + Net->Toggle];
		neuron* PairedNeuron = &Net->Neurons[NeuronIndex * 2 + !Net->Toggle];

		Neuron->Firing = 0; //(float)((double)rand() / (double)RAND_MAX);

		Neuron->NumCurrentDendrites = 0;

		*PairedNeuron = *Neuron;
		if (NeuronIndex >= Net->NumSensorNeurons)
		{
			//Dendrite Generation
			for (int DendriteIndex = 0; DendriteIndex < Net->NumDendrites; DendriteIndex++)
			{
				dendrite* Dendrite = &Neuron->Dendrites[DendriteIndex];
				dendrite* PairedDendrite = &PairedNeuron->Dendrites[DendriteIndex];

				DendriteInitialize(Net, Neuron, Dendrite);

				*PairedDendrite = *Dendrite;
				Neuron->NumCurrentDendrites++;
				PairedNeuron->NumCurrentDendrites++;
			}
		}
	}
	Net->Toggle = 0;
	return true;
}

internal void NeuralNetUpdate (debug_state* DebugState, neural_net* Net, float* SensorValues, float* MotorValues)
{

	for (int NeuronIndex = 0; NeuronIndex < Net->NumNeurons; NeuronIndex++)
	{

		neuron* NewNeuron = &Net->Neurons[NeuronIndex * 2 + Net->Toggle];
		neuron* OldNeuron = &Net->Neurons[NeuronIndex * 2 + !Net->Toggle];

		float TestFiring = 0;

		if (NeuronIndex >= Net->NumSensorNeurons)
		{
			for (int DendriteIndex = 0; DendriteIndex < Net->NumDendrites; DendriteIndex++)
			{
				int SenderNumber = OldNeuron->Dendrites[DendriteIndex].Sender;

				dendrite* OldDendrite = &OldNeuron->Dendrites[DendriteIndex];
				dendrite* NewDendrite = &NewNeuron->Dendrites[DendriteIndex];

				neuron* NeuronOfDendrite = &Net->Neurons[SenderNumber * 2 + !Net->Toggle];
				float DendriteFire = NeuronOfDendrite->Firing * OldDendrite->Strength;
				//NewDendrite->RewardSusceptibility = .25f * OldDendrite->RewardSusceptibility + 0.7f * AbsVal(DendriteFire);
				// * OldDendrite->RewardSusceptibility
				NewDendrite->Strength = .999f * OldDendrite->Strength + 0.05f * OldDendrite->Strength * Net->Reward * (Net->Reward > 0 ? 2.0f : 1.0f); 
				if (NewDendrite->Strength < 0.001f)
				{
					DendriteInitialize(Net, NewNeuron, NewDendrite);
					NewDendrite->Strength *= 2;
					*OldDendrite = *NewDendrite;
				}
				TestFiring += DendriteFire;

				

				DebugAssert(isfinite(TestFiring));

				//change dendritestrengths according to their relative magnitudes of influencing their neuron?? how to implement dendrites being recently used becoming susceptible to reward?
				//make dendrite-strength the likelihood that the NeuronOfDendrite will be read and used, or maybe make it a value shoved through a 1-expdecay function? -- not useful; adds non-useful randomness to the calculations
				//use this ^ to implement reward susceptibility? right now reward is equally important to all dendrites and neurons, and thats not useful in separatingthem? - cant know if they're actually moving in sync and all until you implement grabbing all the fucken data -> that ^ cant be used - the brain uses the strength of the dendrite to determine frequency(in ur system intensity) and the randomness is both mostly removed thru large-scale-predictable randomness and not useful in your system -> implement a minimum value a neuron must fire to fire at all? use that as reward susceptibility? make the neuron firings andor dendritestrengths logistic or 1-expdecay funcs?
				//remove the dendrites being checked on the sensor neurons - not really important, but they're a source of infinities/crashes that dont actually matter to the system


				//change neurons to use logistic functions with some high-magnitude boundary and a near-1 slope for most of that?

				if (NeuronIndex == DebugState->NeuronToDraw
						&& DendriteIndex == DebugState->DendriteToDraw
						&& DebugState->CreatureIndex == DebugState->CreatureToDraw)
				{
					//DebugState->DebugGraph[DebugState->DebugNum % DebugState->GraphSize] = Net->Reward;
					DebugState->DebugGraph[DebugState->DebugNum % DebugState->GraphSize] = NewDendrite->Strength;
					//DebugState->DebugGraph[DebugState->DebugNum % DebugState->GraphSize] = NewDendrite->RewardSusceptibility;
					//DebugState->DebugGraph[DebugState->DebugNum % DebugState->GraphSize] = TempNeuron->Firing;
					DebugState->DebugNum++;
				}
			}

			float Persistence = .98f;
			TestFiring = (OldNeuron->Firing * Persistence) + (TestFiring * (1.0f - Persistence));

			ForceFloatInBounds(&TestFiring, -1000000.0f, 1000000.0f);

			DebugAssert(isfinite(TestFiring));
		}
		else if (NeuronIndex < Net->NumSensorNeurons)
		{
			TestFiring = SensorValues[NeuronIndex];
			DebugAssert(isfinite(TestFiring));
		}

		NewNeuron->Firing = TestFiring;

		int MotorNumber = NeuronIndex - (Net->NumNeurons - Net->NumMotorNeurons);
		if (MotorNumber >= 0)
		{
			MotorValues[MotorNumber] = NewNeuron->Firing;
			DebugAssert(MotorValues[MotorNumber] == MotorValues[MotorNumber]);
		}
		
	}
	Net->Toggle = !Net->Toggle;
	return;
}

#endif
