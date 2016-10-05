#if !defined(BASE_TILES_H)

typedef struct tile_map_position
{
	//Take the TileChunkX and TileX and pack into a single 32-bit value for x and y
	//where there is some low bits for the tile index and
	//the high bits are the tile "page"/TileChunk
	//
	//Note: we can eliminate the current need for floor at the same time!
	uint32_t AbsTileX;
	uint32_t AbsTileY;
	uint32_t AbsTileZ;

	//Tile-relative X and Y
	//TODO: Consider naming offsetX and Y
	float OffsetX;
	float OffsetY;

} tile_map_position;

typedef struct tile_chunk_position
{
	uint32_t TileChunkX;
	uint32_t TileChunkY;
	uint32_t TileChunkZ;

	uint32_t RelTileX;
	uint32_t RelTileY;

} tile_chunk_position;

typedef struct tile_chunk
{
	uint32_t* Tiles;

} tile_chunk;

typedef struct tile_map
{
	
	uint32_t ChunkShift;
	uint32_t ChunkMask;
	uint32_t ChunkDim;

	float TileSideInMeters;

	//TODO: Real sparseness so anywhere in the world can be
	//represented without the giant pointer array
	uint32_t TileChunkCountX;
	uint32_t TileChunkCountY;
	uint32_t TileChunkCountZ;

	tile_chunk *TileChunks;

} tile_map;

#define BASE_TILES_H
#endif
