#include "mainwindow.h"
#include "app_controller.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QFormLayout>

MainWindow::MainWindow(AppController *controller, QWidget *parent)
    : QMainWindow(parent),
      appController(controller),
      chartView(nullptr),
      chart(nullptr),
      moistureSeries(nullptr),
      axisX(nullptr),
      axisY(nullptr),
      pumpOn(false),
      dataPointCounter(0)
{
    setupUI();
    setupChart();
    
    // Connect controller signals
    connect(appController, &AppController::moistureUpdated,
            this, &MainWindow::updateMoistureChart);
    connect(appController, &AppController::tempUpdated,
            this, &MainWindow::updateLabels);
    connect(appController, &AppController::humidityUpdated,
            this, &MainWindow::updateLabels);
    connect(appController, &AppController::rainDetected,
            this, &MainWindow::onRainDetected);
     
    setWindowTitle("Smart Irrigation System");
    resize(1000, 700);
}

MainWindow::~MainWindow() 
{
}

void MainWindow::setupUI()
{
    // Create central widget with tab widget
    QWidget *centralWidget = new QWidget(this);
    QVBoxLayout *mainLayout = new QVBoxLayout(centralWidget);
    
    // Create tab widget
    tabWidget = new QTabWidget(this);
    tabWidget->setTabPosition(QTabWidget::North);
    
    // Setup individual tabs
    setupMonitoringTab();
    setupControlsTab();
    
    mainLayout->addWidget(tabWidget);
    setCentralWidget(centralWidget);
}

void MainWindow::setupMonitoringTab()
{
    // Create monitoring tab widget
    QWidget *monitoringTab = new QWidget();
    QVBoxLayout *monitoringLayout = new QVBoxLayout(monitoringTab);
    
    // Sensor readings group
    QGroupBox *infoGroup = new QGroupBox("Current Sensor Readings", monitoringTab);
    QVBoxLayout *infoMainLayout = new QVBoxLayout(infoGroup);
    
    QHBoxLayout *infoLayout = new QHBoxLayout();
    moistureLabel = new QLabel("Moisture: -- ", monitoringTab);
    tempLabel = new QLabel("Temperature: -- °C", monitoringTab);
    humidityLabel = new QLabel("Humidity: -- %", monitoringTab);
    
    moistureLabel->setStyleSheet("QLabel { font-size: 14px; padding: 5px; }");
    tempLabel->setStyleSheet("QLabel { font-size: 14px; padding: 5px; }");
    humidityLabel->setStyleSheet("QLabel { font-size: 14px; padding: 5px; }");
    
    infoLayout->addWidget(moistureLabel);
    infoLayout->addWidget(tempLabel);
    infoLayout->addWidget(humidityLabel);
    
    // Status indicators
    QHBoxLayout *statusLayout = new QHBoxLayout();
    
    pumpStatusLabel = new QLabel("Pump: OFF", monitoringTab);
    pumpStatusLabel->setStyleSheet("QLabel { font-weight: bold; font-size: 14px; color: #E74C3C; padding: 5px; }");
    
    rainStatusLabel = new QLabel("Rain Status: No Rain", monitoringTab);
    rainStatusLabel->setStyleSheet("QLabel { font-weight: bold; font-size: 14px; color: #666; padding: 5px; }");
    
    statusLayout->addWidget(pumpStatusLabel);
    statusLayout->addWidget(rainStatusLabel);
    
    infoMainLayout->addLayout(infoLayout);
    infoMainLayout->addLayout(statusLayout);
    
    monitoringLayout->addWidget(infoGroup);
    
    // Add to tab widget
    tabWidget->addTab(monitoringTab, "📊 Monitoring");
}

void MainWindow::setupControlsTab()
{
    // Create controls tab widget
    QWidget *controlsTab = new QWidget();
    QVBoxLayout *controlsLayout = new QVBoxLayout(controlsTab);
    
    // Simulation controls group
    QGroupBox *controlGroup = new QGroupBox("Simulation Controls", controlsTab);
    QHBoxLayout *buttonLayout = new QHBoxLayout(controlGroup);
    
    startButton = new QPushButton("▶ Start Simulation", controlsTab);
    stopButton = new QPushButton("⏸ Stop Simulation", controlsTab);
    
    startButton->setStyleSheet(
        "QPushButton { background-color: #27AE60; color: white; "
        "font-weight: bold; padding: 10px; font-size: 14px; }"
        "QPushButton:hover { background-color: #229954; }"
    );
    stopButton->setStyleSheet(
        "QPushButton { background-color: #E74C3C; color: white; "
        "font-weight: bold; padding: 10px; font-size: 14px; }"
        "QPushButton:hover { background-color: #C0392B; }"
    );
    
    connect(startButton, &QPushButton::clicked, 
            appController, &AppController::startSimulation);
    connect(stopButton, &QPushButton::clicked, 
            appController, &AppController::stopSimulation);
    
    buttonLayout->addWidget(startButton);
    buttonLayout->addWidget(stopButton);
    
    // Pump control group
    QGroupBox *pumpControl = new QGroupBox("💧 Pump Control", controlsTab);
    QVBoxLayout *pumpMainLayout = new QVBoxLayout(pumpControl);
    
    QLabel *pumpDescription = new QLabel(
        "Control the irrigation pump to add water to the soil. "
        "The pump efficiency depends on current soil saturation.",
        controlsTab
    );
    pumpDescription->setWordWrap(true);
    pumpDescription->setStyleSheet("QLabel { color: #555; margin-bottom: 10px; }");
    
    QHBoxLayout *pumpLayout = new QHBoxLayout();
    
    pumpOnButton = new QPushButton("🚿 Start Pump", controlsTab);
    pumpOnButton->setStyleSheet(
        "QPushButton { background-color: #27AE60; color: white; "
        "font-weight: bold; padding: 10px; font-size: 14px; }"
        "QPushButton:hover { background-color: #229954; }"
        "QPushButton:disabled { background-color: #95A5A6; }"
    );
    
    pumpOffButton = new QPushButton("⏹ Stop Pump", controlsTab);
    pumpOffButton->setStyleSheet(
        "QPushButton { background-color: #E74C3C; color: white; "
        "font-weight: bold; padding: 10px; font-size: 14px; }"
        "QPushButton:hover { background-color: #C0392B; }"
        "QPushButton:disabled { background-color: #95A5A6; }"
    );
    pumpOffButton->setEnabled(false);
    
    connect(pumpOnButton, &QPushButton::clicked,
            this, &MainWindow::onPumpStartClicked);
    connect(pumpOffButton, &QPushButton::clicked,
            this, &MainWindow::onPumpStopClicked);
    
    pumpLayout->addWidget(pumpOnButton);
    pumpLayout->addWidget(pumpOffButton);
    
    pumpMainLayout->addWidget(pumpDescription);
    pumpMainLayout->addLayout(pumpLayout);
    
    // Rain simulation group
    QGroupBox *rainGroup = new QGroupBox("🌧️ Rain Simulation", controlsTab);
    QVBoxLayout *rainMainLayout = new QVBoxLayout(rainGroup);
    
    QLabel *rainDescription = new QLabel(
        "Simulate rain events with custom duration and intensity. "
        "Rain intensity: 5-10 mm/h (light), 10-20 mm/h (moderate), 20+ mm/h (heavy).",
        controlsTab
    );
    rainDescription->setWordWrap(true);
    rainDescription->setStyleSheet("QLabel { color: #555; margin-bottom: 10px; }");
    
    QHBoxLayout *rainLayout = new QHBoxLayout();
    
    QFormLayout *rainParamsLayout = new QFormLayout();
    
    rainDurationSpinBox = new QSpinBox(controlsTab);
    rainDurationSpinBox->setRange(1, 3600);
    rainDurationSpinBox->setValue(60);
    rainDurationSpinBox->setSuffix(" sec");
    rainDurationSpinBox->setToolTip("Duration of rain in seconds");
    rainDurationSpinBox->setMinimumWidth(150);
    
    rainIntensitySpinBox = new QDoubleSpinBox(controlsTab);
    rainIntensitySpinBox->setRange(1.0, 50.0);
    rainIntensitySpinBox->setValue(15.0);
    rainIntensitySpinBox->setSuffix(" mm/h");
    rainIntensitySpinBox->setDecimals(1);
    rainIntensitySpinBox->setSingleStep(1.0);
    rainIntensitySpinBox->setToolTip("Rain intensity");
    rainIntensitySpinBox->setMinimumWidth(150);
    
    rainParamsLayout->addRow("Duration:", rainDurationSpinBox);
    rainParamsLayout->addRow("Intensity:", rainIntensitySpinBox);
    
    rainButton = new QPushButton("🌧️ Simulate Rain", controlsTab);
    rainButton->setStyleSheet(
        "QPushButton { background-color: #3498DB; color: white; "
        "font-weight: bold; padding: 10px; font-size: 14px; }"
        "QPushButton:hover { background-color: #2980B9; }"
    );
    connect(rainButton, &QPushButton::clicked,
            this, &MainWindow::onSimulateRainClicked);
    
    rainLayout->addLayout(rainParamsLayout);
    rainLayout->addWidget(rainButton);
    
    rainMainLayout->addWidget(rainDescription);
    rainMainLayout->addLayout(rainLayout);
    
    // Add all groups to controls layout
    controlsLayout->addWidget(controlGroup);
    controlsLayout->addWidget(pumpControl);
    controlsLayout->addWidget(rainGroup);
    controlsLayout->addStretch();  // Push everything to top
    
    // Add to tab widget
    tabWidget->addTab(controlsTab, "⚙️ Controls");
}
void MainWindow::setupSettingsTab()
{
    QWidget* settingsTab = new QWidget(this);
    QVBoxLayout *monitoringLayout = new QVBoxLayout(settingsTab);

    tabWidget->addTab(settingsTab, "Settings");

}

void MainWindow::setupHistoryTab()
{
    QWidget* hsitoryTab = new QWidget(this);
    QVBoxLayout *monitoringLayout = new QVBoxLayout(hsitoryTab);
    
    tabWidget->addTab(hsitoryTab, "History");
}

void MainWindow::setupChart()
{
    // Create chart
    chart = new QChart();
    chart->setTitle("Soil Moisture Over Time");
    chart->setAnimationOptions(QChart::NoAnimation);
    
    // Create series
    moistureSeries = new QLineSeries();
    moistureSeries->setName("Moisture Level");
    chart->addSeries(moistureSeries);
    
    // Create axes
    axisX = new QValueAxis();
    axisX->setTitleText("Time (data points)");
    axisX->setRange(0, maxDataPoints);
    
    axisY = new QValueAxis();
    axisY->setTitleText("Moisture Level");
    axisY->setRange(200, 800);
    
    chart->addAxis(axisX, Qt::AlignBottom);
    chart->addAxis(axisY, Qt::AlignLeft);
    moistureSeries->attachAxis(axisX);
    moistureSeries->attachAxis(axisY);
    
    // Create chart view
    chartView = new QChartView(chart);
    chartView->setRenderHint(QPainter::Antialiasing);
    
    // Add chart to monitoring tab
    QWidget *monitoringTab = tabWidget->widget(0);  // Get first tab
    QVBoxLayout *layout = qobject_cast<QVBoxLayout*>(monitoringTab->layout());
    if (layout) {
        layout->addWidget(chartView);
    }
}

void MainWindow::updateMoistureChart(double value)
{
    moistureSeries->append(dataPointCounter, value);
    dataPointCounter++;
    
    // Keep only recent data points
    if (moistureSeries->count() > maxDataPoints) {
        moistureSeries->remove(0);
        
        // Adjust X axis to show sliding window
        axisX->setRange(dataPointCounter - maxDataPoints, dataPointCounter);
    }
    
    updateLabels();
}

void MainWindow::updateLabels()
{
    moistureLabel->setText(QString("💧 Moisture: %1")
                          .arg(appController->getCurrentMoisture(), 0, 'f', 1));
    tempLabel->setText(QString("🌡️ Temperature: %1 °C")
                      .arg(appController->getCurrentTemp(), 0, 'f', 1));
    humidityLabel->setText(QString("💨 Humidity: %1 %")
                          .arg(appController->getCurrentHumidity(), 0, 'f', 1));
}

void MainWindow::onRainDetected()
{
    rainStatusLabel->setText("Rain Status: 🌧️ RAINING");
    rainStatusLabel->setStyleSheet("QLabel { font-weight: bold; font-size: 14px; color: #3498DB; padding: 5px; }");
}

void MainWindow::onSimulateRainClicked()
{
    int duration = rainDurationSpinBox->value();
    double intensity = rainIntensitySpinBox->value();
    
    if (appController && appController->getSimulator()) {
        appController->getSimulator()->deduceRain(duration, intensity);
        
        rainStatusLabel->setText(QString("Rain Status: 🌧️ RAINING (%1s, %2 mm/h)")
                                .arg(duration)
                                .arg(intensity, 0, 'f', 1));
        rainStatusLabel->setStyleSheet("QLabel { font-weight: bold; font-size: 14px; color: #3498DB; padding: 5px; }");
        
        QTimer::singleShot(duration * 1000, this, [this]() {
            rainStatusLabel->setText("Rain Status: No Rain");
            rainStatusLabel->setStyleSheet("QLabel { font-weight: bold; font-size: 14px; color: #666; padding: 5px; }");
        });
    }
}

void MainWindow::onPumpStartClicked()
{
    pumpOn = true;
    
    pumpStatusLabel->setText("Pump: 🚿 ON");
    pumpStatusLabel->setStyleSheet("QLabel { font-weight: bold; font-size: 14px; color: #27AE60; padding: 5px; }");
    
    pumpOnButton->setEnabled(false);
    pumpOffButton->setEnabled(true);
    
    if (appController && appController->getSimulator()) {
        appController->getSimulator()->setPumpRunning(true);
    }
}

void MainWindow::onPumpStopClicked()
{
    pumpOn = false;
    
    pumpStatusLabel->setText("Pump: OFF");
    pumpStatusLabel->setStyleSheet("QLabel { font-weight: bold; font-size: 14px; color: #E74C3C; padding: 5px; }");
    
    pumpOnButton->setEnabled(true);
    pumpOffButton->setEnabled(false);
    
    if (appController && appController->getSimulator()) {
        appController->getSimulator()->setPumpRunning(false);
    }
}