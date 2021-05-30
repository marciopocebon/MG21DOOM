/*
   MG21Wadutil by Nicola Wrachien.

   Based on doomhack's GBAWadutil
   Original source: https://github.com/doomhack/GbaWadUtil

   This command line utility allows to convert your wad file
   to a format convenient for the Ikea Tradfri / MG21 Doom port.

   Usage:
   mg21wadutil <inputwad> <outputwad>
*/
#ifndef WADFILE_H
#define WADFILE_H
#include "doomtypes.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

typedef struct
{
    filelump_t lump;
    void *data;
} lump_t;
typedef struct
{
    wadinfo_t header;
    lump_t *lumps;
} wadfile_t;
//bool loadWad(const char *fileName, uint32_t *lumpFound, lump_t **lumps, wadinfo_t *header);
bool loadWad(const char *fileName, wadfile_t *wadfile);
bool saveWad(const char *fileName, wadfile_t *wadfile, char wadType);
void mergeWadFile(wadfile_t *wad1, wadfile_t *wad2);
void removeLump(wadfile_t * wadfile, int index);
void insertLump(wadfile_t *wadfile, lump_t *newLump, int index);
int getLumpNumByName(wadfile_t *wadfile, const char *name);
lump_t * getLumpByNum(wadfile_t *wadfile, int lumpNum);
void replaceLump(wadfile_t *wadfile, lump_t *newLump, int index);

#if 0
class Lump
{
public:
    QString name;
    quint32 length;
    QByteArray data;
};

class WadFile : public QObject
{
    Q_OBJECT
public:
    explicit WadFile(QString filePath, QObject *parent = nullptr);

    bool LoadWadFile();

    bool SaveWadFile(QString filePath);
    bool SaveWadFile(QIODevice* device);

    qint32 GetLumpByName(QString name, Lump& lump);
    bool GetLumpByNum(quint32 lumpnum, Lump& lump);

    bool ReplaceLump(quint32 lumpnum, Lump newLump);
    bool InsertLump(quint32 lumpnum, Lump newLump);
    bool RemoveLump(quint32 lumpnum);

    quint32 LumpCount();

    bool MergeWadFile(WadFile& wadFile);

private:
    QString wadPath;

    QList<Lump> lumps;
};

#endif

#endif // WADFILE_H
