#ifndef HODWINDOW_H
#define HODWINDOW_H

#include "Database.h"
#include <QWidget>

class QTabWidget;
class QTableWidget;

class HodWindow : public QWidget
{
public:
    explicit HodWindow(Database& database, int hodId,
                       QWidget* parent = nullptr);

    void selectTab(int index);

private:
    Database& database;
    int hodId;
    QTabWidget* tabs;
    QTableWidget* coursesTable;
    QTableWidget* marksTable;
    QTableWidget* releaseTable;

    void setupUi();
    void loadCourses();
    void loadMarks();
    void loadReleaseQueue();
    void moderateSelectedMark(const std::string& decision);
    void releaseApprovedMarks();
    int selectedMarkId() const;
};

#endif
