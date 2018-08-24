#ifndef ENTITY_H
#define ENTITY_H

#include <QVector>

#include "LevelComponents/EntitySet.h"

namespace LevelComponents
{
    struct EntityTile
    {
        int deltaX;
        int deltaY;
        Tile8x8 *objTile;

        // Deconstructor for the struct
        ~EntityTile()
        {
            delete objTile;
        }
    };

    class Entity
    {
    private:
        int EntityID = 0;
        int EntityGlobalID = 0;
        int OAMDataTablePtr = 0;
        int EntityDeltaX = 0, EntityDeltaY = 0;
        int Priority;
        int PaletteOffset;
        int PaletteOffsetChange = 0;
        bool SemiTransparent = false;
        QVector<EntityTile*> entityTiles;
        EntitySet *currentEntityset;
        ~Entity();

    public:
        Entity(int entityID, int entityGlabalId, EntitySet *_currentEntityset);

    private:
        void OAMtoTiles(unsigned short *singleOAM);
        void ExtractSpritesTiles(int spritesFrameDataPtr, int frame);
    };
}

#endif // ENTITY_H
