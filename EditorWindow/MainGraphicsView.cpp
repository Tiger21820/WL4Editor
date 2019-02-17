#include "MainGraphicsView.h"
#include "Operation.h"
#include "LevelComponents/Tile.h"
#include "LevelComponents/Door.h"

#include <QScrollBar>
#include <QMouseEvent>
#include <QMessageBox>

#include <iostream>
#include <cassert>

extern WL4EditorWindow *singleton;

/// <summary>
/// this function will be called when the graphic view in the main window is clicked.
/// </summary>
/// <param name="event">
/// The mouse click event.
/// </param>
void MainGraphicsView::mousePressEvent(QMouseEvent *event)
{
    if(!singleton->FirstROMIsLoaded()) return;

    // Get the ID of the tile that was clicked
    unsigned int X = (unsigned int) event->x() + horizontalScrollBar()->sliderPosition();
    unsigned int Y = (unsigned int) event->y() + verticalScrollBar()->sliderPosition();
    unsigned int tileX = X / 32;
    unsigned int tileY = Y / 32;

    // Different cases for different editting mode
    LevelComponents::Room *room = singleton->GetCurrentRoom();
    if(tileX < room->GetWidth() && tileY < room->GetHeight())
    {
        enum Ui::EditMode editMode = singleton->GetEditModeWidgetPtr()->GetEditModeParams().editMode;

        if(editMode == Ui::LayerEditMode)
        {
            //If we use right click then copy the tile
            if (event->button() == Qt::RightButton) {
                int selectedLayer = singleton->GetEditModeWidgetPtr()->GetEditModeParams().selectedLayer;
                LevelComponents::Layer *layer = room->GetLayer(selectedLayer);
                int selectedTileIndex = tileX + tileY * room->GetWidth();
                unsigned short placedTile=layer->GetLayerData()[selectedTileIndex];
                singleton->GetTile16DockWidgetPtr()->SetSelectedTile(placedTile, true);

            //Otherwise just place the tile
            } else {
                // Change textmaps and layer graphics
                SetTile(tileX, tileY);
            }
        }
        else if(editMode == Ui::DoorEditMode) // select a door
        {
            unsigned int doorCount = room->CountDoors();
            if(doorCount)
            {
                // Select a Door if possible
                for(unsigned int i = 0; i < doorCount; i++)
                {
                    LevelComponents::Door *door = room->GetDoor(i);
                    bool b1 = door->GetX1() <= (int) tileX;
                    bool b2 = door->GetX2() >= (int) tileX;
                    bool b3 = door->GetY1() <= (int) tileY;
                    bool b4 = door->GetY2() >= (int) tileY;
                    if(b1 && b2 && b3 && b4)
                    {
                        // Door "i" was selected
                        if((int) i == SelectedDoorID)
                        {
                            // If the door that was clicked is already selected, open the door config dialog
                            DoorConfigDialog _doorconfigdialog(singleton, room, i, singleton->GetCurrentLevel());
                            if(_doorconfigdialog.exec() == QDialog::Accepted)
                            {
                                // Apply changes from the door config dialog
                                _doorconfigdialog.UpdateCurrentDoorData();
                                singleton->ResetEntitySetDockWidget();
                                singleton->SetUnsavedChanges(true);
                            }
                        }
                        else
                        {
                            // If the door that was clicked was not already selected, then select it
                            SelectedDoorID = i;
                            // Let the Entityset change with the last selected Door
                            singleton->GetCurrentRoom()->SetCurrentEntitySet(singleton->GetCurrentRoom()->GetDoor(i)->GetEntitySetID());
                            singleton->ResetEntitySetDockWidget();
                        }
                        goto DOOR_FOUND;
                    }
                }
                SelectedDoorID = -1;
DOOR_FOUND:     ;
            }
            singleton->RenderScreenElementsLayersUpdate((unsigned int) SelectedDoorID, -1);
        }
        else if(editMode == Ui::EntityEditMode) // select or add an Entity
        {
            SelectedEntityID = room->FindEntity(tileX, tileY);
            if(SelectedEntityID == -1)
            {
                // Add the new entity
                bool success = room->AddEntity(tileX, tileY, singleton->GetEntitySetDockWidgetPtr()->GetCurrentEntityLocalId());
                assert(success /* Failure to add entity */); // TODO: Show information if failure
                int difficulty = singleton->GetEditModeWidgetPtr()->GetEditModeParams().seleteddifficulty;
                room->SetEntityListDirty(difficulty, true);
                singleton->SetUnsavedChanges(true);
            }
            singleton->RenderScreenElementsLayersUpdate((unsigned int) -1, SelectedEntityID);
        }
    }
}

/// <summary>
/// this function will be called when the graphic view in the main window is clicked and then mouse start moving.
/// </summary>
/// <param name="event">
/// The mouse click event.
/// </param>
void MainGraphicsView::mouseMoveEvent(QMouseEvent *event)
{
    if(!singleton->FirstROMIsLoaded()) return;

    // Get the ID of the tile that was clicked
    unsigned int X = (unsigned int) event->x() + horizontalScrollBar()->sliderPosition();
    unsigned int Y = (unsigned int) event->y() + verticalScrollBar()->sliderPosition();
    unsigned int tileX = X / 32;
    unsigned int tileY = Y / 32;

    // If we have moved within the same tile, do nothing
    if(tileX == (unsigned int) drawingTileX && tileY == (unsigned int) drawingTileY)
    {
        return;
    }

    // Draw the new tile
    LevelComponents::Room *room = singleton->GetCurrentRoom();
    if(tileX < room->GetWidth() && tileY < room->GetHeight())
    {
        enum Ui::EditMode editMode = singleton->GetEditModeWidgetPtr()->GetEditModeParams().editMode;

        if((editMode == Ui::LayerEditMode))
        {
            //If we hold right click then copy the tile
            // event->button() cannot work, event->buttons() return the correct mouseState according to the Qt code
            if (event->buttons() == Qt::RightButton) {
                int selectedLayer = singleton->GetEditModeWidgetPtr()->GetEditModeParams().selectedLayer;
                LevelComponents::Layer *layer = room->GetLayer(selectedLayer);
                int selectedTileIndex = tileX + tileY * room->GetWidth();
                unsigned short placedTile=layer->GetLayerData()[selectedTileIndex];
                singleton->GetTile16DockWidgetPtr()->SetSelectedTile(placedTile, true);

            //Otherwise just place the tile
            } else {
                // Change textmaps and layer graphics
                SetTile(tileX, tileY);
            }
        }
    }
    else
    {
        mouseReleaseEvent(event);
    }
}

/// <summary>
/// This is a helper function for setting the tile at a tile position to the currently
/// selected tile from the UI.
/// </summary>
/// <param name="tileX">
/// The X position of the tile (unit: map16)
/// </param>
/// <param name="tileY">
/// The Y position of the tile (unit: map16)
/// </param>
void MainGraphicsView::SetTile(int tileX, int tileY)
{
    // Update which tile has last been drawn, for the tile painting functionality
    drawingTileX = tileX;
    drawingTileY = tileY;

    // Create an execute a tile change operation for the changed tile
    LevelComponents::Room *room = singleton->GetCurrentRoom();
    unsigned short selectedTile = singleton->GetTile16DockWidgetPtr()->GetSelectedTile();
    if(selectedTile == 0xFFFF) return;
    int selectedLayer = singleton->GetEditModeWidgetPtr()->GetEditModeParams().selectedLayer;
    LevelComponents::Layer *layer = room->GetLayer(selectedLayer);
    if(layer->IsEnabled() == false) return;
    if(layer->GetMappingType() == LevelComponents::LayerTile8x8) return; //temporarity skip then condition when the current Layer's MappingType is 0x20 to avoid wrong rendering
    int selectedTileIndex = tileX + tileY * room->GetWidth();
    if(layer->GetLayerData()[selectedTileIndex] == selectedTile) return;
    struct OperationParams *params = new struct OperationParams();
    params->type = ChangeTileOperation;
    params->tileChange = true;
    params->tileChangeParams.push_back(TileChangeParams::Create(
        tileX,
        tileY,
        selectedLayer,
        selectedTile,
        layer->GetLayerData()[selectedTileIndex]
    ));
    ExecuteOperation(params);
}

/// <summary>
/// this function will be called when the graphic view in the main window is clicked then mouse release.
/// </summary>
/// <param name="event">
/// The mouse click event.
/// </param>
void MainGraphicsView::mouseReleaseEvent(QMouseEvent *event)
{
    (void) event;
    drawingTileX = -1;
    drawingTileY = -1;
}

/// <summary>
/// this function will be called when key-press happens.
/// </summary>
/// <param name="event">
/// The key-press event.
/// </param>
void MainGraphicsView::keyPressEvent(QKeyEvent *event)
{
    //If an entity is selected
    if(SelectedEntityID != -1)
    {
        switch (event->key())
        {
            // Delete selected entity if BSP or DEL is pressed
            case Qt::Key_Backspace:
            case Qt::Key_Delete:
            {
                singleton->DeleteEntity(SelectedEntityID);
                SelectedEntityID = -1;
                singleton->RenderScreenElementsLayersUpdate((unsigned int) -1, -1);
                int difficulty = singleton->GetEditModeWidgetPtr()->GetEditModeParams().seleteddifficulty;
                singleton->GetCurrentRoom()->SetEntityListDirty(difficulty, true);
                singleton->SetUnsavedChanges(true);
                break;
            }
                //Move selected entity when a direction key is pressed
            case Qt::Key_Right:
            case Qt::Key_Left:
            case Qt::Key_Up:
            case Qt::Key_Down:
            {
                LevelComponents::Room* currentRoom=singleton->GetCurrentRoom();

                //The new positions
                int px=currentRoom->GetEntityX(SelectedEntityID);
                int py=currentRoom->GetEntityY(SelectedEntityID);
                if (event->key() == Qt::Key_Left)
                {
                    px = px - 1;
                } else if (event->key() == Qt::Key_Right)
                {
                    px = px + 1;
                } else if (event->key() == Qt::Key_Up)
                {
                    py = py - 1;
                } else if (event->key() == Qt::Key_Down)
                {
                    py = py + 1;
                }

                if (currentRoom->IsNewEntityPositionInsideRoom(px, py)) {
                    currentRoom->SetEntityPosition(px,py,SelectedEntityID);
                    singleton->RenderScreenElementsLayersUpdate((unsigned int) -1, SelectedEntityID);
                    int difficulty = singleton->GetEditModeWidgetPtr()->GetEditModeParams(). seleteddifficulty;
                    singleton->GetCurrentRoom()->SetEntityListDirty(difficulty, true);
                    singleton->SetUnsavedChanges(true);
                }
                break;
            }
        }
    //If a door is selected
    } else if(SelectedDoorID != -1)
    {
        switch (event->key())
        {
        // Delete selected door if BSP or DEL is pressed
            case Qt::Key_Backspace:
            case Qt::Key_Delete:
            {
                if(singleton->GetCurrentRoom()->CountDoors() < 2)
                {
                    QMessageBox::critical(nullptr, QString("Error"), QString("Deleting the last Door in a Room is not allowed!"));
                    break;
                }
                singleton->DeleteDoor(singleton->GetCurrentRoom()->GetDoor(SelectedDoorID)->GetGlobalDoorID());
                SelectedDoorID = -1;
                singleton->RenderScreenElementsLayersUpdate((unsigned int) -1, -1);
                singleton->SetUnsavedChanges(true);
                break;
            }
            //Move selected door when a direction key is pressed
            case Qt::Key_Right:
            case Qt::Key_Left:
            case Qt::Key_Up:
            case Qt::Key_Down:
            {
                LevelComponents::Room *currentRoom = singleton->GetCurrentRoom();
                LevelComponents::Door *selectedDoor = currentRoom->GetDoor(SelectedDoorID);

                //The new positions
                int px1 = selectedDoor->GetX1();
                int py1 = selectedDoor->GetY1();
                int px2 = selectedDoor->GetX2();
                int py2 = selectedDoor->GetY2();
                if (event->key() == Qt::Key_Left)
                {
                    px1 = px1 - 1;
                    px2 = px2 - 1;
                } else if (event->key() == Qt::Key_Right)
                {
                    px1 = px1 + 1;
                    px2 = px2 + 1;
                } else if (event->key() == Qt::Key_Up)
                {
                    py1 = py1 - 1;
                    py2 = py2 - 1;
                } else if (event->key() == Qt::Key_Down)
                {
                    py1 = py1 + 1;
                    py2 = py2 + 1;

                }
                if (currentRoom->IsNewDoorPositionInsideRoom(px1, px2, py1, py2)) {
                   selectedDoor->SetDoorPlace(px1, px2, py1, py2);
                   singleton->RenderScreenElementsLayersUpdate((unsigned int) SelectedDoorID, -1);
                }
            break;
            }
        }
    }
}

/// <summary>
/// This function will deselect doors and entities.
/// </summary>
void MainGraphicsView::DeselectDoorAndEntity()
{
    SelectedDoorID = -1;
    SelectedEntityID = -1;
    singleton->RenderScreenElementsLayersUpdate((unsigned int) -1, -1);
}
