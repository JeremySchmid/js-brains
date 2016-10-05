#if !defined(BASE_TILES)
#include "base_intrinsics.h"
#include "base_tiles.h"

inline tile_chunk* GetTileChunk(tile_map* TileMap, uint32_t TileChunkX, uint32_t TileChunkY, uint32_t TileChunkZ)
{
	tile_chunk* TileChunk = 0;
		
	if (TileChunkX < TileMap->TileChunkCountX &&
			TileChunkY < TileMap->TileChunkCountY &&
			 TileChunkZ < TileMap->TileChunkCountZ) {
		TileChunk = &TileMap->TileChunks[
			TileChunkZ * TileMap->TileChunkCountX * TileMap->TileChunkCountY +
			TileChunkY * TileMap->TileChunkCountX +
			TileChunkX];
	}
	return TileChunk;
}

inline uint32_t GetRelativeTileValue(tile_map* TileMap, tile_chunk* TileChunk, uint32_t TileX, uint32_t TileY)
{
	uint32_t TileValue = 0;
	Assert(TileX < TileMap->ChunkDim);
	Assert(TileY < TileMap->ChunkDim);
	
	if(TileChunk && TileChunk->Tiles)
	{
		TileValue = TileChunk->Tiles[TileMap->ChunkDim * TileY + TileX];
	}

	return TileValue;
}

inline tile_chunk_position GetChunkPositionFor(tile_map* TileMap, uint32_t AbsTileX, uint32_t AbsTileY, uint32_t AbsTileZ)
{
	tile_chunk_position Result;

	Result.TileChunkX = AbsTileX >> TileMap->ChunkShift;
	Result.TileChunkY = AbsTileY >> TileMap->ChunkShift;
	Result.TileChunkZ = AbsTileZ;
	Result.RelTileX = AbsTileX & TileMap->ChunkMask;
	Result.RelTileY = AbsTileY & TileMap->ChunkMask;

	return Result;
}

inline uint32_t GetAbsoluteTileValue(tile_map* TileMap, uint32_t AbsTileX, uint32_t AbsTileY, uint32_t AbsTileZ)
{
	uint32_t TileValue = 0;
	tile_chunk_position ChunkPos = GetChunkPositionFor(TileMap, AbsTileX, AbsTileY, AbsTileZ);
	tile_chunk* TileChunk = GetTileChunk(TileMap, ChunkPos.TileChunkX, ChunkPos.TileChunkY, ChunkPos.TileChunkZ);
	TileValue = GetRelativeTileValue(TileMap, TileChunk, ChunkPos.RelTileX, ChunkPos.RelTileY);
	return TileValue;
}

inline uint32_t GetPositionTileValue(tile_map* TileMap, tile_map_position Pos)
{
	uint32_t TileValue = 0;
	TileValue = GetAbsoluteTileValue(TileMap, Pos.AbsTileX, Pos.AbsTileY, Pos.AbsTileZ);
	return TileValue;
}

internal boolint IsTileMapPointEmpty(tile_map* TileMap, tile_map_position TileMapPos)
{

	uint32_t TileValue = GetPositionTileValue(TileMap, TileMapPos);
	boolint Empty = ((TileValue == 1) ||
							(TileValue == 3) ||
							(TileValue == 4));

	return Empty;
}

inline void SetRelativeTileValue(tile_map* TileMap, tile_chunk* TileChunk, uint32_t TileX, uint32_t TileY, uint32_t TileValue)
{
	if(!TileChunk)
	{
	}
	Assert(TileX < TileMap->ChunkDim);
	Assert(TileY < TileMap->ChunkDim);
	TileChunk->Tiles[TileMap->ChunkDim * TileY + TileX] = TileValue;
	
	return;
}

internal void SetAbsoluteTileValue(memory_arena* Arena, tile_map* TileMap, uint32_t AbsTileX, uint32_t AbsTileY, uint32_t AbsTileZ, uint32_t TileValue)
{
	tile_chunk_position ChunkPos = GetChunkPositionFor(TileMap, AbsTileX, AbsTileY, AbsTileZ);
	tile_chunk* TileChunk = GetTileChunk(TileMap, ChunkPos.TileChunkX, ChunkPos.TileChunkY, ChunkPos.TileChunkZ);

	//TODO: On-demand tile chunk creation
	Assert (TileChunk);

	if (!TileChunk->Tiles)
	{
		uint32_t TileCount = TileMap->ChunkDim*TileMap->ChunkDim;
		TileChunk->Tiles = PushArray(Arena, uint32_t, TileCount);
		for (uint32_t TileIndex = 0;
				TileIndex < TileCount;
				++TileIndex)
		{
			TileChunk->Tiles[TileIndex] = 1;
		}
	}
	
	SetRelativeTileValue(TileMap, TileChunk, ChunkPos.RelTileX, ChunkPos.RelTileY, TileValue);

	return;
}

//
//TODO: These dont really have to do with tilemap storage - do these really belong in a positioning or geometry file?
//

inline void ResetCanonicalCoord(tile_map* TileMap, uint32_t* Tile, float* Offset)
{
	//TODO: Need to do something that doesn't use the divide and multiply method
	//bc it can end up rounding back onto the tile you came from
	//
	//Note: TileMap is assumed to be toroidal - step off one end and end up on the other
	int32_t TileOffset = RoundFloatToInt32(*Offset / (float)TileMap->TileSideInMeters);
	*Tile += TileOffset;
	Assert((*Offset - (float)TileOffset * (float)TileMap->TileSideInMeters) <= 0.5 * TileMap->TileSideInMeters);
	*Offset -= (float)TileOffset * (float)TileMap->TileSideInMeters;

	//TODO: fix floating point math so that > and < can be used
	Assert(*Offset >= -0.5f * TileMap->TileSideInMeters);
	Assert(*Offset <= 0.5f * TileMap->TileSideInMeters);

	return;
}

inline tile_map_position ResetCanonicalPosition(tile_map* TileMap, tile_map_position Pos)
{
	tile_map_position Result = Pos;
	
	ResetCanonicalCoord(TileMap, &Result.AbsTileX, &Result.OffsetX);
	ResetCanonicalCoord(TileMap, &Result.AbsTileY, &Result.OffsetY);

	return Result;
}

inline boolint AreOnSameTile(tile_map_position* A, tile_map_position* B)
{
	boolint Result = ((A->AbsTileX == B->AbsTileX) &&
								(A->AbsTileY == B->AbsTileY) &&
								(A->AbsTileZ == B->AbsTileZ));
	return Result;
}

#define BASE_TILES
#endif
