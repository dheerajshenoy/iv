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
        setAttribute(Qt::WA_DeleteOnClose);
        setWindowFlags(Qt::Dialog | Qt::Window);
        QVBoxLayout *layout = new QVBoxLayout(this);

        m_treeWidget = new QTreeWidget(this);
        m_treeWidget->setColumnCount(2);
        m_treeWidget->setHeaderLabels({"Property", "Value"});
        m_treeWidget->header()->setSectionResizeMode(QHeaderView::Stretch);

        layout->addWidget(m_treeWidget);
        setLayout(layout);
    }

    void setProperties(const QMap<QString, QString> &properties)
    {
        m_treeWidget->clear();

        // --- Properties Section ---
        QTreeWidgetItem *propRoot = new QTreeWidgetItem(m_treeWidget);
        propRoot->setText(0, "Properties");
        propRoot->setExpanded(true);

        for (auto it = properties.constBegin(); it != properties.constEnd(); ++it)
        {
            QTreeWidgetItem *item = new QTreeWidgetItem(propRoot);
            item->setText(0, it.key());
            item->setText(1, it.value());
        }

#ifndef HAS_LIBEXIV2
        QTreeWidgetItem *exifRoot = new QTreeWidgetItem(m_treeWidget);
        exifRoot->setText(0, "EXIF");
        exifRoot->setExpanded(true);

        QTreeWidgetItem *msgItem = new QTreeWidgetItem(exifRoot);
        msgItem->setText(0, "EXIF support disabled");
        msgItem->setText(1, "");
        return;
#endif

        // If EXIF is supported but not yet provided, create empty EXIF root
        if (!m_exif_set)
        {
            m_exifRoot = new QTreeWidgetItem(m_treeWidget);
            m_exifRoot->setText(0, "EXIF");
            m_exifRoot->setExpanded(true);
        }
    }

#ifdef HAS_LIBEXIV2
    void setEXIF(const QMap<QString, QString> &exifData)
    {
        // Ensure EXIF root exists
        if (!m_exifRoot)
        {
            m_exifRoot = new QTreeWidgetItem(m_treeWidget);
            m_exifRoot->setText(0, "EXIF");
            m_exifRoot->setExpanded(true);
        }

        // Clear previous EXIF and repopulate
        m_exifRoot->takeChildren();

        for (auto it = exifData.constBegin(); it != exifData.constEnd(); ++it)
        {
            QTreeWidgetItem *item = new QTreeWidgetItem(m_exifRoot);
            item->setText(0, it.key());
            item->setText(1, it.value());
        }

        m_exif_set = true;
    }
#endif

    void clearProperties()
    {
        m_treeWidget->clear();
        m_exif_set = false;
        m_exifRoot = nullptr;
    }

private:
    QTreeWidget *m_treeWidget = nullptr;

    QTreeWidgetItem *m_exifRoot = nullptr;
    bool m_exif_set{false};
};
