#ifndef NEURAL_NET_H

#define NUMNEURONS 15
#define NUMDENDRITES 4

#if 1
typedef struct dendrite
{
	int Sender;
	float Strength;

	float CurrentWobble;
	float WobblePiVal;
	float WobbleMagnitude;

	//or
	//this? still need to try it
	float WobbleLimit;
	float WobbleStep;//? - size of step that the wobble should step thru to reach wobblelimit

} dendrite;

typedef struct neuron
{
	float Firing;

	dendrite Dendrites[NUMDENDRITES];

} neuron;

typedef struct neural_net
{
	int NumOfNeurons;
	int NumDendrites;
	int NumSensorValues;
	int NumMotorValues;
	
	boolint Toggle;
	
	neuron Neurons[2 * NUMNEURONS];
	
} neural_net;

#endif
#define NEURAL_NET_H
#endif
