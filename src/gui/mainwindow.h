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
#include <QTabWidget>

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
    void onPumpStartClicked();
    void onPumpStopClicked();

private:
    void setupUI();
    void setupMonitoringTab();
    void setupControlsTab();
    void setupSettingsTab();
    void setupHistoryTab();
    void setupChart();

    AppController *appController;
    
    // Tab widget
    QTabWidget *tabWidget;
    
    // Monitoring tab widgets
    QChartView *chartView;
    QChart *chart;
    QLineSeries *moistureSeries;
    QValueAxis *axisX;
    QValueAxis *axisY;
    
    QLabel *moistureLabel;
    QLabel *tempLabel;
    QLabel *humidityLabel;
    QLabel *rainStatusLabel;
    QLabel *pumpStatusLabel;
    
    // Controls tab widgets
    QPushButton *startButton;
    QPushButton *stopButton;
    QPushButton *rainButton;
    QPushButton *pumpOnButton;
    QPushButton *pumpOffButton;
    
    QSpinBox *rainDurationSpinBox;
    QDoubleSpinBox *rainIntensitySpinBox;
    
    bool pumpOn;
    
    int dataPointCounter;
    const int maxDataPoints = 100;
};

#endif // MAINWINDOW_H