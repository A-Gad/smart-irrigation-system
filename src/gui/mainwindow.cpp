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
    resize(900, 700);
}

MainWindow::~MainWindow() 
{
}

void MainWindow::setupUI()
{
    QWidget *centralWidget = new QWidget(this);
    QVBoxLayout *mainLayout = new QVBoxLayout(centralWidget);
    
    // Info labels
    QGroupBox *infoGroup = new QGroupBox("Sensor Readings", this);
    QVBoxLayout *infoMainLayout = new QVBoxLayout(infoGroup);
    
    QHBoxLayout *infoLayout = new QHBoxLayout();
    moistureLabel = new QLabel("Moisture: -- ", this);
    tempLabel = new QLabel("Temperature: -- °C", this);
    humidityLabel = new QLabel("Humidity: -- %", this);
    
    infoLayout->addWidget(moistureLabel);
    infoLayout->addWidget(tempLabel);
    infoLayout->addWidget(humidityLabel);
    
    rainStatusLabel = new QLabel("Rain Status: No Rain", this);
    rainStatusLabel->setStyleSheet("QLabel { font-weight: bold; color: #666; }");
    
    infoMainLayout->addLayout(infoLayout);
    infoMainLayout->addWidget(rainStatusLabel);
    
    // Control buttons
    QGroupBox *controlGroup = new QGroupBox("Simulation Controls", this);
    QHBoxLayout *buttonLayout = new QHBoxLayout(controlGroup);
    
    startButton = new QPushButton("Start Simulation", this);
    stopButton = new QPushButton("Stop Simulation", this);
    
    connect(startButton, &QPushButton::clicked, 
            appController, &AppController::startSimulation);
    connect(stopButton, &QPushButton::clicked, 
            appController, &AppController::stopSimulation);
    
    buttonLayout->addWidget(startButton);
    buttonLayout->addWidget(stopButton);
    
    // Rain simulation controls
    QGroupBox *rainGroup = new QGroupBox("Rain Simulation", this);
    QHBoxLayout *rainLayout = new QHBoxLayout(rainGroup);
    
    QFormLayout *rainParamsLayout = new QFormLayout();
    
    rainDurationSpinBox = new QSpinBox(this);
    rainDurationSpinBox->setRange(1, 3600); // 1 second to 1 hour
    rainDurationSpinBox->setValue(60); // Default 60 seconds
    rainDurationSpinBox->setSuffix(" sec");
    rainDurationSpinBox->setToolTip("Duration of rain in seconds");
    
    rainIntensitySpinBox = new QDoubleSpinBox(this);
    rainIntensitySpinBox->setRange(1.0, 50.0); // Light to heavy rain
    rainIntensitySpinBox->setValue(15.0); // Default moderate rain
    rainIntensitySpinBox->setSuffix(" mm/h");
    rainIntensitySpinBox->setDecimals(1);
    rainIntensitySpinBox->setSingleStep(1.0);
    rainIntensitySpinBox->setToolTip("Rain intensity (5-10: light, 10-20: moderate, 20+: heavy)");
    
    rainParamsLayout->addRow("Duration:", rainDurationSpinBox);
    rainParamsLayout->addRow("Intensity:", rainIntensitySpinBox);
    
    rainButton = new QPushButton("Simulate Rain", this);
    rainButton->setStyleSheet("QPushButton { background-color: #4A90E2; color: white; font-weight: bold; padding: 8px; }");
    connect(rainButton, &QPushButton::clicked,
            this, &MainWindow::onSimulateRainClicked);
    
    rainLayout->addLayout(rainParamsLayout);
    rainLayout->addWidget(rainButton);
    
    mainLayout->addWidget(infoGroup);
    mainLayout->addWidget(controlGroup);
    mainLayout->addWidget(rainGroup);
    
    setCentralWidget(centralWidget);
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
    chartView = new QChartView(chart, this);
    chartView->setRenderHint(QPainter::Antialiasing);
    
    // Add to layout
    QVBoxLayout *layout = qobject_cast<QVBoxLayout*>(centralWidget()->layout());
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
    moistureLabel->setText(QString("Moisture: %1")
                          .arg(appController->getCurrentMoisture(), 0, 'f', 1));
    tempLabel->setText(QString("Temperature: %1 °C")
                      .arg(appController->getCurrentTemp(), 0, 'f', 1));
    humidityLabel->setText(QString("Humidity: %1 %")
                          .arg(appController->getCurrentHumidity(), 0, 'f', 1));
}

void MainWindow::onRainDetected()
{
    rainStatusLabel->setText("Rain Status: 🌧️ RAINING");
    rainStatusLabel->setStyleSheet("QLabel { font-weight: bold; color: #2E86DE; }");
}

void MainWindow::onSimulateRainClicked()
{
    int duration = rainDurationSpinBox->value();
    double intensity = rainIntensitySpinBox->value();
    
    // Call the simulator's deduceRain method through the controller
    if (appController && appController->getSimulator()) {
        appController->getSimulator()->deduceRain(duration, intensity);
        
        // Update UI to show rain started
        rainStatusLabel->setText(QString("Rain Status: 🌧️ RAINING (%1s, %2 mm/h)")
                                .arg(duration)
                                .arg(intensity, 0, 'f', 1));
        rainStatusLabel->setStyleSheet("QLabel { font-weight: bold; color: #2E86DE; }");
        
        // Reset status after rain duration (for visual feedback)
        QTimer::singleShot(duration * 1000, this, [this]() {
            rainStatusLabel->setText("Rain Status: No Rain");
            rainStatusLabel->setStyleSheet("QLabel { font-weight: bold; color: #666; }");
        });
    }
}