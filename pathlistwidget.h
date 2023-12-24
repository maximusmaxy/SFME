#ifndef PATHLISTWIDGET_H
#define PATHLISTWIDGET_H

#include <QListWidget>
#include <QDragEnterEvent>
#include <QDragMoveEvent>
#include <QDragLeaveEvent>
#include <QDropEvent>

class PathListWidget : public QListWidget
{
     Q_OBJECT

public:
    PathListWidget(QWidget *parent = nullptr) : QListWidget(parent) {}

    void dragEnterEvent(QDragEnterEvent *evt);
    void dragMoveEvent(QDragMoveEvent *evt);
    void dragLeaveEvent(QDragLeaveEvent *evt);
    void dropEvent(QDropEvent *evt);

signals:
    void urlsAdded(const QList<QUrl>& urls);

public:
    bool isEmpty = false;
};

#endif // PATHLISTWIDGET_H
