#ifndef TEXTENTRYWIDGET_H
#define TEXTENTRYWIDGET_H

#include <QLineEdit>
#include <QDragEnterEvent>
#include <QDragMoveEvent>
#include <QDragLeaveEvent>
#include <QDropEvent>

class TextEntryWidget : public QLineEdit
{
     Q_OBJECT

public:
    TextEntryWidget(QWidget *parent = nullptr) : QLineEdit(parent) {}

    void dragEnterEvent(QDragEnterEvent *evt);
    void dragMoveEvent(QDragMoveEvent *evt);
    void dragLeaveEvent(QDragLeaveEvent *evt);
    void dropEvent(QDropEvent *evt);
    bool shouldAcceptUrl(const QString& url);

signals:
    void urlAdded(const QUrl& url);

public:
    std::vector<const char*> acceptedExtensions;
};

#endif // TEXTENTRYWIDGET_H
