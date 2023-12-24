#include "pathlistwidget.h"

#include <QMimeData>
#include <filesystem>
#include <array>

namespace {
    bool ShouldAcceptUrl(const QString& url) {
        if (std::filesystem::is_directory(url.toStdString()))
            return true;
        constexpr std::array acceptedExtensions {
            ".mat", ".ba2", ".txt", ".nif", ".esp", ".esm", ".esl"
        };
        for (auto& ext : acceptedExtensions) {
            if (url.endsWith(ext, Qt::CaseSensitivity::CaseInsensitive))
                return true;
        }
        return false;
    }
}

void PathListWidget::dragEnterEvent(QDragEnterEvent *evt)
{
    if (evt->mimeData()->hasUrls()) {
        for (auto& url : evt->mimeData()->urls()) {
            if (ShouldAcceptUrl(url.toLocalFile())) {
                evt->acceptProposedAction();
                return;
            }
        }
    }
}

void PathListWidget::dragMoveEvent(QDragMoveEvent *evt) {
    evt->acceptProposedAction();
}

void PathListWidget::dragLeaveEvent(QDragLeaveEvent *evt) {
    evt->accept();
}

void PathListWidget::dropEvent(QDropEvent *evt)
{
    emit urlsAdded(evt->mimeData()->urls());
    evt->acceptProposedAction();
}
