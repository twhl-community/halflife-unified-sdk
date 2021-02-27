#include <algorithm>
#include <cstdlib>
#include <cstdio>
#include <memory>

#define BSPVERSION	30
#define	TOOLVERSION	2


typedef struct
{
	int		fileofs, filelen;
} lump_t;

#define	LUMP_ENTITIES	0
#define	LUMP_PLANES		1
#define	LUMP_TEXTURES	2
#define	LUMP_VERTEXES	3
#define	LUMP_VISIBILITY	4
#define	LUMP_NODES		5
#define	LUMP_TEXINFO	6
#define	LUMP_FACES		7
#define	LUMP_LIGHTING	8
#define	LUMP_CLIPNODES	9
#define	LUMP_LEAFS		10
#define	LUMP_MARKSURFACES 11
#define	LUMP_EDGES		12
#define	LUMP_SURFEDGES	13
#define	LUMP_MODELS		14

#define	HEADER_LUMPS	15

typedef struct
{
	int			version;
	lump_t		lumps[HEADER_LUMPS];
} dheader_t;

int main(int argc, char* argv[])
{
	if (argc < 2)
	{
		std::printf("Usage: BlueShiftBSPConverter <mapname>\n");
		return EXIT_FAILURE;
	}

	const char* mapName = argv[1];

	if (std::unique_ptr<FILE, decltype(std::fclose)*> file{std::fopen(mapName, "rb+"), &std::fclose}; file)
	{
		const auto start = std::ftell(file.get());

		if (start == -1L)
		{
			std::printf("Error getting start position in file \"%s\"\n", mapName);
			return EXIT_FAILURE;
		}

		dheader_t header{};

		if (std::fread(&header, sizeof(header), 1, file.get()) != 1)
		{
			std::printf("Error reading header from file \"%s\"\n", mapName);
			return EXIT_FAILURE;
		}

		//Swap the first 2 lumps
		std::swap(header.lumps[0], header.lumps[1]);

		if (std::fseek(file.get(), start, SEEK_SET))
		{
			std::printf("Error seeking to start in file \"%s\"\n", mapName);
			return EXIT_FAILURE;
		}

		if (std::fwrite(&header, sizeof(header), 1, file.get()) != 1)
		{
			std::printf("Error writing new header to file \"%s\"\n", mapName);
			return EXIT_FAILURE;
		}

		return EXIT_SUCCESS;
	}
	else
	{
		std::printf("Error opening file \"%s\"\n", mapName);
		return EXIT_FAILURE;
	}
}
