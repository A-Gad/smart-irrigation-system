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
#include <QLineEdit>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QTabWidget>
#include <QTimer> // Added for QTimer::singleShot

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
    void updateRainStatus(bool isRaining);
    void onRainDetected();
    void onSimulateRainClicked();
    void onPumpStartClicked();
    void onPumpStopClicked();
    void onConnectClicked();

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
    
    // Settings tab widgets
    QLineEdit *ipInput;
    QSpinBox *portInput;
    QPushButton *connectButton;
    QLabel *connectionStatusLabel;
    
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