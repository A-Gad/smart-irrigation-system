#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QChartView>
#include <QLineSeries>
#include <QValueAxis>
#include <QChart>
#include <QVBoxLayout>
#include <QWidget>
#include <QLabel>
#include <QPushButton>
#include <QSpinBox>
#include <QDoubleSpinBox>

class AppController;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(AppController *controller, QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void updateMoistureChart(double value);
    void updateLabels();
    void onRainDetected();
    void onSimulateRainClicked();

private:
    void setupUI();
    void setupChart();

    AppController *appController;
    
    QChartView *chartView;
    QChart *chart;
    QLineSeries *moistureSeries;
    QValueAxis *axisX;
    QValueAxis *axisY;
    
    QLabel *moistureLabel;
    QLabel *tempLabel;
    QLabel *humidityLabel;
    QLabel *rainStatusLabel;
    
    QPushButton *startButton;
    QPushButton *stopButton;
    QPushButton *rainButton;
    
    QSpinBox *rainDurationSpinBox;
    QDoubleSpinBox *rainIntensitySpinBox;
    
    int dataPointCounter;
    const int maxDataPoints = 100;
};

#endif // MAINWINDOW_H