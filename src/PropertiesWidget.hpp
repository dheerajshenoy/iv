// Exif Properties Widget

#pragma once

#include <QGridLayout>
#include <QGroupBox>
#include <QHeaderView>
#include <QLabel>
#include <QMap>
#include <QScrollBar>
#include <QString>
#include <QTreeWidget>
#include <QVBoxLayout>
#include <QVariant>
#include <QWidget>

class PropertiesWidget : public QWidget
{
    Q_OBJECT
public:
    explicit PropertiesWidget(QWidget *parent = nullptr) : QWidget(parent)
    {
        QVBoxLayout *layout = new QVBoxLayout(this);
        m_treeWidget        = new QTreeWidget(this);
        m_treeWidget->setColumnCount(2);
        QStringList headers;
        headers << "Property"
                << "Value";
        m_treeWidget->setHeaderLabels(headers);
        m_treeWidget->header()->setSectionResizeMode(QHeaderView::Stretch);
        layout->addWidget(m_treeWidget);
        setLayout(layout);
        // Make a dialog-like window
        setWindowFlags(windowFlags() | Qt::Dialog);
    }

    void setProperties(const QMap<QString, QString> &properties)
    {
        m_treeWidget->clear();
        for (auto it = properties.constBegin(); it != properties.constEnd(); ++it)
        {
            QTreeWidgetItem *item = new QTreeWidgetItem(m_treeWidget);
            item->setText(0, it.key());
            item->setText(1, it.value());
            m_treeWidget->addTopLevelItem(item);
        }
    }

    void clearProperties() const noexcept
    {
        m_treeWidget->clear();
    }

private:
    QTreeWidget *m_treeWidget;
};
