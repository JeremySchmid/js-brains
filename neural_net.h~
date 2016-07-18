#ifndef NEURAL_NET_H

#define NUMNEURONS 10
#define NUMDENDRITES 6

#if 1
typedef struct dendrite
{
	int Sender;
	float Strength;

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

#define NUMTESTCYCLES 300

typedef struct neural_net
{
	int NumNeurons;
	int NumDendrites;
	int NumSensorNeurons;
	int NumMotorNeurons;

	float CurrentFitness;
	float LastFitness;
	float Reward;

	float PastRewards[NUMTESTCYCLES];
	
	neuron Neurons[NUMNEURONS];
	
} neural_net;

#endif
#define NEURAL_NET_H
#endif
