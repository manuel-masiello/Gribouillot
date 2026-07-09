#include "commands.h"
#include "gribouillotlayer.h"

AddItemCommand::AddItemCommand(GribouillotLayer *layer, QGraphicsItem *item, QUndoCommand *parent)
    : QUndoCommand(parent), layer(layer), item(item)
{
    setText(QObject::tr("Add item"));
}

AddItemCommand::~AddItemCommand()
{
    if (!attached)
        delete item;
}

void AddItemCommand::undo()
{
    layer->detachItem(item);
    attached = false;
}

void AddItemCommand::redo()
{
    if (firstRedo)
    {
        firstRedo = false;
        return;
    }

    layer->attachItem(item);
    attached = true;
}

DeleteItemsCommand::DeleteItemsCommand(GribouillotLayer *layer, const QList<QGraphicsItem *> &items, QUndoCommand *parent)
    : QUndoCommand(parent), layer(layer), items(items)
{
    setText(QObject::tr("Delete %1 item(s)").arg(items.count()));

    // 'items' must be in itemsList order (as returned by GribouillotLayer::selectedItems())
    // so that undo() can re-insert them at their original indices in ascending order.
    for (QGraphicsItem *item : items)
        indices.append(layer->indexOfItem(item));
}

DeleteItemsCommand::~DeleteItemsCommand()
{
    if (detached)
        qDeleteAll(items);
}

void DeleteItemsCommand::undo()
{
    for (int i = 0; i < items.size(); ++i)
        layer->attachItem(items[i], indices[i]);
    detached = false;
}

void DeleteItemsCommand::redo()
{
    for (QGraphicsItem *item : items)
        layer->detachItem(item);
    detached = true;
}
