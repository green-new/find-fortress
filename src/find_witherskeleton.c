// find a good wither skeleton farm location 
// soul sand valley biomes have a low spawn rate and thus fill the mob cap very slowly,
// leaving more room for wither skeleton spawns. they require very little spawnproofing so little effort is needed compared
// to pre-1.16 wither head farms
// i have no knowledge of how you could generate positions for 2x2 crossroads to filter out more of the seed space,
// thus larger farms, but i'm not sure in this instance area would increase spawn rates.
// it seems like a typical seed has perhaps hundreds of thousands of these fortresses totally surrounded by soul sand valleys
// for the default settings, there is typically only 1-5 of these structures within 10,000 blocks of the origin

// requirements: must be a nether fortress
// surrounding 256x256 area (in blocks) must be the soul sand valley biome

#include "finders.h"
#include <stdio.h>
#include <math.h>
#include <stdlib.h> 
#include <time.h> 

struct s
{
	uint64_t seed;
	uint64_t sha;
	NetherNoise nn;
	StructureConfig sc;
	int range;
	int area;
	int mc;
	FILE *fp;
} settings;


// distance from origin
// note: Pos has members only of type signed int, overflow imminent for distances over 2^15 or even less
float distance(Pos p)
{
	return sqrt((p.x * p.x) + (p.z * p.z));
}

// check if the char data is entirely numerical
int numcheck(char *text)
{
	int j;
    j = strlen(text);
    while(j--)
    {
        if(isdigit(text[j]) || text[j] == '-')
            continue;

        return 0;
    }
    return 1;
}

// surrounding area must be a soul sand valley biome
// lessen area to give a little lee-way if desired
int soul_sand_valley_check(const NetherNoise *nn, uint64_t sha, Pos p, int areaHalf)
{
	// half the area to parse a square around Pos p
	areaHalf /= 2;
	int cornerX, cornerY = 0, cornerZ;
	int limitCornerX = p.x + areaHalf, limitCornerZ = p.z + areaHalf;
	for (cornerX = p.x - areaHalf; cornerX <= limitCornerX; cornerX++)
		for (cornerZ = p.z - areaHalf; cornerZ <= limitCornerZ; cornerZ++)
		{
			// convert 1:1 coordinates to 1:4 for nether biome access
			// use y=0 as they are used for structure spawning
			int scaledX = 0, scaledY = 0, scaledZ = 0;
			voronoiAccess3D(sha, cornerX, cornerY, cornerZ, &scaledX, &scaledY, &scaledZ);
			int biome = getNetherBiome(nn, scaledX, scaledY, scaledZ, NULL);
			if (biome != soul_sand_valley)
			{
				return 0;
			}
		}
	return 1;
}
//
// initalization
//
int init()
{
	settings.fp = fopen("out.txt", "w");
	if (settings.fp == NULL)
	{
		printf("file can't be opened\n");
		return 0;
	}
	srand((unsigned)time(NULL));
	settings.mc = MC_1_17;
	settings.range = 10000;
	settings.area = 256;
	settings.seed = (((uint64_t)rand()) << 32) + ((uint64_t)rand());
	settings.sha = getVoronoiSHA(settings.seed);
	getStructureConfig(Fortress, settings.mc, &settings.sc);
	setNetherSeed(&settings.nn, settings.seed);
	
	return 1;
}

//
// user input
//
int input()
{
	char str_seed[50];
	char str_range[50];
	char str_area[50];
	printf("seed: ");
	gets(str_seed);
	if (!numcheck(str_seed))
	{
		printf("ERROR: provided seed is not a number");
		return 0;
	}
	
	printf("range (10,000 blocks is default): ");
	gets(str_range);
	if (!numcheck(str_range))
	{
		printf("ERROR: provided range is not a number");
		return 0;
	}
	
	printf("area to check for soul sand valley (256 is default): ");
	gets(str_area);
	if (!numcheck(str_area))
	{
		printf("ERROR: provided area is not a number");
		return 0;
	}
	
	uint64_t b_seed;
	int b_range;
	int b_area;
	if (str_area[0] != '\0')
	{
		sscanf(str_area, "%d", &b_area);
		settings.area = b_area;
	}
	if (str_seed[0] != '\0')
	{
		sscanf(str_seed, "%" PRId64, &b_seed);
		settings.seed = b_seed;
	}
	if (str_range[0] != '\0')
	{
		sscanf(str_range, "%d", &b_range);
		settings.range = b_range;
	}
	
	// world limit 
	// these are ints anyway?
	if (settings.range > 29999984)
		settings.range = 29999984;
	if (settings.area > 299999984/2)
		settings.area = 299999984/2;
	
	// update settings
	settings.sha = getVoronoiSHA(settings.seed);
	setNetherSeed(&settings.nn, settings.seed);
	
	return 1;
}

int main()
{
	//
	// initalization
	//
	if (!init())
		exit(1);
	
	//
	// user input
	//
	if (!input())
		exit(1);
	
	
	// convert our range (in blocks) to region coordinates (scaled by structure region size, some are 32, in this case fortresses are 27x27 chunks)
	int maxReg = settings.range / (int)(settings.sc.regionSize << 4); 
	int regX, regZ;
	int n = 0;
	
	printf("\nusing seed %" PRId64 ", range of %d blocks, area of %d*%d:\n\n", (uint64_t)settings.seed, settings.range, settings.area, settings.area);
	fprintf(settings.fp, "seed: %" PRId64 ", range (in blocks): %d, area: %d*%d for MC version %d\n", (uint64_t)settings.seed, settings.range, settings.area, settings.area, settings.mc);
	//
	// find a fortress
	//
	for (regX = -maxReg; regX < maxReg; regX++)
		for (regZ = -maxReg; regZ < maxReg; regZ++)
		{
			Pos p;
			// fortress may not always spawn here (bastions, etc)
			if (!getStructurePos(Fortress, settings.mc, settings.seed & MASK48, regX, regZ, &p))
			{
				continue; // ugly
			}
			else
			{
				if (isViableNetherStructurePos(Fortress, settings.mc, &settings.nn, settings.seed, p.x, p.z))
				{
					// 
					// biome area check
					//
					if (soul_sand_valley_check(&settings.nn, settings.sha, p, settings.area) > 0)
					{
						float hypot = distance(p);
						printf("suitable fortress at (%d, %d) -> %.0f blocks away from 0, 0\n", p.x, p.z, hypot);
						fprintf(settings.fp, "(%d, %d) d = %.0f\n", p.x, p.z, hypot);
						n++;
					}
				}
			}
		}
	if (n <= 0)
	{
		printf("no suitable fortress(es) found\n");
		fprintf(settings.fp, "no suitable fortress(es) found\n");
	}
	else
	{
		printf("found %d suitable fortress(es)\n", n);
		fprintf(settings.fp, "found %d suitable fortress(es)\n", n);
	}
	printf("\nwrote to file out.txt\n");
	return 0;
}
