/*
   MG21Wadutil by Nicola Wrachien.

   Based on doomhack's GBAWadutil
   Original source: https://github.com/doomhack/GbaWadUtil

   This command line utility allows to convert your wad file
   to a format convenient for the Ikea Tradfri / MG21 Doom port.

   Usage:
   mg21wadutil <inputwad> <outputwad>
*/
#include "wadprocessor.h"
#include "doomtypes.h"
#include "wadfile.h"
#include <string.h>
#include <ctype.h>

static bool processD2Levels(wadfile_t *wadfile);
static bool processD1Levels(wadfile_t *wadfile);

static bool processLevel(wadfile_t *wadfile, uint32_t lumpNum);

static bool processVertexes(wadfile_t *wadfile, uint32_t lumpNum);
static bool processLines(wadfile_t *wadfile, uint32_t lumpNum);
static bool processSegs(wadfile_t *wadfile, uint32_t lumpNum);
static bool processSides(wadfile_t *wadfile, uint32_t lumpNum);

static bool processPNames();
static bool removeUnusedLumps(wadfile_t *wadfile, bool removeSound);

int getTextureNumForName(wadfile_t *wadfile, const char* tex_name);

bool processWad(wadfile_t *wadfile, bool removeSound)
{
    //Figure out if our IWAD is Doom or Doom2. (E1M1 or MAP01)


    removeUnusedLumps(wadfile, removeSound);

    int lumpNum = getLumpNumByName(wadfile, "MAP01");
    if(lumpNum != -1)
    {
        printf("Doom 2 level found\r\n");
        return processD2Levels(wadfile);
    }
    else
    {
        int lumpNum = getLumpNumByName(wadfile, "E1M1");

        //Can't find any maps.
        if(lumpNum == -1)
        {
            printf("No maps found\r\n");
            return false;
        }
        printf("Doom 1 level found\r\n");
    }

    return processD1Levels(wadfile);
}
bool processD2Levels(wadfile_t *wadfile)
{
    for(int m = 1; m <= 32; m++)
    {
        char mapName[9];
        snprintf(mapName, sizeof(mapName), "MAP%02d",m);
        int lumpNum = getLumpNumByName(wadfile, mapName);

        if(lumpNum != -1)
        {
            processLevel(wadfile, lumpNum);
        }
    }
    return true;
}

bool processD1Levels(wadfile_t *wadfile)
{
    for(int e = 1; e <= 4; e++)
    {
        for(int m = 1; m <= 9; m++)
        {
            char mapName[9];
            snprintf(mapName, sizeof(mapName), "E%dM%d",e,m);

            int lumpNum = getLumpNumByName(wadfile, mapName);

            if(lumpNum != -1)
            {
                printf("processing %s\r\n", mapName);
                processLevel(wadfile, lumpNum);
            }
            else
            {
                printf("cannot find %s\r\n", mapName);

            }
        }
    }

    return true;
}

bool processLevel(wadfile_t *wadfile, uint32_t lumpNum)
{
    processVertexes(wadfile, lumpNum);
    processLines(wadfile, lumpNum);
    processSegs(wadfile, lumpNum);
    processSides(wadfile, lumpNum);
    processPNames(wadfile);

    return true;
}

bool processVertexes(wadfile_t *wadfile, uint32_t lumpNum)
{
    uint32_t vtxLumpNum = lumpNum + ML_VERTEXES;

    lump_t *vxl;

    vxl = getLumpByNum(wadfile, vtxLumpNum);

    if (NULL == vxl)
        return false;

    if(vxl->lump.size == 0)
        return false;
    // get number of vertex
    uint32_t vtxCount = vxl->lump.size / sizeof(mapvertex_t);
    // allocate new vertex
    vertex_t* newVtx = malloc( sizeof (vertex_t) * vtxCount);
    //
    mapvertex_t* oldVtx = (mapvertex_t*) vxl->data;
    // precalculate vertex
    for(uint32_t i = 0; i < vtxCount; i++)
    {
        newVtx[i].x = (oldVtx[i].x << 16);
        newVtx[i].y = (oldVtx[i].y << 16);
    }
    // allocate new lump info
    lump_t *newVxl = malloc(sizeof (lump_t));
    // copy name
    memcpy(newVxl->lump.name, vxl->lump.name, sizeof (vxl->lump.name));
    // change data and size information
    newVxl->data = newVtx;
    newVxl->lump.size = vtxCount * sizeof(vertex_t);
    // replace lum
    replaceLump(wadfile, newVxl, vtxLumpNum);

    return true;
}

bool processLines(wadfile_t *wadfile, uint32_t lumpNum)
{
    uint32_t lineLumpNum = lumpNum+ML_LINEDEFS;

    lump_t *lines;

    lines = getLumpByNum(wadfile, lineLumpNum);

    if (NULL == lines)
        return false;

    if(lines->lump.size == 0)
        return false;

    uint32_t lineCount = lines->lump.size / sizeof(maplinedef_t);

    line_t* newLines = malloc (sizeof(line_t) * lineCount);

    maplinedef_t* oldLines = (maplinedef_t*) lines->data;

    //We need vertexes for this...

    uint32_t vtxLumpNum = lumpNum+ML_VERTEXES;

    lump_t *vxl;

    vxl = getLumpByNum(wadfile, vtxLumpNum);

    if (NULL == vxl)
        return false;

    if(vxl->lump.size == 0)
        return false;

    vertex_t* vtx = (vertex_t*)vxl->data;

    for(unsigned int i = 0; i < lineCount; i++)
    {
        newLines[i].v1.x = vtx[oldLines[i].v1].x;
        newLines[i].v1.y = vtx[oldLines[i].v1].y;

        newLines[i].v2.x = vtx[oldLines[i].v2].x;
        newLines[i].v2.y = vtx[oldLines[i].v2].y;

        newLines[i].special = oldLines[i].special;
        newLines[i].flags = oldLines[i].flags;
        newLines[i].tag = oldLines[i].tag;

        newLines[i].dx = newLines[i].v2.x - newLines[i].v1.x;
        newLines[i].dy = newLines[i].v2.y - newLines[i].v1.y;

        newLines[i].slopetype =
                !newLines[i].dx ? ST_VERTICAL : !newLines[i].dy ? ST_HORIZONTAL :
                FixedDiv(newLines[i].dy, newLines[i].dx) > 0 ? ST_POSITIVE : ST_NEGATIVE;

        newLines[i].sidenum[0] = oldLines[i].sidenum[0];
        newLines[i].sidenum[1] = oldLines[i].sidenum[1];

        newLines[i].bbox[BOXLEFT] = (newLines[i].v1.x < newLines[i].v2.x ? newLines[i].v1.x : newLines[i].v2.x);
        newLines[i].bbox[BOXRIGHT] = (newLines[i].v1.x < newLines[i].v2.x ? newLines[i].v2.x : newLines[i].v1.x);

        newLines[i].bbox[BOXTOP] = (newLines[i].v1.y < newLines[i].v2.y ? newLines[i].v2.y : newLines[i].v1.y);
        newLines[i].bbox[BOXBOTTOM] = (newLines[i].v1.y < newLines[i].v2.y ? newLines[i].v1.y : newLines[i].v2.y);

        newLines[i].lineno = i;

    }


    // allocate new lump info
    lump_t *newLine = malloc(sizeof (lump_t));
    // copy name
    memcpy(newLine->lump.name, lines->lump.name, sizeof (lines->lump.name));
    // change data and size information
    newLine->data = newLines;
    newLine->lump.size = lineCount * sizeof(line_t);
    // replace lum
    replaceLump(wadfile, newLine, lineLumpNum);

    return true;
}

bool processSegs(wadfile_t *wadfile, uint32_t lumpNum)
{
    uint32_t segsLumpNum = lumpNum+ML_SEGS;

    lump_t *segs;

    segs = getLumpByNum(wadfile, segsLumpNum);

    if (NULL == segs)
        return false;

    if(segs->lump.size == 0)
        return false;

    uint32_t segCount = segs->lump.size / sizeof(mapseg_t);

    seg_t* newSegs = malloc(sizeof(seg_t) * segCount);

    mapseg_t* oldSegs = (mapseg_t*) segs->data;

    //We need vertexes for this...

    uint32_t vtxLumpNum = lumpNum+ML_VERTEXES;

    lump_t *vxl;

    vxl = getLumpByNum(wadfile, vtxLumpNum);

    if (NULL == vxl)
        return false;

    if(vxl->lump.size == 0)
        return false;

    vertex_t* vtx = (vertex_t*)vxl->data;

    //And LineDefs. Must process lines first.

    uint32_t linesLumpNum = lumpNum+ML_LINEDEFS;

    lump_t * lxl;

    lxl = getLumpByNum(wadfile, linesLumpNum);

    if (NULL == lxl)
        return false;

    if(lxl->lump.size == 0)
        return false;


    line_t* lines = (line_t *) lxl->data;

    //And sides too...

    uint32_t sidesLumpNum = lumpNum+ML_SIDEDEFS;

    lump_t * sxl;

    sxl = getLumpByNum(wadfile, sidesLumpNum);

    if (NULL == sxl)
        return false;

    if(sxl->lump.size == 0)
        return false;

    mapsidedef_t* sides = (mapsidedef_t*) sxl->data;


    //****************************

    for(unsigned int i = 0; i < segCount; i++)
    {
        newSegs[i].v1.x = vtx[oldSegs[i].v1].x;
        newSegs[i].v1.y = vtx[oldSegs[i].v1].y;

        newSegs[i].v2.x = vtx[oldSegs[i].v2].x;
        newSegs[i].v2.y = vtx[oldSegs[i].v2].y;

        newSegs[i].angle = oldSegs[i].angle << 16;
        newSegs[i].offset = oldSegs[i].offset << 16;

        newSegs[i].linenum = oldSegs[i].linedef;

        const line_t* ldef = &lines[newSegs[i].linenum];

        int side = oldSegs[i].side;

        newSegs[i].sidenum = ldef->sidenum[side];

        if(newSegs[i].sidenum != NO_INDEX)
        {
            newSegs[i].frontsectornum = sides[newSegs[i].sidenum].sector;
        }
        else
        {
            newSegs[i].frontsectornum = NO_INDEX;
        }

        newSegs[i].backsectornum = NO_INDEX;

        if(ldef->flags & ML_TWOSIDED)
        {
            if(ldef->sidenum[side^1] != NO_INDEX)
            {
                newSegs[i].backsectornum = sides[ldef->sidenum[side^1]].sector;
            }
        }
    }


     // allocate new lump info
    lump_t *newSeg = malloc(sizeof (lump_t));
    // copy name
    memcpy(newSeg->lump.name, segs->lump.name, sizeof (segs->lump.name));
    // change data and size information
    newSeg->data = newSegs;
    newSeg->lump.size = segCount * sizeof(seg_t);
    // replace lum
    replaceLump(wadfile, newSeg, segsLumpNum);

    return true;
}

bool processSides(wadfile_t *wadfile, uint32_t lumpNum)
{
    uint32_t sidesLumpNum = lumpNum+ML_SIDEDEFS;

    lump_t *sides;

    sides = getLumpByNum(wadfile, sidesLumpNum);

    if (NULL == sides)
        return false;

    if(sides->lump.size == 0)
        return false;

    uint32_t sideCount = sides->lump.size / sizeof(mapsidedef_t);

    sidedef_t* newSides = malloc(sizeof(sidedef_t) * sideCount);

    mapsidedef_t* oldSides = (mapsidedef_t*) sides->data;

    for(unsigned int i = 0; i < sideCount; i++)
    {
        newSides[i].textureoffset = oldSides[i].textureoffset;
        newSides[i].rowoffset = oldSides[i].rowoffset;

        newSides[i].toptexture = getTextureNumForName(wadfile, oldSides[i].toptexture);
        newSides[i].bottomtexture = getTextureNumForName(wadfile, oldSides[i].bottomtexture);
        newSides[i].midtexture = getTextureNumForName(wadfile, oldSides[i].midtexture);

        newSides[i].sector = oldSides[i].sector;
    }

       // allocate new lump info
    lump_t *newSide = malloc(sizeof (lump_t));
    // copy name
    memcpy(newSide->lump.name, sides->lump.name, sizeof (sides->lump.name));
    // change data and size information
    newSide->data = newSides;
    newSide->lump.size = sideCount * sizeof(sidedef_t);
    // replace lum
    replaceLump(wadfile, newSide, sidesLumpNum);

    return true;
}

int getTextureNumForName(wadfile_t *wadfile, const char* tex_name)
{

    const int  *maptex1, *maptex2;
    int  numtextures1, numtextures2 = 0;
    const int *directory1, *directory2;


    //Convert name to uppercase for comparison.
    char tex_name_upper[9];

    strncpy(tex_name_upper, tex_name, 8);
    tex_name_upper[8] = 0; //Ensure null terminated.

    for (int i = 0; i < 8; i++)
    {
        tex_name_upper[i] = toupper(tex_name_upper[i]);
    }

    lump_t * tex1lump;
    int tex1LumpNum = getLumpNumByName(wadfile, "TEXTURE1");
    tex1lump = getLumpByNum(wadfile, tex1LumpNum);

    maptex1 = (const int*)tex1lump->data;
    numtextures1 = *maptex1;
    directory1 = maptex1+1;

    lump_t * tex2lump;
    int tex2LumpNum = getLumpNumByName(wadfile, "TEXTURE2");

    if (tex2LumpNum != -1)
    {
        tex2lump = getLumpByNum(wadfile, tex2LumpNum);

        maptex2 = (const int*)tex2lump->data;
        directory2 = maptex2+1;
        numtextures2 = *maptex2;
    }
    else
    {
        maptex2 = NULL;
        directory2 = NULL;
    }

    const int *directory = directory1;
    const int *maptex = maptex1;

    int numtextures = (numtextures1 + numtextures2);

    for (int i=0 ; i<numtextures; i++, directory++)
    {
        if (i == numtextures1)
        {
            // Start looking in second texture file.
            maptex = maptex2;
            directory = directory2;
        }

        int offset = *directory;

        const maptexture_t* mtexture = (const maptexture_t *) ( (const uint8_t *)maptex + offset);

        if(!strncmp(tex_name_upper, mtexture->name, 8))
        {
            return i;
        }
    }

    return 0;
}

bool processPNames(wadfile_t *wadfile)
{
    lump_t * pnamesLump;
    int32_t lumpNum = getLumpNumByName(wadfile, "PNAMES");

    if(lumpNum == -1)
        return false;

    pnamesLump = getLumpByNum(wadfile, lumpNum);

    char* pnamesData = (char*)pnamesLump->data;

    uint32_t count = *((uint32_t*)pnamesData);

    pnamesData += 4; //Fist 4 bytes are count.
    // uppercase everything
    for(uint32_t i = 0; i < count; i++)
    {
        bool nullChar = false;
        for (int j = 0; j < 8; j++)
        {
            uint8_t c;
            if (nullChar)
                c = 0;
            else
                c = toupper(pnamesData[i*8 + j]);
            pnamesData[i*8 + j] = c;
            if (0 == c)
            {
                nullChar = true;
            }

        }
    }
    return true;
}
static volatile bool startsWith(char *string, char *prefix)
{
    return strncmp(string, prefix, strlen(prefix)) == 0;
}
bool removeUnusedLumps(wadfile_t *wadfile, bool removeSound)
{
    for(uint32_t i = 0; i < wadfile->header.numlumps; i++)
    {
        lump_t *l;

        l = getLumpByNum(wadfile, i);
        char *name = l->lump.name;
        if (startsWith(name, "D_") ||
           startsWith(name, "DP") ||
           (startsWith(name, "DS") && removeSound && !startsWith(name, "DSPISTOL"))||
           startsWith(name, "GENMIDI"))
        {
            printf("removing lump %s\r\n", l->lump.name);
            removeLump(wadfile, i);
            i--;
        }
    }
    return true;
}

