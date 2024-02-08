#include <QApplication>
#include <QWidget>
#include <QTableView>
#include <QVBoxLayout>
#include <QPushButton>
#include <QSortFilterProxyModel>
#include <QRegularExpression>
#include <QStandardItemModel>
#include <QLineEdit>
#include <QHeaderView>
#include <QTreeView>
#include <QMessageBox>
#include <QStyledItemDelegate>
#include <QDebug>
#include "MemoryUtils.h"


class ConditionalEditDelegate : public QStyledItemDelegate {
public:
    ConditionalEditDelegate(QObject* parent = nullptr) : QStyledItemDelegate(parent) {}

    QWidget* createEditor(QWidget* parent, const QStyleOptionViewItem& option, const QModelIndex& index) const override {
        // 如果是顶级项，则禁止编辑
        if (!index.parent().isValid()) {
            return nullptr;
        }
        // 否则，使用默认的编辑器
        return QStyledItemDelegate::createEditor(parent, option, index);
    }
};

class CustomFilterProxyModel : public QSortFilterProxyModel {
public:
    CustomFilterProxyModel(QObject* parent = nullptr) : QSortFilterProxyModel(parent) {}

protected:
    bool filterAcceptsRow(int sourceRow, const QModelIndex& sourceParent) const override {
        if (!sourceParent.isValid()) {
            // 顶级项，总是接受
            return true;
        }

        QModelIndex index = sourceModel()->index(sourceRow, filterKeyColumn(), sourceParent);
        QString itemText = sourceModel()->data(index).toString();
        // 使用QRegularExpression进行匹配
        QRegularExpression re(filterRegularExpression());
        return re.match(itemText).hasMatch();
    }
};


class MyApp : public QWidget {
    QStandardItemModel* model;
    QTreeView* treeView;
    std::vector<ItemDescription> items;
    std::vector<ItemDescription> goldMSP;

public:
    MyApp(const std::vector<ItemDescription>& items,
        const std::vector<ItemDescription>& goldMSP,
        QWidget* parent = nullptr)
        : QWidget(parent), items(items), goldMSP(goldMSP) {

        auto* layout = new QVBoxLayout(this);
        auto* filterEdit = new QLineEdit(this);

        filterEdit->setPlaceholderText("请输入想要筛选的名称");// 设置占位符文本


        treeView = new QTreeView(this);
        model = new QStandardItemModel();

        CustomFilterProxyModel* filterProxyModel = new CustomFilterProxyModel(this);
        filterProxyModel->setSourceModel(model); // 设置原始模型
        filterProxyModel->setFilterCaseSensitivity(Qt::CaseInsensitive);
        filterProxyModel->setFilterKeyColumn(0); // 在第一列上应用筛选


        // 设置列标题
        model->setHorizontalHeaderLabels(QStringList() << "物品名称" << "当前值" << "新值");
        treeView->setModel(model);
        treeView->setModel(filterProxyModel); // 使用自定义过滤代理模型

        auto* updateButton = new QPushButton("更新", this);
        auto* modifyButton = new QPushButton("修改", this);

        layout->addWidget(filterEdit);
        layout->addWidget(treeView);
        layout->addWidget(updateButton);
        layout->addWidget(modifyButton);

        auto* conditionalEditDelegate = new ConditionalEditDelegate(treeView);
        treeView->setItemDelegate(conditionalEditDelegate);

        // 填充表格初始数据

        QStandardItem* goldMSPParent = new QStandardItem("金币和技能点");
        model->appendRow(goldMSPParent);
        for (const auto& item : goldMSP) {
            QList<QStandardItem*> row;
            QStandardItem* nameItem = new QStandardItem(QString::fromStdString(item.name));
            QStandardItem* currentValueItem = new QStandardItem(); // 初始为空
            QStandardItem* newValueItem = new QStandardItem(); // 初始为空

            nameItem->setEditable(false); // 不允许编辑
            currentValueItem->setEditable(false); // 允许编辑
            newValueItem->setEditable(true); // 允许编辑

            row << nameItem << currentValueItem << newValueItem;
            goldMSPParent->appendRow(row); // 将子项添加到大标题下
        }


        QStandardItem* itemParent = new QStandardItem("所有物品");
        model->appendRow(itemParent);

        for (const auto& item : items) {
            QList<QStandardItem*> row;
            QStandardItem* nameItem = new QStandardItem(QString::fromStdString(item.name));
            QStandardItem* currentValueItem = new QStandardItem(); // 初始为空
            QStandardItem* newValueItem = new QStandardItem(); // 初始为空

            nameItem->setEditable(false); // 不允许编辑
            currentValueItem->setEditable(false); // 允许编辑
            newValueItem->setEditable(true); // 允许编辑

            row << nameItem << currentValueItem << newValueItem;
            itemParent->appendRow(row); // 将子项添加到大标题下
        }

        treeView->expandAll(); // 默认展开所有项

        // 连接更新按钮点击事件
        connect(updateButton, &QPushButton::clicked, this, &MyApp::updateCurrentValue);

        // 连接修改按钮点击事件
        connect(modifyButton, &QPushButton::clicked, this, &MyApp::modifyCurrentValue);

        // 连接过滤逻辑
        connect(filterEdit, &QLineEdit::textChanged, [filterProxyModel](const QString& text) {
            filterProxyModel->setFilterRegularExpression(QRegularExpression(text, QRegularExpression::CaseInsensitiveOption));
        });

    }

    void updateCurrentValue() {

        DWORD pID = GetProcessId(L"granblue_fantasy_relink.exe");
        DWORD_PTR baseAddress = GetModuleBaseAddress(pID, L"granblue_fantasy_relink.exe");
        HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pID);
        DWORD_PTR dynamicPtrBaseAddrItem = baseAddress + 0x05CEA628; // 物品基址偏移
        DWORD_PTR dynamicPtrBaseAddrGoldMSP = baseAddress + 0x05CEA568; // 金币和技能点基址偏移

        // 获取“金币和技能点”顶级项
        QStandardItem* goldMSPParent = model->item(0);
        if (goldMSPParent && goldMSPParent->text() == "金币和技能点") {
            // 遍历“金币和技能点”下的所有子项
            for (int i = 0; i < goldMSP.size(); ++i) {

                DWORD_PTR finalAddress = FindDmaAddy(hProcess, dynamicPtrBaseAddrGoldMSP, goldMSP[i].finalOffsets);
                int currentValue = 0;
                ReadProcessMemory(hProcess, (LPCVOID)finalAddress, &currentValue, sizeof(currentValue), nullptr);

                QStandardItem* currentValueItem = goldMSPParent->child(i, 1); // 第二列存储当前值
                if (currentValueItem) {
                    currentValueItem->setText(QString::number(currentValue));
                }
            }
        }

        // 获取“所有物品”顶级项
        QStandardItem* allItemsParent = model->item(1);

        if (allItemsParent && allItemsParent->text() == "所有物品") {
            // 遍历“所有物品”下的所有子项
            for (int i = 0; i < items.size(); ++i) {

                DWORD_PTR finalAddress = FindDmaAddy(hProcess, dynamicPtrBaseAddrItem, items[i].finalOffsets);
                int currentValue = 0;
                ReadProcessMemory(hProcess, (LPCVOID)finalAddress, &currentValue, sizeof(currentValue), nullptr);

                QStandardItem* currentValueItem = allItemsParent->child(i, 1); // 第二列存储当前值
                if (currentValueItem) {
                    currentValueItem->setText(QString::number(currentValue));
                }
            }
        }

        CloseHandle(hProcess);
    };

    void modifyCurrentValue() {

        QString updateMessages;

        DWORD pID = GetProcessId(L"granblue_fantasy_relink.exe");
        DWORD_PTR baseAddress = GetModuleBaseAddress(pID, L"granblue_fantasy_relink.exe");
        HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pID);
        DWORD_PTR dynamicPtrBaseAddrItem = baseAddress + 0x05CEA628; // 物品基址偏移
        DWORD_PTR dynamicPtrBaseAddrGoldMSP = baseAddress + 0x05CEA568; // 金币和技能点基址偏移

        // 获取“金币和技能点”顶级项
        QStandardItem* goldMSPParent = model->item(0);

        if (goldMSPParent && goldMSPParent->text() == "金币和技能点") {
            // 遍历“金币和技能点”下的所有子项
            for (int i = 0; i < goldMSP.size(); ++i) {

                DWORD_PTR finalAddress = FindDmaAddy(hProcess, dynamicPtrBaseAddrGoldMSP, goldMSP[i].finalOffsets);

                auto newValueItem = goldMSPParent->child(i, 2); // 获取“新值”列
                auto descriptionItem = goldMSPParent->child(i, 0); // 获取“描述”列

                if (!newValueItem->text().isEmpty()) {
                    int newValue = newValueItem->text().toInt();

                    // 使用 WriteProcessMemory 写入新值
                    if (WriteProcessMemory(hProcess, (LPVOID)finalAddress, &newValue, sizeof(newValue), NULL)) {
                        // 成功更新后，清空"新值"列并记录更新信息
                        newValueItem->setText("");
                        updateMessages += QString("%1 修改至 %2\n").arg(descriptionItem->text()).arg(newValue);
                    }
                    else {
                        // 可以在这里处理写入失败的情况
                        updateMessages += QString("%1 更新失败\n").arg(descriptionItem->text());
                    }
                }

                int currentValue = 0;
                ReadProcessMemory(hProcess, (LPCVOID)finalAddress, &currentValue, sizeof(currentValue), nullptr);

                QStandardItem* currentValueItem = goldMSPParent->child(i, 1); // 第二列存储当前值
                if (currentValueItem) {
                    currentValueItem->setText(QString::number(currentValue));
                }
            }
        }


        // 获取“所有物品”顶级项
        QStandardItem* allItemsParent = model->item(1);

        if (allItemsParent && allItemsParent->text() == "所有物品") {
            // 遍历“所有物品”下的所有子项
            for (int i = 0; i < items.size(); ++i) {

                DWORD_PTR finalAddress = FindDmaAddy(hProcess, dynamicPtrBaseAddrItem, items[i].finalOffsets);
                
                auto newValueItem = allItemsParent->child(i, 2); // 获取“新值”列
                auto descriptionItem = allItemsParent->child(i, 0); // 获取“描述”列

                if (!newValueItem->text().isEmpty()) {
                    int newValue = newValueItem->text().toInt();

                    // 使用 WriteProcessMemory 写入新值
                    if (WriteProcessMemory(hProcess, (LPVOID)finalAddress, &newValue, sizeof(newValue), NULL)) {
                        // 成功更新后，清空"新值"列并记录更新信息
                        newValueItem->setText("");
                        updateMessages += QString("%1 修改至 %2\n").arg(descriptionItem->text()).arg(newValue);
                    }
                    else {
                        // 可以在这里处理写入失败的情况
                        updateMessages += QString("%1 更新失败\n").arg(descriptionItem->text());
                    }
                }

                int currentValue = 0;
                ReadProcessMemory(hProcess, (LPCVOID)finalAddress, &currentValue, sizeof(currentValue), nullptr);

                QStandardItem* currentValueItem = allItemsParent->child(i, 1); // 第二列存储当前值
                if (currentValueItem) {
                    currentValueItem->setText(QString::number(currentValue));
                }
            }
        }

        CloseHandle(hProcess);

        // 如果有更新信息，显示弹窗
        if (!updateMessages.isEmpty()) {
            QMessageBox::information(this, "更新信息", updateMessages);
        }
    };
};

int main(int argc, char *argv[]) {

    std::string filePath = "item.txt";
    std::vector<LONG64> offsets = { 0x20, 0x10, 0x58 }; // 路径偏移

    LONG64 baseOffset = -0xFC; // 第一个物品的最后一级偏移
    LONG64 offsetIncrement = 0x20; // 每个物品之间的偏移差

    std::vector<ItemDescription> items = readItemsFromFile(filePath, offsets, baseOffset, offsetIncrement);

    std::vector<ItemDescription> goldMSP;
    goldMSP.push_back({ "金币", {0x30}});
    goldMSP.push_back({ "技能点(MSP)", {0x98}});


    QApplication app(argc, argv);
    MyApp window(items, goldMSP);
    window.show();

    return app.exec();
};