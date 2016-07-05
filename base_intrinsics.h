#if !defined(BASE_INTRINSICS_H)

internal int SignOf (float Number)
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

internal float AbsVal (float Number)
{
	float Result = Number;
	if (Result < 0)
	{
		Result = -Result;
	}
	return Result;
}

//TODO: Convert all to efficient and remove math.h

inline int32_t RoundFloatToInt32(float Decimal)
{
	//still gotta convert this stuff
	int32_t Result = (int32_t)roundf(Decimal);
	return Result;
}

//TODO: how to implement the math functions
inline int32_t FloorFloatToInt32(float Decimal)
{
	int32_t Result = (int32_t)floorf(Decimal);
	return Result;
}

inline int32_t TruncateFloatToInt32(float Decimal)
{
	int32_t Result = (int32_t)Decimal;
	return Result;
}


inline uint32_t RoundFloatToUInt32(float Decimal)
{
	//DONT PASS NEGATIVE NUMBERS U IDIONS
	uint32_t Result = (uint32_t)(Decimal + 0.5f);
	return Result;
}

inline float Sin(float Angle)
{
	float Result = sinf(Angle);
	return Result;
}

inline float Cos(float Angle)
{
	float Result = cosf(Angle);
	return Result;
}

inline float ATan2(float Y, float X)
{
	float Result = atan2f(Y, X);
	return Result;
}

#define BASE_INTRINSICS_H
#endif
