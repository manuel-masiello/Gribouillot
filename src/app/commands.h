#ifndef COMMANDS_H
#define COMMANDS_H

#include <QGraphicsItem>
#include <QList>
#include <QUndoCommand>

class GribouillotLayer;

class AddItemCommand : public QUndoCommand
{
public:
    AddItemCommand(GribouillotLayer *layer, QGraphicsItem *item, QUndoCommand *parent = nullptr);
    ~AddItemCommand() override;

    void undo() override;
    void redo() override;

private:
    GribouillotLayer *layer;
    QGraphicsItem *item;
    bool firstRedo = true;
    bool attached = true;
};

class DeleteItemsCommand : public QUndoCommand
{
public:
    DeleteItemsCommand(GribouillotLayer *layer, const QList<QGraphicsItem *> &items, QUndoCommand *parent = nullptr);
    ~DeleteItemsCommand() override;

    void undo() override;
    void redo() override;

private:
    GribouillotLayer *layer;
    QList<QGraphicsItem *> items;
    QList<int> indices;
    bool detached = false;
};

#endif // COMMANDS_H
