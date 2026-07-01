#pragma once

#include <QLabel>
#include <QMainWindow>

namespace geosensor::ui
{

class MainWindow : public QMainWindow
{
public:
    explicit MainWindow(QWidget* parent = nullptr);

private:
    void setupUi();

    QLabel* titleLabel_ {};
};

} // namespace geosensor::ui
