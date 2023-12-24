#include "textentrywidget.h"

#include <QMimeData>
#include <filesystem>
#include <array>

bool TextEntryWidget::shouldAcceptUrl(const QString& url) {
    for (auto& ext : acceptedExtensions) {
        if (url.endsWith(ext, Qt::CaseSensitivity::CaseInsensitive))
            return true;
    }
    return false;
}

void TextEntryWidget::dragEnterEvent(QDragEnterEvent *evt)
{
    if (evt->mimeData()->hasUrls()) {
        for (auto& url : evt->mimeData()->urls()) {
            if (shouldAcceptUrl(url.toLocalFile())) {
                evt->acceptProposedAction();
                return;
            }
        }
    }
}

void TextEntryWidget::dragMoveEvent(QDragMoveEvent *evt) {
    evt->acceptProposedAction();
}

void TextEntryWidget::dragLeaveEvent(QDragLeaveEvent *evt) {
    evt->accept();
}

void TextEntryWidget::dropEvent(QDropEvent *evt)
{
    if (evt->mimeData()->urls().size())
        emit urlAdded(evt->mimeData()->urls().first());
    evt->acceptProposedAction();
}
