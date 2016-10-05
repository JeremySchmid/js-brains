#ifndef NEURAL_NET_H

#include "magic_numbers.h"

#if 1
typedef struct dendrite
{
	int Sender;

	float Strength;

	float RewardSusceptibility;

	//float CurrentWobble;

	//float WobbleMAgnitude;
	//float WobblePiVal;

	//float WobbleLimit;
	//float WobbleStep;
	
} dendrite;

typedef struct neuron
{
	float Firing;
	int NumCurrentDendrites;

	dendrite Dendrites[NUMDENDRITES];

} neuron;

typedef struct neural_net
{
	int NumNeurons;
	int NumDendrites;
	int NumSensorNeurons;
	int NumMotorNeurons;

	float CurrentFitness;
	float LastFitness;
	float Reward;

	neuron Neurons[NUMNEURONS * 2];

	boolint Toggle;	
} neural_net;

#endif
#define NEURAL_NET_H
#endif
