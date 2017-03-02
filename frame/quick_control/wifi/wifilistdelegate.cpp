#include "wifilistdelegate.h"
#include "wifilistmodel.h"

#include <QPainter>
#include <QDebug>
#include <QJsonObject>
#include <QDateTime>

WifiListDelegate::WifiListDelegate(QObject *parent)
    : QAbstractItemDelegate(parent),

      m_securityPixmap(QPixmap(":/frame/quick_control/wifi/wireless/security.svg"))
{

}

void WifiListDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    painter->setRenderHints(QPainter::TextAntialiasing | QPainter::SmoothPixmapTransform);
    painter->setPen(Qt::white);
    painter->setBrush(Qt::red);

    const bool isHeader = index.data(WifiListModel::ItemIsHeaderRole).toBool();
    const bool isHovered = index.data(WifiListModel::ItemHoveredRole).toBool();
    const bool isTips = index.data(WifiListModel::ItemIsHiddenTipsRole).toBool();

    if (isHovered && !isHeader)
        painter->fillRect(option.rect, QColor(255, 255, 255, 0.1 * 255));

    if (isHeader)
        painter->fillRect(option.rect, QColor(255, 255, 255, 0.15 * 255));

    QFont f(painter->font());
    f.setBold(isHeader);
    painter->setFont(f);
    if (isTips)
        painter->drawText(option.rect.marginsRemoved(QMargins(70, 0, 0, 0)), Qt::AlignVCenter | Qt::AlignLeft, index.data(WifiListModel::ItemHiddenTipsRole).toString());
    else if (isHeader)
        painter->drawText(option.rect.marginsRemoved(QMargins(24, 0, 0, 0)), Qt::AlignVCenter | Qt::AlignLeft, index.data(Qt::DisplayRole).toString());
    else
        painter->drawText(option.rect.marginsRemoved(QMargins(70, 0, 0, 0)), Qt::AlignVCenter | Qt::AlignLeft, index.data(Qt::DisplayRole).toString());

    if (!isHeader && !isTips)
    {
        const QJsonObject info = index.data(WifiListModel::ItemInfoRole).value<QJsonObject>();
        const bool isSecured = info.value("Secured").toBool();
        const int strength = info.value("Strength").toInt();

        // draw signal icon
        const int iconIndex = (strength / 10) & ~0x1;
        const int x = 40;
        const int y = option.rect.top() + (option.rect.height() - 16) / 2;
        painter->drawPixmap(x, y, QPixmap(QString(":/frame/quick_control/wifi/wireless/wireless-%1-symbolic.svg").arg(iconIndex)));

        // draw secured icon
        if (isSecured)
        {
            const int x = 20;

            painter->drawPixmap(x, y, m_securityPixmap);
        }

        const int icon_x = option.rect.right() - 16 - 10;
        const bool isActivating = index.data(WifiListModel::ItemIsActivatingRole).toBool();
        if (isActivating)
        {
            const quint64 index = QDateTime::currentMSecsSinceEpoch() / 20;
            const QString pix = QString(":/frame/themes/dark/icons/dark_loading/loading_0%1.png").arg((index % 90), 2, 10, QChar('0'));
            painter->drawPixmap(icon_x, y + 1, QPixmap(pix));
        } else {
            // draw actived icon
            const bool isActived = index.data(WifiListModel::ItemIsActiveRole).toBool();
            if (isActived)
            {
                if (isHovered)
                    painter->drawPixmap(icon_x, y, QPixmap(":/frame/themes/dark/icons/disconnect.png"));
                else
                    painter->drawPixmap(icon_x, y, QPixmap(":/frame/themes/dark/icons/select.png"));
            }
        }
        painter->fillRect(option.rect, QColor(255, 255, 255, 0.05 * 255));

        if (!isHovered) {
            QPen pen(QColor(255, 255, 255, 0.1 * 255));
            QLineF line;
            line.setP1(QPoint(option.rect.bottomLeft().x() + 10, option.rect.bottomLeft().y()));
            line.setP2(QPoint(option.rect.bottomRight().x() - 10, option.rect.bottomRight().y()));
            painter->setPen(pen);
            painter->drawLine(line);
        }
    }
}

QSize WifiListDelegate::sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    Q_UNUSED(option);

    return index.data(Qt::SizeHintRole).toSize();
}
